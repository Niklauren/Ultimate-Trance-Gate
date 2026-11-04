#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

using namespace juce;

namespace ids
{
    static const ParameterID variation { "variation", 2 };
    static const ParameterID mode      { "mode",      2 };
    static const ParameterID attack    { "attack",    1 };
    static const ParameterID decay     { "decay",     1 };
    static const ParameterID sustain   { "sustain",   1 };
    static const ParameterID release   { "release",   1 };
    static const ParameterID gate      { "gate",      1 };
    static const ParameterID swing     { "swing",     1 };
    static const ParameterID depth     { "depth",     1 };
    static const ParameterID stereo    { "stereo",    1 };
    static const ParameterID mix       { "mix",       1 };
    static const ParameterID cutoff    { "cutoff",    2 };
    static const ParameterID reso      { "reso",      2 };
}

//==============================================================================
static NormalisableRange<float> skewed (float lo, float hi, float centre)
{
    NormalisableRange<float> r (lo, hi);
    r.setSkewForCentre (centre);
    return r;
}

static AudioParameterFloatAttributes msAttr()
{
    return AudioParameterFloatAttributes().withLabel ("ms")
        .withStringFromValueFunction ([] (float v, int) { return v < 10.0f ? String (v, 1) + " ms" : String (roundToInt (v)) + " ms"; });
}

static AudioParameterFloatAttributes pctAttr()
{
    return AudioParameterFloatAttributes().withLabel ("%")
        .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.0f)) + " %"; })
        .withValueFromStringFunction ([] (const String& s) { return s.getFloatValue() / 100.0f; });
}

static AudioParameterFloatAttributes hzAttr()
{
    return AudioParameterFloatAttributes().withLabel ("Hz")
        .withStringFromValueFunction ([] (float v, int) { return v >= 1000.0f ? String (v / 1000.0f, 1) + " kHz" : String (roundToInt (v)) + " Hz"; });
}

AudioProcessorValueTreeState::ParameterLayout TranceGateProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterChoice> (ids::variation, "Variation", StringArray { "A", "B", "C" }, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ids::mode, "Gate Mode", StringArray { "Volume", "Low-pass", "High-pass" }, 0));

    layout.add (std::make_unique<AudioParameterFloat> (ids::attack,  "Attack",  skewed (0.1f, 300.0f, 15.0f),  1.0f,   msAttr()));
    layout.add (std::make_unique<AudioParameterFloat> (ids::decay,   "Decay",   skewed (1.0f, 2000.0f, 200.0f), 150.0f, msAttr()));
    layout.add (std::make_unique<AudioParameterFloat> (ids::sustain, "Sustain", NormalisableRange<float> (0.0f, 1.0f), 1.0f,  pctAttr()));
    layout.add (std::make_unique<AudioParameterFloat> (ids::release, "Release", skewed (1.0f, 1000.0f, 80.0f),  25.0f,  msAttr()));
    layout.add (std::make_unique<AudioParameterFloat> (ids::gate,    "Gate",    NormalisableRange<float> (0.05f, 1.0f), 0.75f, pctAttr()));
    layout.add (std::make_unique<AudioParameterFloat> (ids::swing,   "Swing",   NormalisableRange<float> (0.0f, 1.0f), 0.0f,   pctAttr()));
    layout.add (std::make_unique<AudioParameterFloat> (ids::depth,   "Depth",   NormalisableRange<float> (0.0f, 1.0f), 1.0f,   pctAttr()));
    layout.add (std::make_unique<AudioParameterInt>   (ids::stereo,  "Stereo Offset", 0, 8, 0,
                AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return v == 0 ? String ("Off") : String (v) + " st"; })));
    layout.add (std::make_unique<AudioParameterFloat> (ids::mix,     "Mix",     NormalisableRange<float> (0.0f, 1.0f), 1.0f,   pctAttr()));
    layout.add (std::make_unique<AudioParameterFloat> (ids::cutoff,  "Filter Cutoff", skewed (20.0f, 20000.0f, 1000.0f), 600.0f, hzAttr()));
    layout.add (std::make_unique<AudioParameterFloat> (ids::reso,    "Filter Resonance", NormalisableRange<float> (0.0f, 1.0f), 0.3f, pctAttr()));
    return layout;
}

