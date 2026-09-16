#include "sdllib/iff.h"

#include <cstddef>
#include <span>

#include "base/buffer.h"

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
    case HORIZONTAL:
      break;
    case LCW:
      return static_cast<size_t>(LCW_Uncompress(payload, output));
    [[unlikely]] case LZW12:
    case LZW14:
    case NOCOMPRESS:
    default:
      if (payload.size() < output.size()) {
        return 0;
      }
      base::CopyBytes(std::as_writable_bytes(output), std::as_bytes(payload),
                      output.size());
      break;
  }
  return output.size();
}
