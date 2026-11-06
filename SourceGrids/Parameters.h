#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace grd
{

namespace pid
{
extern const juce::String mapX;
extern const juce::String mapY;
extern const juce::String chaos;
extern const juce::String bdDensity;
extern const juce::String sdDensity;
extern const juce::String hhDensity;
extern const juce::String swing;
extern const juce::String mode;
extern const juce::String euclidBd;
extern const juce::String euclidSd;
extern const juce::String euclidHh;
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

} // namespace grd
