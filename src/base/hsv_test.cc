#include "base/hsv.h"

#include <gtest/gtest.h>

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
    const Rgb8 color = HsvToRgb8(hue, 0, 200);
    EXPECT_EQ(color.red, 200) << "hue " << hue;
    EXPECT_EQ(color.green, 200) << "hue " << hue;
    EXPECT_EQ(color.blue, 200) << "hue " << hue;
  }
}

TEST(HsvToRgb8Test, ZeroValueProducesBlack) {
  const Rgb8 color = HsvToRgb8(120, 255, 0);
  EXPECT_EQ(color.red, 0);
  EXPECT_EQ(color.green, 0);
  EXPECT_EQ(color.blue, 0);
}

TEST(HsvToRgb8Test, PrimaryHues) {
  // Fully saturated, full value: one gun at maximum, the others at zero.
  const Rgb8 red = HsvToRgb8(0, 255, 255);
  EXPECT_EQ(red.red, 255);
  EXPECT_EQ(red.green, 0);
  EXPECT_EQ(red.blue, 0);

  // Hue wraps a full circle over [0, 255], so the maximum hue is red again.
  const Rgb8 wrapped = HsvToRgb8(255, 255, 255);
  EXPECT_EQ(wrapped.red, 255);
  EXPECT_EQ(wrapped.green, 0);
  EXPECT_EQ(wrapped.blue, 0);

  // One third and two thirds around the circle are green and blue.
  const Rgb8 green = HsvToRgb8(85, 255, 255);
  EXPECT_GT(green.green, green.red);
  EXPECT_GT(green.green, green.blue);

  const Rgb8 blue = HsvToRgb8(170, 255, 255);
  EXPECT_GT(blue.blue, blue.red);
  EXPECT_GT(blue.blue, blue.green);
}

TEST(HsvToRgb8Test, ClampsOutOfRangeInputs) {
  // Out-of-range inputs must not index past the sextant table. The historical
  // Convert_HSV_To_RGB read out of bounds for any hue above the maximum.
  EXPECT_EQ(HsvToRgb8(1000, 255, 255).red, HsvToRgb8(255, 255, 255).red);
  EXPECT_EQ(HsvToRgb8(-50, 255, 255).red, HsvToRgb8(0, 255, 255).red);

  const Rgb8 clamped = HsvToRgb8(0, 9999, 9999);
  EXPECT_EQ(clamped.red, HsvToRgb8(0, 255, 255).red);
  EXPECT_EQ(clamped.green, HsvToRgb8(0, 255, 255).green);
  EXPECT_EQ(clamped.blue, HsvToRgb8(0, 255, 255).blue);
}

TEST(HsvToRgb8Test, AllOutputsStayInRange) {
  for (int hue = 0; hue <= 255; ++hue) {
    for (int saturation = 0; saturation <= 255; saturation += 5) {
      for (int value = 0; value <= 255; value += 5) {
        const Rgb8 color = HsvToRgb8(hue, saturation, value);
        ASSERT_GE(color.red, 0);
        ASSERT_LE(color.red, 255);
        ASSERT_GE(color.green, 0);
        ASSERT_LE(color.green, 255);
        ASSERT_GE(color.blue, 0);
        ASSERT_LE(color.blue, 255);
      }
    }
  }
}

TEST(HsvToRgb8Test, ValueBoundsTheBrightestGun) {
  // The dominant gun always equals the value; nothing may exceed it.
  for (int hue = 0; hue <= 255; hue += 7) {
    for (int saturation = 0; saturation <= 255; saturation += 15) {
      const Rgb8 color = HsvToRgb8(hue, saturation, 180);
      const int brightest =
          std::max({color.red, color.green, color.blue});
      EXPECT_EQ(brightest, 180) << "hue " << hue << " sat " << saturation;
    }
  }
}

}  // namespace
}  // namespace base
