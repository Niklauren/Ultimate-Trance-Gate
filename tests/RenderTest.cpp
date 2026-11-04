// Offline tests for Ultimate Trance Gate v2: renders audio through the processor with a
// fake host transport and checks the gate, variations, undo, filter modes, MIDI export,
// the signal view data, tooltips on every control; writes screenshots of the editor.

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"
#include "Settings.h"

using namespace juce;

struct FakePlayHead : AudioPlayHead
{
    double ppq = 0.0, bpm = 140.0;
    bool playing = true;
    Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p;
        p.setBpm (bpm);
        p.setPpqPosition (ppq);
        p.setIsPlaying (playing);
        return p;
    }
};

static int failures = 0;
static void check (bool ok, const String& what)
{
    std::cout << (ok ? "  [ok]   " : "  [FAIL] ") << what << std::endl;
    if (! ok) ++failures;
}

static void setP (TranceGateProcessor& p, const String& id, float v)
{
    auto* prm = p.apvts.getParameter (id);
    prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

// Render 'bars' bars; input = constant 1.0 (or a sine of 'sineHz' if > 0)
static AudioBuffer<float> render (TranceGateProcessor& proc, FakePlayHead& ph, double sr, int bars,
                                  MidiBuffer* firstMidi = nullptr, double sineHz = 0.0)
{
    const int block = 512;
    const int total = (int) std::round (bars * 4.0 * 60.0 / ph.bpm * sr);
    AudioBuffer<float> out (2, total);
    MidiBuffer midi;
    ph.ppq = 0.0;
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = jmin (block, total - pos);
        AudioBuffer<float> buf (2, n);
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
                buf.setSample (c, i, sineHz > 0 ? (float) std::sin (2.0 * MathConstants<double>::pi * sineHz * (pos + i) / sr) : 1.0f);
        midi.clear();
        if (pos == 0 && firstMidi != nullptr) midi = *firstMidi;
        proc.processBlock (buf, midi);
        for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, buf, c, 0, n);
        ph.ppq += n * ph.bpm / 60.0 / sr;
    }
    return out;
}

static float rms (const AudioBuffer<float>& b, int ch, int start, int len)
{
    double s = 0;
    for (int i = 0; i < len; ++i) { const float v = b.getSample (ch, start + i); s += v * v; }
    return (float) std::sqrt (s / jmax (1, len));
}

static void writePng (Component& c, const File& f)
{
    auto img = c.createComponentSnapshot (c.getLocalBounds(), true, 2.0f);
    f.deleteFile();
    PNGImageFormat png;
    if (auto os = f.createOutputStream()) png.writeImageToStream (img, *os);
    std::cout << "  screenshot: " << f.getFileName() << std::endl;
}

static void collectMissingTips (Component& c, StringArray& missing, const String& path)
{
    for (auto* ch : c.getChildren())
    {
        const bool interactive = dynamic_cast<Button*> (ch) || dynamic_cast<Slider*> (ch) || dynamic_cast<ComboBox*> (ch)
                              || dynamic_cast<tg::ui::Stepper*> (ch) || dynamic_cast<tg::ui::MidiDragButton*> (ch)
                              || dynamic_cast<tg::StepGrid*> (ch);
        if (interactive && ch->isVisible())
        {
            String tip;
            if (auto* tc = dynamic_cast<TooltipClient*> (ch)) tip = tc->getTooltip();
            if (tip.trim().isEmpty())
                missing.add (path + "/" + ch->getName() + " (" + String (typeid (*ch).name()) + ")");
        }
        collectMissingTips (*ch, missing, path + "/" + ch->getName());
    }
}

