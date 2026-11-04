#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cstring>
#include "PluginProcessor.h"
#include "Theme.h"

namespace tg
{
//==============================================================================
// Step editor: level bars, ratchet lane, tie lane, gate curve and the live signal view
class StepGrid : public juce::Component, public juce::TooltipClient
{
public:
    explicit StepGrid (TranceGateProcessor& p) : proc (p) {}

    void refresh();                     // called by the editor timer (30 Hz)
    bool showSignal = true;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    juce::String getTooltip() override;

private:
    PatternData current() const { return proc.currentPattern(); }
    void commit (const PatternData& d, bool newUndoPoint);

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
    float levelAt (float y) const { return juce::jlimit (0.05f, 1.0f, 1.0f - (y - bars.getY()) / bars.getHeight()); }

    void computeCurve();

    TranceGateProcessor& proc;
    PatternData pattern;
    std::vector<float> curve;                  // gate amount over one loop (0..1, mix / depth applied)
    std::array<float, TranceGateProcessor::kSignalBins> inShown {}, outShown {};
    float signalNorm = 0.0f;
    bool hasSignal = false;

    juce::Rectangle<float> bars, ratchetLane, tieLane, legend, signalButton;

    enum class Drag { None, Draw, Erase, Pending, Level };
    Drag drag = Drag::None;
    int dragStartCol = -1, lastDragCol = -1;

    int lastVersion = -1, lastStep = -2;
    GateParams lastParams;
    float lastMix = -1.0f, lastDepth = -1.0f;
    int lastMode = -1;
    double lastBpmSeen = 0.0;
};

} // namespace tg
