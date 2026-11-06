#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

PebbleProcessor::PebbleProcessor()
    : juce::AudioProcessor (BusesProperties()
          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    , apvts (*this, nullptr, "PARAMETERS", bds::createLayout())
{
    time_        = apvts.getRawParameterValue (bds::pid::time);
    timeAmount_  = apvts.getRawParameterValue (bds::pid::timeAmount);
    density_     = apvts.getRawParameterValue (bds::pid::density);
    pitch_       = apvts.getRawParameterValue (bds::pid::pitch);
    pitchAmount_ = apvts.getRawParameterValue (bds::pid::pitchAmount);
    size_        = apvts.getRawParameterValue (bds::pid::size);
    sizeAmount_  = apvts.getRawParameterValue (bds::pid::sizeAmount);
    shape_       = apvts.getRawParameterValue (bds::pid::shape);
    shapeAmount_ = apvts.getRawParameterValue (bds::pid::shapeAmount);
    feedback_    = apvts.getRawParameterValue (bds::pid::feedback);
    dryWet_      = apvts.getRawParameterValue (bds::pid::dryWet);
    reverb_      = apvts.getRawParameterValue (bds::pid::reverb);
    freeze_      = apvts.getRawParameterValue (bds::pid::freeze);
    seed_        = apvts.getRawParameterValue (bds::pid::seed);
    seedLock_    = apvts.getRawParameterValue (bds::pid::seedLock);
    media_       = apvts.getRawParameterValue (bds::pid::media);
    monoGrain_   = apvts.getRawParameterValue (bds::pid::monoGrain);
    outputGate_  = apvts.getRawParameterValue (bds::pid::outputGate);
}

void PebbleProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine_.prepare (sampleRate, samplesPerBlock);
}

bool PebbleProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void PebbleProcessor::pullParameters()
{
    engine_.setTime (time_->load());
    engine_.setTimeAmount (timeAmount_->load());
    engine_.setDensity (density_->load());
    engine_.setPitch (pitch_->load());
    engine_.setPitchAmount (pitchAmount_->load());
    engine_.setSize (size_->load());
    engine_.setSizeAmount (sizeAmount_->load());
    engine_.setShape (shape_->load());
    engine_.setShapeAmount (shapeAmount_->load());
    engine_.setFeedback (feedback_->load());
    engine_.setDryWet (dryWet_->load());
    engine_.setReverb (reverb_->load());
    engine_.setFreeze (freeze_->load() > 0.5f);
    engine_.setSeed (seed_->load() > 0.5f);
    engine_.setSeedLock (seedLock_->load() > 0.5f);
    engine_.setMedia ((int) media_->load());
    engine_.setMonoGrain (monoGrain_->load() > 0.5f);
    engine_.setOutputGate (outputGate_->load() > 0.5f);
}

void PebbleProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn())
            engine_.triggerSeedInput();
    }

    pullParameters();

    if (buffer.getNumChannels() < 2)
        return;

    const int n = buffer.getNumSamples();

    float inPeak = 0.0f, inSumSq = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
    {
        const auto* d = buffer.getReadPointer (ch);
        for (int i = 0; i < n; ++i)
        {
            inPeak = juce::jmax (inPeak, std::abs (d[i]));
            inSumSq += d[i] * d[i];
        }
    }

    engine_.process (buffer.getWritePointer (0), buffer.getWritePointer (1), buffer.getNumSamples());

    float outSumSq = 0.0f;
    for (int ch = 0; ch < 2; ++ch)
    {
        const auto* d = buffer.getReadPointer (ch);
        for (int i = 0; i < n; ++i)
            outSumSq += d[i] * d[i];
    }

    const float norm = n > 0 ? 1.0f / (float) (2 * n) : 0.0f;
    panel.envIn.store  (std::sqrt (inSumSq * norm),  std::memory_order_relaxed);
    panel.envOut.store (std::sqrt (outSumSq * norm), std::memory_order_relaxed);
    panel.inPeak.store (inPeak, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* PebbleProcessor::createEditor()
{
    return new PebbleEditor (*this);
}

void PebbleProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); true)
    {
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
    }
}

void PebbleProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PebbleProcessor();
}
