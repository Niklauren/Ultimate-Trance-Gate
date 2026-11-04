#include "PluginEditor.h"
#include "Presets.h"
#include "Tooltips.h"

using namespace juce;
namespace col = tg::ui::col;
using tg::ui::IconButton;

static constexpr int kW = 1040, kH = 380;

//==============================================================================
static IconButton::IconFn chevronIcon (int dir)
{
    return [dir] (Graphics& g, Rectangle<float> r, Colour c)
    {
        g.setColour (c);
        g.strokePath (tg::ui::chevron (r.withSizeKeepingCentre (14.0f, 14.0f), dir),
                      PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
    };
}

static IconButton::IconFn undoIcon (bool redo)
{
    return [redo] (Graphics& g, Rectangle<float> r, Colour c)
    {
        auto b = r.withSizeKeepingCentre (14.0f, 12.0f);
        Path p;
        if (! redo)
        {
            p.startNewSubPath (b.getX() + 4.0f, b.getY() + 7.5f);
            p.lineTo (b.getX(), b.getY() + 4.0f);
            p.lineTo (b.getX() + 4.0f, b.getY() + 0.5f);
            p.startNewSubPath (b.getX(), b.getY() + 4.0f);
            p.lineTo (b.getX() + 8.5f, b.getY() + 4.0f);
            p.quadraticTo (b.getRight(), b.getY() + 4.0f, b.getRight(), b.getY() + 8.0f);
            p.quadraticTo (b.getRight(), b.getBottom(), b.getX() + 8.0f, b.getBottom());
            p.lineTo (b.getX() + 6.0f, b.getBottom());
        }
        else
        {
            p.startNewSubPath (b.getRight() - 4.0f, b.getY() + 7.5f);
            p.lineTo (b.getRight(), b.getY() + 4.0f);
            p.lineTo (b.getRight() - 4.0f, b.getY() + 0.5f);
            p.startNewSubPath (b.getRight(), b.getY() + 4.0f);
            p.lineTo (b.getRight() - 8.5f, b.getY() + 4.0f);
            p.quadraticTo (b.getX(), b.getY() + 4.0f, b.getX(), b.getY() + 8.0f);
            p.quadraticTo (b.getX(), b.getBottom(), b.getRight() - 8.0f, b.getBottom());
            p.lineTo (b.getRight() - 6.0f, b.getBottom());
        }
        g.setColour (c);
        g.strokePath (p, PathStrokeType (1.8f, PathStrokeType::curved, PathStrokeType::rounded));
    };
}

static IconButton::IconFn closeIcon()
{
    return [] (Graphics& g, Rectangle<float> r, Colour c)
    {
        auto b = r.withSizeKeepingCentre (8.0f, 8.0f);
        g.setColour (c);
        g.drawLine ({ b.getTopLeft(), b.getBottomRight() }, 2.0f);
        g.drawLine ({ b.getTopRight(), b.getBottomLeft() }, 2.0f);
    };
}

//==============================================================================
TranceGateEditor::FilterPanel::FilterPanel (AudioProcessorValueTreeState& s)
    : cutoff (s, "cutoff", "CUTOFF", col::orange, tg::tips::cutoff()),
      reso   (s, "reso",   "RESO",   col::orange, tg::tips::reso()),
      close ("close", closeIcon())
{
    cutoff.setPointerColour (col::text);
    reso.setPointerColour (col::text);
    close.framed = false;
    close.setTooltip (tg::tips::closeFilter());
    addAndMakeVisible (cutoff);
    addAndMakeVisible (reso);
    addAndMakeVisible (close);
}

void TranceGateEditor::FilterPanel::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().withTrimmedBottom (8.0f).reduced (0.5f);
    DropShadow (Colours::black.withAlpha (0.5f), 20, { 0, 10 }).drawForRectangle (g, r.toNearestInt());
    Path p;
    p.addRoundedRectangle (r, 14.0f);
    const float ax = 34.0f;   // pointer towards the mode switch
    p.startNewSubPath (ax - 7.0f, r.getBottom() - 0.5f);
    p.lineTo (ax, r.getBottom() + 7.0f);
    p.lineTo (ax + 7.0f, r.getBottom() - 0.5f);
    p.closeSubPath();
    g.setColour (col::panel);
    g.fillPath (p);
    g.setColour (col::orange);
    g.drawRoundedRectangle (r, 14.0f, 1.0f);
    Path tip;   // pointer outline towards the mode switch
    tip.startNewSubPath (ax - 7.0f, r.getBottom());
    tip.lineTo (ax, r.getBottom() + 7.0f);
    tip.lineTo (ax + 7.0f, r.getBottom());
    g.setColour (col::panel);
    g.fillRect (ax - 6.0f, r.getBottom() - 1.5f, 12.0f, 2.0f);
    g.setColour (col::orange);
    g.strokePath (tip, PathStrokeType (1.0f));
    tg::ui::drawTracked (g, title, { 12.0f, 8.0f, r.getWidth() - 40.0f, 16.0f }, 10.0f, 800, col::orange2, Justification::centredLeft, 0.14f);
}

