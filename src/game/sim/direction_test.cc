#include "game/sim/direction.h"

#include <array>

#include "base/numeric.h"
#include "gtest/gtest.h"

namespace game {
namespace {

TEST(FacingTypeTest, ArithmeticWrapsOntoEightFacings) {
  EXPECT_EQ(AsFacing(8), FacingType::kNorth);
  EXPECT_EQ(AsFacing(-1), FacingType::kNorthWest);
  EXPECT_EQ(FacingType::kNorthWest + 1, FacingType::kNorth);
  EXPECT_EQ(FacingType::kNorth - FacingType::kEast, FacingType::kWest);
  EXPECT_EQ(FacingType::kSouthWest + FacingType::kSouth,
            FacingType::kNorthEast);
  EXPECT_EQ(FacingType::kNorth - 3, FacingType::kSouthWest);
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

TEST(DirectionOfTest, PointsExactlyAtTheFacing) {
  EXPECT_EQ(DirectionOf(FacingType::kNorth), DirType::kNorth);
  EXPECT_EQ(DirectionOf(FacingType::kSouthEast), DirType::kSouthEast);
  EXPECT_EQ(DirectionOf(FacingType::kNorthWest), DirType::kNorthWest);
}

TEST(BodyFrameTest, SixteenFramesRoundToTheNearest) {
  EXPECT_EQ(BodyFrame16(AsDirection(7)), 0);
  EXPECT_EQ(BodyFrame16(AsDirection(8)), 1);
  EXPECT_EQ(BodyFrame16(AsDirection(247)), 15);
  EXPECT_EQ(BodyFrame16(AsDirection(248)), 0);
}

TEST(BodyFrameTest, ThirtyTwoFramesRiseClockwiseInUnevenSteps) {
  std::array<int, 32> directions_per_frame{};
  int previous = 0;
  for (int angle = 0; angle < 256; ++angle) {
    const int frame = BodyFrame32(AsDirection(angle));
    ASSERT_GE(frame, 0);
    ASSERT_LT(frame, 32);
    // The last few directions round up to north again.
    if (frame != 0) {
      EXPECT_GE(frame, previous) << angle;
      previous = frame;
    }
    ++directions_per_frame.at(base::ToSize(frame));
  }
  for (const int count : directions_per_frame) {
    EXPECT_GE(count, 6);
    EXPECT_LE(count, 11);
  }
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

TEST(ShortestTurnTest, IsSignedAndClockwisePositive) {
  EXPECT_EQ(ShortestTurn(DirType::kNorth, DirType::kEast), 64);
  EXPECT_EQ(ShortestTurn(DirType::kEast, DirType::kNorth), -64);
  EXPECT_EQ(ShortestTurn(DirType::kWest, DirType::kWest), 0);
}

TEST(ShortestTurnTest, WrapsAcrossNorthAndSouth) {
  EXPECT_EQ(ShortestTurn(AsDirection(127), AsDirection(128)), 1);
  EXPECT_EQ(ShortestTurn(AsDirection(128), AsDirection(127)), -1);
  EXPECT_EQ(ShortestTurn(AsDirection(255), DirType::kNorth), 1);
  EXPECT_EQ(ShortestTurn(DirType::kSouthWest, AsDirection(96)), -64);
}

TEST(ShortestTurnTest, OppositeDirectionsAreHalfACircleApart) {
  EXPECT_EQ(ShortestTurn(DirType::kNorth, DirType::kSouth), -128);
  EXPECT_EQ(ShortestTurn(DirType::kEast, DirType::kWest), -128);
}

TEST(Direction8TowardTest, ReturnsTheCompassPoint) {
  EXPECT_EQ(Direction8Toward(0, 0, 0, -5), DirType::kNorth);
  EXPECT_EQ(Direction8Toward(0, 0, 5, 0), DirType::kEast);
  EXPECT_EQ(Direction8Toward(0, 0, 5, 5), DirType::kSouthEast);
  EXPECT_EQ(Direction8Toward(0, 0, 0, 5), DirType::kSouth);
  EXPECT_EQ(Direction8Toward(0, 0, -5, 0), DirType::kWest);
  EXPECT_EQ(Direction8Toward(0, 0, -5, -4), DirType::kNorthWest);
}

TEST(Direction8TowardTest, ReturnsTheDiagonals) {
  EXPECT_EQ(Direction8Toward(0, 0, 5, -5), DirType::kNorthEast);
  EXPECT_EQ(Direction8Toward(0, 0, -5, 5), DirType::kSouthWest);
  EXPECT_EQ(Direction8Toward(0, 0, -5, -5), DirType::kNorthWest);
}

TEST(Direction8TowardTest, ASlopeOfOneInTwoIsAlreadyDiagonal) {
  EXPECT_EQ(Direction8Toward(0, 0, 4, -2), DirType::kNorthEast);
  EXPECT_EQ(Direction8Toward(0, 0, 5, -2), DirType::kEast);
  EXPECT_EQ(Direction8Toward(0, 0, 2, -4), DirType::kNorthEast);
  EXPECT_EQ(Direction8Toward(0, 0, 2, -5), DirType::kNorth);
  EXPECT_EQ(Direction8Toward(0, 0, -2, 5), DirType::kSouth);
  EXPECT_EQ(Direction8Toward(0, 0, 2, 5), DirType::kSouth);
  EXPECT_EQ(Direction8Toward(0, 0, -2, -5), DirType::kNorth);
}

TEST(Direction8TowardTest, DependsOnlyOnTheOffset) {
  EXPECT_EQ(Direction8Toward(100, -40, 105, -40), DirType::kEast);
  EXPECT_EQ(Direction8Toward(100, -40, 100, -45), DirType::kNorth);
}

TEST(Direction8TowardTest, SamePointGivesNorthEast) {
  EXPECT_EQ(Direction8Toward(3, 3, 3, 3), DirType::kNorthEast);
}

TEST(Direction256TowardTest, IsExactOnTheAxes) {
  EXPECT_EQ(Direction256Toward(0, 0, 0, -100), DirType::kNorth);
  EXPECT_EQ(Direction256Toward(0, 0, 100, 0), DirType::kEast);
  EXPECT_EQ(Direction256Toward(0, 0, 0, 100), DirType::kSouth);
  EXPECT_EQ(Direction256Toward(0, 0, -100, 0), DirType::kWest);
}

TEST(Direction256TowardTest, IsExactOnTheDiagonals) {
  EXPECT_EQ(Direction256Toward(0, 0, 100, -100), DirType::kNorthEast);
  EXPECT_EQ(Direction256Toward(0, 0, 100, 100), DirType::kSouthEast);
  EXPECT_EQ(Direction256Toward(0, 0, -100, 100), DirType::kSouthWest);
  EXPECT_EQ(Direction256Toward(0, 0, -100, -100), DirType::kNorthWest);
}

TEST(Direction256TowardTest, InterpolatesLinearlyBetweenAxisAndDiagonal) {
  // Halfway along the shorter offset is halfway to the diagonal, 16 steps,
  // where the true angle (26.6 degrees) would be 19.
  EXPECT_EQ(Direction256Toward(0, 0, 50, -100), AsDirection(16));
  // Past the diagonal the count runs back from the axis ahead.
  EXPECT_EQ(Direction256Toward(0, 0, 100, -50), AsDirection(64 - 16));
  EXPECT_EQ(Direction256Toward(0, 0, 50, 100), AsDirection(128 - 16));
  EXPECT_EQ(Direction256Toward(0, 0, -50, 100), AsDirection(128 + 16));
  EXPECT_EQ(Direction256Toward(0, 0, -50, -100), AsDirection(256 - 16));
}

TEST(Direction256TowardTest, RoundsTowardTheStartOfTheQuadrant) {
  // 1 in 3 is 10.67 steps off the nearer axis. Both halves of a quadrant
  // round it toward the axis the quadrant starts on.
  EXPECT_EQ(Direction256Toward(0, 0, 1, -3), AsDirection(10));
  EXPECT_EQ(Direction256Toward(0, 0, 3, -1), AsDirection(64 - 11));
  EXPECT_EQ(Direction256Toward(0, 0, 3, 1), AsDirection(64 + 10));
  EXPECT_EQ(Direction256Toward(0, 0, 1, 3), AsDirection(128 - 11));
}

TEST(Direction256TowardTest, NeverLeavesTheQuadrantOfTheTarget) {
  for (int x = 1; x <= 40; ++x) {
    for (int y = 1; y <= 40; ++y) {
      const int north_east = static_cast<int>(Direction256Toward(0, 0, x, -y));
      EXPECT_LT(north_east, 64) << x << " " << y;
      const int north_west = static_cast<int>(Direction256Toward(0, 0, -x, -y));
      EXPECT_GE(north_west, 192) << x << " " << y;
    }
  }
}

TEST(Direction256TowardTest, SamePointGivesJustShyOfNorth) {
  EXPECT_EQ(Direction256Toward(7, 7, 7, 7), AsDirection(255));
}

}  // namespace
}  // namespace game
