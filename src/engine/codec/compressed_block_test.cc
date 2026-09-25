// UncompressBlock(): the header checks and which methods it decodes.

#include "engine/codec/compressed_block.h"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

#include "engine/base/numeric.h"
#include "engine/base/types.h"
#include "engine/base/unaligned.h"
#include "gtest/gtest.h"

namespace {

// Builds a block: a header with the method byte `method`, `skip_bytes`
// reserved bytes, then `data`.
std::vector<unsigned char> Block(char method, uint32_t uncompressed_bytes,
                                 int16_t skip_bytes,
                                 const std::vector<unsigned char>& data) {
  const CompressedBlockHeader header{.method = method,
                                     .pad = 0,
                                     .uncompressed_bytes = uncompressed_bytes,
                                     .skip_bytes = skip_bytes};
  std::vector<unsigned char> block(sizeof(header));
  port::WriteUnaligned(std::as_writable_bytes(std::span(block)), header);
  block.resize(block.size() + base::ToSize(skip_bytes), 0xee);
  block.insert(block.end(), data.begin(), data.end());
  return block;
}

std::vector<unsigned char> Block(CompressionMethod method,
                                 uint32_t uncompressed_bytes,
                                 int16_t skip_bytes,
                                 const std::vector<unsigned char>& data) {
  return Block(static_cast<char>(method), uncompressed_bytes, skip_bytes, data);
}

// UncompressBlock() over the byte views of `block` and `output`.
base::ssize Decode(std::span<const unsigned char> block,
                   std::span<unsigned char> output) {
  return UncompressBlock(std::as_bytes(block), std::as_writable_bytes(output));
}

TEST(UncompressBlockTest, CopiesUncompressedDataAfterTheSkippedArea) {
  const auto block = Block(NOCOMPRESS, 3, 2, {1, 2, 3});
  std::array<unsigned char, 4> output{};
  EXPECT_EQ(Decode(block, output), 3);
  EXPECT_EQ(output, (std::array<unsigned char, 4>{1, 2, 3, 0}));
}

TEST(UncompressBlockTest, DecodesLcw) {
  // Two literal bytes, then the end marker.
  const auto block = Block(LCW, 2, 0, {0x82, 7, 8, 0x80});
  std::array<unsigned char, 2> output{};
  EXPECT_EQ(Decode(block, output), 2);
  EXPECT_EQ(output, (std::array<unsigned char, 2>{7, 8}));
}

TEST(UncompressBlockTest, RejectsBadHeaders) {
  std::array<unsigned char, 4> output{};
  EXPECT_EQ(Decode(Block(NOCOMPRESS, 1, -1, {1}), output), 0);
  EXPECT_EQ(Decode(Block(NOCOMPRESS, 1, 5, {}), output), 0);
  EXPECT_EQ(Decode(Block(NOCOMPRESS, 5, 0, {1, 2, 3, 4, 5}), output), 0);
  EXPECT_EQ(Decode(Block(NOCOMPRESS, 3, 0, {1, 2}), output), 0);
}

TEST(UncompressBlockTest, RejectsMethodsItCannotDecode) {
  for (const CompressionMethod method : {HORIZONTAL, LZW12, LZW14}) {
    std::array<unsigned char, 3> output{9, 9, 9};
    EXPECT_EQ(Decode(Block(method, 3, 0, {1, 2, 3}), output), 0)
        << static_cast<int>(method);
    EXPECT_EQ(output, (std::array<unsigned char, 3>{9, 9, 9}));
  }
  std::array<unsigned char, 3> output{};
  // Not a CompressionMethod at all.
  EXPECT_EQ(Decode(Block(char{9}, 3, 0, {1, 2, 3}), output), 0);
}

}  // namespace
