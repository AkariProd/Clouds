#pragma once
#include <array>
#include <atomic>
#include <juce_core/juce_core.h>

namespace p12 {

struct MidiLogEntry
{
    int channel = 0;
    int cc = -1;
    int value = -1;
    bool isNrpnDecoded = false;
    int paramIndex = -1;
    int decodedValue = -1;
};

// Single-producer (audio thread, from processBlock) / single-consumer
// (message thread, the editor's Timer) activity log. Lets the editor show
// "is MIDI reaching the plugin at all" and "did the NRPN decoder make sense
// of it" without touching the audio thread with locks or allocations.
// Precision doesn't matter here (it's a debug/diagnostic view only), so a
// plain atomic write-index with no consumer-side locking is good enough.
class MidiActivityLog
{
public:
    static constexpr int kCapacity = 64;

    void pushRaw(int channel, int cc, int value)
    {
        auto idx = writeIndex.load(std::memory_order_relaxed);
        auto& e = buffer[idx % kCapacity];
        e.channel = channel;
        e.cc = cc;
        e.value = value;
        e.isNrpnDecoded = false;
        e.paramIndex = -1;
        e.decodedValue = -1;
        writeIndex.store(idx + 1, std::memory_order_release);
        totalReceived.fetch_add(1, std::memory_order_relaxed);
    }

    // Annotate the entry just pushed (called right after pushRaw, same
    // thread, once the NRPN decoder resolves a complete value from it).
    void markLastDecoded(int paramIndex, int decodedValue)
    {
        auto idx = writeIndex.load(std::memory_order_relaxed);
        if (idx == 0)
            return;
        auto& e = buffer[(idx - 1) % kCapacity];
        e.isNrpnDecoded = true;
        e.paramIndex = paramIndex;
        e.decodedValue = decodedValue;
        totalDecoded.fetch_add(1, std::memory_order_relaxed);
    }

    // Copies up to maxCount most-recent entries into out[], oldest first.
    // Returns how many were copied.
    int readRecent(MidiLogEntry* out, int maxCount) const
    {
        const auto w = writeIndex.load(std::memory_order_acquire);
        const int avail = (int) juce::jmin((uint32_t) maxCount, (uint32_t) kCapacity, w);
        for (int i = 0; i < avail; ++i)
            out[i] = buffer[(w - (uint32_t) avail + (uint32_t) i) % kCapacity];
        return avail;
    }

    int getTotalReceived() const { return totalReceived.load(std::memory_order_relaxed); }
    int getTotalDecoded() const { return totalDecoded.load(std::memory_order_relaxed); }
    int getTotalSentOut() const { return totalSentOut.load(std::memory_order_relaxed); }
    void incrementSentOut() { totalSentOut.fetch_add(1, std::memory_order_relaxed); }

private:
    std::array<MidiLogEntry, kCapacity> buffer;
    std::atomic<uint32_t> writeIndex { 0 };
    std::atomic<int> totalReceived { 0 };
    std::atomic<int> totalDecoded { 0 };
    std::atomic<int> totalSentOut { 0 };
};

}  // namespace p12
