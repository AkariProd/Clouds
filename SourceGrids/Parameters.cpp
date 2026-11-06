#include "Parameters.h"

namespace grd
{

namespace pid
{
const juce::String mapX = "mapX";
const juce::String mapY = "mapY";
const juce::String chaos = "chaos";
const juce::String bdDensity = "bdDensity";
const juce::String sdDensity = "sdDensity";
const juce::String hhDensity = "hhDensity";
const juce::String swing = "swing";
const juce::String mode = "mode";
const juce::String euclidBd = "euclidBd";
const juce::String euclidSd = "euclidSd";
const juce::String euclidHh = "euclidHh";
}

using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;
using Range = juce::NormalisableRange<float>;

Layout createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto addKnob = [&] (const juce::String& id, const juce::String& name, float def)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, Range { 0.0f, 1.0f, 0.0001f }, def));
    };

    addKnob (pid::mapX, "Map X", 0.5f);
    addKnob (pid::mapY, "Map Y", 0.5f);
    addKnob (pid::chaos, "Chaos", 0.0f);
    addKnob (pid::bdDensity, "BD Fill", 0.6f);
    addKnob (pid::sdDensity, "SD Fill", 0.5f);
    addKnob (pid::hhDensity, "HH Fill", 0.5f);

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { pid::swing, 1 }, "Swing", false));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { pid::mode, 1 }, "Mode",
        juce::StringArray { "Drums (topographic)", "Euclidean" }, 0));

    addKnob (pid::euclidBd, "Euclid BD Length", 0.5f);
    addKnob (pid::euclidSd, "Euclid SD Length", 0.5f);
    addKnob (pid::euclidHh, "Euclid HH Length", 0.5f);

    return { params.begin(), params.end() };
}

} // namespace grd
