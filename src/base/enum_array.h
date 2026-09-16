// EnumArray: a fixed-size table with one element per value of an index enum.

#ifndef CNC_RED_ALERT_BASE_ENUM_ARRAY_H_
#define CNC_RED_ALERT_BASE_ENUM_ARRAY_H_

#include <type_traits>

#include "absl/log/check.h"
#include "base/types.h"
#include "magic_enum/magic_enum.hpp"

namespace base {

// A `T` for every value of the index enum `E`, in enumerator order, subscripted
// by the enum itself. It is an aggregate, so it brace-initializes like the C
// array it replaces and can be `constexpr`; a numeric subscript is a compile
// error rather than a silent lookup in the wrong table. `N` defaults to the
// number of enumerators and only needs stating for a table that is
// deliberately shorter or longer than the enum.
//
// Example:
//   base::EnumArray<ArmorType, const char*> ArmorName = {"none", "wood"};
//   const char* name = ArmorName[ARMOR_WOOD];
//   for (const char* n : ArmorName) ...
template <class E, class T, ssize N = ssize{magic_enum::enum_count<E>()}>
struct EnumArray {
  static_assert(std::is_enum_v<E>, "EnumArray is indexed by an enum");
  static_assert(N > 0, "EnumArray needs at least one element");

  using value_type = T;

  T elements[N];

  constexpr T& operator[](E index) noexcept {
    DCHECK(static_cast<ssize>(index) >= 0 && static_cast<ssize>(index) < N);
    return elements[static_cast<ssize>(index)];
  }
  constexpr const T& operator[](E index) const noexcept {
    DCHECK(static_cast<ssize>(index) >= 0 && static_cast<ssize>(index) < N);
    return elements[static_cast<ssize>(index)];
  }

  static constexpr ssize size() noexcept { return N; }

  constexpr T* data() noexcept { return elements; }
  constexpr const T* data() const noexcept { return elements; }
  constexpr T* begin() noexcept { return elements; }
  constexpr const T* begin() const noexcept { return elements; }
  constexpr T* end() noexcept { return elements + N; }
  constexpr const T* end() const noexcept { return elements + N; }
};

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_ENUM_ARRAY_H_
