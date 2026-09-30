#ifndef CNC_RED_ALERT_RA_KEYFRAME_H_
#define CNC_RED_ALERT_RA_KEYFRAME_H_

#include <cstddef>
#include <cstdint>
#include <span>

// Red Alert's keyframe shapes: building, unit and interface images, stored as
// LCW-compressed key frames and XOR deltas against them. `shape_file` is always
// the whole file, as loaded.

// Decompresses `frame` of `shape_file` into `destination`.
// Returns the frame's width * height pixels at the front of `destination`, or
// an empty span if the frame is out of range, an offset in the file points
// past the file's end, or the frame does not fit. Compressed data that ends
// early or is malformed is not detected: the pixels it did not reach keep
// whatever `destination` held.
std::span<uint8_t> BuildFrame(std::span<const std::byte> shape_file, int frame,
                              std::span<uint8_t> destination);
// As above, into kShapeBufferSize bytes of staging storage that every call of
// this overload reuses: the result is only valid until the next call.
std::span<uint8_t> BuildFrame(std::span<const std::byte> shape_file, int frame);

// The fields of the shape's header: its frame count, and the width and height
// every frame shares, in pixels. All return 0 if `shape_file` is too short to
// hold a header.
int KeyframeFrameCount(std::span<const std::byte> shape_file);
int KeyframeWidth(std::span<const std::byte> shape_file);
int KeyframeHeight(std::span<const std::byte> shape_file);

#endif  // CNC_RED_ALERT_RA_KEYFRAME_H_
