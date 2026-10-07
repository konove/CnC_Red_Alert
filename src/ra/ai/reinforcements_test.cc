// Tests the reinforcements against a real Game with no game data: the type
// objects, one house and the team types are built by hand.

#include "ra/ai/reinforcements.h"

#include <vector>

#include "base/fixed.h"
#include "base/pooled.h"
#include "game/sim/direction.h"
#include "gtest/gtest.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/scenario.h"
#include "ra/ai/team.h"
#include "ra/ai/team_type.h"
#include "ra/core/cell.h"
#include "ra/core/coord.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/core/target.h"
#include "ra/core/type_heaps.h"
#include "ra/data/rules.h"
#include "ra/map/display.h"
#include "ra/map/world.h"
#include "ra/objects/aircraft.h"
#include "ra/objects/building.h"
#include "ra/objects/cell.h"
#include "ra/objects/house.h"
#include "ra/objects/infantry.h"
#include "ra/objects/object.h"
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
    AircraftTypeClass::As_Reference(AIRCRAFT_YAK).MaxStrength = 50;
    AircraftTypeClass::As_Reference(AIRCRAFT_BADGER).MaxStrength = 50;
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
  static TeamType& MakeTeamType(const TechnoTypeClass& member_type,
                                const int count) {
    auto* team_type = base::NewPooled<TeamType>();
    EXPECT_NE(team_type, nullptr);
    team_type->set_house(HOUSE_USSR);
    team_type->AddMember({.quantity = count, .type = &member_type});
    return *team_type;
  }

  // Adds `count` objects of `member_type` to `team_type`.
  static void AddMembers(TeamType& team_type,
                         const TechnoTypeClass& member_type, const int count) {
    team_type.AddMember({.quantity = count, .type = &member_type});
  }

  // Puts `object` on the map in `cell`.
  static void Place(TechnoClass* object, const Cell cell) {
    ASSERT_NE(object, nullptr);
    TheWorld().scenario_init()++;
    ASSERT_TRUE(object->Unlimbo(cell.Center(), game::Direction::kNorth));
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

TEST_F(ReinforcementTest, AnAirStrikeArrivesWithItsOrders) {
  TheGameState().active() = true;
  house_->Control.Edge = SOURCE_WEST;
  const Target target = Target::FromCell(Cell::FromXY(40, 50));
  const Target destination = Target::FromCell(Cell::FromXY(60, 70));

  EXPECT_EQ(LaunchAirStrike(house_, AIRCRAFT_BADGER, 2, MISSION_HUNT, target,
                            destination),
            2);

  ASSERT_EQ(TheObjectHeaps().aircraft().Count(), 2);
  for (int index = 0; index < 2; index++) {
    const AircraftClass* bomber = TheObjectHeaps().aircraft().ActiveAt(index);
    EXPECT_FALSE(bomber->IsInLimbo);
    EXPECT_EQ(bomber->Owner(), HOUSE_USSR);
    // It leaves the map when it is done.
    EXPECT_TRUE(bomber->IsALoaner);
    EXPECT_EQ(Cell::Containing(bomber->Coord).x(), 0);
    EXPECT_EQ(bomber->Mission, MISSION_HUNT);
    EXPECT_EQ(bomber->TarCom, target);
    EXPECT_EQ(bomber->NavCom, destination);
  }
  EXPECT_EQ(TheWorld().scenario_init(), 0);
}

TEST_F(ReinforcementTest, AnAirStrikeWithoutOrdersHasNoTargetOrDestination) {
  TheGameState().active() = true;

  EXPECT_EQ(LaunchAirStrike(house_, AIRCRAFT_BADGER, 1, MISSION_HUNT, Target{},
                            Target{}),
            1);

  ASSERT_EQ(TheObjectHeaps().aircraft().Count(), 1);
  const AircraftClass* bomber = TheObjectHeaps().aircraft().ActiveAt(0);
  EXPECT_EQ(bomber->Mission, MISSION_HUNT);
  EXPECT_EQ(bomber->TarCom, Target{});
  EXPECT_EQ(bomber->NavCom, Target{});
}

TEST_F(ReinforcementTest, AnAirStrikeStopsWhenTheAircraftPoolIsFull) {
  TheGameState().active() = true;

  // The pool holds four.
  EXPECT_EQ(LaunchAirStrike(house_, AIRCRAFT_BADGER, 5, MISSION_HUNT, Target{},
                            Target{}),
            4);

  EXPECT_EQ(TheObjectHeaps().aircraft().Count(), 4);
  EXPECT_EQ(TheWorld().scenario_init(), 0);
}

TEST_F(ReinforcementTest, ATeamThatCannotBePlacedIsDeleted) {
  // Nothing leaves limbo while the game is not active, so every placement
  // fails, whichever edge cell is tried.
  TheGameState().active() = false;
  house_->Control.Edge = SOURCE_WEST;
  TeamType& team_type = MakeTeamType(UnitTypeClass::As_Reference(UNIT_JEEP), 2);

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
  ASSERT_TRUE(
      blocker->Unlimbo(Cell::FromXY(0, 19).Center(), game::Direction::kNorth));
  TheWorld().scenario_init()--;
  // Nothing leaves limbo while the game is not active, so every cell fails.
  TheGameState().active() = false;
  TeamType& team_type = MakeTeamType(UnitTypeClass::As_Reference(UNIT_JEEP), 2);
  team_type.set_origin(0);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  // Only the vehicle that was in the way is left.
  EXPECT_EQ(TheObjectHeaps().unit().Count(), 1);
  EXPECT_EQ(TheWorld().scenario_init(), 0);
}

TEST_F(ReinforcementTest, AVehicleTeamEntersFromTheHouseEdge) {
  TheGameState().active() = true;
  house_->Control.Edge = SOURCE_WEST;
  TeamType& team_type = MakeTeamType(UnitTypeClass::As_Reference(UNIT_JEEP), 2);

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

TEST_F(ReinforcementTest, AHouseWithoutAnEdgeEntersFromTheNorth) {
  TheGameState().active() = true;
  house_->Control.Edge = SOURCE_NONE;
  TeamType& team_type = MakeTeamType(UnitTypeClass::As_Reference(UNIT_JEEP), 1);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().unit().Count(), 1);
  const UnitClass* jeep = TheObjectHeaps().unit().ActiveAt(0);
  EXPECT_FALSE(jeep->IsInLimbo);
  // One cell north of the visible map, which starts at row 1.
  EXPECT_EQ(Cell::Containing(jeep->Coord).y(), 0);
  // Facing the edge it came from, as from any other edge.
  EXPECT_EQ(jeep->PrimaryFacing.Current(), game::Direction::kNorth);
}

TEST_F(ReinforcementTest, OnlyAsManyMembersArriveAsThePoolHolds) {
  TheGameState().active() = true;
  house_->Control.Edge = SOURCE_WEST;
  // The infantry pool holds four.
  TeamType& team_type =
      MakeTeamType(InfantryTypeClass::As_Reference(INFANTRY_E1), 5);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().infantry().Count(), 4);
  for (int index = 0; index < 4; index++) {
    EXPECT_FALSE(TheObjectHeaps().infantry().ActiveAt(index)->IsInLimbo);
  }
  EXPECT_EQ(TheWorld().scenario_init(), 0);
}

