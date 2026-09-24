// Decodes Westwood's compressed data blocks.

#include "sdllib/iff.h"

#include <cstddef>
#include <span>

#include "base/buffer.h"
#include "sdllib/lcw_uncompress.h"

size_t Uncompress_Data(std::span<const unsigned char> src,
                       std::span<unsigned char> dst) {
  if (src.size() < sizeof(CompHeaderType)) {
    return 0;
  }
  CompHeaderType header{};
  base::CopyBytes(base::ObjectBytes(header), std::as_bytes(src),
                  sizeof(header));
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
      base::CopyBytes(std::as_writable_bytes(output), std::as_bytes(payload),
                      output.size());
      break;
    case LCW:
      return static_cast<size_t>(LCW_Uncompress(payload, output));
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
  return output.size();
}
