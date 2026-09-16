// Bitwise operators for scoped enums that hold flag sets.

#ifndef CNC_RED_ALERT_BASE_FLAGS_H_
#define CNC_RED_ALERT_BASE_FLAGS_H_

#include <type_traits>

namespace base {

// Opt an enum in by specializing this to true after its definition. The enum
// should also carry CNC_FLAG_ENUM (base/attributes.h), which tells the
// analyzer that combinations of its enumerators are valid values:
//
//   enum class CNC_FLAG_ENUM ShapeFlags : uint32_t { kNormal = 0, kGhost = 0x0400 };
//   template <>
//   inline constexpr bool base::kIsFlagEnum<ShapeFlags> = true;
//
// The operators below are then found for it, and `base::Any(flags & kGhost)`
// tests a bit. Nothing is defined for an enum that has not opted in.
template <class E>
inline constexpr bool kIsFlagEnum = false;

template <class E>
concept FlagEnum = std::is_enum_v<E> && kIsFlagEnum<E>;

// The underlying type may be signed; the bit operations run on its unsigned
// counterpart.
template <class E>
constexpr auto Bits(E e) noexcept {
  return static_cast<std::make_unsigned_t<std::underlying_type_t<E>>>(e);
}

}  // namespace base

// The operators live at global scope because the enums do; the constraint
// keeps them off every other type.
template <base::FlagEnum E>
constexpr E operator|(E a, E b) noexcept {
  return static_cast<E>(base::Bits(a) | base::Bits(b));
}

template <base::FlagEnum E>
constexpr E operator&(E a, E b) noexcept {
  return static_cast<E>(base::Bits(a) & base::Bits(b));
}

template <base::FlagEnum E>
constexpr E operator^(E a, E b) noexcept {
  return static_cast<E>(base::Bits(a) ^ base::Bits(b));
}

template <base::FlagEnum E>
constexpr E operator~(E a) noexcept {
  return static_cast<E>(~base::Bits(a));
}

template <base::FlagEnum E>
constexpr E& operator|=(E& a, E b) noexcept {
  return a = a | b;
}

template <base::FlagEnum E>
constexpr E& operator&=(E& a, E b) noexcept {
  return a = a & b;
}

#endif  // CNC_RED_ALERT_BASE_FLAGS_H_
