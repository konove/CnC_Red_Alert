// A single-bit mask from a bit index.
#ifndef CNC_RED_ALERT_BASE_BIT_H_
#define CNC_RED_ALERT_BASE_BIT_H_

#include <concepts>
#include <limits>
#include <type_traits>

#include "absl/log/check.h"

namespace base {

// Returns the value of unsigned type T with only bit `index` set: the flag
// for an index enum value (a house, a building type) or a loop counter. This
// is the one place a signed index becomes a shift count.
//
// The index must be non-negative and below the width of T; debug builds check
// this.
//
// Example:
//   allies |= base::Bit<uint32_t>(house);
//   if ((BScan & base::Bit<uint64_t>(STRUCT_WEAP)) != 0) ...
template <std::unsigned_integral T>
constexpr T Bit(int index) noexcept {
  DCHECK(index >= 0 && index < std::numeric_limits<T>::digits);
  return static_cast<T>(T{1} << static_cast<unsigned>(index));
}

// The same for an index enum: `base::Bit<uint32_t>(HOUSE_SPAIN)`.
template <std::unsigned_integral T, class E>
  requires std::is_enum_v<E>
constexpr T Bit(E index) noexcept {
  return Bit<T>(static_cast<int>(index));
}

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_BIT_H_
