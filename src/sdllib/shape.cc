#include "sdllib/include/shape.h"

#include <cstddef>
#include <span>

char* _ShapeBuffer;
long _ShapeBufferSize;

int Extract_Shape_Count(void const* buffer) {
  ShapeBlock_Type* block = (ShapeBlock_Type*)buffer;
  return block->NumShapes;
}

int Extract_Shape_Count(const std::span<const std::byte> span) {
  return Extract_Shape_Count(span.data());
}

void* Extract_Shape(void const* buffer, int shape) {
  ShapeBlock_Type* block = (ShapeBlock_Type*)buffer;
  long offset;  // Offset of shape data, from start of block
  char* bytebuf = (char*)buffer;

  /*
  ----------------------- Return if invalid argument -----------------------
  */
  if (buffer == nullptr || shape < 0 || shape >= block->NumShapes) {
    return nullptr;
  }

  offset = block->Offsets[shape];

  return bytebuf + 2 + offset;
}

void Set_Shape_Buffer(void* buffer, int size) {
  _ShapeBuffer = static_cast<char*>(buffer);
  _ShapeBufferSize = size;
}
