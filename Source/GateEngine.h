#pragma once

#include "Pattern.h"
#include <cstdint>
#include <limits>

namespace tg
{
struct GateParams
{
    float attackMs  = 4.0f;
    float decayMs   = 150.0f;
    float sustain   = 0.5f;
    float releaseMs = 40.0f;
    float gate      = 1.0f;   // fraction of a (sub)step the gate stays open
    float swing     = 0.0f;   // 0..1  (1 = second 16th pushed to the triplet-ish 75%)
    int   character = 0;      // Character enum below
};

// Envelope character: the shape of the attack and release curves.
enum Character { CharSmooth = 0, CharHard, CharSoft, CharPump, kNumCharacters };

inline const juce::StringArray& characterNames()
{
    static const juce::StringArray n { "Smooth", "Hard", "Soft", "Pump" };
    return n;
}

//==============================================================================
// Where are we inside the pattern for a given musical position (in quarter notes)?
struct StepPos
{
    int64_t absStep = 0;   // absolute step counter (does not wrap)
    int     index   = 0;   // step index inside the pattern
    int     sub     = 0;   // ratchet sub-step
    int     ratchet = 1;
    double  subFrac = 0.0; // 0..1 inside the sub-step
};

inline StepPos locate (double beat, const PatternData& p, float swing) noexcept
{
    const double stepBeats = resolutions()[(size_t) p.resolution].beats;
    const double pairBeats = stepBeats * 2.0;
    const double split     = stepBeats * (1.0 + 0.5 * (double) swing);

    const double pairIdx = std::floor (beat / pairBeats);
    const double inPair  = beat - pairIdx * pairBeats;

    StepPos sp;
    double frac;
    if (inPair < split) { sp.absStep = (int64_t) pairIdx * 2;     frac = inPair / split; }
    else                { sp.absStep = (int64_t) pairIdx * 2 + 1; frac = (inPair - split) / (pairBeats - split); }

    sp.index = (int) (((sp.absStep % p.length) + p.length) % p.length);

    sp.ratchet = juce::jlimit (1, kMaxRatchet, p.steps[(size_t) sp.index].ratchet);
    const double r = frac * sp.ratchet;
    sp.sub     = juce::jlimit (0, sp.ratchet - 1, (int) r);
    sp.subFrac = r - sp.sub;
    return sp;
}

//==============================================================================
// One gate "voice" (one per audio channel so the right side can be offset).
class GateVoice
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = sampleRate;
        reset();
        setParams (params);
    }

    void reset() noexcept
    {
        value = 0.0f;
        out = 0.0f;
        stage = Idle;
        lastAbs = std::numeric_limits<int64_t>::min();
        lastSub = -1;
    }

    void setParams (const GateParams& p) noexcept
    {
        params = p;
        const double msToSamples = sr * 0.001;
        attackInc   = (float) (1.0 / juce::jmax (1.0, p.attackMs * msToSamples));
        releaseInc  = (float) (1.0 / juce::jmax (1.0, p.releaseMs * msToSamples));
        // one-pole on the final gain: Smooth / Pump round the corners, Soft rounds a lot, Hard keeps them
        const double declickMs = p.character == CharHard ? 0.0 : (p.character == CharSoft ? 2.5 : 0.35);
        declickCoef = declickMs <= 0.0 ? 0.0f : (float) std::exp (-1.0 / (declickMs * msToSamples));
        decayCoef   = coefFor (p.decayMs * msToSamples);
        releaseCoef = coefFor (p.releaseMs * msToSamples);
    }

    // Returns gain 0..1 for the given position (in quarter notes)
    float process (double beat, const PatternData& pat, int* stepOut = nullptr) noexcept
    {
        const auto sp = locate (beat, pat, params.swing);
        const auto& st = pat.steps[(size_t) sp.index];
        const bool on = st.level > 0.0001f;

        if (sp.absStep != lastAbs || sp.sub != lastSub)
        {
            // new step or new ratchet hit
            const bool jumpedOneStep = (sp.absStep == lastAbs + 1);
            const int prevIdx = ((sp.index - 1) % pat.length + pat.length) % pat.length;
            const bool tiedIn = sp.sub == 0 && jumpedOneStep
                                && pat.steps[(size_t) prevIdx].tie && pat.isOn (prevIdx);

            if (! on)          release();
            else if (tiedIn)   glideTo (st.level);
            else               trigger (st.level);

            lastAbs = sp.absStep;
            lastSub = sp.sub;
        }

        if (on)
        {
            const bool holdsIntoNext = st.tie && sp.sub == sp.ratchet - 1;
            if (! holdsIntoNext && sp.subFrac >= (double) params.gate)
                release();
        }

        if (stepOut != nullptr) *stepOut = sp.index;
        return tick();
    }

    float getValue() const noexcept { return out; }

private:
    enum Stage { Idle, Attack, Decay, Release };

    static float coefFor (double samples) noexcept
    {
        // reaches ~1% of the distance after 'samples'
        return (float) std::exp (-4.6 / juce::jmax (1.0, samples));
    }

    // Attack moves from wherever the gain is now to the step level along an S-curve
    // (raised cosine): no corner at the start or the end, so short attacks do not click.
    // It also glides DOWN when a quieter step follows a louder one, instead of jumping.
    void trigger (float level) noexcept
    {
        peak = level;
        from = value;
        phase = 0.0f;
        stage = Attack;
    }

    void glideTo (float level) noexcept
    {
        if (value < level) trigger (level);
        else { peak = level; stage = Decay; }
    }

    void release() noexcept
    {
        if (stage != Idle && stage != Release) { relFrom = value; stage = Release; }
    }

    // attack curve 0..1 -> 0..1 for each character
    float attackShape (float p) const noexcept
    {
        switch (params.character)
        {
            case CharHard: return p;                                                       // straight line, corners at both ends
            case CharSoft: return (1.0f - std::exp (-5.0f * p)) / (1.0f - std::exp (-5.0f)); // fast start, gentle landing (analog VCA)
            case CharPump: return p * p * p;                                               // slow start, sucks in at the end (sidechain feel)
            default:       return 0.5f - 0.5f * std::cos (juce::MathConstants<float>::pi * p); // S-curve, no corners
        }
    }

    float tick() noexcept
    {
        switch (stage)
        {
            case Attack:
                phase += attackInc;
                if (phase >= 1.0f) { value = peak; stage = Decay; }
                else value = from + (peak - from) * attackShape (phase);
                break;
            case Decay:
            {
                const float target = peak * params.sustain;
                value = target + (value - target) * decayCoef;
                break;
            }
            case Release:
                if (params.character == CharHard)
                    value -= relFrom * releaseInc;          // straight line down, hard stop at the release time
                else
                    value *= releaseCoef;                   // natural exponential tail
                if (value < 1.0e-5f) { value = 0.0f; stage = Idle; }
                break;
            case Idle:
            default:
                value = 0.0f;
                break;
        }
        // de-click: rounds the remaining corners (step jumps, release onset) without softening the rhythm
        out = value + (out - value) * declickCoef;
        return out;
    }

    double sr = 44100.0;
    GateParams params;
    float value = 0.0f, peak = 1.0f, from = 0.0f, phase = 0.0f, out = 0.0f, relFrom = 1.0f;
    float attackInc = 0.01f, decayCoef = 0.999f, releaseCoef = 0.99f, releaseInc = 0.01f, declickCoef = 0.9f;
    Stage stage = Idle;
    int64_t lastAbs = std::numeric_limits<int64_t>::min();
    int lastSub = -1;
};

} // namespace tg