TEST_F(ReinforcementTest, ATeamWhoseMembersCannotBeCreatedIsDeleted) {
  TheGameState().active() = true;
  // The vehicle pool holds four, and these fill it.
  for (int index = 0; index < 4; index++) {
    ASSERT_NE(base::NewPooled<UnitClass>(UNIT_JEEP, HOUSE_USSR), nullptr);
  }
  TeamType& team_type = MakeTeamType(UnitTypeClass::As_Reference(UNIT_JEEP), 2);

  EXPECT_FALSE(DeliverReinforcements(&team_type));

  EXPECT_EQ(TheObjectHeaps().team().Count(), 0);
  EXPECT_EQ(TheObjectHeaps().unit().Count(), 4);
  EXPECT_EQ(TheWorld().scenario_init(), 0);
}

TEST_F(ReinforcementTest, AGroupArrivesWithoutATeamWhenTheTeamPoolIsFull) {
  TheGameState().active() = true;
  house_->Control.Edge = SOURCE_WEST;
  TeamType& team_type = MakeTeamType(UnitTypeClass::As_Reference(UNIT_JEEP), 1);
  // The team pool holds two.
  ASSERT_NE(base::NewPooled<TeamClass>(&team_type), nullptr);
  ASSERT_NE(base::NewPooled<TeamClass>(&team_type), nullptr);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  EXPECT_EQ(TheObjectHeaps().team().Count(), 2);
  ASSERT_EQ(TheObjectHeaps().unit().Count(), 1);
  const UnitClass* jeep = TheObjectHeaps().unit().ActiveAt(0);
  EXPECT_FALSE(jeep->IsInLimbo);
  EXPECT_EQ(jeep->Team, nullptr);
  EXPECT_EQ(Cell::Containing(jeep->Coord).x(), 0);
}

