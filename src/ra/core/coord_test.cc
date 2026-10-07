// Tests the inline direction and adjacency helpers of ra/core/coord.h.

#include "ra/core/coord.h"

#include "game/sim/direction.h"
#include "gtest/gtest.h"
#include "ra/core/cell.h"
#include "ra/core/coordinate.h"

namespace ra {
namespace {

constexpr Cell kCell = Cell::FromXY(10, 20);

TEST(CoordTest, CoordFromPixelsRoundsToTheNearestLepton) {
  // A cell is 24 pixels and 256 leptons wide, so a pixel is 10.67 leptons.
  EXPECT_EQ(CoordFromPixels(24, 48), Coordinate::FromLeptons(256, 512));
  EXPECT_EQ(CoordFromPixels(1, 2), Coordinate::FromLeptons(11, 21));
}

TEST(CoordTest, DirectionHasTheFullResolution) {
  const Coordinate from = kCell.Center();

  EXPECT_EQ(DirectionBetween(from, Cell::FromXY(10, 15).Center()),
            game::Direction::kNorth);
  EXPECT_EQ(DirectionBetween(from, Cell::FromXY(15, 20).Center()),
            game::Direction::kEast);
  // A little north of east is neither compass point.
  const game::Direction dir =
      DirectionBetween(from, Cell::FromXY(18, 19).Center());
  EXPECT_GT(dir, game::Direction::kNorthEast);
  EXPECT_LT(dir, game::Direction::kEast);
}

TEST(CoordTest, Direction8SnapsToACompassPoint) {
  const Coordinate from = kCell.Center();

  EXPECT_EQ(Direction8(from, Cell::FromXY(18, 19).Center()),
            game::Direction::kEast);
  EXPECT_EQ(Direction8(from, Cell::FromXY(10, 25).Center()),
            game::Direction::kSouth);
}

TEST(CoordTest, CellDirectionSnapsToACompassPoint) {
  EXPECT_EQ(DirectionBetween(kCell, Cell::FromXY(2, 19)),
            game::Direction::kWest);
}

TEST(CoordTest, AdjacentCellStepsOneCell) {
  EXPECT_EQ(AdjacentCell(kCell, game::FacingType::kNorth),
            Cell::FromXY(10, 19));
  EXPECT_EQ(AdjacentCell(kCell, game::FacingType::kSouthEast),
            Cell::FromXY(11, 21));
  EXPECT_EQ(AdjacentCell(kCell, game::Direction::kWest), Cell::FromXY(9, 20));
}

TEST(CoordTest, AdjacentCellWrapsFacingNoneToNorthWest) {
  EXPECT_EQ(AdjacentCell(kCell, game::FacingType::kNone), Cell::FromXY(9, 19));
  EXPECT_EQ(AdjacentCell(kCell.Center(), game::FacingType::kNone),
            Cell::FromXY(9, 19).Center());
}

TEST(CoordTest, AdjacentCoordinateIsTheNeighbouringCellCenter) {
  // Wherever in the cell the coordinate lies, the result is a cell center.
  const Coordinate corner = kCell.Center() - Coordinate::FromLeptons(100, 100);

  EXPECT_EQ(AdjacentCell(corner, game::FacingType::kSouth),
            Cell::FromXY(10, 21).Center());
  EXPECT_EQ(AdjacentCell(corner, game::Direction::kEast),
            Cell::FromXY(11, 20).Center());
}

TEST(CoordTest, AdjacentOffsetTablesAgree) {
  for (const game::FacingType facing :
       {game::FacingType::kNorth, game::FacingType::kNorthEast,
        game::FacingType::kEast, game::FacingType::kSouthEast,
        game::FacingType::kSouth, game::FacingType::kSouthWest,
        game::FacingType::kWest, game::FacingType::kNorthWest}) {
    EXPECT_EQ(Cell::Containing(AdjacentCell(kCell.Center(), facing)),
              AdjacentCell(kCell, facing));
  }
}

}  // namespace
}  // namespace ra
