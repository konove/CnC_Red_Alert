// UncompressBlock(): the header checks and which methods it decodes.

#include "engine/codec/compressed_block.h"

#include <cstdint>
#include <span>
#include <vector>

#include "base/numeric.h"
#include "base/types.h"
#include "base/unaligned.h"
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
  base::WriteUnaligned(std::as_writable_bytes(std::span(block)), header);
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

// UncompressBlock() over the byte view of `block`.
std::vector<uint8_t> Decode(std::span<const unsigned char> block,
                            base::ssize max_size) {
  return UncompressBlock(std::as_bytes(block), max_size);
}

TEST(UncompressBlockTest, CopiesUncompressedDataAfterTheSkippedArea) {
  const auto block = Block(NOCOMPRESS, 3, 2, {1, 2, 3});
  EXPECT_EQ(Decode(block, 4), (std::vector<uint8_t>{1, 2, 3}));
}

TEST(UncompressBlockTest, DecodesLcw) {
  // Two literal bytes, then the end marker.
  const auto block = Block(LCW, 2, 0, {0x82, 7, 8, 0x80});
  EXPECT_EQ(Decode(block, 2), (std::vector<uint8_t>{7, 8}));
}

TEST(UncompressBlockTest, LcwStopsAtTheHeaderSize) {
  // The stream holds three bytes and the header promises two.
  const auto block = Block(LCW, 2, 0, {0x83, 7, 8, 9, 0x80});
  EXPECT_EQ(Decode(block, 4), (std::vector<uint8_t>{7, 8}));
}

TEST(UncompressBlockTest, RejectsBadHeaders) {
  EXPECT_TRUE(Decode(Block(NOCOMPRESS, 1, -1, {1}), 4).empty());
  EXPECT_TRUE(Decode(Block(NOCOMPRESS, 1, 5, {}), 4).empty());
  EXPECT_TRUE(Decode(Block(NOCOMPRESS, 5, 0, {1, 2, 3, 4, 5}), 4).empty());
  EXPECT_TRUE(Decode(Block(NOCOMPRESS, 3, 0, {1, 2}), 4).empty());
}

TEST(UncompressBlockTest, RejectsMethodsItCannotDecode) {
  for (const CompressionMethod method : {HORIZONTAL, LZW12, LZW14}) {
    EXPECT_TRUE(Decode(Block(method, 3, 0, {1, 2, 3}), 3).empty())
        << static_cast<int>(method);
  }
  // Not a CompressionMethod at all.
  EXPECT_TRUE(Decode(Block(char{9}, 3, 0, {1, 2, 3}), 3).empty());
}

}  // namespace
