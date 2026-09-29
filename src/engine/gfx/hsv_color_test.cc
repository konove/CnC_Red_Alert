#include "engine/gfx/hsv_color.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>

#include "engine/gfx/palette.h"
#include "engine/gfx/palette_color.h"
#include "gtest/gtest.h"

namespace engine::gfx {
namespace {

// The color with VGA guns `r`, `g` and `b` (0..63), as a palette file
// loads it.
PaletteColor Guns(const int r, const int g, const int b) {
  return {ExpandGun(r), ExpandGun(g), ExpandGun(b)};
}

std::array<int, 3> HsvToRgb(const int h, const int s, const int v) {
  const PaletteColor color =
      HsvColor(static_cast<uint8_t>(h), static_cast<uint8_t>(s),
               static_cast<uint8_t>(v))
          .ToPaletteColor();
  return {color.red(), color.green(), color.blue()};
}

std::array<int, 3> GunsToHsv(const int r, const int g, const int b) {
  const HsvColor hsv = Guns(r, g, b).ToHsv();
  return {hsv.hue(), hsv.saturation(), hsv.value()};
}

// Pinned against Tiberian Dawn's original conversions, which its options
// dialog builds its palette from. Those scaled a gun up by rounding gun * 255 /
// 63; ExpandGun() differs at 10 of the 64 levels, so 50 now reads as 203, not
// 202.
TEST(HsvColorTest, ToHsvPinnedValues) {
  EXPECT_EQ(GunsToHsv(63, 0, 0), (std::array{255, 255, 255}));
  EXPECT_EQ(GunsToHsv(0, 63, 0), (std::array{85, 255, 255}));
  EXPECT_EQ(GunsToHsv(63, 32, 0), (std::array{22, 255, 255}));
  EXPECT_EQ(GunsToHsv(10, 40, 50), (std::array{138, 205, 203}));
  EXPECT_EQ(GunsToHsv(32, 32, 32), (std::array{0, 0, 130}));
}

TEST(HsvColorTest, ToPaletteColorPinnedValues) {
  EXPECT_EQ(HsvToRgb(0, 255, 255), (std::array{255, 0, 0}));
  EXPECT_EQ(HsvToRgb(85, 255, 255), (std::array{0, 255, 0}));
  EXPECT_EQ(HsvToRgb(170, 255, 255), (std::array{0, 0, 255}));
  EXPECT_EQ(HsvToRgb(30, 128, 200), (std::array{200, 170, 100}));
}

TEST(HsvColorTest, PrimariesConvertToFullGuns) {
  const PaletteColor red = HsvColor(0, 255, 255).ToPaletteColor();
  EXPECT_EQ(red.red(), 255);
  EXPECT_EQ(red.green(), 0);
  EXPECT_EQ(red.blue(), 0);
  const PaletteColor blue = HsvColor(170, 255, 255).ToPaletteColor();
  EXPECT_EQ(blue.red(), 0);
  EXPECT_EQ(blue.green(), 0);
  EXPECT_EQ(blue.blue(), 255);
}

// Every 8-bit color survives the trip through HsvColor to within 4 of 255
// per component: HsvColor's own 8-bit hue, saturation and value are the
// limit, since a sextant of the hue circle spans only 42 or 43 hues.
TEST(HsvColorTest, PaletteRoundTripIsWithinFourSteps) {
  constexpr int kMax = PaletteColor::kMaxComponent;
  int changed = 0;
  int worst = 0;
  for (int r = 0; r <= kMax; ++r) {
    for (int g = 0; g <= kMax; ++g) {
      for (int b = 0; b <= kMax; ++b) {
        const PaletteColor original(static_cast<uint8_t>(r),
                                    static_cast<uint8_t>(g),
                                    static_cast<uint8_t>(b));
        const PaletteColor color = original.ToHsv().ToPaletteColor();
        worst =
            std::max({worst, std::abs(color.red() - r),
                      std::abs(color.green() - g), std::abs(color.blue() - b)});
        if (color != original) {
          ++changed;
        }
      }
    }
  }
  EXPECT_EQ(worst, 4);
  EXPECT_EQ(changed, 9'966'339);
}

TEST(HsvColorTest, FullGunsReadAsFullValue) {
  EXPECT_EQ(Guns(63, 63, 63).ToHsv().value(), 255);
  EXPECT_EQ(Guns(0, 0, 63).ToHsv().hue(), 170);
}

}  // namespace
}  // namespace engine::gfx