const StringArray& TranceGateProcessor::variationParamIds()
{
    static const StringArray list { "mode", "attack", "decay", "sustain", "release", "gate",
                                    "swing", "depth", "stereo", "mix", "cutoff", "reso" };
    return list;
}

//==============================================================================
TranceGateProcessor::TranceGateProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  AudioChannelSet::stereo(), true)
                        .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    pVariation = apvts.getRawParameterValue ("variation");
    pMode    = apvts.getRawParameterValue ("mode");
    pAttack  = apvts.getRawParameterValue ("attack");
    pDecay   = apvts.getRawParameterValue ("decay");
    pSustain = apvts.getRawParameterValue ("sustain");
    pRelease = apvts.getRawParameterValue ("release");
    pGate    = apvts.getRawParameterValue ("gate");
    pSwing   = apvts.getRawParameterValue ("swing");
    pDepth   = apvts.getRawParameterValue ("depth");
    pStereo  = apvts.getRawParameterValue ("stereo");
    pMix     = apvts.getRawParameterValue ("mix");
    pCutoff  = apvts.getRawParameterValue ("cutoff");
    pReso    = apvts.getRawParameterValue ("reso");

    // Factory start-up: A = Rage Chop, B = Rhythm Stab 3-3-2, C = Supersaw Stutter
    const int starts[] = { 2, 1, 5 };
    for (int v = 0; v < kNumVariations; ++v)
    {
        const int idx = tg::presetIndex ("Standard", starts[v]);
        presetIndex[(size_t) v] = idx;
        presetEdited[(size_t) v] = false;
        if (idx >= 0) patterns[(size_t) v].set (tg::presetLibrary()[(size_t) idx].data);
        storeKnobsInto (v);
    }

    apvts.addParameterListener ("variation", this);
    for (auto* p : getParameters())
        p->addListener (this);
}

TranceGateProcessor::~TranceGateProcessor()
{
    cancelPendingUpdate();
    apvts.removeParameterListener ("variation", this);
    for (auto* p : getParameters())
        p->removeListener (this);
}

bool TranceGateProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != AudioChannelSet::mono() && out != AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

tg::GateParams TranceGateProcessor::getGateParams() const
{
    tg::GateParams g;
    g.attackMs  = pAttack->load();
    g.decayMs   = pDecay->load();
    g.sustain   = pSustain->load();
    g.releaseMs = pRelease->load();
    g.gate      = pGate->load();
    g.swing     = pSwing->load();
    return g;
}

//==============================================================================
// Variations, presets, undo (message thread)
//==============================================================================
void TranceGateProcessor::setParam (const String& id, float plainValue)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
}

void TranceGateProcessor::storeKnobsInto (int v)
{
    for (auto& id : variationParamIds())
        knobSnapshots[(size_t) v][id] = apvts.getRawParameterValue (id)->load();
}

void TranceGateProcessor::recallKnobsFrom (int v)
{
    const ScopedValueSetter<bool> svs (restoring, true);
    for (auto& [id, value] : knobSnapshots[(size_t) v])
        setParam (id, value);
}

void TranceGateProcessor::selectVariation (int v)
{
    v = jlimit (0, kNumVariations - 1, v);
    if (v == getVariation()) return;
    pushUndoPoint();
    {
        const ScopedValueSetter<bool> svs (restoring, true);
        setParam ("variation", (float) v);
    }
    cancelPendingUpdate();
    handleAsyncUpdate();
}

void TranceGateProcessor::selectMode (int m)
{
    if (m == getMode()) return;
    pushUndoPoint();
    const ScopedValueSetter<bool> svs (restoring, true);
    setParam ("mode", (float) m);
}

void TranceGateProcessor::editPattern (const tg::PatternData& p, bool newUndoPoint)
{
    if (newUndoPoint) pushUndoPoint();
    const int v = getVariation();
    patterns[(size_t) v].set (p);
    presetEdited[(size_t) v] = true;
    ++stateVersion;
}

