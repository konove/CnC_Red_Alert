// Checked recovery of objects stored in aligned, type-erased buffers.
#ifndef CNC_RED_ALERT_PORT_ALIGNED_BUFFER_H_
#define CNC_RED_ALERT_PORT_ALIGNED_BUFFER_H_

#include <bit>
#include <cstdint>

#include "absl/log/check.h"

namespace port {

// Returns the object at storage, checking its alignment. storage must be null
// or point to a live T (including implicitly created objects in allocated
// storage). This does not make arbitrary file/packet bytes into a T; use
// ReadUnaligned for byte offsets whose alignment and object lifetime are not
// guaranteed.
template <typename T>
T* AlignedObject(void* storage) {
  CHECK_EQ(std::bit_cast<uintptr_t>(storage) % alignof(T), 0);
  return static_cast<T*>(storage);
}

// Recovers a mutable object whose pointer was stored opaquely as const void*.
// The original object must be mutable and remain alive. Null is permitted.
template <typename T>
T* RestoreMutableObject(const void* storage) {
  // The const qualifier belongs to the opaque view, not the original object.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  return AlignedObject<T>(const_cast<void*>(storage));
}

}  // namespace port

#endif  // CNC_RED_ALERT_PORT_ALIGNED_BUFFER_H_
