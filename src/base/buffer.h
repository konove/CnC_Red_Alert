// Checked operations on byte spans and object representations.

#ifndef CNC_RED_ALERT_BASE_BUFFER_H_
#define CNC_RED_ALERT_BASE_BUFFER_H_

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <span>
#include <type_traits>
#include <utility>

#include "absl/log/check.h"

namespace base {

// Returns the bytes of an existing trivially copyable object, including arrays
// and padding. An lvalue is required so a temporary cannot escape as a view.
// Const objects produce a read-only view. The bound comes solely from T.
// Pointers are rejected: viewing a pointer variable is not viewing its buffer.
template <class T>
  requires(std::is_trivially_copyable_v<T> && !std::is_volatile_v<T> &&
           !std::is_pointer_v<T>)
auto ObjectBytes(T& object) {
  // A reference denotes exactly one live T. This fixed bound cannot disagree
  // with the allocation; no pointer/count pair is accepted from the caller.
  // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
  const std::span<T, 1> objects(std::addressof(object), 1);
  if constexpr (std::is_const_v<T>) {
    return std::as_bytes(objects);
  } else {
    return std::as_writable_bytes(objects);
  }
}

// Copies count bytes, checking both spans before access. The ranges must not
// overlap unless their starts are identical; use MoveBytes for overlap.
inline void CopyBytes(std::span<std::byte> dest,
                      std::span<const std::byte> source,
                      std::integral auto count) {
  CHECK(std::cmp_greater_equal(+count, 0));
  CHECK(std::cmp_less_equal(+count, dest.size()));
  CHECK(std::cmp_less_equal(+count, source.size()));
  if (dest.data() != source.data()) {
    std::ranges::copy(source.first(static_cast<std::size_t>(count)),
                      dest.begin());
  }
}

// Copies count bytes safely in either overlapping direction.
inline void MoveBytes(std::span<std::byte> dest,
                      std::span<const std::byte> source,
                      std::integral auto count) {
  CHECK(std::cmp_greater_equal(+count, 0));
  CHECK(std::cmp_less_equal(+count, dest.size()));
  CHECK(std::cmp_less_equal(+count, source.size()));
  const auto input = source.first(static_cast<std::size_t>(count));
  const auto output = dest.first(static_cast<std::size_t>(count));
  if (std::less<const std::byte*>{}(source.data(), dest.data())) {
    std::ranges::copy_backward(input, output.end());
  } else if (source.data() != dest.data()) {
    std::ranges::copy(input, output.begin());
  }
}

// Fills count bytes, with the same low-byte conversion as memset.
inline void FillBytes(std::span<std::byte> dest, int value,
                      std::integral auto count) {
  CHECK(std::cmp_greater_equal(+count, 0));
  CHECK(std::cmp_less_equal(+count, dest.size()));
  std::ranges::fill(dest.first(static_cast<std::size_t>(count)),
                    static_cast<std::byte>(static_cast<unsigned char>(value)));
}

// Compares count bytes as unsigned values, returning their first difference.
inline int CompareBytes(std::span<const std::byte> left,
                        std::span<const std::byte> right,
                        std::integral auto count) {
  CHECK(std::cmp_greater_equal(+count, 0));
  CHECK(std::cmp_less_equal(+count, left.size()));
  CHECK(std::cmp_less_equal(+count, right.size()));
  for (std::size_t i = 0; i < static_cast<std::size_t>(count); ++i) {
    const int difference =
        std::to_integer<int>(left[i]) - std::to_integer<int>(right[i]);
    if (difference != 0) {
      return difference;
    }
  }
  return 0;
}

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_BUFFER_H_
