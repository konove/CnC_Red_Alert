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

TEST(HeadingTest, SettersReportWhetherTheValueChanged) {
  Heading heading(Direction::kEast);
  EXPECT_FALSE(heading.set_desired(Direction::kEast));
  EXPECT_TRUE(heading.set_desired(Direction::kSouth));
  EXPECT_FALSE(heading.set_current(Direction::kEast));
  EXPECT_TRUE(heading.set_current(Direction::kWest));
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

TEST(HeadingTest, ReportsNoRedrawInsideOneFrame) {
  Heading heading(Direction::kNorth);
  heading.set_desired(Direction::kEast);
  // Directions 0..4 all show body frame 0 of 32.
  EXPECT_FALSE(heading.Turn(2));
  EXPECT_EQ(heading.current(), Direction(2));
}

}  // namespace
}  // namespace game
