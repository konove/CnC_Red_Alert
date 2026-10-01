#include "game/sim/direction.h"

#include "gtest/gtest.h"

namespace game {
namespace {

TEST(FacingTypeTest, ArithmeticWrapsOntoEightFacings) {
  EXPECT_EQ(AsFacing(8), FACING_N);
  EXPECT_EQ(AsFacing(-1), FACING_NW);
  EXPECT_EQ(FACING_NW + 1, FACING_N);
  EXPECT_EQ(FACING_N - FACING_E, FACING_W);
  FacingType facing = FACING_SW;
  facing += FACING_S;
  EXPECT_EQ(facing, FACING_NE);
}

TEST(DirTypeTest, ArithmeticWrapsAroundTheCircle) {
  EXPECT_EQ(AsDirection(256), DIR_N);
  EXPECT_EQ(AsDirection(-1), AsDirection(255));
  EXPECT_EQ(DIR_NW + DIR_E, DIR_NE);
  EXPECT_EQ(DIR_N - 32, DIR_NW);
}

TEST(DirFacingTest, RoundsToTheNearestFacing) {
  EXPECT_EQ(Dir_Facing(AsDirection(15)), FACING_N);
  EXPECT_EQ(Dir_Facing(AsDirection(16)), FACING_NE);
  EXPECT_EQ(Dir_Facing(AsDirection(239)), FACING_NW);
  EXPECT_EQ(Dir_Facing(AsDirection(240)), FACING_N);
}

TEST(DirTo8Test, TruncatesToTheFacingBelow) {
  EXPECT_EQ(Dir_To_8(AsDirection(31)), FACING_N);
  EXPECT_EQ(Dir_To_8(DIR_NE), FACING_NE);
  EXPECT_EQ(Dir_To_8(AsDirection(255)), FACING_NW);
}

TEST(FacingDirTest, PointsExactlyAtTheFacing) {
  EXPECT_EQ(Facing_Dir(FACING_N), DIR_N);
  EXPECT_EQ(Facing_Dir(FACING_SE), DIR_SE);
  EXPECT_EQ(Facing_Dir(FACING_NW), DIR_NW);
}

TEST(DirToFramesTest, CompassPointsLandOnEvenFrames) {
  EXPECT_EQ(Dir_To_16(DIR_N), 0);
  EXPECT_EQ(Dir_To_16(DIR_E), 4);
  EXPECT_EQ(Dir_To_16(DIR_S), 8);
  EXPECT_EQ(Dir_To_16(AsDirection(255)), 0);
  EXPECT_EQ(Dir_To_32(DIR_N), 0);
  EXPECT_EQ(Dir_To_32(DIR_E), 8);
  EXPECT_EQ(Dir_To_32(DIR_S), 16);
  EXPECT_EQ(Dir_To_32(DIR_W), 24);
  EXPECT_EQ(Dir_To_32(AsDirection(255)), 0);
}

TEST(DirToFramesTest, ThirtyTwoStepsAreSkewedTowardTheDiagonals) {
  // An even split would put DIR_NE at frame 4; the 3D Studio correction
  // puts it at 3.
  EXPECT_EQ(Dir_To_32(DIR_NE), 3);
  EXPECT_EQ(Dir_To_32(DIR_SE), 13);
}

TEST(DirDiffTest, SubtractsSignedBytes) {
  EXPECT_EQ(Dir_Diff(DIR_N, DIR_E), 64);
  EXPECT_EQ(Dir_Diff(DIR_E, DIR_N), -64);
  EXPECT_EQ(Dir_Diff(AsDirection(127), AsDirection(128)), -255);
}

TEST(DesiredFacing8Test, ReturnsTheCompassPoint) {
  EXPECT_EQ(Desired_Facing8(0, 0, 0, -5), DIR_N);
  EXPECT_EQ(Desired_Facing8(0, 0, 5, 0), DIR_E);
  EXPECT_EQ(Desired_Facing8(0, 0, 5, 5), DIR_SE);
  EXPECT_EQ(Desired_Facing8(0, 0, 0, 5), DIR_S);
  EXPECT_EQ(Desired_Facing8(0, 0, -5, 0), DIR_W);
  EXPECT_EQ(Desired_Facing8(0, 0, -5, -4), DIR_NW);
}

TEST(DesiredFacing8Test, SamePointGivesNorthEast) {
  EXPECT_EQ(Desired_Facing8(3, 3, 3, 3), DIR_NE);
}

TEST(DesiredFacing256Test, ReturnsTheExactAngle) {
  EXPECT_EQ(Desired_Facing256(0, 0, 0, -100), DIR_N);
  EXPECT_EQ(Desired_Facing256(0, 0, -100, 0), DIR_W);
  EXPECT_EQ(Desired_Facing256(0, 0, 100, -100), DIR_NE);
}

TEST(DesiredFacing256Test, EastAndSouthLandOneStepShort) {
  // The quadrants that count down from the next axis subtract one too many,
  // as the original did.
  EXPECT_EQ(Desired_Facing256(0, 0, 100, 0), DIR_E - 1);
  EXPECT_EQ(Desired_Facing256(0, 0, 0, 100), DIR_S - 1);
}

TEST(DesiredFacing256Test, SamePointGivesJustShyOfNorth) {
  EXPECT_EQ(Desired_Facing256(7, 7, 7, 7), AsDirection(255));
}

}  // namespace
}  // namespace game
