// Minimal stand-in for the AVR firmware's hardware_config.h.
//
// The original declares ADC/GPIO/LED pin mappings for the physical module
// (avrlib/gpio.h, parallel_io.h, serial.h, spi.h -- all AVR register
// definitions that don't exist off-AVR). pattern_generator.h only actually
// needs the LedBits enum from it (used by PatternGenerator::led_pattern(),
// which this port doesn't call, but the method must still compile), so
// only that is reproduced here.

#ifndef GRIDS_HARDWARE_CONFIG_H_
#define GRIDS_HARDWARE_CONFIG_H_

#include "avrlib/base.h"

namespace grids {

enum LedBits {
  LED_CLOCK = 1,
  LED_BD = 8,
  LED_SD = 4,
  LED_HH = 2,
  LED_ALL = LED_CLOCK | LED_BD | LED_SD | LED_HH
};

}  // namespace grids

#endif  // GRIDS_HARDWARE_CONFIG_H_
