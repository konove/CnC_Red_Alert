#include "engine/gfx/fading_table.h"

#include <array>
#include <cstdint>
#include <span>

#include "base/array.h"
#include "gtest/gtest.h"

namespace {

using Palette = std::array<uint8_t, 768>;

void SetColor(std::span<uint8_t> palette, int index, uint8_t red, uint8_t green,
              uint8_t blue) {
  base::At(palette, (index * 3) + 0) = red;
  base::At(palette, (index * 3) + 1) = green;
  base::At(palette, (index * 3) + 2) = blue;
}

TEST(BuildFadingTableTest, FullFadeReachesTargetWithDistinctGuns) {
  // Every gun of the target differs, so a builder that mixes up the guns
  // aims somewhere else. The unset entries are black.
  Palette palette{};
  SetColor(palette, 1, 10, 20, 50);
  SetColor(palette, 2, 50, 20, 10);
  SetColor(palette, 3, 20, 50, 10);
  std::array<uint8_t, 256> table{};

  Build_Fading_Table(palette, table, 1, 255);

  EXPECT_EQ(table.at(0), 0);
  EXPECT_EQ(table.at(2), 1);
  EXPECT_EQ(table.at(3), 1);
}

TEST(BuildFadingTableTest, TiesGoToTheLaterColor) {
  // With no fade, entry 1 is equally far from entries 2 and 3.
  Palette palette{};
  palette.fill(63);
  SetColor(palette, 1, 20, 20, 20);
  SetColor(palette, 2, 18, 20, 20);
  SetColor(palette, 3, 22, 20, 20);
  std::array<uint8_t, 256> table{};

  Build_Fading_Table(palette, table, 1, 0);

  EXPECT_EQ(table.at(1), 3);
}

TEST(BuildFadingTableTest, PerfectMatchTakesTheFirstColor) {
  Palette palette{};
  palette.fill(63);
  SetColor(palette, 1, 20, 20, 20);
  std::array<uint8_t, 256> table{};

  Build_Fading_Table(palette, table, 1, 0);

  // Entry 4 is white like every entry after it; entry 2 is the first match.
  EXPECT_EQ(table.at(4), 2);
}

TEST(BuildFadingTableTest, RejectsShortBuffers) {
  const std::array<uint8_t, 767> palette{};
  std::array<uint8_t, 256> table{};
  table.fill(7);

  Build_Fading_Table(palette, table, 1, 128);

  EXPECT_EQ(table.at(0), 7);
}

}  // namespace
