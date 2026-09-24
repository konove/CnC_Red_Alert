// File: LcwBuffer, the buffer a VQA movie's codebooks, palettes and vector
// pointers are loaded into, raw or LCW compressed.

#ifndef CNC_RED_ALERT_WINVQ_VQA32_LCW_BUFFER_H_
#define CNC_RED_ALERT_WINVQ_VQA32_LCW_BUFFER_H_

#include <iterator>
#include <span>
#include <vector>

#include "absl/base/attributes.h"
#include "base/types.h"
#include "winvq/vqa32/chunk_reader.h"

// A fixed-capacity buffer for data that arrives either raw or LCW compressed.
// Raw data is loaded at the start. Compressed data is loaded at the end and
// decompressed in place towards the start, which is safe because the capacity
// leaves slack for the output never to overtake input still to be read.
// Decompressing is deferred until the data is needed, and done once.
//
// Example:
//   LcwBuffer palette(1792);
//   palette.LoadCompressed(reader, chunk);
//   ...
//   palette.Decompress();
//   Use(palette.contents());
class LcwBuffer {
 public:
  explicit LcwBuffer(base::ssize capacity);

  // Loads the chunk's payload raw at the start, or compressed at the end.
  // Returns false, leaving the contents as they were, when the payload and its
  // pad byte do not fit, and false with the contents undefined when the read
  // fails.
  bool LoadRaw(ChunkReader& reader, const Chunk& chunk);
  bool LoadCompressed(ChunkReader& reader, const Chunk& chunk);

  // For data assembled from several chunks: reads the chunk's payload and pad
  // byte at offset, without changing what the contents are said to be. Returns
  // false when it does not fit or the read fails. SetRaw() or SetCompressed()
  // then declares the assembled contents.
  bool ReadAt(ChunkReader& reader, const Chunk& chunk, base::ssize offset);
  // The contents are size raw bytes at the start.
  void SetRaw(base::ssize size);
  // The contents are compressed, starting at offset, size bytes long.
  void SetCompressed(base::ssize offset, base::ssize size);

  // Decompresses the contents in place if they are compressed.
  void Decompress();

  [[nodiscard]] bool compressed() const { return compressed_; }
  // Bytes of contents: the raw size, the compressed size while compressed, or
  // what decompression produced.
  [[nodiscard]] base::ssize size() const { return size_; }
  [[nodiscard]] base::ssize capacity() const { return std::ssize(bytes_); }
  // The whole buffer, contents or not.
  [[nodiscard]] std::span<const unsigned char> data() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return bytes_;
  }
  // The whole buffer, writable. Only for the game's palette hook, which
  // adjusts the palette it is handed in place; see QueueVqaPalette().
  [[nodiscard]] std::span<unsigned char> writable_data()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return bytes_;
  }
  // The raw or decompressed contents. Empty while compressed. The span is
  // into the buffer, which clang's lifetimebound-violation check cannot see
  // through first().
  // NOLINTBEGIN(clang-diagnostic-lifetime-safety-lifetimebound-violation)
  [[nodiscard]] std::span<const unsigned char> contents() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND;
  // NOLINTEND(clang-diagnostic-lifetime-safety-lifetimebound-violation)

 private:
  std::vector<unsigned char> bytes_;
  bool compressed_ = false;
  // Where compressed contents start; 0 for raw contents.
  base::ssize offset_ = 0;
  base::ssize size_ = 0;
};

#endif  // CNC_RED_ALERT_WINVQ_VQA32_LCW_BUFFER_H_
