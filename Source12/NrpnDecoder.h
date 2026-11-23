#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"

namespace p12 {

// Decodes the Prophet 12's transmitted NRPN stream (running-status CC99/98
// for the parameter number, CC6/CC38 for the value) and reports finished
// (paramIndex, 0..1 normalised value) pairs. See the Prophet 12 Module
// Operation Manual, "MIDI Implementation -> NRPN Messages".
class NrpnDecoder
{
public:
    // Call once per incoming MIDI CC message. Returns true and fills
    // outParamIndex/outRawValue when a full NRPN value was just completed.
    bool handleControllerChange(int controllerNumber, int value, int& outParamIndex, int& outRawValue)
    {
        switch (controllerNumber)
        {
            case 99: nrpnMSB = value; haveMSB = true; return false;           // NRPN param number MSB
            case 98: nrpnLSB = value; haveLSB = true; return false;           // NRPN param number LSB
            case 6:  dataMSB = value; return tryComplete(outParamIndex, outRawValue, true);
            case 38: dataLSB = value; return tryComplete(outParamIndex, outRawValue, false);
            default: return false;
        }
    }

private:
    bool tryComplete(int& outParamIndex, int& outRawValue, bool fromMSB)
    {
        if (!haveMSB || !haveLSB)
            return false;

        const int nrpn = (nrpnMSB << 7) | nrpnLSB;
        const auto& table = nrpnToParamIndex();
        auto it = table.find(nrpn);
        if (it == table.end())
            return false;

        // The Prophet 12 sends CC6 (and, for wide-range params, CC38) after
        // selecting the NRPN number. Treat CC6 alone as a complete 7-bit
        // value; if CC38 follows, combine into a 14-bit value.
        if (fromMSB)
        {
            outParamIndex = it->second;
            outRawValue = dataMSB;
            return true;
        }
        outParamIndex = it->second;
        outRawValue = (dataMSB << 7) | dataLSB;
        return true;
    }

    int nrpnMSB = 0, nrpnLSB = 0, dataMSB = 0, dataLSB = 0;
    bool haveMSB = false, haveLSB = false;
};

}  // namespace p12
