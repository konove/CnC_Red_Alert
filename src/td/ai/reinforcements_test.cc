// Tests the reinforcements against a real Game with no game data: one house
// and the team types are built by hand.

#include "td/ai/reinforcements.h"

#include <cstdlib>

#include "base/array.h"
#include "base/pooled.h"
#include "game/sim/direction.h"
#include "gtest/gtest.h"
#include "td/ai/team.h"
#include "td/ai/teamtype.h"
#include "td/core/cell.h"
#include "td/core/defines.h"
#include "td/core/object_heaps.h"
#include "td/core/target.h"
#include "td/map/display.h"
#include "td/map/layer.h"
#include "td/map/map.h"
#include "td/map/world.h"
#include "td/objects/aircraft.h"
#include "td/objects/building.h"
#include "td/objects/cell.h"
#include "td/objects/house.h"
#include "td/objects/infantry.h"
#include "td/objects/type.h"
#include "td/objects/unit.h"
#include "td/startup/game.h"
#include "td/startup/game_state.h"

namespace td {
namespace {

class ReinforcementTest : public ::testing::Test {
 protected:
  ReinforcementTest() {
    TheObjectHeaps().house().Reset(2);
    TheObjectHeaps().aircraft().Reset(4);
    TheObjectHeaps().unit().Reset(4);
    TheObjectHeaps().infantry().Reset(4);
    TheObjectHeaps().team().Reset(2);
    TheObjectHeaps().team_type().Reset(2);
    house_ = base::NewPooled<HouseClass>(HOUSE_GOOD);
    // DeliverReinforcements() announces the player's own reinforcements.
    ThePlayer() = house_;
    // The map layer alone: the display layers above it load game data.
    TheMap().MapClass::One_Time();
    TheMap().MapClass::Set_Map_Dimensions(1, 1, MAP_CELL_W - 2, MAP_CELL_H - 2);
    // Nothing leaves limbo while the game is not active, so every placement
    // fails, whichever cell is tried.
    TheGameState().active() = false;
  }

  // The display layers are static and outlive the Game, so they would keep the
  // objects a test put on the map.
  void TearDown() override {
    for (LayerClass& layer : DisplayClass::Layer) {
      layer.Clear();
    }
  }

  // A team type for the house with no members and no missions, so that
  // delivering it makes no team.
  static TeamTypeClass& MakeTeamType() {
    auto* team_type = base::NewPooled<TeamTypeClass>();
    EXPECT_NE(team_type, nullptr);
    team_type->House = HOUSE_GOOD;
    return *team_type;
  }

  // Adds `count` objects of `member_type` to `team_type`.
  static void AddMembers(TeamTypeClass& team_type,
                         const TechnoTypeClass& member_type, const int count) {
    base::At(team_type.Class, team_type.ClassCount) = &member_type;
    base::At(team_type.DesiredNum, team_type.ClassCount) =
        static_cast<unsigned char>(count);
    team_type.ClassCount++;
  }

  // Adds a first mission to `team_type`, so that delivering it makes a team.
  static void AddMission(TeamTypeClass& team_type) {
    base::At(team_type.MissionList, 0).Mission = TMISSION_GUARD;
    team_type.MissionCount = 1;
  }

  // Turns the cells of `row` from `first_column` to `last_column` into water.
  static void Flood(const int row, const int first_column,
                    const int last_column) {
    for (int column = first_column; column <= last_column; column++) {
      CellClass& cell = TheMap().at(Cell::FromXY(column, row));
      cell.TType = TEMPLATE_WATER;
      cell.Recalc_Attributes();
    }
  }

  // An airstrip of the house with its top left corner in `cell`.
  static BuildingClass& PlaceAirstrip(const Cell cell) {
    TheObjectHeaps().building().Reset(1);
    auto* airstrip =
        base::NewPooled<BuildingClass>(STRUCT_AIRSTRIP, HOUSE_GOOD);
    EXPECT_NE(airstrip, nullptr);
    TheWorld().scenario_init()++;
    EXPECT_TRUE(airstrip->Unlimbo(cell.Center(), game::Direction::kNorth));
    TheWorld().scenario_init()--;
    return *airstrip;
  }