TEST_F(ReinforcementTest, InfantryThatCannotLeaveTheBuildingIsDeleted) {
  TheGameState().active() = true;
  constexpr Cell kOrigin = Cell::FromXY(40, 50);
  TheScenario().Waypoint[0] = kOrigin;
  // No rules are loaded, so no ground can be crossed and the barracks has no
  // exit cell a soldier can walk onto.
  Place(base::NewPooled<BuildingClass>(STRUCT_BARRACKS, HOUSE_USSR), kOrigin);
  TeamType& team_type =
      MakeTeamType(InfantryTypeClass::As_Reference(INFANTRY_E1), 2);
  team_type.set_origin(0);

  EXPECT_FALSE(DeliverReinforcements(&team_type));

  EXPECT_EQ(TheObjectHeaps().infantry().Count(), 0);
  EXPECT_EQ(TheWorld().scenario_init(), 0);
}

TEST_F(ReinforcementTest,
       AnInfantryTeamWithNothingAtItsOriginEntersFromTheEdge) {
  TheGameState().active() = true;
  TheRules().ground().at(LAND_CLEAR).Cost.at(SPEED_FOOT) = Fixed(1);
  TheScenario().Waypoint[0] = Cell::FromXY(3, 20);
  TeamType& team_type =
      MakeTeamType(InfantryTypeClass::As_Reference(INFANTRY_E1), 1);
  team_type.set_origin(0);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().infantry().Count(), 1);
  const InfantryClass* soldier = TheObjectHeaps().infantry().ActiveAt(0);
  EXPECT_FALSE(soldier->IsInLimbo);
  EXPECT_EQ(Cell::Containing(soldier->Coord).x(), 0);
}

TEST_F(ReinforcementTest,
       AnInfantryTeamIgnoresAVehicleThatCarriesNoPassengers) {
  TheGameState().active() = true;
  TheRules().ground().at(LAND_CLEAR).Cost.at(SPEED_FOOT) = Fixed(1);
  constexpr Cell kOrigin = Cell::FromXY(3, 20);
  TheScenario().Waypoint[0] = kOrigin;
  auto* jeep = base::NewPooled<UnitClass>(UNIT_JEEP, HOUSE_USSR);
  Place(jeep, kOrigin);
  TeamType& team_type =
      MakeTeamType(InfantryTypeClass::As_Reference(INFANTRY_E1), 1);
  team_type.set_origin(0);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  EXPECT_EQ(jeep->How_Many(), 0);
  ASSERT_EQ(TheObjectHeaps().infantry().Count(), 1);
  const InfantryClass* soldier = TheObjectHeaps().infantry().ActiveAt(0);
  EXPECT_FALSE(soldier->IsInLimbo);
  EXPECT_EQ(Cell::Containing(soldier->Coord).x(), 0);
}

