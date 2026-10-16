#include "engine/video/vqa/vq_decoder.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "base/numeric.h"
#include "gtest/gtest.h"

namespace {
TEST(VqDecoderTest, NamesTheBlockSizesItDecodes) {
  EXPECT_EQ(BlockShapeFor(4, 2), BlockShape::k4x2);
  EXPECT_EQ(BlockShapeFor(4, 4), BlockShape::k4x4);
  EXPECT_EQ(BlockShapeFor(2, 2), std::nullopt);
  EXPECT_EQ(BlockShapeFor(4, 8), std::nullopt);
}

TEST(VqDecoderTest, DecodesUnalignedCodewordsToOddStrideRows) {
  std::array<unsigned char, 17> codebook{};
  for (int i = 0; i < 16; ++i) {
    codebook.at(base::ToSize(i + 1)) = static_cast<uint8_t>(i + 1);
  }
  const std::array<unsigned char, 2> pointers{};
  std::array<unsigned char, 24> output{};
  output.fill(0xa5);
  DecodeVqFrame(BlockShape::k4x4, std::span(codebook).subspan(1), pointers,
                std::span(output).subspan(1), 1, 1, 5);
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      EXPECT_EQ(output.at(base::ToSize(1 + (row * 5) + col)),
                1 + (row * 4) + col);
    }
    EXPECT_EQ(output.at(base::ToSize(row * 5)), 0xa5);
  }
  EXPECT_EQ(output.at(20), 0xa5);

  output.fill(0xa5);
  DecodeVqFrame(BlockShape::k4x2, std::span(codebook).subspan(1), pointers,
                std::span(output).subspan(1), 1, 1, 5);
  for (int row = 0; row < 2; ++row) {
    for (int col = 0; col < 4; ++col) {
      EXPECT_EQ(output.at(base::ToSize(1 + (row * 5) + col)),
                1 + (row * 4) + col);
    }
    EXPECT_EQ(output.at(base::ToSize(row * 5)), 0xa5);
  }
  EXPECT_EQ(output.at(10), 0xa5);
}

TEST(VqDecoderTest, FillsSolidBlocksWithoutTouchingRowPadding) {
  const std::array<unsigned char, 2> pointers4 = {0x81, 0xff};
  const std::array<unsigned char, 2> pointers2 = {0x81, 0x0f};
  std::array<unsigned char, 24> output{};
  output.fill(0xa5);
  DecodeVqFrame(BlockShape::k4x4, {}, pointers4, std::span(output).subspan(1),
                1, 1, 5);
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      EXPECT_EQ(output.at(base::ToSize(1 + (row * 5) + col)), 0x81);
    }
    EXPECT_EQ(output.at(base::ToSize(row * 5)), 0xa5);
  }
  EXPECT_EQ(output.at(20), 0xa5);
  output.fill(0xa5);
  DecodeVqFrame(BlockShape::k4x2, {}, pointers2, std::span(output).subspan(1),
                1, 1, 5);
  for (int row = 0; row < 2; ++row) {
    for (int col = 0; col < 4; ++col) {
      EXPECT_EQ(output.at(base::ToSize(1 + (row * 5) + col)), 0x81);
    }
    EXPECT_EQ(output.at(base::ToSize(row * 5)), 0xa5);
  }
  EXPECT_EQ(output.at(10), 0xa5);
}
TEST(VqDecoderTest, RejectsTruncatedBuffersAndInvalidDimensions) {
  const std::array<unsigned char, 2> pointers{0x81, 0xff};
  std::array<unsigned char, 16> output{};
  output.fill(0xa5);
  DecodeVqFrame(BlockShape::k4x4, {}, pointers, output, 0, 1, 4);
  EXPECT_EQ(output.front(), 0xa5);
  DecodeVqFrame(BlockShape::k4x4, {}, std::span(pointers).first(1), output, 1,
                1, 4);
  EXPECT_EQ(output.front(), 0xa5);
  DecodeVqFrame(BlockShape::k4x4, {}, pointers, std::span(output).first(15), 1,
                1, 4);
  EXPECT_EQ(output.front(), 0xa5);
  const std::array<unsigned char, 2> codeword{0, 0};
  const std::array<unsigned char, 15> truncated_codebook{};
  DecodeVqFrame(BlockShape::k4x4, truncated_codebook, codeword, output, 1, 1,
                4);
  EXPECT_EQ(output.front(), 0xa5);
  EXPECT_EQ(output.back(), 0xa5);
}

// Entry 257 needs its high byte; entry 258 is past the end of a codebook of
// 258 blocks, and stops the decode before it.
TEST(VqDecoderTest, EntriesTakeTheirHighByteAndStopPastTheCodebook) {
  constexpr std::size_t kBlockBytes = 8;
  std::vector<unsigned char> codebook(258 * kBlockBytes);
  std::ranges::fill(std::span(codebook).subspan(257 * kBlockBytes), 0x42);
  // Low bytes, then high bytes: entries 257, 258, 0.
  const std::array<unsigned char, 6> pointers{0x01, 0x02, 0x00,
                                              0x01, 0x01, 0x00};
  std::array<unsigned char, 24> output{};
  output.fill(0xa5);
  DecodeVqFrame(BlockShape::k4x2, codebook, pointers, output, 3, 1, 12);
  for (std::size_t i = 0; i < output.size(); ++i) {
    EXPECT_EQ(output.at(i), i % 12 < 4 ? 0x42 : 0xa5) << "at " << i;
  }
}

TEST(VqDecoderDeathTest, RejectsAnImageThatHoldsItsPointers) {
  std::vector<uint8_t> buffer(64);
  const std::vector<uint8_t> codebook(16);
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(DecodeVqFrame(BlockShape::k4x2, codebook,
                             std::span(buffer).first(8), buffer, 2, 2, 8),
               "its own codebook or pointers");
}
}  // namespace
