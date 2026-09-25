// Checks CellClass::Adjacent_Cell's map-edge handling on a real cell array.
#include "engine/base/installed.h"
#include "gtest/gtest.h"
#include "td/cell.h"
#include "td/defines.h"
#include "td/inline.h"
#include "td/world.h"

namespace {

// The map these tests walk; Game owns the World that holds it in the real
// game.
// NOLINTBEGIN(bugprone-throwing-static-initialization): a test binary
// that runs out of memory building the world has nothing to report.
World world;
const base::Installed<World>::Scope world_scope(world);
// NOLINTEND(bugprone-throwing-static-initialization)

class TdAdjacentCellTest : public testing::Test {
 protected:
  static void SetUpTestSuite() {
    TheMap().Resize(MAP_CELL_TOTAL);
    TheMap().Init_Cells();
  }
  static void TearDownTestSuite() { TheMap().Clear(); }

  static const CellClass& Cell(int x, int y) {
    return TheMap().at(XY_Cell(x, y));
  }
};

TEST_F(TdAdjacentCellTest, InteriorCellReturnsEachNeighbour) {
  const CellClass& center = Cell(10, 10);
  EXPECT_EQ(&center.Adjacent_Cell(FACING_N), &Cell(10, 9));
  EXPECT_EQ(&center.Adjacent_Cell(FACING_NE), &Cell(11, 9));
  EXPECT_EQ(&center.Adjacent_Cell(FACING_E), &Cell(11, 10));
  EXPECT_EQ(&center.Adjacent_Cell(FACING_SE), &Cell(11, 11));
  EXPECT_EQ(&center.Adjacent_Cell(FACING_S), &Cell(10, 11));
  EXPECT_EQ(&center.Adjacent_Cell(FACING_SW), &Cell(9, 11));
  EXPECT_EQ(&center.Adjacent_Cell(FACING_W), &Cell(9, 10));
  EXPECT_EQ(&center.Adjacent_Cell(FACING_NW), &Cell(9, 9));
}

TEST_F(TdAdjacentCellTest, StepOffTopOfArrayReturnsSelf) {
  const CellClass& top = Cell(5, 0);
  EXPECT_EQ(&top.Adjacent_Cell(FACING_N), &top);
  EXPECT_EQ(&top.Adjacent_Cell(FACING_NE), &top);
  EXPECT_EQ(&top.Adjacent_Cell(FACING_NW), &top);
  EXPECT_EQ(&top.Adjacent_Cell(FACING_S), &Cell(5, 1));
}

TEST_F(TdAdjacentCellTest, StepOffBottomOfArrayReturnsSelf) {
  const CellClass& last = TheMap().at(static_cast<CELL>(MAP_CELL_TOTAL - 1));
  EXPECT_EQ(&last.Adjacent_Cell(FACING_S), &last);
  EXPECT_EQ(&last.Adjacent_Cell(FACING_SE), &last);
  EXPECT_EQ(&last.Adjacent_Cell(FACING_SW), &last);
  EXPECT_EQ(&last.Adjacent_Cell(FACING_E), &last);
  EXPECT_EQ(&last.Adjacent_Cell(FACING_W),
            &TheMap().at(static_cast<CELL>(MAP_CELL_TOTAL - 2)));
}

TEST_F(TdAdjacentCellTest, FirstCellStepWestReturnsSelf) {
  const CellClass& first = Cell(0, 0);
  EXPECT_EQ(&first.Adjacent_Cell(FACING_W), &first);
  EXPECT_EQ(&first.Adjacent_Cell(FACING_E), &Cell(1, 0));
}

TEST_F(TdAdjacentCellTest, InvalidFacingReturnsSelf) {
  const CellClass& center = Cell(10, 10);
  EXPECT_EQ(&center.Adjacent_Cell(FACING_NONE), &center);
  EXPECT_EQ(&center.Adjacent_Cell(FACING_COUNT), &center);
}

}  // namespace
