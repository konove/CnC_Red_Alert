// Decodes Westwood's compressed data blocks.

#include "engine/codec/compressed_block.h"

#include <cstddef>
#include <span>

#include "engine/base/buffer.h"
#include "engine/base/numeric.h"
#include "engine/base/types.h"
#include "engine/base/unaligned.h"
#include "engine/codec/lcw_uncompress.h"

base::ssize UncompressBlock(const std::span<const std::byte> block,
                            const std::span<std::byte> dest) {
  if (block.size() < sizeof(CompressedBlockHeader)) {
    return 0;
  }
  const auto header = base::ReadUnaligned<CompressedBlockHeader>(block);
  if (header.skip_bytes < 0) {
    return 0;
  }
  const auto skip_bytes = base::ToSize(header.skip_bytes);
  if (skip_bytes > block.size() - sizeof(header) ||
      header.uncompressed_bytes > dest.size()) {
    return 0;
  }
  const auto payload = block.subspan(sizeof(header) + skip_bytes);
  const auto output = dest.first(header.uncompressed_bytes);
  switch (static_cast<CompressionMethod>(header.method)) {
    case NOCOMPRESS:
      if (payload.size() < output.size()) {
        return 0;
      }
      base::CopyBytes(output, payload, output.size());
      return std::ssize(output);
    case LCW:
      return LCW_Uncompress(payload, output);
    // Westwood's library returned the size for HORIZONTAL without writing
    // anything (its RLE decoder was left out of the build) and copied LZW and
    // unknown methods as if uncompressed. Either way the caller took garbage
    // for a picture.
    case HORIZONTAL:
    case LZW12:
    case LZW14:
    default:
      return 0;
  }
}
