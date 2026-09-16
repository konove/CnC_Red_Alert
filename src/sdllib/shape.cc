#include "sdllib/shape.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

char* ShapeBuffer;
int ShapeBufferSize;

int Extract_Shape_Count(const void* buffer) {
  const auto* block = static_cast<const ShapeBlock_Type*>(buffer);
  return block->NumShapes;
}

int Extract_Shape_Count(const std::span<const std::byte> span) {
  return Extract_Shape_Count(span.data());
}

const void* Extract_Shape(const void* buffer, int shape) {
  const auto* block = static_cast<const ShapeBlock_Type*>(buffer);
  const char* bytebuf = static_cast<const char*>(buffer);

  /*
  ----------------------- Return if invalid argument -----------------------
  */
  if (buffer == nullptr || shape < 0 ||
      std::cmp_greater_equal(shape, block->NumShapes)) {
    return nullptr;
  }

  const uint32_t offset =
      block->Offsets[shape];  // Offset of shape data, from start of block

  return bytebuf + 2 + offset;
}

void Set_Shape_Buffer(void* buffer, int size) {
  ShapeBuffer = static_cast<char*>(buffer);
  ShapeBufferSize = size;
}
