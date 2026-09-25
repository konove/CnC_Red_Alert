#include "winvq/vqa32/lcw_buffer.h"

#include <span>

#include "engine/base/numeric.h"
#include "engine/base/types.h"
#include "sdllib/lcw_uncompress.h"
#include "winvq/vqa32/chunk_reader.h"

LcwBuffer::LcwBuffer(const base::ssize capacity)
    : bytes_(base::ToSize(capacity)) {}

bool LcwBuffer::LoadRaw(ChunkReader& reader, const Chunk& chunk) {
  if (!ReadAt(reader, chunk, 0)) {
    return false;
  }
  SetRaw(chunk.size);
  return true;
}

bool LcwBuffer::LoadCompressed(ChunkReader& reader, const Chunk& chunk) {
  // A payload larger than the buffer would start before it.
  const base::ssize offset = capacity() - chunk.padded_size();
  if (offset < 0 || !ReadAt(reader, chunk, offset)) {
    return false;
  }
  SetCompressed(offset, chunk.size);
  return true;
}

bool LcwBuffer::ReadAt(ChunkReader& reader, const Chunk& chunk,
                       const base::ssize offset) {
  if (offset < 0 || offset + chunk.padded_size() > capacity()) {
    return false;
  }
  return reader.ReadPayload(chunk,
                            std::span(bytes_).subspan(base::ToSize(offset)));
}

void LcwBuffer::SetRaw(const base::ssize size) {
  compressed_ = false;
  offset_ = 0;
  size_ = size;
}

void LcwBuffer::SetCompressed(const base::ssize offset,
                              const base::ssize size) {
  compressed_ = true;
  offset_ = offset;
  size_ = size;
}

void LcwBuffer::Decompress() {
  if (!compressed_) {
    return;
  }
  // The input runs to the end of the buffer rather than to size_: LCW stops at
  // its own end marker.
  const auto source =
      std::span<const unsigned char>(bytes_).subspan(base::ToSize(offset_));
  SetRaw(LCW_Uncompress(source, bytes_));
}

std::span<const unsigned char> LcwBuffer::contents() const {
  return std::span<const unsigned char>(bytes_).first(
      compressed_ ? 0 : base::ToSize(size_));
}
