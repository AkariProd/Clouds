// Minimal portable stand-in for avrlib/op.h.
//
// The real file's "optimized" path (unconditionally selected by its own
// USE_OPTIMIZED_OP #define, not guarded by __AVR__) is hand-written AVR
// inline assembly and won't compile on any other target. Rather than
// patch around that, this reproduces just the handful of fixed-point
// helpers pattern_generator.cc actually calls, using the same semantics
// as the original's portable (non-AVR) C++ fallback branch.
#ifndef AVRLIB_OP_H_
#define AVRLIB_OP_H_

#include "avrlib/base.h"

namespace avrlib {

static inline uint8_t U8Mix(uint8_t a, uint8_t b, uint8_t balance) {
  return (a * (255 - balance) + b * balance) >> 8;
}

static inline uint8_t U8U8MulShift8(uint8_t a, uint8_t b) {
  return (uint16_t) a * b >> 8;
}

static inline uint16_t U8U8Mul(uint8_t a, uint8_t b) {
  return (uint16_t) a * b;
}

}  // namespace avrlib

#endif  // AVRLIB_OP_H_
