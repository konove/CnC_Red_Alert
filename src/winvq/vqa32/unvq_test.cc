#include "winvq/vqa32/unvq.h"

#include <array>
#include <cstdint>
#include <span>

#include "base/numeric.h"
#include "gtest/gtest.h"

namespace {
TEST(UnvqTest, DecodesUnalignedCodewordsToOddStrideRows) {
  std::array<unsigned char, 17> codebook{};
  for (int i = 0; i < 16; ++i) {
    codebook[base::ToSize(i + 1)] = static_cast<uint8_t>(i + 1);
  }
  const std::array<unsigned char, 2> pointers{};
  std::array<unsigned char, 24> output{};
  output.fill(0xa5);
  UnVQ_4x4(std::span(codebook).subspan(1), pointers,
           std::span(output).subspan(1), 1, 1, 5);
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      EXPECT_EQ(output[base::ToSize(1 + (row * 5) + col)], 1 + (row * 4) + col);
    }
    EXPECT_EQ(output[base::ToSize(row * 5)], 0xa5);
  }
  EXPECT_EQ(output[20], 0xa5);

  output.fill(0xa5);
  UnVQ_4x2(std::span(codebook).subspan(1), pointers,
           std::span(output).subspan(1), 1, 1, 5);
  for (int row = 0; row < 2; ++row) {
    for (int col = 0; col < 4; ++col) {
      EXPECT_EQ(output[base::ToSize(1 + (row * 5) + col)], 1 + (row * 4) + col);
    }
    EXPECT_EQ(output[base::ToSize(row * 5)], 0xa5);
  }
  EXPECT_EQ(output[10], 0xa5);
}

TEST(UnvqTest, FillsSolidBlocksWithoutTouchingRowPadding) {
  const std::array<unsigned char, 2> pointers4 = {0x81, 0xff};
  const std::array<unsigned char, 2> pointers2 = {0x81, 0x0f};
  std::array<unsigned char, 24> output{};
  output.fill(0xa5);
  UnVQ_4x4({}, pointers4, std::span(output).subspan(1), 1, 1, 5);
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      EXPECT_EQ(output[base::ToSize(1 + (row * 5) + col)], 0x81);
    }
    EXPECT_EQ(output[base::ToSize(row * 5)], 0xa5);
  }
  EXPECT_EQ(output[20], 0xa5);
  output.fill(0xa5);
  UnVQ_4x2({}, pointers2, std::span(output).subspan(1), 1, 1, 5);
  for (int row = 0; row < 2; ++row) {
    for (int col = 0; col < 4; ++col) {
      EXPECT_EQ(output[base::ToSize(1 + (row * 5) + col)], 0x81);
    }
    EXPECT_EQ(output[base::ToSize(row * 5)], 0xa5);
  }
  EXPECT_EQ(output[10], 0xa5);
}
TEST(UnvqTest, RejectsTruncatedBuffersAndInvalidDimensions) {
  const std::array<unsigned char, 2> pointers{0x81, 0xff};
  std::array<unsigned char, 16> output{};
  output.fill(0xa5);
  UnVQ_4x4({}, pointers, output, 0, 1, 4);
  EXPECT_EQ(output.front(), 0xa5);
  UnVQ_4x4({}, std::span(pointers).first(1), output, 1, 1, 4);
  EXPECT_EQ(output.front(), 0xa5);
  UnVQ_4x4({}, pointers, std::span(output).first(15), 1, 1, 4);
  EXPECT_EQ(output.front(), 0xa5);
  const std::array<unsigned char, 2> codeword{0, 0};
  const std::array<unsigned char, 15> truncated_codebook{};
  UnVQ_4x4(truncated_codebook, codeword, output, 1, 1, 4);
  EXPECT_EQ(output.front(), 0xa5);
  EXPECT_EQ(output.back(), 0xa5);
}
}  // namespace
