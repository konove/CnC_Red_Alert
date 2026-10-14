// Typed storage for a tagged union whose bytes are part of a wire or file
// format.

#ifndef CNC_RED_ALERT_BASE_PAYLOAD_H_
#define CNC_RED_ALERT_BASE_PAYLOAD_H_

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <span>
#include <type_traits>
#include <utility>

#include "absl/base/attributes.h"
#include "base/unaligned.h"

namespace base {

// The bytes of one of several trivially copyable "arms", laid out as a union
// of them: the same size and alignment, every arm at offset 0. An arm is never
// accessed in place. Get() copies it out and Update() copies it back, so
// reading bytes another arm wrote reinterprets them, with no undefined
// behavior. The selector (an event type, a packet command) lives outside, as
// it did beside the union.
//
// Writing an arm changes only its own sizeof(T) bytes. The rest keep what they
// held, as with a union member store. Update() takes the arm's padding from the
// storage and writes it back, so storage that starts zeroed stays
// deterministic byte for byte, which matters to data compared or sent as raw
// bytes.
//
// Example:
//   struct Move { int32_t who; int32_t where; };
//   struct Speed { int16_t value; };
//   base::Payload<Move, Speed> data;
//   data.Update<Move>([](Move& m) { m.who = 3; m.where = 9; });
//   const int32_t where = data.Get<Move>().where;
template <class... Arms>
  requires(sizeof...(Arms) > 0 && (std::is_trivially_copyable_v<Arms> && ...))
class Payload {
 public:
  // True for the types this payload can hold.
  template <class T>
  static constexpr bool kHolds = (std::is_same_v<T, Arms> || ...);

  // Returns a copy of arm T as the storage holds it.
  template <class T>
    requires kHolds<T>
  [[nodiscard]] T Get() const {
    return ReadUnaligned<T>(bytes_);
  }

  // Copies arm T out, lets `edit` change it, and copies it back.
  template <class T, std::invocable<T&> Edit>
    requires kHolds<T>
  void Update(Edit&& edit) {
    T arm = Get<T>();
    std::invoke(std::forward<Edit>(edit), arm);
    WriteUnaligned(std::span(bytes_), arm);
  }

  // Stores `arm`, padding bytes included; the bytes past sizeof(T) are
  // unchanged. Only for a value read with Get(): a freshly built T has
  // indeterminate padding. Use Update() otherwise.
  template <class T>
    requires kHolds<T>
  void Set(const T& arm) {
    WriteUnaligned(std::span(bytes_), arm);
  }

  [[nodiscard]] std::span<const std::byte> bytes() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return bytes_;
  }
  [[nodiscard]] std::span<std::byte> bytes() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return bytes_;
  }

 private:
  alignas(Arms...) std::array<std::byte, std::max({sizeof(Arms)...})> bytes_{};
};

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_PAYLOAD_H_