  // The one team type DeliverSpecialReinforcement() made.
  static const TeamTypeClass& SpecialTeamType() {
    EXPECT_EQ(TheObjectHeaps().team_type().Count(), 1);
    return *TheObjectHeaps().team_type().ActiveAt(0);
  }

  // The number of vehicles, soldiers and aircraft that exist, in limbo or not.
  static int ObjectCount() {
    return TheObjectHeaps().unit().Count() +
           TheObjectHeaps().infantry().Count() +
           TheObjectHeaps().aircraft().Count();
  }

  HouseClass* house_ = nullptr;

 private:
  Game game_;
};

TEST_F(ReinforcementTest, AnAirStrikeThatCannotBePlacedCountsNoAircraft) {
  EXPECT_EQ(LaunchAirStrike(house_, AIRCRAFT_A10, 2, MISSION_HUNT, Target{},
                            Target{}),
            0);
  EXPECT_EQ(ObjectCount(), 0);
}

TEST_F(ReinforcementTest, ATeamThatCannotBePlacedAtTheEdgeIsDeleted) {
  house_->Edge = SOURCE_WEST;
  TeamTypeClass& team_type = MakeTeamType();
  AddMembers(team_type, UnitTypeClass::As_Reference(UNIT_JEEP), 2);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  EXPECT_EQ(ObjectCount(), 0);
  EXPECT_EQ(TheWorld().scenario_init(), 0);
}

TEST_F(ReinforcementTest, AVehicleGroupEntersFromTheHouseEdge) {
  TheGameState().active() = true;
  house_->Edge = SOURCE_WEST;
  TeamTypeClass& team_type = MakeTeamType();
  AddMembers(team_type, UnitTypeClass::As_Reference(UNIT_JEEP), 2);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().unit().Count(), 2);
  for (int index = 0; index < 2; index++) {
    const UnitClass* unit = TheObjectHeaps().unit().ActiveAt(index);
    EXPECT_FALSE(unit->IsInLimbo);
    EXPECT_EQ(unit->Owner(), HOUSE_GOOD);
    // One cell west of the visible map, which starts at column 1.
    EXPECT_EQ(Cell::Containing(unit->Coord).x(), 0);
  }
  EXPECT_EQ(TheWorld().scenario_init(), 0);
}

TEST_F(ReinforcementTest, AHovercraftWithNoBeachToLandOnIsDeleted) {
  // The map has no water, so there is no beach cell. A transport deletes its
  // cargo only while the game is active.
  TheGameState().active() = true;
  TeamTypeClass& team_type = MakeTeamType();
  AddMembers(team_type, UnitTypeClass::As_Reference(UNIT_HOVER), 1);
  AddMembers(team_type, InfantryTypeClass::As_Reference(INFANTRY_E1), 2);

  DeliverReinforcements(&team_type);

  EXPECT_EQ(ObjectCount(), 0);
}

TEST_F(ReinforcementTest, AGroupForAHouseEdgeThatIsNoEdgeIsDeleted) {
  house_->Edge = SOURCE_VISIBLE;
  TeamTypeClass& team_type = MakeTeamType();
  AddMembers(team_type, UnitTypeClass::As_Reference(UNIT_JEEP), 2);

  DeliverReinforcements(&team_type);

  EXPECT_EQ(ObjectCount(), 0);
}

TEST_F(ReinforcementTest, TheGroundMembersOfAnAirGroupAreDeleted) {
  // No transport, so the soldier cannot ride in the A-10.
  TeamTypeClass& team_type = MakeTeamType();
  AddMembers(team_type, AircraftTypeClass::As_Reference(AIRCRAFT_A10), 1);
  AddMembers(team_type, InfantryTypeClass::As_Reference(INFANTRY_E1), 1);

  DeliverReinforcements(&team_type);

  EXPECT_EQ(ObjectCount(), 0);
}

TEST_F(ReinforcementTest, EveryGunboatThatCannotBePlacedIsDeleted) {
  TeamTypeClass& team_type = MakeTeamType();
  AddMembers(team_type, UnitTypeClass::As_Reference(UNIT_GUNBOAT), 2);

  EXPECT_FALSE(DeliverReinforcements(&team_type));

  EXPECT_EQ(ObjectCount(), 0);
}

// The four map edges a ground group can enter from.
class EdgeEntryTest : public ReinforcementTest,
                      public ::testing::WithParamInterface<SourceType> {};

TEST_P(EdgeEntryTest, AGroupWithoutATeamMovesOntoTheMap) {
  TheGameState().active() = true;
  house_->Edge = GetParam();
  TeamTypeClass& team_type = MakeTeamType();
  AddMembers(team_type, UnitTypeClass::As_Reference(UNIT_JEEP), 1);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  EXPECT_EQ(TheObjectHeaps().team().Count(), 0);
  ASSERT_EQ(TheObjectHeaps().unit().Count(), 1);
  const UnitClass* unit = TheObjectHeaps().unit().ActiveAt(0);
  EXPECT_EQ(unit->Team, nullptr);
  EXPECT_EQ(unit->Mission, MISSION_MOVE);
  // The member enters one cell off the visible map and is sent to the map
  // cell straight across the edge from it.
  const Cell entry_cell = Cell::Containing(unit->Coord);
  EXPECT_FALSE(TheMap().In_Radar(entry_cell));
  const Cell destination = unit->NavCom.AsCell();
  EXPECT_TRUE(TheMap().In_Radar(destination));
  EXPECT_EQ(std::abs(destination.x() - entry_cell.x()) +
                std::abs(destination.y() - entry_cell.y()),
            1);
}

INSTANTIATE_TEST_SUITE_P(Edges, EdgeEntryTest,
                         ::testing::Values(SOURCE_NORTH, SOURCE_EAST,
                                           SOURCE_SOUTH, SOURCE_WEST));

TEST_F(ReinforcementTest, AGroupWithATeamWaitsForItsOrders) {
  TheGameState().active() = true;
  house_->Edge = SOURCE_WEST;
  TeamTypeClass& team_type = MakeTeamType();
  AddMembers(team_type, UnitTypeClass::As_Reference(UNIT_JEEP), 2);
  AddMission(team_type);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().team().Count(), 1);
  const TeamClass* team = TheObjectHeaps().team().ActiveAt(0);
  ASSERT_EQ(TheObjectHeaps().unit().Count(), 2);
  for (int index = 0; index < 2; index++) {
    const UnitClass* unit = TheObjectHeaps().unit().ActiveAt(index);
    EXPECT_FALSE(unit->IsInLimbo);
    EXPECT_EQ(unit->Team, team);
    EXPECT_EQ(unit->Mission, MISSION_GUARD);
    EXPECT_EQ(unit->NavCom, Target{});
  }
  EXPECT_EQ(TheWorld().scenario_init(), 0);
}

