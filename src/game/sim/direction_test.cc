#include "game/sim/direction.h"

#include "gtest/gtest.h"

namespace game {
namespace {

TEST(FacingTypeTest, ArithmeticWrapsOntoEightFacings) {
  EXPECT_EQ(AsFacing(8), FacingType::kNorth);
  EXPECT_EQ(AsFacing(-1), FacingType::kNorthWest);
  EXPECT_EQ(FacingType::kNorthWest + 1, FacingType::kNorth);
  EXPECT_EQ(FacingType::kNorth - FacingType::kEast, FacingType::kWest);
  FacingType facing = FacingType::kSouthWest;
  facing += FacingType::kSouth;
  EXPECT_EQ(facing, FacingType::kNorthEast);
}

TEST(DirTypeTest, ArithmeticWrapsAroundTheCircle) {
  EXPECT_EQ(AsDirection(256), DirType::kNorth);
  EXPECT_EQ(AsDirection(-1), AsDirection(255));
  EXPECT_EQ(DirType::kNorthWest + DirType::kEast, DirType::kNorthEast);
  EXPECT_EQ(DirType::kNorth - 32, DirType::kNorthWest);
}

TEST(DirFacingTest, RoundsToTheNearestFacing) {
  EXPECT_EQ(Dir_Facing(AsDirection(15)), FacingType::kNorth);
  EXPECT_EQ(Dir_Facing(AsDirection(16)), FacingType::kNorthEast);
  EXPECT_EQ(Dir_Facing(AsDirection(239)), FacingType::kNorthWest);
  EXPECT_EQ(Dir_Facing(AsDirection(240)), FacingType::kNorth);
}

TEST(DirTo8Test, TruncatesToTheFacingBelow) {
  EXPECT_EQ(Dir_To_8(AsDirection(31)), FacingType::kNorth);
  EXPECT_EQ(Dir_To_8(DirType::kNorthEast), FacingType::kNorthEast);
  EXPECT_EQ(Dir_To_8(AsDirection(255)), FacingType::kNorthWest);
}

TEST(FacingDirTest, PointsExactlyAtTheFacing) {
  EXPECT_EQ(Facing_Dir(FacingType::kNorth), DirType::kNorth);
  EXPECT_EQ(Facing_Dir(FacingType::kSouthEast), DirType::kSouthEast);
  EXPECT_EQ(Facing_Dir(FacingType::kNorthWest), DirType::kNorthWest);
}

TEST(DirToFramesTest, CompassPointsLandOnEvenFrames) {
  EXPECT_EQ(Dir_To_16(DirType::kNorth), 0);
  EXPECT_EQ(Dir_To_16(DirType::kEast), 4);
  EXPECT_EQ(Dir_To_16(DirType::kSouth), 8);
  EXPECT_EQ(Dir_To_16(AsDirection(255)), 0);
  EXPECT_EQ(Dir_To_32(DirType::kNorth), 0);
  EXPECT_EQ(Dir_To_32(DirType::kEast), 8);
  EXPECT_EQ(Dir_To_32(DirType::kSouth), 16);
  EXPECT_EQ(Dir_To_32(DirType::kWest), 24);
  EXPECT_EQ(Dir_To_32(AsDirection(255)), 0);
}

TEST(DirToFramesTest, ThirtyTwoStepsAreSkewedTowardTheDiagonals) {
  // An even split would put DirType::kNorthEast at frame 4; the 3D Studio
  // correction puts it at 3.
  EXPECT_EQ(Dir_To_32(DirType::kNorthEast), 3);
  EXPECT_EQ(Dir_To_32(DirType::kSouthEast), 13);
}

TEST(DirDiffTest, SubtractsSignedBytes) {
  EXPECT_EQ(Dir_Diff(DirType::kNorth, DirType::kEast), 64);
  EXPECT_EQ(Dir_Diff(DirType::kEast, DirType::kNorth), -64);
  EXPECT_EQ(Dir_Diff(AsDirection(127), AsDirection(128)), -255);
}

TEST(DesiredFacing8Test, ReturnsTheCompassPoint) {
  EXPECT_EQ(Desired_Facing8(0, 0, 0, -5), DirType::kNorth);
  EXPECT_EQ(Desired_Facing8(0, 0, 5, 0), DirType::kEast);
  EXPECT_EQ(Desired_Facing8(0, 0, 5, 5), DirType::kSouthEast);
  EXPECT_EQ(Desired_Facing8(0, 0, 0, 5), DirType::kSouth);
  EXPECT_EQ(Desired_Facing8(0, 0, -5, 0), DirType::kWest);
  EXPECT_EQ(Desired_Facing8(0, 0, -5, -4), DirType::kNorthWest);
}

TEST(DesiredFacing8Test, SamePointGivesNorthEast) {
  EXPECT_EQ(Desired_Facing8(3, 3, 3, 3), DirType::kNorthEast);
}

TEST(DesiredFacing256Test, ReturnsTheExactAngle) {
  EXPECT_EQ(Desired_Facing256(0, 0, 0, -100), DirType::kNorth);
  EXPECT_EQ(Desired_Facing256(0, 0, -100, 0), DirType::kWest);
  EXPECT_EQ(Desired_Facing256(0, 0, 100, -100), DirType::kNorthEast);
}

TEST(DesiredFacing256Test, EastAndSouthLandOneStepShort) {
  // The quadrants that count down from the next axis subtract one too many,
  // as the original did.
  EXPECT_EQ(Desired_Facing256(0, 0, 100, 0), DirType::kEast - 1);
  EXPECT_EQ(Desired_Facing256(0, 0, 0, 100), DirType::kSouth - 1);
}

TEST(DesiredFacing256Test, SamePointGivesJustShyOfNorth) {
  EXPECT_EQ(Desired_Facing256(7, 7, 7, 7), AsDirection(255));
}

}  // namespace
}  // namespace game
