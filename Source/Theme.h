#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"

// Palette: Cassis (ground) · Cool Blue (second colour) · Orange Topaze (accents)
namespace tg::ui
{
namespace col
{
    const juce::Colour bg       { 0xff22101a };   // deep cassis
    const juce::Colour panel    { 0xff2d1621 };
    const juce::Colour raised   { 0xff3a1f2b };
    const juce::Colour raised2  { 0xff331a25 };
    const juce::Colour selected { 0xff4d2a3a };
    const juce::Colour line     { 0xff4f2d3d };
    const juce::Colour line2    { 0xff64394c };
    const juce::Colour text     { 0xffeaf6ff };
    const juce::Colour text2    { 0xffc4ddee };
    const juce::Colour dim      { 0xffa9c3d6 };
    const juce::Colour faint    { 0xff8fa7ba };
    const juce::Colour blue     { 0xffd7efff };   // Cool Blue
    const juce::Colour orange   { 0xffff5c34 };   // Orange Topaze
    const juce::Colour orange2  { 0xffff8a6a };
    const juce::Colour ghost    { 0xff8e6f7e };
}

// Figtree (SIL OFL), embedded
inline juce::Typeface::Ptr typeface (int weight)
{
    static juce::Typeface::Ptr regular  = juce::Typeface::createSystemTypefaceFor (BinaryData::FigtreeRegular_ttf,   BinaryData::FigtreeRegular_ttfSize);
    static juce::Typeface::Ptr semibold = juce::Typeface::createSystemTypefaceFor (BinaryData::FigtreeSemiBold_ttf,  BinaryData::FigtreeSemiBold_ttfSize);
    static juce::Typeface::Ptr bold     = juce::Typeface::createSystemTypefaceFor (BinaryData::FigtreeBold_ttf,      BinaryData::FigtreeBold_ttfSize);
    static juce::Typeface::Ptr xbold    = juce::Typeface::createSystemTypefaceFor (BinaryData::FigtreeExtraBold_ttf, BinaryData::FigtreeExtraBold_ttfSize);
    if (weight >= 800) return xbold;
    if (weight >= 700) return bold;
    if (weight >= 600) return semibold;
    return regular;
}

inline juce::Font font (float size, int weight = 400)
{
    return juce::Font (juce::FontOptions (typeface (weight)).withHeight (size));
}

// System font for Cyrillic tooltip text
inline juce::Font systemFont (float size, bool bold = false)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}

inline void drawTracked (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r, float size,
                         int weight, juce::Colour c, juce::Justification j, float tracking = 0.12f)
{
    juce::AttributedString s;
    s.append (text, font (size, weight).withExtraKerningFactor (tracking), c);
    s.setJustification (j);
    s.setWordWrap (juce::AttributedString::none);
    s.draw (g, r);
}

