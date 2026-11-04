#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "StepGrid.h"
#include "UiParts.h"
#include "PresetBrowser.h"
#include "Settings.h"

//==============================================================================
class TranceGateEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TranceGateEditor (TranceGateProcessor&);
    ~TranceGateEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

    // used by the offline test to show the menu / filter panel in screenshots
    void showPresetBrowser (bool show);
    void showFilterPanel (bool show) { filterPanelOpen = show; syncControls(); }
    bool keyPressed (const juce::KeyPress&) override;

private:
    void timerCallback() override;
    void syncControls();
    tg::PatternData edit() const { return proc.currentPattern(); }
    void commit (const tg::PatternData& p) { proc.editPattern (p, true); syncControls(); grid.refresh(); }
    void showRandomMenu();
    void applyEuclid();
    juce::Rectangle<int> varGroupBounds() const;

    struct Dismiss : public juce::Component
    {
        std::function<void()> onClick;
        void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
    };

    struct FilterPanel : public juce::Component
    {
        FilterPanel (juce::AudioProcessorValueTreeState& s);
        void paint (juce::Graphics&) override;
        void resized() override;
        juce::String title;
        tg::ui::KnobItem cutoff, reso;
        tg::ui::IconButton close;
    };

    TranceGateProcessor& proc;
    tg::ui::Look look;
    juce::SharedResourcePointer<tg::Settings> settings;
    juce::TooltipWindow tooltips { this, 2000 };

    juce::HyperlinkButton credit;
    tg::ui::IconButton prevPreset, nextPreset;
    tg::ui::PresetDisplay presetDisplay;
    std::array<juce::TextButton, TranceGateProcessor::kNumVariations> varButtons;

    tg::ui::IconButton undoButton, redoButton;
    tg::ui::Stepper steps { "STEPS", 1, tg::kMaxSteps };
    juce::ComboBox rate;
    tg::ui::Stepper hits { "EUCLID", 0, tg::kMaxSteps }, rot { "ROT", 0, tg::kMaxSteps - 1 };
    juce::TextButton randomButton { "RANDOM" };
    std::array<juce::TextButton, 8> tools;
    tg::ui::MidiDragButton midiDrag;

    tg::StepGrid grid;

    std::array<tg::ui::ModeButton, 3> modeButtons;
    std::vector<std::unique_ptr<tg::ui::KnobItem>> knobs;
    FilterPanel filterPanel;
    bool filterPanelOpen = true;

    Dismiss dismiss;
    tg::ui::PresetBrowser browser;

    std::mt19937 rng { std::random_device{}() };
    int lastVersion = -1, lastModeSeen = -1;
    bool syncing = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TranceGateEditor)
};
