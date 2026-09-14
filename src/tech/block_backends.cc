#include "tech/block_backends.h"

#include <cstddef>
#include <span>

#include "absl/base/attributes.h"
#include "lzo/lzo.h"
#include "lzo/lzo1x.h"
#include "lzo/lzoconf.h"
#include "tech/lcw.h"
#include "tech/lzw.h"

namespace {

// The block header a straw once stored in front of the data.
constexpr int kHeaderSize = 4;

// LZO works on unsigned char and the codec on std::byte; both are raw bytes.
const unsigned char* AsLzoBytes(
    const void* data ABSL_ATTRIBUTE_LIFETIME_BOUND) {
  return static_cast<const unsigned char*>(data);
}
unsigned char* AsLzoBytes(void* data ABSL_ATTRIBUTE_LIFETIME_BOUND) {
  return static_cast<unsigned char*>(data);
}

}  // namespace

int LzoBackend::Compress(std::span<const std::byte> input,
                         std::span<std::byte> output) {
  // The compressor indexes 16384 pointers, so a fixed 64K dictionary
  // overflowed on 64-bit hosts.
  if (work_.empty()) {
    work_.resize(LZO1X_MEM_COMPRESS);
  }
  auto length = static_cast<lzo_uint>(output.size());
  lzo1x_1_compress(AsLzoBytes(input.data()),
                   static_cast<lzo_uint>(input.size()),
                   AsLzoBytes(output.data()), &length, work_.data());
  return static_cast<int>(length);
}

int LzoBackend::Decompress(std::span<const std::byte> input,
                           std::span<std::byte> output) {
  auto length = static_cast<lzo_uint>(output.size());
  // The checked decoder keeps a corrupt payload inside both buffers.
  if (lzo1x_decompress_safe(
          AsLzoBytes(input.data()), static_cast<lzo_uint>(input.size()),
          AsLzoBytes(output.data()), &length, nullptr) != LZO_E_OK) {
    return -1;
  }
  return static_cast<int>(length);
}

int LcwBackend::Capacity(int block_size) {
  return LcwWorstCaseSize(block_size) + kHeaderSize;
}

int LcwBackend::Compress(std::span<const std::byte> input,
                         std::span<std::byte> output) {
  return LCW_Comp(input.data(), output.data(), static_cast<int>(input.size()));
}

int LcwBackend::Decompress(std::span<const std::byte> input,
                           std::span<std::byte> output) {
  return LcwUncompBounded(input, output);
}

int LzwBackend::Capacity(int block_size) {
  return LzwWorstCaseSize(block_size) + kHeaderSize;
}

int LzwBackend::Compress(std::span<const std::byte> input,
                         std::span<std::byte> output) {
  return LZW_Compress(input, output);
}

int LzwBackend::Decompress(std::span<const std::byte> input,
                           std::span<std::byte> output) {
  // The spans stop a corrupt code stream from reading or writing past
  // either buffer.
  return LZW_Uncompress(input, output);
}
