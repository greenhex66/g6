#include "PluginEditor.h"

//==============================================================================
FractureLookAndFeel::FractureLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (0xff0b0b10));
}

void FractureLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                            float pos, float startAngle, float endAngle,
                                            juce::Slider& slider)
{
    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (8.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float arcR = radius - 3.0f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff22222c));
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, angle, true);
    g.setColour (accent.withAlpha (0.25f));
    g.strokePath (value, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (value, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float bodyR = arcR - 9.0f;
    g.setColour (juce::Colour (0xff14141b));
    g.fillEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour (juce::Colour (0xff2a2a36));
    g.drawEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -bodyR + 4.0f, 3.0f, bodyR * 0.42f, 1.5f);
    g.setColour (accent);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
}

void FractureLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                const juce::Colour&, bool highlighted, bool)
{
    const juce::Colour c ((juce::uint32) (int) button.getProperties()["accent"]);
    const bool on = button.getToggleState();
    auto b = button.getLocalBounds().toFloat().reduced (1.0f);

    g.setColour (on ? c.withAlpha (0.14f) : juce::Colour (0xff13131a));
    g.fillRoundedRectangle (b, 9.0f);

    g.setColour (on ? c : juce::Colour (0xff2a2a36).brighter (highlighted ? 0.25f : 0.0f));
    g.drawRoundedRectangle (b, 9.0f, 1.3f);
}

void FractureLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    const juce::Colour c ((juce::uint32) (int) button.getProperties()["accent"]);
    g.setColour (button.getToggleState() ? c : juce::Colour (0xff77778a));
    g.setFont (12.5f);
    g.drawText (button.getButtonText(), button.getLocalBounds().withTrimmedLeft (14),
                juce::Justification::centred);
}

//==============================================================================
FractureAudioProcessorEditor::FractureAudioProcessorEditor (FractureAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    setSize (560, 330);

    const char* ids[5]   = { "rate", "repeat", "chance", "crush", "mix" };
    const char* names[5] = { "RATE", "REPEAT", "CHANCE", "CRUSH", "MIX" };
    const juce::Colour cols[5] = { juce::Colour (0xffc9b8ff), glitchColour (0), juce::Colour (0xffc9b8ff),
                                   glitchColour (2), juce::Colour (0xffc9b8ff) };

    for (int i = 0; i < 5; ++i)
    {
        auto& s = knobs[(size_t) i].slider;
        knobs[(size_t) i].name = names[i];

        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, true, 80, 18);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                               juce::MathConstants<float>::pi * 2.75f, true);
        s.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffb8b8c6));
        s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        s.setColour (juce::Slider::rotarySliderFillColourId, cols[i]);
        addAndMakeVisible (s);

        sliderAtt.push_back (std::make_unique<SA> (proc.apvts, ids[i], s));
    }

    juce::TextButton* btns[3] = { &btnStutter, &btnReverse, &btnCrush };
    const char* btnIds[3] = { "stutterOn", "reverseOn", "crushOn" };

    for (int i = 0; i < 3; ++i)
    {
        btns[i]->setClickingTogglesState (true);
        btns[i]->getProperties().set ("accent", (int) glitchColour (i).getARGB());
        addAndMakeVisible (*btns[i]);
        buttonAtt.push_back (std::make_unique<BA> (proc.apvts, btnIds[i], *btns[i]));
    }

    startTimerHz (30);
}

FractureAudioProcessorEditor::~FractureAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void FractureAudioProcessorEditor::timerCallback()
{
    const int m = proc.currentMode.load();
    if (m != activeMode)
    {
        activeMode = m;
        repaint();
    }
}

void FractureAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff16131f), 0.0f, 0.0f,
                                             juce::Colour (0xff0a0a0f), 0.0f, (float) getHeight(), false));
    g.fillAll();

    g.setColour (juce::Colour (0xffe6e6f0));
    g.setFont (24.0f);
    g.drawText ("F R A C T U R E", 30, 18, 300, 30, juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xff6a6a7c));
    g.setFont (10.5f);
    g.drawText ("3-MODE GLITCH MACHINE", getWidth() - 230, 22, 200, 24, juce::Justification::centredRight);

    g.setColour (juce::Colour (0xff22222c));
    g.fillRect (30, 58, getWidth() - 60, 1);
    g.fillRect (30, 128, getWidth() - 60, 1);

    g.setColour (juce::Colour (0xff77778a));
    g.setFont (11.0f);
    for (auto& k : knobs)
        g.drawText (k.name, k.labelArea, juce::Justification::centred);

    g.setColour (juce::Colour (0xff3a3a48));
    g.setFont (10.0f);
    g.drawText ("FRACTURE v1.0", 30, getHeight() - 24, 200, 16, juce::Justification::centredLeft);
}

void FractureAudioProcessorEditor::paintOverChildren (juce::Graphics& g)
{
    juce::TextButton* btns[3] = { &btnStutter, &btnReverse, &btnCrush };

    for (int i = 0; i < 3; ++i)
    {
        const auto c = glitchColour (i);
        const auto b = btns[i]->getBounds().toFloat();
        const float d = 7.0f;
        const juce::Rectangle<float> led (b.getX() + 18.0f, b.getCentreY() - d * 0.5f, d, d);
        const bool lit = (activeMode == i + 1);

        if (lit)
        {
            g.setColour (c.withAlpha (0.25f));
            g.fillEllipse (led.expanded (4.0f));
        }

        g.setColour (lit ? c : juce::Colour (0xff2a2a36));
        g.fillEllipse (led);
    }
}

void FractureAudioProcessorEditor::resized()
{
    const int margin = 30;
    const int gap = 12;
    const int bw = (getWidth() - 2 * margin - 2 * gap) / 3;

    juce::TextButton* btns[3] = { &btnStutter, &btnReverse, &btnCrush };
    for (int i = 0; i < 3; ++i)
        btns[i]->setBounds (margin + i * (bw + gap), 74, bw, 40);

    const int kw = (getWidth() - 2 * margin) / 5;
    for (int i = 0; i < 5; ++i)
    {
        knobs[(size_t) i].labelArea = { margin + i * kw, 140, kw, 16 };
        knobs[(size_t) i].slider.setBounds (margin + i * kw, 158, kw, 140);
    }
}
