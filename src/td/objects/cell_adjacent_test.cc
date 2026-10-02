// Checks CellClass::Adjacent_Cell's map-edge handling on a real cell array.
#include "base/installed.h"
#include "game/sim/direction.h"
#include "gtest/gtest.h"
#include "td/core/defines.h"
#include "td/core/inline.h"
#include "td/map/world.h"
#include "td/objects/cell.h"

namespace td {

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
    TheMap().Replace_Cells(MAP_CELL_TOTAL);
    TheMap().Init_Cells();
  }
  static void TearDownTestSuite() { TheMap().Free_Cells(); }

  static const CellClass& Cell(int x, int y) {
    return TheMap().at(XY_Cell(x, y));
  }
};

TEST_F(TdAdjacentCellTest, InteriorCellReturnsEachNeighbour) {
  const CellClass& center = Cell(10, 10);
  EXPECT_EQ(&center.Adjacent_Cell(game::FACING_N), &Cell(10, 9));
  EXPECT_EQ(&center.Adjacent_Cell(game::FACING_NE), &Cell(11, 9));
  EXPECT_EQ(&center.Adjacent_Cell(game::FACING_E), &Cell(11, 10));
  EXPECT_EQ(&center.Adjacent_Cell(game::FACING_SE), &Cell(11, 11));
  EXPECT_EQ(&center.Adjacent_Cell(game::FACING_S), &Cell(10, 11));
  EXPECT_EQ(&center.Adjacent_Cell(game::FACING_SW), &Cell(9, 11));
  EXPECT_EQ(&center.Adjacent_Cell(game::FACING_W), &Cell(9, 10));
  EXPECT_EQ(&center.Adjacent_Cell(game::FACING_NW), &Cell(9, 9));
}

TEST_F(TdAdjacentCellTest, StepOffTopOfArrayReturnsSelf) {
  const CellClass& top = Cell(5, 0);
  EXPECT_EQ(&top.Adjacent_Cell(game::FACING_N), &top);
  EXPECT_EQ(&top.Adjacent_Cell(game::FACING_NE), &top);
  EXPECT_EQ(&top.Adjacent_Cell(game::FACING_NW), &top);
  EXPECT_EQ(&top.Adjacent_Cell(game::FACING_S), &Cell(5, 1));
}

TEST_F(TdAdjacentCellTest, StepOffBottomOfArrayReturnsSelf) {
  const CellClass& last = TheMap().at(static_cast<CELL>(MAP_CELL_TOTAL - 1));
  EXPECT_EQ(&last.Adjacent_Cell(game::FACING_S), &last);
  EXPECT_EQ(&last.Adjacent_Cell(game::FACING_SE), &last);
  EXPECT_EQ(&last.Adjacent_Cell(game::FACING_SW), &last);
  EXPECT_EQ(&last.Adjacent_Cell(game::FACING_E), &last);
  EXPECT_EQ(&last.Adjacent_Cell(game::FACING_W),
            &TheMap().at(static_cast<CELL>(MAP_CELL_TOTAL - 2)));
}

TEST_F(TdAdjacentCellTest, FirstCellStepWestReturnsSelf) {
  const CellClass& first = Cell(0, 0);
  EXPECT_EQ(&first.Adjacent_Cell(game::FACING_W), &first);
  EXPECT_EQ(&first.Adjacent_Cell(game::FACING_E), &Cell(1, 0));
}

TEST_F(TdAdjacentCellTest, InvalidFacingReturnsSelf) {
  const CellClass& center = Cell(10, 10);
  EXPECT_EQ(&center.Adjacent_Cell(game::FACING_NONE), &center);
  // One past FACING_NW, the value the old FACING_COUNT sentinel held.
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  const auto past_last = static_cast<game::FacingType>(8);
  EXPECT_EQ(&center.Adjacent_Cell(past_last), &center);
}

}  // namespace

}  // namespace td
