// Iteration over the game's index enums without X_FIRST/X_COUNT sentinels.
//
// An index enum's real values run 0..N-1 and index arrays of size N; a
// negative "none" value may precede them. EnumValues<E>() yields the real
// values in order, for range-for:
//
//   for (HousesType house : EnumValues<HousesType>()) { ... }
//
// The reflection behind it costs compile time in every translation unit that
// instantiates it, so headers size their arrays with the kXxxCount constants
// declared next to each enum in defines.h; enum_util_test.cc checks that
// those constants agree with kEnumCount<E>.

#ifndef CNC_RED_ALERT_RA_ENUM_UTIL_H_
#define CNC_RED_ALERT_RA_ENUM_UTIL_H_

#include <array>

#include "magic_enum/magic_enum.hpp"
#include "ra/defines.h"

// Two enums outgrow magic_enum's default [-128, 128] window. TEMPLATE_NONE
// (65535) stays outside on purpose; it is not an index.
template <>
struct magic_enum::customize::enum_range<VocType> {
  static constexpr int min = -1;
  static constexpr int max = 256;
};
template <>
struct magic_enum::customize::enum_range<TemplateType> {
  static constexpr int min = 0;
  static constexpr int max = 512;
};

// Number of index values of E, i.e. its non-negative enumerators.
template <typename E>
inline constexpr int kEnumCount = [] {
  int count = 0;
  for (E value : magic_enum::enum_values<E>()) {
    if (static_cast<int>(value) >= 0) {
      ++count;
    }
  }
  return count;
}();

// True if E's non-negative enumerators are exactly 0..kEnumCount<E> - 1.
template <typename E>
constexpr bool IsIndexEnum() {
  int expected = 0;
  for (E value : magic_enum::enum_values<E>()) {
    if (static_cast<int>(value) < 0) {
      continue;
    }
    if (static_cast<int>(value) != expected) {
      return false;
    }
    ++expected;
  }
  return expected == kEnumCount<E>;
}

// The index values of E in ascending order.
template <typename E>
constexpr std::array<E, kEnumCount<E>> EnumValues() {
  static_assert(IsIndexEnum<E>(), "enum has gaps or aliases below zero");
  std::array<E, kEnumCount<E>> values{};
  int n = 0;
  for (E value : magic_enum::enum_values<E>()) {
    if (static_cast<int>(value) >= 0) {
      values[n++] = value;
    }
  }
  return values;
}

#endif  // CNC_RED_ALERT_RA_ENUM_UTIL_H_
