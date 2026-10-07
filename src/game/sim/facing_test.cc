#include "game/sim/facing.h"

#include "game/sim/direction.h"
#include "gtest/gtest.h"

namespace game {
namespace {

TEST(FacingClassTest, StartsAtNorthAndStill) {
  const FacingClass facing;
  EXPECT_EQ(facing.Current(), DirType::kNorth);
  EXPECT_EQ(facing.Desired(), DirType::kNorth);
  EXPECT_FALSE(facing.Is_Rotating());
}

TEST(FacingClassTest, SettersReportWhetherTheValueChanged) {
  FacingClass facing(DirType::kEast);
  EXPECT_FALSE(facing.Set_Desired(DirType::kEast));
  EXPECT_TRUE(facing.Set_Desired(DirType::kSouth));
  EXPECT_FALSE(facing.Set_Current(DirType::kEast));
  EXPECT_TRUE(facing.Set_Current(DirType::kWest));
}

TEST(FacingClassTest, DifferenceTakesTheShortWayRound) {
  FacingClass facing(DirType::kNorth);
  facing.Set_Desired(DirType::kEast);
  EXPECT_EQ(facing.Difference(), 64);
  facing.Set_Desired(DirType::kWest);
  EXPECT_EQ(facing.Difference(), -64);
  EXPECT_EQ(facing.Difference(DirType(200)), -56);
}

TEST(FacingClassTest, RotatesClockwiseByTheRate) {
  FacingClass facing(DirType::kNorth);
  facing.Set_Desired(DirType::kEast);
  EXPECT_TRUE(facing.Rotation_Adjust(16));
  EXPECT_EQ(facing.Current(), DirType(16));
  EXPECT_TRUE(facing.Is_Rotating());
}

TEST(FacingClassTest, RotatesCounterclockwiseTowardACloserTarget) {
  FacingClass facing(DirType::kNorth);
  facing.Set_Desired(DirType::kWest);
  facing.Rotation_Adjust(16);
  EXPECT_EQ(facing.Current(), DirType(240));
}

TEST(FacingClassTest, SnapsWhenTheRateCoversTheGap) {
  FacingClass facing(DirType::kNorth);
  facing.Set_Desired(DirType::kEast);
  facing.Rotation_Adjust(100);
  EXPECT_EQ(facing.Current(), DirType::kEast);
  EXPECT_FALSE(facing.Is_Rotating());
  EXPECT_FALSE(facing.Rotation_Adjust(100));
}

TEST(FacingClassTest, ReportsNoRedrawInsideOneFrame) {
  FacingClass facing(DirType::kNorth);
  facing.Set_Desired(DirType::kEast);
  // Directions 0..4 all show body frame 0 of 32.
  EXPECT_FALSE(facing.Rotation_Adjust(2));
  EXPECT_EQ(facing.Current(), DirType(2));
}

}  // namespace
}  // namespace game
