// Tests the fading and translucency table builders.

#include "ra/core/fading_table_builders.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>

#include "base/array.h"
#include "engine/gfx/fading_table.h"
#include "engine/gfx/palette_color.h"
#include "engine/gfx/palette_index.h"
#include "gtest/gtest.h"
#include "ra/assets/palette.h"

namespace ra {

namespace {

using engine::gfx::PaletteColor;

TEST(BuildTranslucencyTableWithTest, LooksUpEachSourceColorsFade) {
  constexpr auto kColors =
      std::to_array<TranslucentColor>({{5, 40, 0}, {9, 70, 0}});
  const TranslucencyTable<2> table =
      BuildTranslucencyTableWith(kColors, [](const TranslucentColor& entry) {
        FadingTable fade{};
        fade.fill(entry.target_color);
        return fade;
      });
  const std::span<const uint8_t> view(table);

  EXPECT_EQ(base::At(view, 5), 0);
  EXPECT_EQ(base::At(view, 9), 1);
  EXPECT_EQ(base::At(view, 6), 255);
  EXPECT_EQ(base::At(view, 256), 40);
  EXPECT_EQ(base::At(view, 511), 40);
  EXPECT_EQ(base::At(view, 512), 70);
  EXPECT_EQ(base::At(view, 767), 70);
}

TEST(BuildTranslucencyTableTest, FadesMatchTheEngineFadingTable) {
  PaletteClass palette(PaletteColor(30, 30, 30));
  palette.at(7) = PaletteColor(200, 10, 10);
  palette.at(8) = PaletteColor(110, 20, 20);
  constexpr auto kColors = std::to_array<TranslucentColor>({{3, 7, 128}});

  const TranslucencyTable<1> table = BuildTranslucencyTable(palette, kColors);

  const FadingTable expected =
      BuildFadingTable(palette, engine::gfx::PaletteIndex{7}, 128);
  EXPECT_TRUE(std::ranges::equal(std::span(table).subspan(256), expected));
}

TEST(BuildShadowFadingTableTest, StaysInsideTheShadowRange) {
  // Color 255 matches the target exactly, so only its exclusion keeps it out.
  PaletteClass palette(PaletteColor(200, 200, 200));
  palette.at(255) = PaletteColor(0, 0, 0);
  palette.at(250) = PaletteColor(20, 20, 20);

  const FadingTable table = BuildShadowFadingTable(palette, 255, 255);

  EXPECT_EQ(table.at(0), 0);
  EXPECT_EQ(table.at(1), 250);
  EXPECT_EQ(table.at(240), 250);
  for (int index = 241; index < 256; index++) {
    EXPECT_EQ(base::At(table, index), index);
  }
}

TEST(BuildShadowFadingTableTest, TiesGoToTheLowerColor) {
  PaletteClass palette(PaletteColor(200, 200, 200));
  palette.at(245) = PaletteColor(20, 20, 20);
  palette.at(250) = PaletteColor(20, 20, 20);

  const FadingTable table = BuildShadowFadingTable(palette, 0, 0);

  EXPECT_EQ(table.at(1), 240);
  EXPECT_EQ(table.at(245), 245);
  EXPECT_EQ(table.at(250), 250);
  // Color 240 is itself a candidate and the first of the equal ones.
  EXPECT_EQ(table.at(240), 240);

  const FadingTable dark = BuildShadowFadingTable(palette, 245, 255);
  EXPECT_EQ(dark.at(1), 245);
}

}  // namespace

}  // namespace ra
