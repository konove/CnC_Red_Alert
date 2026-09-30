#ifndef CNC_RED_ALERT_ENGINE_GFX_KEYFRAME_H_
#define CNC_RED_ALERT_ENGINE_GFX_KEYFRAME_H_

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

// Both games' keyframe shapes (.SHP): building, unit and interface images,
// stored as LCW-compressed key frames and XOR deltas against them. `shape_file`
// is always the whole file, as loaded.

namespace engine::gfx {

// The largest frame BuildFrame() decodes, in pixels: the size of the buffer
// Red Alert decoded into (Tiberian Dawn's held 40,000). The header's 16-bit
// width and height could otherwise ask for 4 GiB.
inline constexpr int kMaxFramePixels = 65000;

// Decompresses `frame` of `shape_file`.
// Returns the frame's width * height pixels, row by row, or an empty vector if
// the frame is out of range, an offset in the file points past the file's end,
// or the frame is larger than kMaxFramePixels. Compressed data that ends early
// or is malformed is not detected: the pixels it did not reach stay 0, which
// draws as transparent.
std::vector<uint8_t> BuildFrame(std::span<const std::byte> shape_file,
                                int frame);

// The fields of the shape's header: its frame count, and the width and height
// every frame shares, in pixels. All return 0 if `shape_file` is too short to
// hold a header.
int KeyframeFrameCount(std::span<const std::byte> shape_file);
int KeyframeWidth(std::span<const std::byte> shape_file);
int KeyframeHeight(std::span<const std::byte> shape_file);

}  // namespace engine::gfx

#endif  // CNC_RED_ALERT_ENGINE_GFX_KEYFRAME_H_