TEST_F(ReinforcementTest, AHovercraftStartsBelowTheBeachItLandsOn) {
  TheGameState().active() = true;
  // Water in column 30 of the map's bottom row and of the row below the map,
  // so the beach is the cell above them.
  Flood(MAP_CELL_H - 2, 30, 30);
  Flood(MAP_CELL_H - 1, 30, 30);
  constexpr Cell kBeach = Cell::FromXY(30, MAP_CELL_H - 3);
  TeamTypeClass& team_type = MakeTeamType();
  AddMembers(team_type, UnitTypeClass::As_Reference(UNIT_HOVER), 1);
  AddMembers(team_type, InfantryTypeClass::As_Reference(INFANTRY_E1), 2);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().unit().Count(), 1);
  const UnitClass* hovercraft = TheObjectHeaps().unit().ActiveAt(0);
  EXPECT_FALSE(hovercraft->IsInLimbo);
  EXPECT_EQ(Cell::Containing(hovercraft->Coord),
            Cell::FromXY(30, MAP_CELL_H - 1));
  EXPECT_EQ(hovercraft->cargo().How_Many(), 2);
  EXPECT_EQ(hovercraft->Mission, MISSION_UNLOAD);
  EXPECT_EQ(hovercraft->NavCom, Target::FromCellIndex(kBeach));
  // UnitClass::Unlimbo() makes every hovercraft a loaner, so that it can
  // leave the map again.
  EXPECT_TRUE(hovercraft->IsALoaner);
  EXPECT_EQ(TheObjectHeaps().infantry().Count(), 2);
}