int main (int argc, char** argv)
{
    ScopedJuceInitialiser_GUI gui;
    const double sr = 48000.0;
    const File outDir = File::getCurrentWorkingDirectory().getChildFile ("test-output");
    outDir.createDirectory();

    TranceGateProcessor proc;
    FakePlayHead ph;
    proc.setPlayHead (&ph);
    proc.setPlayConfigDetails (2, 2, sr, 512);
    proc.prepareToPlay (sr, 512);
    setP (proc, "attack", 0.5f);
    setP (proc, "release", 5.0f);

    const auto& lib = tg::presetLibrary();
    std::cout << "Library: " << lib.size() << " rhythms in " << tg::presetCategories().size() << " categories" << std::endl;
    check (lib.size() >= 240, "library has 240+ rhythms (incl. Standard A-H)");
    check (tg::presetCategories()[0] == "Standard" && tg::presetCategories()[tg::presetCategories().size() - 1] == "Creative",
           "Standard is the first category, Creative the last");
    {
        bool ok = true;
        StringArray names;
        for (auto& pr : lib)
        {
            const double beats = pr.data.length * tg::resolutions()[(size_t) pr.data.resolution].beats;
            const double bars = beats / 4.0, perBar = 4.0 / beats;
            const bool whole = std::abs (bars - std::round (bars)) < 1e-6 || std::abs (perBar - std::round (perBar)) < 1e-6;
            if (pr.category != "Euclidean / Poly" && pr.category != "Creative" && ! whole) { ok = false; std::cout << "    not whole bars: " << pr.name << std::endl; }
            if (pr.category != "Standard")
            {
                if (names.contains (pr.name)) { ok = false; std::cout << "    duplicate: " << pr.name << std::endl; }
                names.add (pr.name);
            }
        }
        check (ok, "genre presets line up with the bar, names are unique");
    }

    //---------------------------------------------------------------- 1. gate renders as drawn
    std::cout << "\n1) Patterns render as written (VOL mode, 140 BPM)" << std::endl;
    for (auto name : { "Sunrise Gate", "Rhythm Stab 3-3-2", "Rage Chop", "Gallop Gate", "Anthem Legato", "2-Step Stabs", "Jersey Kick", "Seven Up" })
    {
        int idx = -1;
        for (int i = 0; i < (int) lib.size(); ++i) if (lib[(size_t) i].name == name && lib[(size_t) i].category != "Standard") { idx = i; break; }
        if (idx < 0) { check (false, String ("preset exists: ") + name); continue; }
        const auto& pr = lib[(size_t) idx];
        proc.patterns[0].set (pr.data);
        auto b = render (proc, ph, sr, 4);
        const double stepSec = tg::resolutions()[(size_t) pr.data.resolution].beats * 60.0 / ph.bpm;
        bool ok = true;
        for (int s = 0; s < pr.data.length; ++s)
        {
            if (pr.data.steps[(size_t) s].ratchet > 1) continue;
            const float g = b.getSample (0, (int) (((pr.data.length + s) + 0.3) * stepSec * sr));
            const float expected = pr.data.isOn (s) ? pr.data.steps[(size_t) s].level : 0.0f;
            if (std::abs (g - expected) > 0.05f) ok = false;
        }
        check (ok, String ("gain at 30% of each step matches the levels: ") + name);
    }

    //---------------------------------------------------------------- 2. ratchets / ties / stereo
    std::cout << "\n2) Ratchets, ties, stereo offset" << std::endl;
    {
        proc.patterns[0].set (tg::PatternData::fromString ("4..............."));
        auto b = render (proc, ph, sr, 2);
        const double stepSec = 0.25 * 60.0 / ph.bpm;
        int rises = 0; bool high = false;
        for (int i = 0; i < (int) (stepSec * sr); ++i) { const bool h = b.getSample (0, i) > 0.5f; if (h && ! high) ++rises; high = h; }
        check (rises == 4, "x4 ratchet = 4 hits inside one step");

        proc.patterns[0].set (tg::PatternData::fromString ("X---............"));
        b = render (proc, ph, sr, 2);
        float minG = 1.0f;
        for (int i = (int) (0.1 * stepSec * sr); i < (int) (3.6 * stepSec * sr); ++i) minG = jmin (minG, b.getSample (0, i));
        check (minG > 0.95f, "tied steps keep the gate open");

        proc.patterns[0].set (tg::PatternData::fromString ("X...X...X...X..."));
        setP (proc, "stereo", 2.0f);
        b = render (proc, ph, sr, 2);
        const int at2 = (int) ((16 + 2.3) * stepSec * sr), at0 = (int) ((16 + 0.3) * stepSec * sr);
        check (b.getSample (0, at2) < 0.05f && b.getSample (1, at2) > 0.95f && b.getSample (0, at0) > 0.95f, "right channel delayed by 2 steps");
        setP (proc, "stereo", 0.0f);
    }

    //---------------------------------------------------------------- 3. mix / depth
    std::cout << "\n3) Mix and Depth" << std::endl;
    {
        proc.patterns[0].set (tg::PatternData::fromString ("X..............."));
        setP (proc, "mix", 0.5f);
        auto b = render (proc, ph, sr, 2);
        const double stepSec = 0.25 * 60.0 / ph.bpm;
        const float onStep = b.getSample (0, (int) ((16 + 0.3) * stepSec * sr));
        const float closed = b.getSample (0, (int) ((16 + 5.5) * stepSec * sr));
        check (onStep > 0.95f && std::abs (closed - 0.5f) < 0.02f, "Mix 50%: full level on steps, half level between (" + String (onStep, 2) + " / " + String (closed, 2) + ")");
        setP (proc, "mix", 1.0f);
        setP (proc, "depth", 0.5f);
        b = render (proc, ph, sr, 2);
        check (std::abs (b.getSample (0, (int) ((16 + 5.5) * stepSec * sr)) - 0.5f) < 0.02f, "Depth 50%: closed gate at half level");
        setP (proc, "depth", 1.0f);
    }

    //---------------------------------------------------------------- 4. filter modes
    std::cout << "\n4) Filter gate modes" << std::endl;
    {
        proc.patterns[0].set (tg::PatternData::fromString ("X.......X......."));
        setP (proc, "cutoff", 150.0f);
        const double stepSec = 0.25 * 60.0 / ph.bpm;
        const int on0 = (int) ((16 + 0.2) * stepSec * sr), off0 = (int) ((16 + 4.0) * stepSec * sr), len = (int) (stepSec * 0.4 * sr);

        proc.selectMode (TranceGateProcessor::ModeLowPass);
        auto b = render (proc, ph, sr, 2, nullptr, 4000.0);
        const float onL = rms (b, 0, on0, len), offL = rms (b, 0, off0, len);
        check (onL > 0.5f && offL < 0.05f, "LPF: 4 kHz passes on steps, is filtered out between (" + String (onL, 3) + " / " + String (offL, 3) + ")");
        b = render (proc, ph, sr, 2, nullptr, 60.0);
        check (rms (b, 0, off0, len) > 0.4f, "LPF: low notes stay between steps (sound is muffled, not muted)");

        setP (proc, "cutoff", 4000.0f);
        proc.selectMode (TranceGateProcessor::ModeHighPass);
        b = render (proc, ph, sr, 2, nullptr, 80.0);
        const float onH = rms (b, 0, on0, len), offH = rms (b, 0, off0, len);
        check (onH > 0.5f && offH < 0.05f, "HPF: bass passes on steps, is removed between (" + String (onH, 3) + " / " + String (offH, 3) + ")");
        proc.selectMode (TranceGateProcessor::ModeVolume);
        setP (proc, "cutoff", 600.0f);
    }

    //---------------------------------------------------------------- 5. variations / presets / undo
    std::cout << "\n5) Variations A/B/C, presets, undo / redo" << std::endl;
    {
        const int sunrise = tg::presetIndex ("Standard", 0);
        proc.loadPreset (sunrise);
        setP (proc, "mix", 0.3f);
        const auto aPattern = proc.patterns[0].get().toString();
        proc.selectVariation (1);
        check (proc.getVariation() == 1, "switched to B");
        check (std::abs (proc.apvts.getRawParameterValue ("mix")->load() - 1.0f) < 1e-3f, "B has its own knobs (Mix 100%)");
        proc.loadPreset (tg::presetIndex ("Standard", 5));
        proc.selectVariation (0);
        check (std::abs (proc.apvts.getRawParameterValue ("mix")->load() - 0.3f) < 1e-3f, "back on A: Mix 30% restored");
        check (proc.patterns[0].get().toString() == aPattern, "A kept its pattern");
        check (proc.patterns[1].get().toString() == lib[(size_t) tg::presetIndex ("Standard", 5)].data.toString(), "B kept its pattern");

        const auto before = proc.patterns[0].get().toString();
        proc.loadPreset (tg::presetIndex ("Standard", 3));
        check (proc.patterns[0].get().toString() != before, "preset loaded into A");
        proc.undo();
        check (proc.patterns[0].get().toString() == before && proc.presetIndex[0].load() == sunrise, "undo restores the previous preset");
        proc.redo();
        check (proc.presetIndex[0].load() == tg::presetIndex ("Standard", 3), "redo re-applies it");

        auto edited = proc.currentPattern();
        edited.clear();
        proc.editPattern (edited, true);
        check (proc.presetEdited[0].load(), "editing marks the preset as edited (*)");
        proc.undo();
        check (! proc.presetEdited[0].load() && proc.currentPattern().toString() != edited.toString(), "undo of a pattern edit");
        setP (proc, "mix", 1.0f);
    }

    //---------------------------------------------------------------- 6. MIDI notes -> Standard
    std::cout << "\n6) MIDI C1..G1 load Standard A..H" << std::endl;
    {
        MidiBuffer m;
        m.addEvent (MidiMessage::noteOn (1, 36 + 3, (uint8) 100), 0);
        render (proc, ph, sr, 1, &m);
        MessageManager::getInstance()->runDispatchLoopUntil (50);
        check (proc.presetIndex[0].load() == tg::presetIndex ("Standard", 3), "note D#1 -> Standard D (Gallop Gate) in the current variation");
    }

    //---------------------------------------------------------------- 7. state
    std::cout << "\n7) Save / restore" << std::endl;
    {
        auto special = tg::PatternData::fromString ("X-x.2o.3,..4X---", tg::R16T);
        proc.selectVariation (2);
        proc.editPattern (special, true);
        setP (proc, "gate", 0.4f);
        proc.selectMode (TranceGateProcessor::ModeLowPass);
        proc.selectVariation (0);
        MemoryBlock mb;
        proc.getStateInformation (mb);
        TranceGateProcessor other;
        other.setStateInformation (mb.getData(), (int) mb.getSize());
        check (other.patterns[2].get().toString() == special.toString(), "variation C pattern survives save/load");
        check (other.presetIndex[0].load() == proc.presetIndex[0].load(), "preset names survive save/load");
        other.selectVariation (2);
        check (std::abs (other.apvts.getRawParameterValue ("gate")->load() - 0.4f) < 1e-3f && other.getMode() == TranceGateProcessor::ModeLowPass,
               "variation C knobs + mode survive save/load");
        proc.selectVariation (0);
        proc.selectMode (TranceGateProcessor::ModeVolume);
    }

    //---------------------------------------------------------------- 8. signal view + MIDI export
    std::cout << "\n8) Signal view data, MIDI drag file" << std::endl;
    {
        proc.patterns[0].set (tg::PatternData::fromString ("X.X.X.X.X.X.X.X."));
        render (proc, ph, sr, 2, nullptr, 220.0);
        float in = 0, outOn = 0, outOff = 0;
        for (int b = 0; b < TranceGateProcessor::kSignalBins; ++b) in = jmax (in, proc.inPeaks[(size_t) b].load());
        const int bin0 = (int) (0.2 / 16.0 * TranceGateProcessor::kSignalBins), bin1 = (int) (1.6 / 16.0 * TranceGateProcessor::kSignalBins);
        outOn = proc.outPeaks[(size_t) bin0].load(); outOff = proc.outPeaks[(size_t) bin1].load();
        check (in > 0.9f && outOn > 0.9f && outOff < 0.05f, "signal bins: input recorded, output full on steps and silent between");

        auto pattern = tg::PatternData::fromString ("X.2.X-..4.......");
        auto f = tg::writePatternMidi (pattern, 0.75f, "Test Gate");
        MidiFile mf;
        FileInputStream fis (f);
        const bool read = mf.readFrom (fis);
        int notes = 0;
        if (read && mf.getNumTracks() > 0)
            for (auto* e : *mf.getTrack (0)) if (e->message.isNoteOn()) ++notes;
        check (read && notes == 1 + 2 + 1 + 4, "MIDI file: one note per hit, ratchets and ties respected (" + String (notes) + " notes)");
        ph.playing = false;
        auto b = render (proc, ph, sr, 1);
        int rises = 0; bool high = false;
        for (int i = 0; i < b.getNumSamples(); ++i) { const bool h = b.getSample (0, i) > 0.5f; if (h && ! high) ++rises; high = h; }
        check (rises >= 7, "gate keeps running with the transport stopped");
        ph.playing = true;
    }

    //---------------------------------------------------------------- 9. editor: tooltips + screenshots
    if (argc > 1 && String (argv[1]) == "--screenshot")
    {
        std::cout << "\n9) Editor" << std::endl;
        proc.selectVariation (0);
        proc.loadPreset (tg::presetIndex ("Standard", 2));   // Rage Chop
        // fill the signal view with a chord-like signal
        render (proc, ph, sr, 2, nullptr, 0.0);
        {
            const int total = (int) (2 * 4 * 60.0 / ph.bpm * sr);
            ph.ppq = 0;
            for (int pos = 0; pos < total; pos += 512)
            {
                AudioBuffer<float> buf (2, 512);
                for (int c = 0; c < 2; ++c)
                    for (int i = 0; i < 512; ++i)
                    {
                        const double t = (pos + i) / sr;
                        buf.setSample (c, i, (float) (0.35 * std::sin (2 * MathConstants<double>::pi * 220 * t) + 0.25 * std::sin (2 * MathConstants<double>::pi * 277 * t)
                                                     + 0.2 * std::sin (2 * MathConstants<double>::pi * 330 * t)) * (float) (0.8 + 0.2 * std::sin (t * 3.0)));
                    }
                MidiBuffer mm;
                proc.processBlock (buf, mm);
                ph.ppq += 512 * ph.bpm / 60.0 / sr;
            }
        }
        proc.currentStep = 6;
        std::unique_ptr<AudioProcessorEditor> ed (proc.createEditor());
        auto* editor = dynamic_cast<TranceGateEditor*> (ed.get());
        for (int i = 0; i < 4; ++i) MessageManager::getInstance()->runDispatchLoopUntil (40);

        StringArray missing;
        collectMissingTips (*ed, missing, "");
        for (auto& m : missing) std::cout << "    no tooltip: " << m << std::endl;
        check (missing.isEmpty(), "every control has a tooltip");

        writePng (*ed, outDir.getChildFile ("ui-main.png"));
        editor->showPresetBrowser (true);
        for (int i = 0; i < 2; ++i) MessageManager::getInstance()->runDispatchLoopUntil (40);
        writePng (*ed, outDir.getChildFile ("ui-presets.png"));
        editor->showPresetBrowser (false);

        proc.selectMode (TranceGateProcessor::ModeLowPass);
        setP (proc, "mix", 1.0f);
        editor->showFilterPanel (true);
        for (int i = 0; i < 3; ++i) MessageManager::getInstance()->runDispatchLoopUntil (40);
        writePng (*ed, outDir.getChildFile ("ui-filter.png"));
        proc.selectMode (TranceGateProcessor::ModeVolume);
    }

    std::cout << "\n" << (failures == 0 ? "ALL TESTS PASSED" : String (failures) + " TEST(S) FAILED") << std::endl;
    return failures == 0 ? 0 : 1;
}
