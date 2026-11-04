#include "StepGrid.h"
#include "Tooltips.h"

using namespace juce;
namespace col = tg::ui::col;

namespace tg
{

void StepGrid::resized()
{
    auto r = getLocalBounds().toFloat().reduced (10.0f, 8.0f);
    r.removeFromBottom (-2.0f);
    tieLane = r.removeFromBottom (12.0f);
    r.removeFromBottom (4.0f);
    ratchetLane = r.removeFromBottom (18.0f);
    r.removeFromBottom (4.0f);
    bars = r;
    legend = Rectangle<float> (bars.getRight() - 214.0f, bars.getY() + 4.0f, 208.0f, 20.0f);
    signalButton = Rectangle<float> (legend.getRight() - 58.0f, legend.getY() + 1.0f, 54.0f, 18.0f);
}

void StepGrid::commit (const PatternData& d, bool newUndoPoint)
{
    proc.editPattern (d, newUndoPoint);
    refresh();
}

//==============================================================================
void StepGrid::refresh()
{
    const int ver = proc.stateVersion.load();
    const auto gp = proc.getGateParams();
    const float mix   = proc.apvts.getRawParameterValue ("mix")->load();
    const float depth = proc.apvts.getRawParameterValue ("depth")->load();
    const int mode = proc.getMode();
    const double bpm = proc.lastBpm.load();
    const int step = proc.currentStep.load();
    bool dirty = false;

    if (ver != lastVersion || std::memcmp (&gp, &lastParams, sizeof (GateParams)) != 0
        || std::abs (mix - lastMix) > 1.0e-4f || std::abs (depth - lastDepth) > 1.0e-4f
        || mode != lastMode || std::abs (bpm - lastBpmSeen) > 1.0e-6 || curve.empty())
    {
        lastVersion = ver; lastParams = gp; lastMix = mix; lastDepth = depth; lastMode = mode; lastBpmSeen = bpm;
        pattern = current();
        computeCurve();
        dirty = true;
    }
    if (step != lastStep) { lastStep = step; dirty = true; }

    // live signal
    float maxIn = 0.0f;
    for (int b = 0; b < TranceGateProcessor::kSignalBins; ++b)
    {
        inShown[(size_t) b]  = proc.inPeaks[(size_t) b].load (std::memory_order_relaxed);
        outShown[(size_t) b] = proc.outPeaks[(size_t) b].load (std::memory_order_relaxed);
        maxIn = jmax (maxIn, inShown[(size_t) b]);
    }
    hasSignal = maxIn > 1.0e-4f;
    signalNorm = jmax (jmax (maxIn, 0.02f), signalNorm * 0.97f);
    if (showSignal && hasSignal) dirty = true;

    if (dirty) repaint();
}

void StepGrid::computeCurve()
{
    curve.clear();
    const int width = jmax (64, (int) bars.getWidth());
    const int n = width * 2;
    const double stepBeats = resolutions()[(size_t) pattern.resolution].beats;
    const double loopBeats = stepBeats * pattern.length;
    const double loopSeconds = loopBeats * 60.0 / jmax (20.0, lastBpmSeen);
    const double simRate = (double) n / loopSeconds;

    GateVoice v;
    v.prepare (simRate);
    v.setParams (lastParams);
    for (int i = 0; i < n; ++i)   // one loop first so ties / tails wrap correctly
        v.process (-loopBeats + loopBeats * (double) i / (double) n, pattern);

    curve.resize ((size_t) n);
    for (int i = 0; i < n; ++i)
    {
        const float e = v.process (loopBeats * (double) i / (double) n, pattern);
        const float amount = (1.0f - lastDepth) + lastDepth * e;
        curve[(size_t) i] = (1.0f - lastMix) + lastMix * amount;
    }
}

//==============================================================================
void StepGrid::paint (Graphics& g)
{
    auto area = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (col::panel);
    g.fillRoundedRectangle (area, 16.0f);
    g.setColour (col::line);
    g.drawRoundedRectangle (area, 16.0f, 1.0f);

    const int len = pattern.length;
    const bool triplet = pattern.resolution == R8T || pattern.resolution == R16T || pattern.resolution == R32T;
    const int group = triplet ? 3 : 4;

    // ---- columns
    for (int i = 0; i < len; ++i)
    {
        auto c = column (bars, i);
        g.setColour (i == lastStep ? col::selected : ((i / group) % 2 == 0 ? col::raised : col::raised2));
        g.fillRect (c);
        if (i % group == 0)
            ui::drawTracked (g, String (i + 1), c.withHeight (14.0f).translated (4.0f, 2.0f), 9.0f, 600, col::dim, Justification::topLeft, 0.0f);
    }

    // ---- live signal: input ghost behind, output on top
    const float mid = bars.getCentreY(), half = bars.getHeight() * 0.5f - 2.0f;
    if (showSignal && hasSignal)
    {
        const int nb = TranceGateProcessor::kSignalBins;
        auto band = [&] (const std::array<float, TranceGateProcessor::kSignalBins>& v)
        {
            Path p;
            for (int b = 0; b < nb; ++b)
            {
                const float x = bars.getX() + bars.getWidth() * ((float) b + 0.5f) / (float) nb;
                const float y = mid - half * jmin (1.0f, v[(size_t) b] / signalNorm);
                if (b == 0) p.startNewSubPath (bars.getX(), y);
                p.lineTo (x, y);
            }
            p.lineTo (bars.getRight(), mid - half * jmin (1.0f, v[(size_t) nb - 1] / signalNorm));
            for (int b = nb - 1; b >= 0; --b)
            {
                const float x = bars.getX() + bars.getWidth() * ((float) b + 0.5f) / (float) nb;
                p.lineTo (x, mid + half * jmin (1.0f, v[(size_t) b] / signalNorm));
            }
            p.lineTo (bars.getX(), mid + half * jmin (1.0f, v[0] / signalNorm));
            p.closeSubPath();
            return p;
        };
        g.setColour (col::ghost.withAlpha (0.5f));
        g.fillPath (band (inShown));
        g.setColour (col::blue.withAlpha (0.9f));
        g.fillPath (band (outShown));
        g.setColour (col::bg.withAlpha (0.6f));
        g.drawHorizontalLine ((int) mid, bars.getX(), bars.getRight());
    }

    // ---- step bars
    const auto barFill = showSignal && hasSignal ? col::blue.withAlpha (0.30f) : col::blue;
    for (int i = 0; i < len; ++i)
    {
        if (! pattern.isOn (i)) continue;
        const auto& st = pattern.steps[(size_t) i];
        auto inner = column (bars, i).reduced (len > 24 ? 1.0f : 3.0f, 0.0f);
        const float h = inner.getHeight() * st.level;
        auto bar = inner.withTop (inner.getBottom() - h);
        const int r = jmax (1, st.ratchet);
        const float gap = r > 1 ? 2.0f : 0.0f;
        const float sw = (bar.getWidth() - gap * (float) (r - 1)) / (float) r;
        for (int k = 0; k < r; ++k)
        {
            Rectangle<float> sb (bar.getX() + (sw + gap) * (float) k, bar.getY(), sw, bar.getHeight());
            Path p;
            p.addRoundedRectangle (sb.getX(), sb.getY(), sb.getWidth(), sb.getHeight(), 4.0f, 4.0f, true, true, false, false);
            g.setColour (barFill);
            g.fillPath (p);
            g.setColour (col::orange);
            g.fillRect (sb.withHeight (2.0f));
        }
    }

    // ---- gate curve (with Mix / Depth)
    if (! curve.empty())
    {
        Path p;
        const float top = bars.getY() + 4.0f, bottom = bars.getBottom() - 2.0f;
        const size_t n = curve.size();
        for (size_t i = 0; i < n; ++i)
        {
            const float x = bars.getX() + bars.getWidth() * (float) i / (float) n;
            const float y = bottom - (bottom - top) * curve[i];
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        g.setColour (col::orange);
        g.strokePath (p, PathStrokeType (1.5f));
    }

    // ---- legend + SIGNAL toggle
    g.setColour (col::bg.withAlpha (0.8f));
    g.fillRoundedRectangle (legend, 6.0f);
    auto lr = legend.reduced (8.0f, 0.0f);
    g.setColour (col::ghost);
    g.fillRoundedRectangle (lr.getX(), lr.getCentreY() - 3.0f, 10.0f, 6.0f, 2.0f);
    ui::drawTracked (g, "INPUT", lr.withTrimmedLeft (14.0f).withWidth (40.0f), 9.0f, 600, col::text2, Justification::centredLeft);
    const String dot (CharPointer_UTF8 ("\xc2\xb7"));
    const String outLabel = proc.getMode() == TranceGateProcessor::ModeLowPass ? "OUT " + dot + " LPF"
                          : proc.getMode() == TranceGateProcessor::ModeHighPass ? "OUT " + dot + " HPF" : String ("OUTPUT");
    g.setColour (col::blue);
    g.fillRoundedRectangle (lr.getX() + 58.0f, lr.getCentreY() - 3.0f, 10.0f, 6.0f, 2.0f);
    ui::drawTracked (g, outLabel, lr.withTrimmedLeft (72.0f).withWidth (70.0f), 9.0f, 600, col::text2, Justification::centredLeft);
    g.setColour (showSignal ? col::blue : col::line2);
    g.drawRoundedRectangle (signalButton.reduced (0.5f), 4.0f, 1.0f);
    ui::drawTracked (g, "SIGNAL", signalButton, 9.0f, 800, showSignal ? col::blue : col::dim, Justification::centred);

    if (showSignal && ! hasSignal)
        ui::drawTracked (g, "play audio through the plugin to see the signal", bars.withTrimmedLeft (8.0f).withTrimmedBottom (6.0f).withTop (bars.getBottom() - 20.0f),
                         10.0f, 500, col::dim.withAlpha (0.7f), Justification::bottomLeft, 0.02f);

    // ---- ratchet lane
    for (int i = 0; i < len; ++i)
    {
        auto c = column (ratchetLane, i).reduced (2.0f, 0.0f);
        const int r = pattern.steps[(size_t) i].ratchet;
        const bool active = r > 1 && pattern.isOn (i);
        g.setColour (active ? col::orange : col::raised);
        g.fillRoundedRectangle (c, 4.0f);
        ui::drawTracked (g, active ? "x" + String (r) : String (CharPointer_UTF8 ("\xc2\xb7")), c, 9.0f, 800, active ? col::bg : col::dim, Justification::centred, 0.0f);
    }

    // ---- tie lane
    for (int i = 0; i < len; ++i)
    {
        auto c = column (tieLane, i);
        if (pattern.steps[(size_t) i].tie)
        {
            auto pill = Rectangle<float> (c.getCentreX(), c.getCentreY() - 2.5f, i == len - 1 ? c.getWidth() * 0.5f : c.getWidth(), 5.0f);
            g.setColour (col::blue);
            g.fillRoundedRectangle (pill, 2.5f);
        }
        else
        {
            g.setColour (col::line);
            g.fillEllipse (c.withSizeKeepingCentre (4.0f, 4.0f));
        }
    }
}

//==============================================================================
String StepGrid::getTooltip()
{
    const auto p = getMouseXYRelative().toFloat();
    if (signalButton.expanded (2.0f).contains (p)) return tips::signal();
    if (ratchetLane.expanded (0.0f, 2.0f).contains (p)) return tips::ratchetLane();
    if (tieLane.expanded (0.0f, 4.0f).contains (p))     return tips::tieLane();
    return tips::levelLane();
}

void StepGrid::mouseDown (const MouseEvent& e)
{
    const auto pos = e.position;
    if (signalButton.expanded (2.0f).contains (pos)) { showSignal = ! showSignal; repaint(); return; }

    auto p = current();
    pattern = p;

    if (ratchetLane.contains (pos))
    {
        const int i = columnAt (pos.x);
        auto& st = p.steps[(size_t) i];
        if (e.mods.isPopupMenu()) st.ratchet = 1;
        else st.ratchet = st.ratchet >= kMaxRatchet ? 1 : st.ratchet + 1;
        if (! p.isOn (i) && st.ratchet > 1) st.level = 1.0f;
        commit (p, true);
        return;
    }

    if (tieLane.expanded (0.0f, 2.0f).contains (pos))
    {
        const int i = columnAt (pos.x);
        auto& st = p.steps[(size_t) i];
        st.tie = ! st.tie;
        if (st.tie)
        {
            if (! p.isOn (i)) st.level = 1.0f;
            const int nx = (i + 1) % p.length;
            if (! p.isOn (nx)) p.steps[(size_t) nx].level = st.level;
        }
        commit (p, true);
        return;
    }

    if (! bars.expanded (0.0f, 6.0f).contains (pos)) return;

    const int i = columnAt (pos.x);
    dragStartCol = lastDragCol = i;

    if (e.mods.isPopupMenu())
    {
        p.steps[(size_t) i].tie = ! p.steps[(size_t) i].tie;
        commit (p, true);
        drag = Drag::None;
        return;
    }

    proc.pushUndoPoint();   // one undo step for the whole gesture

    if (e.mods.isShiftDown() || e.mods.isCommandDown() || e.mods.isAltDown())
    {
        drag = Drag::Level;
        p.steps[(size_t) i].level = levelAt (pos.y);
        commit (p, false);
        return;
    }

    if (p.isOn (i))
        drag = Drag::Pending;
    else
    {
        drag = Drag::Draw;
        p.steps[(size_t) i].level = levelAt (pos.y);
        p.steps[(size_t) i].ratchet = 1;
        commit (p, false);
    }
}

void StepGrid::mouseDrag (const MouseEvent& e)
{
    if (drag == Drag::None) return;
    auto p = current();
    const int i = columnAt (e.position.x);

    if (drag == Drag::Pending)
    {
        if (i != dragStartCol)
        {
            drag = Drag::Erase;
            p.steps[(size_t) dragStartCol].level = 0.0f;
            p.steps[(size_t) dragStartCol].tie = false;
        }
        else if (std::abs (e.getDistanceFromDragStartY()) > 4)
            drag = Drag::Level;
    }

    const int from = jmin (lastDragCol, i), to = jmax (lastDragCol, i);
    for (int c = from; c <= to; ++c)
    {
        auto& st = p.steps[(size_t) c];
        switch (drag)
        {
            case Drag::Draw:  if (! p.isOn (c)) st.ratchet = 1; st.level = levelAt (e.position.y); break;
            case Drag::Erase: st.level = 0.0f; st.tie = false; break;
            case Drag::Level: if (c == i && (p.isOn (c) || c == dragStartCol)) st.level = levelAt (e.position.y); break;
            case Drag::None:
            case Drag::Pending: break;
        }
    }
    lastDragCol = i;
    commit (p, false);
}

void StepGrid::mouseUp (const MouseEvent&)
{
    if (drag == Drag::Pending && dragStartCol >= 0)
    {
        auto p = current();
        p.steps[(size_t) dragStartCol].level = 0.0f;
        p.steps[(size_t) dragStartCol].tie = false;
        commit (p, false);
    }
    drag = Drag::None;
}

void StepGrid::mouseDoubleClick (const MouseEvent& e)
{
    if (! bars.contains (e.position)) return;
    auto p = current();
    const int i = columnAt (e.position.x);
    p.steps[(size_t) i].level = 1.0f;
    p.steps[(size_t) i].ratchet = 1;
    commit (p, false);
}

} // namespace tg
