// Tests for the direction-based movement helpers.

#include "base/trig.h"

#include <cstdint>

#include "base/point.h"
#include "gtest/gtest.h"

namespace {

// Game directions: 0 is north, 64 east, 128 south, 192 west.
constexpr uint8_t kNorth = 0;
constexpr uint8_t kEast = 64;
constexpr uint8_t kSouth = 128;
constexpr uint8_t kWest = 192;

TEST(MoveOffsetTest, FollowsTheCompassWithYGrowingDownward) {
  EXPECT_EQ(base::MoveOffset(kNorth, 100), (base::Point{0, -100}));
  EXPECT_EQ(base::MoveOffset(kEast, 100), (base::Point{100, 0}));
  EXPECT_EQ(base::MoveOffset(kSouth, 100), (base::Point{0, 100}));
  EXPECT_EQ(base::MoveOffset(kWest, 100), (base::Point{-100, 0}));
}

TEST(MoveOffsetTest, DoesNotWrapAtSixteenBits) {
  EXPECT_EQ(base::MoveOffset(kEast, 40000), (base::Point{40000, 0}));
  EXPECT_EQ(base::MoveOffset(kWest, 40000), (base::Point{-40000, 0}));
}

TEST(MovePointTest, AddsTheOffsetToThePoint) {
  int16_t x = 500;
  int16_t y = 300;

  base::MovePoint(x, y, kWest, 100);
  base::MovePoint(x, y, kSouth, 50);

  EXPECT_EQ(x, 400);
  EXPECT_EQ(y, 350);
}

TEST(MovePointTest, WrapsAtSixteenBits) {
  int16_t x = 100;
  int16_t y = 0;

  base::MovePoint(x, y, kWest, 500);

  EXPECT_EQ(x, -400);
}

}  // namespace
