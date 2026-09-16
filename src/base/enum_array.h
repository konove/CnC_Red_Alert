// EnumArray: a fixed-size table with one element per value of an index enum.

#ifndef CNC_RED_ALERT_BASE_ENUM_ARRAY_H_
#define CNC_RED_ALERT_BASE_ENUM_ARRAY_H_

#include <iterator>
#include <type_traits>

#include "absl/base/attributes.h"
#include "absl/log/check.h"
#include "base/array.h"
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

  constexpr T& operator[](E index) noexcept ABSL_ATTRIBUTE_LIFETIME_BOUND {
    DCHECK(static_cast<ssize>(index) >= 0 && static_cast<ssize>(index) < N);
    return base::At(elements, static_cast<ssize>(index));
  }
  constexpr const T& operator[](E index) const noexcept
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    DCHECK(static_cast<ssize>(index) >= 0 && static_cast<ssize>(index) < N);
    return base::At(elements, static_cast<ssize>(index));
  }

  [[nodiscard]] static constexpr ssize size() noexcept { return N; }

  // Serializes the elements in order, exactly as the C array did.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(elements);
  }

  [[nodiscard]] constexpr T* data() noexcept ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return elements;
  }
  [[nodiscard]] constexpr const T* data() const noexcept
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return elements;
  }
  [[nodiscard]] constexpr T* begin() noexcept ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return elements;
  }
  [[nodiscard]] constexpr const T* begin() const noexcept
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return elements;
  }
  // std::end returns the one-past pointer into elements. Clang 23 cannot
  // infer this through the library template.
  // NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-lifetimebound-violation)
  [[nodiscard]] constexpr T* end() noexcept ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return std::end(elements);
  }
  [[nodiscard]] constexpr const T* end() const noexcept
      // std::end has the same owner as elements.
      // NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-lifetimebound-violation)
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return std::end(elements);
  }
};

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_ENUM_ARRAY_H_
