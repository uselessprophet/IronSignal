#include "PluginProcessor.h"
#include "PluginEditor.h"

IronSignalProcessor::IronSignalProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout()),
      oversampling(2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR)
{
    apvts.addParameterListener("gain",      this);
    apvts.addParameterListener("tightness", this);
    apvts.addParameterListener("presence",  this);
    apvts.addParameterListener("bass",      this);
    apvts.addParameterListener("mid",       this);
    apvts.addParameterListener("treble",    this);
    apvts.addParameterListener("master",    this);
    apvts.addParameterListener("delaymix",  this);
    apvts.addParameterListener("delaytime", this);
    apvts.addParameterListener("delayfb",   this);
}

IronSignalProcessor::~IronSignalProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout IronSignalProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "gain", "Gain (Distortion)", 0.0f, 100.0f, 75.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "tightness", "Kemik (Tightness)", 0.0f, 100.0f, 60.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "presence", "Sertlik (Presence)", 0.0f, 100.0f, 50.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "bass", "Bass", 0.0f, 100.0f, 60.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "mid", "Mid", 0.0f, 100.0f, 35.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "treble", "Treble", 0.0f, 100.0f, 65.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "master", "Master Volume", 0.0f, 100.0f, 70.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "boost", "Boost", true));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "gate", "Noise Gate", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "cab", "Cab Sim", true));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "delayOn", "Delay On", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "delaymix", "Delay Mix", 0.0f, 100.0f, 25.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "delaytime", "Delay Time", 0.0f, 100.0f, 40.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "delayfb", "Delay Feedback", 0.0f, 100.0f, 30.0f));

    return { params.begin(), params.end() };
}

void IronSignalProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32)samplesPerBlock, 2 };

    oversampling.initProcessing(samplesPerBlock);
    juce::dsp::ProcessSpec osSpec { sampleRate * 4.0,
        (juce::uint32)(samplesPerBlock * 4), 2 };

    inputGain.prepare(spec);
    outputGain.prepare(spec);
    delayLine.prepare(spec);
    delayLine.setMaximumDelayInSamples((int)(sampleRate * 2.0));

    // Smoothed values
    smoothGain.reset(sampleRate, 0.02);
    smoothTightness.reset(sampleRate, 0.02);
    smoothPresence.reset(sampleRate, 0.02);
    smoothBass.reset(sampleRate, 0.02);
    smoothMid.reset(sampleRate, 0.02);
    smoothTreble.reset(sampleRate, 0.02);
    smoothMaster.reset(sampleRate, 0.02);
    smoothDelayTime.reset(sampleRate, 0.05);
    smoothDelayFeedback.reset(sampleRate, 0.02);
    smoothDelayMix.reset(sampleRate, 0.02);

    smoothGain.setCurrentAndTargetValue(*apvts.getRawParameterValue("gain"));
    smoothTightness.setCurrentAndTargetValue(*apvts.getRawParameterValue("tightness"));
    smoothPresence.setCurrentAndTargetValue(*apvts.getRawParameterValue("presence"));
    smoothBass.setCurrentAndTargetValue(*apvts.getRawParameterValue("bass"));
    smoothMid.setCurrentAndTargetValue(*apvts.getRawParameterValue("mid"));
    smoothTreble.setCurrentAndTargetValue(*apvts.getRawParameterValue("treble"));
    smoothMaster.setCurrentAndTargetValue(*apvts.getRawParameterValue("master"));

    gateEnvelope = 0.0f;
    delayFeedbackSample = 0.0f;

    updateFilters();
}

void IronSignalProcessor::releaseResources() {}

