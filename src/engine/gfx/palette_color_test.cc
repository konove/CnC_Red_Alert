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

TEST(ExpandGunTest, CopiesTheTopBitsDown) {
  EXPECT_EQ(ExpandGun(0), 0);
  EXPECT_EQ(ExpandGun(1), 4);
  EXPECT_EQ(ExpandGun(16), 65);
  EXPECT_EQ(ExpandGun(48), 195);
  EXPECT_EQ(ExpandGun(63), 255);
}

TEST(PaletteColorTest, FullGunsReadAsFullComponents) {
  const PaletteColor white = PaletteColor::FromGuns(63, 63, 63);
  EXPECT_EQ(white.red(), 255);
  EXPECT_EQ(white.green(), 255);
  EXPECT_EQ(white.blue(), 255);
}

TEST(PaletteColorTest, ComponentsUseTheDisplaysExpansion) {
  for (int gun = 0; gun <= PaletteColor::kMaxGun; ++gun) {
    EXPECT_EQ(PaletteColor::FromGuns(gun, gun, gun).red(), ExpandGun(gun));
  }
}

}  // namespace
}  // namespace engine::gfx
