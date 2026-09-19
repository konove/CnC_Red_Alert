// Decoders for Westwood's uncompressed XOR delta ("format 40"), which WSA
// animation frames and key-framed shapes are built from. Formerly the assembly
// in LP_ASM.ASM.

#ifndef CNC_RED_ALERT_SDLLIB_XOR_DELTA_H_
#define CNC_RED_ALERT_SDLLIB_XOR_DELTA_H_

#include <cstddef>
#include <cstdint>
#include <span>

// Both stop quietly at the first command that is malformed or would
// step outside `target` or `delta`.

// Applies the uncompressed XOR delta in `delta` to `target`, treating `target`
// as one contiguous run of pixels.
void ApplyXorDelta(std::span<uint8_t> target, std::span<const std::byte> delta);

// Applies the uncompressed XOR delta in `delta` to a `width`-pixel-wide image
// whose rows start `stride` bytes apart in `target`; pixels past `width` on
// each row are left alone. The delta is XORed onto `target`, or overwrites it
// if `copy` is set. Does nothing if `width` or `stride` is not positive or
// `stride` is less than `width`.
void ApplyXorDeltaToView(std::span<uint8_t> target,
                         std::span<const std::byte> delta, int width,
                         int stride, bool copy);

#endif  // CNC_RED_ALERT_SDLLIB_XOR_DELTA_H_
