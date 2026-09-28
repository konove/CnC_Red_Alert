#include "engine/gfx/shape.h"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>

#include "base/numeric.h"
#include "base/types.h"
#include "base/unaligned.h"

std::span<uint8_t> shape_buffer;

int CountShapeFrames(const std::span<const std::byte> shape_file) {
  if (shape_file.size() < sizeof(uint16_t)) {
    return 0;
  }
  return base::ReadUnaligned<uint16_t>(shape_file);
}

std::span<const std::byte> ExtractShapeFrame(
    const std::span<const std::byte> shape_file, const int frame) {
  if (frame < 0 || frame >= CountShapeFrames(shape_file)) {
    return {};
  }
  // The offset table, and the offsets in it, count from the end of the frame
  // count rather than from the start of the file.
  const auto frames = shape_file.subspan(sizeof(uint16_t));
  const auto entry_offset = base::ToSize(frame) * sizeof(uint32_t);
  if (frames.size() < entry_offset + sizeof(uint32_t)) {
    return {};
  }
  const auto frame_offset =
      base::ReadUnaligned<uint32_t>(frames.subspan(entry_offset));
  if (frame_offset > frames.size()) {
    return {};
  }
  const auto frame_data = frames.subspan(frame_offset);
  // A frame shorter than its own header is corrupt.
  if (std::ssize(frame_data) < kShapeHeaderSize) {
    return {};
  }
  const base::ssize frame_bytes = base::ReadUnaligned<uint16_t>(
      frame_data.subspan(offsetof(ShapeHeader, frame_bytes)));
  return frame_bytes >= kShapeHeaderSize &&
                 frame_bytes <= std::ssize(frame_data)
             ? frame_data.first(base::ToSize(frame_bytes))
             : std::span<const std::byte>{};
}
