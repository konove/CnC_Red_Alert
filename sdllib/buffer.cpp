#include "buffer.h"

#include <cstdint>

BufferClass::BufferClass(long size)
    : Buffer(new uint8_t[size]), Size(size), Allocated(true) {}

BufferClass::BufferClass() : Buffer(nullptr), Size(0), Allocated(false) {}

BufferClass::~BufferClass() {
  if (Allocated) delete[] (uint8_t *)Buffer;
}