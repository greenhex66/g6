#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <vector>

class FractureAudioProcessor : public juce::AudioProcessor
{
public:
    FractureAudioProcessor();
    ~FractureAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Fracture"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // 0 = none, 1 = stutter, 2 = reverse, 3 = crush (read by the UI)
    std::atomic<int> currentMode { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void startNewSlice();

    int wrap (int x) const noexcept
    {
        x %= bufSize;
        if (x < 0) x += bufSize;
        return x;
    }

    double sampleRate = 44100.0;
    int bufSize = 0;
    std::array<std::vector<float>, 2> ring;
    int writePos = 0;

    int sliceLen = 11025;
    int samplesIntoSlice = 0;
    int sliceStart = 0;
    long long lastSliceIdx = -1000000000LL;

    bool glitchActive = false;
    int renderMode = 0;
    float env = 0.0f;

    float chanceP = 0.5f;
    bool stutterOn = true, reverseOn = true, crushOn = true;
    int repeats = 4;

    float crushStep = 32768.0f;
    float crushFactor = 1.0f;
    float crushHeld[2] { 0.0f, 0.0f };
    float crushPhase[2] { 0.0f, 0.0f };

    juce::Random random;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FractureAudioProcessor)
};