//==============================================================================
class Look : public juce::LookAndFeel_V4
{
public:
    Look()
    {
        setColour (juce::ResizableWindow::backgroundColourId, col::bg);
        setColour (juce::TextButton::buttonColourId, col::panel);
        setColour (juce::TextButton::buttonOnColourId, col::orange);
        setColour (juce::TextButton::textColourOffId, col::text);
        setColour (juce::TextButton::textColourOnId, col::bg);
        setColour (juce::ComboBox::backgroundColourId, col::panel);
        setColour (juce::ComboBox::textColourId, col::text);
        setColour (juce::ComboBox::outlineColourId, col::line);
        setColour (juce::ComboBox::arrowColourId, col::blue);
        setColour (juce::PopupMenu::backgroundColourId, col::panel);
        setColour (juce::PopupMenu::textColourId, col::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, col::selected);
        setColour (juce::PopupMenu::highlightedTextColourId, col::text);
        setColour (juce::PopupMenu::headerTextColourId, col::orange2);
        setColour (juce::Label::textColourId, col::text);
        setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::TextEditor::textColourId, col::text);
        setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::TextEditor::highlightColourId, col::selected);
        setColour (juce::CaretComponent::caretColourId, col::orange);
        setColour (juce::ScrollBar::thumbColourId, col::line2);
        setColour (juce::TooltipWindow::backgroundColourId, col::raised);
        setColour (juce::TooltipWindow::textColourId, col::text);
        setColour (juce::TooltipWindow::outlineColourId, col::blue);
        setColour (juce::HyperlinkButton::textColourId, col::dim);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& bgColour, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = b.getToggleState();
        auto fill = on ? b.findColour (juce::TextButton::buttonOnColourId) : bgColour;
        if (! on && (over || down)) fill = fill.brighter (down ? 0.18f : 0.08f);
        const float radius = (float) b.getProperties().getWithDefault ("radius", 7.0f);
        g.setColour (fill);
        g.fillRoundedRectangle (r, radius);
        if (! on && ! (bool) b.getProperties().getWithDefault ("noOutline", false))
        {
            g.setColour (b.findColour (juce::ComboBox::outlineColourId));
            g.drawRoundedRectangle (r, radius, 1.0f);
        }
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool) override
    {
        const auto c = b.getToggleState() ? b.findColour (juce::TextButton::textColourOnId)
                                          : b.findColour (juce::TextButton::textColourOffId);
        const float size = (float) b.getProperties().getWithDefault ("fontSize", 11.0f);
        const int weight = (int) b.getProperties().getWithDefault ("weight", 700);
        drawTracked (g, b.getButtonText(), b.getLocalBounds().toFloat(), size, weight,
                     c.withMultipliedAlpha (b.isEnabled() ? 1.0f : 0.4f), juce::Justification::centred, 0.08f);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider& s) override
    {
        auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat();
        const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
        const auto c = bounds.getCentre();
        const float r = size * 0.5f - 3.0f;
        const float thick = size * 0.094f;
        const auto arcColour = s.findColour (juce::Slider::rotarySliderFillColourId);

        juce::Path track;
        track.addCentredArc (c.x, c.y, r, r, 0.0f, startAngle, endAngle, true);
        g.setColour (col::line);
        g.strokePath (track, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float a = startAngle + pos * (endAngle - startAngle);
        if (pos > 0.001f)
        {
            juce::Path arc;
            arc.addCentredArc (c.x, c.y, r, r, 0.0f, startAngle, a, true);
            g.setColour (arcColour);
            g.strokePath (arc, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        const float inner = r * 0.62f;
        g.setColour (col::raised);
        g.fillEllipse (c.x - inner, c.y - inner, inner * 2, inner * 2);
        g.setColour (col::line);
        g.drawEllipse (c.x - inner, c.y - inner, inner * 2, inner * 2, 1.0f);

        const auto tip = c.getPointOnCircumference (inner * 0.78f, a);
        g.setColour (s.findColour (juce::Slider::thumbColourId));
        g.drawLine ({ c, tip }, thick * 0.75f);
    }

    juce::Font getPopupMenuFont() override { return font (13.0f, 600); }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return font (12.0f, 600); }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&) override
    {
        auto r = juce::Rectangle<int> (w, h).toFloat().reduced (0.5f);
        g.setColour (col::panel);
        g.fillRoundedRectangle (r, 7.0f);
        g.setColour (col::line);
        g.drawRoundedRectangle (r, 7.0f, 1.0f);
        juce::Path p;
        const float ax = r.getRight() - 11.0f, ay = r.getCentreY();
        p.startNewSubPath (ax - 3.5f, ay - 1.5f);
        p.lineTo (ax, ay + 2.0f);
        p.lineTo (ax + 3.5f, ay - 1.5f);
        g.setColour (col::blue);
        g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (box.getLocalBounds().withTrimmedRight (16).withTrimmedLeft (2));
        label.setFont (font (12.0f, 600));
        label.setJustificationType (juce::Justification::centredRight);
    }

    void drawPopupMenuBackground (juce::Graphics& g, int w, int h) override
    {
        g.fillAll (col::panel);
        g.setColour (col::line2);
        g.drawRect (0, 0, w, h);
    }

    void drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical,
                        int thumbPos, int thumbSize, bool over, bool down) override
    {
        auto thumb = vertical ? juce::Rectangle<int> (x, thumbPos, w, thumbSize) : juce::Rectangle<int> (thumbPos, y, thumbSize, h);
        g.setColour ((over || down) ? col::line2.brighter (0.2f) : col::line2);
        g.fillRoundedRectangle (thumb.toFloat().reduced (2.0f), 3.0f);
    }

    // Tooltips: system font (Cyrillic), left aligned, orange stripe
    static juce::TextLayout layoutTip (const juce::String& text, juce::Colour c)
    {
        juce::AttributedString s;
        s.setJustification (juce::Justification::topLeft);
        s.setLineSpacing (2.0f);
        s.append (text, systemFont (13.0f), c);
        juce::TextLayout tl;
        tl.createLayout (s, 460.0f);
        return tl;
    }

    juce::Rectangle<int> getTooltipBounds (const juce::String& tip, juce::Point<int> pos, juce::Rectangle<int> parent) override
    {
        const auto tl = layoutTip (tip, juce::Colours::white);
        const int w = (int) std::ceil (tl.getWidth()) + 26, h = (int) std::ceil (tl.getHeight()) + 18;
        const int px = pos.x > parent.getCentreX() ? pos.x - (w + 12) : pos.x + 12;
        const int py = pos.y > parent.getCentreY() ? pos.y - (h + 8)  : pos.y + 18;
        return juce::Rectangle<int> (px, py, w, h).constrainedWithin (parent);
    }

    void drawTooltip (juce::Graphics& g, const juce::String& text, int w, int h) override
    {
        const auto r = juce::Rectangle<int> (w, h).toFloat();
        g.setColour (col::raised);
        g.fillRect (r);
        g.setColour (col::blue.withAlpha (0.6f));
        g.drawRect (r, 1.0f);
        g.setColour (col::orange);
        g.fillRect (r.withWidth (3.0f));
        layoutTip (text, col::text).draw (g, r.reduced (14.0f, 9.0f));
    }
};

} // namespace tg::ui
