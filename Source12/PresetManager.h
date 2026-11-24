#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace p12 {

// Native, in-plugin preset system -- deliberately NOT the host's own
// preset/patch browser (FL Studio's channel presets, Ableton's preset
// browser, etc.), which differs per DAW and isn't available the same way
// (or at all) everywhere. A Seer12 preset is a plain XML file holding a
// full APVTS state snapshot (the same format getStateInformation already
// produces), stored once per machine under the user's application-data
// folder, so Save/Load work identically in any host Seer12 is loaded into.
//
// Loading a preset only replaces the APVTS state -- it does NOT by itself
// push anything to the hardware. The processor is responsible for calling
// forceSendAllParams() after a successful load (see loadPreset/stepPreset
// call sites in PluginProcessor.cpp), same as it already does for a DAW
// project's own saved state (setStateInformation).
class PresetManager
{
public:
    explicit PresetManager(juce::AudioProcessorValueTreeState& state) : apvts(state)
    {
        presetDir.createDirectory();
    }

    static juce::File presetDirectory()
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("Seer12")
            .getChildFile("Presets");
    }

    juce::StringArray getPresetNames() const
    {
        juce::StringArray names;
        for (const auto& f : presetDir.findChildFiles(juce::File::findFiles, false, "*.xml"))
            names.add(f.getFileNameWithoutExtension());
        names.sort(true);
        return names;
    }

    juce::String getCurrentPresetName() const { return currentName; }

    bool savePresetAs(const juce::String& name)
    {
        if (name.trim().isEmpty())
            return false;
        auto state = apvts.copyState();
        if (! state.isValid())
            return false;
        if (auto xml = state.createXml())
        {
            if (xml->writeTo(fileFor(name)))
            {
                currentName = name;
                return true;
            }
        }
        return false;
    }

    bool loadPreset(const juce::String& name)
    {
        const auto file = fileFor(name);
        if (! file.existsAsFile())
            return false;
        if (auto xml = juce::XmlDocument::parse(file))
        {
            auto tree = juce::ValueTree::fromXml(*xml);
            if (tree.isValid())
            {
                apvts.replaceState(tree);
                currentName = name;
                return true;
            }
        }
        return false;
    }

    bool deletePreset(const juce::String& name)
    {
        const bool ok = fileFor(name).deleteFile();
        if (ok && currentName == name)
            currentName.clear();
        return ok;
    }

    // Loads the preset alphabetically before/after the current one (wraps
    // around). Returns the new current name, or an empty string if there
    // are no saved presets at all.
    juce::String stepPreset(int delta)
    {
        const auto names = getPresetNames();
        if (names.isEmpty())
            return {};
        int idx = names.indexOf(currentName);
        idx = (idx < 0 ? 0 : (idx + delta + names.size()) % names.size());
        loadPreset(names[idx]);
        return names[idx];
    }

private:
    juce::File fileFor(const juce::String& name) const
    {
        return presetDir.getChildFile(juce::File::createLegalFileName(name) + ".xml");
    }

    juce::AudioProcessorValueTreeState& apvts;
    juce::File presetDir { presetDirectory() };
    juce::String currentName;
};

}  // namespace p12
