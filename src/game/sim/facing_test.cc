#include "game/sim/facing.h"

#include "game/sim/direction.h"
#include "gtest/gtest.h"

namespace game {
namespace {

TEST(HeadingTest, StartsAtNorthAndStill) {
  const Heading heading;
  EXPECT_EQ(heading.current(), Direction::kNorth);
  EXPECT_EQ(heading.desired(), Direction::kNorth);
  EXPECT_FALSE(heading.IsRotating());
}

TEST(HeadingTest, ReadsAndAssignsAsItsCurrentDirection) {
  Heading heading = Direction::kEast;
  heading.set_desired(Direction::kSouth);
  const Direction direction = heading;
  EXPECT_EQ(direction, Direction::kEast);
  EXPECT_TRUE(heading == Direction::kEast);

  heading = Direction::kWest;
  EXPECT_EQ(heading.current(), Direction::kWest);
  EXPECT_EQ(heading.desired(), Direction::kWest);
}

TEST(HeadingTest, EachSetterLeavesTheOtherDirectionAlone) {
  Heading heading(Direction::kEast);
  heading.set_desired(Direction::kSouth);
  EXPECT_EQ(heading.current(), Direction::kEast);
  EXPECT_EQ(heading.desired(), Direction::kSouth);
  heading.set_current(Direction::kWest);
  EXPECT_EQ(heading.current(), Direction::kWest);
  EXPECT_EQ(heading.desired(), Direction::kSouth);
}

TEST(HeadingTest, SnapToLeavesNothingToTurn) {
  Heading heading(Direction::kEast);
  heading.set_desired(Direction::kSouth);
  heading.SnapTo(Direction::kWest);
  EXPECT_EQ(heading.current(), Direction::kWest);
  EXPECT_FALSE(heading.IsRotating());
}

TEST(HeadingTest, TurnsAreMeasuredTheShortWayRound) {
  Heading heading(Direction::kNorth);
  heading.set_desired(Direction::kEast);
  EXPECT_EQ(heading.RemainingTurn(), 64);
  heading.set_desired(Direction::kWest);
  EXPECT_EQ(heading.RemainingTurn(), -64);
  EXPECT_EQ(heading.ShortestTurnTo(Direction(200)), -56);
}

TEST(HeadingTest, RotatesClockwiseByTheRate) {
  Heading heading(Direction::kNorth);
  heading.set_desired(Direction::kEast);
  EXPECT_TRUE(heading.Turn(16));
  EXPECT_EQ(heading.current(), Direction(16));
  EXPECT_TRUE(heading.IsRotating());
}

TEST(HeadingTest, RotatesCounterclockwiseTowardACloserTarget) {
  Heading heading(Direction::kNorth);
  heading.set_desired(Direction::kWest);
  heading.Turn(16);
  EXPECT_EQ(heading.current(), Direction(240));
}

TEST(HeadingTest, SnapsWhenTheRateCoversTheGap) {
  Heading heading(Direction::kNorth);
  heading.set_desired(Direction::kEast);
  heading.Turn(100);
  EXPECT_EQ(heading.current(), Direction::kEast);
  EXPECT_FALSE(heading.IsRotating());
  EXPECT_FALSE(heading.Turn(100));
}

TEST(HeadingTest, AStepThatExactlyCoversTheGapArrives) {
  Heading heading(Direction::kNorth);
  heading.set_desired(Direction(16));
  heading.Turn(16);
  EXPECT_EQ(heading.current(), Direction(16));
  EXPECT_FALSE(heading.IsRotating());
}

TEST(HeadingTest, TheFastestTurnNeedsTwoCallsToFaceAbout) {
  Heading heading(Direction::kNorth);
  heading.set_desired(Direction::kSouth);
  // South is 128 steps away, one more than the 127 a call may take, and the
  // tie between the two ways round goes anticlockwise.
  EXPECT_EQ(heading.RemainingTurn(), -128);
  heading.Turn(1000);
  EXPECT_EQ(heading.current(), Direction(129));
  heading.Turn(1000);
  EXPECT_EQ(heading.current(), Direction::kSouth);
}

TEST(HeadingTest, ReportsNoRedrawInsideOneFrame) {
  Heading heading(Direction::kNorth);
  heading.set_desired(Direction::kEast);
  // Directions 0..4 all show body frame 0 of 32.
  EXPECT_FALSE(heading.Turn(2));
  EXPECT_EQ(heading.current(), Direction(2));
}

}  // namespace
}  // namespace game
