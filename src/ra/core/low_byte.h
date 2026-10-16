// The low-byte rule of the trigger arguments. The original game kept the
// one-byte values a trigger event or action takes (a house, a special weapon,
// a movie, a speech, a flag) in a union with the int the scenario stores, so
// only the int's low byte, read as signed, was the value. The shipped
// scenarios rely on it: SCG12EA's nuke is special weapon -255 and SCG10EA's
// "lch3" plays movie -170.

#ifndef CNC_RED_ALERT_RA_CORE_LOW_BYTE_H_
#define CNC_RED_ALERT_RA_CORE_LOW_BYTE_H_

#include <cstdint>

namespace ra {

// Returns the low byte of `value` as a signed number, -128 to 127.
constexpr int LowByte(const int value) { return static_cast<int8_t>(value); }

// Returns the one-byte enum `value` means, which is its low byte.
template <class Enum>
constexpr Enum LowByteAs(const int value) {
  return static_cast<Enum>(LowByte(value));
}

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_CORE_LOW_BYTE_H_
