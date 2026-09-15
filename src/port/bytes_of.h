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

}  // namespace port

#endif  // CNC_RED_ALERT_PORT_BYTES_OF_H_
