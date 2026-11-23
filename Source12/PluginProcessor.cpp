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

    // Always send the full 14-bit (MSB, LSB) pair, matching what the
    // hardware itself transmits -- it always sends both bytes too, even for
    // narrow-range params where the MSB is just a constant 0. Sending CC6
    // alone (as this used to do for "narrow" params) left the MSB implicit,
    // which the hardware can't be relied on to zero-fill correctly.
    const int clamped = juce::jlimit(0, 16383, rawValue);
    out.addEvent(juce::MidiMessage::controllerEvent(ch, 6, (clamped >> 7) & 0x7F), samplePosition);
    out.addEvent(juce::MidiMessage::controllerEvent(ch, 38, clamped & 0x7F), samplePosition);

    midiLog.incrementSentOut();
}

void Seer12Processor::requestEditBufferDump()
{
    pendingDumpRequest.store(true, std::memory_order_release);
}

void Seer12Processor::changeProgram(int delta)
{
    int cur = currentProgramGuess.load(std::memory_order_relaxed);
    int next = juce::jlimit(0, 127, cur + delta);
    currentProgramGuess.store(next, std::memory_order_relaxed);
    pendingProgramChange.store(next, std::memory_order_release);
}

void Seer12Processor::applyFullDump(const std::vector<uint8_t>& dump)
{
    int applied = 0;
    suppressOutgoing.store(true, std::memory_order_relaxed);
    for (int i = 0; i < p12::kNumParams; ++i)
    {
        const auto& def = p12::kParams[i];
        if (def.dumpByteOffset < 0 || def.dumpByteOffset >= (int) dump.size())
            continue;
        auto* param = apvts.getParameter(def.id);
        if (param == nullptr)
            continue;

        const int raw = juce::jlimit(def.minValue, def.maxValue, (int) dump[(size_t) def.dumpByteOffset]);
        const float normalised = (def.type == p12::ParamType::Toggle)
                                      ? (raw > 0 ? 1.0f : 0.0f)
                                      : param->convertTo0to1((float) raw);
        param->setValueNotifyingHost(normalised);
        ++applied;
    }
    suppressOutgoing.store(false, std::memory_order_relaxed);

    // Patch name: bytes 402-421, ASCII, per the manual's "Name Character
    // 1-20" NRPN entries (byte offset confirmed against a known-working
    // open-source Prophet 12 patch librarian, since the manual itself
    // doesn't publish a byte-offset table for the dump the way it does
    // for NRPN numbers).
    juce::String name;
    constexpr int kNamePos = 402, kNameLen = 20;
    if ((int) dump.size() >= kNamePos + kNameLen)
    {
        for (int i = 0; i < kNameLen; ++i)
        {
            const char c = (char) dump[(size_t) (kNamePos + i)];
            if (c >= 32 && c < 127)
                name << c;
        }
        name = name.trim();
    }
    {
        const juce::ScopedLock sl(patchNameLock);
        lastSyncedPatchName = name;
    }

    fullSyncAppliedCount.store(applied, std::memory_order_relaxed);
    fullSyncSequence.fetch_add(1, std::memory_order_relaxed);
}

juce::String Seer12Processor::getLastSyncedPatchName() const
{
    const juce::ScopedLock sl(patchNameLock);
    return lastSyncedPatchName;
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
        else if (msg.isSysEx())
        {
            std::vector<uint8_t> unpacked;
            bool isEditBuffer = false;
            int bank = -1, program = -1;
            if (p12::tryParseProphetDump(msg, unpacked, isEditBuffer, bank, program))
            {
                sysexLog.publish(unpacked, isEditBuffer, bank, program);
                applyFullDump(unpacked);
            }
            else
                outgoing.addEvent(msg, metadata.samplePosition);  // some other sysex -- pass through
        }
        else if (msg.isProgramChange())
        {
            // The hardware just recalled a different patch (requires its
            // GLOBAL > MIDI Program Change Send to be on) -- NRPN alone
            // never broadcasts the whole patch, so ask for a full dump to
            // actually sync every knob instead of leaving them all stale.
            pendingDumpRequest.store(true, std::memory_order_release);
            outgoing.addEvent(msg, metadata.samplePosition);
        }
        else
        {
            // notes, pitch bend, (poly/channel) pressure --
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

    if (pendingDumpRequest.exchange(false, std::memory_order_acquire))
        outgoing.addEvent(p12::buildRequestEditBufferDump(), 0);

    const int pc = pendingProgramChange.exchange(-1, std::memory_order_acquire);
    if (pc >= 0)
    {
        const int ch = juce::jlimit(1, 16, midiChannel.load(std::memory_order_relaxed)) - 1;
        outgoing.addEvent(juce::MidiMessage::programChange(ch + 1, pc), 0);
        pendingDumpRequest.store(true, std::memory_order_release);  // follow up with a sync
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
