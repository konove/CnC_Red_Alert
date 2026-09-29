#include "engine/gfx/shape.h"

#include <cstddef>
#include <cstdint>
#include <span>

#include "base/buffer.h"
#include "base/types.h"
#include "base/unaligned.h"

namespace engine::gfx {

int CountShapeFrames(const std::span<const std::byte> shape_file) {
  return base::ReadLittleEndian<uint16_t>(shape_file, 0);
}

std::span<const std::byte> ExtractShapeFrame(
    const std::span<const std::byte> shape_file, const int frame) {
  if (frame < 0 || frame >= CountShapeFrames(shape_file)) {
    return {};
  }
  // The offset table, and the offsets in it, count from the end of the frame
  // count rather than from the start of the file.
  const auto frames = shape_file.subspan(sizeof(uint16_t));
  const auto entry =
      base::SliceOrEmpty(frames, base::ssize{frame} * 4, sizeof(uint32_t));
  if (entry.empty()) {
    return {};
  }
  const auto frame_data =
      base::SliceOrEmpty(frames, base::ReadLittleEndian<uint32_t>(entry, 0));
  // A frame shorter than its own header is corrupt, and SliceOrEmpty() rejects
  // one that runs past the end of the file.
  const base::ssize frame_bytes = base::ReadLittleEndian<uint16_t>(
      frame_data, offsetof(ShapeHeader, frame_bytes));
  return frame_bytes >= kShapeHeaderSize
             ? base::SliceOrEmpty(frame_data, 0, frame_bytes)
             : std::span<const std::byte>{};
}

}  // namespace engine::gfx
