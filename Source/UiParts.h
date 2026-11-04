#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"
#include "Pattern.h"

namespace tg::ui
{
//==============================================================================
inline juce::Path starPath (juce::Rectangle<float> r)
{
    juce::Path p;
    p.addStar (r.getCentre(), 5, r.getWidth() * 0.22f, r.getWidth() * 0.5f, 0.0f);
    return p;
}

inline juce::Path chevron (juce::Rectangle<float> r, int dir)   // 0 left, 1 right, 2 down, 3 up
{
    juce::Path p;
    const auto c = r.getCentre();
    const float s = r.getWidth() * 0.22f;
    switch (dir)
    {
        case 0: p.startNewSubPath (c.x + s * 0.6f, c.y - s); p.lineTo (c.x - s * 0.6f, c.y); p.lineTo (c.x + s * 0.6f, c.y + s); break;
        case 1: p.startNewSubPath (c.x - s * 0.6f, c.y - s); p.lineTo (c.x + s * 0.6f, c.y); p.lineTo (c.x - s * 0.6f, c.y + s); break;
        case 2: p.startNewSubPath (c.x - s, c.y - s * 0.5f); p.lineTo (c.x, c.y + s * 0.5f); p.lineTo (c.x + s, c.y - s * 0.5f); break;
        default: p.startNewSubPath (c.x - s, c.y + s * 0.5f); p.lineTo (c.x, c.y - s * 0.5f); p.lineTo (c.x + s, c.y + s * 0.5f); break;
    }
    return p;
}

// Mini rhythm bars (same look in the preset bar and the preset list)
inline void drawMini (juce::Graphics& g, juce::Rectangle<float> r, const PatternData& p)
{
    const int n = p.length;
    const float w = n <= 16 ? 5.0f : (n <= 24 ? 3.0f : 2.0f);
    float x = r.getX();
    for (int i = 0; i < n && x + w <= r.getRight() + 0.5f; ++i)
    {
        const auto& st = p.steps[(size_t) i];
        const bool on = p.isOn (i);
        const float h = on ? (st.ratchet > 1 ? r.getHeight() : 4.0f + st.level * (r.getHeight() - 5.0f)) : 3.0f;
        g.setColour (on ? (st.ratchet > 1 ? col::orange : col::blue) : col::line);
        g.fillRoundedRectangle (x, r.getBottom() - h, w, h, 1.0f);
        x += w + 1.0f;
    }
}

//==============================================================================
// Small button that draws an icon path
class IconButton : public juce::Button
{
public:
    using IconFn = std::function<void (juce::Graphics&, juce::Rectangle<float>, juce::Colour)>;
    IconButton (const juce::String& name, IconFn fn) : juce::Button (name), icon (std::move (fn)) {}

    bool framed = true;
    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        if (framed)
        {
            g.setColour (down ? col::raised.brighter (0.15f) : over ? col::raised : col::panel);
            g.fillRoundedRectangle (r, 7.0f);
            g.setColour (col::line);
            g.drawRoundedRectangle (r, 7.0f, 1.0f);
        }
        else if (over || down)
        {
            g.setColour (col::raised);
            g.fillRoundedRectangle (r, 6.0f);
        }
        icon (g, r, isEnabled() ? col::text : col::line2);
    }

private:
    IconFn icon;
};

//==============================================================================
// Rotary knob with label + value to the right
class KnobItem : public juce::Component, public juce::SettableTooltipClient
{
public:
    KnobItem (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& labelText,
              juce::Colour arc, const juce::String& tip)
        : label (labelText), param (state.getParameter (paramId))
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        slider.setColour (juce::Slider::rotarySliderFillColourId, arc);
        slider.setColour (juce::Slider::thumbColourId, col::orange);
        slider.setPopupDisplayEnabled (false, false, nullptr);
        slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
        slider.setTooltip (tip);
        setTooltip (tip);
        slider.onValueChange = [this] { repaint(); };
        addAndMakeVisible (slider);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, paramId, slider);
    }

    void setPointerColour (juce::Colour c) { slider.setColour (juce::Slider::thumbColourId, c); }

    void resized() override
    {
        const int s = juce::jmin (40, getHeight());
        slider.setBounds (0, (getHeight() - s) / 2, s, s);
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().withTrimmedLeft ((float) slider.getRight() + 5.0f);
        const float cy = r.getCentreY();
        drawTracked (g, label, { r.getX(), cy - 14.0f, r.getWidth() + 10.0f, 12.0f }, 9.0f, 700, col::dim, juce::Justification::centredLeft, 0.1f);
        g.setColour (col::text);
        g.setFont (font (12.0f, 600));
        g.drawText (param->getCurrentValueAsText(), juce::Rectangle<float> (r.getX(), cy, r.getWidth() + 10.0f, 15.0f), juce::Justification::centredLeft, false);
    }

    juce::Slider slider;

