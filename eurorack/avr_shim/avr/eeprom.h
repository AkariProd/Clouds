// Stand-in for AVR's <avr/eeprom.h> on a desktop build.
//
// The firmware persists a handful of option bits to the module's EEPROM
// across power cycles. This port persists state via JUCE's own
// AudioProcessorValueTreeState instead, so these are harmless no-ops
// (LoadSettings() is simply never called -- see GridsEngine).
#pragma once

#include <cstdint>

inline uint8_t eeprom_read_byte(const uint8_t*) { return 0; }
inline void eeprom_write_byte(uint8_t*, uint8_t) {}
