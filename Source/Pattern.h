#pragma once

#include <array>
#include <atomic>
#include <cmath>
#include <random>
#include <juce_core/juce_core.h>

namespace tg
{
constexpr int kMaxSteps    = 32;
constexpr int kNumPatterns = 8;
constexpr int kMaxRatchet  = 4;

//==============================================================================
// Step resolutions (length of one step in quarter notes)
struct ResolutionInfo { const char* name; double beats; };

inline const std::array<ResolutionInfo, 10>& resolutions()
{
    static const std::array<ResolutionInfo, 10> r {{
        { "1/4",   1.0 },
        { "1/8",   0.5 },
        { "1/8T",  1.0 / 3.0 },
        { "1/8D",  0.75 },
        { "1/16",  0.25 },
        { "1/16T", 1.0 / 6.0 },
        { "1/16D", 0.375 },
        { "1/32",  0.125 },
        { "1/32T", 1.0 / 12.0 },
        { "1/64",  0.0625 },
    }};
    return r;
}

enum ResIndex { R4 = 0, R8, R8T, R8D, R16, R16T, R16D, R32, R32T, R64 };

//==============================================================================
struct Step
{
    float level   = 0.0f;   // 0 = off, (0..1] = gate level
    int   ratchet = 1;      // 1..4 retriggers inside the step
    bool  tie     = false;  // hold gate into next step (legato)
};

struct PatternData
{
    int length     = 16;
    int resolution = R16;
    std::array<Step, kMaxSteps> steps {};

    bool isOn (int i) const noexcept { return steps[(size_t) i].level > 0.0001f; }

    //--------------------------------------------------------------------------
    // Pattern string syntax (spaces/| ignored):
    //   X = 100%   x = 70%   o = 40%   , = 20%   . = off
    //   - = tie (continue previous step, legato)
    //   2 3 4 = ratchet (2/3/4 retriggers inside the step)
    static PatternData fromString (const juce::String& s, int res = R16)
    {
        PatternData p;
        p.resolution = res;
        int n = 0;
        for (auto c : s)
        {
            if (c == ' ' || c == '|' || c == '\n') continue;
            if (n >= kMaxSteps) break;
            auto& st = p.steps[(size_t) n];
            switch (c)
            {
                case 'X': st.level = 1.0f;  break;
                case 'x': st.level = 0.7f;  break;
                case 'o': st.level = 0.4f;  break;
                case ',': st.level = 0.2f;  break;
                case '2': st.level = 1.0f; st.ratchet = 2; break;
                case '3': st.level = 1.0f; st.ratchet = 3; break;
                case '4': st.level = 1.0f; st.ratchet = 4; break;
                case '-':
                    if (n > 0 && p.isOn (n - 1))
                    {
                        p.steps[(size_t) n - 1].tie = true;
                        st.level = p.steps[(size_t) n - 1].level;
                    }
                    break;
                default: st.level = 0.0f; break;
            }
            ++n;
        }
        p.length = juce::jmax (1, n);
        return p;
    }

    juce::String toString() const
    {
        juce::String s;
        for (int i = 0; i < length; ++i)
        {
            const auto& st = steps[(size_t) i];
            const bool tiedIn = i > 0 && steps[(size_t) i - 1].tie && isOn (i);
            if (tiedIn)                 s << '-';
            else if (! isOn (i))        s << '.';
            else if (st.ratchet > 1)    s << juce::String (st.ratchet);
            else if (st.level > 0.85f)  s << 'X';
            else if (st.level > 0.55f)  s << 'x';
            else if (st.level > 0.3f)   s << 'o';
            else                        s << ',';
        }
        return s;
    }

    //--------------------------------------------------------------------------
    // Editing tools
    void shift (int dir)
    {
        std::array<Step, kMaxSteps> tmp = steps;
        for (int i = 0; i < length; ++i)
            steps[(size_t) ((i + dir + length) % length)] = tmp[(size_t) i];
    }

    void reverse()
    {
        for (int i = 0; i < length; ++i) steps[(size_t) i].tie = false;
        std::reverse (steps.begin(), steps.begin() + length);
    }

    void invert()
    {
        for (int i = 0; i < length; ++i)
        {
            auto& st = steps[(size_t) i];
            st.level   = isOn (i) ? 0.0f : 1.0f;
            st.ratchet = 1;
            st.tie     = false;
        }
    }

    void clear()
    {
        for (auto& st : steps) st = {};
    }

    void fill()
    {
        for (int i = 0; i < length; ++i) steps[(size_t) i] = { 1.0f, 1, false };
    }

