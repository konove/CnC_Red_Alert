// File: Fixed-point fractions of a whole, as the games scale health bars,
// sound volumes, speeds and costs: 0x100 is one.

#ifndef CNC_RED_ALERT_BASE_CARDINAL_H_
#define CNC_RED_ALERT_BASE_CARDINAL_H_

#include <cstdint>

namespace base {

// Returns "cardinal" as a fixed-point fraction of "base", where 0x100 is one;
// 0xFFFF if "base" is zero.
constexpr int CardinalToFixed(const int base, const int cardinal) {
  if (base == 0) {
    return 0xFFFF;
  }
  // Unsigned 32-bit arithmetic keeps the original results, including the
  // wraparound for inputs outside the game's range; the simulation is
  // deterministic across peers only if these stay bit-identical.
  return static_cast<int>((static_cast<uint32_t>(cardinal) << 8) /
                          static_cast<uint32_t>(base));
}

// Returns the rounded "fixed" fraction of "base", where "fixed" is 0x100 for
// one; 0xFFFF if the product does not fit in 24 bits.
constexpr int FixedToCardinal(const int base, const int fixed) {
  // Unsigned 32-bit arithmetic for the same reason as CardinalToFixed.
  const uint32_t ret =
      (static_cast<uint32_t>(base) * static_cast<uint32_t>(fixed)) + 0x80;
  if (ret > 0x00FFFFFF) {
    return 0xFFFF;
  }
  return static_cast<int>(ret >> 8);
}

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_CARDINAL_H_
