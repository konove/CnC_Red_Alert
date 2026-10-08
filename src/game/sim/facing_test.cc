#include "game/sim/facing.h"

#include "game/sim/direction.h"
#include "gtest/gtest.h"

namespace game {
namespace {

TEST(HeadingTest, StartsAtNorthAndStill) {
  const Heading facing;
  EXPECT_EQ(facing.Current(), Direction::kNorth);
  EXPECT_EQ(facing.Desired(), Direction::kNorth);
  EXPECT_FALSE(facing.Is_Rotating());
}

TEST(HeadingTest, SettersReportWhetherTheValueChanged) {
  Heading facing(Direction::kEast);
  EXPECT_FALSE(facing.Set_Desired(Direction::kEast));
  EXPECT_TRUE(facing.Set_Desired(Direction::kSouth));
  EXPECT_FALSE(facing.Set_Current(Direction::kEast));
  EXPECT_TRUE(facing.Set_Current(Direction::kWest));
}

TEST(HeadingTest, DifferenceTakesTheShortWayRound) {
  Heading facing(Direction::kNorth);
  facing.Set_Desired(Direction::kEast);
  EXPECT_EQ(facing.Difference(), 64);
  facing.Set_Desired(Direction::kWest);
  EXPECT_EQ(facing.Difference(), -64);
  EXPECT_EQ(facing.Difference(Direction(200)), -56);
}

TEST(HeadingTest, RotatesClockwiseByTheRate) {
  Heading facing(Direction::kNorth);
  facing.Set_Desired(Direction::kEast);
  EXPECT_TRUE(facing.Rotation_Adjust(16));
  EXPECT_EQ(facing.Current(), Direction(16));
  EXPECT_TRUE(facing.Is_Rotating());
}

TEST(HeadingTest, RotatesCounterclockwiseTowardACloserTarget) {
  Heading facing(Direction::kNorth);
  facing.Set_Desired(Direction::kWest);
  facing.Rotation_Adjust(16);
  EXPECT_EQ(facing.Current(), Direction(240));
}

TEST(HeadingTest, SnapsWhenTheRateCoversTheGap) {
  Heading facing(Direction::kNorth);
  facing.Set_Desired(Direction::kEast);
  facing.Rotation_Adjust(100);
  EXPECT_EQ(facing.Current(), Direction::kEast);
  EXPECT_FALSE(facing.Is_Rotating());
  EXPECT_FALSE(facing.Rotation_Adjust(100));
}

TEST(HeadingTest, ReportsNoRedrawInsideOneFrame) {
  Heading facing(Direction::kNorth);
  facing.Set_Desired(Direction::kEast);
  // Directions 0..4 all show body frame 0 of 32.
  EXPECT_FALSE(facing.Rotation_Adjust(2));
  EXPECT_EQ(facing.Current(), Direction(2));
}

}  // namespace
}  // namespace game
