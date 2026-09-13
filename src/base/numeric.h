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

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_NUMERIC_H_
