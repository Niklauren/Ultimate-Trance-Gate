#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <map>
#include "GateEngine.h"

//==============================================================================
// Ultimate Trance Gate v2
//  - 3 variations (A / B / C): each keeps its own pattern AND its own knob settings
//  - gate modes: VOL (volume), LPF / HPF (the gate opens / closes a filter)
//  - undo / redo of everything the user does (pattern edits, presets, knobs, variation)
//  - input / output peaks recorded per pattern position for the signal view
//==============================================================================
class TranceGateProcessor : public juce::AudioProcessor,
                            private juce::AudioProcessorValueTreeState::Listener,
                            private juce::AudioProcessorParameter::Listener,
                            private juce::AsyncUpdater
{
public:
    TranceGateProcessor();
    ~TranceGateProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Ultimate Trance Gate"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override    { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    static constexpr int kNumVariations = 3;
    static constexpr int kSignalBins    = 384;
    enum Mode { ModeVolume = 0, ModeLowPass, ModeHighPass };

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static const juce::StringArray& variationParamIds();   // knobs that each variation keeps its own copy of

    juce::AudioProcessorValueTreeState apvts;

    // pattern of each variation (lock-free: written by GUI / MIDI, read by audio)
    std::array<tg::PatternSlot, kNumVariations> patterns;
    // which library preset each variation came from (-1 = custom) and whether it was edited since
    std::array<std::atomic<int>,  kNumVariations> presetIndex {};
    std::array<std::atomic<bool>, kNumVariations> presetEdited {};

    int  getVariation() const noexcept { return juce::jlimit (0, kNumVariations - 1, (int) pVariation->load()); }
    int  getMode() const noexcept      { return juce::jlimit (0, 2, (int) pMode->load()); }
    void selectVariation (int v);        // GUI: switch A / B / C (undoable)
    void selectMode (int m);             // GUI: VOL / LPF / HPF (undoable)

    // ---- pattern edits (message thread)
    void editPattern (const tg::PatternData& p, bool newUndoPoint);   // edits the current variation
    void loadPreset (int libraryIndex);                               // loads into the current variation
    tg::PatternData currentPattern() const { return patterns[(size_t) getVariation()].get(); }

    // ---- undo / redo (message thread)
    void pushUndoPoint();
    bool canUndo() const noexcept { return ! undoStack.empty(); }
    bool canRedo() const noexcept { return ! redoStack.empty(); }
    void undo();
    void redo();
    std::atomic<int> stateVersion { 0 };   // bumps whenever patterns / presets / variation change

    // ---- GUI feedback
    std::atomic<int>    currentStep { -1 };
    std::atomic<double> lastBpm     { 120.0 };
    std::atomic<bool>   hostPlaying { false };

    // ---- signal view: peaks per position inside the current pattern loop
    std::array<std::atomic<float>, kSignalBins> inPeaks {}, outPeaks {};

    tg::GateParams getGateParams() const;

private:
    struct Snapshot
    {
        std::array<tg::PatternData, kNumVariations> pats;
        std::array<int,  kNumVariations> preset {};
        std::array<bool, kNumVariations> edited {};
        std::map<juce::String, float> params;
        std::array<std::map<juce::String, float>, kNumVariations> knobs;
    };
    Snapshot capture() const;
    void restore (const Snapshot&);

    void parameterChanged (const juce::String& id, float newValue) override;
    void parameterValueChanged (int, float) override {}
    void parameterGestureChanged (int, bool gestureIsStarting) override;
    void handleAsyncUpdate() override;
    void storeKnobsInto (int variation);
    void recallKnobsFrom (int variation);
    void setParam (const juce::String& id, float plainValue);

    std::array<tg::GateVoice, 2> voices;
    std::array<juce::dsp::StateVariableTPTFilter<float>, 2> filters;
    juce::SmoothedValue<float> mixSmoothed, depthSmoothed;

    double internalBeat = 0.0;
    int lastFilterMode = -1;
    float binIn = 0.0f, binOut = 0.0f;
    int lastBin = -1;

    std::vector<Snapshot> undoStack, redoStack;
    std::array<std::map<juce::String, float>, kNumVariations> knobSnapshots;
    int shownVariation = 0;
    bool restoring = false;
    std::atomic<int> pendingStandard { -1 };   // MIDI C1..G1 -> load Standard preset (message thread)

    std::atomic<float>* pVariation = nullptr;
    std::atomic<float>* pMode    = nullptr;
    std::atomic<float>* pAttack  = nullptr;
    std::atomic<float>* pDecay   = nullptr;
    std::atomic<float>* pSustain = nullptr;
    std::atomic<float>* pRelease = nullptr;
    std::atomic<float>* pGate    = nullptr;
    std::atomic<float>* pSwing   = nullptr;
    std::atomic<float>* pDepth   = nullptr;
    std::atomic<float>* pStereo  = nullptr;
    std::atomic<float>* pMix     = nullptr;
    std::atomic<float>* pCutoff  = nullptr;
    std::atomic<float>* pReso    = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TranceGateProcessor)
};