private:
    juce::String label;
    juce::RangedAudioParameter* param;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

//==============================================================================
// [LABEL  -  value  +]
class Stepper : public juce::Component, public juce::SettableTooltipClient
{
public:
    Stepper (const juce::String& labelText, int lo, int hi) : label (labelText), minV (lo), maxV (hi) {}

    std::function<void (int)> onChange;
    int getValue() const noexcept { return value; }
    void setValue (int v) { v = juce::jlimit (minV, maxV, v); if (v != value) { value = v; repaint(); } }
    void setRange (int lo, int hi) { minV = lo; maxV = hi; setValue (value); }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        if (framed)
        {
            g.setColour (col::panel);
            g.fillRoundedRectangle (r, 7.0f);
            g.setColour (col::line);
            g.drawRoundedRectangle (r, 7.0f, 1.0f);
        }
        drawTracked (g, label, labelArea(), 9.0f, 700, col::dim, juce::Justification::centredLeft, 0.14f);
        for (auto b : { minusArea(), plusArea() })
        {
            g.setColour (col::raised);
            g.fillRoundedRectangle (b, 5.0f);
        }
        g.setColour (col::text);
        g.setFont (font (13.0f, 700));
        g.drawText ("-", minusArea(), juce::Justification::centred);
        g.drawText ("+", plusArea(), juce::Justification::centred);
        g.setFont (font (12.0f, 600));
        g.drawText (juce::String (value), valueArea(), juce::Justification::centred);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        dragStartValue = value;
        if (minusArea().contains (e.position)) change (value - 1);
        else if (plusArea().contains (e.position)) change (value + 1);
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (valueArea().expanded (4.0f).contains (e.mouseDownPosition))
            change (dragStartValue - e.getDistanceFromDragStartY() / 6);
    }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        change (value + (w.deltaY > 0 ? 1 : -1));
    }

    bool framed = true;
    float labelWidth = 40.0f;

private:
    void change (int v)
    {
        v = juce::jlimit (minV, maxV, v);
        if (v == value) return;
        value = v;
        repaint();
        if (onChange) onChange (value);
    }
    juce::Rectangle<float> labelArea() const { return { 8.0f, 0.0f, labelWidth, (float) getHeight() }; }
    juce::Rectangle<float> minusArea() const { return { 8.0f + labelWidth, (getHeight() - 20.0f) * 0.5f, 20.0f, 20.0f }; }
    juce::Rectangle<float> valueArea() const { return { 30.0f + labelWidth, 0.0f, 22.0f, (float) getHeight() }; }
    juce::Rectangle<float> plusArea()  const { return { 54.0f + labelWidth, (getHeight() - 20.0f) * 0.5f, 20.0f, 20.0f }; }

    juce::String label;
    int minV, maxV, value = 0, dragStartValue = 0;
};

