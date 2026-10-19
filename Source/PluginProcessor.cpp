#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

FractureAudioProcessor::FractureAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout FractureAudioProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto percent = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return String ((int) std::round (v)) + "%"; });

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "rate", 1 }, "Rate",
                                                        StringArray { "1/4", "1/8", "1/16", "1/32" }, 1));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "repeat", 1 }, "Repeat",
                                                        StringArray { "2x", "4x", "8x" }, 1));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "chance", 1 }, "Chance",
                                                       NormalisableRange<float> (0.0f, 100.0f, 1.0f), 50.0f, percent));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "crush", 1 }, "Crush",
                                                       NormalisableRange<float> (0.0f, 100.0f, 1.0f), 50.0f, percent));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix",
                                                       NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f, percent));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "stutterOn", 1 }, "Stutter", true));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "reverseOn", 1 }, "Reverse", true));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "crushOn", 1 }, "Crush On", true));

    return layout;
}

void FractureAudioProcessor::prepareToPlay (double sr, int)
{
    sampleRate = sr;
    bufSize = (int) (sr * 6.0);
    for (auto& r : ring)
        r.assign ((size_t) bufSize, 0.0f);

    writePos = 0;
    sliceLen = (int) (sr * 0.25);
    samplesIntoSlice = 0;
    sliceStart = 0;
    lastSliceIdx = -1000000000LL;
    glitchActive = false;
    renderMode = 0;
    env = 0.0f;
    crushHeld[0] = crushHeld[1] = 0.0f;
    crushPhase[0] = crushPhase[1] = 0.0f;
    currentMode.store (0);
}

bool FractureAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void FractureAudioProcessor::startNewSlice()
{
    sliceStart = wrap (writePos - samplesIntoSlice);
    glitchActive = false;

    if (random.nextFloat() < chanceP)
    {
        int options[3];
        int count = 0;
        if (stutterOn) options[count++] = 1;
        if (reverseOn) options[count++] = 2;
        if (crushOn)   options[count++] = 3;

        if (count > 0)
        {
            renderMode = options[random.nextInt (count)];
            glitchActive = true;
        }
    }

    currentMode.store (glitchActive ? renderMode : 0);
}

void FractureAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, numSamples);

    if (bufSize <= 0 || numCh <= 0)
        return;

    // ---- parameters
    const int rateIdx = (int) apvts.getRawParameterValue ("rate")->load();
    const int repIdx  = (int) apvts.getRawParameterValue ("repeat")->load();
    chanceP   = apvts.getRawParameterValue ("chance")->load() * 0.01f;
    const float depth = apvts.getRawParameterValue ("crush")->load() * 0.01f;
    const float mix   = apvts.getRawParameterValue ("mix")->load() * 0.01f;
    stutterOn = apvts.getRawParameterValue ("stutterOn")->load() > 0.5f;
    reverseOn = apvts.getRawParameterValue ("reverseOn")->load() > 0.5f;
    crushOn   = apvts.getRawParameterValue ("crushOn")->load() > 0.5f;

    static const int repeatTable[3] = { 2, 4, 8 };
    static const double beatTable[4] = { 1.0, 0.5, 0.25, 0.125 };
    repeats = repeatTable[juce::jlimit (0, 2, repIdx)];
    const double beatsPerSlice = beatTable[juce::jlimit (0, 3, rateIdx)];

    const float bits = 16.0f - depth * 13.0f;
    crushStep = std::pow (2.0f, bits - 1.0f);
    crushFactor = 1.0f + depth * depth * 30.0f;

    // ---- host tempo / position
    double bpm = 120.0;
    double ppq = 0.0;
    bool hostPlaying = false;
    bool havePpq = false;

    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm())
                bpm = *b;
            hostPlaying = pos->getIsPlaying();
            if (auto p = pos->getPpqPosition())
            {
                ppq = *p;
                havePpq = true;
            }
        }
    }

    bpm = juce::jlimit (20.0, 400.0, bpm);
    sliceLen = juce::jlimit (64, bufSize / 2 - 1,
                             (int) std::round (60.0 / bpm * beatsPerSlice * sampleRate));

    if (hostPlaying && havePpq)
    {
        const double slicePos = ppq / beatsPerSlice + 1.0e-6;
        const long long idx = (long long) std::floor (slicePos);
        const double phase = slicePos - (double) idx;
        samplesIntoSlice = juce::jlimit (0, sliceLen - 1, (int) (phase * (double) sliceLen));

        if (idx != lastSliceIdx)
        {
            startNewSlice();
            lastSliceIdx = idx;
        }
    }
    else
    {
        lastSliceIdx = -1000000000LL;
    }

    float* ch[2] = { nullptr, nullptr };
    for (int c = 0; c < numCh; ++c)
        ch[c] = buffer.getWritePointer (c);

    const float envCoef = 1.0f - std::exp (-1.0f / (0.003f * (float) sampleRate));
    const int chunk = juce::jmax (32, sliceLen / repeats);

    // ---- per-sample processing
    for (int i = 0; i < numSamples; ++i)
    {
        if (samplesIntoSlice >= sliceLen)
        {
            samplesIntoSlice -= sliceLen;
            if (hostPlaying && havePpq)
                ++lastSliceIdx;
            startNewSlice();
        }

        const float target = glitchActive ? 1.0f : 0.0f;
        env += (target - env) * envCoef;
        const int n = samplesIntoSlice;

        for (int c = 0; c < numCh; ++c)
        {
            const float dry = ch[c][i];
            auto& r = ring[(size_t) c];
            r[(size_t) writePos] = dry;

            float wet = dry;

            if (renderMode == 1)            // STUTTER: loop the first part of the slice
            {
                if (n >= chunk)
                    wet = r[(size_t) wrap (sliceStart + (n % chunk))];
            }
            else if (renderMode == 2)       // REVERSE: play the previous slice backwards
            {
                wet = r[(size_t) wrap (sliceStart - 1 - n)];
            }
            else if (renderMode == 3)       // CRUSH: bit + sample-rate reduction
            {
                crushPhase[c] += 1.0f;
                if (crushPhase[c] >= crushFactor)
                {
                    crushPhase[c] -= crushFactor;
                    crushHeld[c] = std::round (dry * crushStep) / crushStep;
                }
                wet = crushHeld[c];
            }

            const float amt = mix * env;
            ch[c][i] = dry + (wet - dry) * amt;
        }

        if (++writePos >= bufSize)
            writePos = 0;
        ++samplesIntoSlice;
    }
}

juce::AudioProcessorEditor* FractureAudioProcessor::createEditor()
{
    return new FractureAudioProcessorEditor (*this);
}

void FractureAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void FractureAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FractureAudioProcessor();
}
