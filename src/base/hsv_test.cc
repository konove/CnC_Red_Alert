#include "base/hsv.h"

#include <algorithm>

#include "gtest/gtest.h"

namespace base {
namespace {

TEST(DivideWithRoundTest, RoundsHalvesUp) {
  EXPECT_EQ(DivideWithRound(0, 255), 0);
  EXPECT_EQ(DivideWithRound(127, 255), 0);  // Just below half.
  EXPECT_EQ(DivideWithRound(128, 255), 1);  // Exactly half rounds up.
  EXPECT_EQ(DivideWithRound(255, 255), 1);
  EXPECT_EQ(DivideWithRound(3, 2), 2);
  EXPECT_EQ(DivideWithRound(10, 5), 2);  // Exact division is unchanged.
}

TEST(DivideWithRoundTest, IsUsableInConstantExpressions) {
  static_assert(DivideWithRound(128, 255) == 1);
  static_assert(DivideWithRound(127, 255) == 0);
}

TEST(HsvToRgb8Test, ZeroSaturationProducesGrey) {
  // With no saturation every gun collapses to the value, whatever the hue.
  for (int hue = 0; hue <= 255; hue += 17) {
    const auto [red, green, blue] = HsvToRgb8(hue, 0, 200);
    EXPECT_EQ(red, 200) << "hue " << hue;
    EXPECT_EQ(green, 200) << "hue " << hue;
    EXPECT_EQ(blue, 200) << "hue " << hue;
  }
}

TEST(HsvToRgb8Test, ZeroValueProducesBlack) {
  const auto [red, green, blue] = HsvToRgb8(120, 255, 0);
  EXPECT_EQ(red, 0);
  EXPECT_EQ(green, 0);
  EXPECT_EQ(blue, 0);
}

TEST(HsvToRgb8Test, HueZeroIsRed) {
  // Fully saturated, full value: one gun at maximum, the others at zero.
  const auto [red, green, blue] = HsvToRgb8(0, 255, 255);
  EXPECT_EQ(red, 255);
  EXPECT_EQ(green, 0);
  EXPECT_EQ(blue, 0);
}

TEST(HsvToRgb8Test, MaximumHueWrapsToRed) {
  // Hue wraps a full circle over [0, 255], so the maximum hue is red again.
  const auto [red, green, blue] = HsvToRgb8(255, 255, 255);
  EXPECT_EQ(red, 255);
  EXPECT_EQ(green, 0);
  EXPECT_EQ(blue, 0);
}

TEST(HsvToRgb8Test, HueOneThirdAroundIsGreen) {
  const auto [red, green, blue] = HsvToRgb8(85, 255, 255);
  EXPECT_GT(green, red);
  EXPECT_GT(green, blue);
}

TEST(HsvToRgb8Test, HueTwoThirdsAroundIsBlue) {
  const auto [red, green, blue] = HsvToRgb8(170, 255, 255);
  EXPECT_GT(blue, red);
  EXPECT_GT(blue, green);
}

TEST(HsvToRgb8Test, ClampsOutOfRangeInputs) {
  // Out-of-range inputs must not index past the sextant table. The historical
  // Convert_HSV_To_RGB read out of bounds for any hue above the maximum.
  EXPECT_EQ(HsvToRgb8(1000, 255, 255).red, HsvToRgb8(255, 255, 255).red);
  EXPECT_EQ(HsvToRgb8(-50, 255, 255).red, HsvToRgb8(0, 255, 255).red);

  const auto [red, green, blue] = HsvToRgb8(0, 9999, 9999);
  EXPECT_EQ(red, HsvToRgb8(0, 255, 255).red);
  EXPECT_EQ(green, HsvToRgb8(0, 255, 255).green);
  EXPECT_EQ(blue, HsvToRgb8(0, 255, 255).blue);
}

TEST(HsvToRgb8Test, AllOutputsStayInRange) {
  for (int hue = 0; hue <= 255; ++hue) {
    for (int saturation = 0; saturation <= 255; saturation += 5) {
      for (int value = 0; value <= 255; value += 5) {
        const auto [red, green, blue] = HsvToRgb8(hue, saturation, value);
        ASSERT_GE(red, 0);
        ASSERT_LE(red, 255);
        ASSERT_GE(green, 0);
        ASSERT_LE(green, 255);
        ASSERT_GE(blue, 0);
        ASSERT_LE(blue, 255);
      }
    }
  }
}

TEST(HsvToRgb8Test, ValueBoundsTheBrightestGun) {
  // The dominant gun always equals the value; nothing may exceed it.
  for (int hue = 0; hue <= 255; hue += 7) {
    for (int saturation = 0; saturation <= 255; saturation += 15) {
      const auto [red, green, blue] = HsvToRgb8(hue, saturation, 180);
      const int brightest = std::max({red, green, blue});
      EXPECT_EQ(brightest, 180) << "hue " << hue << " sat " << saturation;
    }
  }
}

}  // namespace
}  // namespace base
