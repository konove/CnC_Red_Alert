// Checks CellClass::Adjacent_Cell's map-edge handling on a real cell array.
#include "base/installed.h"
#include "game/sim/direction.h"
#include "gtest/gtest.h"
#include "td/core/defines.h"
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

  static const CellClass& CellAt(int x, int y) {
    return TheMap().at(Cell::FromXY(x, y));
  }
};

TEST_F(TdAdjacentCellTest, InteriorCellReturnsEachNeighbour) {
  const CellClass& center = CellAt(10, 10);
  EXPECT_EQ(&center.Adjacent_Cell(game::FacingType::kNorth), &CellAt(10, 9));
  EXPECT_EQ(&center.Adjacent_Cell(game::FacingType::kNorthEast),
            &CellAt(11, 9));
  EXPECT_EQ(&center.Adjacent_Cell(game::FacingType::kEast), &CellAt(11, 10));
  EXPECT_EQ(&center.Adjacent_Cell(game::FacingType::kSouthEast),
            &CellAt(11, 11));
  EXPECT_EQ(&center.Adjacent_Cell(game::FacingType::kSouth), &CellAt(10, 11));
  EXPECT_EQ(&center.Adjacent_Cell(game::FacingType::kSouthWest),
            &CellAt(9, 11));
  EXPECT_EQ(&center.Adjacent_Cell(game::FacingType::kWest), &CellAt(9, 10));
  EXPECT_EQ(&center.Adjacent_Cell(game::FacingType::kNorthWest), &CellAt(9, 9));
}

TEST_F(TdAdjacentCellTest, StepOffTopOfArrayReturnsSelf) {
  const CellClass& top = CellAt(5, 0);
  EXPECT_EQ(&top.Adjacent_Cell(game::FacingType::kNorth), &top);
  EXPECT_EQ(&top.Adjacent_Cell(game::FacingType::kNorthEast), &top);
  EXPECT_EQ(&top.Adjacent_Cell(game::FacingType::kNorthWest), &top);
  EXPECT_EQ(&top.Adjacent_Cell(game::FacingType::kSouth), &CellAt(5, 1));
}

TEST_F(TdAdjacentCellTest, StepOffBottomOfArrayReturnsSelf) {
  const CellClass& last = TheMap().at(Cell::FromIndex(MAP_CELL_TOTAL - 1));
  EXPECT_EQ(&last.Adjacent_Cell(game::FacingType::kSouth), &last);
  EXPECT_EQ(&last.Adjacent_Cell(game::FacingType::kSouthEast), &last);
  EXPECT_EQ(&last.Adjacent_Cell(game::FacingType::kSouthWest), &last);
  EXPECT_EQ(&last.Adjacent_Cell(game::FacingType::kEast), &last);
  EXPECT_EQ(&last.Adjacent_Cell(game::FacingType::kWest),
            &TheMap().at(Cell::FromIndex(MAP_CELL_TOTAL - 2)));
}

TEST_F(TdAdjacentCellTest, FirstCellStepWestReturnsSelf) {
  const CellClass& first = CellAt(0, 0);
  EXPECT_EQ(&first.Adjacent_Cell(game::FacingType::kWest), &first);
  EXPECT_EQ(&first.Adjacent_Cell(game::FacingType::kEast), &CellAt(1, 0));
}

TEST_F(TdAdjacentCellTest, InvalidFacingReturnsSelf) {
  const CellClass& center = CellAt(10, 10);
  EXPECT_EQ(&center.Adjacent_Cell(game::FacingType::kNone), &center);
  // One past FacingType::kNorthWest, the value the old FACING_COUNT sentinel
  // held. NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  const auto past_last = static_cast<game::FacingType>(8);
  EXPECT_EQ(&center.Adjacent_Cell(past_last), &center);
}

}  // namespace

}  // namespace td
