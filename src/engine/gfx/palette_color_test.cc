#include "engine/gfx/palette_color.h"

#include "gtest/gtest.h"

namespace engine::gfx {
namespace {

constexpr PaletteColor kOrange(252, 128, 0);
constexpr PaletteColor kTeal(0, 128, 252);

void ExpectSameColor(const PaletteColor& actual, const PaletteColor& expected) {
  EXPECT_EQ(actual.red(), expected.red());
  EXPECT_EQ(actual.green(), expected.green());
  EXPECT_EQ(actual.blue(), expected.blue());
}

TEST(PaletteColorFadedTowardTest, ZeroRatioKeepsTheColor) {
  ExpectSameColor(kOrange.FadedToward(0, kTeal), kOrange);
}

TEST(PaletteColorFadedTowardTest, FullRatioReachesTheTarget) {
  ExpectSameColor(kOrange.FadedToward(255, kTeal), kTeal);
}

TEST(PaletteColorFadedTowardTest, OutOfRangeRatioIsClamped) {
  ExpectSameColor(kOrange.FadedToward(-5, kTeal), kOrange);
  ExpectSameColor(kOrange.FadedToward(256, kTeal), kTeal);
}

TEST(PaletteColorFadedTowardTest, ComponentsRoundDownEitherWay) {
  // Red falls 252 -> 0: 252 * 155 / 255 = 153.2. Blue rises 0 -> 252:
  // 252 * 100 / 255 = 98.8. Green stays.
  ExpectSameColor(kOrange.FadedToward(100, kTeal), PaletteColor(153, 128, 98));
}

TEST(PaletteColorFadedTowardTest, ExactQuotientIsNotRoundedDown) {
  // 51 * 190 / 255 is exactly 38; a float lerp landed a hair under and
  // truncated to 37.
  constexpr PaletteColor color(51, 51, 51);
  ExpectSameColor(color.FadedToward(65, kBlackColor), PaletteColor(38, 38, 38));
}

TEST(PaletteColorFadedTowardTest, LeavesTheSourceUntouched) {
  constexpr PaletteColor source = kOrange;
  static_cast<void>(source.FadedToward(200, kTeal));
  ExpectSameColor(source, kOrange);
}

TEST(PaletteColorTest, KeepsEightBitComponents) {
  constexpr PaletteColor color(255, 130, 3);
  EXPECT_EQ(color.red(), 255);
  EXPECT_EQ(color.green(), 130);
  EXPECT_EQ(color.blue(), 3);
}

}  // namespace
}  // namespace engine::gfx