void TranceGateProcessor::loadPreset (int libraryIndex)
{
    const auto& lib = tg::presetLibrary();
    if (libraryIndex < 0 || libraryIndex >= (int) lib.size()) return;
    pushUndoPoint();
    const int v = getVariation();
    patterns[(size_t) v].set (lib[(size_t) libraryIndex].data);
    presetIndex[(size_t) v] = libraryIndex;
    presetEdited[(size_t) v] = false;
    ++stateVersion;
}

TranceGateProcessor::Snapshot TranceGateProcessor::capture() const
{
    Snapshot s;
    for (int v = 0; v < kNumVariations; ++v)
    {
        s.pats[(size_t) v]   = patterns[(size_t) v].get();
        s.preset[(size_t) v] = presetIndex[(size_t) v].load();
        s.edited[(size_t) v] = presetEdited[(size_t) v].load();
    }
    s.params["variation"] = pVariation->load();
    for (auto& id : variationParamIds())
        s.params[id] = apvts.getRawParameterValue (id)->load();
    s.knobs = knobSnapshots;
    return s;
}

void TranceGateProcessor::restore (const Snapshot& s)
{
    const ScopedValueSetter<bool> svs (restoring, true);
    for (int v = 0; v < kNumVariations; ++v)
    {
        patterns[(size_t) v].set (s.pats[(size_t) v]);
        presetIndex[(size_t) v]  = s.preset[(size_t) v];
        presetEdited[(size_t) v] = s.edited[(size_t) v];
    }
    knobSnapshots = s.knobs;
    shownVariation = jlimit (0, kNumVariations - 1, (int) s.params.at ("variation"));
    for (auto& [id, value] : s.params)
        setParam (id, value);
    ++stateVersion;
}

void TranceGateProcessor::pushUndoPoint()
{
    if (restoring) return;
    undoStack.push_back (capture());
    if (undoStack.size() > 100) undoStack.erase (undoStack.begin());
    redoStack.clear();
}

void TranceGateProcessor::undo()
{
    if (undoStack.empty()) return;
    redoStack.push_back (capture());
    restore (undoStack.back());
    undoStack.pop_back();
}

void TranceGateProcessor::redo()
{
    if (redoStack.empty()) return;
    undoStack.push_back (capture());
    restore (redoStack.back());
    redoStack.pop_back();
}

void TranceGateProcessor::parameterGestureChanged (int, bool gestureIsStarting)
{
    // a knob drag from the GUI starts -> remember the state before it
    if (gestureIsStarting && ! restoring && MessageManager::existsAndIsCurrentThread())
        pushUndoPoint();
}

void TranceGateProcessor::parameterChanged (const String& id, float)
{
    if (id == "variation" && ! restoring)
        triggerAsyncUpdate();
}

void TranceGateProcessor::handleAsyncUpdate()
{
    const int stdIndex = pendingStandard.exchange (-1);
    if (stdIndex >= 0)
        loadPreset (tg::presetIndex ("Standard", stdIndex));

    const int v = getVariation();
    if (v != shownVariation)
    {
        storeKnobsInto (shownVariation);
        recallKnobsFrom (v);
        shownVariation = v;
        ++stateVersion;
    }
}

//==============================================================================
// Audio
//==============================================================================
void TranceGateProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    for (auto& v : voices) v.prepare (sampleRate);
    for (auto& f : filters)
    {
        f.prepare ({ sampleRate, (uint32) jmax (1, samplesPerBlock), 1 });
        f.reset();
    }
    lastFilterMode = -1;
    mixSmoothed.reset (sampleRate, 0.02);
    depthSmoothed.reset (sampleRate, 0.02);
    mixSmoothed.setCurrentAndTargetValue (pMix->load());
    depthSmoothed.setCurrentAndTargetValue (pDepth->load());
}

void TranceGateProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = jmin (2, getTotalNumOutputChannels());

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    // MIDI C1..G1 (36..43) loads Standard A..H into the current variation
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn())
        {
            const int n = msg.getNoteNumber() - 36;
            if (n >= 0 && n < 8) { pendingStandard = n; triggerAsyncUpdate(); }
        }
    }
    midi.clear();

    const auto pat = patterns[(size_t) getVariation()].get();

    //--------------------------------------------------------------- transport
    double bpm = 120.0, beat = internalBeat;
    bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            playing = pos->getIsPlaying();
            if (playing)
                if (auto ppq = pos->getPpqPosition())
                    beat = *ppq;
        }
    bpm = jlimit (20.0, 999.0, bpm);
    lastBpm = bpm;
    hostPlaying = playing;
    const double sr = getSampleRate() > 0 ? getSampleRate() : 44100.0;
    const double beatsPerSample = bpm / (60.0 * sr);

    //--------------------------------------------------------------- params
    const auto gp = getGateParams();
    for (auto& v : voices) v.setParams (gp);
    const double stepBeats = tg::resolutions()[(size_t) pat.resolution].beats;
    const double rightOffset = (double) pStereo->load() * stepBeats;
    const double loopBeats = stepBeats * pat.length;

    const int mode = getMode();
    if (mode != lastFilterMode)
    {
        for (auto& f : filters)
        {
            f.setType (mode == ModeHighPass ? dsp::StateVariableTPTFilterType::highpass
                                            : dsp::StateVariableTPTFilterType::lowpass);
            f.reset();
        }
        lastFilterMode = mode;
    }
    const float cutoff = jlimit (20.0f, 20000.0f, pCutoff->load());
    const float q = 0.55f + pReso->load() * 7.5f;
    for (auto& f : filters) f.setResonance (q);
    const float nyq = (float) (sr * 0.45);
    const float openFreq = mode == ModeHighPass ? 20.0f : jmin (20000.0f, nyq);

    mixSmoothed.setTargetValue (pMix->load());
    depthSmoothed.setTargetValue (pDepth->load());

    int step = -1;
    float env[2] { 0.0f, 0.0f };

    for (int i = 0; i < numSamples; ++i)
    {
        const double b = beat + (double) i * beatsPerSample;
        const float mix   = mixSmoothed.getNextValue();
        const float depth = depthSmoothed.getNextValue();

        env[0] = voices[0].process (b, pat, &step);
        env[1] = rightOffset > 0.0 ? voices[1].process (b - rightOffset, pat) : env[0];

        float inPk = 0.0f, outPk = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* d = buffer.getWritePointer (ch);
            const float x = d[i];
            const float amount = (1.0f - depth) + depth * env[ch];
            float y;
            if (mode == ModeVolume)
            {
                y = x * ((1.0f - mix) + mix * amount);
            }
            else
            {
                if ((i & 7) == 0)
                {
                    // closed gate = filter at CUTOFF, open gate = filter fully open
                    const float fc = cutoff * std::pow (openFreq / cutoff, amount);
                    filters[(size_t) ch].setCutoffFrequency (jlimit (20.0f, nyq, fc));
                }
                const float f = filters[(size_t) ch].processSample (0, x);
                y = x * (1.0f - mix) + f * mix;
            }
            d[i] = y;
            inPk  = jmax (inPk,  std::abs (x));
            outPk = jmax (outPk, std::abs (y));
        }

        // signal view: peak per position inside the loop
        if (loopBeats > 0.0)
        {
            double p = std::fmod (b, loopBeats);
            if (p < 0.0) p += loopBeats;
            const int bin = jlimit (0, kSignalBins - 1, (int) (p / loopBeats * kSignalBins));
            if (bin != lastBin)
            {
                if (lastBin >= 0)
                {
                    inPeaks[(size_t) lastBin].store (binIn, std::memory_order_relaxed);
                    outPeaks[(size_t) lastBin].store (binOut, std::memory_order_relaxed);
                }
                lastBin = bin;
                binIn = binOut = 0.0f;
            }
            binIn  = jmax (binIn, inPk);
            binOut = jmax (binOut, outPk);
        }
    }

    internalBeat = beat + (double) numSamples * beatsPerSample;
    if (internalBeat > 1.0e7) internalBeat = 0.0;
    currentStep = step;
}

