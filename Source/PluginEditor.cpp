#include "PluginEditor.h"
#include "Presets.h"
#include "Tooltips.h"

using namespace juce;
namespace col = tg::colours;

tg::PatternData TranceGateEditor::clipboard;
bool TranceGateEditor::clipboardFull = false;

//==============================================================================
TGLookAndFeel::TGLookAndFeel()
{
    setColour (ResizableWindow::backgroundColourId, col::bg);
    setColour (TextButton::buttonColourId, col::panelHi);
    setColour (TextButton::buttonOnColourId, col::accentB);
    setColour (TextButton::textColourOffId, col::text);
    setColour (TextButton::textColourOnId, Colours::white);
    setColour (ComboBox::backgroundColourId, col::panelHi);
    setColour (ComboBox::textColourId, col::text);
    setColour (ComboBox::outlineColourId, col::outline);
    setColour (ComboBox::arrowColourId, col::accentA);
    setColour (PopupMenu::backgroundColourId, col::panel);
    setColour (PopupMenu::textColourId, col::text);
    setColour (PopupMenu::highlightedBackgroundColourId, col::accentC);
    setColour (PopupMenu::highlightedTextColourId, Colours::white);
    setColour (PopupMenu::headerTextColourId, col::accentA);
    setColour (Label::textColourId, col::text);
    setColour (Slider::textBoxTextColourId, col::text);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour (Slider::textBoxHighlightColourId, col::accentC);
    setColour (TooltipWindow::backgroundColourId, col::panelHi);
    setColour (TooltipWindow::textColourId, col::text);
    setColour (TooltipWindow::outlineColourId, col::outline);
}

void TGLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                      float startAngle, float endAngle, Slider&)
{
    auto bounds = Rectangle<int> (x, y, w, h).toFloat().reduced (6.0f);
    const float radius = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float track = 4.0f;

    Path bg;
    bg.addCentredArc (centre.x, centre.y, radius - track, radius - track, 0.0f, startAngle, endAngle, true);
    g.setColour (col::panelHi.brighter (0.15f));
    g.strokePath (bg, PathStrokeType (track, PathStrokeType::curved, PathStrokeType::rounded));

    if (pos > 0.0f)
    {
        Path arc;
        arc.addCentredArc (centre.x, centre.y, radius - track, radius - track, 0.0f, startAngle, angle, true);
        g.setGradientFill (ColourGradient (col::accentA, centre.x - radius, centre.y,
                                           col::accentB, centre.x + radius, centre.y, false));
        g.strokePath (arc, PathStrokeType (track, PathStrokeType::curved, PathStrokeType::rounded));
    }

    const float knobR = radius - track * 2.6f;
    g.setColour (col::panelHi);
    g.fillEllipse (centre.x - knobR, centre.y - knobR, knobR * 2.0f, knobR * 2.0f);
    g.setColour (col::outline);
    g.drawEllipse (centre.x - knobR, centre.y - knobR, knobR * 2.0f, knobR * 2.0f, 1.0f);

    Path pointer;
    pointer.addRoundedRectangle (-1.5f, -knobR + 3.0f, 3.0f, knobR * 0.45f, 1.5f);
    g.setColour (Colours::white);
    g.fillPath (pointer, AffineTransform::rotation (angle).translated (centre));
}

void TGLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();

    if (on)
        g.setGradientFill (ColourGradient (col::accentB, r.getX(), r.getY(), col::accentC, r.getRight(), r.getBottom(), false));
    else
        g.setColour (down ? col::panelHi.brighter (0.2f) : highlighted ? col::panelHi.brighter (0.1f) : col::panelHi);

    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (on ? Colours::transparentBlack : col::outline);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
}

Font TGLookAndFeel::getTextButtonFont (TextButton&, int h)
{
    return Font (FontOptions (jmin (13.0f, (float) h * 0.5f), Font::bold));
}

void TGLookAndFeel::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox&)
{
    auto r = Rectangle<int> (0, 0, w, h).toFloat().reduced (1.0f);
    g.setColour (col::panelHi);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (col::outline);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);

    Path arrow;
    const float ax = r.getRight() - 14.0f, ay = r.getCentreY();
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (col::accentA);
    g.fillPath (arrow);
}

Font TGLookAndFeel::getComboBoxFont (ComboBox&) { return Font (FontOptions (13.0f, Font::bold)); }

