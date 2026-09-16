// File: The unsigned byte view of a trivially copyable object.
//
// Palettes, packets, registry values and checksummed records are handed to
// byte-oriented code as the object's representation. BytesOf spells that view
// once, so call sites need neither a C-style cast nor a reinterpret_cast.
//
// Example:
//   uint32_t value = 0;
//   RegQueryValueEx(key, "Setting", nullptr, nullptr, port::BytesOf(value),
//                   &size);

#ifndef CNC_RED_ALERT_PORT_BYTES_OF_H_
#define CNC_RED_ALERT_PORT_BYTES_OF_H_

#include <cstddef>
#include <span>
#include <type_traits>

#include "absl/base/attributes.h"

namespace port {

// Viewing an object's representation through unsigned char* is one of the
// few things reinterpret_cast is defined for; keeping it here keeps it out of
// the call sites. The NOLINTs cover only that check.

// Returns a pointer to the first byte of `object`, which stays valid for as
// long as `object` does. For an array this is its first element's first byte.
template <typename T>
  requires std::is_trivially_copyable_v<T>
unsigned char* BytesOf(T& object ABSL_ATTRIBUTE_LIFETIME_BOUND) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<unsigned char*>(&object);
}

template <typename T>
  requires std::is_trivially_copyable_v<T>
const unsigned char* BytesOf(const T& object ABSL_ATTRIBUTE_LIFETIME_BOUND) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<const unsigned char*>(&object);
}

// Returns a character view of an existing byte span, preserving its exact
// capacity and constness. Character aliasing may inspect any object bytes.
template <typename T, std::size_t Extent>
  requires(sizeof(T) == 1 && std::is_trivially_copyable_v<T>)
auto CharBytes(std::span<T, Extent> bytes) {
  using Char = std::conditional_t<std::is_const_v<T>, const char, char>;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto* chars = reinterpret_cast<Char*>(bytes.data());
  // A byte and a character have equal size; the source span proves capacity.
  // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
  return std::span<Char>(chars, bytes.size());
}

// An unsigned character may alias byte storage. Preserve the source span's
// actual capacity and constness for byte-oriented graphics APIs.
template <typename T, std::size_t Extent>
  requires(sizeof(T) == 1 && std::is_trivially_copyable_v<T>)
auto UnsignedBytes(std::span<T, Extent> bytes) {
  using Byte = std::conditional_t<std::is_const_v<T>, const unsigned char, unsigned char>;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto* data = reinterpret_cast<Byte*>(bytes.data());
  // The source span proves the complete extent; unsigned char has size one.
  // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
  return std::span<Byte>(data, bytes.size());
}

}  // namespace port

#endif  // CNC_RED_ALERT_PORT_BYTES_OF_H_
