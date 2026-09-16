#include "sdllib/shape.h"

#include <cstddef>
#include <cstdint>
#include <span>

#include "base/numeric.h"
#include "port/unaligned.h"

char* ShapeBuffer;
std::span<uint8_t> ShapeBufferBytes;
int ShapeBufferSize;

int Extract_Shape_Count(std::span<const std::byte> buffer) {
  if (buffer.size() < sizeof(uint16_t)) {
    return 0;
  }
  return port::ReadUnaligned<uint16_t>(buffer);
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
  const auto offset = port::ReadUnaligned<uint32_t>(buffer.subspan(entry));
  if (buffer.size() < 2 || offset > buffer.size() - 2) {
    return {};
  }
  const auto data = buffer.subspan(2 + offset);
  if (data.size() < 10) {
    return {};
  }
  const auto size = port::ReadUnaligned<uint16_t>(data.subspan(6));
  return size >= 10 && size <= data.size() ? data.first(size)
                                           : std::span<const std::byte>{};
}

void Set_Shape_Buffer(std::span<uint8_t> buffer) {
  ShapeBufferBytes = buffer;
  // char aliases the byte allocation for legacy identity-only callers.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  ShapeBuffer = reinterpret_cast<char*>(buffer.data());
  ShapeBufferSize = static_cast<int>(buffer.size());
}
