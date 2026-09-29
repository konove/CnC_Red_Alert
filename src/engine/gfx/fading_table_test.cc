#include "engine/gfx/fading_table.h"

#include <array>
#include <cstdint>
#include <span>

#include "engine/gfx/palette.h"
#include "engine/gfx/palette_color.h"
#include "gtest/gtest.h"

namespace {

using engine::gfx::Palette;
using engine::gfx::PaletteColor;

void SetColor(Palette& palette, int index, int red, int green, int blue) {
  palette.at(index) = PaletteColor::FromGuns(red, green, blue);
}

constexpr PaletteColor kWhiteGuns = PaletteColor::FromGuns(63, 63, 63);

TEST(BuildFadingTableTest, FullFadeReachesTargetWithDistinctGuns) {
  // Every gun of the target differs, so a builder that mixes up the guns
  // aims somewhere else. The unset entries are black.
  Palette palette;
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
  Palette palette(kWhiteGuns);
  SetColor(palette, 1, 20, 20, 20);
  SetColor(palette, 2, 18, 20, 20);
  SetColor(palette, 3, 22, 20, 20);
  std::array<uint8_t, 256> table{};

  Build_Fading_Table(palette, table, 1, 0);

  EXPECT_EQ(table.at(1), 3);
}

TEST(BuildFadingTableTest, PerfectMatchTakesTheFirstColor) {
  Palette palette(kWhiteGuns);
  SetColor(palette, 1, 20, 20, 20);
  std::array<uint8_t, 256> table{};

  Build_Fading_Table(palette, table, 1, 0);

  // Entry 4 is white like every entry after it; entry 2 is the first match.
  EXPECT_EQ(table.at(4), 2);
}

TEST(BuildFadingTableTest, RejectsShortTables) {
  const Palette palette;
  std::array<uint8_t, 255> table{};
  table.fill(7);

  Build_Fading_Table(palette, table, 1, 128);

  EXPECT_EQ(table.at(0), 7);
}

}  // namespace
