// Checked conversions between signed and unsigned integer types.
//
// The project counts, sizes and indexes with signed types (see
// docs/TYPE_MIGRATION.md), while the C library and the standard containers
// take and return size_t. These helpers mark those boundaries and verify in
// debug builds that the value survives the change of signedness.
#ifndef CNC_RED_ALERT_BASE_NUMERIC_H_
#define CNC_RED_ALERT_BASE_NUMERIC_H_

#include <concepts>
#include <cstddef>
#include <limits>
#include <utility>

#include "absl/log/check.h"
#include "base/types.h"

namespace base {

// Converts a non-negative count, size or index to std::size_t for a library
// parameter such as memcpy's length or std::vector::resize.
//
// The value must be representable as std::size_t, i.e. not negative; debug
// builds check this.
//
// Example:
//   std::memcpy(dest, src, base::ToSize(length));
constexpr std::size_t ToSize(std::integral auto value) {
  DCHECK(std::in_range<std::size_t>(value));
  return static_cast<std::size_t>(value);
}

// Converts a size or count returned by a library call to base::ssize. Prefer
// std::ssize() where the source is a container.
//
// The value must fit in base::ssize; debug builds check this.
//
// Example:
//   base::ssize length = base::ToSigned(std::strlen(text));
constexpr ssize ToSigned(std::integral auto value) {
  DCHECK(std::in_range<ssize>(value));
  return static_cast<ssize>(value);
}

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

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_NUMERIC_H_
