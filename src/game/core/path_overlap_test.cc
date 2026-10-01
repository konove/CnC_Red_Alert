#include "game/core/path_overlap.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "gtest/gtest.h"

namespace game {

namespace {

// Red Alert's 128 x 128 map; Tiberian Dawn's 64 x 64 is a prefix of it.
constexpr int kCellCount = 128 * 128;
constexpr int kWordCount = OverlapWordCount(kCellCount);

using OverlapBuffer = std::array<uint32_t, kWordCount>;

bool IsEmpty(const OverlapBuffer& words) {
  return std::ranges::all_of(words, [](uint32_t word) { return word == 0; });
}

TEST(PathOverlapTest, BufferHoldsExactlyOneBitPerCell) {
  EXPECT_EQ(kWordCount * 32, kCellCount);
  EXPECT_EQ(OverlapWordCount(64 * 64) * 32, 64 * 64);
  EXPECT_EQ(OverlapWordCount(33), 2);
}

TEST(PathOverlapTest, EveryCellMapsToItsOwnBit) {
  std::vector<bool> used(static_cast<size_t>(kWordCount) * 32);
  for (int cell = 0; cell < kCellCount; ++cell) {
    const uint32_t mask = OverlapMask(cell);
    ASSERT_EQ(std::popcount(mask), 1) << "cell " << cell;
    const int word = OverlapWord(cell);
    ASSERT_LT(word, kWordCount) << "cell " << cell;
    const size_t bit = (static_cast<size_t>(word) * 32) +
                       static_cast<size_t>(std::countr_zero(mask));
    ASSERT_FALSE(used.at(bit)) << "cell " << cell;
    used.at(bit) = true;
  }
}

TEST(PathOverlapTest, WordBoundaryCellsUseDefinedShifts) {
  // The original computed bit (cell & 31) - 1, a shift by -1 for cells 0 and
  // 32; x86 masked that count to bit 31.
  EXPECT_EQ(OverlapMask(0), uint32_t{1});
  EXPECT_EQ(OverlapMask(32), uint32_t{1});
  EXPECT_EQ(OverlapMask(31), uint32_t{0x80000000});
  EXPECT_EQ(OverlapWord(31), 0);
  EXPECT_EQ(OverlapWord(32), 1);
}

TEST(PathOverlapTest, SetAndClearTouchOnlyTheirCell) {
  for (const int cell : {0, 1, 31, 32, 33, kCellCount - 1}) {
    OverlapBuffer words{};
    SetOverlap(words, cell);
    EXPECT_TRUE(IsOverlapped(words, cell)) << "cell " << cell;
    if (cell > 0) {
      EXPECT_FALSE(IsOverlapped(words, cell - 1)) << "cell " << cell;
    }
    if (cell < kCellCount - 1) {
      EXPECT_FALSE(IsOverlapped(words, cell + 1)) << "cell " << cell;
    }
    ClearOverlap(words, cell);
    EXPECT_FALSE(IsOverlapped(words, cell)) << "cell " << cell;
    EXPECT_TRUE(IsEmpty(words)) << "cell " << cell;
  }
}

TEST(PathOverlapTest, ClearKeepsTheRestOfTheWord) {
  OverlapBuffer words{};
  for (int cell = 32; cell < 64; ++cell) {
    SetOverlap(words, cell);
  }
  EXPECT_EQ(words.at(1), uint32_t{0xFFFFFFFF});
  ClearOverlap(words, 63);
  EXPECT_EQ(words.at(1), uint32_t{0x7FFFFFFF});
  EXPECT_TRUE(IsOverlapped(words, 62));
  EXPECT_EQ(words.at(0), uint32_t{0});
  EXPECT_EQ(words.at(2), uint32_t{0});
}

}  // namespace

}  // namespace game
