#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"

namespace p12 {

// Decodes the Prophet 12's transmitted NRPN stream (running-status CC99/98
// for the parameter number, CC6/CC38 for the value) and reports finished
// (paramIndex, raw value) pairs. See the Prophet 12 Module Operation
// Manual, "MIDI Implementation -> NRPN Messages".
//
// The hardware sends a CC6+CC38 pair for EVERY parameter, including narrow
// (<=127) ones -- CC38 there is padding, not meaningful low-order data. So
// which byte(s) actually carry the real value depends on the target
// parameter's own range (def.maxValue), not on which CCs merely showed up:
// treating CC38 as always-significant made every narrow-range knob snap to
// its min/max, because (dataMSB << 7 | dataLSB) is wildly out of range for
// e.g. a 0-120 or 0-19 parameter and gets clamped straight to an extreme.
class NrpnDecoder
{
public:
    bool handleControllerChange(int controllerNumber, int value, int& outParamIndex, int& outRawValue)
    {
        switch (controllerNumber)
        {
            case 99: nrpnMSB = value; haveMSB = true; resolveParam(); return false;
            case 98: nrpnLSB = value; haveLSB = true; resolveParam(); return false;
            case 6:
                dataMSB = value;
                if (!haveParam)
                    return false;
                if (isWide)
                    return false;  // wait for CC38 to complete the 14-bit value
                outParamIndex = paramIndex;
                outRawValue = dataMSB;
                return true;
            case 38:
                dataLSB = value;
                if (!haveParam || !isWide)
                    return false;  // narrow params don't use the LSB -- CC38 here is
                                    // just padding; ignore it rather than clobber the
                                    // already-complete CC6 value with a bogus combined one
                outParamIndex = paramIndex;
                outRawValue = (dataMSB << 7) | dataLSB;
                return true;
            default:
                return false;
        }
    }

private:
    void resolveParam()
    {
        if (!haveMSB || !haveLSB) { haveParam = false; return; }
        const int nrpn = (nrpnMSB << 7) | nrpnLSB;
        const auto& table = nrpnToParamIndex();
        auto it = table.find(nrpn);
        if (it == table.end()) { haveParam = false; return; }
        paramIndex = it->second;
        isWide = kParams[paramIndex].maxValue > 127;
        haveParam = true;
    }

    int nrpnMSB = 0, nrpnLSB = 0, dataMSB = 0, dataLSB = 0;
    bool haveMSB = false, haveLSB = false;
    bool haveParam = false;
    bool isWide = false;
    int paramIndex = -1;
};

}  // namespace p12
