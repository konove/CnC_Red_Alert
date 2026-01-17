#include "sdllib/include/iff.h"

#include <cstddef>
#include <cstring>

#include "absl/log/log.h"

[[nodiscard]] size_t Uncompress_Data(void* src, void* dst) {
  if (src == nullptr || dst == nullptr) {
    return 0;
  }

  // Interpret the data block header structure to determine
  // compression method, size, and skip data amount.
  CompHeaderType header;
  std::memcpy(&header, src, sizeof(header));

  const size_t uncompressed_size = header.Size;
  // Number of leading data to skip.
  const size_t skip = static_cast<size_t>(header.Skip);
  // Compression method used.
  const auto method = static_cast<CompressionType>(header.Method);

  // Advance past header and skip data.
  auto* payload_src =
      static_cast<std::byte*>(src) + sizeof(CompHeaderType) + skip;
  auto* payload_dst = static_cast<std::byte*>(dst);

  switch (method) {
    case HORIZONTAL:
      break;
    case LCW:
      LCW_Uncompress(payload_src, payload_dst, uncompressed_size);
      break;
    // Unsupported compression methods - treat as uncompressed.
    case LZW12:
    case LZW14:
    case NOCOMPRESS:
    [[unlikely]] default:
      std::memcpy(payload_src, payload_dst, uncompressed_size);
      break;
  }

  return uncompressed_size;
}

extern "C" int LCW_Comp(const void* /*source*/, void* /*dest*/,
                        int /*length*/) {
  DLOG(INFO) << "LCW compression not implemented";
  return 0;
}
