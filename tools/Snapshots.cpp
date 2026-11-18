// Renders the editor in every theme to PNG files (for docs). Build with -DROTARYPRISM_BUILD_SNAPSHOTS=ON;
// on Linux run under a display (e.g. xvfb-run). Usage: rotaryprism_snapshots <out-dir> [preset-index]
#include "../Source/plugin/PluginProcessor.h"
#include "../Source/plugin/PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File dir (argc > 1 ? juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]) : juce::File::getCurrentWorkingDirectory());
    dir.createDirectory();
    RotaryPrismProcessor proc;
    proc.setCurrentProgram (argc > 2 ? juce::String (argv[2]).getIntValue() : 3);
    proc.prepareToPlay (48000, 512);
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    for (int b = 0; b < 200; ++b)   // run some audio so meters and indicators show something
    {
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < 512; ++i) buf.setSample (c, i, 0.3f * std::sin ((float) (b * 512 + i) * 0.03f));
        proc.processBlock (buf, midi);
    }
    for (int t = 0; t < numThemes(); ++t)
    {
        proc.setTheme (t);
        proc.setUiScale (1.0f);
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        ed->setSize (RotaryPrismEditor::kDesignW, RotaryPrismEditor::kDesignH);
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::FileOutputStream out (dir.getChildFile ("theme-" + juce::String (t) + "-" + juce::String (themeAt (t).name).replace (" ", "-").toLowerCase() + ".png"));
        out.setPosition (0); out.truncate();
        juce::PNGImageFormat().writeImageToStream (img, out);
    }
    return 0;
}