TEST_F(ReinforcementTest, AHovercraftNeverJoinsTheTeamItsCargoIsIn) {
  TheGameState().active() = true;
  Flood(MAP_CELL_H - 2, 30, 30);
  Flood(MAP_CELL_H - 1, 30, 30);
  TeamTypeClass& team_type = MakeTeamType();
  AddMembers(team_type, UnitTypeClass::As_Reference(UNIT_HOVER), 1);
  AddMembers(team_type, InfantryTypeClass::As_Reference(INFANTRY_E1), 1);
  AddMission(team_type);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().team().Count(), 1);
  ASSERT_EQ(TheObjectHeaps().unit().Count(), 1);
  ASSERT_EQ(TheObjectHeaps().infantry().Count(), 1);
  const UnitClass* hovercraft = TheObjectHeaps().unit().ActiveAt(0);
  EXPECT_EQ(hovercraft->Team, nullptr);
  EXPECT_EQ(TheObjectHeaps().infantry().ActiveAt(0)->Team,
            TheObjectHeaps().team().ActiveAt(0));
  // The team gives the orders, so the hovercraft is not sent to the beach.
  EXPECT_EQ(hovercraft->NavCom, Target{});
}

TEST_F(ReinforcementTest, OneGunboatSailsWestAlongTheShippingLane) {
  TheGameState().active() = true;
  // The lane is the first row of the visible map that is all water; the
  // gunboat starts one cell east of it.
  constexpr int kLaneRow = 12;
  Flood(kLaneRow, 1, MAP_CELL_W - 1);
  TeamTypeClass& team_type = MakeTeamType();
  AddMembers(team_type, UnitTypeClass::As_Reference(UNIT_GUNBOAT), 2);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().unit().Count(), 1);
  const UnitClass* gunboat = TheObjectHeaps().unit().ActiveAt(0);
  EXPECT_FALSE(gunboat->IsInLimbo);
  EXPECT_EQ(Cell::Containing(gunboat->Coord),
            Cell::FromXY(MAP_CELL_W - 1, kLaneRow));
  EXPECT_TRUE(gunboat->IsALoaner);
  EXPECT_EQ(gunboat->Mission, MISSION_GUARD);
  EXPECT_EQ(gunboat->NavCom, Target::FromCellIndex(Cell::FromXY(0, kLaneRow)));
}

TEST_F(ReinforcementTest, AnA10FliesInAsALoanerAndHunts) {
  TheGameState().active() = true;
  house_->Edge = SOURCE_WEST;
  TeamTypeClass& team_type = MakeTeamType();
  AddMembers(team_type, AircraftTypeClass::As_Reference(AIRCRAFT_A10), 1);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().aircraft().Count(), 1);
  const AircraftClass* a10 = TheObjectHeaps().aircraft().ActiveAt(0);
  EXPECT_FALSE(a10->IsInLimbo);
  EXPECT_EQ(Cell::Containing(a10->Coord).x(), 0);
  EXPECT_TRUE(a10->IsALoaner);
  EXPECT_EQ(a10->Mission, MISSION_HUNT);
  EXPECT_EQ(TheWorld().scenario_init(), 0);
}

