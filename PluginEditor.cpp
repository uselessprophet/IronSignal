#include "PluginEditor.h"

static const juce::Colour COL_BG       = juce::Colour(0xff1a1a1a);
static const juce::Colour COL_PANEL    = juce::Colour(0xff111111);
static const juce::Colour COL_ACCENT   = juce::Colour(0xffff4a00);
static const juce::Colour COL_TEXT     = juce::Colour(0xffcccccc);
static const juce::Colour COL_MUTED    = juce::Colour(0xff555555);
static const juce::Colour COL_BORDER   = juce::Colour(0xff2a2a2a);

IronSignalEditor::IronSignalEditor(IronSignalProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(620, 420);
    setResizable(false, false);

    // Knob attachments
    gainAtt       = std::make_unique<APVTS::SliderAttachment>(processor.apvts, "gain",      gainKnob);
    tightnessAtt  = std::make_unique<APVTS::SliderAttachment>(processor.apvts, "tightness", tightnessKnob);
    presenceAtt   = std::make_unique<APVTS::SliderAttachment>(processor.apvts, "presence",  presenceKnob);
    bassAtt       = std::make_unique<APVTS::SliderAttachment>(processor.apvts, "bass",       bassKnob);
    midAtt        = std::make_unique<APVTS::SliderAttachment>(processor.apvts, "mid",        midKnob);
    trebleAtt     = std::make_unique<APVTS::SliderAttachment>(processor.apvts, "treble",     trebleKnob);
    masterAtt     = std::make_unique<APVTS::SliderAttachment>(processor.apvts, "master",     masterKnob);
    dlyMixAtt     = std::make_unique<APVTS::SliderAttachment>(processor.apvts, "delaymix",  delayMixKnob);
    dlyTimeAtt    = std::make_unique<APVTS::SliderAttachment>(processor.apvts, "delaytime", delayTimeKnob);
    dlyFbAtt      = std::make_unique<APVTS::SliderAttachment>(processor.apvts, "delayfb",   delayFbKnob);

    boostAtt = std::make_unique<APVTS::ButtonAttachment>(processor.apvts, "boost",   boostBtn);
    gateAtt  = std::make_unique<APVTS::ButtonAttachment>(processor.apvts, "gate",    gateBtn);
    cabAtt   = std::make_unique<APVTS::ButtonAttachment>(processor.apvts, "cab",     cabBtn);
    delayAtt = std::make_unique<APVTS::ButtonAttachment>(processor.apvts, "delayOn", delayBtn);

    // Style all knobs
    for (auto* k : { &gainKnob, &tightnessKnob, &presenceKnob, &bassKnob,
                     &midKnob, &trebleKnob, &masterKnob,
                     &delayMixKnob, &delayTimeKnob, &delayFbKnob })
    {
        k->setRange(0.0, 100.0);
        addAndMakeVisible(k);
    }

    // Style toggle buttons
    auto styleToggle = [&](juce::TextButton& btn) {
        btn.setClickingTogglesState(true);
        btn.setColour(juce::TextButton::buttonColourId,    COL_PANEL);
        btn.setColour(juce::TextButton::buttonOnColourId,  COL_ACCENT);
        btn.setColour(juce::TextButton::textColourOffId,   COL_MUTED);
        btn.setColour(juce::TextButton::textColourOnId,    juce::Colours::white);
        addAndMakeVisible(btn);
    };

    styleToggle(boostBtn);
    styleToggle(gateBtn);
    styleToggle(cabBtn);
    styleToggle(delayBtn);

    // Preset buttons
    auto stylePreset = [&](juce::TextButton& btn) {
        btn.setColour(juce::TextButton::buttonColourId,   COL_PANEL);
        btn.setColour(juce::TextButton::buttonOnColourId, COL_ACCENT);
        btn.setColour(juce::TextButton::textColourOffId,  COL_MUTED);
        btn.setColour(juce::TextButton::textColourOnId,   juce::Colours::white);
        addAndMakeVisible(btn);
    };

    stylePreset(djentBtn);
    stylePreset(deathBtn);
    stylePreset(progBtn);
    stylePreset(cleanBtn);

    djentBtn.onClick = [this] { processor.setCurrentProgram(0); repaint(); };
    deathBtn.onClick = [this] { processor.setCurrentProgram(1); repaint(); };
    progBtn.onClick  = [this] { processor.setCurrentProgram(2); repaint(); };
    cleanBtn.onClick = [this] { processor.setCurrentProgram(3); repaint(); };

    startTimerHz(30);
}

