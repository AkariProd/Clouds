#include "Parameters.h"

#include <algorithm>
#include <cmath>

namespace bds
{

namespace pid
{
const juce::String time        = "time";
const juce::String timeAmount  = "timeAmount";
const juce::String density     = "density";
const juce::String pitch       = "pitch";
const juce::String pitchAmount = "pitchAmount";
const juce::String size        = "size";
const juce::String sizeAmount  = "sizeAmount";
const juce::String shape       = "shape";
const juce::String shapeAmount = "shapeAmount";

const juce::String feedback = "feedback";
const juce::String dryWet   = "dryWet";
const juce::String reverb   = "reverb";

const juce::String freeze   = "freeze";
const juce::String seed     = "seed";
const juce::String seedLock = "seedLock";

const juce::String media      = "media";
const juce::String monoGrain  = "monoGrain";
const juce::String outputGate = "outputGate";
}

float attenuverterLaw (float knob01)
{
    float v = knob01 - 0.5f;
    v = (2.0f / 0.47f) * v * std::max (std::abs (v) - 0.03f, 0.0f);
    return v;
}

float detuneLaw (float knob01)
{
    float detune = 2.0f * knob01 - 1.0f;
    const float sign = detune < 0.0f ? -1.0f : 1.0f;
    detune = detune * sign * 6.0f;

    int integral = (int) detune;
    float fractional = detune - (float) integral;
    integral = std::min (integral, 6);

    fractional = std::max ((fractional - 0.33333333f) * 1.5f, 0.0f);
    fractional = std::min (fractional, 1.0f);
    // Hardware's SmoothStep.
    const float t = fractional;
    const float smooth = t * t * (3.0f - 2.0f * t);

    const float intervals[8] = { 0.0f, 1.0f, 3.0f, 4.0f, 7.0f, 12.0f, 24.0f, 24.0f };
    const float a = intervals[integral];
    const float b = intervals[integral + 1];
    const float crossfade = a + (b - a) * smooth;

    return crossfade * sign;
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

    addKnob (pid::time,        "Time",         0.5f);
    addKnob (pid::timeAmount,  "Time Amount",  0.5f);
    addKnob (pid::density,     "Density",      0.5f);
    addKnob (pid::pitch,       "Pitch",        0.5f);
    addKnob (pid::pitchAmount, "Pitch Amount", 0.5f);
    addKnob (pid::size,        "Size",         0.5f);
    addKnob (pid::sizeAmount,  "Size Amount",  0.5f);
    addKnob (pid::shape,       "Shape",        0.5f);
    addKnob (pid::shapeAmount, "Shape Amount", 0.5f);

    addKnob (pid::feedback, "Feedback", 0.0f);
    addKnob (pid::dryWet,   "Dry/Wet",  0.5f);
    addKnob (pid::reverb,   "Reverb",   0.0f);

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { pid::freeze, 1 }, "Freeze", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { pid::seed, 1 }, "Seed", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { pid::seedLock, 1 }, "Seed Lock", false));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { pid::media, 1 }, "Media",
        juce::StringArray { "Dirty Tape", "Clean Tape", "Vintage Digital", "Digital" },
        3));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { pid::monoGrain, 1 }, "Mono Grain", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { pid::outputGate, 1 }, "Output Gate", false));

    return { params.begin(), params.end() };
}

} // namespace bds
