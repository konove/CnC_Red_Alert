// Tests the initial state of Palettes and ThePalettes().

#include "ra/palettes.h"

#include "base/installed.h"
#include "engine/gfx/rgb.h"
#include "gtest/gtest.h"
#include "ra/defines.h"

namespace {

// Returns `rgb` as one number, 0xRRGGBB, for comparing in expectations.
int Packed(const engine::gfx::PaletteColor& rgb) {
  return (rgb.red() * 65536) + (rgb.green() * 256) + rgb.blue();
}

TEST(PalettesTest, BlackAndWhiteAreFilledIn) {
  Palettes palettes;
  const int white = Packed(
      engine::gfx::PaletteColor(engine::gfx::PaletteColor::kMaxComponent,
                                engine::gfx::PaletteColor::kMaxComponent,
                                engine::gfx::PaletteColor::kMaxComponent));
  ASSERT_NE(white, 0);
  for (int i = 0; i < 256; ++i) {
    EXPECT_EQ(Packed(palettes.black_palette().at(i)), 0);
    EXPECT_EQ(Packed(palettes.white_palette().at(i)), white);
  }
}

TEST(PalettesTest, EverythingElseStartsZeroed) {
  Palettes palettes;
  EXPECT_EQ(Packed(palettes.game_palette().at(255)), 0);
  EXPECT_EQ(palettes.color_remaps().at(PCOLOR_GOLD).Color, 0);
  EXPECT_EQ(palettes.grey_scheme().BrightColor, 0);
  EXPECT_FALSE(palettes.slow_palette());
}

TEST(PalettesTest, ThePalettesReturnsTheInstalledPalettes) {
  Palettes palettes;
  const base::Installed<Palettes>::Scope scope(palettes);
  EXPECT_EQ(&ThePalettes(), &palettes);
}

}  // namespace
