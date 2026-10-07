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

  EXPECT_EQ(Direction(from, Cell::FromXY(10, 15).Center()),
            game::DirType::DIR_N);
  // Due east comes out one step short of DIR_E: game::Desired_Facing256()
  // rounds the octant's last step down. Every machine does the same.
  EXPECT_EQ(Direction(from, Cell::FromXY(15, 20).Center()),
            game::AsDirection(0x3F));
  // A little north of east is neither compass point.
  const game::DirType dir = Direction(from, Cell::FromXY(18, 19).Center());
  EXPECT_GT(dir, game::DirType::DIR_NE);
  EXPECT_LT(dir, game::DirType::DIR_E);
}

TEST(CoordTest, Direction8SnapsToACompassPoint) {
  const Coordinate from = kCell.Center();

  EXPECT_EQ(Direction8(from, Cell::FromXY(18, 19).Center()),
            game::DirType::DIR_E);
  EXPECT_EQ(Direction8(from, Cell::FromXY(10, 25).Center()),
            game::DirType::DIR_S);
}

TEST(CoordTest, CellDirectionSnapsToACompassPoint) {
  EXPECT_EQ(Direction(kCell, Cell::FromXY(2, 19)), game::DirType::DIR_W);
}

TEST(CoordTest, AdjacentCellStepsOneCell) {
  EXPECT_EQ(AdjacentCell(kCell, game::FacingType::kNorth),
            Cell::FromXY(10, 19));
  EXPECT_EQ(AdjacentCell(kCell, game::FacingType::kSouthEast),
            Cell::FromXY(11, 21));
  EXPECT_EQ(AdjacentCell(kCell, game::DirType::DIR_W), Cell::FromXY(9, 20));
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
  EXPECT_EQ(AdjacentCell(corner, game::DirType::DIR_E),
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
