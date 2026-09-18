#include "ra/count_up.h"

#include "gtest/gtest.h"

namespace {

TEST(CountUpValueTest, RunsFromZeroToTheFinalValue) {
  EXPECT_EQ(CountUpValue(80, 0, 100), 0);
  EXPECT_EQ(CountUpValue(80, 50, 100), 40);
  EXPECT_EQ(CountUpValue(80, 100, 100), 80);
}

// The casualty graph: 50 losses drawn as a 59-pixel bar next to a 118-pixel
// one. The counter has to arrive at 50 when its own bar is complete.
TEST(CountUpValueTest, ReachesTheFinalValueOnTheLastStepOfAShortRun) {
  EXPECT_EQ(CountUpValue(50, 59, 59), 50);
  EXPECT_EQ(CountUpValue(50, 58, 59), 49);
}

TEST(CountUpValueTest, NeverDecreases) {
  int previous = 0;
  for (int step = 0; step <= 118; step++) {
    const int value = CountUpValue(7, step, 118);
    EXPECT_GE(value, previous);
    previous = value;
  }
  EXPECT_EQ(previous, 7);
}

TEST(CountUpValueTest, ClampsStepsOutsideTheRun) {
  EXPECT_EQ(CountUpValue(80, -30, 100), 0);
  EXPECT_EQ(CountUpValue(80, 130, 100), 80);
}

TEST(CountUpValueTest, EmptyRunIsAlreadyFinal) {
  EXPECT_EQ(CountUpValue(80, 0, 0), 80);
}

TEST(CountUpValueTest, NegativeFinalValue) {
  EXPECT_EQ(CountUpValue(-9999, 100, 100), -9999);
}

TEST(CountUpValueTest, LargeValuesDoNotOverflow) {
  EXPECT_EQ(CountUpValue(2'000'000'000, 99, 100), 1'980'000'000);
}

}  // namespace