Label* TGLookAndFeel::createSliderTextBox (Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (Font (FontOptions (12.0f)));
    l->setColour (Label::textColourId, col::text);
    l->setColour (Label::outlineColourId, Colours::transparentBlack);
    l->setColour (Label::backgroundColourId, Colours::transparentBlack);
    return l;
}

static TextLayout layoutTooltip (const String& text, Colour colour)
{
    AttributedString s;
    s.setJustification (Justification::topLeft);
    s.setLineSpacing (2.0f);
    s.append (text, Font (FontOptions (13.0f)), colour);
    TextLayout tl;
    tl.createLayout (s, 460.0f);
    return tl;
}

Rectangle<int> TGLookAndFeel::getTooltipBounds (const String& tip, Point<int> pos, Rectangle<int> parentArea)
{
    const auto tl = layoutTooltip (tip, Colours::white);
    const int w = (int) std::ceil (tl.getWidth()) + 24;
    const int h = (int) std::ceil (tl.getHeight()) + 18;
    const int x = pos.x > parentArea.getCentreX() ? pos.x - (w + 12) : pos.x + 12;
    const int y = pos.y > parentArea.getCentreY() ? pos.y - (h + 8)  : pos.y + 16;
    return Rectangle<int> (x, y, w, h).constrainedWithin (parentArea);
}

void TGLookAndFeel::drawTooltip (Graphics& g, const String& text, int width, int height)
{
    const auto r = Rectangle<int> (width, height).toFloat();
    g.setColour (col::panelHi);
    g.fillRect (r);
    g.setColour (col::accentA.withAlpha (0.7f));
    g.drawRect (r, 1.0f);
    g.setColour (col::accentA);
    g.fillRect (r.withWidth (3.0f));
    layoutTooltip (text, col::text).draw (g, r.reduced (14.0f, 9.0f).withTrimmedLeft (0.0f));
}