TEST_F(ReinforcementTest, AnInfantryTeamIsLoadedIntoTheTransportOnItsOrigin) {
  TheGameState().active() = true;
  constexpr Cell kOrigin = Cell::FromXY(40, 50);
  TheScenario().Waypoint[0] = kOrigin;
  auto* apc = base::NewPooled<UnitClass>(UNIT_APC, HOUSE_USSR);
  Place(apc, kOrigin);
  TeamType& team_type =
      MakeTeamType(InfantryTypeClass::As_Reference(INFANTRY_E1), 2);
  team_type.set_origin(0);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  EXPECT_EQ(apc->How_Many(), 2);
  EXPECT_EQ(apc->MissionQueue, MISSION_UNLOAD);
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
  ASSERT_TRUE(barracks->Unlimbo(kOrigin.Center(), game::Direction::kNorth));
  TheWorld().scenario_init()--;
  TeamType& team_type =
      MakeTeamType(InfantryTypeClass::As_Reference(INFANTRY_E1), 2);
  team_type.set_origin(0);

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
  TeamType& team_type =
      MakeTeamType(InfantryTypeClass::As_Reference(INFANTRY_E1), 2);
  team_type.set_origin(0);

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
  TeamType& team_type = MakeTeamType(UnitTypeClass::As_Reference(UNIT_JEEP), 1);
  team_type.set_origin(0);

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
  TeamType& team_type =
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

TEST_F(ReinforcementTest, AGroundTransportThatUnloadsIsNotALoaner) {
  TheGameState().active() = true;
  house_->Control.Edge = SOURCE_WEST;
  TeamType& team_type = MakeTeamType(UnitTypeClass::As_Reference(UNIT_APC), 1);
  AddMembers(team_type, InfantryTypeClass::As_Reference(INFANTRY_E1), 1);
  team_type.AddMission({TeamMissionType::kUnload, 0});

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().unit().Count(), 1);
  const UnitClass* apc = TheObjectHeaps().unit().ActiveAt(0);
  EXPECT_EQ(apc->How_Many(), 1);
  // Only a flying or sailing transport leaves the map after unloading.
  EXPECT_FALSE(apc->IsALoaner);
}

TEST_F(ReinforcementTest, ALoadedHelicopterIsALoanerOnlyWhenTheTeamUnloads) {
  TheGameState().active() = true;
  const AircraftTypeClass& helicopter =
      AircraftTypeClass::As_Reference(AIRCRAFT_TRANSPORT);
  const InfantryTypeClass& soldier =
      InfantryTypeClass::As_Reference(INFANTRY_E1);
  // No airstrip, so every aircraft is a loaner when it is created; only the
  // empty transport helicopter is handed to the house afterwards.
  TeamType& unloading = MakeTeamType(helicopter, 1);
  AddMembers(unloading, soldier, 1);
  unloading.AddMission({TeamMissionType::kUnload, 0});
  TeamType& empty = MakeTeamType(helicopter, 1);

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
  TeamType& team_type = MakeTeamType(UnitTypeClass::As_Reference(UNIT_ANT1), 1);
  team_type.set_origin(0);

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
  TeamType& ants = MakeTeamType(UnitTypeClass::As_Reference(UNIT_ANT1), 1);
  ants.set_origin(0);
  // Only ants use the hill.
  TheScenario().Waypoint[1] = Cell::FromXY(3, 40);
  TheMap().at(TheScenario().Waypoint[1]).TType = TEMPLATE_HILL01;
  TeamType& jeeps = MakeTeamType(UnitTypeClass::As_Reference(UNIT_JEEP), 1);
  jeeps.set_origin(1);

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
  TeamType& team_type =
      MakeTeamType(AircraftTypeClass::As_Reference(AIRCRAFT_MIG), 1);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().aircraft().Count(), 1);
  EXPECT_FALSE(TheObjectHeaps().aircraft().ActiveAt(0)->IsInLimbo);
  EXPECT_EQ(KeptAircraftCount(), 0);
}

TEST_F(ReinforcementTest, AnAircraftArrivesFacingInwardFromItsEdge) {
  TheGameState().active() = true;
  TeamType& team_type =
      MakeTeamType(AircraftTypeClass::As_Reference(AIRCRAFT_YAK), 2);

  house_->Control.Edge = SOURCE_EAST;
  EXPECT_TRUE(DeliverReinforcements(&team_type));
  house_->Control.Edge = SOURCE_NORTH;
  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().aircraft().Count(), 4);
  for (int index = 0; index < 4; index++) {
    const AircraftClass* yak = TheObjectHeaps().aircraft().ActiveAt(index);
    EXPECT_EQ(yak->PrimaryFacing.Current(),
              index < 2 ? game::Direction::kWest : game::Direction::kSouth);
  }
}

