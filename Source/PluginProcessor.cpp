#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

using namespace juce;

namespace ids
{
    static const ParameterID pattern { "pattern", 1 };
    static const ParameterID attack  { "attack",  1 };
    static const ParameterID decay   { "decay",   1 };
    static const ParameterID sustain { "sustain", 1 };
    static const ParameterID release { "release", 1 };
    static const ParameterID gate    { "gate",    1 };
    static const ParameterID swing   { "swing",   1 };
    static const ParameterID depth   { "depth",   1 };
    static const ParameterID stereo  { "stereo",  1 };
    static const ParameterID mix     { "mix",     1 };
}

//==============================================================================
static NormalisableRange<float> msRange (float lo, float hi, float centre)
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

AudioProcessorValueTreeState::ParameterLayout TranceGateProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterChoice> (ids::pattern, "Pattern",
                StringArray { "A", "B", "C", "D", "E", "F", "G", "H" }, 0));

    layout.add (std::make_unique<AudioParameterFloat> (ids::attack,  "Attack",  msRange (0.1f, 300.0f, 15.0f),  1.0f,   msAttr()));
    layout.add (std::make_unique<AudioParameterFloat> (ids::decay,   "Decay",   msRange (1.0f, 2000.0f, 200.0f), 150.0f, msAttr()));
    layout.add (std::make_unique<AudioParameterFloat> (ids::sustain, "Sustain", NormalisableRange<float> (0.0f, 1.0f), 1.0f,  pctAttr()));
    layout.add (std::make_unique<AudioParameterFloat> (ids::release, "Release", msRange (1.0f, 1000.0f, 80.0f),  25.0f,  msAttr()));
    layout.add (std::make_unique<AudioParameterFloat> (ids::gate,    "Gate",    NormalisableRange<float> (0.05f, 1.0f), 0.75f, pctAttr()));
    layout.add (std::make_unique<AudioParameterFloat> (ids::swing,   "Swing",   NormalisableRange<float> (0.0f, 1.0f), 0.0f,   pctAttr()));
    layout.add (std::make_unique<AudioParameterFloat> (ids::depth,   "Depth",   NormalisableRange<float> (0.0f, 1.0f), 1.0f,   pctAttr()));
    layout.add (std::make_unique<AudioParameterInt>   (ids::stereo,  "Stereo Offset", 0, 8, 0,
                AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return v == 0 ? String ("Off") : String (v) + " st"; })));
    layout.add (std::make_unique<AudioParameterFloat> (ids::mix,     "Mix",     NormalisableRange<float> (0.0f, 1.0f), 1.0f,   pctAttr()));

    return layout;
}

//==============================================================================
TranceGateProcessor::TranceGateProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  AudioChannelSet::stereo(), true)
                        .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    pPattern = apvts.getRawParameterValue ("pattern");
    pAttack  = apvts.getRawParameterValue ("attack");
    pDecay   = apvts.getRawParameterValue ("decay");
    pSustain = apvts.getRawParameterValue ("sustain");
    pRelease = apvts.getRawParameterValue ("release");
    pGate    = apvts.getRawParameterValue ("gate");
    pSwing   = apvts.getRawParameterValue ("swing");
    pDepth   = apvts.getRawParameterValue ("depth");
    pStereo  = apvts.getRawParameterValue ("stereo");
    pMix     = apvts.getRawParameterValue ("mix");

    // Factory start-up: 8 different patterns in the slots
    const char* defaults[] = { "Sunrise Gate", "Rhythm Stab 3-3-2", "Rage Chop", "Gallop Gate",
                               "KBBB Roll", "Supersaw Stutter", "3-3-2 Chords", "Build-Up 32" };
    for (int i = 0; i < tg::kNumPatterns; ++i)
        for (auto& p : tg::presetLibrary())
            if (p.name == defaults[i]) { patterns[(size_t) i].set (p.data); break; }
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

int TranceGateProcessor::getStereoOffset() const { return (int) pStereo->load(); }

