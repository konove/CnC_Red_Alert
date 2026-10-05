#include "engine/platform/manual_clock.h"

#include <cstdint>

#include "engine/platform/sleep.h"
#include "engine/platform/timer.h"
#include "gtest/gtest.h"

namespace engine::platform {
namespace {

TEST(ManualClockTest, TheSystemClockReadsItWhileItExists) {
  const ManualClock clock;

  EXPECT_EQ(SystemTicks(), 0);
  EXPECT_EQ(SystemMilliseconds(), 0);
}

TEST(ManualClockTest, TimeMovesOnlyWhenAdvanced) {
  ManualClock clock;

  clock.Advance(60);
  EXPECT_EQ(SystemTicks(), 60);
  EXPECT_EQ(SystemMilliseconds(), 1000);

  clock.Advance(1);
  EXPECT_EQ(SystemTicks(), 61);
  EXPECT_EQ(SystemMilliseconds(), 1016);
}

TEST(ManualClockTest, ATimerRunsOutWhenTheClockIsAdvancedPastIt) {
  ManualClock clock;
  const Timer<SystemTickSource> wait{3};

  clock.Advance(2);
  EXPECT_TRUE(wait.HasTimeLeft());

  clock.Advance(1);
  EXPECT_FALSE(wait.HasTimeLeft());
}

TEST(ManualClockTest, TheRealClockComesBackWhenItIsGone) {
  const int64_t before = SystemTicks();
  {
    const ManualClock clock;
    ASSERT_EQ(SystemTicks(), 0);
  }

  // The real clock counts from boot, far past the manual clock's zero.
  EXPECT_GE(SystemTicks(), before);
  EXPECT_GT(SystemMilliseconds(), 0);
}

TEST(ManualClockTest, MillisecondsAndTicksAddUpWithoutDrift) {
  ManualClock clock;

  // 50 ms is exactly 3 ticks; a clock kept in whole ticks would lose each
  // millisecond as it came.
  for (int i = 0; i < 50; ++i) {
    clock.AdvanceMilliseconds(1);
  }
  EXPECT_EQ(SystemTicks(), 3);
  EXPECT_EQ(SystemMilliseconds(), 50);

  clock.Advance(1);
  EXPECT_EQ(SystemMilliseconds(), 66);
}

TEST(ManualClockTest, SleepingMovesItInsteadOfWaiting) {
  const ManualClock clock;

  // An hour, which the test would not survive sleeping through.
  SleepMs(3'600'000);

  EXPECT_EQ(clock.milliseconds(), 3'600'000);
  EXPECT_EQ(clock.ticks(), 216'000);
}

TEST(ManualClockTest, ASleepOfNoTimeLeavesItAlone) {
  const ManualClock clock;

  SleepMs(0);
  SleepMs(-5);

  EXPECT_EQ(clock.milliseconds(), 0);
}

// A scripted run installs its clock over whatever a fixture installed.
TEST(ManualClockTest, TheInnerOfTwoClocksIsTheOneRead) {
  ManualClock outer;
  outer.Advance(5);
  {
    const ManualClock inner;
    EXPECT_EQ(SystemTicks(), 0);
  }

  EXPECT_EQ(SystemTicks(), 5);
}

}  // namespace
}  // namespace engine::platform
