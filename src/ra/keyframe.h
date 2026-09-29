#ifndef CNC_RED_ALERT_RA_KEYFRAME_H_
#define CNC_RED_ALERT_RA_KEYFRAME_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "absl/base/attributes.h"

// Decompresses `frame` of the keyframe shape `data` into `destination`.
// Returns the frame's width * height pixels at the front of `destination`, or
// an empty span if the frame is out of range, corrupt or does not fit.
std::span<uint8_t> Build_Frame(std::span<const std::byte> data, uint16_t frame,
                               std::span<uint8_t> destination);
// As above, into staging storage that every call of this overload reuses: the
// result is only valid until the next call.
std::span<uint8_t> Build_Frame(std::span<const std::byte> data, uint16_t frame);
uint16_t Get_Build_Frame_Count(std::span<const std::byte> data);
uint16_t Get_Build_Frame_X(std::span<const std::byte> data);
uint16_t Get_Build_Frame_Y(std::span<const std::byte> data);
uint16_t Get_Build_Frame_Width(std::span<const std::byte> data);
uint16_t Get_Build_Frame_Height(std::span<const std::byte> data);

#endif  // CNC_RED_ALERT_RA_KEYFRAME_H_
