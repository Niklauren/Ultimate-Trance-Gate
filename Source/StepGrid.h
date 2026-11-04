#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cstring>
#include "PluginProcessor.h"

namespace tg
{
namespace colours
{
    const juce::Colour bg        { 0xff0e0f15 };
    const juce::Colour panel     { 0xff161822 };
    const juce::Colour panelHi   { 0xff1e2130 };
    const juce::Colour outline   { 0xff2a2e40 };
    const juce::Colour text      { 0xffd8dbe8 };
    const juce::Colour textDim   { 0xff7c8199 };
    const juce::Colour accentA   { 0xff2de2e6 };   // cyan
    const juce::Colour accentB   { 0xffff2fa8 };   // magenta
    const juce::Colour accentC   { 0xff9b5cff };   // violet
}

//==============================================================================
class StepGrid : public juce::Component, public juce::TooltipClient
{
public:
    explicit StepGrid (TranceGateProcessor& p) : proc (p) {}

    // called from the editor timer
    void refresh()
    {
        const int slot = proc.activePattern.load();
        const int ver  = proc.patterns[(size_t) slot].getVersion();
        const auto gp  = proc.getGateParams();
        const int step = proc.currentStep.load();
        const double bpm = proc.lastBpm.load();

        const bool patternChanged = slot != lastSlot || ver != lastVersion;
        const bool paramsChanged  = std::memcmp (&gp, &lastParams, sizeof (GateParams)) != 0 || std::abs (bpm - lastBpmSeen) > 1.0e-6;

        if (patternChanged || paramsChanged || curve.empty())
        {
            lastSlot = slot; lastVersion = ver; lastParams = gp; lastBpmSeen = bpm;
            pattern = proc.patterns[(size_t) slot].get();
            computeCurve();
            repaint();
        }
        if (step != lastStep)
        {
            lastStep = step;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    juce::String getTooltip() override;

private:
    PatternData current() const { return proc.patterns[(size_t) proc.activePattern.load()].get(); }
    void commit (const PatternData& d)
    {
        proc.patterns[(size_t) proc.activePattern.load()].set (d);
        refresh();
    }

    int columnAt (float x) const
    {
        const float w = bars.getWidth() / (float) pattern.length;
        return juce::jlimit (0, pattern.length - 1, (int) ((x - bars.getX()) / w));
    }
    juce::Rectangle<float> column (const juce::Rectangle<float>& lane, int i) const
    {
        const float w = lane.getWidth() / (float) pattern.length;
        return { lane.getX() + w * (float) i, lane.getY(), w, lane.getHeight() };
    }
    float levelAt (float y) const
    {
        return juce::jlimit (0.05f, 1.0f, 1.0f - (y - bars.getY()) / bars.getHeight());
    }

    void computeCurve();

    TranceGateProcessor& proc;
    PatternData pattern;
    std::vector<float> curve;

    juce::Rectangle<float> bars, ratchetLane, tieLane, labels;

    enum class Drag { None, Draw, Erase, Pending, Level };
    Drag drag = Drag::None;
    int dragStartCol = -1, lastDragCol = -1;
    float dragLevel = 1.0f;

    int lastSlot = -1, lastVersion = -1, lastStep = -2;
    GateParams lastParams;
    double lastBpmSeen = 0.0;
};

} // namespace tg
