#include "engine/window/palette_fade.h"

#include <vector>

#include "engine/gfx/palette.h"
#include "engine/gfx/palette_color.h"
#include "gtest/gtest.h"

namespace engine::window {
namespace {

std::vector<gfx::Palette>& Shown() {
  static std::vector<gfx::Palette> shown;
  return shown;
}
int& Steps() {
  static int steps = 0;
  return steps;
}
void RecordShow(const gfx::Palette& palette) { Shown().push_back(palette); }
void CountStep() { ++Steps(); }

class PaletteFadeTest : public testing::Test {
 protected:
  void SetUp() override {
    Shown().clear();
    Steps() = 0;
  }

  const gfx::Palette black_{gfx::kBlackColor};
  const gfx::Palette white_{gfx::PaletteColor(255, 255, 255)};
};

TEST_F(PaletteFadeTest, ZeroTicksShowsNothing) {
  FadePalette(black_, white_, 0, RecordShow, CountStep);
  EXPECT_TRUE(Shown().empty());
  EXPECT_EQ(Steps(), 0);
}

TEST_F(PaletteFadeTest, FadeToTheSamePaletteShowsNothing) {
  FadePalette(white_, white_, 30, RecordShow, CountStep);
  EXPECT_TRUE(Shown().empty());
  EXPECT_EQ(Steps(), 0);
}

TEST_F(PaletteFadeTest, StepsOncePerTickAndEndsOnTheTarget) {
  constexpr int kTicks = 3;
  FadePalette(black_, white_, kTicks, RecordShow, CountStep);
  ASSERT_FALSE(Shown().empty());
  EXPECT_EQ(Shown().back(), white_);
  EXPECT_EQ(Steps(), static_cast<int>(Shown().size()));
  // One step per tick change: at most one for each of ticks 0..kTicks, more
  // only if the clock skipped none.
  EXPECT_LE(Steps(), kTicks + 1);
}

}  // namespace
}  // namespace engine::window