TEST_F(ReinforcementTest, ALoadedHelicopterUnloadsAtTheReinforcementWaypoint) {
  TheGameState().active() = true;
  house_->Edge = SOURCE_WEST;
  constexpr Cell kWaypoint = Cell::FromXY(20, 30);
  base::At(TheWorld().waypoint(), kWayptReinf) = kWaypoint;
  TeamTypeClass& team_type = MakeTeamType();
  AddMembers(team_type, AircraftTypeClass::As_Reference(AIRCRAFT_TRANSPORT), 1);
  AddMembers(team_type, InfantryTypeClass::As_Reference(INFANTRY_E1), 2);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().aircraft().Count(), 1);
  const AircraftClass* helicopter = TheObjectHeaps().aircraft().ActiveAt(0);
  EXPECT_FALSE(helicopter->IsInLimbo);
  EXPECT_EQ(helicopter->cargo().How_Many(), 2);
  // It only delivers the soldiers and then leaves.
  EXPECT_TRUE(helicopter->IsALoaner);
  EXPECT_EQ(helicopter->Mission, MISSION_UNLOAD);
  EXPECT_EQ(helicopter->NavCom, Target::FromCellIndex(kWaypoint));
}

TEST_F(ReinforcementTest, AnEmptyHelicopterIsKeptAndMovesToTheWaypoint) {
  TheGameState().active() = true;
  house_->Edge = SOURCE_WEST;
  constexpr Cell kWaypoint = Cell::FromXY(20, 30);
  base::At(TheWorld().waypoint(), kWayptReinf) = kWaypoint;
  TeamTypeClass& team_type = MakeTeamType();
  AddMembers(team_type, AircraftTypeClass::As_Reference(AIRCRAFT_TRANSPORT), 1);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().aircraft().Count(), 1);
  const AircraftClass* helicopter = TheObjectHeaps().aircraft().ActiveAt(0);
  EXPECT_FALSE(helicopter->IsALoaner);
  EXPECT_EQ(helicopter->Mission, MISSION_MOVE);
  EXPECT_EQ(helicopter->NavCom, Target::FromCellIndex(kWaypoint));
}

TEST_F(ReinforcementTest, AnAircraftWithATeamIsLeftToItsTeam) {
  TheGameState().active() = true;
  house_->Edge = SOURCE_WEST;
  TeamTypeClass& team_type = MakeTeamType();
  AddMembers(team_type, AircraftTypeClass::As_Reference(AIRCRAFT_TRANSPORT), 1);
  AddMission(team_type);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().team().Count(), 1);
  ASSERT_EQ(TheObjectHeaps().aircraft().Count(), 1);
  const AircraftClass* helicopter = TheObjectHeaps().aircraft().ActiveAt(0);
  EXPECT_FALSE(helicopter->IsInLimbo);
  EXPECT_EQ(helicopter->Team, TheObjectHeaps().team().ActiveAt(0));
  EXPECT_EQ(helicopter->NavCom, Target{});
}

TEST_F(ReinforcementTest, ACargoPlaneEntersInTheRowOfItsAirstrip) {
  TheGameState().active() = true;
  // The house's own edge would bring it in from the west.
  house_->Edge = SOURCE_WEST;
  const BuildingClass& airstrip = PlaceAirstrip(Cell::FromXY(20, 30));
  TeamTypeClass& team_type = MakeTeamType();
  AddMembers(team_type, AircraftTypeClass::As_Reference(AIRCRAFT_CARGO), 1);
  AddMembers(team_type, UnitTypeClass::As_Reference(UNIT_JEEP), 1);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().aircraft().Count(), 1);
  const AircraftClass* plane = TheObjectHeaps().aircraft().ActiveAt(0);
  EXPECT_FALSE(plane->IsInLimbo);
  // One cell east of the visible map.
  EXPECT_EQ(Cell::Containing(plane->Coord),
            Cell::FromXY(MAP_CELL_W - 1, airstrip.Docking_Coord().cell_y()));
  EXPECT_EQ(plane->cargo().How_Many(), 1);
  EXPECT_TRUE(plane->IsALoaner);
}

