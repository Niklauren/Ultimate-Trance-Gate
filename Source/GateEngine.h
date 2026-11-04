#pragma once

#include "Pattern.h"
#include <cstdint>
#include <limits>

namespace tg
{
struct GateParams
{
    float attackMs  = 1.0f;
    float decayMs   = 150.0f;
    float sustain   = 1.0f;
    float releaseMs = 25.0f;
    float gate      = 0.75f;  // fraction of a (sub)step the gate stays open
    float swing     = 0.0f;   // 0..1  (1 = second 16th pushed to the triplet-ish 75%)
};

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
        stage = Idle;
        lastAbs = std::numeric_limits<int64_t>::min();
        lastSub = -1;
    }

    void setParams (const GateParams& p) noexcept
    {
        params = p;
        const double msToSamples = sr * 0.001;
        attackInc   = (float) (1.0 / juce::jmax (1.0, p.attackMs * msToSamples));
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

    float getValue() const noexcept { return value; }

private:
    enum Stage { Idle, Attack, Decay, Release };

    static float coefFor (double samples) noexcept
    {
        // reaches ~1% of the distance after 'samples'
        return (float) std::exp (-4.6 / juce::jmax (1.0, samples));
    }

    void trigger (float level) noexcept { peak = level; stage = Attack; }

    void glideTo (float level) noexcept
    {
        peak = level;
        stage = value < peak ? Attack : Decay;
    }

    void release() noexcept { if (stage != Idle) stage = Release; }

    float tick() noexcept
    {
        switch (stage)
        {
            case Attack:
                value += attackInc;
                if (value >= peak) { value = peak; stage = Decay; }
                break;
            case Decay:
            {
                const float target = peak * params.sustain;
                value = target + (value - target) * decayCoef;
                break;
            }
            case Release:
                value *= releaseCoef;
                if (value < 1.0e-5f) { value = 0.0f; stage = Idle; }
                break;
            case Idle:
            default:
                value = 0.0f;
                break;
        }
        return value;
    }

    double sr = 44100.0;
    GateParams params;
    float value = 0.0f, peak = 1.0f;
    float attackInc = 0.01f, decayCoef = 0.999f, releaseCoef = 0.99f;
    Stage stage = Idle;
    int64_t lastAbs = std::numeric_limits<int64_t>::min();
    int lastSub = -1;
};

} // namespace tg
