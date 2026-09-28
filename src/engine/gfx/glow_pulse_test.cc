#include "engine/gfx/glow_pulse.h"

#include <cstdint>
#include <vector>

#include "engine/gfx/palette_color.h"
#include "gtest/gtest.h"

namespace {

struct FakeTick {
  static int64_t now;
  static int64_t Tick() { return now; }
};
int64_t FakeTick::now = 0;

constexpr int64_t kPeriod = 10;

class GlowPulseTest : public ::testing::Test {
 protected:
  void SetUp() override { FakeTick::now = 1000; }
};

TEST_F(GlowPulseTest, FirstUpdateStepsAtOnce) {
  GlowPulse<FakeTick> pulse(kPeriod);
  EXPECT_EQ(pulse.fade(), GlowPulse<FakeTick>::kMaxFade);
  EXPECT_TRUE(pulse.Update());
  EXPECT_EQ(pulse.fade(), 130);
}

TEST_F(GlowPulseTest, WaitsOutThePeriodBetweenSteps) {
  GlowPulse<FakeTick> pulse(kPeriod);
  ASSERT_TRUE(pulse.Update());

  FakeTick::now += kPeriod - 1;
  EXPECT_FALSE(pulse.Update());
  EXPECT_EQ(pulse.fade(), 130);

  FakeTick::now += 1;
  EXPECT_TRUE(pulse.Update());
  EXPECT_EQ(pulse.fade(), 110);
}

// The range is not a whole number of steps, so the clamp at the dark end
// shifts the brightening leg off the dimming leg's values.
TEST_F(GlowPulseTest, SwingsBetweenTheClampedEnds) {
  GlowPulse<FakeTick> pulse(kPeriod);
  std::vector<int> fades;
  for (int step = 0; step < 14; ++step) {
    ASSERT_TRUE(pulse.Update());
    fades.push_back(pulse.fade());
    FakeTick::now += kPeriod;
  }
  EXPECT_EQ(fades, (std::vector<int>{130, 110, 90, 70, 50, 32, 52, 72, 92, 112,
                                     132, 150, 130, 110}));
}

TEST_F(GlowPulseTest, ApplyDimsTowardBlackByTheCurrentFade) {
  GlowPulse<FakeTick> pulse(kPeriod);
  ASSERT_TRUE(pulse.Update());

  constexpr engine::gfx::PaletteColor kWhite(255, 255, 255);
  const engine::gfx::PaletteColor dimmed = pulse.Apply(kWhite);
  const engine::gfx::PaletteColor expected =
      kWhite.FadedToward(pulse.fade(), engine::gfx::kBlackColor);
  EXPECT_EQ(dimmed.red(), expected.red());
  EXPECT_LT(dimmed.red(), kWhite.red());
}

}  // namespace
