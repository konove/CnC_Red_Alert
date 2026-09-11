#include "winvq/vqa32/unvq.h"

#include <array>
#include <cstddef>
#include <cstdint>

#include "gtest/gtest.h"

namespace {
TEST(UnvqTest, DecodesUnalignedCodewordsToOddStrideRows) {
  std::array<unsigned char, 17> codebook{};
  for (int i = 0; i < 16; ++i) {
    codebook[i + 1] = static_cast<uint8_t>(i + 1);
  }
  const std::array<unsigned char, 2> pointers{};
  std::array<unsigned char, 24> output{};
  output.fill(0xa5);
  UnVQ_4x4(codebook.data() + 1, pointers.data(), output.data() + 1, 1, 1, 5);
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      EXPECT_EQ(output[1 + row * 5 + col], 1 + row * 4 + col);
    }
    EXPECT_EQ(output[static_cast<size_t>(row * 5)], 0xa5);
  }
  EXPECT_EQ(output[20], 0xa5);

  output.fill(0xa5);
  UnVQ_4x2(codebook.data() + 1, pointers.data(), output.data() + 1, 1, 1, 5);
  for (int row = 0; row < 2; ++row) {
    for (int col = 0; col < 4; ++col) {
      EXPECT_EQ(output[1 + row * 5 + col], 1 + row * 4 + col);
    }
    EXPECT_EQ(output[static_cast<size_t>(row * 5)], 0xa5);
  }
  EXPECT_EQ(output[10], 0xa5);
}

TEST(UnvqTest, FillsSolidBlocksWithoutTouchingRowPadding) {
  const std::array<unsigned char, 2> pointers4 = {0x81, 0xff};
  const std::array<unsigned char, 2> pointers2 = {0x81, 0x0f};
  std::array<unsigned char, 24> output{};
  output.fill(0xa5);
  UnVQ_4x4(nullptr, pointers4.data(), output.data() + 1, 1, 1, 5);
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      EXPECT_EQ(output[1 + row * 5 + col], 0x81);
    }
    EXPECT_EQ(output[static_cast<size_t>(row * 5)], 0xa5);
  }
  EXPECT_EQ(output[20], 0xa5);
  output.fill(0xa5);
  UnVQ_4x2(nullptr, pointers2.data(), output.data() + 1, 1, 1, 5);
  for (int row = 0; row < 2; ++row) {
    for (int col = 0; col < 4; ++col) {
      EXPECT_EQ(output[1 + row * 5 + col], 0x81);
    }
    EXPECT_EQ(output[static_cast<size_t>(row * 5)], 0xa5);
  }
  EXPECT_EQ(output[10], 0xa5);
}
}  // namespace
