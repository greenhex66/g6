#pragma once
#include "PluginProcessor.h"
#include <array>
#include <memory>
#include <vector>

inline juce::Colour glitchColour (int i)
{
    static const juce::uint32 cols[3] = { 0xff5ef2d0, 0xffb48cff, 0xffff6b8a };
    return juce::Colour (cols[juce::jlimit (0, 2, i)]);
}

class FractureLookAndFeel : public juce::LookAndFeel_V4
{
public:
    FractureLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics&, juce::TextButton&,
                         bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

class FractureAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit FractureAudioProcessorEditor (FractureAudioProcessor&);
    ~FractureAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    FractureAudioProcessor& proc;
    FractureLookAndFeel laf;

    struct Knob
    {
        juce::Slider slider;
        juce::String name;
        juce::Rectangle<int> labelArea;
    };

    std::array<Knob, 5> knobs;
    juce::TextButton btnStutter { "STUTTER" }, btnReverse { "REVERSE" }, btnCrush { "CRUSH" };
    int activeMode = 0;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::vector<std::unique_ptr<SA>> sliderAtt;
    std::vector<std::unique_ptr<BA>> buttonAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FractureAudioProcessorEditor)
};
