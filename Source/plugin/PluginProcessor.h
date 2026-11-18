#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Engine.h"

class RotaryPrismProcessor : public juce::AudioProcessor
{
public:
    RotaryPrismProcessor();
    ~RotaryPrismProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    const rp::Engine& getEngine() const { return engine; }

    // UI settings stored with the session (not automatable)
    int getTheme() const { return theme; }
    void setTheme (int t) { theme = t; }
    float getUiScale() const { return uiScale; }
    void setUiScale (float s) { uiScale = s; }

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static juce::String formatValue (const rp::ParamDesc&, float);

private:
    rp::Params readParams() const;
    void handleMidi (const juce::MidiMessage&);

    rp::Engine engine;
    std::vector<std::atomic<float>*> raw;    // parallel to rp::paramTable
    std::vector<float> scratchL, scratchR;
    int currentProgram = 0;
    int theme = 5;
    float uiScale = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RotaryPrismProcessor)
};
