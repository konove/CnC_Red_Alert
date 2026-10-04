// Tests the reinforcements against a real Game with no game data: the type
// objects, one house and the team types are built by hand.

#include "ra/ai/reinforcements.h"

#include "base/fixed.h"
#include "base/pooled.h"
#include "game/sim/direction.h"
#include "gtest/gtest.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/scenario.h"
#include "ra/ai/team.h"
#include "ra/ai/teamtype.h"
#include "ra/core/cell.h"
#include "ra/core/coord.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/core/target.h"
#include "ra/core/type_heaps.h"
#include "ra/data/rules.h"
#include "ra/map/display.h"
#include "ra/map/layer.h"
#include "ra/map/map.h"
#include "ra/map/world.h"
#include "ra/objects/building.h"
#include "ra/objects/house.h"
#include "ra/objects/infantry.h"
#include "ra/objects/type.h"
#include "ra/objects/unit.h"
#include "ra/startup/game.h"
#include "ra/startup/game_state.h"

namespace ra {
namespace {

class ReinforcementTest : public ::testing::Test {
 protected:
  ReinforcementTest() {
    TheTypeHeaps().house().Reset(magic_enum::enum_count<HousesType>());
    TheTypeHeaps().aircraft().Reset(magic_enum::enum_count<AircraftType>());
    TheTypeHeaps().unit().Reset(magic_enum::enum_count<UnitType>());
    TheTypeHeaps().infantry().Reset(magic_enum::enum_count<InfantryType>());
    TheTypeHeaps().building().Reset(magic_enum::enum_count<StructType>());
    HouseTypeClass::Init_Heap();
    AircraftTypeClass::Init_Heap();
    UnitTypeClass::Init_Heap();
    InfantryTypeClass::Init_Heap();
    BuildingTypeClass::Init_Heap();
    // No rules are loaded, so every type has no strength, and a team takes no
    // member that looks dead.
    UnitTypeClass::As_Reference(UNIT_JEEP).MaxStrength = 100;
    InfantryTypeClass::As_Reference(INFANTRY_E1).MaxStrength = 50;
    BuildingTypeClass::As_Reference(STRUCT_BARRACKS).MaxStrength = 500;
    TheObjectHeaps().house().Reset(2);
    TheObjectHeaps().aircraft().Reset(4);
    TheObjectHeaps().unit().Reset(4);
    TheObjectHeaps().infantry().Reset(4);
    TheObjectHeaps().building().Reset(2);
    TheObjectHeaps().team().Reset(2);
    TheObjectHeaps().team_type().Reset(2);
    house_ = base::NewPooled<HouseClass>(HOUSE_USSR);
    // DeliverReinforcements() announces the player's own reinforcements.
    ThePlayer() = house_;
    // The map layer alone: the display layers above it load game data.
    TheMap().MapClass::One_Time();
    TheMap().MapClass::Set_Map_Dimensions(1, 1, MAP_CELL_W - 2, MAP_CELL_H - 2);
  }

  // The display layers are static and outlive the Game, so they would keep the
  // objects a test put on the map.
  void TearDown() override {
    for (LayerClass& layer : DisplayClass::Layer) {
      layer.Clear();
    }
  }

  // A team type for the house of `count` objects of `member_type`, with no
  // missions and no origin waypoint.
  static TeamTypeClass& MakeTeamType(const TechnoTypeClass& member_type,
                                     const int count) {
    auto* team_type = base::NewPooled<TeamTypeClass>();
    EXPECT_NE(team_type, nullptr);
    team_type->House = HOUSE_USSR;
    team_type->ClassCount = 1;
    team_type->Members[0].Class = &member_type;
    team_type->Members[0].Quantity = count;
    return *team_type;
  }

  HouseClass* house_ = nullptr;

