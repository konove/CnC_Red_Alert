#include "game/sim/direction.h"

#include <array>
#include <cstddef>
#include <span>

#include "base/numeric.h"
#include "engine/stream/archive.h"
#include "engine/stream/span_sink.h"
#include "engine/stream/span_source.h"
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

TEST(DirectionTest, ArithmeticWrapsAroundTheCircle) {
  EXPECT_EQ(Direction(256), Direction::kNorth);
  EXPECT_EQ(Direction(-1), Direction(255));
  EXPECT_EQ(Direction::kNorthWest + Direction::kEast, Direction::kNorthEast);
  EXPECT_EQ(Direction::kNorth - 32, Direction::kNorthWest);
}

TEST(DirectionTest, StartsNorthAndReadsBackItsAngle) {
  EXPECT_EQ(Direction(), Direction::kNorth);
  EXPECT_EQ(Direction::kEast.value(), 64);
  EXPECT_EQ(Direction(300).value(), 44);
}

TEST(DirectionTest, OrdersByAngle) {
  EXPECT_LT(Direction::kNorth, Direction::kNorthEast);
  EXPECT_GT(Direction(255), Direction::kNorthWest);
}

// Saves written while a direction was an enum hold it as four bytes.
TEST(DirectionTest, SerializesAsFourBytes) {
  std::array<std::byte, 4> bytes{};
  SpanSink sink(bytes);
  ArchiveWriter writer(sink);
  Direction written = Direction::kSouthWest;
  writer(written);
  ASSERT_TRUE(writer.ok());
  EXPECT_EQ(bytes, (std::array{std::byte{0xA0}, std::byte{0}, std::byte{0},
                               std::byte{0}}));

  SpanSource source{std::span<const std::byte>(bytes)};
  ArchiveReader reader(source);
  Direction read;
  reader(read);
  ASSERT_TRUE(reader.ok());
  EXPECT_EQ(read, Direction::kSouthWest);
}

TEST(NearestFacingTest, RoundsToTheNearestFacing) {
  EXPECT_EQ(NearestFacing(Direction(15)), FacingType::kNorth);
  EXPECT_EQ(NearestFacing(Direction(16)), FacingType::kNorthEast);
  EXPECT_EQ(NearestFacing(Direction(239)), FacingType::kNorthWest);
  EXPECT_EQ(NearestFacing(Direction(240)), FacingType::kNorth);
}

TEST(DirectionOfTest, PointsExactlyAtTheFacing) {
  EXPECT_EQ(DirectionOf(FacingType::kNorth), Direction::kNorth);
  EXPECT_EQ(DirectionOf(FacingType::kSouthEast), Direction::kSouthEast);
  EXPECT_EQ(DirectionOf(FacingType::kNorthWest), Direction::kNorthWest);
}

TEST(BodyFrameTest, SixteenFramesRoundToTheNearest) {
  EXPECT_EQ(BodyFrame16(Direction(7)), 0);
  EXPECT_EQ(BodyFrame16(Direction(8)), 1);
  EXPECT_EQ(BodyFrame16(Direction(247)), 15);
  EXPECT_EQ(BodyFrame16(Direction(248)), 0);
}

