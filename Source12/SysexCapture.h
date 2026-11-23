#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <mutex>

namespace p12 {

// DSI/Sequential manufacturer ID + Prophet 12 device ID, from the manual's
// MIDI Implementation chapter.
constexpr uint8_t kDsiManufacturerId = 0x01;
constexpr uint8_t kProphet12DeviceId = 0x2A;
constexpr uint8_t kCmdRequestEditBufferDump = 0x06;
constexpr uint8_t kCmdProgramData = 0x02;       // reply carries bank + program number
constexpr uint8_t kCmdEditBufferData = 0x03;    // reply has no bank/program

// Builds "F0 01 2A 06 F7" -- Request Program Edit Buffer Transmit.
inline juce::MidiMessage buildRequestEditBufferDump()
{
    const uint8_t bytes[] { kDsiManufacturerId, kProphet12DeviceId, kCmdRequestEditBufferDump };
    return juce::MidiMessage::createSysExMessage(bytes, (int) sizeof(bytes));
}

// Un-does the Prophet 12's "packed MS bit" format (manual, Appendix D,
// "Packed Data Format"): every 8 packed MIDI bytes encode 7 real data
// bytes -- the first byte of each group of 8 holds bit 7 (the MSB) of each
// of the following 7 bytes, which are otherwise sent with that MSB
// stripped (so every packed byte is a plain 7-bit MIDI data byte).
inline std::vector<uint8_t> unpackProphetData(const uint8_t* packed, int packedLen)
{
    std::vector<uint8_t> out;
    out.reserve((size_t) (packedLen * 7 / 8 + 8));
    int i = 0;
    while (i < packedLen)
    {
        const uint8_t msbBits = packed[i++];
        for (int bit = 0; bit < 7 && i < packedLen; ++bit)
        {
            const uint8_t lo7 = packed[i++];
            const uint8_t msb = (msbBits >> bit) & 0x01;
            out.push_back((uint8_t) ((msb << 7) | lo7));
        }
    }
    return out;
}

// Parses a complete incoming sysex message. Returns true and fills
// outUnpacked (the 1024-ish raw program bytes) if it was a recognised
// Program Data or Program Edit Buffer Data dump from a Prophet 12.
inline bool tryParseProphetDump(const juce::MidiMessage& msg, std::vector<uint8_t>& outUnpacked,
                                 bool& outWasEditBuffer, int& outBank, int& outProgram)
{
    if (!msg.isSysEx())
        return false;
    const auto* data = msg.getSysExData();
    const int len = msg.getSysExDataSize();
    if (len < 3 || data[0] != kDsiManufacturerId || data[1] != kProphet12DeviceId)
        return false;

    const uint8_t cmd = (uint8_t) data[2];
    int payloadStart = 3;
    outBank = outProgram = -1;
    outWasEditBuffer = (cmd == kCmdEditBufferData);

    if (cmd == kCmdProgramData)
    {
        if (len < 5) return false;
        outBank = data[3];
        outProgram = data[4];
        payloadStart = 5;
    }
    else if (cmd != kCmdEditBufferData)
    {
        return false;  // not a dump we understand (e.g. Global Parameter Data, device inquiry reply)
    }

    outUnpacked = unpackProphetData((const uint8_t*) data + payloadStart, len - payloadStart);
    return true;
}

// Message-thread-safe holder for the most recent capture, written from the
// audio thread (processBlock, when a dump finishes arriving) and read from
// the editor's Timer. Sysex dumps are rare, large, one-off events, not a
// hot path, so a plain mutex is an acceptable, simple choice here (unlike
// MidiActivityLog's lock-free ring buffer, which really is hot-path).
class SysexCaptureLog
{
public:
    void publish(std::vector<uint8_t> unpacked, bool wasEditBuffer, int bank, int program)
    {
        std::lock_guard<std::mutex> lock(mutex);
        latest = std::move(unpacked);
        latestIsEditBuffer = wasEditBuffer;
        latestBank = bank;
        latestProgram = program;
        ++sequence;
    }

    // Returns true and fills the out-params if a new capture has arrived
    // since lastSeenSequence (caller should pass its own tracked value in
    // and update it from outSequence after a successful read).
    bool takeIfNew(int lastSeenSequence, std::vector<uint8_t>& out, bool& outIsEditBuffer,
                    int& outBank, int& outProgram, int& outSequence)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (sequence == lastSeenSequence)
            return false;
        out = latest;
        outIsEditBuffer = latestIsEditBuffer;
        outBank = latestBank;
        outProgram = latestProgram;
        outSequence = sequence;
        return true;
    }

private:
    std::mutex mutex;
    std::vector<uint8_t> latest;
    bool latestIsEditBuffer = false;
    int latestBank = -1, latestProgram = -1;
    int sequence = 0;
};

}  // namespace p12
