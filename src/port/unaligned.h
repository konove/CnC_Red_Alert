// Copy trivially copyable values to and from byte buffers without alignment
// requirements.
#ifndef CNC_RED_ALERT_PORT_UNALIGNED_H_
#define CNC_RED_ALERT_PORT_UNALIGNED_H_

#include <cstring>
#include <type_traits>

namespace port {

// Reads a native-representation value from at least sizeof(T) readable bytes.
// The source must be non-null and contain a valid representation of T.
template <typename T>
  requires std::is_trivially_copyable_v<T>
T ReadUnaligned(const void* source) {
  T value{};
  std::memcpy(&value, source, sizeof(value));
  return value;
}

// Writes a native-representation value to at least sizeof(T) writable bytes.
// The destination must be non-null. Byte order is unchanged.
template <typename T>
  requires std::is_trivially_copyable_v<T>
void WriteUnaligned(void* destination, const T& value) {
  std::memcpy(destination, &value, sizeof(value));
}

}  // namespace port

#endif  // CNC_RED_ALERT_PORT_UNALIGNED_H_
