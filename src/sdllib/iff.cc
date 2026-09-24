// Decodes Westwood's compressed data blocks.

#include "sdllib/iff.h"

#include <cstddef>
#include <span>

#include "base/buffer.h"
#include "base/types.h"
#include "sdllib/lcw_uncompress.h"

base::ssize Uncompress_Data(std::span<const std::byte> src,
                            std::span<std::byte> dst) {
  if (src.size() < sizeof(CompHeaderType)) {
    return 0;
  }
  CompHeaderType header{};
  base::CopyBytes(base::ObjectBytes(header), src, sizeof(header));
  if (header.Skip < 0) {
    return 0;
  }
  const auto skip = static_cast<size_t>(header.Skip);
  if (skip > src.size() - sizeof(header) || header.Size > dst.size()) {
    return 0;
  }
  const auto payload = src.subspan(sizeof(header) + skip);
  const auto output = dst.first(header.Size);
  switch (static_cast<CompressionType>(header.Method)) {
    case NOCOMPRESS:
      if (payload.size() < output.size()) {
        return 0;
      }
      base::CopyBytes(output, payload, output.size());
      break;
    case LCW:
      return LCW_Uncompress(payload, output);
    // Westwood's library returned Size for HORIZONTAL without writing anything
    // (its RLE decoder was left out of the build) and copied LZW and unknown
    // methods as if uncompressed. Either way the caller took garbage for a
    // picture.
    case HORIZONTAL:
    case LZW12:
    case LZW14:
    default:
      return 0;
  }
  return std::ssize(output);
}