void TranceGateEditor::FilterPanel::resized()
{
    close.setBounds (getWidth() - 30, 6, 22, 22);
    cutoff.setBounds (12, 32, 116, 50);
    reso.setBounds (132, 32, 110, 50);
}

//==============================================================================
TranceGateEditor::TranceGateEditor (TranceGateProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      prevPreset ("prev", chevronIcon (0)), nextPreset ("next", chevronIcon (1)),
      undoButton ("undo", undoIcon (false)), redoButton ("redo", undoIcon (true)),
      grid (p), filterPanel (p.apvts), browser (p, *settings)
{
    setLookAndFeel (&look);
    setOpaque (true);

    //---------------------------------------------------------------- header
    credit.setButtonText ("created by mikita_laurenau");
    credit.setURL (URL ("https://www.instagram.com/mikita_laurenau/"));
    credit.setFont (tg::ui::font (11.5f, 500), false, Justification::centredLeft);
    credit.setTooltip ("instagram.com/mikita_laurenau");
    addAndMakeVisible (credit);

    prevPreset.setTooltip (tg::tips::prevPreset());
    nextPreset.setTooltip (tg::tips::nextPreset());
    auto step = [this] (int d)
    {
        const auto& lib = tg::presetLibrary();
        int idx = proc.presetIndex[(size_t) proc.getVariation()].load();
        if (idx < 0) idx = 0;
        const auto& cat = lib[(size_t) idx].category;
        Array<int> inCat;
        for (int i = 0; i < (int) lib.size(); ++i) if (lib[(size_t) i].category == cat) inCat.add (i);
        const int pos = jmax (0, inCat.indexOf (idx));
        proc.loadPreset (inCat[(pos + d + inCat.size()) % inCat.size()]);
        syncControls();
    };
    prevPreset.onClick = [step] { step (-1); };
    nextPreset.onClick = [step] { step (1); };
    addAndMakeVisible (prevPreset);
    addAndMakeVisible (nextPreset);

    presetDisplay.setTooltip (tg::tips::presetButton());
    presetDisplay.onClick = [this] { showPresetBrowser (! browser.isVisible()); };
    addAndMakeVisible (presetDisplay);

    for (int v = 0; v < TranceGateProcessor::kNumVariations; ++v)
    {
        auto& b = varButtons[(size_t) v];
        b.setButtonText (String::charToString ((juce_wchar) ('A' + v)));
        b.getProperties().set ("fontSize", 13.0f);
        b.getProperties().set ("weight", 800);
        b.getProperties().set ("noOutline", true);
        b.setColour (TextButton::buttonColourId, Colours::transparentBlack);
        b.setColour (TextButton::textColourOffId, col::text2);
        b.setTooltip (tg::tips::variation ((char) ('A' + v)));
        b.onClick = [this, v] { proc.selectVariation (v); syncControls(); };
        addAndMakeVisible (b);
    }

    //---------------------------------------------------------------- pattern row
    undoButton.setTooltip (tg::tips::undo());
    redoButton.setTooltip (tg::tips::redo());
    undoButton.onClick = [this] { proc.undo(); syncControls(); grid.refresh(); };
    redoButton.onClick = [this] { proc.redo(); syncControls(); grid.refresh(); };
    addAndMakeVisible (undoButton);
    addAndMakeVisible (redoButton);

    steps.setTooltip (tg::tips::steps());
    steps.onChange = [this] (int n)
    {
        if (syncing) return;
        auto pt = edit();
        for (int i = pt.length; i < n; ++i) pt.steps[(size_t) i] = {};
        pt.length = n;
        commit (pt);
    };
    addAndMakeVisible (steps);

    for (int i = 0; i < (int) tg::resolutions().size(); ++i)
        rate.addItem (tg::resolutions()[(size_t) i].name, i + 1);
    rate.setTooltip (tg::tips::rate());
    rate.onChange = [this]
    {
        if (syncing) return;
        auto pt = edit();
        pt.resolution = rate.getSelectedId() - 1;
        commit (pt);
    };
    addAndMakeVisible (rate);

    hits.labelWidth = 46.0f;
    rot.labelWidth = 26.0f;
    hits.setValue (5);
    hits.setTooltip (tg::tips::hits());
    rot.setTooltip (tg::tips::rot());
    hits.onChange = [this] (int) { if (! syncing) applyEuclid(); };
    rot.onChange  = [this] (int) { if (! syncing) applyEuclid(); };
    addAndMakeVisible (hits);
    addAndMakeVisible (rot);

    randomButton.setColour (TextButton::buttonColourId, col::orange);
    randomButton.setColour (TextButton::textColourOffId, col::bg);
    randomButton.getProperties().set ("noOutline", true);
    randomButton.getProperties().set ("weight", 800);
    randomButton.setTooltip (tg::tips::random());
    randomButton.onClick = [this] { showRandomMenu(); };
    addAndMakeVisible (randomButton);

    const char* toolNames[] = { "<<", ">>", "REV", "INV", "x2", "/2", "FILL", "CLR" };
    const String toolTips[] = { tg::tips::shiftL(), tg::tips::shiftR(), tg::tips::rev(), tg::tips::inv(),
                                tg::tips::dup(), tg::tips::half(), tg::tips::fill(), tg::tips::clr() };
    for (int i = 0; i < 8; ++i)
    {
        auto& b = tools[(size_t) i];
        b.setButtonText (toolNames[i]);
        b.getProperties().set ("fontSize", 10.0f);
        b.setTooltip (toolTips[i]);
        b.onClick = [this, i]
        {
            auto pt = edit();
            switch (i)
            {
                case 0: pt.shift (-1); break;
                case 1: pt.shift (1); break;
                case 2: pt.reverse(); break;
                case 3: pt.invert(); break;
                case 4: pt.duplicate(); break;
                case 5: pt.halve(); break;
                case 6: pt.fill(); break;
                default: pt.clear(); break;
            }
            commit (pt);
        };
        addAndMakeVisible (b);
    }

    midiDrag.setTooltip (tg::tips::midi());
    midiDrag.makeFile = [this]
    {
        const int idx = proc.presetIndex[(size_t) proc.getVariation()].load();
        const auto name = idx >= 0 ? tg::presetLibrary()[(size_t) idx].name : String ("Custom Gate");
        return tg::writePatternMidi (edit(), proc.getGateParams().gate, name);
    };
    addAndMakeVisible (midiDrag);

    addAndMakeVisible (grid);

    //---------------------------------------------------------------- mode + knobs
    const char* modeNames[] = { "VOL", "LPF", "HPF" };
    const String modeTips[] = { tg::tips::modeVol(), tg::tips::modeLp(), tg::tips::modeHp() };
    for (int m = 0; m < 3; ++m)
    {
        auto& b = modeButtons[(size_t) m];
        b.setButtonText (modeNames[m]);
        b.getProperties().set ("fontSize", 9.0f);
        b.getProperties().set ("weight", 800);
        b.getProperties().set ("radius", 5.0f);
        b.getProperties().set ("noOutline", true);
        b.setColour (TextButton::buttonColourId, col::raised);
        b.setColour (TextButton::textColourOffId, col::text2);
        b.setTooltip (modeTips[m]);
        b.onClick = [this, m]
        {
            if (m == proc.getMode() && m != TranceGateProcessor::ModeVolume)
                filterPanelOpen = ! filterPanelOpen;
            else
            {
                proc.selectMode (m);
                filterPanelOpen = m != TranceGateProcessor::ModeVolume;
            }
            syncControls();
        };
        addAndMakeVisible (b);
    }

    struct K { const char* id; const char* label; String tip; };
    const K list[] = {
        { "attack", "ATTACK", tg::tips::attack() }, { "decay", "DECAY", tg::tips::decay() },
        { "sustain", "SUSTAIN", tg::tips::sustain() }, { "release", "RELEASE", tg::tips::release() },
        { "gate", "GATE", tg::tips::gate() }, { "swing", "SWING", tg::tips::swing() },
        { "depth", "DEPTH", tg::tips::depth() }, { "stereo", "STEREO", tg::tips::stereo() },
        { "mix", "MIX", tg::tips::mix() } };
    for (auto& k : list)
    {
        knobs.push_back (std::make_unique<tg::ui::KnobItem> (proc.apvts, k.id, k.label, col::blue, k.tip));
        addAndMakeVisible (*knobs.back());
    }

    filterPanel.close.onClick = [this] { filterPanelOpen = false; syncControls(); };
    addChildComponent (filterPanel);

    //---------------------------------------------------------------- preset browser (overlay)
    dismiss.onClick = [this] { showPresetBrowser (false); };
    addChildComponent (dismiss);
    browser.onClose = [this] { showPresetBrowser (false); };
    addChildComponent (browser);

    setSize (kW, kH);
    syncControls();
    startTimerHz (30);
}

TranceGateEditor::~TranceGateEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

//==============================================================================
void TranceGateEditor::showPresetBrowser (bool show)
{
    if (show)
        browser.openAt (proc.presetIndex[(size_t) proc.getVariation()].load());
    dismiss.setVisible (show);
    browser.setVisible (show);
    if (show) { dismiss.toFront (false); browser.toFront (false); }
    syncControls();
}

bool TranceGateEditor::keyPressed (const KeyPress& k)
{
    if (k.getModifiers().isCommandDown() && k.getKeyCode() == 'Z')
    {
        if (k.getModifiers().isShiftDown()) proc.redo(); else proc.undo();
        syncControls();
        grid.refresh();
        return true;
    }
    return false;
}

void TranceGateEditor::applyEuclid()
{
    auto pt = edit();
    pt.euclid (hits.getValue(), rot.getValue());
    commit (pt);
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
                            auto pt = edit();
                            pt.randomise (modes[r - 1], rng);
                            commit (pt);
                        });
}