    void duplicate()   // copy pattern into the second half, double length
    {
        const int newLen = juce::jmin (kMaxSteps, length * 2);
        for (int i = length; i < newLen; ++i)
            steps[(size_t) i] = steps[(size_t) (i - length)];
        length = newLen;
    }

    void halve()
    {
        length = juce::jmax (1, length / 2);
    }

    // Euclidean rhythm: k hits spread over current length, rotated
    void euclid (int hits, int rotation)
    {
        clear();
        hits = juce::jlimit (0, length, hits);
        for (int i = 0; i < length; ++i)
        {
            const bool on = hits > 0 && ((i * hits) % length) < hits;
            const int idx = ((i + rotation) % length + length) % length;
            steps[(size_t) idx].level = on ? 1.0f : 0.0f;
        }
    }

    enum class RandomMode { Sparse, Medium, Dense, Rage, Mutate, Accents };

    void randomise (RandomMode mode, std::mt19937& rng)
    {
        std::uniform_real_distribution<float> u (0.0f, 1.0f);

        if (mode == RandomMode::Mutate)
        {
            for (int i = 0; i < length; ++i)
                if (u (rng) < 0.25f)
                {
                    auto& st = steps[(size_t) i];
                    st.level = isOn (i) ? 0.0f : 1.0f;
                    st.tie = false;
                    if (st.level > 0 && u (rng) < 0.15f) st.ratchet = 2;
                }
            return;
        }

        if (mode == RandomMode::Accents)
        {
            for (int i = 0; i < length; ++i)
                if (isOn (i))
                {
                    const float r = u (rng);
                    steps[(size_t) i].level = r < 0.4f ? 1.0f : (r < 0.75f ? 0.7f : 0.4f);
                }
            return;
        }

        clear();
        const float density = mode == RandomMode::Sparse ? 0.3f
                            : mode == RandomMode::Medium ? 0.55f
                            : mode == RandomMode::Dense  ? 0.8f
                                                         : 0.5f;
        for (int i = 0; i < length; ++i)
        {
            // downbeats are more likely to be on
            const float bias = (i % 4 == 0) ? 0.25f : 0.0f;
            if (u (rng) < density + bias)
            {
                auto& st = steps[(size_t) i];
                st.level = 1.0f;
                if (mode == RandomMode::Rage)
                {
                    const float r = u (rng);
                    if (r < 0.18f)      st.ratchet = 2;
                    else if (r < 0.26f) st.ratchet = 3;
                    else if (r < 0.31f) st.ratchet = 4;
                    else if (r < 0.50f) st.tie = true;
                    if (u (rng) < 0.25f) st.level = 0.7f;
                }
            }
        }
        // a tie into an off step means nothing - clean it up
        for (int i = 0; i < length; ++i)
            if (steps[(size_t) i].tie && ! isOn ((i + 1) % length))
                steps[(size_t) i].tie = false;
    }
};

//==============================================================================
// Lock-free storage shared between GUI thread and audio thread.
class PatternSlot
{
public:
    PatternSlot() { set (PatternData::fromString ("X.X.X.X.X.X.X.X.")); }

    PatternData get() const noexcept
    {
        PatternData p;
        p.length     = length.load (std::memory_order_relaxed);
        p.resolution = resolution.load (std::memory_order_relaxed);
        for (size_t i = 0; i < kMaxSteps; ++i)
        {
            p.steps[i].level   = level[i].load (std::memory_order_relaxed);
            p.steps[i].ratchet = ratchet[i].load (std::memory_order_relaxed);
            p.steps[i].tie     = tie[i].load (std::memory_order_relaxed);
        }
        return p;
    }

    void set (const PatternData& p) noexcept
    {
        for (size_t i = 0; i < kMaxSteps; ++i)
        {
            level[i].store   (juce::jlimit (0.0f, 1.0f, p.steps[i].level), std::memory_order_relaxed);
            ratchet[i].store (juce::jlimit (1, kMaxRatchet, p.steps[i].ratchet), std::memory_order_relaxed);
            tie[i].store     (p.steps[i].tie, std::memory_order_relaxed);
        }
        resolution.store (juce::jlimit (0, (int) resolutions().size() - 1, p.resolution), std::memory_order_relaxed);
        length.store     (juce::jlimit (1, kMaxSteps, p.length), std::memory_order_relaxed);
        version.fetch_add (1);
    }

    int getVersion() const noexcept { return version.load(); }

private:
    std::atomic<int> length { 16 }, resolution { R16 }, version { 0 };
    std::array<std::atomic<float>, kMaxSteps> level {};
    std::array<std::atomic<int>,   kMaxSteps> ratchet {};
    std::array<std::atomic<bool>,  kMaxSteps> tie {};
};

} // namespace tg
