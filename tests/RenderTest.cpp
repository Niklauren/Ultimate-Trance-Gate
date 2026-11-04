// Offline test: runs the processor with a fake host transport, prints the gate
// shape per step, checks state save/restore, MIDI pattern switching and stereo
// offset, writes WAV files and (optionally) a PNG screenshot of the editor.

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "Presets.h"

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

// Render 'bars' bars of a constant DC signal; returns gain curve (L,R)
static AudioBuffer<float> render (TranceGateProcessor& proc, FakePlayHead& ph, double sr, int bars, MidiBuffer* firstMidi = nullptr)
{
    const int block = 512;
    const double bpm = ph.bpm;
    const int total = (int) std::round (bars * 4.0 * 60.0 / bpm * sr);
    AudioBuffer<float> out (2, total);
    MidiBuffer midi;
    ph.ppq = 0.0;
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = jmin (block, total - pos);
        AudioBuffer<float> buf (2, n);
        for (int c = 0; c < 2; ++c) for (int i = 0; i < n; ++i) buf.setSample (c, i, 1.0f);
        midi.clear();
        if (pos == 0 && firstMidi != nullptr) midi = *firstMidi;
        proc.processBlock (buf, midi);
        for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, buf, c, 0, n);
        ph.ppq += n * bpm / 60.0 / sr;
    }
    return out;
}

static void printSteps (const AudioBuffer<float>& b, const tg::PatternData& p, double sr, double bpm, int ch = 0)
{
    const double stepSec = tg::resolutions()[(size_t) p.resolution].beats * 60.0 / bpm;
    String line1, line2;
    for (int s = 0; s < p.length; ++s)
    {
        // average gain in the first half of the step, from the second loop
        const double loopSec = stepSec * p.length;
        const int start = (int) ((loopSec + s * stepSec) * sr);
        const int len = (int) (stepSec * 0.5 * sr);
        if (start + len >= b.getNumSamples()) break;
        double sum = 0;
        for (int i = 0; i < len; ++i) sum += b.getSample (ch, start + i);
        const double avg = sum / len;
        line1 << (avg > 0.75 ? "#" : avg > 0.4 ? "+" : avg > 0.1 ? "-" : ".");
    }
    line2 = p.toString();
    std::cout << "      pattern : " << line2 << "\n      rendered: " << line1 << std::endl;
}

