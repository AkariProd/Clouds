#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

Seer12Processor::Seer12Processor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , apvts(*this, nullptr, "PARAMETERS", p12::createParameterLayout())
{
    for (int i = 0; i < p12::kNumParams; ++i)
    {
        pendingOutRaw[(size_t) i].store(0, std::memory_order_relaxed);
        pendingFlag[(size_t) i].store(false, std::memory_order_relaxed);
        idToIndex[p12::kParams[i].id] = i;
        apvts.addParameterListener(p12::kParams[i].id, this);
    }
}

Seer12Processor::~Seer12Processor()
{
    for (const auto& def : p12::kParams)
        apvts.removeParameterListener(def.id, this);
}

bool Seer12Processor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // No audio in; stereo (silent) out only, so hosts happy to slot an
    // "instrument" plugin into an instrument track with the usual stereo
    // output bus. This plugin makes no sound of its own -- the real
    // instrument is the hardware synth it's driving.
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
           && layouts.getMainInputChannelSet() == juce::AudioChannelSet::disabled();
}

void Seer12Processor::prepareToPlay(double, int) {}

void Seer12Processor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (suppressOutgoing.load(std::memory_order_relaxed))
        return;  // this change came FROM incoming hardware MIDI -- don't echo it back

    auto it = idToIndex.find(parameterID.toStdString());
    if (it == idToIndex.end())
        return;
    const int paramIndex = it->second;
    const auto& def = p12::kParams[paramIndex];

    int raw;
    if (def.type == p12::ParamType::Toggle)
        raw = newValue > 0.5f ? 1 : 0;
    else
        raw = (int) std::lround(newValue);  // AudioParameterInt/Choice newValue is already the real value

    raw = juce::jlimit(def.minValue, def.maxValue, raw);
    pendingOutRaw[(size_t) paramIndex].store(raw, std::memory_order_relaxed);
    pendingFlag[(size_t) paramIndex].store(true, std::memory_order_release);
}

void Seer12Processor::sendNrpnOut(juce::MidiBuffer& out, int paramIndex, int rawValue, int samplePosition)
{
    const auto& def = p12::kParams[paramIndex];
    const int nrpn = def.nrpnA;  // transmit on Layer A's number
    const int ch = juce::jlimit(1, 16, midiChannel.load(std::memory_order_relaxed));

    out.addEvent(juce::MidiMessage::controllerEvent(ch, 99, (nrpn >> 7) & 0x7F), samplePosition);
    out.addEvent(juce::MidiMessage::controllerEvent(ch, 98, nrpn & 0x7F), samplePosition);

    if (def.maxValue > 127)
    {
        const int clamped = juce::jlimit(0, 16383, rawValue);
        out.addEvent(juce::MidiMessage::controllerEvent(ch, 6, (clamped >> 7) & 0x7F), samplePosition);
        out.addEvent(juce::MidiMessage::controllerEvent(ch, 38, clamped & 0x7F), samplePosition);
    }
    else
    {
        out.addEvent(juce::MidiMessage::controllerEvent(ch, 6, juce::jlimit(0, 127, rawValue)), samplePosition);
    }

    midiLog.incrementSentOut();
}

void Seer12Processor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    juce::MidiBuffer outgoing;

    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();

        if (msg.isController())
        {
            midiLog.pushRaw(msg.getChannel(), msg.getControllerNumber(), msg.getControllerValue());

            int paramIndex = -1, rawValue = -1;
            if (nrpnDecoder.handleControllerChange(msg.getControllerNumber(), msg.getControllerValue(),
                                                    paramIndex, rawValue))
            {
                const auto& def = p12::kParams[paramIndex];
                if (auto* param = apvts.getParameter(def.id))
                {
                    float normalised;
                    if (def.type == p12::ParamType::Toggle)
                    {
                        normalised = rawValue > 0 ? 1.0f : 0.0f;
                    }
                    else
                    {
                        const int clamped = juce::jlimit(def.minValue, def.maxValue, rawValue);
                        normalised = param->convertTo0to1(static_cast<float>(clamped));
                    }
                    suppressOutgoing.store(true, std::memory_order_relaxed);
                    param->setValueNotifyingHost(normalised);
                    suppressOutgoing.store(false, std::memory_order_relaxed);
                    lastNrpnParamIndex.store(paramIndex, std::memory_order_relaxed);
                    midiLog.markLastDecoded(paramIndex, rawValue);
                }
            }
            // the four NRPN plumbing CCs (99/98/6/38) are hardware->plugin
            // only; don't forward them on to MIDI out.
        }
        else
        {
            // notes, pitch bend, (poly/channel) pressure, program change --
            // pass straight through so a pattern on this instrument track
            // actually plays the Prophet 12.
            outgoing.addEvent(msg, metadata.samplePosition);
        }
    }

    // Flush any GUI/automation knob moves made since the last block as
    // outgoing NRPN, so the hardware mirrors what's shown on screen too.
    for (int i = 0; i < p12::kNumParams; ++i)
    {
        if (pendingFlag[(size_t) i].exchange(false, std::memory_order_acquire))
            sendNrpnOut(outgoing, i, pendingOutRaw[(size_t) i].load(std::memory_order_relaxed), 0);
    }

    midi.swapWith(outgoing);
}

juce::AudioProcessorEditor* Seer12Processor::createEditor()
{
    return new Seer12Editor(*this);
}

void Seer12Processor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        juce::MemoryOutputStream stream(destData, true);
        state.writeToStream(stream);
    }
}

void Seer12Processor::setStateInformation(const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes));
    if (tree.isValid())
        apvts.replaceState(tree);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Seer12Processor();
}