TEST_F(ReinforcementTest, AnAircraftWithAnOriginArrivesFacingIt) {
  // The origin is next to the north edge, so that is where the Yak enters,
  // whatever the house's own edge.
  TheScenario().Waypoint[0] = Cell::FromXY(20, 3);
  TheGameState().active() = true;
  house_->Control.Edge = SOURCE_EAST;
  TeamType& team_type =
      MakeTeamType(AircraftTypeClass::As_Reference(AIRCRAFT_YAK), 1);
  team_type.set_origin(0);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().aircraft().Count(), 1);
  EXPECT_EQ(TheObjectHeaps().aircraft().ActiveAt(0)->PrimaryFacing.Current(),
            game::Direction::kSouth);
}

TEST_F(ReinforcementTest, AParatrooperIsDrawnBelowThePlaneItJumpedFrom) {
  TheGameState().active() = true;
  TheRules().ground().at(LAND_CLEAR).Cost.at(SPEED_FOOT) = Fixed(1);
  TeamType& team_type =
      MakeTeamType(AircraftTypeClass::As_Reference(AIRCRAFT_BADGER), 1);
  ASSERT_TRUE(DeliverReinforcements(&team_type));
  ASSERT_EQ(TheObjectHeaps().aircraft().Count(), 1);
  AircraftClass* plane = TheObjectHeaps().aircraft().ActiveAt(0);
  ObjectClass* trooper =
      InfantryTypeClass::As_Reference(INFANTRY_E1).Create_One_Of(house_);
  ASSERT_NE(trooper, nullptr);
  ASSERT_TRUE(trooper->Paradrop(Cell::FromXY(20, 20).Center()));

  // Both start in the top layer, the trooper created after the plane.
  ASSERT_EQ(plane->In_Which_Layer(), LAYER_TOP);
  ASSERT_EQ(trooper->In_Which_Layer(), LAYER_TOP);
  std::vector<ObjectClass*> draw_list = {plane, trooper};

  DisplayClass::SortForDrawing(LAYER_TOP, draw_list);

  EXPECT_EQ(draw_list, (std::vector<ObjectClass*>{trooper, plane}));
}

TEST_F(ReinforcementTest, AHouseKeepsOneMigForEachAirstrip) {
  TheGameState().active() = true;
  Place(base::NewPooled<BuildingClass>(STRUCT_AIRSTRIP, HOUSE_USSR),
        Cell::FromXY(40, 50));
  ASSERT_EQ(house_->Get_Quantity(STRUCT_AIRSTRIP), 1);
  TeamType& team_type =
      MakeTeamType(AircraftTypeClass::As_Reference(AIRCRAFT_MIG), 2);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().aircraft().Count(), 2);
  EXPECT_EQ(KeptAircraftCount(), 1);
}

TEST_F(ReinforcementTest, YaksAndMigsShareTheAirstrips) {
  TheGameState().active() = true;
  Place(base::NewPooled<BuildingClass>(STRUCT_AIRSTRIP, HOUSE_USSR),
        Cell::FromXY(40, 50));
  Place(base::NewPooled<BuildingClass>(STRUCT_AIRSTRIP, HOUSE_USSR),
        Cell::FromXY(50, 50));
  ASSERT_EQ(house_->Get_Quantity(STRUCT_AIRSTRIP), 2);
  TeamType& team_type =
      MakeTeamType(AircraftTypeClass::As_Reference(AIRCRAFT_YAK), 2);
  AddMembers(team_type, AircraftTypeClass::As_Reference(AIRCRAFT_MIG), 1);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  // Two airstrips, three aircraft: the last one created is borrowed.
  ASSERT_EQ(TheObjectHeaps().aircraft().Count(), 3);
  EXPECT_EQ(KeptAircraftCount(), 2);
  EXPECT_TRUE(TheObjectHeaps().aircraft().ActiveAt(2)->IsALoaner);
}

