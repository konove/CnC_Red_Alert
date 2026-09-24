// Uncompress_Data(): the header checks and which methods it decodes.

#include "sdllib/iff.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "base/buffer.h"
#include "gtest/gtest.h"

namespace {

// Builds a block: a header with the method byte `method`, `skip` reserved
// bytes, then `data`.
std::vector<unsigned char> Block(char method, uint32_t size, int16_t skip,
                                 const std::vector<unsigned char>& data) {
  const CompHeaderType header{.Method = method,
                              .pad = 0,
                              .Size = size,
                              .Skip = skip};
  std::vector<unsigned char> block(sizeof(header));
  base::CopyBytes(std::as_writable_bytes(std::span(block)),
                  base::ObjectBytes(header), sizeof(header));
  block.resize(block.size() + static_cast<size_t>(skip), 0xee);
  block.insert(block.end(), data.begin(), data.end());
  return block;
}

std::vector<unsigned char> Block(CompressionType method, uint32_t size,
                                 int16_t skip,
                                 const std::vector<unsigned char>& data) {
  return Block(static_cast<char>(method), size, skip, data);
}

TEST(UncompressDataTest, CopiesUncompressedDataAfterTheSkippedArea) {
  const auto block = Block(NOCOMPRESS, 3, 2, {1, 2, 3});
  std::array<unsigned char, 4> output{};
  EXPECT_EQ(Uncompress_Data(block, output), 3U);
  EXPECT_EQ(output, (std::array<unsigned char, 4>{1, 2, 3, 0}));
}

TEST(UncompressDataTest, DecodesLcw) {
  // Two literal bytes, then the end marker.
  const auto block = Block(LCW, 2, 0, {0x82, 7, 8, 0x80});
  std::array<unsigned char, 2> output{};
  EXPECT_EQ(Uncompress_Data(block, output), 2U);
  EXPECT_EQ(output, (std::array<unsigned char, 2>{7, 8}));
}

TEST(UncompressDataTest, RejectsBadHeaders) {
  std::array<unsigned char, 4> output{};
  EXPECT_EQ(Uncompress_Data(Block(NOCOMPRESS, 1, -1, {1}), output), 0U);
  EXPECT_EQ(Uncompress_Data(Block(NOCOMPRESS, 1, 5, {}), output), 0U);
  EXPECT_EQ(Uncompress_Data(Block(NOCOMPRESS, 5, 0, {1, 2, 3, 4, 5}), output),
            0U);
  EXPECT_EQ(Uncompress_Data(Block(NOCOMPRESS, 3, 0, {1, 2}), output), 0U);
}

TEST(UncompressDataTest, RejectsMethodsItCannotDecode) {
  for (const CompressionType method : {HORIZONTAL, LZW12, LZW14}) {
    std::array<unsigned char, 3> output{9, 9, 9};
    EXPECT_EQ(Uncompress_Data(Block(method, 3, 0, {1, 2, 3}), output), 0U)
        << static_cast<int>(method);
    EXPECT_EQ(output, (std::array<unsigned char, 3>{9, 9, 9}));
  }
  std::array<unsigned char, 3> output{};
  // Not a CompressionType at all.
  EXPECT_EQ(Uncompress_Data(Block(char{9}, 3, 0, {1, 2, 3}), output), 0U);
}

}  // namespace