//==============================================================================
// State
//==============================================================================
static String patternToString (const tg::PatternData& p)
{
    String steps;
    for (int s = 0; s < tg::kMaxSteps; ++s)
    {
        const auto& st = p.steps[(size_t) s];
        steps << String (st.level, 3) << ":" << st.ratchet << ":" << (st.tie ? 1 : 0) << ";";
    }
    return steps;
}

static tg::PatternData patternFromTree (const ValueTree& pt)
{
    tg::PatternData p;
    p.length     = (int) pt.getProperty ("len", 16);
    p.resolution = (int) pt.getProperty ("res", (int) tg::R16);
    auto tokens = StringArray::fromTokens (pt.getProperty ("steps").toString(), ";", "");
    for (int s = 0; s < jmin (tg::kMaxSteps, tokens.size()); ++s)
    {
        auto f = StringArray::fromTokens (tokens[s], ":", "");
        if (f.size() < 3) continue;
        p.steps[(size_t) s].level   = f[0].getFloatValue();
        p.steps[(size_t) s].ratchet = f[1].getIntValue();
        p.steps[(size_t) s].tie     = f[2].getIntValue() != 0;
    }
    return p;
}

void TranceGateProcessor::getStateInformation (MemoryBlock& destData)
{
    storeKnobsInto (getVariation());
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName ("PATTERNS"), nullptr);
    ValueTree pats ("PATTERNS");
    for (int v = 0; v < kNumVariations; ++v)
    {
        const auto p = patterns[(size_t) v].get();
        ValueTree pt ("P");
        pt.setProperty ("len", p.length, nullptr);
        pt.setProperty ("res", p.resolution, nullptr);
        pt.setProperty ("steps", patternToString (p), nullptr);
        const int idx = presetIndex[(size_t) v].load();
        if (idx >= 0)
        {
            const auto& pr = tg::presetLibrary()[(size_t) idx];
            pt.setProperty ("presetCat", pr.category, nullptr);
            pt.setProperty ("presetName", pr.name, nullptr);
        }
        pt.setProperty ("edited", presetEdited[(size_t) v].load(), nullptr);
        ValueTree knobs ("KNOBS");
        for (auto& [id, value] : knobSnapshots[(size_t) v])
            knobs.setProperty (Identifier (id), value, nullptr);
        pt.addChild (knobs, -1, nullptr);
        pats.addChild (pt, -1, nullptr);
    }
    state.addChild (pats, -1, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void TranceGateProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr) return;
    auto state = ValueTree::fromXml (*xml);
    if (! state.hasType (apvts.state.getType())) return;

    auto pats = state.getChildWithName ("PATTERNS");
    for (int v = 0; v < jmin (kNumVariations, pats.getNumChildren()); ++v)
    {
        auto pt = pats.getChild (v);
        patterns[(size_t) v].set (patternFromTree (pt));
        presetIndex[(size_t) v] = tg::presetIndexByName (pt.getProperty ("presetCat").toString(), pt.getProperty ("presetName").toString());
        presetEdited[(size_t) v] = (bool) pt.getProperty ("edited", presetIndex[(size_t) v].load() < 0);
        auto knobs = pt.getChildWithName ("KNOBS");
        for (int k = 0; k < knobs.getNumProperties(); ++k)
        {
            const auto name = knobs.getPropertyName (k);
            knobSnapshots[(size_t) v][name.toString()] = (float) knobs.getProperty (name);
        }
    }
    state.removeChild (pats, nullptr);

    const ScopedValueSetter<bool> svs (restoring, true);
    apvts.replaceState (state);
    shownVariation = getVariation();
    undoStack.clear();
    redoStack.clear();
    ++stateVersion;
}

//==============================================================================
AudioProcessorEditor* TranceGateProcessor::createEditor()
{
    return new TranceGateEditor (*this);
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TranceGateProcessor();
}
