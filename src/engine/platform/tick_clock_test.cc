#include "engine/platform/tick_clock.h"

#include <cstdint>

#include "engine/platform/sleep.h"
#include "gtest/gtest.h"

namespace {

TEST(TickClockTest, NeverRunsBackward) {
  int64_t previous = SystemTicks();
  for (int i = 0; i < 1000; ++i) {
    const int64_t now = SystemTicks();
    EXPECT_GE(now, previous);
    previous = now;
  }
}

TEST(TickClockTest, CountsSixtyTicksPerSecond) {
  // 100 ms is 6 ticks; flooring both readings can lose one. Only a lower bound
  // is checked, since a loaded machine may sleep much longer.
  const int64_t start = SystemTicks();
  engine::platform::SleepMs(100);
  EXPECT_GE(SystemTicks() - start, 5);
}

TEST(TickClockTest, MillisecondsNeverRunBackward) {
  int64_t previous = Get_Time_Ms();
  for (int i = 0; i < 1000; ++i) {
    const int64_t now = Get_Time_Ms();
    EXPECT_GE(now, previous);
    previous = now;
  }
}

TEST(TickClockTest, CountsMilliseconds) {
  // Only a lower bound is checked, since a loaded machine may sleep longer.
  const int64_t start = Get_Time_Ms();
  engine::platform::SleepMs(100);
  EXPECT_GE(Get_Time_Ms() - start, 100);
}

}  // namespace
