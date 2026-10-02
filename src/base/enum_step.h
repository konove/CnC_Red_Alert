// Increment and decrement for enums whose values are walked in order.

#ifndef CNC_RED_ALERT_BASE_ENUM_STEP_H_
#define CNC_RED_ALERT_BASE_ENUM_STEP_H_

#include <type_traits>
#include <utility>

namespace base {

// Opt an enum in by specializing this to true after its definition:
//
//   enum class Theater { kTemperate, kSnow, kInterior };
//   template <> inline constexpr bool base::kIsSteppableEnum<Theater> = true;
//
// `++` and `--` then move to the next or previous underlying value. That value
// need not be a named enumerator, so the caller bounds the walk. Nothing is
// defined for an enum that has not opted in. To visit every enumerator of an
// index enum, prefer `for (E e : magic_enum::enum_values<E>())`.
template <class E>
inline constexpr bool kIsSteppableEnum = false;

template <class E>
concept SteppableEnum = std::is_enum_v<E> && kIsSteppableEnum<E>;

}  // namespace base

// The operators live at global scope, like base/flags.h's; the constraint keeps
// them off every other type.
template <base::SteppableEnum E>
constexpr E& operator++(E& e) noexcept {
  return e = static_cast<E>(std::to_underlying(e) + 1);
}

template <base::SteppableEnum E>
constexpr E operator++(E& e, int) noexcept {
  const E old = e;
  ++e;
  return old;
}

template <base::SteppableEnum E>
constexpr E& operator--(E& e) noexcept {
  return e = static_cast<E>(std::to_underlying(e) - 1);
}

template <base::SteppableEnum E>
constexpr E operator--(E& e, int) noexcept {
  const E old = e;
  --e;
  return old;
}

#endif  // CNC_RED_ALERT_BASE_ENUM_STEP_H_
