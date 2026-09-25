#include "engine/video/vqa/chunk_reader.h"

#include <bit>
#include <cstdint>
#include <expected>
#include <optional>

#include "engine/stream/seek_origin.h"

namespace {

// The 8 bytes in front of every chunk, as stored: the size is big-endian.
struct RawChunkHeader {
  uint32_t id;
  uint32_t big_endian_size;
};

}  // namespace

std::expected<Chunk, ChunkError> ChunkReader::Next() {
  RawChunkHeader raw{};
  if (!io_->ReadObject(raw)) {
    return std::unexpected(ChunkError::kEndOfFile);
  }
  // A size of 2^31 or more reads back negative, and INT32_MAX would overflow
  // padded_size(). Rejecting both here lets every caller trust the size.
  const auto size = static_cast<int32_t>(std::byteswap(raw.big_endian_size));
  if (size < 0 || size == INT32_MAX) {
    return std::unexpected(ChunkError::kBadSize);
  }
  return Chunk{.id = raw.id, .size = size};
}

std::optional<uint32_t> ChunkReader::ReadId() {
  uint32_t id = 0;
  if (!io_->ReadObject(id)) {
    return std::nullopt;
  }
  return id;
}

bool ChunkReader::Skip(const Chunk& chunk) {
  return io_->Seek(chunk.padded_size(), SeekOrigin::kCurrent);
}