//==============================================================================
void TranceGateEditor::syncControls()
{
    const ScopedValueSetter<bool> svs (syncing, true);
    const int v = proc.getVariation();
    const auto pt = edit();
    const int idx = proc.presetIndex[(size_t) v].load();
    const auto& lib = tg::presetLibrary();

    if (idx >= 0)
    {
        const auto& pr = lib[(size_t) idx];
        presetDisplay.name = pr.name;
        presetDisplay.category = pr.category;
        presetDisplay.badge = pr.category == "Standard" ? String::charToString ((juce_wchar) ('A' + (idx - tg::presetIndex ("Standard", 0)))) : String();
        presetDisplay.fav = settings->isFavourite (idx);
    }
    else
    {
        presetDisplay.name = "Custom pattern";
        presetDisplay.category = "Custom";
        presetDisplay.badge = {};
        presetDisplay.fav = false;
    }
    presetDisplay.edited = proc.presetEdited[(size_t) v].load();
    presetDisplay.pattern = pt;
    presetDisplay.open = browser.isVisible();
    presetDisplay.repaint();

    for (int i = 0; i < TranceGateProcessor::kNumVariations; ++i)
    {
        auto& b = varButtons[(size_t) i];
        b.setToggleState (i == v, dontSendNotification);
    }

    undoButton.setEnabled (proc.canUndo());
    redoButton.setEnabled (proc.canRedo());

    steps.setValue (pt.length);
    rate.setSelectedId (pt.resolution + 1, dontSendNotification);
    hits.setRange (0, pt.length);
    rot.setRange (0, jmax (0, pt.length - 1));

    const int mode = proc.getMode();
    for (int m = 0; m < 3; ++m)
    {
        auto& b = modeButtons[(size_t) m];
        b.setToggleState (m == mode, dontSendNotification);
        b.showCaret = m != TranceGateProcessor::ModeVolume;
        b.caretUp = ! filterPanelOpen;
        b.repaint();
    }
    filterPanel.title = mode == TranceGateProcessor::ModeHighPass ? "HIGH-PASS FILTER" : "LOW-PASS FILTER";
    filterPanel.setVisible (mode != TranceGateProcessor::ModeVolume && filterPanelOpen);
    filterPanel.repaint();

    lastVersion = proc.stateVersion.load();
}