IronSignalEditor::~IronSignalEditor()
{
    stopTimer();
}

void IronSignalEditor::timerCallback()
{
    repaint();
}

void IronSignalEditor::paintKnob(juce::Graphics& g, juce::Rectangle<float> bounds,
                                   float value, const juce::String& label,
                                   const juce::Colour& accent)
{
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY() - 6;
    float r  = bounds.getWidth() * 0.38f;

    // Track background
    float startAngle = juce::MathConstants<float>::pi * 0.75f;
    float endAngle   = juce::MathConstants<float>::pi * 2.25f;
    float valueAngle = startAngle + (value / 100.0f) * (endAngle - startAngle);

    juce::Path trackBg;
    trackBg.addArc(cx - r, cy - r, r*2, r*2, startAngle, endAngle, true);
    g.setColour(COL_BORDER);
    g.strokePath(trackBg, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

    // Value arc
    juce::Path trackVal;
    trackVal.addArc(cx - r, cy - r, r*2, r*2, startAngle, valueAngle, true);
    g.setColour(accent);
    g.strokePath(trackVal, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

    // Knob body
    float knobR = r * 0.62f;
    g.setColour(juce::Colour(0xff252525));
    g.fillEllipse(cx - knobR, cy - knobR, knobR*2, knobR*2);
    g.setColour(COL_BORDER);
    g.drawEllipse(cx - knobR, cy - knobR, knobR*2, knobR*2, 1.0f);

    // Indicator dot
    float dotAngle = valueAngle;
    float dotDist  = knobR * 0.6f;
    float dotX     = cx + dotDist * std::cos(dotAngle);
    float dotY     = cy + dotDist * std::sin(dotAngle);
    g.setColour(accent);
    g.fillEllipse(dotX - 2.5f, dotY - 2.5f, 5.0f, 5.0f);

    // Label
    g.setColour(COL_MUTED);
    g.setFont(juce::Font(9.0f, juce::Font::bold));
    g.drawText(label, (int)(cx - 30), (int)(cy + r + 4), 60, 14,
               juce::Justification::centred, false);

    // Value
    g.setColour(COL_TEXT.withAlpha(0.6f));
    g.setFont(juce::Font(9.0f));
    g.drawText(juce::String((int)value), (int)(cx - 15), (int)(cy + r + 16), 30, 12,
               juce::Justification::centred, false);
}

void IronSignalEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background
    g.fillAll(COL_BG);

    // Header
    juce::Rectangle<float> header(0, 0, bounds.getWidth(), 46);
    g.setColour(COL_PANEL);
    g.fillRect(header);
    g.setColour(COL_ACCENT);
    g.fillRect(0.0f, 44.0f, bounds.getWidth(), 2.0f);

    g.setFont(juce::Font(22.0f, juce::Font::bold));
    g.setColour(COL_ACCENT);
    g.drawText("IRON SIGNAL", 20, 8, 200, 30, juce::Justification::left, false);

    g.setFont(juce::Font(9.0f));
    g.setColour(COL_MUTED);
    g.drawText("PROG METAL AMP SIM  v1.0", 20, 30, 250, 14, juce::Justification::left, false);

    // VU meter in header
    float vuW = 160.0f;
    float vuX = bounds.getWidth() - vuW - 20;
    float vuY = 16.0f;
    g.setColour(COL_BORDER);
    g.fillRoundedRectangle(vuX, vuY, vuW, 12, 2);
    float fillW = juce::jlimit(0.0f, vuW, vuLevel * vuW);
    juce::ColourGradient grad(juce::Colour(0xff00ff88), vuX, vuY,
                               juce::Colour(0xffff4a00), vuX + vuW, vuY, false);
    grad.addColour(0.7, juce::Colour(0xffffaa00));
    g.setGradientFill(grad);
    g.fillRoundedRectangle(vuX, vuY, fillW, 12, 2);
    g.setColour(COL_MUTED);
    g.setFont(juce::Font(8.0f));
    g.drawText("OUTPUT", (int)vuX, (int)(vuY + 14), (int)vuW, 10, juce::Justification::left);

    // Section panels
    auto drawPanel = [&](juce::Rectangle<float> r, const juce::String& title) {
        g.setColour(COL_PANEL);
        g.fillRoundedRectangle(r, 6);
        g.setColour(COL_BORDER);
        g.drawRoundedRectangle(r, 6, 1.0f);
        g.setColour(COL_ACCENT);
        g.setFont(juce::Font(8.5f, juce::Font::bold));
        g.drawText(title, (int)r.getX() + 8, (int)r.getY() + 6, 120, 12,
                   juce::Justification::left, false);
        g.setColour(COL_BORDER);
        g.fillRect(r.getX() + 6, r.getY() + 20, r.getWidth() - 12, 0.5f);
    };

    drawPanel({ 12,  54, 290, 120 }, "PREAMP / GAIN");
    drawPanel({ 318, 54, 290, 120 }, "EQ");
    drawPanel({ 12, 184, 180, 120 }, "DELAY");
    drawPanel({ 202, 184, 200, 120 }, "OUTPUT");
    drawPanel({ 412, 184, 196, 120 }, "PRESETS");

    // Draw all knob values via custom paint
    auto drawK = [&](IronKnob& k, float x, float y, float size) {
        juce::Rectangle<float> r(x, y, size, size);
        float val = (float)k.getValue();
        paintKnob(g, r, val, k.labelText, COL_ACCENT);
    };

    drawK(gainKnob,      24,  68, 80);
    drawK(tightnessKnob, 110, 68, 80);
    drawK(presenceKnob,  196, 68, 80);

    drawK(bassKnob,   330,  68, 80);
    drawK(midKnob,    416,  68, 80);
    drawK(trebleKnob, 502,  68, 80);

    drawK(delayMixKnob,  22, 198, 68);
    drawK(delayTimeKnob, 96, 198, 68);
    drawK(delayFbKnob,  170, 244, 50);

    drawK(masterKnob, 222, 198, 80);

    // Decorative lines between sections
    g.setColour(COL_BORDER);
    g.fillRect(310.0f, 58.0f, 1.0f, 112.0f);
}

void IronSignalEditor::resized()
{
    // Position knobs (invisible - we draw them manually, but they still need bounds for mouse)
    gainKnob.setBounds(24, 68, 80, 100);
    tightnessKnob.setBounds(110, 68, 80, 100);
    presenceKnob.setBounds(196, 68, 80, 100);

    bassKnob.setBounds(330, 68, 80, 100);
    midKnob.setBounds(416, 68, 80, 100);
    trebleKnob.setBounds(502, 68, 80, 100);

    delayMixKnob.setBounds(22, 198, 68, 90);
    delayTimeKnob.setBounds(96, 198, 68, 90);
    delayFbKnob.setBounds(170, 244, 50, 70);

    masterKnob.setBounds(222, 198, 80, 100);

    // Toggle buttons
    boostBtn.setBounds(24, 146, 80, 22);
    gateBtn.setBounds(110, 146, 80, 22);
    cabBtn.setBounds(202, 302, 90, 22);
    delayBtn.setBounds(170, 222, 54, 22);

    // Preset buttons
    int px = 420, py = 200, pw = 80, ph = 24, pg = 8;
    djentBtn.setBounds(px, py,       pw, ph);
    deathBtn.setBounds(px, py+pg+ph, pw, ph);
    progBtn.setBounds( px, py+(pg+ph)*2, pw, ph);
    cleanBtn.setBounds(px, py+(pg+ph)*3, pw, ph);
}

juce::AudioProcessorEditor* IronSignalProcessor::createEditor()
{
    return new IronSignalEditor(*this);
}
