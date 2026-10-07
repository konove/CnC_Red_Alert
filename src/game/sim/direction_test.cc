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

TEST(NearestFacingTest, RoundsToTheNearestFacing) {
  EXPECT_EQ(NearestFacing(AsDirection(15)), FacingType::kNorth);
  EXPECT_EQ(NearestFacing(AsDirection(16)), FacingType::kNorthEast);
  EXPECT_EQ(NearestFacing(AsDirection(239)), FacingType::kNorthWest);
  EXPECT_EQ(NearestFacing(AsDirection(240)), FacingType::kNorth);
}

TEST(FacingSectorTest, TruncatesToTheFacingBelow) {
  EXPECT_EQ(FacingSector(AsDirection(31)), FacingType::kNorth);
  EXPECT_EQ(FacingSector(DirType::kNorthEast), FacingType::kNorthEast);
  EXPECT_EQ(FacingSector(AsDirection(255)), FacingType::kNorthWest);
}

TEST(DirectionOfTest, PointsExactlyAtTheFacing) {
  EXPECT_EQ(DirectionOf(FacingType::kNorth), DirType::kNorth);
  EXPECT_EQ(DirectionOf(FacingType::kSouthEast), DirType::kSouthEast);
  EXPECT_EQ(DirectionOf(FacingType::kNorthWest), DirType::kNorthWest);
}

TEST(BodyFrameTest, CompassPointsLandOnEvenFrames) {
  EXPECT_EQ(BodyFrame16(DirType::kNorth), 0);
  EXPECT_EQ(BodyFrame16(DirType::kEast), 4);
  EXPECT_EQ(BodyFrame16(DirType::kSouth), 8);
  EXPECT_EQ(BodyFrame16(AsDirection(255)), 0);
  EXPECT_EQ(BodyFrame32(DirType::kNorth), 0);
  EXPECT_EQ(BodyFrame32(DirType::kEast), 8);
  EXPECT_EQ(BodyFrame32(DirType::kSouth), 16);
  EXPECT_EQ(BodyFrame32(DirType::kWest), 24);
  EXPECT_EQ(BodyFrame32(AsDirection(255)), 0);
}

TEST(BodyFrameTest, ThirtyTwoStepsAreSkewedTowardTheDiagonals) {
  // An even split would put DirType::kNorthEast at frame 4; the 3D Studio
  // correction puts it at 3.
  EXPECT_EQ(BodyFrame32(DirType::kNorthEast), 3);
  EXPECT_EQ(BodyFrame32(DirType::kSouthEast), 13);
}

TEST(SignedDirectionDifferenceTest, SubtractsSignedBytes) {
  EXPECT_EQ(SignedDirectionDifference(DirType::kNorth, DirType::kEast), 64);
  EXPECT_EQ(SignedDirectionDifference(DirType::kEast, DirType::kNorth), -64);
  EXPECT_EQ(SignedDirectionDifference(AsDirection(127), AsDirection(128)),
            -255);
}

TEST(Direction8TowardTest, ReturnsTheCompassPoint) {
  EXPECT_EQ(Direction8Toward(0, 0, 0, -5), DirType::kNorth);
  EXPECT_EQ(Direction8Toward(0, 0, 5, 0), DirType::kEast);
  EXPECT_EQ(Direction8Toward(0, 0, 5, 5), DirType::kSouthEast);
  EXPECT_EQ(Direction8Toward(0, 0, 0, 5), DirType::kSouth);
  EXPECT_EQ(Direction8Toward(0, 0, -5, 0), DirType::kWest);
  EXPECT_EQ(Direction8Toward(0, 0, -5, -4), DirType::kNorthWest);
}

TEST(Direction8TowardTest, SamePointGivesNorthEast) {
  EXPECT_EQ(Direction8Toward(3, 3, 3, 3), DirType::kNorthEast);
}

TEST(Direction256TowardTest, ReturnsTheExactAngle) {
  EXPECT_EQ(Direction256Toward(0, 0, 0, -100), DirType::kNorth);
  EXPECT_EQ(Direction256Toward(0, 0, -100, 0), DirType::kWest);
  EXPECT_EQ(Direction256Toward(0, 0, 100, -100), DirType::kNorthEast);
}

TEST(Direction256TowardTest, EastAndSouthLandOneStepShort) {
  // The quadrants that count down from the next axis subtract one too many,
  // as the original did.
  EXPECT_EQ(Direction256Toward(0, 0, 100, 0), DirType::kEast - 1);
  EXPECT_EQ(Direction256Toward(0, 0, 0, 100), DirType::kSouth - 1);
}

TEST(Direction256TowardTest, SamePointGivesJustShyOfNorth) {
  EXPECT_EQ(Direction256Toward(7, 7, 7, 7), AsDirection(255));
}

}  // namespace
}  // namespace game
