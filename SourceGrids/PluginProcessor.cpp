#include "PluginProcessor.h"
#include "PluginEditor.h"

ContourProcessor::ContourProcessor()
    : juce::AudioProcessor (BusesProperties()
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    , apvts (*this, nullptr, "PARAMETERS", grd::createLayout())
{
    mapX_ = apvts.getRawParameterValue (grd::pid::mapX);
    mapY_ = apvts.getRawParameterValue (grd::pid::mapY);
    chaos_ = apvts.getRawParameterValue (grd::pid::chaos);
    bdDensity_ = apvts.getRawParameterValue (grd::pid::bdDensity);
    sdDensity_ = apvts.getRawParameterValue (grd::pid::sdDensity);
    hhDensity_ = apvts.getRawParameterValue (grd::pid::hhDensity);
    swing_ = apvts.getRawParameterValue (grd::pid::swing);
    mode_ = apvts.getRawParameterValue (grd::pid::mode);
    euclidBd_ = apvts.getRawParameterValue (grd::pid::euclidBd);
    euclidSd_ = apvts.getRawParameterValue (grd::pid::euclidSd);
    euclidHh_ = apvts.getRawParameterValue (grd::pid::euclidHh);
}

void ContourProcessor::prepareToPlay (double sampleRate, int)
{
    engine_.prepare (sampleRate);
}

bool ContourProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void ContourProcessor::pullParameters()
{
    engine_.setMapX (mapX_->load());
    engine_.setMapY (mapY_->load());
    engine_.setChaos (chaos_->load());
    engine_.setBdDensity (bdDensity_->load());
    engine_.setSdDensity (sdDensity_->load());
    engine_.setHhDensity (hhDensity_->load());
    engine_.setSwing (swing_->load() > 0.5f);
    engine_.setMode (((int) mode_->load()) == 0 ? grd::GridsMode::drums : grd::GridsMode::euclidean);
    engine_.setEuclideanLength (0, euclidBd_->load());
    engine_.setEuclideanLength (1, euclidSd_->load());
    engine_.setEuclideanLength (2, euclidHh_->load());
}

void ContourProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    pullParameters();

    if (buffer.getNumChannels() < 2)
        return;

    engine_.process (buffer.getWritePointer (0), buffer.getWritePointer (1),
                      buffer.getNumSamples(), getPlayHead());
}

juce::AudioProcessorEditor* ContourProcessor::createEditor()
{
    return new ContourEditor (*this);
}

void ContourProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void ContourProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ContourProcessor();
}
