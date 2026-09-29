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
