// Tests UnitClass against a real Game with no game data: a house and its
// vehicles are built by hand.

#include "ra/objects/unit.h"

#include "base/pooled.h"
#include "game/sim/direction.h"
#include "gtest/gtest.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/map/world.h"
#include "ra/objects/house.h"
#include "ra/startup/game_test_util.h"

namespace ra {
namespace {

class UnitTest : public GameFixture {
 protected:
  UnitTest() {
    // The fixture leaves room for four vehicles.
    TheObjectHeaps().unit().Reset(8);
    ThePlayer() = base::NewPooled<HouseClass>(HOUSE_GREECE);
  }

  // An APC in limbo that faces `facing`.
  static UnitClass& MakeApc(const game::Direction facing) {
    auto* apc = base::NewPooled<UnitClass>(UNIT_APC, HOUSE_GREECE);
    EXPECT_NE(apc, nullptr);
    apc->PrimaryFacing.set_current(facing);
    return *apc;
  }

  // Runs the door of `apc` until `done` says it has arrived and returns the
  // game frames that took. Every frame on the way has to be one of the three
  // the door has: the draw code adds it to shape 32 (north-east) or 35
  // (north-west), and the door's shapes end at 37.
  template <class Done>
  static int FramesUntil(UnitClass& apc, const Done done) {
    int frames = 0;
    while (!done(apc.door()) && frames < kTooManyFrames) {
      apc.door().AI();
      ++frames;
      EXPECT_GE(apc.door().Frame(), 0);
      EXPECT_LT(apc.door().Frame(), kDoorFrames);
    }
    return frames;
  }

  static int FramesToOpen(UnitClass& apc) {
    return FramesUntil(apc,
                       [](const game::Door& door) { return door.IsOpen(); });
  }

  static int FramesToClose(UnitClass& apc) {
    return FramesUntil(apc,
                       [](const game::Door& door) { return door.IsClosed(); });
  }

  static constexpr int kDoorFrames = 3;
  // More than any door animation lasts: a door that never arrives ends here.
  static constexpr int kTooManyFrames = 100;
};

// The door is drawn at these two facings only, three game frames a stage.
TEST_F(UnitTest, AnApcDoorOpensInNineFramesWhereItIsDrawn) {
  for (const game::Direction facing :
       {game::Direction::kNorthEast, game::Direction::kNorthWest}) {
    UnitClass& apc = MakeApc(facing);
    apc.APC_Open_Door();
    EXPECT_TRUE(apc.door().IsOpening());
    EXPECT_EQ(FramesToOpen(apc), 9);
    EXPECT_EQ(apc.door().Frame(), kDoorFrames - 1);
  }
}

TEST_F(UnitTest, AnApcDoorOpensInThreeFramesWhereItIsNotDrawn) {
  for (const game::Direction facing :
       {game::Direction::kNorth, game::Direction::kEast,
        game::Direction::kSouthEast, game::Direction::kSouth,
        game::Direction::kSouthWest, game::Direction::kWest}) {
    UnitClass& apc = MakeApc(facing);
    apc.APC_Open_Door();
    EXPECT_EQ(FramesToOpen(apc), 3);
  }
}

TEST_F(UnitTest, AnApcDoorStaysShutWhileTheApcDrivesOrTurns) {
  UnitClass& driving = MakeApc(game::Direction::kNorthEast);
  driving.IsDriving = true;
  driving.APC_Open_Door();
  EXPECT_TRUE(driving.door().IsClosed());

  UnitClass& turning = MakeApc(game::Direction::kNorthEast);
  turning.IsRotating = true;
  turning.APC_Open_Door();
  EXPECT_TRUE(turning.door().IsClosed());
}

// Closing takes the drawn door's time at every facing.
TEST_F(UnitTest, AnApcDoorClosesInNineFrames) {
  for (const game::Direction facing :
       {game::Direction::kNorthEast, game::Direction::kSouth}) {
    UnitClass& apc = MakeApc(facing);
    apc.APC_Open_Door();
    FramesToOpen(apc);
    apc.APC_Close_Door();
    EXPECT_EQ(FramesToClose(apc), 9);
    EXPECT_EQ(apc.door().Frame(), 0);
  }
}

}  // namespace
}  // namespace ra