//==============================================================================
// Mode button (VOL / LPF / HPF): shows a small caret when it can open the filter panel
class ModeButton : public juce::TextButton
{
public:
    using juce::TextButton::TextButton;
    bool showCaret = false, caretUp = false;

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        juce::TextButton::paintButton (g, over, down);
        if (showCaret && getToggleState())
        {
            auto r = getLocalBounds().toFloat();
            g.setColour (col::bg);
            g.strokePath (chevron ({ r.getRight() - 13.0f, r.getCentreY() - 5.0f, 10.0f, 10.0f }, caretUp ? 3 : 2),
                          juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }
};

//==============================================================================
// Drag the current rhythm out of the plugin as a .mid file
class MidiDragButton : public juce::Component, public juce::SettableTooltipClient
{
public:
    std::function<juce::File()> makeFile;

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (hover ? col::selected : col::raised);
        g.fillRoundedRectangle (r, 7.0f);
        juce::Path border;
        border.addRoundedRectangle (r, 7.0f);
        juce::Path dashed;
        const float dashes[] = { 3.0f, 2.5f };
        juce::PathStrokeType (1.0f).createDashedStroke (dashed, border, dashes, 2);
        g.setColour (col::blue);
        g.fillPath (dashed);
        for (int row = 0; row < 3; ++row)
            for (int c = 0; c < 2; ++c)
                g.fillEllipse (9.0f + c * 5.0f, r.getCentreY() - 6.5f + row * 5.0f, 2.8f, 2.8f);
        drawTracked (g, "MIDI", r.withTrimmedLeft (22.0f), 10.0f, 800, col::blue, juce::Justification::centredLeft, 0.1f);
    }

    void mouseEnter (const juce::MouseEvent&) override { hover = true; setMouseCursor (juce::MouseCursor::DraggingHandCursor); repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { hover = false; repaint(); }
    void mouseDown (const juce::MouseEvent&) override  { dragging = false; }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging || e.getDistanceFromDragStart() < 4 || ! makeFile) return;
        dragging = true;
        const auto f = makeFile();
        if (f.existsAsFile())
            juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
    }

private:
    bool hover = false, dragging = false;
};

//==============================================================================
// The big "current preset" button in the header
class PresetDisplay : public juce::Button
{
public:
    PresetDisplay() : juce::Button ("Presets") {}

    juce::String badge, category, name;
    bool edited = false, fav = false, open = false;
    PatternData pattern;

    void paintButton (juce::Graphics& g, bool over, bool) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (over ? col::raised : col::panel);
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (open ? col::orange : col::line);
        g.drawRoundedRectangle (r, 8.0f, 1.0f);

        auto a = r.reduced (9.0f, 0.0f);
        if (badge.isNotEmpty())
        {
            auto b = a.removeFromLeft (22.0f).withSizeKeepingCentre (22.0f, 22.0f);
            g.setColour (col::orange);
            g.fillRoundedRectangle (b, 6.0f);
            g.setColour (col::bg);
            g.setFont (font (12.0f, 800));
            g.drawText (badge, b, juce::Justification::centred);
            a.removeFromLeft (8.0f);
        }
        // category tag
        const auto tagText = category.toUpperCase();
        const float tagW = juce::jmin (110.0f, font (9.0f, 700).withExtraKerningFactor (0.1f).getStringWidthFloat (tagText) + 12.0f);
        auto tag = a.removeFromLeft (tagW).withSizeKeepingCentre (tagW, 17.0f);
        g.setColour (col::blue);
        g.fillRoundedRectangle (tag, 4.0f);
        drawTracked (g, tagText, tag, 9.0f, 700, col::bg, juce::Justification::centred, 0.1f);
        a.removeFromLeft (8.0f);

        // right side: chevron, mini bars, star
        auto chev = a.removeFromRight (14.0f);
        g.setColour (col::blue);
        g.strokePath (chevron (chev.withSizeKeepingCentre (14.0f, 14.0f), open ? 3 : 2),
                      juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        a.removeFromRight (8.0f);
        drawMini (g, a.removeFromRight (98.0f).withSizeKeepingCentre (98.0f, 14.0f), pattern);
        a.removeFromRight (8.0f);
        if (fav)
        {
            g.setColour (col::orange);
            g.fillPath (starPath (a.removeFromRight (12.0f).withSizeKeepingCentre (12.0f, 12.0f)));
            a.removeFromRight (6.0f);
        }
        g.setColour (col::text);
        g.setFont (font (13.0f, 700));
        g.drawText (name + (edited ? " *" : ""), a, juce::Justification::centredLeft, true);
    }
};

} // namespace tg::ui
