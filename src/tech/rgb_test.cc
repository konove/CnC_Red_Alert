#include "tech/rgb.h"

#include <gtest/gtest.h>

namespace {

constexpr RGBClass kOrange(252, 128, 0);
constexpr RGBClass kTeal(0, 128, 252);

void ExpectSameColor(const RGBClass& actual, const RGBClass& expected) {
  EXPECT_EQ(actual.Red_Component(), expected.Red_Component());
  EXPECT_EQ(actual.Green_Component(), expected.Green_Component());
  EXPECT_EQ(actual.Blue_Component(), expected.Blue_Component());
}

TEST(RgbAdjustedTest, ZeroRatioKeepsTheColor) {
  ExpectSameColor(kOrange.Adjusted(0, kTeal), kOrange);
}

TEST(RgbAdjustedTest, FullRatioReachesTheTarget) {
  ExpectSameColor(kOrange.Adjusted(255, kTeal), kTeal);
}

TEST(RgbAdjustedTest, OutOfRangeRatioIsClamped) {
  ExpectSameColor(kOrange.Adjusted(-5, kTeal), kOrange);
  ExpectSameColor(kOrange.Adjusted(256, kTeal), kTeal);
}

TEST(RgbAdjustedTest, MatchesInPlaceAdjust) {
  RGBClass in_place = kOrange;
  in_place.Adjust(100, kTeal);
  ExpectSameColor(kOrange.Adjusted(100, kTeal), in_place);
}

TEST(RgbAdjustedTest, LeavesTheSourceUntouched) {
  const RGBClass source = kOrange;
  static_cast<void>(source.Adjusted(200, kTeal));
  ExpectSameColor(source, kOrange);
}

}  // namespace
