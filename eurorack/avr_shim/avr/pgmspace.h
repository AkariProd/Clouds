// Stand-in for AVR's <avr/pgmspace.h> on a desktop build.
//
// On AVR, "program memory" (flash) is a separate address space from RAM,
// so constant tables need PROGMEM placement + pgm_read_* accessors to read
// them correctly. On a desktop CPU there's only one address space, so
// every one of these is a plain no-op / direct read.
#pragma once

#include <cstring>
#include <cstdint>

#define PROGMEM

typedef uint8_t  prog_uint8_t;
typedef uint16_t prog_uint16_t;
typedef uint32_t prog_uint32_t;
typedef char     prog_char;

#define pgm_read_byte(p)  (*reinterpret_cast<const uint8_t*>(p))
#define pgm_read_word(p)  (*reinterpret_cast<const uint16_t*>(p))
#define pgm_read_dword(p) (*reinterpret_cast<const uint32_t*>(p))

inline char* strncpy_P(char* dest, const char* src, size_t n) { return strncpy(dest, src, n); }
inline void* memcpy_P(void* dest, const void* src, size_t n) { return memcpy(dest, src, n); }
