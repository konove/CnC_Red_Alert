// Tests the air strike against a real Game with no game data: the type
// objects and one house are built by hand.

#include "ra/ai/reinf.h"

#include "base/pooled.h"
#include "gtest/gtest.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/core/target.h"
#include "ra/core/type_heaps.h"
#include "ra/map/map.h"
#include "ra/map/world.h"
#include "ra/objects/house.h"
#include "ra/objects/type.h"
#include "ra/startup/game.h"
#include "ra/startup/game_state.h"

namespace ra {
namespace {

class ReinforcementTest : public ::testing::Test {
 protected:
  ReinforcementTest() {
    TheTypeHeaps().house().Reset(magic_enum::enum_count<HousesType>());
    TheTypeHeaps().aircraft().Reset(magic_enum::enum_count<AircraftType>());
    HouseTypeClass::Init_Heap();
    AircraftTypeClass::Init_Heap();
    TheObjectHeaps().house().Reset(2);
    TheObjectHeaps().aircraft().Reset(4);
    house_ = base::NewPooled<HouseClass>(HOUSE_USSR);
    // The map layer alone: the display layers above it load game data.
    TheMap().MapClass::One_Time();
    TheMap().MapClass::Set_Map_Dimensions(1, 1, MAP_CELL_W - 2, MAP_CELL_H - 2);
  }

  HouseClass* house_ = nullptr;

 private:
  Game game_;
};

TEST_F(ReinforcementTest, AnAirStrikeThatCannotBePlacedCountsNoAircraft) {
  // Nothing leaves limbo while the game is not active.
  TheGameState().active() = false;

  EXPECT_EQ(Create_Air_Reinforcement(house_, AIRCRAFT_BADGER, 2, MISSION_HUNT,
                                     Target{}, Target{}),
            0);
  EXPECT_EQ(TheObjectHeaps().aircraft().Count(), 0);
}

}  // namespace
}  // namespace ra
