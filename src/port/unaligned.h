// Copy trivially copyable values to and from byte buffers without alignment
// requirements.
#ifndef CNC_RED_ALERT_PORT_UNALIGNED_H_
#define CNC_RED_ALERT_PORT_UNALIGNED_H_

#include <cstddef>
#include <span>

#include "base/buffer.h"
#include <type_traits>

namespace port {

// Reads a native-representation value from at least sizeof(T) readable bytes.
// The source must be non-null and contain a valid representation of T.
template <typename T>
  requires std::is_trivially_copyable_v<T>
T ReadUnaligned(std::span<const std::byte> source) {
  T value{};
  base::CopyBytes(base::ObjectBytes(value), source, sizeof(value));
  return value;
}

// Writes a native-representation value to at least sizeof(T) writable bytes.
// The destination must be non-null. Byte order is unchanged.
template <typename T>
  requires std::is_trivially_copyable_v<T>
void WriteUnaligned(std::span<std::byte> destination, const T& value) {
  base::CopyBytes(destination, base::ObjectBytes(value), sizeof(value));
}

}  // namespace port

#endif  // CNC_RED_ALERT_PORT_UNALIGNED_H_
