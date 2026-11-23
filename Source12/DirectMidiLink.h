#pragma once
#include <juce_audio_devices/juce_audio_devices.h>
#include "SysexCapture.h"
#include <functional>
#include <memory>

namespace p12 {

// Why this exists: VST3 (the format FL Studio and most other hosts use)
// does not reliably pass SysEx through to/from a hosted plugin -- several
// hosts (FL Studio among them, per Image-Line's own forum/doc answers)
// simply drop it in one or both directions. NRPN (plain MIDI CC) sails
// through fine, which is why hardware<->plugin knob sync works inside FL
// while the SysEx-based full-preset-dump sync does not.
//
// The fix: open a direct connection to the Prophet 12's real MIDI ports
// ourselves, independent of whatever the host is doing with the plugin's
// own MIDI in/out buses, exactly the way the Standalone build (and the
// original Python auto_map_bytes.py mapper) already talk to the hardware.
// This is used ONLY for the full-dump request/reply; notes, NRPN and
// Program Change keep going through the normal host-routed path, which
// already works everywhere.
class DirectMidiLink : private juce::MidiInputCallback
{
public:
    using DumpCallback = std::function<void(std::vector<uint8_t>, bool isEditBuffer, int bank, int program)>;

    ~DirectMidiLink() override { disconnect(); }

    void setOnDumpReceived(DumpCallback cb) { onDump = std::move(cb); }

    // Scans currently available MIDI devices for one whose name contains
    // "prophet" (case-insensitive) and opens it for both directions.
    // Safe to call repeatedly (e.g. from a "Reconnect" button) -- always
    // disconnects first.
    bool autoConnect()
    {
        disconnect();
        bool foundIn = false, foundOut = false;

        for (const auto& dev : juce::MidiInput::getAvailableDevices())
        {
            if (dev.name.containsIgnoreCase("prophet"))
            {
                midiIn = juce::MidiInput::openDevice(dev.identifier, this);
                if (midiIn) { midiIn->start(); foundIn = true; }
                break;
            }
        }
        for (const auto& dev : juce::MidiOutput::getAvailableDevices())
        {
            if (dev.name.containsIgnoreCase("prophet"))
            {
                midiOut = juce::MidiOutput::openDevice(dev.identifier);
                foundOut = (midiOut != nullptr);
                break;
            }
        }
        return foundIn && foundOut;
    }

    void disconnect()
    {
        if (midiIn) midiIn->stop();
        midiIn.reset();
        midiOut.reset();
    }

    bool isConnected() const { return midiIn != nullptr && midiOut != nullptr; }

    juce::String statusText() const
    {
        if (isConnected())
            return "Connected directly: in=\"" + midiIn->getName() + "\", out=\"" + midiOut->getName() + "\"";
        return "Not connected -- plug in the Prophet 12 via USB and click Reconnect";
    }

    void sendDumpRequest()
    {
        if (midiOut)
            midiOut->sendMessageNow(buildRequestEditBufferDump());
    }

private:
    void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& msg) override
    {
        std::vector<uint8_t> unpacked;
        bool isEditBuffer = false;
        int bank = -1, program = -1;
        if (tryParseProphetDump(msg, unpacked, isEditBuffer, bank, program) && onDump)
        {
            // Called back on the OS MIDI thread -- hop to the message
            // thread before touching APVTS parameters / host notifications.
            auto cb = onDump;
            auto data = unpacked;
            juce::MessageManager::callAsync([cb, data, isEditBuffer, bank, program]() mutable
            {
                cb(std::move(data), isEditBuffer, bank, program);
            });
        }
    }

    std::unique_ptr<juce::MidiInput> midiIn;
    std::unique_ptr<juce::MidiOutput> midiOut;
    DumpCallback onDump;
};

}  // namespace p12
