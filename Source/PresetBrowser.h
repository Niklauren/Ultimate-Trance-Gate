#pragma once

#include "UiParts.h"
#include "Presets.h"
#include "Settings.h"
#include "Tooltips.h"
#include "PluginProcessor.h"

namespace tg::ui
{
//==============================================================================
// Two-pane preset menu: categories (★ Favorites pinned) | search + presets with rhythm bars and a star
class PresetBrowser : public juce::Component
{
public:
    static constexpr int kFavourites = -1;

    PresetBrowser (TranceGateProcessor& p, Settings& s) : proc (p), settings (s), catList (*this), itemList (*this)
    {
        categories = presetCategories();

        search.setTextToShowWhenEmpty ("Search " + juce::String ((int) presetLibrary().size()) + " presets: rage, triplet, stutter...", col::faint);
        search.setFont (font (12.0f, 500));
        search.setIndents (26, 7);
        search.setTooltip (tips::search());
        search.onTextChange = [this] { rebuild(); };
        search.onEscapeKey  = [this] { if (onClose) onClose(); };
        addAndMakeVisible (search);

        catView.setViewedComponent (&catList, false);
        catView.setScrollBarsShown (true, false);
        catView.setScrollBarThickness (8);
        addAndMakeVisible (catView);

        itemView.setViewedComponent (&itemList, false);
        itemView.setScrollBarsShown (true, false);
        itemView.setScrollBarThickness (8);
        addAndMakeVisible (itemView);
    }

    std::function<void()> onClose;

    void openAt (int libraryIndex)
    {
        search.clear();
        view = libraryIndex >= 0 ? categories.indexOf (presetLibrary()[(size_t) libraryIndex].category) : 0;
        rebuild();
        const int row = items.indexOf (libraryIndex);
        if (row >= 0) itemView.setViewPosition (0, juce::jmax (0, row * kRow - 3 * kRow));
        const int catRow = view + 1;
        catView.setViewPosition (0, juce::jmax (0, catRow * kCatRow - 4 * kCatRow));
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.55f), 24, { 0, 12 }).drawForRectangle (g, getLocalBounds());
        g.setColour (col::panel);
        g.fillRoundedRectangle (r, 14.0f);
        g.setColour (col::line2);
        g.drawRoundedRectangle (r, 14.0f, 1.0f);
        g.setColour (col::line);
        g.drawVerticalLine (kCatWidth, 1.0f, (float) getHeight() - 1.0f);

        // search box frame + magnifier
        auto sb = search.getBounds().toFloat();
        g.setColour (col::raised);
        g.fillRoundedRectangle (sb, 8.0f);
        g.setColour (search.getText().isNotEmpty() ? col::blue : col::line);
        g.drawRoundedRectangle (sb, 8.0f, 1.0f);
        g.setColour (col::dim);
        g.drawEllipse (sb.getX() + 9.0f, sb.getCentreY() - 6.0f, 10.0f, 10.0f, 1.8f);
        g.drawLine (sb.getX() + 17.5f, sb.getCentreY() + 2.5f, sb.getX() + 21.0f, sb.getCentreY() + 6.0f, 1.8f);

        // header line
        auto hdr = juce::Rectangle<float> ((float) kCatWidth + 10.0f, sb.getBottom() + 4.0f, (float) getWidth() - kCatWidth - 20.0f, 16.0f);
        drawTracked (g, headerName().toUpperCase(), hdr, 10.0f, 700, col::dim, juce::Justification::centredLeft, 0.12f);
        g.setColour (col::dim);
        g.setFont (font (10.0f, 500));
        g.drawText (headerHint(), hdr, juce::Justification::centredRight);

        if (items.isEmpty())
        {
            g.setFont (font (12.0f, 500));
            g.drawText (search.getText().isNotEmpty() ? "Nothing found for \"" + search.getText() + "\""
                                                      : "No favorites yet: click the star next to any preset.",
                        itemView.getBounds().reduced (10, 10).withHeight (20), juce::Justification::centredLeft);
        }
    }

    void resized() override
    {
        catView.setBounds (6, 6, kCatWidth - 8, getHeight() - 12);
        catList.setSize (catView.getWidth() - 8, (categories.size() + 1) * kCatRow);
        search.setBounds (kCatWidth + 8, 8, getWidth() - kCatWidth - 16, 30);
        itemView.setBounds (kCatWidth + 4, 62, getWidth() - kCatWidth - 8, getHeight() - 68);
        itemList.setSize (itemView.getWidth() - 10, juce::jmax (itemView.getHeight(), items.size() * kRow));
    }

