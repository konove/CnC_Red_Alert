#include "engine/gfx/shape.h"

#include <cstddef>
#include <cstdint>
#include <span>

#include "base/numeric.h"
#include "base/unaligned.h"

std::span<uint8_t> shape_buffer;

int CountShapeFrames(std::span<const std::byte> shape_file) {
  if (shape_file.size() < sizeof(uint16_t)) {
    return 0;
  }
  return base::ReadUnaligned<uint16_t>(shape_file);
}

std::span<const std::byte> ExtractShapeFrame(
    std::span<const std::byte> shape_file, int frame) {
  if (frame < 0 || frame >= CountShapeFrames(shape_file)) {
    return {};
  }
  const auto entry_offset =
      sizeof(uint16_t) + (base::ToSize(frame) * sizeof(uint32_t));
  if (entry_offset > shape_file.size() ||
      shape_file.size() - entry_offset < sizeof(uint32_t)) {
    return {};
  }
  // Offsets count from the end of the two-byte frame count, not from the
  // start of the block.
  const auto frame_offset =
      base::ReadUnaligned<uint32_t>(shape_file.subspan(entry_offset));
  if (shape_file.size() < 2 || frame_offset > shape_file.size() - 2) {
    return {};
  }
  const auto frame_data = shape_file.subspan(2 + frame_offset);
  // 10 is the header through ShapeHeader::decompressed_bytes, and frame_bytes
  // sits at byte 6 of it; a frame shorter than its own header is corrupt.
  if (frame_data.size() < 10) {
    return {};
  }
  const auto frame_bytes = base::ReadUnaligned<uint16_t>(frame_data.subspan(6));
  return frame_bytes >= 10 && frame_bytes <= frame_data.size()
             ? frame_data.first(frame_bytes)
             : std::span<const std::byte>{};
}
