#include "StepGrid.h"
#include "Tooltips.h"

using namespace juce;

namespace tg
{

void StepGrid::resized()
{
    auto r = getLocalBounds().toFloat().reduced (8.0f);
    labels = r.removeFromLeft (62.0f);
    tieLane = r.removeFromBottom (20.0f);
    r.removeFromBottom (4.0f);
    ratchetLane = r.removeFromBottom (24.0f);
    r.removeFromBottom (6.0f);
    bars = r;
}

//==============================================================================
void StepGrid::computeCurve()
{
    curve.clear();
    const int width = jmax (64, (int) bars.getWidth());
    const int n = width * 2;

    const double stepBeats = resolutions()[(size_t) pattern.resolution].beats;
    const double loopBeats = stepBeats * pattern.length;
    const double bpm = jmax (20.0, lastBpmSeen);
    const double loopSeconds = loopBeats * 60.0 / bpm;
    const double simRate = (double) n / loopSeconds;

    GateVoice v;
    v.prepare (simRate);
    v.setParams (lastParams);

    // run one loop first so ties / releases from the end wrap correctly
    for (int i = 0; i < n; ++i)
        v.process (-loopBeats + loopBeats * (double) i / (double) n, pattern);

    curve.resize ((size_t) n);
    for (int i = 0; i < n; ++i)
        curve[(size_t) i] = v.process (loopBeats * (double) i / (double) n, pattern);
}

//==============================================================================
void StepGrid::paint (Graphics& g)
{
    g.setColour (colours::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 8.0f);

    const int len = pattern.length;
    const bool triplet = pattern.resolution == R8T || pattern.resolution == R16T || pattern.resolution == R32T;
    const int group = triplet ? 3 : 4;
    const int playStep = lastStep;

    // lane labels
    g.setFont (FontOptions (11.0f, Font::bold));
    g.setColour (colours::textDim);
    g.drawText ("LEVEL",   labels.withHeight (16.0f).withY (bars.getY()), Justification::centredLeft);
    g.drawText ("RATCHET", labels.withY (ratchetLane.getY()).withHeight (ratchetLane.getHeight()), Justification::centredLeft);
    g.drawText ("TIE",     labels.withY (tieLane.getY()).withHeight (tieLane.getHeight()), Justification::centredLeft);

    g.setFont (FontOptions (10.0f));
    g.drawText (String (len) + " x " + resolutions()[(size_t) pattern.resolution].name,
                labels.withY (bars.getBottom() - 16.0f).withHeight (16.0f), Justification::centredLeft);

    //--------------------------------------------------------------- bars
    for (int i = 0; i < len; ++i)
    {
        auto col = column (bars, i);
        const bool groupShade = (i / group) % 2 == 0;
        g.setColour (groupShade ? colours::panelHi : colours::panelHi.darker (0.25f));
        g.fillRect (col.reduced (1.0f, 0.0f));

        if (i == playStep)
        {
            g.setColour (Colours::white.withAlpha (0.10f));
            g.fillRect (col.reduced (1.0f, 0.0f));
        }

        const auto& st = pattern.steps[(size_t) i];
        if (pattern.isOn (i))
        {
            auto inner = col.reduced (3.0f, 2.0f);
            const float h = inner.getHeight() * st.level;
            auto bar = inner.withTop (inner.getBottom() - h);

            ColourGradient grad (colours::accentB, bar.getX(), inner.getY(),
                                 colours::accentA, bar.getX(), inner.getBottom(), false);
            g.setGradientFill (grad);

            const int r = jmax (1, st.ratchet);
            const float gap = r > 1 ? 2.0f : 0.0f;
            const float sw = (bar.getWidth() - gap * (float) (r - 1)) / (float) r;
            for (int k = 0; k < r; ++k)
                g.fillRoundedRectangle (bar.getX() + (sw + gap) * (float) k, bar.getY(), sw, bar.getHeight(), 2.0f);

            // tie bridge to the next step
            if (st.tie && i < len - 1 && pattern.isOn (i + 1))
            {
                const float nextH = inner.getHeight() * jmin (st.level, pattern.steps[(size_t) i + 1].level);
                g.fillRect (Rectangle<float> (bar.getRight() - 1.0f, inner.getBottom() - nextH,
                                              6.0f + 2.0f, nextH));
            }
        }
    }

    //--------------------------------------------------------------- envelope preview
    if (! curve.empty())
    {
        Path p;
        const float x0 = bars.getX(), w = bars.getWidth();
        const float y0 = bars.getBottom() - 2.0f, hh = bars.getHeight() - 4.0f;
        p.startNewSubPath (x0, y0);
        const size_t n = curve.size();
        for (size_t i = 0; i < n; i += 1)
            p.lineTo (x0 + w * (float) i / (float) n, y0 - hh * curve[i]);
        p.lineTo (x0 + w, y0);

        g.setColour (Colours::white.withAlpha (0.06f));
        g.fillPath (p);
        g.setColour (Colours::white.withAlpha (0.75f));
        g.strokePath (p, PathStrokeType (1.4f));
    }

    //--------------------------------------------------------------- ratchet lane
    g.setFont (FontOptions (11.0f, Font::bold));
    for (int i = 0; i < len; ++i)
    {
        auto c = column (ratchetLane, i).reduced (2.0f, 1.0f);
        const int r = pattern.steps[(size_t) i].ratchet;
        const bool active = r > 1 && pattern.isOn (i);
        g.setColour (active ? colours::accentC : colours::panelHi);
        g.fillRoundedRectangle (c, 3.0f);
        g.setColour (active ? Colours::white : colours::textDim.withAlpha (0.5f));
        g.drawText (r > 1 ? ("x" + String (r)) : String ("-"), c, Justification::centred);
    }

    //--------------------------------------------------------------- tie lane
    for (int i = 0; i < len; ++i)
    {
        auto c = column (tieLane, i);
        const bool tie = pattern.steps[(size_t) i].tie;
        if (tie)
        {
            auto pill = Rectangle<float> (c.getCentreX(), c.getY() + 4.0f, c.getWidth(), c.getHeight() - 8.0f);
            if (i == len - 1) pill.setWidth (c.getWidth() * 0.5f);
            g.setColour (colours::accentA);
            g.fillRoundedRectangle (pill, 3.0f);
        }
        else
        {
            g.setColour (colours::panelHi);
            g.fillEllipse (c.withSizeKeepingCentre (6.0f, 6.0f));
        }
    }

    // step numbers on the downbeats
    g.setFont (FontOptions (9.0f));
    g.setColour (colours::textDim.withAlpha (0.7f));
    for (int i = 0; i < len; i += group)
        g.drawText (String (i + 1), column (bars, i).withHeight (12.0f).translated (3.0f, 1.0f), Justification::topLeft);
}

//==============================================================================
void StepGrid::mouseDown (const MouseEvent& e)
{
    const auto pos = e.position;
    auto p = current();
    pattern = p;

    if (ratchetLane.contains (pos))
    {
        const int i = columnAt (pos.x);
        auto& st = p.steps[(size_t) i];
        if (e.mods.isPopupMenu()) st.ratchet = 1;
        else st.ratchet = st.ratchet >= kMaxRatchet ? 1 : st.ratchet + 1;
        if (! p.isOn (i) && st.ratchet > 1) st.level = 1.0f;
        commit (p);
        return;
    }

    if (tieLane.contains (pos))
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
        commit (p);
        return;
    }

