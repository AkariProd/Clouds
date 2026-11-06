#include "PluginProcessor.h"
#include "PluginEditor.h"

ChimeProcessor::ChimeProcessor()
    : juce::AudioProcessor (BusesProperties()
          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    , apvts (*this, nullptr, "PARAMETERS", rng::createLayout())
{
    frequency_       = apvts.getRawParameterValue (rng::pid::frequency);
    frequencyAmount_ = apvts.getRawParameterValue (rng::pid::frequencyAmount);
    frequencyCv_     = apvts.getRawParameterValue (rng::pid::frequencyCv);
    structure_       = apvts.getRawParameterValue (rng::pid::structure);
    structureAmount_ = apvts.getRawParameterValue (rng::pid::structureAmount);
    structureCv_     = apvts.getRawParameterValue (rng::pid::structureCv);
    brightness_       = apvts.getRawParameterValue (rng::pid::brightness);
    brightnessAmount_ = apvts.getRawParameterValue (rng::pid::brightnessAmount);
    brightnessCv_     = apvts.getRawParameterValue (rng::pid::brightnessCv);
    damping_       = apvts.getRawParameterValue (rng::pid::damping);
    dampingAmount_ = apvts.getRawParameterValue (rng::pid::dampingAmount);
    dampingCv_     = apvts.getRawParameterValue (rng::pid::dampingCv);
    position_       = apvts.getRawParameterValue (rng::pid::position);
    positionAmount_ = apvts.getRawParameterValue (rng::pid::positionAmount);
    positionCv_     = apvts.getRawParameterValue (rng::pid::positionCv);
    model_      = apvts.getRawParameterValue (rng::pid::model);
    polyphony_  = apvts.getRawParameterValue (rng::pid::polyphony);
    strumMode_  = apvts.getRawParameterValue (rng::pid::strumMode);
}

void ChimeProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine_.prepare (sampleRate, samplesPerBlock);
}

bool ChimeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void ChimeProcessor::pullParameters()
{
    engine_.setFrequency (frequency_->load());
    engine_.setFrequencyAmount (frequencyAmount_->load());
    engine_.setFrequencyCv (frequencyCv_->load());
    engine_.setStructure (structure_->load());
    engine_.setStructureAmount (structureAmount_->load());
    engine_.setStructureCv (structureCv_->load());
    engine_.setBrightness (brightness_->load());
    engine_.setBrightnessAmount (brightnessAmount_->load());
    engine_.setBrightnessCv (brightnessCv_->load());
    engine_.setDamping (damping_->load());
    engine_.setDampingAmount (dampingAmount_->load());
    engine_.setDampingCv (dampingCv_->load());
    engine_.setPosition (position_->load());
    engine_.setPositionAmount (positionAmount_->load());
    engine_.setPositionCv (positionCv_->load());
    engine_.setModel ((int) model_->load());
    engine_.setPolyphony ((int) polyphony_->load());
    engine_.setStrumMode (static_cast<rng::StrumMode> ((int) strumMode_->load()));
}

void ChimeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn())
            engine_.noteOn (msg.getNoteNumber());
        else if (msg.isNoteOff())
            engine_.noteOff();
    }

    pullParameters();

    if (buffer.getNumChannels() < 2)
        return;

    engine_.process (buffer.getWritePointer (0), buffer.getWritePointer (1), buffer.getNumSamples());
}

juce::AudioProcessorEditor* ChimeProcessor::createEditor()
{
    return new ChimeEditor (*this);
}

void ChimeProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void ChimeProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChimeProcessor();
}
