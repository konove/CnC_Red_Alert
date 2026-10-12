#include "engine/gfx/fading_table.h"

#include "engine/gfx/palette.h"
#include "engine/gfx/palette_color.h"
#include "engine/gfx/palette_index.h"
#include "gtest/gtest.h"

namespace {

using engine::gfx::Palette;
using engine::gfx::PaletteColor;
using engine::gfx::PaletteIndex;

void SetColor(Palette& palette, int index, int red, int green, int blue) {
  palette.at(index) =
      PaletteColor(engine::gfx::ExpandGun(red), engine::gfx::ExpandGun(green),
                   engine::gfx::ExpandGun(blue));
}

constexpr PaletteColor kWhiteGuns(255, 255, 255);

TEST(BuildFadingTableTest, FullFadeReachesTargetWithDistinctGuns) {
  // Every gun of the target differs, so a builder that mixes up the guns
  // aims somewhere else. The unset entries are black.
  Palette palette;
  SetColor(palette, 1, 10, 20, 50);
  SetColor(palette, 2, 50, 20, 10);
  SetColor(palette, 3, 20, 50, 10);
  const FadingTable table = BuildFadingTable(palette, PaletteIndex{1}, 255);

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
  const FadingTable table = BuildFadingTable(palette, PaletteIndex{1}, 0);

  EXPECT_EQ(table.at(1), 3);
}

TEST(BuildFadingTableTest, PerfectMatchTakesTheFirstColor) {
  Palette palette(kWhiteGuns);
  SetColor(palette, 1, 20, 20, 20);
  const FadingTable table = BuildFadingTable(palette, PaletteIndex{1}, 0);

  // Entry 4 is white like every entry after it; entry 2 is the first match.
  EXPECT_EQ(table.at(4), 2);
}

}  // namespace
