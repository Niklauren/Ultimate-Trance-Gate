#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "GateEngine.h"

class TranceGateProcessor : public juce::AudioProcessor
{
public:
    TranceGateProcessor();
    ~TranceGateProcessor() override = default;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
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
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;
    std::array<tg::PatternSlot, tg::kNumPatterns> patterns;

    // GUI feedback
    std::atomic<int>   activePattern { 0 };
    std::atomic<int>   currentStep   { -1 };
    std::atomic<float> currentGain   { 0.0f };
    std::atomic<double> lastBpm      { 120.0 };
    std::atomic<bool>  hostPlaying   { false };

    tg::GateParams getGateParams() const;
    int getStereoOffset() const;

    void selectPatternFromGui (int index);

private:
    std::array<tg::GateVoice, 2> voices;
    juce::SmoothedValue<float> mixSmoothed, depthSmoothed;

    double internalBeat = 0.0;
    int lastParamPattern = -1;

    std::atomic<float>* pPattern = nullptr;
    std::atomic<float>* pAttack  = nullptr;
    std::atomic<float>* pDecay   = nullptr;
    std::atomic<float>* pSustain = nullptr;
    std::atomic<float>* pRelease = nullptr;
    std::atomic<float>* pGate    = nullptr;
    std::atomic<float>* pSwing   = nullptr;
    std::atomic<float>* pDepth   = nullptr;
    std::atomic<float>* pStereo  = nullptr;
    std::atomic<float>* pMix     = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TranceGateProcessor)
};