//==============================================================================
TranceGateEditor::TranceGateEditor (TranceGateProcessor& owner)
    : AudioProcessorEditor (&owner), proc (owner), grid (owner)
{
    setLookAndFeel (&lnf);

    //---------------------------------------------------------------- slots
    for (int i = 0; i < tg::kNumPatterns; ++i)
    {
        auto& b = slotButtons[(size_t) i];
        b.setButtonText (String::charToString ((juce_wchar) ('A' + i)));
        b.setClickingTogglesState (false);
        b.setTooltip (tg::tips::slot ((char) ('A' + i), MidiMessage::getMidiNoteName (36 + i, true, true, 3)));
        b.onClick = [this, i] { proc.selectPatternFromGui (i); };
        addAndMakeVisible (b);
    }

    //---------------------------------------------------------------- credits
    const URL insta ("https://www.instagram.com/mikita_laurenau/");
    auto setupLink = [this, insta] (HyperlinkButton& l, const String& text, Justification j, Colour c)
    {
        l.setButtonText (text);
        l.setURL (insta);
        l.setFont (Font (FontOptions (11.5f)), false, j);
        l.setColour (HyperlinkButton::textColourId, c);
        l.setTooltip ("instagram.com/mikita_laurenau");
        addAndMakeVisible (l);
    };
    setupLink (creditLink, "created by mikita_laurenau", Justification::centredLeft, col::textDim);
    setupLink (instaLink, "Instagram: @mikita_laurenau", Justification::centredRight, col::accentA);

    presetButton.setTooltip (tg::tips::presets());
    presetButton.onClick = [this] { showPresetMenu(); };
    addAndMakeVisible (presetButton);

    //---------------------------------------------------------------- pattern settings
    auto setupLabel = [this] (Label& l, const String& t)
    {
        l.setText (t, dontSendNotification);
        l.setFont (FontOptions (11.0f, Font::bold));
        l.setColour (Label::textColourId, col::textDim);
        l.setJustificationType (Justification::centredRight);
        addAndMakeVisible (l);
    };
    setupLabel (lengthLabel, "STEPS");
    setupLabel (resLabel, "RATE");
    setupLabel (euclidLabel, "EUCLID");
    setupLabel (hitsLabel, "HITS");
    setupLabel (rotLabel, "ROT");

    auto setupIncDec = [this] (Slider& s, int lo, int hi)
    {
        s.setSliderStyle (Slider::IncDecButtons);
        s.setRange (lo, hi, 1);
        s.setTextBoxStyle (Slider::TextBoxLeft, false, 34, 26);
        s.setIncDecButtonsMode (Slider::incDecButtonsDraggable_Vertical);
        s.setColour (Slider::textBoxOutlineColourId, col::outline);
        s.setColour (Slider::textBoxBackgroundColourId, col::panel);
        addAndMakeVisible (s);
    };
    setupIncDec (lengthSlider, 1, tg::kMaxSteps);
    setupIncDec (hitsSlider, 0, tg::kMaxSteps);
    setupIncDec (rotSlider, 0, tg::kMaxSteps - 1);
    hitsSlider.setValue (5, dontSendNotification);

    lengthSlider.onValueChange = [this]
    {
        if (syncing) return;
        auto p = edit();
        const int newLen = (int) lengthSlider.getValue();
        for (int i = p.length; i < newLen; ++i) p.steps[(size_t) i] = {};   // fresh steps start empty
        p.length = newLen;
        commit (p);
    };

    hitsSlider.onValueChange = [this] { if (! syncing) applyEuclid(); };
    rotSlider.onValueChange  = [this] { if (! syncing) applyEuclid(); };
    hitsSlider.setTooltip (tg::tips::hits());
    rotSlider.setTooltip (tg::tips::rot());
    hitsLabel.setTooltip (tg::tips::hits());
    rotLabel.setTooltip (tg::tips::rot());
    euclidLabel.setTooltip (tg::tips::hits());
    lengthSlider.setTooltip (tg::tips::steps());
    lengthLabel.setTooltip (tg::tips::steps());
    resLabel.setTooltip (tg::tips::rate());

    for (int i = 0; i < (int) tg::resolutions().size(); ++i)
        resBox.addItem (tg::resolutions()[(size_t) i].name, i + 1);
    resBox.onChange = [this]
    {
        if (syncing) return;
        auto p = edit();
        p.resolution = resBox.getSelectedId() - 1;
        commit (p);
    };
    resBox.setTooltip (tg::tips::rate());
    addAndMakeVisible (resBox);

    randomButton.setTooltip (tg::tips::random());
    randomButton.onClick = [this] { showRandomMenu(); };
    addAndMakeVisible (randomButton);

    //---------------------------------------------------------------- tools
    setupSmallButton (shiftL, "<<",   tg::tips::shiftL(),  [this] { auto p = edit(); p.shift (-1); commit (p); });
    setupSmallButton (shiftR, ">>",   tg::tips::shiftR(), [this] { auto p = edit(); p.shift (1);  commit (p); });
    setupSmallButton (revB,   "REV",  tg::tips::rev(),             [this] { auto p = edit(); p.reverse(); commit (p); });
    setupSmallButton (invB,   "INV",  tg::tips::inv(), [this] { auto p = edit(); p.invert();  commit (p); });
    setupSmallButton (dupB,   "x2",   tg::tips::dup(), [this] { auto p = edit(); p.duplicate(); commit (p); });
    setupSmallButton (halfB,  "/2",   tg::tips::half(),        [this] { auto p = edit(); p.halve(); commit (p); });
    setupSmallButton (fillB,  "FILL", tg::tips::fill(),        [this] { auto p = edit(); p.fill();  commit (p); });
    setupSmallButton (clrB,   "CLR",  tg::tips::clr(),       [this] { auto p = edit(); p.clear(); commit (p); });
    setupSmallButton (copyB,  "COPY", tg::tips::copy(),        [this] { clipboard = edit(); clipboardFull = true; });
    setupSmallButton (pasteB, "PASTE",tg::tips::paste(), [this] { if (clipboardFull) commit (clipboard); });

    addAndMakeVisible (grid);

    //---------------------------------------------------------------- knobs
    setupKnob (attack,  "attack",  "ATTACK",  tg::tips::attack());
    setupKnob (decay,   "decay",   "DECAY",   tg::tips::decay());
    setupKnob (sustain, "sustain", "SUSTAIN", tg::tips::sustain());
    setupKnob (release, "release", "RELEASE", tg::tips::release());
    setupKnob (gate,    "gate",    "GATE",    tg::tips::gate());
    setupKnob (swing,   "swing",   "SWING",   tg::tips::swing());
    setupKnob (depth,   "depth",   "DEPTH",   tg::tips::depth());
    setupKnob (stereo,  "stereo",  "STEREO",  tg::tips::stereo());
    setupKnob (mix,     "mix",     "MIX",     tg::tips::mix());

    setSize (960, 620);
    syncControls();
    startTimerHz (30);
}

