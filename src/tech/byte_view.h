// File: byte views of pointer-and-length buffers, for the codec sinks and
// sources whose internals still manage their data that way.
//
// TODO: Delete once the codecs become ByteCodec implementations (Tier B of
// docs/STREAMS_REFACTOR_PLAN.md).

#ifndef CNC_RED_ALERT_TECH_BYTE_VIEW_H_
#define CNC_RED_ALERT_TECH_BYTE_VIEW_H_

#include <cstddef>
#include <span>

#include "base/numeric.h"

// Returns the length bytes at data. length must not be negative.
inline std::span<const std::byte> ByteView(const void* data, int length) {
  return {static_cast<const std::byte*>(data), base::ToSize(length)};
}

// Returns the length writable bytes at data. length must not be negative.
inline std::span<std::byte> WritableByteView(void* data, int length) {
  return {static_cast<std::byte*>(data), base::ToSize(length)};
}

#endif  // CNC_RED_ALERT_TECH_BYTE_VIEW_H_
