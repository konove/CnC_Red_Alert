#include "port/sleep.h"

#include <chrono>

#include "gtest/gtest.h"

namespace {

TEST(SleepTest, WaitsAtLeastTheRequestedTime) {
  const auto start = std::chrono::steady_clock::now();
  port::SleepMs(20);
  const auto elapsed = std::chrono::steady_clock::now() - start;

  // Only the lower bound is asserted. How much later the scheduler gets around
  // to us is not ours to promise, so there is no upper bound to make flaky.
  EXPECT_GE(elapsed, std::chrono::milliseconds(20));
}

TEST(SleepTest, NonPositiveDurationsReturnImmediately) {
  const auto start = std::chrono::steady_clock::now();
  port::SleepMs(0);
  port::SleepMs(-5);
  const auto elapsed = std::chrono::steady_clock::now() - start;

  EXPECT_LT(elapsed, std::chrono::seconds(1));
}

}  // namespace