void IronSignalProcessor::updateFilters()
{
    double sr = currentSampleRate;

    float tightness = smoothTightness.getTargetValue();
    float hpFreq = 60.0f + tightness * 1.2f;
    auto hpCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, hpFreq, 0.7f);
    *highpassL.coefficients = *hpCoeffs;
    *highpassR.coefficients = *hpCoeffs;

    float bassGainDB = (smoothBass.getTargetValue() - 50.0f) / 4.0f;
    auto bassCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sr, 120.0, 0.7, juce::Decibels::decibelsToGain(bassGainDB));
    *bassL.coefficients = *bassCoeffs;
    *bassR.coefficients = *bassCoeffs;

    float midGainDB = (smoothMid.getTargetValue() - 50.0f) / 4.0f;
    auto midCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, 800.0, 1.5, juce::Decibels::decibelsToGain(midGainDB));
    *midL.coefficients = *midCoeffs;
    *midR.coefficients = *midCoeffs;

    float trebleGainDB = (smoothTreble.getTargetValue() - 50.0f) / 4.0f;
    auto trebleCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sr, 3500.0, 0.7, juce::Decibels::decibelsToGain(trebleGainDB));
    *trebleL.coefficients = *trebleCoeffs;
    *trebleR.coefficients = *trebleCoeffs;

    float presenceGainDB = (smoothPresence.getTargetValue() - 50.0f) / 3.5f;
    auto presCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, 4500.0, 2.0, juce::Decibels::decibelsToGain(presenceGainDB));
    *presenceL.coefficients = *presCoeffs;
    *presenceR.coefficients = *presCoeffs;

    // Cab sim: lowpass ~5kHz, notch at ~1.2kHz (box resonance)
    auto cabLowCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sr, 5000.0, 0.9);
    *cabLowL.coefficients = *cabLowCoeffs;
    *cabLowR.coefficients = *cabLowCoeffs;

    auto cabHighCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, 80.0, 0.7);
    *cabHighL.coefficients = *cabHighCoeffs;
    *cabHighR.coefficients = *cabHighCoeffs;
}

void IronSignalProcessor::parameterChanged(const juce::String& paramID, float newValue)
{
    if (paramID == "gain")      smoothGain.setTargetValue(newValue);
    if (paramID == "tightness") { smoothTightness.setTargetValue(newValue); updateFilters(); }
    if (paramID == "presence")  { smoothPresence.setTargetValue(newValue);  updateFilters(); }
    if (paramID == "bass")      { smoothBass.setTargetValue(newValue);      updateFilters(); }
    if (paramID == "mid")       { smoothMid.setTargetValue(newValue);       updateFilters(); }
    if (paramID == "treble")    { smoothTreble.setTargetValue(newValue);    updateFilters(); }
    if (paramID == "master")    smoothMaster.setTargetValue(newValue);
    if (paramID == "delaymix")  smoothDelayMix.setTargetValue(newValue);
    if (paramID == "delaytime") smoothDelayTime.setTargetValue(newValue);
    if (paramID == "delayfb")   smoothDelayFeedback.setTargetValue(newValue);
}

float IronSignalProcessor::hardClip(float x, float amount)
{
    // Asymmetric waveshaper for even + odd harmonics (tube-like aggression)
    float k = 1.0f + amount * 0.06f;
    float shaped = (juce::MathConstants<float>::pi + k) * x /
                   (juce::MathConstants<float>::pi + k * std::abs(x));

    // Hard clip ceiling
    float ceiling = 1.0f - amount * 0.003f;
    return juce::jlimit(-ceiling, ceiling, shaped);
}

void IronSignalProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& /*midi*/)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumIn  = getTotalNumInputChannels();
    auto totalNumOut = getTotalNumOutputChannels();

    for (int i = totalNumIn; i < totalNumOut; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    const bool doBoost  = *apvts.getRawParameterValue("boost")   > 0.5f;
    const bool doGate   = *apvts.getRawParameterValue("gate")    > 0.5f;
    const bool doCab    = *apvts.getRawParameterValue("cab")     > 0.5f;
    const bool doDelay  = *apvts.getRawParameterValue("delayOn") > 0.5f;

    const float gainVal      = smoothGain.getNextValue();
    const float masterVal    = smoothMaster.getNextValue();
    const float delayTimeVal = smoothDelayTime.getNextValue();
    const float delayFbVal   = smoothDelayFeedback.getNextValue();
    const float delayMixVal  = smoothDelayMix.getNextValue();

    // Pre-gain
    float preGain = doBoost ? (1.0f + gainVal / 20.0f) : (1.0f + gainVal / 40.0f);

    int numSamples = buffer.getNumSamples();

    // Oversampled distortion block
    juce::dsp::AudioBlock<float> inputBlock(buffer);
    auto osBlock = oversampling.processSamplesUp(inputBlock);

    for (int sample = 0; sample < (int)osBlock.getNumSamples(); ++sample)
    {
        for (int ch = 0; ch < (int)osBlock.getNumChannels(); ++ch)
        {
            float* data = osBlock.getChannelPointer(ch);
            data[sample] = hardClip(data[sample] * preGain, gainVal);
        }
    }

    juce::dsp::AudioBlock<float> outputBlock(buffer);
    oversampling.processSamplesDown(outputBlock);

    // Per-sample processing
    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int ch = 0; ch < juce::jmin(2, buffer.getNumChannels()); ++ch)
        {
            float* data = buffer.getWritePointer(ch);
            float s = data[sample];

            // Noise gate
            if (doGate)
            {
                float level = std::abs(s);
                float threshold = 0.015f;
                float attackCoeff  = 0.001f;
                float releaseCoeff = 0.0001f;
                if (level > threshold)
                    gateEnvelope += attackCoeff * (1.0f - gateEnvelope);
                else
                    gateEnvelope -= releaseCoeff * gateEnvelope;
                gateEnvelope = juce::jlimit(0.0f, 1.0f, gateEnvelope);
                s *= gateEnvelope;
            }

            // Highpass (tightness)
            if (ch == 0) s = highpassL.processSample(s);
            else         s = highpassR.processSample(s);

            // EQ
            if (ch == 0) { s = bassL.processSample(s); s = midL.processSample(s); s = trebleL.processSample(s); s = presenceL.processSample(s); }
            else         { s = bassR.processSample(s); s = midR.processSample(s); s = trebleR.processSample(s); s = presenceR.processSample(s); }

            // Cab sim
            if (doCab)
            {
                if (ch == 0) { s = cabHighL.processSample(s); s = cabLowL.processSample(s); }
                else         { s = cabHighR.processSample(s); s = cabLowR.processSample(s); }
            }

            // Delay
            float drySignal = s;
            if (doDelay)
            {
                float delaySamples = (0.05f + delayTimeVal / 100.0f * 0.8f) * (float)currentSampleRate;
                delayLine.setDelay(delaySamples);
                float delayOut = delayLine.popSample(ch);
                delayLine.pushSample(ch, s + delayOut * (delayFbVal / 100.0f * 0.75f));
                s = drySignal + delayOut * (delayMixVal / 100.0f * 0.8f);
            }

            // Master volume
            s *= (masterVal / 100.0f) * 1.2f;

            // Soft limiter on output
            s = std::tanh(s * 0.9f);

            data[sample] = s;
        }
    }

    // If mono input, copy L to R
    if (buffer.getNumChannels() >= 2 && getTotalNumInputChannels() == 1)
        buffer.copyFrom(1, 0, buffer, 0, 0, numSamples);
}

void IronSignalProcessor::setCurrentProgram(int index)
{
    currentProgram = index;
    struct Preset { float gain, tightness, presence, bass, mid, treble, master; bool boost, gate; };
    Preset presets[4] = {
        { 85, 90, 70, 55, 25, 65, 70, true,  true  }, // DJENT
        { 95, 75, 55, 70, 30, 60, 65, true,  true  }, // DEATH METAL
        { 70, 60, 65, 50, 45, 70, 72, false, false }, // PROG
        { 15, 40, 45, 55, 55, 55, 75, false, false }, // CLEAN
    };
    auto& p = presets[juce::jlimit(0, 3, index)];
    apvts.getParameter("gain")->setValueNotifyingHost(p.gain / 100.0f);
    apvts.getParameter("tightness")->setValueNotifyingHost(p.tightness / 100.0f);
    apvts.getParameter("presence")->setValueNotifyingHost(p.presence / 100.0f);
    apvts.getParameter("bass")->setValueNotifyingHost(p.bass / 100.0f);
    apvts.getParameter("mid")->setValueNotifyingHost(p.mid / 100.0f);
    apvts.getParameter("treble")->setValueNotifyingHost(p.treble / 100.0f);
    apvts.getParameter("master")->setValueNotifyingHost(p.master / 100.0f);
    apvts.getParameter("boost")->setValueNotifyingHost(p.boost ? 1.0f : 0.0f);
    apvts.getParameter("gate")->setValueNotifyingHost(p.gate ? 1.0f : 0.0f);
}

const juce::String IronSignalProcessor::getProgramName(int index)
{
    switch(index) {
        case 0: return "Djent";
        case 1: return "Death Metal";
        case 2: return "Prog";
        case 3: return "Clean";
        default: return {};
    }
}

void IronSignalProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void IronSignalProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new IronSignalProcessor();
}
