#include "engine/platform/manual_clock.h"

#include <cstdint>

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
