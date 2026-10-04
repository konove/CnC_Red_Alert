// Tests the reinforcements against a real Game with no game data: the type
// objects, one house and the team types are built by hand.

#include "ra/ai/reinforcements.h"

#include "base/array.h"
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
#include "ra/map/world.h"
#include "ra/objects/aircraft.h"
#include "ra/objects/building.h"
#include "ra/objects/cell.h"
#include "ra/objects/house.h"
#include "ra/objects/infantry.h"
#include "ra/objects/techno.h"
#include "ra/objects/type.h"
#include "ra/objects/unit.h"
#include "ra/startup/game_state.h"
#include "ra/startup/game_test_util.h"

namespace ra {
namespace {

class ReinforcementTest : public GameFixture {
 protected:
  ReinforcementTest() {
    TheTypeHeaps().aircraft().Reset(magic_enum::enum_count<AircraftType>());
    TheTypeHeaps().infantry().Reset(magic_enum::enum_count<InfantryType>());
    TheTypeHeaps().building().Reset(magic_enum::enum_count<StructType>());
    TheTypeHeaps().tmplate().Reset(magic_enum::enum_count<TemplateType>());
    AircraftTypeClass::Init_Heap();
    InfantryTypeClass::Init_Heap();
    BuildingTypeClass::Init_Heap();
    TemplateTypeClass::Init_Heap();
    // No rules are loaded, so every type has no strength, and a team takes no
    // member that looks dead.
    UnitTypeClass::As_Reference(UNIT_JEEP).MaxStrength = 100;
    InfantryTypeClass::As_Reference(INFANTRY_E1).MaxStrength = 50;
    BuildingTypeClass::As_Reference(STRUCT_BARRACKS).MaxStrength = 500;
    BuildingTypeClass::As_Reference(STRUCT_AIRSTRIP).MaxStrength = 500;
    UnitTypeClass::As_Reference(UNIT_ANT1).MaxStrength = 100;
    AircraftTypeClass::As_Reference(AIRCRAFT_MIG).MaxStrength = 50;
    AircraftTypeClass::As_Reference(AIRCRAFT_HIND).MaxStrength = 50;
    AircraftTypeClass::As_Reference(AIRCRAFT_TRANSPORT).MaxStrength = 50;
    // Nor does any type carry passengers.
    UnitTypeClass& apc = UnitTypeClass::As_Reference(UNIT_APC);
    apc.MaxStrength = 200;
    apc.MaxPassengers = 5;
    AircraftTypeClass::As_Reference(AIRCRAFT_TRANSPORT).MaxPassengers = 5;
    TheObjectHeaps().aircraft().Reset(4);
    TheObjectHeaps().infantry().Reset(4);
    TheObjectHeaps().building().Reset(2);
    TheObjectHeaps().team().Reset(2);
    TheObjectHeaps().team_type().Reset(2);
    house_ = base::NewPooled<HouseClass>(HOUSE_USSR);
    // DeliverReinforcements() announces the player's own reinforcements.
    ThePlayer() = house_;
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

  // Adds `count` objects of `member_type` to `team_type`.
  static void AddMembers(TeamTypeClass& team_type,
                         const TechnoTypeClass& member_type, const int count) {
    TeamMemberClass& member = base::At(team_type.Members, team_type.ClassCount);
    member.Class = &member_type;
    member.Quantity = count;
    team_type.ClassCount++;
  }

  // Puts `object` on the map in `cell`.
  static void Place(TechnoClass* object, const Cell cell) {
    ASSERT_NE(object, nullptr);
    TheWorld().scenario_init()++;
    ASSERT_TRUE(object->Unlimbo(cell.Center(), game::DIR_N));
    TheWorld().scenario_init()--;
  }

  // The aircraft that exist and are not loaners.
  static int KeptAircraftCount() {
    int count = 0;
    for (int index = 0; index < TheObjectHeaps().aircraft().Count(); index++) {
      if (!TheObjectHeaps().aircraft().ActiveAt(index)->IsALoaner) {
        count++;
      }
    }
    return count;
  }

  HouseClass* house_ = nullptr;
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

TEST_F(ReinforcementTest, AnInfantryTeamIsLoadedIntoTheTransportAtItsOrigin) {
  TheGameState().active() = true;
  constexpr Cell kOrigin = Cell::FromXY(40, 50);
  TheScenario().Waypoint[0] = kOrigin;
  // A building at the origin and a transport next to it: the transport wins.
  Place(base::NewPooled<BuildingClass>(STRUCT_BARRACKS, HOUSE_USSR), kOrigin);
  auto* apc = base::NewPooled<UnitClass>(UNIT_APC, HOUSE_USSR);
  Place(apc, kOrigin.Offset(-1));
  TeamTypeClass& team_type =
      MakeTeamType(InfantryTypeClass::As_Reference(INFANTRY_E1), 2);
  team_type.Origin = 0;

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  EXPECT_EQ(apc->How_Many(), 2);
  // Queued: the mission starts on the transport's next turn.
  EXPECT_EQ(apc->MissionQueue, MISSION_UNLOAD);
  ASSERT_EQ(TheObjectHeaps().infantry().Count(), 2);
  for (int index = 0; index < 2; index++) {
    EXPECT_TRUE(TheObjectHeaps().infantry().ActiveAt(index)->IsInLimbo);
  }
  EXPECT_EQ(TheWorld().scenario_init(), 0);
}

TEST_F(ReinforcementTest, AVehicleTeamIgnoresTheTransportAtItsOrigin) {
  TheGameState().active() = true;
  TheRules().ground().at(LAND_CLEAR).Cost.at(SPEED_WHEEL) = Fixed(1);
  constexpr Cell kOrigin = Cell::FromXY(3, 20);
  TheScenario().Waypoint[0] = kOrigin;
  auto* apc = base::NewPooled<UnitClass>(UNIT_APC, HOUSE_USSR);
  Place(apc, kOrigin);
  TeamTypeClass& team_type =
      MakeTeamType(UnitTypeClass::As_Reference(UNIT_JEEP), 1);
  team_type.Origin = 0;

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  EXPECT_EQ(apc->How_Many(), 0);
  ASSERT_EQ(TheObjectHeaps().unit().Count(), 2);
  const UnitClass* jeep = TheObjectHeaps().unit().ActiveAt(1);
  EXPECT_FALSE(jeep->IsInLimbo);
  // In from the map edge nearest the origin.
  EXPECT_EQ(Cell::Containing(jeep->Coord).x(), 0);
}

TEST_F(ReinforcementTest, ATeamWithATransportArrivesLoaded) {
  TheGameState().active() = true;
  house_->Control.Edge = SOURCE_WEST;
  TeamTypeClass& team_type =
      MakeTeamType(InfantryTypeClass::As_Reference(INFANTRY_E1), 2);
  AddMembers(team_type, UnitTypeClass::As_Reference(UNIT_APC), 1);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().team().Count(), 1);
  const TeamClass* team = TheObjectHeaps().team().ActiveAt(0);
  ASSERT_EQ(TheObjectHeaps().unit().Count(), 1);
  const UnitClass* apc = TheObjectHeaps().unit().ActiveAt(0);
  EXPECT_FALSE(apc->IsInLimbo);
  EXPECT_EQ(Cell::Containing(apc->Coord).x(), 0);
  EXPECT_EQ(apc->How_Many(), 2);
  EXPECT_EQ(apc->Team, team);
  ASSERT_EQ(TheObjectHeaps().infantry().Count(), 2);
  for (int index = 0; index < 2; index++) {
    const InfantryClass* soldier = TheObjectHeaps().infantry().ActiveAt(index);
    EXPECT_TRUE(soldier->IsInLimbo);
    EXPECT_EQ(soldier->Team, team);
  }
  EXPECT_EQ(TheWorld().scenario_init(), 0);
}

TEST_F(ReinforcementTest, ALoadedHelicopterIsALoanerOnlyWhenTheTeamUnloads) {
  TheGameState().active() = true;
  const AircraftTypeClass& helicopter =
      AircraftTypeClass::As_Reference(AIRCRAFT_TRANSPORT);
  const InfantryTypeClass& soldier =
      InfantryTypeClass::As_Reference(INFANTRY_E1);
  // No airstrip, so every aircraft is a loaner when it is created; only the
  // empty transport helicopter is handed to the house afterwards.
  TeamTypeClass& unloading = MakeTeamType(helicopter, 1);
  AddMembers(unloading, soldier, 1);
  unloading.MissionCount = 1;
  unloading.MissionList[0].Mission = TMISSION_UNLOAD;
  TeamTypeClass& empty = MakeTeamType(helicopter, 1);

  EXPECT_TRUE(DeliverReinforcements(&unloading));
  ASSERT_EQ(TheObjectHeaps().aircraft().Count(), 1);
  EXPECT_EQ(TheObjectHeaps().aircraft().ActiveAt(0)->How_Many(), 1);
  EXPECT_EQ(KeptAircraftCount(), 0);

  EXPECT_TRUE(DeliverReinforcements(&empty));
  ASSERT_EQ(TheObjectHeaps().aircraft().Count(), 2);
  EXPECT_FALSE(TheObjectHeaps().aircraft().ActiveAt(1)->IsALoaner);
}

TEST_F(ReinforcementTest, AnAntTeamComesOutOfTheAntHillAtItsOrigin) {
  TheGameState().active() = true;
  constexpr Cell kHill = Cell::FromXY(40, 50);
  TheScenario().Waypoint[0] = kHill;
  TheMap().at(kHill).TType = TEMPLATE_HILL01;
  TeamTypeClass& team_type =
      MakeTeamType(UnitTypeClass::As_Reference(UNIT_ANT1), 1);
  team_type.Origin = 0;

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().unit().Count(), 1);
  const UnitClass* ant = TheObjectHeaps().unit().ActiveAt(0);
  EXPECT_FALSE(ant->IsInLimbo);
  EXPECT_EQ(Cell::Containing(ant->Coord), kHill);
}

TEST_F(ReinforcementTest, AnAntTeamWithoutAnAntHillEntersFromTheEdge) {
  TheGameState().active() = true;
  TheRules().ground().at(LAND_CLEAR).Cost.at(SPEED_WHEEL) = Fixed(1);
  TheRules().ground().at(LAND_CLEAR).Cost.at(SPEED_FOOT) = Fixed(1);
  TheScenario().Waypoint[0] = Cell::FromXY(3, 20);
  TeamTypeClass& ants = MakeTeamType(UnitTypeClass::As_Reference(UNIT_ANT1), 1);
  ants.Origin = 0;
  // Only ants use the hill.
  TheScenario().Waypoint[1] = Cell::FromXY(3, 40);
  TheMap().at(TheScenario().Waypoint[1]).TType = TEMPLATE_HILL01;
  TeamTypeClass& jeeps =
      MakeTeamType(UnitTypeClass::As_Reference(UNIT_JEEP), 1);
  jeeps.Origin = 1;

  EXPECT_TRUE(DeliverReinforcements(&ants));
  EXPECT_TRUE(DeliverReinforcements(&jeeps));

  ASSERT_EQ(TheObjectHeaps().unit().Count(), 2);
  for (int index = 0; index < 2; index++) {
    const UnitClass* unit = TheObjectHeaps().unit().ActiveAt(index);
    EXPECT_FALSE(unit->IsInLimbo);
    EXPECT_EQ(Cell::Containing(unit->Coord).x(), 0);
  }
}

TEST_F(ReinforcementTest, AHouseWithoutAnAirstripBorrowsItsAircraft) {
  TheGameState().active() = true;
  TeamTypeClass& team_type =
      MakeTeamType(AircraftTypeClass::As_Reference(AIRCRAFT_MIG), 1);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().aircraft().Count(), 1);
  EXPECT_FALSE(TheObjectHeaps().aircraft().ActiveAt(0)->IsInLimbo);
  EXPECT_EQ(KeptAircraftCount(), 0);
}

TEST_F(ReinforcementTest, AHouseKeepsOneMigForEachAirstrip) {
  TheGameState().active() = true;
  Place(base::NewPooled<BuildingClass>(STRUCT_AIRSTRIP, HOUSE_USSR),
        Cell::FromXY(40, 50));
  ASSERT_EQ(house_->Get_Quantity(STRUCT_AIRSTRIP), 1);
  TeamTypeClass& team_type =
      MakeTeamType(AircraftTypeClass::As_Reference(AIRCRAFT_MIG), 2);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().aircraft().Count(), 2);
  EXPECT_EQ(KeptAircraftCount(), 1);
}

TEST_F(ReinforcementTest, AnAirstripKeepsNoHelicopter) {
  TheGameState().active() = true;
  Place(base::NewPooled<BuildingClass>(STRUCT_AIRSTRIP, HOUSE_USSR),
        Cell::FromXY(40, 50));
  TeamTypeClass& team_type =
      MakeTeamType(AircraftTypeClass::As_Reference(AIRCRAFT_HIND), 1);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().aircraft().Count(), 1);
  EXPECT_EQ(KeptAircraftCount(), 0);
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