 private:
  Game game_;
};

TEST_F(ReinforcementTest, AnAirStrikeThatCannotBePlacedCountsNoAircraft) {
  // Nothing leaves limbo while the game is not active.
  TheGameState().active() = false;

  EXPECT_EQ(LaunchAirStrike(house_, AIRCRAFT_BADGER, 2, MISSION_HUNT, Target{},
                            Target{}),
            0);
  EXPECT_EQ(TheObjectHeaps().aircraft().Count(), 0);
}

TEST_F(ReinforcementTest, ATeamThatCannotBePlacedIsDeleted) {
  // Nothing leaves limbo while the game is not active, so every placement
  // fails, whichever edge cell is tried.
  TheGameState().active() = false;
  house_->Control.Edge = SOURCE_WEST;
  TeamTypeClass& team_type =
      MakeTeamType(UnitTypeClass::As_Reference(UNIT_JEEP), 2);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  EXPECT_EQ(TheObjectHeaps().unit().Count(), 0);
  EXPECT_EQ(TheWorld().scenario_init(), 0);
}

TEST_F(ReinforcementTest, GivesUpOnATeamThatNoEdgeCellTakes) {
  // The team enters at the west edge next to its origin, in column 0. With a
  // vehicle in the cell to the north, the next cell to try is the one to the
  // south, and from there the first one again.
  TheRules().ground().at(LAND_CLEAR).Cost.at(SPEED_WHEEL) = Fixed(1);
  TheScenario().Waypoint[0] = Cell::FromXY(3, 20);
  TheGameState().active() = true;
  auto* blocker = base::NewPooled<UnitClass>(UNIT_JEEP, HOUSE_USSR);
  ASSERT_NE(blocker, nullptr);
  TheWorld().scenario_init()++;
  ASSERT_TRUE(blocker->Unlimbo(Cell::FromXY(0, 19).Center(), game::DIR_N));
  TheWorld().scenario_init()--;
  // Nothing leaves limbo while the game is not active, so every cell fails.
  TheGameState().active() = false;
  TeamTypeClass& team_type =
      MakeTeamType(UnitTypeClass::As_Reference(UNIT_JEEP), 2);
  team_type.Origin = 0;

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  // Only the vehicle that was in the way is left.
  EXPECT_EQ(TheObjectHeaps().unit().Count(), 1);
  EXPECT_EQ(TheWorld().scenario_init(), 0);
}

TEST_F(ReinforcementTest, AVehicleTeamEntersFromTheHouseEdge) {
  TheGameState().active() = true;
  house_->Control.Edge = SOURCE_WEST;
  TeamTypeClass& team_type =
      MakeTeamType(UnitTypeClass::As_Reference(UNIT_JEEP), 2);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().unit().Count(), 2);
  ASSERT_EQ(TheObjectHeaps().team().Count(), 1);
  const TeamClass* team = TheObjectHeaps().team().ActiveAt(0);
  for (int index = 0; index < 2; index++) {
    const UnitClass* unit = TheObjectHeaps().unit().ActiveAt(index);
    EXPECT_FALSE(unit->IsInLimbo);
    EXPECT_EQ(unit->Owner(), HOUSE_USSR);
    EXPECT_EQ(unit->Team, team);
    // One cell west of the visible map, which starts at column 1.
    EXPECT_EQ(Cell::Containing(unit->Coord).x(), 0);
  }
  EXPECT_EQ(TheWorld().scenario_init(), 0);
}

TEST_F(ReinforcementTest, AnInfantryTeamLeavesTheBuildingAtItsOrigin) {
  TheGameState().active() = true;
  constexpr Cell kOrigin = Cell::FromXY(40, 50);
  // No rules are loaded, so no ground can be crossed; the soldiers need an
  // exit cell they can walk onto.
  TheRules().ground().at(LAND_CLEAR).Cost.at(SPEED_FOOT) = Fixed(1);
  TheScenario().Waypoint[0] = kOrigin;
  auto* barracks = base::NewPooled<BuildingClass>(STRUCT_BARRACKS, HOUSE_USSR);
  ASSERT_NE(barracks, nullptr);
  TheWorld().scenario_init()++;
  ASSERT_TRUE(barracks->Unlimbo(kOrigin.Center(), game::DIR_N));
  TheWorld().scenario_init()--;
  TeamTypeClass& team_type =
      MakeTeamType(InfantryTypeClass::As_Reference(INFANTRY_E1), 2);
  team_type.Origin = 0;

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().infantry().Count(), 2);
  for (int index = 0; index < 2; index++) {
    const InfantryClass* soldier = TheObjectHeaps().infantry().ActiveAt(index);
    EXPECT_FALSE(soldier->IsInLimbo);
    // Out of the building, not in from the map edge.
    EXPECT_LE(Distance(soldier->Coord, kOrigin.Center()), 4 * 256);
  }
  EXPECT_EQ(TheWorld().scenario_init(), 0);
}

TEST_F(ReinforcementTest, ATeamTypeWithoutMissionsAttacksItsOrigin) {
  TheGameState().active() = true;
  TeamTypeClass& team_type =
      MakeTeamType(UnitTypeClass::As_Reference(UNIT_JEEP), 1);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(team_type.MissionCount, 1);
  EXPECT_EQ(team_type.MissionList[0].Mission, TMISSION_ATT_WAYPT);
  EXPECT_EQ(team_type.MissionList[0].Data.Value, -1);
}

TEST_F(ReinforcementTest, ATeamTypeWithoutMembersDeliversNothing) {
  TeamTypeClass& team_type =
      MakeTeamType(UnitTypeClass::As_Reference(UNIT_JEEP), 1);
  team_type.ClassCount = 0;

  EXPECT_FALSE(DeliverReinforcements(&team_type));

  EXPECT_EQ(TheObjectHeaps().team().Count(), 0);
}

}  // namespace
}  // namespace ra
