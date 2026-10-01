// Tests the initial state of Palettes and ThePalettes().

#include "td/assets/palettes.h"

#include "base/installed.h"
#include "engine/gfx/palette.h"
#include "gtest/gtest.h"

namespace {

TEST(PalettesTest, PalettesStartBlack) {
  Palettes palettes;
  const engine::gfx::Palette black;
  EXPECT_EQ(palettes.game_palette(), black);
  EXPECT_EQ(palettes.original_palette(), black);
  EXPECT_EQ(palettes.title_palette(), black);
  EXPECT_EQ(palettes.black_palette(), black);
  EXPECT_EQ(palettes.white_palette(), black);
}

TEST(PalettesTest, SlowPaletteDefaultsToOn) {
  Palettes palettes;
  EXPECT_TRUE(palettes.slow_palette());
  palettes.set_slow_palette(false);
  EXPECT_FALSE(palettes.slow_palette());
}

TEST(PalettesTest, ThePalettesReturnsTheInstalledPalettes) {
  Palettes palettes;
  const base::Installed<Palettes>::Scope scope(palettes);
  EXPECT_EQ(&ThePalettes(), &palettes);
}

}  // namespace
