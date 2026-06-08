#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class IronKnob : public juce::Slider
{
public:
    IronKnob(const juce::String& label) : labelText(label)
    {
        setSliderStyle(juce::Slider::RotaryVerticalDrag);
        setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        setLookAndFeel(nullptr);
    }
    juce::String labelText;
};

class IronSignalEditor : public juce::AudioProcessorEditor,
                          private juce::Timer
{
public:
    IronSignalEditor(IronSignalProcessor&);
    ~IronSignalEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void buildUI();
    void paintKnob(juce::Graphics& g, juce::Rectangle<float> bounds,
                   float value, const juce::String& label, const juce::Colour& accent);

    IronSignalProcessor& processor;

    // Knobs
    IronKnob gainKnob      { "GAIN" };
    IronKnob tightnessKnob { "KEMIK" };
    IronKnob presenceKnob  { "SERTLiK" };
    IronKnob bassKnob      { "BASS" };
    IronKnob midKnob       { "MID" };
    IronKnob trebleKnob    { "TREBLE" };
    IronKnob masterKnob    { "MASTER" };
    IronKnob delayMixKnob  { "MiX" };
    IronKnob delayTimeKnob { "SURE" };
    IronKnob delayFbKnob   { "FEEDBK" };

    // Toggles
    juce::TextButton boostBtn  { "BOOST" };
    juce::TextButton gateBtn   { "GATE" };
    juce::TextButton cabBtn    { "CAB SIM" };
    juce::TextButton delayBtn  { "DELAY" };

    // Preset buttons
    juce::TextButton djentBtn    { "DJENT" };
    juce::TextButton deathBtn    { "DEATH" };
    juce::TextButton progBtn     { "PROG" };
    juce::TextButton cleanBtn    { "CLEAN" };

    // VU meter
    float vuLevel = 0.0f;

    using APVTS = juce::AudioProcessorValueTreeState;
    std::unique_ptr<APVTS::SliderAttachment> gainAtt, tightnessAtt, presenceAtt;
    std::unique_ptr<APVTS::SliderAttachment> bassAtt, midAtt, trebleAtt, masterAtt;
    std::unique_ptr<APVTS::SliderAttachment> dlyMixAtt, dlyTimeAtt, dlyFbAtt;
    std::unique_ptr<APVTS::ButtonAttachment> boostAtt, gateAtt, cabAtt, delayAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IronSignalEditor)
};
