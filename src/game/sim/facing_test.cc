#include "game/sim/facing.h"

#include "game/sim/direction.h"
#include "gtest/gtest.h"

namespace game {
namespace {

TEST(FacingClassTest, StartsAtNorthAndStill) {
  const FacingClass facing;
  EXPECT_EQ(facing.Current(), DIR_N);
  EXPECT_EQ(facing.Desired(), DIR_N);
  EXPECT_FALSE(facing.Is_Rotating());
}

TEST(FacingClassTest, SettersReportWhetherTheValueChanged) {
  FacingClass facing(DIR_E);
  EXPECT_FALSE(facing.Set_Desired(DIR_E));
  EXPECT_TRUE(facing.Set_Desired(DIR_S));
  EXPECT_FALSE(facing.Set_Current(DIR_E));
  EXPECT_TRUE(facing.Set_Current(DIR_W));
}

TEST(FacingClassTest, DifferenceTakesTheShortWayRound) {
  FacingClass facing(DIR_N);
  facing.Set_Desired(DIR_E);
  EXPECT_EQ(facing.Difference(), 64);
  facing.Set_Desired(DIR_W);
  EXPECT_EQ(facing.Difference(), -64);
  EXPECT_EQ(facing.Difference(AsDirection(200)), -56);
}

TEST(FacingClassTest, RotatesClockwiseByTheRate) {
  FacingClass facing(DIR_N);
  facing.Set_Desired(DIR_E);
  EXPECT_TRUE(facing.Rotation_Adjust(16));
  EXPECT_EQ(facing.Current(), AsDirection(16));
  EXPECT_TRUE(facing.Is_Rotating());
}

TEST(FacingClassTest, RotatesCounterclockwiseTowardACloserTarget) {
  FacingClass facing(DIR_N);
  facing.Set_Desired(DIR_W);
  facing.Rotation_Adjust(16);
  EXPECT_EQ(facing.Current(), AsDirection(240));
}

TEST(FacingClassTest, SnapsWhenTheRateCoversTheGap) {
  FacingClass facing(DIR_N);
  facing.Set_Desired(DIR_E);
  facing.Rotation_Adjust(100);
  EXPECT_EQ(facing.Current(), DIR_E);
  EXPECT_FALSE(facing.Is_Rotating());
  EXPECT_FALSE(facing.Rotation_Adjust(100));
}

TEST(FacingClassTest, ReportsNoRedrawInsideOneFrame) {
  FacingClass facing(DIR_N);
  facing.Set_Desired(DIR_E);
  // Directions 0..4 all show body frame 0 of 32.
  EXPECT_FALSE(facing.Rotation_Adjust(2));
  EXPECT_EQ(facing.Current(), AsDirection(2));
}

}  // namespace
}  // namespace game
