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
#include "GateEngineOld.h"   // frozen v2.0 engine, only for the before/after comparison

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

// High-frequency splatter of a gated 220 Hz sine: energy above 2 kHz relative to the total, in dB.
// Pure gating artefacts (clicks, hard corners) are what lands up there.
template <typename Voice, typename Params>
static double splatterDb (const Params& gp, const tg::PatternData& pat, double sr = 48000.0, double bpm = 140.0)
{
    Voice v;
    v.prepare (sr);
    v.setParams (gp);
    const int order = 17, N = 1 << order;
    std::vector<float> buf ((size_t) N * 2, 0.0f);
    for (int n = 0; n < N; ++n)
    {
        const double beat = n * bpm / 60.0 / sr;
        const float g = v.process (beat, pat);
        const double w = 0.5 - 0.5 * std::cos (2.0 * MathConstants<double>::pi * n / (N - 1));
        buf[(size_t) n] = (float) (std::sin (2.0 * MathConstants<double>::pi * 220.0 * n / sr) * g * w);
    }
    dsp::FFT fft (order);
    fft.performFrequencyOnlyForwardTransform (buf.data());
    double hi = 0, all = 0;
    for (int k = 1; k < N / 2; ++k)
    {
        const double e = (double) buf[(size_t) k] * buf[(size_t) k];
        all += e;
        if (k * sr / N > 2000.0) hi += e;
    }
    return 10.0 * std::log10 (hi / all + 1e-30);
}

static void writePng (Component& c, const File& f)
{
    auto img = c.createComponentSnapshot (c.getLocalBounds(), true, 2.0f);
    f.deleteFile();
    PNGImageFormat png;
    if (auto os = f.createOutputStream()) png.writeImageToStream (img, *os);
    std::cout << "  screenshot: " << f.getFileName() << std::endl;
}

static void collectTips (Component& c, StringArray& tips)
{
    for (auto* ch : c.getChildren())
    {
        if (auto* tc = dynamic_cast<TooltipClient*> (ch))
            if (ch->isVisible() && tc->getTooltip().isNotEmpty()) tips.addIfNotAlreadyThere (tc->getTooltip());
        collectTips (*ch, tips);
    }
}

