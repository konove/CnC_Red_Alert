// Tests the fading and translucency tables jshell builds.

#include "ra/core/jshell.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>

#include "base/array.h"
#include "engine/gfx/fading_table.h"
#include "engine/gfx/palette_color.h"
#include "gtest/gtest.h"
#include "ra/assets/palette.h"

namespace ra {

namespace {

using engine::gfx::PaletteColor;

TEST(MakeTranslucencyTableTest, LooksUpEachSourceColorsFade) {
  constexpr auto kControl =
      std::to_array<TLucentType>({{5, 40, 0, 0}, {9, 70, 0, 0}});
  const TranslucencyTable<2> table =
      MakeTranslucencyTable(kControl, [](const TLucentType& entry) {
        FadingTable fade{};
        fade.fill(entry.DestColor);
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

TEST(BuildTranslucentTableTest, FadesMatchTheEngineFadingTable) {
  PaletteClass palette(PaletteColor(30, 30, 30));
  palette.at(7) = PaletteColor(200, 10, 10);
  palette.at(8) = PaletteColor(110, 20, 20);
  constexpr auto kControl = std::to_array<TLucentType>({{3, 7, 128, 0}});

  const TranslucencyTable<1> table = BuildTranslucentTable(palette, kControl);

  const FadingTable expected = BuildFadingTable(palette, 7, 128);
  EXPECT_TRUE(std::ranges::equal(std::span(table).subspan(256), expected));
}

TEST(ConquerBuildFadingTableTest, StaysInsideTheShadowRange) {
  // Color 255 matches the target exactly, so only its exclusion keeps it out.
  PaletteClass palette(PaletteColor(200, 200, 200));
  palette.at(255) = PaletteColor(0, 0, 0);
  palette.at(250) = PaletteColor(20, 20, 20);

  const FadingTable table = ConquerBuildFadingTable(palette, 255, 255);

  EXPECT_EQ(table.at(0), 0);
  EXPECT_EQ(table.at(1), 250);
  EXPECT_EQ(table.at(240), 250);
  for (int index = 241; index < 256; index++) {
    EXPECT_EQ(base::At(table, index), index);
  }
}

}  // namespace

}  // namespace ra