static void writeWav (const AudioBuffer<float>& b, double sr, const File& f)
{
    f.deleteFile();
    WavAudioFormat wav;
    std::unique_ptr<OutputStream> os (f.createOutputStream().release());
    if (auto* w = wav.createWriterFor (os.get(), sr, (unsigned) b.getNumChannels(), 24, {}, 0))
    {
        os.release();
        std::unique_ptr<AudioFormatWriter> writer (w);
        writer->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
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

    // short envelope so the step boundaries are crisp
    proc.apvts.getParameter ("attack")->setValueNotifyingHost (proc.apvts.getParameter ("attack")->convertTo0to1 (0.5f));
    proc.apvts.getParameter ("release")->setValueNotifyingHost (proc.apvts.getParameter ("release")->convertTo0to1 (5.0f));

    std::cout << "Library: " << tg::presetLibrary().size() << " rhythms in "
              << tg::presetCategories().size() << " categories" << std::endl;
    check (tg::presetLibrary().size() >= 230, "preset library has 230+ rhythms");
    {
        bool allBars = true;
        juce::StringArray names;
        for (auto& pr : tg::presetLibrary())
        {
            const double beats = pr.data.length * tg::resolutions()[(size_t) pr.data.resolution].beats;
            const double bars = beats / 4.0;
            const double perBar = 4.0 / beats;   // loops that divide the bar evenly are fine too
            const bool wholeBars = std::abs (bars - std::round (bars)) < 1.0e-6 || std::abs (perBar - std::round (perBar)) < 1.0e-6;
            if (pr.category != "Euclidean / Poly" && pr.category != "Creative" && ! wholeBars)
            {
                allBars = false;
                std::cout << "    not whole bars: " << pr.category << " / " << pr.name << " (" << beats << " beats)" << std::endl;
            }
            if (names.contains (pr.name)) { allBars = false; std::cout << "    duplicate name: " << pr.name << std::endl; }
            names.add (pr.name);
        }
        check (allBars, "every genre preset lines up with the bar, names are unique");
        for (auto& c : tg::presetCategories())
        {
            int n = 0;
            for (auto& pr : tg::presetLibrary()) if (pr.category == c) ++n;
            std::cout << "    " << c << ": " << n << std::endl;
        }
    }

    std::cout << "\n1) Patterns render as written (140 BPM)" << std::endl;
    for (auto name : { "Sunrise Gate", "Rhythm Stab 3-3-2", "Rage Chop", "Gallop Gate", "Euclid 5/8 Cinquillo", "Anthem Legato", "2-Step Stabs", "Jersey Kick" })
    {
        for (auto& pr : tg::presetLibrary())
            if (pr.name == name)
            {
                proc.patterns[0].set (pr.data);
                std::cout << "   " << name << std::endl;
                auto b = render (proc, ph, sr, 4);
                printSteps (b, pr.data, sr, ph.bpm);
                writeWav (b, sr, outDir.getChildFile (String (name).replaceCharacters (" (),", "____") + ".wav"));

                // compare: every "on" step that is not tied-in must have a gain dip right before it (unless previous step tied)
                bool ok = true;
                const double stepSec = tg::resolutions()[(size_t) pr.data.resolution].beats * 60.0 / ph.bpm;
                for (int s = 0; s < pr.data.length; ++s)
                {
                    const double t = (pr.data.length + s + 0.3) * stepSec;
                    const float g = b.getSample (0, (int) (t * sr));
                    const float expected = pr.data.isOn (s) ? pr.data.steps[(size_t) s].level : 0.0f;
                    if (pr.data.steps[(size_t) s].ratchet > 1) continue;  // sub-steps, skip simple check
                    if (std::abs (g - expected) > 0.05f) ok = false;
                }
                check (ok, String ("gain at 30% of each step matches the pattern levels: ") + name);
            }
    }

    std::cout << "\n2) Ratchets produce extra hits" << std::endl;
    {
        auto p = tg::PatternData::fromString ("4...............");
        proc.patterns[0].set (p);
        auto b = render (proc, ph, sr, 2);
        const double stepSec = 0.25 * 60.0 / ph.bpm;
        int rises = 0; bool high = false;
        for (int i = 0; i < (int) (stepSec * sr); ++i)
        {
            const bool h = b.getSample (0, i) > 0.5f;
            if (h && ! high) ++rises;
            high = h;
        }
        check (rises == 4, "x4 ratchet = 4 hits inside one step (got " + String (rises) + ")");
    }

    std::cout << "\n3) Ties keep the gate open across steps" << std::endl;
    {
        auto p = tg::PatternData::fromString ("X---............");
        proc.patterns[0].set (p);
        auto b = render (proc, ph, sr, 2);
        const double stepSec = 0.25 * 60.0 / ph.bpm;
        float minG = 1.0f;
        for (int i = (int) (0.1 * stepSec * sr); i < (int) (3.6 * stepSec * sr); ++i)
            minG = jmin (minG, b.getSample (0, i));
        check (minG > 0.95f, "gain stays open over 4 tied steps (min " + String (minG, 3) + ")");
    }

    std::cout << "\n4) Stereo offset" << std::endl;
    {
        proc.patterns[0].set (tg::PatternData::fromString ("X...X...X...X..."));
        auto* st = proc.apvts.getParameter ("stereo");
        st->setValueNotifyingHost (st->convertTo0to1 (2.0f));
        auto b = render (proc, ph, sr, 2);
        const double stepSec = 0.25 * 60.0 / ph.bpm;
        const int at0 = (int) (0.3 * stepSec * sr), at2 = (int) (2.3 * stepSec * sr);
        check (b.getSample (0, at2 + (int) (16 * stepSec * sr)) < 0.05f && b.getSample (1, at2 + (int) (16 * stepSec * sr)) > 0.95f
               && b.getSample (0, at0 + (int) (16 * stepSec * sr)) > 0.95f,
               "right channel is delayed by 2 steps");
        writeWav (b, sr, outDir.getChildFile ("stereo_offset.wav"));
        st->setValueNotifyingHost (st->convertTo0to1 (0.0f));
    }

    std::cout << "\n5) MIDI note switches pattern" << std::endl;
    {
        MidiBuffer m;
        m.addEvent (MidiMessage::noteOn (1, 36 + 3, (uint8) 100), 0);
        render (proc, ph, sr, 1, &m);
        check (proc.activePattern.load() == 3, "note D#1 -> pattern D");
    }

    std::cout << "\n6) Depth / Mix" << std::endl;
    {
        proc.patterns[3].set (tg::PatternData::fromString ("X..............."));
        auto* d = proc.apvts.getParameter ("depth");
        d->setValueNotifyingHost (0.5f);
        auto b = render (proc, ph, sr, 2);
        const double stepSec = 0.25 * 60.0 / ph.bpm;
        const float closed = b.getSample (0, (int) (5.5 * stepSec * sr));
        check (std::abs (closed - 0.5f) < 0.02f, "depth 50% -> closed gate at 0.5 (" + String (closed, 3) + ")");
        d->setValueNotifyingHost (1.0f);
    }

    std::cout << "\n7) Save / restore state" << std::endl;
    {
        auto special = tg::PatternData::fromString ("X-x.2o.3,..4X---", tg::R16T);
        proc.patterns[5].set (special);
        MemoryBlock mb;
        proc.getStateInformation (mb);

        TranceGateProcessor other;
        other.setStateInformation (mb.getData(), (int) mb.getSize());
        const auto r = other.patterns[5].get();
        check (r.toString() == special.toString() && r.resolution == tg::R16T,
               "pattern F survives save/load: " + r.toString());
        check (std::abs (other.apvts.getRawParameterValue ("attack")->load() - 0.5f) < 0.01f, "parameters survive save/load");
    }

    std::cout << "\n8) Transport stopped -> free-running" << std::endl;
    {
        ph.playing = false;
        proc.patterns[3].set (tg::PatternData::fromString ("X.X.X.X.X.X.X.X."));
        auto b = render (proc, ph, sr, 1);
        int rises = 0; bool high = false;
        for (int i = 0; i < b.getNumSamples(); ++i) { bool h = b.getSample (0, i) > 0.5f; if (h && ! high) ++rises; high = h; }
        check (rises >= 7, "gate keeps running without transport (" + String (rises) + " hits in a bar)");
        ph.playing = true;
    }

    //---------------------------------------------------------------- screenshot
    if (argc > 1 && String (argv[1]) == "--screenshot")
    {
        proc.activePattern = 1;
        for (auto& pr : tg::presetLibrary())
            if (pr.name == "Rage Chop") proc.patterns[1].set (pr.data);
        proc.currentStep = 5;
        std::unique_ptr<AudioProcessorEditor> ed (proc.createEditor());
        ed->setSize (960, 620);
        // let the timer-driven refresh happen once
        for (int i = 0; i < 5; ++i) MessageManager::getInstance()->runDispatchLoopUntil (40);
        std::function<void(Component*)> dump = [&] (Component* c)
        {
            if (auto* l = dynamic_cast<Label*> (c))
                if (getenv ("TG_DEBUG")) std::cout << "label '" << l->getText() << "' outline=" << l->findColour (Label::outlineColourId).toDisplayString (true)
                          << " hasOwn=" << (int) l->isColourSpecified (Label::outlineColourId) << " lnf=" << (void*) &l->getLookAndFeel() << std::endl;
            for (auto* ch : c->getChildren()) dump (ch);
        };
        dump (ed.get());
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);

        // Draw the ATTACK tooltip on top, the way it appears after a 2 s hover
        {
            String tip;
            for (auto* c : ed->getChildren())
                if (auto* s = dynamic_cast<Slider*> (c))
                    if (s->getTooltip().startsWith ("ATTACK")) { tip = s->getTooltip(); break; }
            auto& lf = ed->getLookAndFeel();
            const auto tb = lf.getTooltipBounds (tip, { 60, 520 }, ed->getLocalBounds());
            Graphics g (img);
            g.addTransform (AffineTransform::scale (2.0f).translated ((float) tb.getX() * 2.0f, (float) tb.getY() * 2.0f));
            lf.drawTooltip (g, tip, tb.getWidth(), tb.getHeight());
            std::cout << "Tooltip size: " << tb.getWidth() << "x" << tb.getHeight() << std::endl;
        }
        PNGImageFormat png;
        File f = outDir.getChildFile ("screenshot.png");
        f.deleteFile();
        if (auto os = f.createOutputStream()) png.writeImageToStream (img, *os);
        std::cout << "\nScreenshot: " << f.getFullPathName() << std::endl;
    }

    if (argc > 1 && String (argv[argc - 1]) == "--presets-md")
    {
        String md;
        md << "# Ultimate Trance Gate - preset library\n\n"
           << "`X`=100% `x`=70% `o`=40% `,`=20% `.`=off `-`=tie `2/3/4`=ratchet\n";
        for (auto& c : tg::presetCategories())
        {
            md << "\n## " << c << "\n\n| Preset | Grid | Pattern |\n|---|---|---|\n";
            for (auto& pr : tg::presetLibrary())
                if (pr.category == c)
                    md << "| " << pr.name << " | " << pr.data.length << " x " << tg::resolutions()[(size_t) pr.data.resolution].name
                       << " | `" << pr.data.toString() << "` |\n";
        }
        File f = File::getCurrentWorkingDirectory().getChildFile ("docs/PRESETS.md");
        f.replaceWithText (md);
        std::cout << "Preset list: " << f.getFullPathName() << std::endl;
    }

    std::cout << "\n" << (failures == 0 ? "ALL TESTS PASSED" : String (failures) + " TEST(S) FAILED") << std::endl;
    return failures == 0 ? 0 : 1;
}
