#include "engine/codec/block_backends.h"

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

#include "absl/base/attributes.h"
#include "base/numeric.h"
#include "base/types.h"
#include "engine/codec/lcw.h"
#include "engine/codec/lzw.h"
#include "lzo/lzo.h"
#include "lzo/lzo1x.h"
#include "lzo/lzoconf.h"

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

std::vector<std::byte> LzoBackend::Compress(
    const std::span<const std::byte> input) {
  // The compressor indexes 16384 pointers, so a fixed 64K dictionary
  // overflowed on 64-bit hosts.
  if (work_.empty()) {
    work_.resize(LZO1X_MEM_COMPRESS);
  }
  // LZO1X's documented worst case; the compressor does not check its output.
  std::vector<std::byte> output(input.size() + (input.size() / 16) + 64 + 3);
  auto length = static_cast<lzo_uint>(output.size());
  lzo1x_1_compress(AsLzoBytes(input.data()),
                   static_cast<lzo_uint>(input.size()),
                   AsLzoBytes(output.data()), &length, work_.data());
  output.resize(length);
  return output;
}

std::optional<std::vector<std::byte>> LzoBackend::Decompress(
    const std::span<const std::byte> input, const base::ssize max_size) {
  std::vector<std::byte> output(base::ToSize(max_size));
  auto length = static_cast<lzo_uint>(output.size());
  // The checked decoder keeps a corrupt payload inside both buffers.
  if (lzo1x_decompress_safe(
          AsLzoBytes(input.data()), static_cast<lzo_uint>(input.size()),
          AsLzoBytes(output.data()), &length, nullptr) != LZO_E_OK) {
    return std::nullopt;
  }
  output.resize(length);
  return output;
}

int LcwBackend::Capacity(int block_size) {
  return LcwWorstCaseSize(block_size) + kHeaderSize;
}

std::vector<std::byte> LcwBackend::Compress(
    const std::span<const std::byte> input) {
  return LCW_Comp(input);
}

std::optional<std::vector<std::byte>> LcwBackend::Decompress(
    const std::span<const std::byte> input, const base::ssize max_size) {
  return LcwUncompBounded(input, max_size);
}

int LzwBackend::Capacity(int block_size) {
  return LzwWorstCaseSize(block_size) + kHeaderSize;
}

std::vector<std::byte> LzwBackend::Compress(
    const std::span<const std::byte> input) {
  std::vector<std::byte> output(
      base::ToSize(LzwWorstCaseSize(static_cast<int>(input.size()))));
  output.resize(base::ToSize(LZW_Compress(input, output)));
  return output;
}

std::optional<std::vector<std::byte>> LzwBackend::Decompress(
    const std::span<const std::byte> input, const base::ssize max_size) {
  std::vector<std::byte> output(base::ToSize(max_size));
  // The spans stop a corrupt code stream from reading or writing past
  // either buffer.
  const int length = LZW_Uncompress(input, output);
  if (length < 0) {
    return std::nullopt;
  }
  output.resize(base::ToSize(length));
  return output;
}
