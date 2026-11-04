#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "StepGrid.h"

//==============================================================================
class TGLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TGLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool highlighted, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
    juce::Rectangle<int> getTooltipBounds (const juce::String& tip, juce::Point<int> pos, juce::Rectangle<int> parentArea) override;
    void drawTooltip (juce::Graphics&, const juce::String& text, int width, int height) override;
};

//==============================================================================
class TranceGateEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TranceGateEditor (TranceGateProcessor&);
    ~TranceGateEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    tg::PatternData edit() const;
    void commit (const tg::PatternData&);
    void syncControls();
    void showPresetMenu();
    void showRandomMenu();
    void applyEuclid();

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<Attachment> attachment;
    };

    void setupKnob (Knob&, const juce::String& paramId, const juce::String& name, const juce::String& tip);
    void setupSmallButton (juce::TextButton&, const juce::String& text, const juce::String& tip, std::function<void()> fn);

    TranceGateProcessor& proc;
    TGLookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 2000 };   // tips appear after 2 s hover

    std::array<juce::TextButton, tg::kNumPatterns> slotButtons;
    juce::TextButton presetButton { "PRESETS" };

    juce::HyperlinkButton creditLink, instaLink;

    juce::Label lengthLabel, resLabel, euclidLabel, hitsLabel, rotLabel;
    juce::Slider lengthSlider, hitsSlider, rotSlider;
    juce::ComboBox resBox;
    juce::TextButton randomButton { "RANDOM" };

    juce::TextButton shiftL, shiftR, revB, invB, dupB, halfB, clrB, fillB, copyB, pasteB;

    tg::StepGrid grid;

    Knob attack, decay, sustain, release, gate, swing, depth, stereo, mix;

    std::mt19937 rng { std::random_device{}() };
    static tg::PatternData clipboard;
    static bool clipboardFull;

    int lastSlot = -1, lastVersion = -1;
    bool syncing = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TranceGateEditor)
};
