// File: Byte spans as the slices the cxx bridge to src/rust takes.

#ifndef CNC_RED_ALERT_ENGINE_CODEC_RUST_BYTES_H_
#define CNC_RED_ALERT_ENGINE_CODEC_RUST_BYTES_H_

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>

#include "rust/cxx.h"

// Returns `bytes` as a Rust slice. cxx gives an empty span's null data the
// dangling non-null pointer Rust slices require.
inline rust::Slice<const uint8_t> AsRustSlice(
    const std::span<const std::byte> bytes) {
  const void* const data = bytes.data();
  return {static_cast<const uint8_t*>(data), bytes.size()};
}
inline rust::Slice<uint8_t> AsRustSlice(const std::span<std::byte> bytes) {
  void* const data = bytes.data();
  return {static_cast<uint8_t*>(data), bytes.size()};
}

// Returns whether `a` and `b` share a byte. Rust may not see one byte through
// a shared and a mutable slice at once, so overlapping spans must not both
// cross the bridge.
inline bool Overlaps(const std::span<const std::byte> a,
                     const std::span<const std::byte> b) {
  if (a.empty() || b.empty()) {
    return false;
  }
  const auto a_begin = std::bit_cast<uintptr_t>(a.data());
  const auto b_begin = std::bit_cast<uintptr_t>(b.data());
  return a_begin < b_begin + b.size() && b_begin < a_begin + a.size();
}

#endif  // CNC_RED_ALERT_ENGINE_CODEC_RUST_BYTES_H_
