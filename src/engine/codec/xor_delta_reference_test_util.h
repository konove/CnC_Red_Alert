// The C++ XOR delta decoder that engine_rs's xor_delta module replaced
// (src/rust/src/xor_delta.rs), kept so that a differential test and
// kernel_bench can hold the Rust one to it.

#ifndef CNC_RED_ALERT_ENGINE_CODEC_XOR_DELTA_REFERENCE_TEST_UTIL_H_
#define CNC_RED_ALERT_ENGINE_CODEC_XOR_DELTA_REFERENCE_TEST_UTIL_H_

#include <cstddef>
#include <cstdint>
#include <span>

namespace xor_delta_reference {

// As ::ApplyXorDelta() and ::ApplyXorDeltaToView() (engine/codec/xor_delta.h).
void ApplyXorDelta(std::span<uint8_t> target, std::span<const std::byte> delta);
void ApplyXorDeltaToView(std::span<uint8_t> target,
                         std::span<const std::byte> delta, int width,
                         int stride, bool copy);

}  // namespace xor_delta_reference

#endif  // CNC_RED_ALERT_ENGINE_CODEC_XOR_DELTA_REFERENCE_TEST_UTIL_H_
