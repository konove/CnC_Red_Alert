// Tests Increase_Palette_Luminance().

#include "td/interpal.h"

#include "engine/gfx/palette.h"
#include "engine/gfx/palette_color.h"
#include "gtest/gtest.h"

namespace {

TEST(IncreasePaletteLuminanceTest, BrightensEachComponentByItsPercentage) {
  engine::gfx::Palette palette(engine::gfx::PaletteColor(100, 50, 10));
  Increase_Palette_Luminance(palette, 30, 10, 0);
  EXPECT_EQ(palette.at(7), engine::gfx::PaletteColor(130, 55, 10));
}

TEST(IncreasePaletteLuminanceTest, StopsAtFullIntensity) {
  engine::gfx::Palette palette(engine::gfx::PaletteColor(240, 250, 255));
  Increase_Palette_Luminance(palette, 30, 30, 30);
  EXPECT_EQ(palette.at(0), engine::gfx::PaletteColor(255, 255, 255));
}

}  // namespace
