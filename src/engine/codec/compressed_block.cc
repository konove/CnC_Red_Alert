// Decodes Westwood's compressed data blocks.

#include "engine/codec/compressed_block.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "base/buffer.h"
#include "base/numeric.h"
#include "base/types.h"
#include "base/unaligned.h"
#include "engine/codec/lcw_uncompress.h"

std::vector<uint8_t> UncompressBlock(const std::span<const std::byte> block,
                                     const base::ssize max_size) {
  if (block.size() < sizeof(CompressedBlockHeader)) {
    return {};
  }
  const auto header = base::ReadUnaligned<CompressedBlockHeader>(block);
  if (header.skip_bytes < 0) {
    return {};
  }
  const auto skip_bytes = base::ToSize(header.skip_bytes);
  if (skip_bytes > block.size() - sizeof(header) ||
      header.uncompressed_bytes > base::ToSize(max_size)) {
    return {};
  }
  const auto payload = block.subspan(sizeof(header) + skip_bytes);
  const std::size_t size = header.uncompressed_bytes;
  switch (static_cast<CompressionMethod>(header.method)) {
    case NOCOMPRESS: {
      if (payload.size() < size) {
        return {};
      }
      std::vector<uint8_t> data(size);
      base::CopyBytes(std::as_writable_bytes(std::span(data)), payload, size);
      return data;
    }
    case LCW:
      return LcwUncompress(payload, base::ssize{header.uncompressed_bytes});
    // Westwood's library returned the size for HORIZONTAL without writing
    // anything (its RLE decoder was left out of the build) and copied LZW and
    // unknown methods as if uncompressed. Either way the caller took garbage
    // for a picture.
    case HORIZONTAL:
    case LZW12:
    case LZW14:
    default:
      return {};
  }
}