void TranceGateEditor::timerCallback()
{
    // cheap checks: re-sync when the processor state, mode or undo availability changed
    const int mode = proc.getMode();
    if (proc.stateVersion.load() != lastVersion || mode != lastModeSeen
        || undoButton.isEnabled() != proc.canUndo() || redoButton.isEnabled() != proc.canRedo())
    {
        lastModeSeen = mode;
        syncControls();
    }
    grid.refresh();
}

//==============================================================================
Rectangle<int> TranceGateEditor::varGroupBounds() const
{
    return { kW - 20 - 132, 13, 132, 38 };
}

void TranceGateEditor::paint (Graphics& g)
{
    g.fillAll (col::bg);

    // title
    AttributedString title;
    const auto tf = tg::ui::font (21.0f, 800).withExtraKerningFactor (-0.02f);
    title.append ("Ultimate Trance ", tf, col::text);
    title.append ("Gate", tf, col::orange);
    title.setWordWrap (AttributedString::none);
    title.draw (g, { 20.0f, 11.0f, 230.0f, 24.0f });

    // variation group
    auto vg = varGroupBounds().toFloat().reduced (0.5f);
    g.setColour (col::panel);
    g.fillRoundedRectangle (vg, 10.0f);
    g.setColour (col::line);
    g.drawRoundedRectangle (vg, 10.0f, 1.0f);
    tg::ui::drawTracked (g, "VAR", { vg.getX() + 8.0f, vg.getY(), 30.0f, vg.getHeight() }, 9.0f, 700, col::dim, Justification::centredLeft, 0.14f);

    // knob row panel
    auto kr = Rectangle<float> (20.0f, 286.0f, (float) kW - 40.0f, 76.0f).reduced (0.5f);
    g.setColour (col::panel);
    g.fillRoundedRectangle (kr, 16.0f);
    g.setColour (col::line);
    g.drawRoundedRectangle (kr, 16.0f, 1.0f);
    if (knobs.size() > 5)
    {
        const float x = (float) knobs[5]->getX() - 7.0f;
        g.drawVerticalLine ((int) x, kr.getY() + 10.0f, kr.getBottom() - 10.0f);
    }
}