TranceGateEditor::~TranceGateEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void TranceGateEditor::setupKnob (Knob& k, const String& paramId, const String& name, const String& tip)
{
    k.slider.setTooltip (tip);
    k.label.setTooltip (tip);
    k.slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (Slider::TextBoxBelow, false, 80, 18);
    k.slider.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    k.slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    addAndMakeVisible (k.slider);
    k.attachment = std::make_unique<Attachment> (proc.apvts, paramId, k.slider);

    k.label.setText (name, dontSendNotification);
    k.label.setFont (FontOptions (11.0f, Font::bold));
    k.label.setColour (Label::textColourId, col::textDim);
    k.label.setJustificationType (Justification::centred);
    addAndMakeVisible (k.label);
}

void TranceGateEditor::setupSmallButton (TextButton& b, const String& text, const String& tip, std::function<void()> fn)
{
    b.setButtonText (text);
    b.setTooltip (tip);
    b.onClick = std::move (fn);
    addAndMakeVisible (b);
}

//==============================================================================
tg::PatternData TranceGateEditor::edit() const
{
    return proc.patterns[(size_t) proc.activePattern.load()].get();
}

void TranceGateEditor::commit (const tg::PatternData& p)
{
    proc.patterns[(size_t) proc.activePattern.load()].set (p);
    syncControls();
    grid.refresh();
}

void TranceGateEditor::syncControls()
{
    const ScopedValueSetter<bool> svs (syncing, true);
    const auto p = edit();
    lengthSlider.setValue (p.length, dontSendNotification);
    resBox.setSelectedId (p.resolution + 1, dontSendNotification);
    hitsSlider.setRange (0, p.length, 1);
    rotSlider.setRange (0, jmax (1, p.length - 1), 1);

    const int slot = proc.activePattern.load();
    for (int i = 0; i < tg::kNumPatterns; ++i)
        slotButtons[(size_t) i].setToggleState (i == slot, dontSendNotification);

    lastSlot = slot;
    lastVersion = proc.patterns[(size_t) slot].getVersion();
}

void TranceGateEditor::applyEuclid()
{
    auto p = edit();
    p.euclid ((int) hitsSlider.getValue(), (int) rotSlider.getValue());
    commit (p);
}

void TranceGateEditor::showPresetMenu()
{
    PopupMenu menu;
    const auto& lib = tg::presetLibrary();
    for (auto& cat : tg::presetCategories())
    {
        PopupMenu sub;
        for (int i = 0; i < (int) lib.size(); ++i)
            if (lib[(size_t) i].category == cat)
                sub.addItem (i + 1, lib[(size_t) i].name + "   (" + String (lib[(size_t) i].data.length) + " x "
                                    + tg::resolutions()[(size_t) lib[(size_t) i].data.resolution].name + ")");
        menu.addSubMenu (cat, sub);
    }

    menu.showMenuAsync (PopupMenu::Options().withTargetComponent (&presetButton),
                        [this] (int r)
                        {
                            if (r > 0) commit (tg::presetLibrary()[(size_t) r - 1].data);
                        });
}

void TranceGateEditor::showRandomMenu()
{
    using RM = tg::PatternData::RandomMode;
    PopupMenu menu;
    menu.addSectionHeader ("Generate");
    menu.addItem (1, "Sparse");
    menu.addItem (2, "Medium");
    menu.addItem (3, "Dense");
    menu.addItem (4, "Rage (ratchets + ties)");
    menu.addSeparator();
    menu.addSectionHeader ("Modify current");
    menu.addItem (5, "Mutate 25%");
    menu.addItem (6, "Random accents");

    menu.showMenuAsync (PopupMenu::Options().withTargetComponent (&randomButton),
                        [this] (int r)
                        {
                            if (r <= 0) return;
                            const RM modes[] = { RM::Sparse, RM::Medium, RM::Dense, RM::Rage, RM::Mutate, RM::Accents };
                            auto p = edit();
                            p.randomise (modes[r - 1], rng);
                            commit (p);
                        });
}