void TranceGateProcessor::selectPatternFromGui (int index)
{
    index = jlimit (0, tg::kNumPatterns - 1, index);
    activePattern = index;
    if (auto* p = apvts.getParameter ("pattern"))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 ((float) index));
        p->endChangeGesture();
    }
}

void TranceGateProcessor::prepareToPlay (double sampleRate, int)
{
    for (auto& v : voices) v.prepare (sampleRate);
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

    //--------------------------------------------------------------- pattern select
    const int paramPattern = (int) pPattern->load();
    if (paramPattern != lastParamPattern)
    {
        activePattern = paramPattern;
        lastParamPattern = paramPattern;
    }
    // MIDI notes C1..G1 (36..43) switch patterns A..H
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn())
        {
            const int n = msg.getNoteNumber() - 36;
            if (n >= 0 && n < tg::kNumPatterns)
                activePattern = n;
        }
    }
    midi.clear();

    const auto pat = patterns[(size_t) activePattern.load()].get();

    //--------------------------------------------------------------- transport
    double bpm = 120.0;
    double beat = internalBeat;
    bool playing = false;

    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            playing = pos->getIsPlaying();
            if (playing)
                if (auto ppq = pos->getPpqPosition())
                    beat = *ppq;
        }
    }
    bpm = jlimit (20.0, 999.0, bpm);
    lastBpm = bpm;
    hostPlaying = playing;

    const double beatsPerSample = bpm / (60.0 * getSampleRate());

    //--------------------------------------------------------------- params
    const auto gp = getGateParams();
    for (auto& v : voices) v.setParams (gp);

    const double stepBeats = tg::resolutions()[(size_t) pat.resolution].beats;
    const double rightOffset = (double) getStereoOffset() * stepBeats;

    mixSmoothed.setTargetValue (pMix->load());
    depthSmoothed.setTargetValue (pDepth->load());

    auto* left  = buffer.getWritePointer (0);
    auto* right = numChannels > 1 ? buffer.getWritePointer (1) : nullptr;

    int step = -1;
    float g0 = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const double b = beat + (double) i * beatsPerSample;
        const float mix   = mixSmoothed.getNextValue();
        const float depth = depthSmoothed.getNextValue();

        g0 = voices[0].process (b, pat, &step);
        const float gainL = (1.0f - mix) + mix * ((1.0f - depth) + depth * g0);
        left[i] *= gainL;

        if (right != nullptr)
        {
            float gainR = gainL;
            if (rightOffset > 0.0)
            {
                const float g1 = voices[1].process (b - rightOffset, pat);
                gainR = (1.0f - mix) + mix * ((1.0f - depth) + depth * g1);
            }
            right[i] *= gainR;
        }
    }

    internalBeat = beat + (double) numSamples * beatsPerSample;
    if (internalBeat > 1.0e7) internalBeat = 0.0;

    currentStep = step;
    currentGain = g0;
}

//==============================================================================
void TranceGateProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    ValueTree pats ("PATTERNS");
    for (int i = 0; i < tg::kNumPatterns; ++i)
    {
        const auto p = patterns[(size_t) i].get();
        ValueTree pt ("P");
        pt.setProperty ("len", p.length, nullptr);
        pt.setProperty ("res", p.resolution, nullptr);
        String steps;
        for (int s = 0; s < tg::kMaxSteps; ++s)
        {
            const auto& st = p.steps[(size_t) s];
            steps << String (st.level, 3) << ":" << st.ratchet << ":" << (st.tie ? 1 : 0) << ";";
        }
        pt.setProperty ("steps", steps, nullptr);
        pats.addChild (pt, -1, nullptr);
    }
    state.removeChild (state.getChildWithName ("PATTERNS"), nullptr);
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
    for (int i = 0; i < jmin (tg::kNumPatterns, pats.getNumChildren()); ++i)
    {
        auto pt = pats.getChild (i);
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
        patterns[(size_t) i].set (p);
    }

    state.removeChild (pats, nullptr);
    apvts.replaceState (state);
    lastParamPattern = -1;
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
