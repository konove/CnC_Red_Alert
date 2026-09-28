#include "engine/gfx/shape.h"

#include <cstddef>
#include <cstdint>
#include <span>

#include "base/numeric.h"
#include "base/unaligned.h"

char* ShapeBuffer;
std::span<uint8_t> ShapeBufferBytes;
int ShapeBufferSize;

int Extract_Shape_Count(std::span<const std::byte> buffer) {
  if (buffer.size() < sizeof(uint16_t)) {
    return 0;
  }
  return base::ReadUnaligned<uint16_t>(buffer);
}

std::span<const std::byte> Extract_Shape(std::span<const std::byte> buffer,
                                         int shape) {
  if (shape < 0 || shape >= Extract_Shape_Count(buffer)) {
    return {};
  }
  const auto entry =
      sizeof(uint16_t) + (base::ToSize(shape) * sizeof(uint32_t));
  if (entry > buffer.size() || buffer.size() - entry < sizeof(uint32_t)) {
    return {};
  }
  // Offsets count from the end of the two-byte frame count, not from the
  // start of the block.
  const auto offset = base::ReadUnaligned<uint32_t>(buffer.subspan(entry));
  if (buffer.size() < 2 || offset > buffer.size() - 2) {
    return {};
  }
  const auto data = buffer.subspan(2 + offset);
  // 10 is the header through Shape_Type::DataLength, and ShapeSize sits at
  // byte 6 of it; a frame shorter than its own header is corrupt.
  if (data.size() < 10) {
    return {};
  }
  const auto size = base::ReadUnaligned<uint16_t>(data.subspan(6));
  return size >= 10 && size <= data.size() ? data.first(size)
                                           : std::span<const std::byte>{};
}

void Set_Shape_Buffer(std::span<uint8_t> buffer) {
  ShapeBufferBytes = buffer;
  // Tiberian Dawn's INI code still reads and writes the buffer as char text
  // through this alias of the same bytes.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  ShapeBuffer = reinterpret_cast<char*>(buffer.data());
  ShapeBufferSize = static_cast<int>(buffer.size());
}
