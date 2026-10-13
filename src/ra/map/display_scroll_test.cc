// Tests how DisplayClass::Scroll_Map() stops the tactical view at the edges
// of the map, against the map of a real Game.

#include "game/sim/direction.h"
#include "gtest/gtest.h"
#include "ra/core/coordinate.h"
#include "ra/core/packed.h"
#include "ra/map/display.h"
#include "ra/map/world.h"
#include "ra/startup/game_test_util.h"

namespace ra {
namespace {

// A view of 20 by 10 cells on the fixture's map, which starts at cell (1, 1).
class DisplayScrollTest : public GameFixture {
 protected:
  void SetUp() override {
    GameFixture::SetUp();
    TheMap().TacLeptonWidth = Cell_To_Lepton(20);
    TheMap().TacLeptonHeight = Cell_To_Lepton(10);
  }

  // Asks how far the view at `from` can scroll, without moving it. Returns
  // the distance Scroll_Map() left, or 0 if it reported no scroll at all.
  static int ScrollableDistance(const Coordinate from,
                                const game::Direction facing, int distance) {
    TheMap().TacticalCoord = from;
    const bool scrolls =
        TheMap().DisplayClass::Scroll_Map(facing, distance, false);
    return scrolls ? distance : 0;
  }

  static constexpr int kMapLeft = 0x100;
  static constexpr int kMapTop = 0x100;
};

TEST_F(DisplayScrollTest, ScrollInsideTheMapKeepsItsDistance) {
  const Coordinate from = Coordinate::FromLeptons(kMapLeft + 1000, kMapTop);

  EXPECT_EQ(ScrollableDistance(from, game::Direction::kEast, 500), 500);
  EXPECT_EQ(ScrollableDistance(from, game::Direction::kWest, 500), 500);
}

// The target is left of lepton 0 here: read unsigned it would be far to the
// right, and the view would jump to the map's right edge.
TEST_F(DisplayScrollTest, ScrollPastTheLeftEdgeStopsAtIt) {
  const Coordinate from = Coordinate::FromLeptons(kMapLeft + 100, kMapTop);

  EXPECT_EQ(ScrollableDistance(from, game::Direction::kWest, 500), 100);
}

TEST_F(DisplayScrollTest, ScrollPastTheTopEdgeStopsAtIt) {
  const Coordinate from = Coordinate::FromLeptons(kMapLeft, kMapTop + 100);

  EXPECT_EQ(ScrollableDistance(from, game::Direction::kNorth, 500), 100);
}

TEST_F(DisplayScrollTest, ScrollAtTheEdgeDoesNothing) {
  const Coordinate from = Coordinate::FromLeptons(kMapLeft, kMapTop);

  EXPECT_EQ(ScrollableDistance(from, game::Direction::kWest, 500), 0);
  EXPECT_EQ(ScrollableDistance(from, game::Direction::kNorth, 500), 0);
}

}  // namespace
}  // namespace ra
