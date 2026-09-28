#include "engine/gfx/hsv.h"

#include <array>
#include <cstdlib>

#include "engine/gfx/rgb.h"
#include "gtest/gtest.h"

namespace {

std::array<int, 3> RgbToHsv(int r, int g, int b) {
  int h = 0;
  int s = 0;
  int v = 0;
  Convert_RGB_To_HSV(r, g, b, &h, &s, &v);
  return {h, s, v};
}

std::array<int, 3> HsvToRgb(int h, int s, int v) {
  int r = 0;
  int g = 0;
  int b = 0;
  Convert_HSV_To_RGB(h, s, v, &r, &g, &b);
  return {r, g, b};
}

// Pinned against the unsigned implementation these replaced; Tiberian
// Dawn's options dialog builds its palette from them.
TEST(ConvertRgbToHsvTest, PinnedValues) {
  EXPECT_EQ(RgbToHsv(63, 0, 0), (std::array{255, 255, 255}));
  EXPECT_EQ(RgbToHsv(0, 63, 0), (std::array{85, 255, 255}));
  EXPECT_EQ(RgbToHsv(0, 0, 63), (std::array{170, 255, 255}));
  EXPECT_EQ(RgbToHsv(63, 32, 0), (std::array{22, 255, 255}));
  EXPECT_EQ(RgbToHsv(10, 40, 50), (std::array{138, 205, 202}));
  EXPECT_EQ(RgbToHsv(50, 5, 30), (std::array{231, 230, 202}));
}

TEST(ConvertRgbToHsvTest, GreyHasNoHueOrSaturation) {
  EXPECT_EQ(RgbToHsv(0, 0, 0), (std::array{0, 0, 0}));
  EXPECT_EQ(RgbToHsv(32, 32, 32), (std::array{0, 0, 130}));
  EXPECT_EQ(RgbToHsv(63, 63, 63), (std::array{0, 0, 255}));
}

TEST(ConvertHsvToRgbTest, PinnedValues) {
  EXPECT_EQ(HsvToRgb(0, 255, 255), (std::array{63, 0, 0}));
  EXPECT_EQ(HsvToRgb(85, 255, 255), (std::array{0, 63, 0}));
  EXPECT_EQ(HsvToRgb(170, 255, 255), (std::array{0, 0, 63}));
  EXPECT_EQ(HsvToRgb(30, 128, 200), (std::array{49, 42, 25}));
}

TEST(ConvertHsvToRgbTest, RoundTripIsWithinOneStep) {
  for (int r = 0; r < 64; ++r) {
    for (int g = 0; g < 64; ++g) {
      for (int b = 0; b < 64; ++b) {
        const auto [h, s, v] = RgbToHsv(r, g, b);
        const auto [red, green, blue] = HsvToRgb(h, s, v);
        ASSERT_LE(std::abs(red - r), 1) << r << "," << g << "," << b;
        ASSERT_LE(std::abs(green - g), 1) << r << "," << g << "," << b;
        ASSERT_LE(std::abs(blue - b), 1) << r << "," << g << "," << b;
      }
    }
  }
}

TEST(HsvClassTest, PrimariesConvertToFullGuns) {
  const PaletteColor red = HsvColor(0, 255, 255);
  EXPECT_EQ(red.Red_Component(), 252);
  EXPECT_EQ(red.Green_Component(), 0);
  EXPECT_EQ(red.Blue_Component(), 0);
  const PaletteColor blue = HsvColor(170, 255, 255);
  EXPECT_EQ(blue.Red_Component(), 0);
  EXPECT_EQ(blue.Green_Component(), 0);
  EXPECT_EQ(blue.Blue_Component(), 252);
}

}  // namespace
