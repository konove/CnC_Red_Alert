// Copy trivially copyable values to and from byte buffers without alignment
// requirements, read the little-endian integers of the game's file formats and
// append the big-endian integers of the network and movie formats.
#ifndef CNC_RED_ALERT_BASE_UNALIGNED_H_
#define CNC_RED_ALERT_BASE_UNALIGNED_H_

#include <bit>
#include <concepts>
#include <cstddef>
#include <span>
#include <type_traits>
#include <vector>

#include "base/buffer.h"

namespace base {

// Reads a native-representation value from at least sizeof(T) readable bytes.
// The source must be non-null and contain a valid representation of T.
template <typename T>
  requires std::is_trivially_copyable_v<T>
T ReadUnaligned(std::span<const std::byte> source) {
  T value{};
  base::CopyBytes(base::ObjectBytes(value), source, sizeof(value));
  return value;
}

// Reads the little-endian integer stored at byte offset in data, or returns 0
// if any of its bytes lies outside data (see SliceOrEmpty). Unlike
// ReadUnaligned, the result does not depend on the host's byte order.
//
// Example:
//   const auto frame_count = base::ReadLittleEndian<uint16_t>(shape_file, 0);
template <std::integral T>
  requires(!std::same_as<T, bool>)
T ReadLittleEndian(const std::span<const std::byte> data,
                   const std::integral auto offset) {
  const auto bytes = SliceOrEmpty(data, offset, sizeof(T));
  if (bytes.empty()) {
    return 0;
  }
  const auto value = ReadUnaligned<T>(bytes);
  if constexpr (std::endian::native == std::endian::big) {
    return std::byteswap(value);
  }
  return value;
}

// Writes a native-representation value to at least sizeof(T) writable bytes.
// The destination must be non-null. Byte order is unchanged.
template <typename T>
  requires std::is_trivially_copyable_v<T>
void WriteUnaligned(std::span<std::byte> destination, const T& value) {
  base::CopyBytes(destination, base::ObjectBytes(value), sizeof(value));
}

// Appends `value` to `out` as sizeof(T) bytes, most significant first. Like
// ReadLittleEndian, the result does not depend on the host's byte order. `out`
// holds bytes of any one-byte type (std::byte, uint8_t).
//
// Example:
//   base::AppendBigEndian(bytes, int32_t{258});  // 00 00 01 02
template <std::integral T, typename Byte>
  requires(!std::same_as<T, bool> && sizeof(Byte) == 1)
// LLVM 23 mistakes element invalidation for invalidating the vector reference;
// no element reference or iterator is retained across these appends.
// NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-invalidation)
void AppendBigEndian(std::vector<Byte>& out, const T value) {
  auto bits = static_cast<std::make_unsigned_t<T>>(value);
  if constexpr (std::endian::native == std::endian::little) {
    bits = std::byteswap(bits);
  }
  for (const std::byte byte : base::ObjectBytes(bits)) {
    out.push_back(static_cast<Byte>(byte));
  }
}

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_UNALIGNED_H_
