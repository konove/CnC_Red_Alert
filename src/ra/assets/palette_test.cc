// Tests PaletteClass's per-entry adjustments.

#include "ra/assets/palette.h"

#include <array>

#include "engine/gfx/palette_color.h"
#include "gtest/gtest.h"

namespace {

using engine::gfx::kBlackColor;
using engine::gfx::kWhiteColor;
using engine::gfx::PaletteColor;

TEST(PaletteClassTest, AdjustMovesTowardThePaletteByRatioOver256) {
  PaletteClass palette(PaletteColor(0, 100, 200));
  palette.Adjust(0x40, PaletteClass(kWhiteColor));
  // Each gun moves (255 - gun) * 64 / 256.
  EXPECT_EQ(palette.at(0), PaletteColor(63, 138, 213));
  EXPECT_EQ(palette.at(255), PaletteColor(63, 138, 213));
}

TEST(PaletteClassTest, AdjustWithoutAPaletteMovesTowardBlack) {
  PaletteClass palette(PaletteColor(200, 100, 8));
  palette.Adjust(0x80);
  EXPECT_EQ(palette.at(17), PaletteColor(100, 50, 4));
}

// Play_Movie's flash: black to 8/256 of white and back with ratio 0xFF, which
// rounds toward the start and so stops one short of black.
TEST(PaletteClassTest, BlackFlashRoundTripStopsAtOne) {
  PaletteClass palette(kBlackColor);
  palette.Adjust(0x08, PaletteClass(kWhiteColor));
  EXPECT_EQ(palette.at(0), PaletteColor(7, 7, 7));
  palette.Adjust(0xFF);
  EXPECT_EQ(palette.at(0), PaletteColor(1, 1, 1));
}

TEST(PaletteClassTest, PartialAdjustLeavesUnselectedEntries) {
  std::array<char, PaletteClass::kColorCount> lookup{};
  lookup.at(3) = 1;
  const PaletteColor start(40, 80, 120);

  PaletteClass toward_white(start);
  toward_white.Partial_Adjust(0x80, PaletteClass(kWhiteColor), lookup);
  EXPECT_EQ(toward_white.at(3), PaletteColor(147, 167, 187));
  EXPECT_EQ(toward_white.at(2), start);
  EXPECT_EQ(toward_white.at(4), start);

  PaletteClass toward_black(start);
  toward_black.Partial_Adjust(0x80, lookup);
  EXPECT_EQ(toward_black.at(3), PaletteColor(20, 40, 60));
  EXPECT_EQ(toward_black.at(2), start);
}

}  // namespace
