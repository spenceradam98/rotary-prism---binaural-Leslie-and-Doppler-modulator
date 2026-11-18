#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "Themes.h"

// A rotary knob with its name and value painted underneath.
class Knob : public juce::Component
{
public:
    Knob (RotaryPrismProcessor&, const juce::String& paramId, const juce::String& label);
    void paint (juce::Graphics&) override;
    void resized() override;
    juce::Slider slider;
    juce::Colour accent;

private:
    juce::String label;
    juce::RangedAudioParameter* param = nullptr;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
};

// A titled panel holding a row of choices/toggles and a row of knobs.
class Section : public juce::Component
{
public:
    Section (RotaryPrismProcessor&, const juce::String& title, int index);
    Knob& addKnob (const juce::String& id, const juce::String& label);
    void addChoice (const juce::String& id);
    void addToggle (const juce::String& id, const juce::String& label);
    void setAccent (juce::Colour);
    void paint (juce::Graphics&) override;
    void resized() override;

    const int index;

private:
    RotaryPrismProcessor& proc;
    juce::String title;
    juce::Colour accent;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<juce::ComboBox>> combos;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttach;
    std::vector<std::unique_ptr<juce::ToggleButton>> toggles;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> toggleAttach;
};

// Top-down cutaway of the cabinet: horn and drum spin at the modelled speed, mics can be dragged
// (distance from the cabinet and the angle between them). In Binaural mode a listener's head
// sits inside the rotation instead.
class CabinetView : public juce::Component
{
public:
    explicit CabinetView (RotaryPrismProcessor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;

private:
    juce::Point<float> centre() const;
    float cabRadius() const;
    float micRadius (float distance) const;
    juce::Point<float> micPos (int which) const;   // 0 left, 1 right, 2 mono
    int hitMic (juce::Point<float>) const;
    float param (const char* id) const;
    void setParam (const char* id, float value);
    void drawRotors (juce::Graphics&, const Theme&, bool binaural);
    void drawMic (juce::Graphics&, const Theme&, juce::Point<float>, bool hot);
    void drawHead (juce::Graphics&, const Theme&);

    RotaryPrismProcessor& proc;
    int dragging = -1, hover = -1;
};

class RotaryPrismEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit RotaryPrismEditor (RotaryPrismProcessor&);
    ~RotaryPrismEditor() override;
    void paint (juce::Graphics&) override {}
    void resized() override;

    static constexpr int kDesignW = 1120, kDesignH = 680;

private:
    class Content : public juce::Component
    {
    public:
        explicit Content (RotaryPrismEditor& e) : ed (e) {}
        void paint (juce::Graphics&) override;
        void resized() override;
        RotaryPrismEditor& ed;
    };

    void timerCallback() override;
    void applyTheme (int index);
    void refreshPresetBox();
    void setSpeed (int s);
    void syncSpeedButtons();

    RotaryPrismProcessor& proc;
    PrismLookAndFeel lnf;
    Content content { *this };
    CabinetView cabinet { proc };
    std::vector<std::unique_ptr<Section>> sections;
    juce::ComboBox presetBox, themeBox, midiBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> midiAttach;
    juce::TextButton prevPreset { "<" }, nextPreset { ">" };
    juce::TextButton speedBtn[3] { juce::TextButton ("SLOW"), juce::TextButton ("FAST"), juce::TextButton ("BRAKE") };
    juce::ComponentBoundsConstrainer constrainer;
    int lastProgram = -1;
    float limiterShown = 1.0f;

    friend class Content;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RotaryPrismEditor)
};