TEST(BodyFrameTest, ThirtyTwoFramesRiseClockwiseInUnevenSteps) {
  std::array<int, 32> directions_per_frame{};
  int previous = 0;
  for (int angle = 0; angle < 256; ++angle) {
    const int frame = BodyFrame32(Direction(angle));
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
  EXPECT_EQ(BodyFrame16(Direction::kNorth), 0);
  EXPECT_EQ(BodyFrame16(Direction::kEast), 4);
  EXPECT_EQ(BodyFrame16(Direction::kSouth), 8);
  EXPECT_EQ(BodyFrame16(Direction(255)), 0);
  EXPECT_EQ(BodyFrame32(Direction::kNorth), 0);
  EXPECT_EQ(BodyFrame32(Direction::kEast), 8);
  EXPECT_EQ(BodyFrame32(Direction::kSouth), 16);
  EXPECT_EQ(BodyFrame32(Direction::kWest), 24);
  EXPECT_EQ(BodyFrame32(Direction(255)), 0);
}

TEST(BodyFrameTest, ThirtyTwoStepsAreSkewedTowardTheDiagonals) {
  // An even split would put Direction::kNorthEast at frame 4; the 3D Studio
  // correction puts it at 3.
  EXPECT_EQ(BodyFrame32(Direction::kNorthEast), 3);
  EXPECT_EQ(BodyFrame32(Direction::kSouthEast), 13);
}

TEST(ShortestTurnTest, IsSignedAndClockwisePositive) {
  EXPECT_EQ(ShortestTurn(Direction::kNorth, Direction::kEast), 64);
  EXPECT_EQ(ShortestTurn(Direction::kEast, Direction::kNorth), -64);
  EXPECT_EQ(ShortestTurn(Direction::kWest, Direction::kWest), 0);
}

TEST(ShortestTurnTest, WrapsAcrossNorthAndSouth) {
  EXPECT_EQ(ShortestTurn(Direction(127), Direction(128)), 1);
  EXPECT_EQ(ShortestTurn(Direction(128), Direction(127)), -1);
  EXPECT_EQ(ShortestTurn(Direction(255), Direction::kNorth), 1);
  EXPECT_EQ(ShortestTurn(Direction::kSouthWest, Direction(96)), -64);
}

TEST(ShortestTurnTest, OppositeDirectionsAreHalfACircleApart) {
  EXPECT_EQ(ShortestTurn(Direction::kNorth, Direction::kSouth), -128);
  EXPECT_EQ(ShortestTurn(Direction::kEast, Direction::kWest), -128);
}

TEST(Direction8TowardTest, ReturnsTheCompassPoint) {
  EXPECT_EQ(Direction8Toward(0, 0, 0, -5), Direction::kNorth);
  EXPECT_EQ(Direction8Toward(0, 0, 5, 0), Direction::kEast);
  EXPECT_EQ(Direction8Toward(0, 0, 5, 5), Direction::kSouthEast);
  EXPECT_EQ(Direction8Toward(0, 0, 0, 5), Direction::kSouth);
  EXPECT_EQ(Direction8Toward(0, 0, -5, 0), Direction::kWest);
  EXPECT_EQ(Direction8Toward(0, 0, -5, -4), Direction::kNorthWest);
}

TEST(Direction8TowardTest, ReturnsTheDiagonals) {
  EXPECT_EQ(Direction8Toward(0, 0, 5, -5), Direction::kNorthEast);
  EXPECT_EQ(Direction8Toward(0, 0, -5, 5), Direction::kSouthWest);
  EXPECT_EQ(Direction8Toward(0, 0, -5, -5), Direction::kNorthWest);
}

TEST(Direction8TowardTest, ASlopeOfOneInTwoIsAlreadyDiagonal) {
  EXPECT_EQ(Direction8Toward(0, 0, 4, -2), Direction::kNorthEast);
  EXPECT_EQ(Direction8Toward(0, 0, 5, -2), Direction::kEast);
  EXPECT_EQ(Direction8Toward(0, 0, 2, -4), Direction::kNorthEast);
  EXPECT_EQ(Direction8Toward(0, 0, 2, -5), Direction::kNorth);
  EXPECT_EQ(Direction8Toward(0, 0, -2, 5), Direction::kSouth);
  EXPECT_EQ(Direction8Toward(0, 0, 2, 5), Direction::kSouth);
  EXPECT_EQ(Direction8Toward(0, 0, -2, -5), Direction::kNorth);
}

TEST(Direction8TowardTest, DependsOnlyOnTheOffset) {
  EXPECT_EQ(Direction8Toward(100, -40, 105, -40), Direction::kEast);
  EXPECT_EQ(Direction8Toward(100, -40, 100, -45), Direction::kNorth);
}

TEST(Direction8TowardTest, SamePointGivesNorthEast) {
  EXPECT_EQ(Direction8Toward(3, 3, 3, 3), Direction::kNorthEast);
}

TEST(Direction256TowardTest, IsExactOnTheAxes) {
  EXPECT_EQ(Direction256Toward(0, 0, 0, -100), Direction::kNorth);
  EXPECT_EQ(Direction256Toward(0, 0, 100, 0), Direction::kEast);
  EXPECT_EQ(Direction256Toward(0, 0, 0, 100), Direction::kSouth);
  EXPECT_EQ(Direction256Toward(0, 0, -100, 0), Direction::kWest);
}

TEST(Direction256TowardTest, IsExactOnTheDiagonals) {
  EXPECT_EQ(Direction256Toward(0, 0, 100, -100), Direction::kNorthEast);
  EXPECT_EQ(Direction256Toward(0, 0, 100, 100), Direction::kSouthEast);
  EXPECT_EQ(Direction256Toward(0, 0, -100, 100), Direction::kSouthWest);
  EXPECT_EQ(Direction256Toward(0, 0, -100, -100), Direction::kNorthWest);
}

TEST(Direction256TowardTest, InterpolatesLinearlyBetweenAxisAndDiagonal) {
  // Halfway along the shorter offset is halfway to the diagonal, 16 steps,
  // where the true angle (26.6 degrees) would be 19.
  EXPECT_EQ(Direction256Toward(0, 0, 50, -100), Direction(16));
  // Past the diagonal the count runs back from the axis ahead.
  EXPECT_EQ(Direction256Toward(0, 0, 100, -50), Direction(64 - 16));
  EXPECT_EQ(Direction256Toward(0, 0, 50, 100), Direction(128 - 16));
  EXPECT_EQ(Direction256Toward(0, 0, -50, 100), Direction(128 + 16));
  EXPECT_EQ(Direction256Toward(0, 0, -50, -100), Direction(256 - 16));
}

TEST(Direction256TowardTest, RoundsTowardTheStartOfTheQuadrant) {
  // 1 in 3 is 10.67 steps off the nearer axis. Both halves of a quadrant
  // round it toward the axis the quadrant starts on.
  EXPECT_EQ(Direction256Toward(0, 0, 1, -3), Direction(10));
  EXPECT_EQ(Direction256Toward(0, 0, 3, -1), Direction(64 - 11));
  EXPECT_EQ(Direction256Toward(0, 0, 3, 1), Direction(64 + 10));
  EXPECT_EQ(Direction256Toward(0, 0, 1, 3), Direction(128 - 11));
}

TEST(Direction256TowardTest, NeverLeavesTheQuadrantOfTheTarget) {
  for (int x = 1; x <= 40; ++x) {
    for (int y = 1; y <= 40; ++y) {
      const int north_east = Direction256Toward(0, 0, x, -y).value();
      EXPECT_LT(north_east, 64) << x << " " << y;
      const int north_west = Direction256Toward(0, 0, -x, -y).value();
      EXPECT_GE(north_west, 192) << x << " " << y;
    }
  }
}

TEST(Direction256TowardTest, SamePointGivesJustShyOfNorth) {
  EXPECT_EQ(Direction256Toward(7, 7, 7, 7), Direction(255));
}

}  // namespace
}  // namespace game