TEST_F(ReinforcementTest, ACargoPlaneWithoutAnAirstripEntersAtTheHouseEdge) {
  TheGameState().active() = true;
  house_->Edge = SOURCE_WEST;
  TeamTypeClass& team_type = MakeTeamType();
  AddMembers(team_type, AircraftTypeClass::As_Reference(AIRCRAFT_CARGO), 1);
  AddMembers(team_type, UnitTypeClass::As_Reference(UNIT_JEEP), 1);

  EXPECT_TRUE(DeliverReinforcements(&team_type));

  ASSERT_EQ(TheObjectHeaps().aircraft().Count(), 1);
  EXPECT_EQ(
      Cell::Containing(TheObjectHeaps().aircraft().ActiveAt(0)->Coord).x(), 0);
}

TEST_F(ReinforcementTest, ASpecialVehicleDrivesToTheReinforcementWaypoint) {
  TheGameState().active() = true;
  house_->Edge = SOURCE_WEST;
  constexpr Cell kWaypoint = Cell::FromXY(20, 30);
  base::At(TheWorld().waypoint(), kWayptReinf) = kWaypoint;

  // The mission asked for is replaced.
  EXPECT_TRUE(DeliverSpecialReinforcement(
      house_, &UnitTypeClass::As_Reference(UNIT_HARVESTER), nullptr,
      TMISSION_GUARD, 5));

  const TeamTypeClass& team_type = SpecialTeamType();
  EXPECT_TRUE(team_type.IsTransient);
  EXPECT_EQ(team_type.ClassCount, 1);
  EXPECT_EQ(team_type.Class[0], &UnitTypeClass::As_Reference(UNIT_HARVESTER));
  ASSERT_EQ(team_type.MissionCount, 1);
  EXPECT_EQ(team_type.MissionList[0].Mission, TMISSION_MOVECELL);
  EXPECT_EQ(team_type.MissionList[0].Argument, kWaypoint.index());
  ASSERT_EQ(TheObjectHeaps().unit().Count(), 1);
  const UnitClass* harvester = TheObjectHeaps().unit().ActiveAt(0);
  EXPECT_EQ(Cell::Containing(harvester->Coord).x(), 0);
  EXPECT_EQ(harvester->Team, TheObjectHeaps().team().ActiveAt(0));
}

TEST_F(ReinforcementTest,
       ASpecialVehicleWithNoWayInFromTheSouthGetsAHovercraft) {
  TheGameState().active() = true;
  house_->Edge = SOURCE_SOUTH;
  // No cell below the map can be driven over, and column 30 has a beach.
  Flood(MAP_CELL_H - 1, 0, MAP_CELL_W - 1);
  Flood(MAP_CELL_H - 2, 30, 30);

  EXPECT_TRUE(DeliverSpecialReinforcement(
      house_, &UnitTypeClass::As_Reference(UNIT_HARVESTER), nullptr,
      TMISSION_GUARD, 5));

  const TeamTypeClass& team_type = SpecialTeamType();
  ASSERT_EQ(team_type.ClassCount, 2);
  EXPECT_EQ(team_type.Class[1], &UnitTypeClass::As_Reference(UNIT_HOVER));
  // The mission asked for is dropped: the hovercraft delivers and that is all.
  ASSERT_EQ(team_type.MissionCount, 1);
  EXPECT_EQ(team_type.MissionList[0].Mission, TMISSION_NONE);
  ASSERT_EQ(TheObjectHeaps().unit().Count(), 2);
}

TEST_F(ReinforcementTest, ASpecialVehicleWithAWayInFromTheSouthDrives) {
  TheGameState().active() = true;
  house_->Edge = SOURCE_SOUTH;

  EXPECT_TRUE(DeliverSpecialReinforcement(
      house_, &UnitTypeClass::As_Reference(UNIT_HARVESTER), nullptr));

  EXPECT_EQ(SpecialTeamType().ClassCount, 1);
  ASSERT_EQ(TheObjectHeaps().unit().Count(), 1);
  EXPECT_EQ(Cell::Containing(TheObjectHeaps().unit().ActiveAt(0)->Coord).y(),
            MAP_CELL_H - 1);
}

