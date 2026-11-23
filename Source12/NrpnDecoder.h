#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"

namespace p12 {

// Decodes the Prophet 12's transmitted NRPN stream (CC99/98 for the
// parameter number, CC6/CC38 for the value) and reports finished
// (paramIndex, raw value) pairs. See the Prophet 12 Module Operation
// Manual, "MIDI Implementation -> NRPN Messages".
//
// The hardware always sends the full 4-message block (CC99, CC98, CC6,
// CC38) for every single value change, with the real value as a standard
// 14-bit (dataMSB << 7 | dataLSB) number -- CC6 (the MSB) reads as a
// constant 0 for any parameter whose range fits under 128, which is by
// far most of them, NOT because CC38 is unused padding. Only use CC6
// alone as a shortcut once a complete (MSB, LSB) pair has actually been
// seen for this parameter; always wait for both before completing.
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
                return false;  // always wait for CC38 -- see class comment
            case 38:
                dataLSB = value;
                if (!haveParam)
                    return false;
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
        haveParam = true;
    }

    int nrpnMSB = 0, nrpnLSB = 0, dataMSB = 0, dataLSB = 0;
    bool haveMSB = false, haveLSB = false;
    bool haveParam = false;
    int paramIndex = -1;
};

}  // namespace p12
