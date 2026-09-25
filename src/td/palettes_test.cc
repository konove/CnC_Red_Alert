// Tests the initial state of Palettes and ThePalettes().

#include "td/palettes.h"

#include "engine/base/installed.h"
#include "gtest/gtest.h"

namespace {

TEST(PalettesTest, PalettesStartEmpty) {
  Palettes palettes;
  EXPECT_TRUE(palettes.game_palette().empty());
  EXPECT_TRUE(palettes.original_palette().empty());
  EXPECT_TRUE(palettes.title_palette().empty());
  EXPECT_TRUE(palettes.black_palette().empty());
  EXPECT_TRUE(palettes.white_palette().empty());
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