TEST_F(ReinforcementTest, ASpecialTransportUnloadsAtTheReinforcementWaypoint) {
  TheGameState().active() = true;
  house_->Edge = SOURCE_WEST;

  EXPECT_TRUE(DeliverSpecialReinforcement(
      house_, &InfantryTypeClass::As_Reference(INFANTRY_E1),
      &AircraftTypeClass::As_Reference(AIRCRAFT_TRANSPORT)));

  const TeamTypeClass& team_type = SpecialTeamType();
  ASSERT_EQ(team_type.ClassCount, 2);
  EXPECT_EQ(team_type.Class[1],
            &AircraftTypeClass::As_Reference(AIRCRAFT_TRANSPORT));
  ASSERT_EQ(team_type.MissionCount, 1);
  EXPECT_EQ(team_type.MissionList[0].Mission, TMISSION_UNLOAD);
  EXPECT_EQ(team_type.MissionList[0].Argument, kWayptReinf);
  ASSERT_EQ(TheObjectHeaps().aircraft().Count(), 1);
  EXPECT_EQ(TheObjectHeaps().aircraft().ActiveAt(0)->cargo().How_Many(), 1);
}

TEST_F(ReinforcementTest, ASpecialTransportKeepsTheMissionAskedFor) {
  TheGameState().active() = true;
  house_->Edge = SOURCE_WEST;

  EXPECT_TRUE(DeliverSpecialReinforcement(
      house_, &InfantryTypeClass::As_Reference(INFANTRY_E1),
      &AircraftTypeClass::As_Reference(AIRCRAFT_TRANSPORT), TMISSION_GUARD, 5));

  const TeamTypeClass& team_type = SpecialTeamType();
  ASSERT_EQ(team_type.MissionCount, 1);
  EXPECT_EQ(team_type.MissionList[0].Mission, TMISSION_GUARD);
  EXPECT_EQ(team_type.MissionList[0].Argument, 5);
}

TEST_F(ReinforcementTest, ASpecialAircraftKeepsItsMissionOrMovesToTheWaypoint) {
  TheGameState().active() = true;
  house_->Edge = SOURCE_WEST;
  constexpr Cell kWaypoint = Cell::FromXY(20, 30);
  base::At(TheWorld().waypoint(), kWayptReinf) = kWaypoint;
  const AircraftTypeClass& helicopter =
      AircraftTypeClass::As_Reference(AIRCRAFT_HELICOPTER);

  EXPECT_TRUE(DeliverSpecialReinforcement(house_, &helicopter, nullptr,
                                          TMISSION_GUARD, 5));
  EXPECT_TRUE(DeliverSpecialReinforcement(house_, &helicopter, nullptr));

  ASSERT_EQ(TheObjectHeaps().team_type().Count(), 2);
  const TeamTypeClass* with_mission = TheObjectHeaps().team_type().ActiveAt(0);
  EXPECT_EQ(with_mission->MissionList[0].Mission, TMISSION_GUARD);
  EXPECT_EQ(with_mission->MissionList[0].Argument, 5);
  const TeamTypeClass* without = TheObjectHeaps().team_type().ActiveAt(1);
  EXPECT_EQ(without->MissionList[0].Mission, TMISSION_MOVECELL);
  EXPECT_EQ(without->MissionList[0].Argument, kWaypoint.index());
}

TEST_F(ReinforcementTest, AFailedSpecialReinforcementLeavesNoTeamType) {
  // No room for the harvester, so the delivery fails after making its team.
  TheObjectHeaps().unit().Reset(0);

  EXPECT_FALSE(DeliverSpecialReinforcement(
      house_, &UnitTypeClass::As_Reference(UNIT_HARVESTER), nullptr));

  EXPECT_EQ(TheObjectHeaps().team_type().Count(), 0);
  EXPECT_EQ(TheObjectHeaps().team().Count(), 0);
}

TEST_F(ReinforcementTest, ATeamTypeWithoutMembersDeliversNothing) {
  const TeamTypeClass& team_type = MakeTeamType();

  EXPECT_FALSE(DeliverReinforcements(&team_type));
  EXPECT_FALSE(DeliverReinforcements(nullptr));

  EXPECT_EQ(ObjectCount(), 0);
}

}  // namespace
}  // namespace td