void TranceGateEditor::paintOverChildren (Graphics& g)
{
    auto r = rate.getBounds().toFloat();
    tg::ui::drawTracked (g, "RATE", { r.getX() + 9.0f, r.getY(), 40.0f, r.getHeight() }, 9.0f, 700, col::dim, Justification::centredLeft, 0.14f);
}

void TranceGateEditor::resized()
{
    // header
    credit.setBounds (17, 34, 200, 16);
    const auto vg = varGroupBounds();
    for (int v = 0; v < TranceGateProcessor::kNumVariations; ++v)
        varButtons[(size_t) v].setBounds (vg.getX() + 34 + v * 32, vg.getY() + 4, 30, 30);
    const int px = 244, pr = vg.getX() - 10;
    prevPreset.setBounds (px, 13, 28, 38);
    nextPreset.setBounds (pr - 28, 13, 28, 38);
    presetDisplay.setBounds (px + 32, 13, pr - px - 64, 38);

    // pattern row
    const int y = 60, h = 28;
    undoButton.setBounds (20, y, 30, h);
    redoButton.setBounds (54, y, 30, h);
    steps.setBounds (90, y, 120, h);
    rate.setBounds (216, y, 94, h);
    hits.setBounds (316, y, 126, h);
    rot.setBounds (446, y, 106, h);
    randomButton.setBounds (558, y, 86, h);
    midiDrag.setBounds (kW - 20 - 64, y, 64, h);
    int tx = midiDrag.getX() - 6 - (8 * 34 + 7 * 3);
    for (auto& b : tools) { b.setBounds (tx, y, 34, h); tx += 37; }

    // grid
    grid.setBounds (20, 96, kW - 40, 182);

    // mode + knobs
    for (int m = 0; m < 3; ++m)
        modeButtons[(size_t) m].setBounds (30, 292 + m * 22, 50, 20);
    const int kx0 = 90, kx1 = kW - 28;
    const int n = (int) knobs.size();
    const float kw = (float) (kx1 - kx0 - 14) / (float) n;
    for (int i = 0; i < n; ++i)
    {
        const int extra = i >= 5 ? 14 : 0;   // gap for the divider before SWING
        knobs[(size_t) i]->setBounds (kx0 + extra + roundToInt (kw * (float) i), 296, roundToInt (kw) - 2, 56);
    }

    filterPanel.setBounds (20, 286 - 8 - 100, 256, 100);

    // overlay
    dismiss.setBounds (getLocalBounds());
    browser.setBounds (px + 32, 56, 660, 304);
}
