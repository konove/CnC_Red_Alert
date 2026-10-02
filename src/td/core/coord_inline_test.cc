// Checks the cell/coordinate conversions in td/core/inline.h, and Dir_Diff
// against the signed byte poke it replaced.

#include <cstring>
#include <span>

#include "base/buffer.h"
#include "game/sim/direction.h"
#include "gtest/gtest.h"
#include "td/core/defines.h"
#include "td/core/inline.h"

namespace td {

namespace {

int LegacyDirDiff(game::DirType dir1, game::DirType dir2) {
  signed char first = 0;
  signed char second = 0;
  base::CopyBytes(base::ObjectBytes(first), base::ObjectBytes(dir1), 1);
  base::CopyBytes(base::ObjectBytes(second), base::ObjectBytes(dir2), 1);
  return second - first;
}

// Coord_Cell keeps the X cell's whole byte: an X cell of 64 or more sets bits
// that belong to the Y cell, as the original assembly's OR did.
TEST(TdCoordInline, CoordCellOrsTheWholeXCellByte) {
  EXPECT_EQ(Coord_Cell(Coordinate::FromPacked(0x03800580U)), XY_Cell(5, 3));
  EXPECT_EQ(Coord_Cell(Coordinate::FromPacked(0x12345678U)), 0x04D6);
  EXPECT_EQ(Coord_Cell(Coordinate::FromPacked(0x01007F00U)), 0x007F);
  EXPECT_EQ(Coord_Cell(Coordinate::FromPacked(0x0000FF00U)), 0x00FF);
  EXPECT_EQ(Coord_Cell(Coordinate::FromPacked(0xFFFFFFFFU)), 0x3FFF);
}

TEST(TdCoordInline, CellCoordIsTheCellCenter) {
  EXPECT_EQ(Cell_Coord(XY_Cell(5, 3)).packed(), 0x03800580U);
  EXPECT_EQ(Cell_Coord(0).packed(), 0x00800080U);
  EXPECT_EQ(Cell_Coord(XY_Cell(63, 63)).packed(), 0x3F803F80U);
  for (CELL cell = 0; cell < MAP_CELL_TOTAL; cell += 37) {
    EXPECT_EQ(Coord_Cell(Cell_Coord(cell)), cell) << cell;
  }
}

TEST(TdCoordInline, DirDiffMatchesSignedBytes) {
  for (int first = 0; first < 256; first += 5) {
    for (int second = 0; second < 256; second += 3) {
      const game::DirType dir1 = game::AsDirection(first);
      const game::DirType dir2 = game::AsDirection(second);
      EXPECT_EQ(game::Dir_Diff(dir1, dir2), LegacyDirDiff(dir1, dir2))
          << first << " " << second;
    }
  }
}

}  // namespace

}  // namespace td
