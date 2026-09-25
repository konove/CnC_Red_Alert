// Bounds-checked access to fixed arrays and spans without changing storage.

#ifndef CNC_RED_ALERT_BASE_ARRAY_H_
#define CNC_RED_ALERT_BASE_ARRAY_H_

#include <concepts>
#include <cstddef>
#include <span>
#include <utility>

#include "absl/base/attributes.h"
#include "absl/log/check.h"

namespace base {

// Returns an element of a bounded view. The reference belongs to the viewed
// storage, which must outlive its use; the span itself may be temporary.
// Negative and past-the-end indices fail in every build mode.
template <class T, std::size_t N>
constexpr T& At(std::span<T, N> view, std::integral auto index) {
  CHECK(std::cmp_greater_equal(+index, 0) &&
        std::cmp_less(+index, view.size()));
  return view.subspan(static_cast<std::size_t>(index), 1).front();
}

// Returns an element of a fixed array. The extent is deduced from the array,
// never supplied by the caller. Invalid signed or unsigned indices fail in
// all build modes; valid access is also usable in constant expressions.
//
// Example:
//   int samples[8] = {};
//   base::At(samples, index) = 42;
template <class T, std::size_t N>
// Clang 23 cannot trace the reference through libstdc++ span operations.
// The span is constructed directly from array and never escapes.
// NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-lifetimebound-violation)
constexpr T& At(T (&array ABSL_ATTRIBUTE_LIFETIME_BOUND)[N],
                std::integral auto index) {
  return base::At(std::span(array), index);
}

// Returns the tail starting at index. Unlike At, this accepts the array's
// size to represent an empty tail and its valid one-past pointer.
template <class T, std::size_t N>
constexpr std::span<T> Suffix(T (&array)[N], std::integral auto index) {
  CHECK(std::cmp_greater_equal(+index, 0) && std::cmp_less_equal(+index, N));
  return std::span(array).subspan(static_cast<std::size_t>(index));
}

// Consumes one element of an existing bounded view. The referenced storage
// remains owned by the original array/container; an empty view is rejected.
template <class T>
// Clang cannot follow the element reference through libstdc++ span::front.
constexpr T& ConsumeFront(
    // NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-lifetimebound-violation)
    std::span<T>& remaining ABSL_ATTRIBUTE_LIFETIME_BOUND) {
  CHECK(!remaining.empty());
  T& value = remaining.front();
  remaining = remaining.subspan(1);
  return value;
}

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_ARRAY_H_
