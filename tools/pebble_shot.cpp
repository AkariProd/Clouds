// Offscreen render of the real Pebble editor. Dev tool: verifies the panel
// without a host (FL Studio, computer_use, etc). Writes pebble*.png into the
// directory given as argv[1]. See DESIGN.md section 10 ("how to work").
#include <juce_gui_basics/juce_gui_basics.h>

#include "Panel.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

namespace
{
void set (PebbleProcessor& p, const juce::String& id, float normalised)
{
    if (auto* param = p.apvts.getParameter (id))
        param->setValueNotifyingHost (normalised);
}

void shoot (PebbleProcessor& p, const juce::File& out)
{
    std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditor());
    editor->setSize ((int) bds::panel::designW, (int) bds::panel::designH);

    auto img = editor->createComponentSnapshot (editor->getLocalBounds(), true, 2.0f);

    juce::FileOutputStream stream (out);
    stream.setPosition (0);
    stream.truncate();
    juce::PNGImageFormat().writeImageToStream (img, stream);
    std::printf ("%s\n", out.getFullPathName().toRawUTF8());
}
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File outDir (argc > 1 ? juce::String (argv[1]) : juce::String ("."));

    {   // Default: everything at its power-up value.
        PebbleProcessor p;
        p.setRateAndBufferSizeDetails (48000.0, 128);
        p.prepareToPlay (48000.0, 128);
        shoot (p, outDir.getChildFile ("pebble.png"));
    }

    {   // Played-in: mid-performance, grain cloud populated, live signal.
        PebbleProcessor p;
        p.setRateAndBufferSizeDetails (48000.0, 128);
        p.prepareToPlay (48000.0, 128);
        set (p, bds::pid::density, 0.7f);
        set (p, bds::pid::time, 0.4f);
        set (p, bds::pid::pitch, 0.8f);
        set (p, bds::pid::size, 0.55f);
        set (p, bds::pid::shape, 0.3f);
        set (p, bds::pid::timeAmount, 0.65f);
        set (p, bds::pid::sizeAmount, 0.3f);
        set (p, bds::pid::feedback, 0.4f);
        set (p, bds::pid::dryWet, 0.6f);
        set (p, bds::pid::reverb, 0.2f);
        p.panel.envIn.store (0.22f);
        p.panel.envOut.store (0.3f);
        shoot (p, outDir.getChildFile ("pebble_played.png"));
    }

    {   // Frozen: buffer latched, check the frozen-state treatment.
        PebbleProcessor p;
        p.setRateAndBufferSizeDetails (48000.0, 128);
        p.prepareToPlay (48000.0, 128);
        set (p, bds::pid::freeze, 1.0f);
        set (p, bds::pid::density, 0.5f);
        set (p, bds::pid::media, 2.0f / 3.0f); // vintage digital
        p.panel.envIn.store (0.1f);
        p.panel.envOut.store (0.15f);
        shoot (p, outDir.getChildFile ("pebble_frozen.png"));
    }

    return 0;
}
