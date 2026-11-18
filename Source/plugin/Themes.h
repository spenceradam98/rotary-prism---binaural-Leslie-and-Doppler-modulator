// UI looks: Penumbra's five themes plus "Kaleido" (prism colours, rainbow light trails).
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

struct Theme
{
    const char* name;
    juce::Colour bg, bg2, panel, panelEdge, text, dim, accent, knob, knobCap, pointer;
    int style;            // 0 Spectrum, 1 Console, 2 Eclipse, 3 Moon Dust, 4 Tape Room, 5 Kaleido
    bool spectrumAccents; // each section gets its own spectral colour
    bool light;
};

const Theme& themeAt (int index);
int numThemes();
juce::Colour sectionColour (const Theme&, int section, int numSections);

class PrismLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PrismLookAndFeel() { setTheme (0); }
    void setTheme (int index);
    const Theme& theme() const { return *th; }

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;

private:
    const Theme* th = nullptr;
};