TEST_F(ReinforcementTest, AComputerHouseKeepsOneMigForEachAirstrip) {
  TheGameState().active() = true;
  // The house is no longer the player's; its airstrip limits it all the same.
  ThePlayer() = base::NewPooled<HouseClass>(HOUSE_GREECE);
  Place(base::NewPooled<BuildingClass>(STRUCT_AIRSTRIP, HOUSE_USSR),
        Cell::FromXY(40, 50));
  TeamType& team_type =
      MakeTeamType(AircraftTypeClass::As_Reference(AIRCRAFT_MIG), 2);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().aircraft().Count(), 2);
  EXPECT_EQ(KeptAircraftCount(), 1);
}

TEST_F(ReinforcementTest, AnotherHousesMigUsesNoAirstrip) {
  TheGameState().active() = true;
  ThePlayer() = base::NewPooled<HouseClass>(HOUSE_GREECE);
  const auto* players_mig =
      base::NewPooled<AircraftClass>(AIRCRAFT_MIG, HOUSE_GREECE);
  ASSERT_NE(players_mig, nullptr);
  ASSERT_FALSE(players_mig->IsALoaner);
  Place(base::NewPooled<BuildingClass>(STRUCT_AIRSTRIP, HOUSE_USSR),
        Cell::FromXY(40, 50));
  TeamType& team_type =
      MakeTeamType(AircraftTypeClass::As_Reference(AIRCRAFT_MIG), 1);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  // The house's own airstrip is free, so it keeps the MiG it is sent.
  ASSERT_EQ(TheObjectHeaps().aircraft().Count(), 2);
  EXPECT_FALSE(TheObjectHeaps().aircraft().ActiveAt(1)->IsALoaner);
}

TEST_F(ReinforcementTest, AHelicopterOrABorrowedMigUsesNoAirstrip) {
  TheGameState().active() = true;
  // Without an airstrip the first MiG is borrowed.
  TeamType& before =
      MakeTeamType(AircraftTypeClass::As_Reference(AIRCRAFT_MIG), 1);
  AddMembers(before, AircraftTypeClass::As_Reference(AIRCRAFT_HIND), 1);
  EXPECT_TRUE(DeliverReinforcements(&before));
  ASSERT_EQ(KeptAircraftCount(), 0);
  Place(base::NewPooled<BuildingClass>(STRUCT_AIRSTRIP, HOUSE_USSR),
        Cell::FromXY(40, 50));
  TeamType& after =
      MakeTeamType(AircraftTypeClass::As_Reference(AIRCRAFT_MIG), 1);

  EXPECT_TRUE(DeliverReinforcements(&after));

  // The airstrip is still free for the MiG that arrives next.
  ASSERT_EQ(TheObjectHeaps().aircraft().Count(), 3);
  EXPECT_FALSE(TheObjectHeaps().aircraft().ActiveAt(2)->IsALoaner);
}

TEST_F(ReinforcementTest, AnAirstripKeepsNoHelicopter) {
  TheGameState().active() = true;
  Place(base::NewPooled<BuildingClass>(STRUCT_AIRSTRIP, HOUSE_USSR),
        Cell::FromXY(40, 50));
  TeamType& team_type =
      MakeTeamType(AircraftTypeClass::As_Reference(AIRCRAFT_HIND), 1);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().aircraft().Count(), 1);
  EXPECT_EQ(KeptAircraftCount(), 0);
}

TEST_F(ReinforcementTest, ATeamTypeWithoutMissionsAttacksItsOrigin) {
  TheGameState().active() = true;
  TeamType& team_type = MakeTeamType(UnitTypeClass::As_Reference(UNIT_JEEP), 1);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(team_type.mission_count(), 1);
  EXPECT_EQ(team_type.missions()[0].mission(),
            TeamMissionType::kAttackWaypoint);
  EXPECT_EQ(team_type.missions()[0].value(), -1);
}

TEST_F(ReinforcementTest, ATeamTypeWithoutMembersDeliversNothing) {
  TeamType& team_type = MakeTeamType(UnitTypeClass::As_Reference(UNIT_JEEP), 1);
  team_type.ClearMembers();

  EXPECT_FALSE(DeliverReinforcements(&team_type));

  EXPECT_EQ(TheObjectHeaps().team().Count(), 0);
}

}  // namespace
}  // namespace ra
