#include "Parameters.h"

#include <algorithm>
#include <cmath>

namespace rng
{

namespace pid
{
const juce::String frequency       = "frequency";
const juce::String frequencyAmount = "frequencyAmount";
const juce::String frequencyCv     = "frequencyCv";
const juce::String structure       = "structure";
const juce::String structureAmount = "structureAmount";
const juce::String structureCv     = "structureCv";
const juce::String brightness       = "brightness";
const juce::String brightnessAmount = "brightnessAmount";
const juce::String brightnessCv     = "brightnessCv";
const juce::String damping       = "damping";
const juce::String dampingAmount = "dampingAmount";
const juce::String dampingCv     = "dampingCv";
const juce::String position       = "position";
const juce::String positionAmount = "positionAmount";
const juce::String positionCv     = "positionCv";

const juce::String model      = "model";
const juce::String polyphony  = "polyphony";
const juce::String strumMode  = "strumMode";
}

float quadraticBipolarLaw (float knob01)
{
    float v = knob01 - 0.5f;
    const float v2 = v * v * 4.0f * 3.3f;
    return v < 0.0f ? -v2 : v2;
}

float quarticBipolarLaw (float knob01)
{
    float v = knob01 - 0.5f;
    const float v2 = v * v * 4.0f;
    const float v4 = v2 * v2 * 3.3f;
    return v < 0.0f ? -v4 : v4;
}

using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;
using Range  = juce::NormalisableRange<float>;

Layout createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto addKnob = [&] (const juce::String& id, const juce::String& name, float def)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, Range { 0.0f, 1.0f, 0.0001f }, def));
    };
    auto addBipolarCv = [&] (const juce::String& id, const juce::String& name)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, Range { -1.0f, 1.0f, 0.0001f }, 0.0f));
    };

    addKnob (pid::frequency, "Frequency", 0.5f);
    addKnob (pid::frequencyAmount, "Frequency Amount", 0.5f);
    addBipolarCv (pid::frequencyCv, "Frequency CV");

    addKnob (pid::structure, "Structure", 0.5f);
    addKnob (pid::structureAmount, "Structure Amount", 0.5f);
    addBipolarCv (pid::structureCv, "Structure CV");

    addKnob (pid::brightness, "Brightness", 0.5f);
    addKnob (pid::brightnessAmount, "Brightness Amount", 0.5f);
    addBipolarCv (pid::brightnessCv, "Brightness CV");

    addKnob (pid::damping, "Damping", 0.5f);
    addKnob (pid::dampingAmount, "Damping Amount", 0.5f);
    addBipolarCv (pid::dampingCv, "Damping CV");

    addKnob (pid::position, "Position", 0.5f);
    addKnob (pid::positionAmount, "Position Amount", 0.5f);
    addBipolarCv (pid::positionCv, "Position CV");

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { pid::model, 1 }, "Model",
        juce::StringArray { "Modal Resonator", "Sympathetic Strings", "Modulated/Inharmonic String",
                             "FM Voice", "Sympathetic Strings (Quantized)", "String + Reverb" },
        0));

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { pid::polyphony, 1 }, "Polyphony", 1, 4, 1));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { pid::strumMode, 1 }, "Strum Mode",
        juce::StringArray { "External Trigger", "Note Change", "Audio Onset" },
        0));

    return { params.begin(), params.end() };
}

} // namespace rng