//==============================================================================
void TranceGateEditor::timerCallback()
{
    const int slot = proc.activePattern.load();
    if (slot != lastSlot || proc.patterns[(size_t) slot].getVersion() != lastVersion)
        syncControls();
    grid.refresh();
}

void TranceGateEditor::paint (Graphics& g)
{
    g.fillAll (col::bg);

    // header
    auto header = getLocalBounds().removeFromTop (56).toFloat();
    g.setGradientFill (ColourGradient (col::panel, 0, 0, col::bg, 0, header.getBottom(), false));
    g.fillRect (header);

    g.setFont (FontOptions (22.0f, Font::bold));
    g.setGradientFill (ColourGradient (col::accentA, 18.0f, 0, col::accentB, 300.0f, 0, false));
    g.drawText ("ULTIMATE TRANCE GATE", 18, 6, 310, 28, Justification::centredLeft);

    // knob section titles
    g.setFont (FontOptions (11.0f, Font::bold));
    g.setColour (col::textDim);
    g.drawText ("ENVELOPE", 18, 432, 200, 14, Justification::centredLeft);
    g.drawText ("GROOVE / OUTPUT", 18 + 5 * 100 + 30, 432, 200, 14, Justification::centredLeft);

    g.setColour (col::outline);
    g.drawVerticalLine (18 + 5 * 100 + 14, 440.0f, 590.0f);

    // footer hint
    g.setFont (FontOptions (11.0f));
    g.setColour (col::textDim.withAlpha (0.8f));
    g.drawText ("Click = on/off   Drag across = paint   Drag up/down on a step = level   Right-click = tie   "
                "MIDI C1-G1 = patterns A-H", getLocalBounds().removeFromBottom (24).reduced (18, 0).withTrimmedRight (200),
                Justification::centredLeft);
}

void TranceGateEditor::resized()
{
    // header
    {
        const int slotW = 38, gap = 6;
        int x = 340;
        for (auto& b : slotButtons) { b.setBounds (x, 12, slotW, 32); x += slotW + gap; }
        creditLink.setBounds (17, 32, 220, 18);
        presetButton.setBounds (getWidth() - 18 - 140, 12, 140, 32);
    }

    // row 1: pattern settings + generators
    {
        int x = 18;
        const int y = 66, h = 28;
        lengthLabel.setBounds (x, y, 46, h); x += 50;
        lengthSlider.setBounds (x, y, 92, h); x += 104;
        resLabel.setBounds (x, y, 40, h); x += 44;
        resBox.setBounds (x, y, 84, h); x += 110;
        euclidLabel.setBounds (x, y, 56, h); x += 58;
        hitsLabel.setBounds (x, y, 36, h); x += 38;
        hitsSlider.setBounds (x, y, 92, h); x += 100;
        rotLabel.setBounds (x, y, 30, h); x += 32;
        rotSlider.setBounds (x, y, 92, h); x += 112;
        randomButton.setBounds (getWidth() - 18 - 140, y, 140, h);
    }

    // row 2: tools
    {
        int x = 18;
        const int y = 102, h = 26, w = 56, gap = 6;
        for (auto* b : { &shiftL, &shiftR, &revB, &invB, &dupB, &halfB, &fillB, &clrB })
        {
            b->setBounds (x, y, w, h);
            x += w + gap;
        }
        pasteB.setBounds (getWidth() - 18 - 66, y, 66, h);
        copyB.setBounds (pasteB.getX() - 6 - 66, y, 66, h);
    }

    grid.setBounds (12, 136, getWidth() - 24, 288);
    instaLink.setBounds (getWidth() - 18 - 210, getHeight() - 24, 210, 24);

    // knobs
    {
        const int y = 450, kw = 100, kh = 112;
        int x = 18;
        for (auto* k : { &attack, &decay, &sustain, &release, &gate })
        {
            k->label.setBounds (x, y, kw, 14);
            k->slider.setBounds (x, y + 16, kw, kh);
            x += kw;
        }
        x += 30;
        for (auto* k : { &swing, &depth, &stereo, &mix })
        {
            k->label.setBounds (x, y, kw, 14);
            k->slider.setBounds (x, y + 16, kw, kh);
            x += kw;
        }
    }
}
