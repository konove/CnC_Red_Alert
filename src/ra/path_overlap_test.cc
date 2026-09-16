#include "ra/path_overlap.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "gtest/gtest.h"
#include "ra/defines.h"

namespace {

constexpr int kWordCount = OverlapWordCount(MAP_CELL_TOTAL);

using OverlapBuffer = std::array<uint32_t, kWordCount>;

bool IsEmpty(const OverlapBuffer& words) {
  return std::ranges::all_of(words, [](uint32_t word) { return word == 0; });
}

TEST(RaPathOverlapTest, BufferHoldsExactlyOneBitPerCell) {
  EXPECT_EQ(MAP_CELL_TOTAL % 32, 0);
  EXPECT_EQ(kWordCount * 32, MAP_CELL_TOTAL);
  EXPECT_EQ(OverlapWordCount(33), 2);
}

TEST(RaPathOverlapTest, EveryCellMapsToItsOwnBit) {
  std::vector<bool> used(static_cast<size_t>(kWordCount) * 32);
  for (int cell = 0; cell < MAP_CELL_TOTAL; ++cell) {
    const uint32_t mask = OverlapMask(cell);
    ASSERT_EQ(std::popcount(mask), 1) << "cell " << cell;
    const int word = OverlapWord(cell);
    ASSERT_LT(word, kWordCount) << "cell " << cell;
    const size_t bit = (static_cast<size_t>(word) * 32) +
                       static_cast<size_t>(std::countr_zero(mask));
    ASSERT_FALSE(used[bit]) << "cell " << cell;
    used[bit] = true;
  }
}

TEST(RaPathOverlapTest, WordBoundaryCellsUseDefinedShifts) {
  // The TD original computed bit (cell & 31) - 1, a shift by -1 for these.
  EXPECT_EQ(OverlapMask(0), uint32_t{1});
  EXPECT_EQ(OverlapMask(32), uint32_t{1});
  EXPECT_EQ(OverlapMask(31), uint32_t{0x80000000});
  EXPECT_EQ(OverlapWord(31), 0);
  EXPECT_EQ(OverlapWord(32), 1);
}

TEST(RaPathOverlapTest, SetAndClearTouchOnlyTheirCell) {
  for (const int cell : {0, 1, 31, 32, 33, MAP_CELL_TOTAL - 1}) {
    OverlapBuffer words{};
    SetOverlap(words, cell);
    EXPECT_TRUE(IsOverlapped(words, cell)) << "cell " << cell;
    if (cell > 0) {
      EXPECT_FALSE(IsOverlapped(words, cell - 1)) << "cell " << cell;
    }
    if (cell < MAP_CELL_TOTAL - 1) {
      EXPECT_FALSE(IsOverlapped(words, cell + 1)) << "cell " << cell;
    }
    ClearOverlap(words, cell);
    EXPECT_FALSE(IsOverlapped(words, cell)) << "cell " << cell;
    EXPECT_TRUE(IsEmpty(words)) << "cell " << cell;
  }
}

TEST(RaPathOverlapTest, ClearKeepsTheRestOfTheWord) {
  OverlapBuffer words{};
  for (int cell = 32; cell < 64; ++cell) {
    SetOverlap(words, cell);
  }
  EXPECT_EQ(words[1], uint32_t{0xFFFFFFFF});
  ClearOverlap(words, 63);
  EXPECT_EQ(words[1], uint32_t{0x7FFFFFFF});
  EXPECT_TRUE(IsOverlapped(words, 62));
  EXPECT_EQ(words[0], uint32_t{0});
  EXPECT_EQ(words[2], uint32_t{0});
}

}  // namespace
