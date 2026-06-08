#pragma once
#include <JuceHeader.h>

class IronSignalProcessor : public juce::AudioProcessor,
                             public juce::AudioProcessorValueTreeState::Listener
{
public:
    IronSignalProcessor();
    ~IronSignalProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override { return 4; }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void parameterChanged(const juce::String& paramID, float newValue) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP nodes
    juce::dsp::Gain<float> inputGain, outputGain;
    juce::dsp::WaveShaper<float> waveShaper;
    juce::dsp::Oversampling<float> oversampling;

    // Filters
    juce::dsp::IIR::Filter<float> highpassL, highpassR;
    juce::dsp::IIR::Filter<float> bassL, bassR;
    juce::dsp::IIR::Filter<float> midL, midR;
    juce::dsp::IIR::Filter<float> trebleL, trebleR;
    juce::dsp::IIR::Filter<float> presenceL, presenceR;
    juce::dsp::IIR::Filter<float> cabLowL, cabLowR;
    juce::dsp::IIR::Filter<float> cabHighL, cabHighR;

    // Delay
    juce::dsp::DelayLine<float> delayLine { 96000 };
    juce::SmoothedValue<float> smoothDelayTime, smoothDelayFeedback, smoothDelayMix;
    float delayFeedbackSample = 0.0f;

    // Noise gate
    float gateEnvelope = 0.0f;

    // Smoothed params
    juce::SmoothedValue<float> smoothGain, smoothTightness, smoothPresence;
    juce::SmoothedValue<float> smoothBass, smoothMid, smoothTreble, smoothMaster;

    double currentSampleRate = 44100.0;
    int currentProgram = 0;

    void updateFilters();
    float hardClip(float x, float amount);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IronSignalProcessor)
};
