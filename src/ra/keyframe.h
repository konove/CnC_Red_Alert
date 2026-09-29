#ifndef CNC_RED_ALERT_RA_KEYFRAME_H_
#define CNC_RED_ALERT_RA_KEYFRAME_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "absl/base/attributes.h"

// The shape staging buffer: scratch memory that Build_Frame() decompresses
// frames into, and that the expansion dialog borrows for reading scenario
// INI text. Bootstrap() points it at storage that lives for the rest of the
// run.
extern std::span<uint8_t> shape_buffer;

std::span<uint8_t> Build_Frame(std::span<const std::byte> data, uint16_t frame,
                               std::span<uint8_t> destination);
uint16_t Get_Build_Frame_Count(std::span<const std::byte> data);
uint16_t Get_Build_Frame_X(std::span<const std::byte> data);
uint16_t Get_Build_Frame_Y(std::span<const std::byte> data);
uint16_t Get_Build_Frame_Width(std::span<const std::byte> data);
uint16_t Get_Build_Frame_Height(std::span<const std::byte> data);

#endif  // CNC_RED_ALERT_RA_KEYFRAME_H_
