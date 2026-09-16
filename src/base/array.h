// Bounds-checked access to fixed arrays without changing their storage layout.

#ifndef CNC_RED_ALERT_BASE_ARRAY_H_
#define CNC_RED_ALERT_BASE_ARRAY_H_

#include <concepts>
#include <cstddef>
#include <span>
#include <utility>

#include "absl/base/attributes.h"
#include "absl/log/check.h"

namespace base {

// Returns an element of a fixed array. The extent is deduced from the array,
// never supplied by the caller. Invalid signed or unsigned indices fail in
// all build modes; valid access is also usable in constant expressions.
//
// Example:
//   int samples[8] = {};
//   base::At(samples, index) = 42;
template <class T, std::size_t N>
// Clang 23 cannot trace the reference through libstdc++ span::operator[].
// The span is constructed directly from array and never escapes.
// NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-lifetimebound-violation)
constexpr T& At(T (&array ABSL_ATTRIBUTE_LIFETIME_BOUND)[N],
                std::integral auto index) {
  CHECK(std::cmp_greater_equal(+index, 0) && std::cmp_less(+index, N));
  return std::span(array)[static_cast<std::size_t>(index)];
}

// Returns the tail starting at index. Unlike At, this accepts the array's
// size to represent an empty tail and its valid one-past pointer.
template <class T, std::size_t N>
constexpr std::span<T> Suffix(T (&array)[N], std::integral auto index) {
  CHECK(std::cmp_greater_equal(+index, 0) && std::cmp_less_equal(+index, N));
  return std::span(array).subspan(static_cast<std::size_t>(index));
}

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_ARRAY_H_
