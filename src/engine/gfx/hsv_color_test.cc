#include "engine/gfx/hsv_color.h"

#include <array>
#include <cstdint>
#include <cstdlib>

#include "engine/gfx/palette_color.h"
#include "gtest/gtest.h"

namespace engine::gfx {
namespace {

std::array<int, 3> HsvToGuns(const int h, const int s, const int v) {
  const PaletteColor color =
      HsvColor(static_cast<uint8_t>(h), static_cast<uint8_t>(s),
               static_cast<uint8_t>(v))
          .ToPaletteColor();
  return {color.red_gun(), color.green_gun(), color.blue_gun()};
}

std::array<int, 3> GunsToHsv(const int r, const int g, const int b) {
  const HsvColor hsv = PaletteColor::FromGuns(r, g, b).ToHsv();
  return {hsv.hue(), hsv.saturation(), hsv.value()};
}

// Pinned against Tiberian Dawn's original conversions, which these reproduce
// exactly; its options dialog builds its palette from them.
TEST(HsvColorTest, ToHsvPinnedValues) {
  EXPECT_EQ(GunsToHsv(63, 0, 0), (std::array{255, 255, 255}));
  EXPECT_EQ(GunsToHsv(0, 63, 0), (std::array{85, 255, 255}));
  EXPECT_EQ(GunsToHsv(63, 32, 0), (std::array{22, 255, 255}));
  EXPECT_EQ(GunsToHsv(10, 40, 50), (std::array{138, 205, 202}));
  EXPECT_EQ(GunsToHsv(32, 32, 32), (std::array{0, 0, 130}));
}

TEST(HsvColorTest, ToPaletteColorPinnedValues) {
  EXPECT_EQ(HsvToGuns(0, 255, 255), (std::array{63, 0, 0}));
  EXPECT_EQ(HsvToGuns(85, 255, 255), (std::array{0, 63, 0}));
  EXPECT_EQ(HsvToGuns(170, 255, 255), (std::array{0, 0, 63}));
  EXPECT_EQ(HsvToGuns(30, 128, 200), (std::array{49, 42, 25}));
}

TEST(HsvColorTest, PrimariesConvertToFullGuns) {
  const PaletteColor red = HsvColor(0, 255, 255).ToPaletteColor();
  EXPECT_EQ(red.red(), 252);
  EXPECT_EQ(red.green(), 0);
  EXPECT_EQ(red.blue(), 0);
  const PaletteColor blue = HsvColor(170, 255, 255).ToPaletteColor();
  EXPECT_EQ(blue.red(), 0);
  EXPECT_EQ(blue.green(), 0);
  EXPECT_EQ(blue.blue(), 252);
}

TEST(HsvColorTest, PaletteRoundTripIsWithinOneStep) {
  int changed = 0;
  for (int r = 0; r <= PaletteColor::kMaxGun; ++r) {
    for (int g = 0; g <= PaletteColor::kMaxGun; ++g) {
      for (int b = 0; b <= PaletteColor::kMaxGun; ++b) {
        const PaletteColor color =
            PaletteColor::FromGuns(r, g, b).ToHsv().ToPaletteColor();
        ASSERT_LE(std::abs(color.red_gun() - r), 1)
            << r << "," << g << "," << b;
        ASSERT_LE(std::abs(color.green_gun() - g), 1)
            << r << "," << g << "," << b;
        ASSERT_LE(std::abs(color.blue_gun() - b), 1)
            << r << "," << g << "," << b;
        if (color.SquaredDistance(PaletteColor::FromGuns(r, g, b)) != 0) {
          ++changed;
        }
      }
    }
  }
  // The residue of the 8-bit HSV grid; truncating either direction instead of
  // rounding would raise it (127,769 when both truncated).
  EXPECT_EQ(changed, 21'612);
}

TEST(HsvColorTest, FullGunsReadAsFullValue) {
  EXPECT_EQ(PaletteColor::FromGuns(63, 63, 63).ToHsv().value(), 255);
  EXPECT_EQ(PaletteColor::FromGuns(0, 0, 63).ToHsv().hue(), 170);
}

}  // namespace
}  // namespace engine::gfx
