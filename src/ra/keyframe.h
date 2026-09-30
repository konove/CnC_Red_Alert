#ifndef CNC_RED_ALERT_RA_KEYFRAME_H_
#define CNC_RED_ALERT_RA_KEYFRAME_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "absl/base/attributes.h"

// Red Alert's keyframe shapes: building, unit and interface images, stored as
// LCW-compressed key frames and XOR deltas against them. `data` is always the
// whole shape file, as loaded.

// Decompresses `frame` of the keyframe shape `data` into `destination`.
// Returns the frame's width * height pixels at the front of `destination`, or
// an empty span if the frame is out of range, an offset in the file points
// past its end, or the frame does not fit. Compressed data that ends early or
// is malformed is not detected: the pixels it did not reach keep whatever
// `destination` held.
std::span<uint8_t> Build_Frame(std::span<const std::byte> data, int frame,
                               std::span<uint8_t> destination);
// As above, into kShapeBufferSize bytes of staging storage that every call of
// this overload reuses: the result is only valid until the next call.
std::span<uint8_t> Build_Frame(std::span<const std::byte> data, int frame);

// The fields of the shape's header: its frame count, its x and y, and the width
// and height every frame shares, in pixels. All return 0 if `data` is too short
// to hold a header.
int Get_Build_Frame_Count(std::span<const std::byte> data);
int Get_Build_Frame_X(std::span<const std::byte> data);
int Get_Build_Frame_Y(std::span<const std::byte> data);
int Get_Build_Frame_Width(std::span<const std::byte> data);
int Get_Build_Frame_Height(std::span<const std::byte> data);

#endif  // CNC_RED_ALERT_RA_KEYFRAME_H_