    if (! bars.expanded (0.0f, 6.0f).contains (pos))
        return;

    const int i = columnAt (pos.x);
    dragStartCol = lastDragCol = i;

    if (e.mods.isPopupMenu())
    {
        // right click = toggle tie
        p.steps[(size_t) i].tie = ! p.steps[(size_t) i].tie;
        commit (p);
        drag = Drag::None;
        return;
    }

    if (e.mods.isShiftDown() || e.mods.isCommandDown() || e.mods.isAltDown())
    {
        drag = Drag::Level;
        p.steps[(size_t) i].level = levelAt (pos.y);
        commit (p);
        return;
    }

    if (p.isOn (i))
    {
        drag = Drag::Pending;
    }
    else
    {
        drag = Drag::Draw;
        dragLevel = levelAt (pos.y);
        p.steps[(size_t) i].level = dragLevel;
        p.steps[(size_t) i].ratchet = 1;
        commit (p);
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
        {
            drag = Drag::Level;
        }
    }

    const int from = jmin (lastDragCol, i), to = jmax (lastDragCol, i);
    for (int c = from; c <= to; ++c)
    {
        auto& st = p.steps[(size_t) c];
        switch (drag)
        {
            case Drag::Draw:
                if (! p.isOn (c)) st.ratchet = 1;
                st.level = levelAt (e.position.y);
                break;
            case Drag::Erase:
                st.level = 0.0f; st.tie = false;
                break;
            case Drag::Level:
                if (c == i && (p.isOn (c) || c == dragStartCol)) st.level = levelAt (e.position.y);
                break;
            case Drag::None:
            case Drag::Pending:
                break;
        }
    }
    lastDragCol = i;
    commit (p);
}

void StepGrid::mouseUp (const MouseEvent&)
{
    if (drag == Drag::Pending && dragStartCol >= 0)
    {
        auto p = current();
        p.steps[(size_t) dragStartCol].level = 0.0f;
        p.steps[(size_t) dragStartCol].tie = false;
        commit (p);
    }
    drag = Drag::None;
}

String StepGrid::getTooltip()
{
    const auto p = getMouseXYRelative().toFloat();
    if (ratchetLane.expanded (0.0f, 2.0f).contains (p)) return tips::ratchetLane();
    if (tieLane.expanded (0.0f, 4.0f).contains (p))     return tips::tieLane();
    return tips::levelLane();
}

void StepGrid::mouseDoubleClick (const MouseEvent& e)
{
    if (! bars.contains (e.position)) return;
    auto p = current();
    const int i = columnAt (e.position.x);
    p.steps[(size_t) i].level = 1.0f;
    p.steps[(size_t) i].ratchet = 1;
    commit (p);
}

} // namespace tg
