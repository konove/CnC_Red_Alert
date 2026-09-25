// File: ChunkReader, which reads the IFF chunks a VQA movie is made of from a
// VqaIo, keeping the chunk format's size encoding and padding in one place.

#ifndef CNC_RED_ALERT_ENGINE_VIDEO_VQA_CHUNK_READER_H_
#define CNC_RED_ALERT_ENGINE_VIDEO_VQA_CHUNK_READER_H_

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <type_traits>

#include "absl/base/attributes.h"
#include "engine/video/vqa/vqaio.h"

// Chunk: the header in front of an IFF chunk's payload.
struct Chunk {
  // The four ID characters as read, comparable with the MakeChunkId()
  // constants.
  uint32_t id = 0;
  // Bytes of payload, not counting the pad byte; never negative.
  int32_t size = 0;

  // The payload with the pad byte that keeps chunks at even offsets.
  [[nodiscard]] int32_t padded_size() const { return size + (size % 2); }
};

// Why ChunkReader::Next() read no chunk.
enum class ChunkError {
  kEndOfFile,  // The file ended before a whole chunk header.
  kBadSize,    // A size of 2 GiB or more, which the format never uses.
};

// Reads chunks through a VqaIo it does not own. The reader holds no state of
// its own, so any number of them can be made over the same io.
//
// Example:
//   ChunkReader reader(io);
//   auto chunk = reader.Next();
//   if (chunk && chunk->id == kChunkCpl0) reader.ReadPayload(*chunk, palette);
class ChunkReader {
 public:
  explicit ChunkReader(VqaIo& io ABSL_ATTRIBUTE_LIFETIME_BOUND) : io_(&io) {}

  // Reads the next chunk header. The read position is then at its payload.
  std::expected<Chunk, ChunkError> Next();

  // Reads a bare chunk ID, such as the form type after a FORM header.
  // Returns nullopt when the file ends first.
  std::optional<uint32_t> ReadId();

  // Moves past the chunk's payload and pad byte. Returns false if the io
  // cannot seek there.
  bool Skip(const Chunk& chunk);

  // Reads the chunk's payload and pad byte into the start of dest. Returns
  // false, reading nothing, when dest is smaller than padded_size(), or when
  // the read fails.
  template <typename T, std::size_t kExtent>
    requires(sizeof(T) == 1 && std::is_trivially_copyable_v<T>)
  bool ReadPayload(const Chunk& chunk, std::span<T, kExtent> dest) {
    return io_->Read(std::span<T>(dest), chunk.padded_size());
  }

  // Reads one trivially copyable value from the current position, such as a
  // header at the start of a payload.
  template <typename T>
    requires std::is_trivially_copyable_v<T>
  bool ReadObject(T& value) {
    return io_->ReadObject(value);
  }

  // Reads count bytes from the current position into the start of dest.
  // Returns false, reading nothing, when dest is smaller than count.
  template <typename T, std::size_t kExtent>
    requires(sizeof(T) == 1 && std::is_trivially_copyable_v<T>)
  bool Read(std::span<T, kExtent> dest, int32_t count) {
    return io_->Read(std::span<T>(dest), count);
  }

 private:
  VqaIo* io_;
};

#endif  // CNC_RED_ALERT_ENGINE_VIDEO_VQA_CHUNK_READER_H_
