#include "engine/codec/xor_delta.h"

#include <cstddef>
#include <cstdint>
#include <span>

#include "absl/log/check.h"
#include "base/numeric.h"
#include "engine/codec/rust_bytes.h"
#include "engine_rs_cxx/lib.h"

namespace {

// The decoder is engine_rs's xor_delta::apply (src/rust/src/xor_delta.rs).
void Apply(const std::span<uint8_t> target,
           const std::span<const std::byte> delta, const size_t width,
           const size_t stride, const bool copy) {
  const auto pixels = std::as_writable_bytes(target);
  // Rust may not see one byte through a shared and a mutable slice at once.
  // No caller decodes a delta onto itself.
  CHECK(!Overlaps(pixels, delta)) << "XOR delta overlaps its target";
  engine_rs::xor_delta_apply(AsRustSlice(pixels), AsRustSlice(delta), width,
                             stride, copy);
}

}  // namespace

void ApplyXorDelta(const std::span<uint8_t> target,
                   const std::span<const std::byte> delta) {
  // One row as wide as the whole target makes the decoder treat it as a
  // contiguous run of pixels.
  Apply(target, delta, target.size(), target.size(), false);
}

void ApplyXorDeltaToView(const std::span<uint8_t> target,
                         const std::span<const std::byte> delta,
                         const int width, const int stride, const bool copy) {
  if (width <= 0 || stride <= 0) {
    return;
  }
  Apply(target, delta, base::ToSize(width), base::ToSize(stride), copy);
}