// Every tooltip drawn exactly as the plug-in draws it, stacked in two columns.
static void writeTipSheet (Component& editor, const StringArray& tips, const File& f)
{
    auto& lf = editor.getLookAndFeel();
    const Rectangle<int> parent (0, 0, 4000, 4000);
    Array<Rectangle<int>> sizes;
    for (auto& t : tips) sizes.add (lf.getTooltipBounds (t, {}, parent));
    const int colW = 520, gap = 10;
    int y[2] = { gap, gap };
    Array<Point<int>> at;
    for (auto& r : sizes) { const int c = y[0] <= y[1] ? 0 : 1; at.add ({ gap + c * (colW + gap), y[c] }); y[c] += r.getHeight() + gap; }
    Image img (Image::ARGB, 2 * (colW + gap) + gap, jmax (y[0], y[1]), true);
    Graphics g (img);
    g.fillAll (Colour (0xff22101a));
    for (int i = 0; i < tips.size(); ++i)
    {
        Graphics::ScopedSaveState s (g);
        g.setOrigin (at[i]);
        lf.drawTooltip (g, tips[i], sizes[i].getWidth(), sizes[i].getHeight());
    }
    f.deleteFile();
    PNGImageFormat png;
    if (auto os = f.createOutputStream()) png.writeImageToStream (img, *os);
    std::cout << "  tooltips: " << tips.size() << " -> " << f.getFileName() << std::endl;
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
    // sections 1-8 check exact levels: hard edges, full sustain, gate closing at 75 %
    auto testEnvelope = [&proc]
    {
        setP (proc, "character", 0.0f);
        setP (proc, "attack", 0.5f);
        setP (proc, "decay", 150.0f);
        setP (proc, "release", 5.0f);
        setP (proc, "sustain", 1.0f);
        setP (proc, "gate", 0.75f);
    };
    testEnvelope();

    const auto& lib = tg::presetLibrary();
    std::cout << "Library: " << lib.size() << " rhythms in " << tg::presetCategories().size() << " categories" << std::endl;
    check (lib.size() >= 240, "library has 240+ rhythms (incl. Standard)");
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

    testEnvelope();   // presets loaded above brought their own envelopes

    //---------------------------------------------------------------- 6. MIDI notes -> Standard
    std::cout << "\n6) MIDI C1..G1 load the Standard rhythms" << std::endl;
    {
        MidiBuffer m;
        m.addEvent (MidiMessage::noteOn (1, 36 + 3, (uint8) 100), 0);
        render (proc, ph, sr, 1, &m);
        MessageManager::getInstance()->runDispatchLoopUntil (50);
        check (proc.presetIndex[0].load() == tg::presetIndex ("Standard", 3), "note D#1 -> 4th Standard rhythm in the current variation");
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
        proc.loadPreset (tg::presetIndexByName ("Rage", "Rage Chop"));
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
        {
            StringArray tips;
            collectTips (*ed, tips);
            editor->showFilterPanel (true);   // filter panel tips too
            proc.selectMode (TranceGateProcessor::ModeLowPass);
            for (int i = 0; i < 2; ++i) MessageManager::getInstance()->runDispatchLoopUntil (40);
            collectTips (*ed, tips);
            proc.selectMode (TranceGateProcessor::ModeVolume);
            editor->showFilterPanel (false);
            for (int i = 0; i < 2; ++i) MessageManager::getInstance()->runDispatchLoopUntil (40);
            for (auto t : { tg::tips::ratchetLane(), tg::tips::tieLane(), tg::tips::signal(), tg::tips::star() })
                tips.addIfNotAlreadyThere (t);
            writeTipSheet (*ed, tips, outDir.getChildFile ("tooltips.png"));
        }
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

    //---------------------------------------------------------------- 10. smoothness vs v2.0 engine
    std::cout << "\n10) Gate smoothness (energy above 2 kHz on a gated 220 Hz sine; lower = smoother)" << std::endl;
    {
        const auto classic = lib[(size_t) tg::presetIndexByName ("Trance", "Classic Gate")].data;
        check (classic.length == 16 && classic.resolution == tg::R32 && classic.toString() == "X.X.X--.X.X.X--.",
               "Classic Gate = Kilohearts start-up pattern (16 x 1/32)");
        TranceGateProcessor fresh;
        check (fresh.patterns[0].get().toString() == classic.toString(), "a new instance starts on Classic Gate (variation A)");
        {
            const char* original[] = { "Sunrise Gate", "Rhythm Stab 3-3-2", "Rage Chop", "Gallop Gate",
                                       "KBBB Roll", "Supersaw Stutter", "3-3-2 Chords", "Build-Up 32" };
            bool same = true;
            for (int i = 0; i < 8; ++i) same = same && lib[(size_t) tg::presetIndex ("Standard", i)].name == original[i];
            check (same && tg::presetIndex ("Standard", 8) < 0, "Standard folder is the original 8 (A-H for MIDI C1..G1)");
        }
        {
            // presets bring their envelope; loading one sets Character + ADSR + Gate, nothing else
            TranceGateProcessor q;
            setP (q, "mix", 0.7f);
            q.loadPreset (tg::presetIndexByName ("Rage", "Rage Chop"));
            const auto e = tg::presetEnvelope ("Rage", "Rage Chop");
            check ((int) q.apvts.getRawParameterValue ("character")->load() == tg::CharHard
                     && std::abs (q.apvts.getRawParameterValue ("release")->load() - e.releaseMs) < 0.01f
                     && std::abs (q.apvts.getRawParameterValue ("mix")->load() - 0.7f) < 1e-3f,
                   "Rage Chop loads Hard character + its ADSR, keeps Mix");
            q.undo();
            check ((int) q.apvts.getRawParameterValue ("character")->load() == tg::CharSmooth
                     && std::abs (q.apvts.getRawParameterValue ("sustain")->load() - 0.5f) < 1e-3f,
                   "one undo brings back the previous pattern and envelope");
            q.loadPreset (tg::presetIndexByName ("Trance", "Breakdown Swell"));
            check ((int) q.apvts.getRawParameterValue ("character")->load() == tg::CharPump, "Breakdown Swell -> Pump character");
            // every preset gets a sane envelope
            bool ok = true;
            std::map<int, int> perChar;
            for (auto& pr : lib)
            {
                const auto en = tg::presetEnvelope (pr);
                ok = ok && en.attackMs >= 0.1f && en.attackMs <= 300.0f && en.releaseMs >= 1.0f && en.gate >= 0.05f && en.gate <= 1.0f
                        && en.sustain >= 0.0f && en.sustain <= 1.0f && en.character >= 0 && en.character < tg::kNumCharacters;
                ++perChar[en.character];
            }
            check (ok, "all " + String ((int) lib.size()) + " presets have an envelope in range");
            std::cout << "    by character: Smooth " << perChar[0] << ", Hard " << perChar[1] << ", Soft " << perChar[2] << ", Pump " << perChar[3] << std::endl;
        }
        {
            // the four characters really differ
            tg::PatternData p = tg::PatternData::fromString ("X...X...X...X...");
            double prevE = -1.0;
            bool differ = true;
            for (int c = 0; c < tg::kNumCharacters; ++c)
            {
                tg::GateParams gp; gp.character = c; gp.attackMs = 20.0f; gp.releaseMs = 40.0f; gp.gate = 0.5f; gp.sustain = 1.0f;
                tg::GateVoice v; v.prepare (48000.0); v.setParams (gp);
                double e = 0;   // energy of the first 10 ms of a hit
                for (int n = 0; n < 480; ++n) { const float g = v.process (n * 2.0 / 48000.0, p); e += g * g; }
                std::cout << "    " << tg::characterNames()[c] << ": first 10 ms energy " << String (e, 1) << std::endl;
                differ = differ && std::abs (e - prevE) > 1.0;
                prevE = e;
            }
            check (differ, "Smooth / Hard / Soft / Pump attacks are audibly different");
        }

        tg_old::GateParams oldDefaults;                       // A 1 ms, R 25 ms, Gate 75 %, Sustain 100 %
        tg::GateParams sameKnobs; sameKnobs.character = tg::CharSmooth; sameKnobs.attackMs = 1.0f; sameKnobs.releaseMs = 25.0f; sameKnobs.gate = 0.75f; sameKnobs.sustain = 1.0f;
        tg::GateParams newDefaults;                            // A 4 ms, S 50 %, R 40 ms, Gate 100 %, Smooth
        const double a = splatterDb<tg_old::GateVoice> (oldDefaults, classic);
        const double b = splatterDb<tg::GateVoice> (sameKnobs, classic);
        const double c = splatterDb<tg::GateVoice> (newDefaults, classic);
        std::cout << "    v2.0 engine, old defaults : " << String (a, 1) << " dB" << std::endl;
        std::cout << "    new engine, same knobs    : " << String (b, 1) << " dB" << std::endl;
        std::cout << "    new engine, new defaults  : " << String (c, 1) << " dB" << std::endl;
        check (b < a - 3.0, "new envelope is smoother with the same knobs");
        check (c < a - 10.0, "new defaults are much smoother than the old ones");

        // a quiet step after a loud one used to jump down in one sample
        tg::PatternData p; p.length = 2; p.resolution = tg::R16; p.steps[0].level = 1.0f; p.steps[1].level = 0.3f;
        tg::GateParams gp; gp.gate = 1.0f; gp.attackMs = 5.0f; gp.sustain = 1.0f;
        tg::GateVoice v; v.prepare (48000.0); v.setParams (gp);
        float prev = 0.0f, maxJump = 0.0f;
        for (int n = 1; n < 48000; ++n) { const float g = v.process (n * 120.0 / 60.0 / 48000.0, p); if (n > 2000) maxJump = jmax (maxJump, std::abs (g - prev)); prev = g; }
        check (maxJump < 0.01f, "no sample-to-sample jumps between steps of different level (max " + String (maxJump, 4) + ")");
    }

    //---------------------------------------------------------------- docs/PRESETS.md
    {
        String md;
        md << "# Ultimate Trance Gate — preset library\n\n"
           << "`X`=100% `x`=70% `o`=40% `,`=20% `.`=off `-`=tie `2/3/4`=ratchet\n\n"
           << "Each preset also sets the envelope: Character, Attack, Decay, Sustain, Release, Gate.\n";
        String cat;
        for (auto& pr : lib)
        {
            if (pr.category != cat)
            {
                cat = pr.category;
                md << "\n## " << cat << "\n\n| Preset | Grid | Pattern | Character | A / D / S / R | Gate |\n|---|---|---|---|---|---|\n";
            }
            const auto e = tg::presetEnvelope (pr);
            md << "| " << pr.name << " | " << pr.data.length << " x " << tg::resolutions()[(size_t) pr.data.resolution].name
               << " | `" << pr.data.toString() << "` | " << tg::characterNames()[e.character]
               << " | " << String (e.attackMs, e.attackMs < 1.0f ? 1 : 0) << " ms / " << (int) e.decayMs << " ms / "
               << roundToInt (e.sustain * 100.0f) << "% / " << (int) e.releaseMs << " ms | " << roundToInt (e.gate * 100.0f) << "% |\n";
        }
        outDir.getChildFile ("PRESETS.md").replaceWithText (md);
    }

    std::cout << "\n" << (failures == 0 ? "ALL TESTS PASSED" : String (failures) + " TEST(S) FAILED") << std::endl;
    return failures == 0 ? 0 : 1;
}