private:
    static constexpr int kCatWidth = 186, kCatRow = 27, kRow = 38;

    juce::String headerName() const
    {
        if (search.getText().trim().isNotEmpty()) return "Search";
        return view == kFavourites ? "Favorites" : categories[view];
    }
    juce::String headerHint() const
    {
        if (search.getText().trim().isNotEmpty()) return juce::String (items.size()) + " found";
        if (view == kFavourites) return "star any preset to pin it here";
        if (categories[view] == "Standard") return "MIDI C1-G1 loads A-H";
        return juce::String (items.size()) + " presets";
    }

    void rebuild()
    {
        items.clear();
        const auto& lib = presetLibrary();
        const auto q = search.getText().trim().toLowerCase();
        if (q.isNotEmpty())
        {
            for (int i = 0; i < (int) lib.size(); ++i)
                if ((lib[(size_t) i].name + " " + lib[(size_t) i].category).toLowerCase().contains (q))
                    items.add (i);
        }
        else if (view == kFavourites)
            items = settings.favouriteIndices();
        else
            for (int i = 0; i < (int) lib.size(); ++i)
                if (lib[(size_t) i].category == categories[view])
                    items.add (i);
        resized();
        itemView.setViewPosition (0, 0);
        catList.repaint();
        itemList.repaint();
        repaint();
    }

    //------------------------------------------------------------------------------
    struct CategoryList : public juce::Component
    {
        explicit CategoryList (PresetBrowser& o) : owner (o) {}
        void paint (juce::Graphics& g) override
        {
            const bool searching = owner.search.getText().trim().isNotEmpty();
            const int curCat = owner.currentCategory();
            for (int row = 0; row <= owner.categories.size(); ++row)
            {
                const int cat = row - 1;   // row 0 = favourites
                auto r = juce::Rectangle<float> (0.0f, (float) (row * kCatRow), (float) getWidth(), (float) kCatRow - 1.0f);
                const bool sel = ! searching && cat == owner.view;
                if (sel) { g.setColour (col::selected); g.fillRoundedRectangle (r, 6.0f); }
                const juce::String name = cat == kFavourites ? juce::String (juce::CharPointer_UTF8 ("\xe2\x98\x85 Favorites")) : owner.categories[cat];
                const int count = cat == kFavourites ? owner.settings.favouriteIndices().size() : owner.countIn (cat);
                g.setColour (sel ? col::text : (cat == curCat ? col::orange : (cat == kFavourites ? col::orange2 : col::text2)));
                g.setFont (font (12.0f, sel ? 800 : 500));
                g.drawText (name, r.reduced (8.0f, 0.0f).withTrimmedRight (24.0f), juce::Justification::centredLeft, true);
                g.setColour (col::dim);
                g.setFont (font (10.0f, 500));
                g.drawText (juce::String (count), r.reduced (8.0f, 0.0f), juce::Justification::centredRight);
            }
        }
        void mouseDown (const juce::MouseEvent& e) override
        {
            const int row = juce::jlimit (0, owner.categories.size(), e.y / kCatRow);
            owner.view = row - 1;
            owner.search.clear();
            owner.rebuild();
        }
        PresetBrowser& owner;
    };

    struct ItemList : public juce::Component, public juce::TooltipClient
    {
        explicit ItemList (PresetBrowser& o) : owner (o) {}
        void paint (juce::Graphics& g) override
        {
            const auto& lib = presetLibrary();
            const int cur = owner.proc.presetIndex[(size_t) owner.proc.getVariation()].load();
            const bool searching = owner.search.getText().trim().isNotEmpty();
            for (int row = 0; row < owner.items.size(); ++row)
            {
                const int idx = owner.items[row];
                const auto& pr = lib[(size_t) idx];
                auto r = juce::Rectangle<float> (0.0f, (float) (row * kRow), (float) getWidth(), (float) kRow - 2.0f);
                const bool sel = idx == cur;
                if (sel || row == hoverRow)
                {
                    g.setColour (sel ? col::selected : col::raised2);
                    g.fillRoundedRectangle (r, 8.0f);
                }
                auto a = r.reduced (8.0f, 0.0f);
                auto starArea = a.removeFromRight (26.0f);
                const bool fav = owner.settings.isFavourite (idx);
                if (fav) { g.setColour (col::orange); g.fillPath (starPath (starArea.withSizeKeepingCentre (14.0f, 14.0f))); }
                else     { g.setColour (col::faint);  g.strokePath (starPath (starArea.withSizeKeepingCentre (14.0f, 14.0f)), juce::PathStrokeType (1.3f)); }

                const bool standard = pr.category == "Standard";
                if (standard)
                {
                    const int n = owner.indexInCategory (idx);
                    auto b = a.removeFromLeft (22.0f).withSizeKeepingCentre (22.0f, 22.0f);
                    g.setColour (sel ? col::orange : col::raised);
                    g.fillRoundedRectangle (b, 6.0f);
                    g.setColour (sel ? col::bg : col::text);
                    g.setFont (font (11.0f, 800));
                    g.drawText (juce::String::charToString ((juce::juce_wchar) ('A' + n)), b, juce::Justification::centred);
                    a.removeFromLeft (10.0f);
                }
                juce::String meta;
                if (searching || owner.view == kFavourites) meta = pr.category;
                else if (standard) meta = juce::MidiMessage::getMidiNoteName (36 + owner.indexInCategory (idx), true, true, 3);
                else meta = juce::String (pr.data.length) + " x " + resolutions()[(size_t) pr.data.resolution].name;
                g.setColour (col::dim);
                g.setFont (font (10.0f, 500));
                g.drawText (meta, a.removeFromRight (86.0f), juce::Justification::centredRight, true);
                a.removeFromRight (10.0f);
                drawMini (g, a.removeFromRight (98.0f).withSizeKeepingCentre (98.0f, 16.0f), pr.data);
                a.removeFromRight (10.0f);
                g.setColour (col::text);
                g.setFont (font (13.0f, 600));
                g.drawText (pr.name, a, juce::Justification::centredLeft, true);
            }
        }
        void mouseMove (const juce::MouseEvent& e) override { const int r = e.y / kRow; if (r != hoverRow) { hoverRow = r; repaint(); } }
        void mouseExit (const juce::MouseEvent&) override   { hoverRow = -1; repaint(); }
        void mouseDown (const juce::MouseEvent& e) override
        {
            const int row = e.y / kRow;
            if (row < 0 || row >= owner.items.size()) return;
            const int idx = owner.items[row];
            if (e.x > getWidth() - 36)
            {
                owner.settings.toggleFavourite (idx);
                if (owner.view == kFavourites && owner.search.getText().isEmpty()) owner.rebuild();
                else { repaint(); owner.catList.repaint(); }
                return;
            }
            owner.proc.loadPreset (idx);
            if (owner.onClose) owner.onClose();
        }
        juce::String getTooltip() override
        {
            return getMouseXYRelative().x > getWidth() - 36 ? tips::star() : juce::String();
        }
        PresetBrowser& owner;
        int hoverRow = -1;
    };

    int countIn (int cat) const
    {
        int n = 0;
        for (auto& p : presetLibrary()) if (p.category == categories[cat]) ++n;
        return n;
    }
    int indexInCategory (int libraryIndex) const
    {
        const auto& lib = presetLibrary();
        int n = 0;
        for (int i = 0; i < libraryIndex; ++i) if (lib[(size_t) i].category == lib[(size_t) libraryIndex].category) ++n;
        return n;
    }
    int currentCategory() const
    {
        const int cur = proc.presetIndex[(size_t) proc.getVariation()].load();
        return cur >= 0 ? categories.indexOf (presetLibrary()[(size_t) cur].category) : -2;
    }

    TranceGateProcessor& proc;
    Settings& settings;
    juce::StringArray categories;
    int view = 0;
    juce::Array<int> items;

    juce::TextEditor search;
    juce::Viewport catView, itemView;
    CategoryList catList;
    ItemList itemList;
};

} // namespace tg::ui
