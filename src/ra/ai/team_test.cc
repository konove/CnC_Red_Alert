// Tests for TeamClass, a team of objects working through its type's script.

#include "ra/ai/team.h"

#include "base/array.h"
#include "base/pooled.h"
#include "gtest/gtest.h"
#include "ra/ai/reinforcements.h"
#include "ra/ai/scenario.h"
#include "ra/ai/team_type.h"
#include "ra/core/cell.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/map/world.h"
#include "ra/objects/house.h"
#include "ra/objects/type.h"
#include "ra/startup/game_state.h"
#include "ra/startup/game_test_util.h"

namespace ra {
namespace {

class TeamTest : public GameFixture {
 protected:
  TeamTest() {
    // No rules are loaded, so every type has no strength, and a team takes no
    // member that looks dead.
    UnitTypeClass::As_Reference(UNIT_JEEP).MaxStrength = 100;
    TheObjectHeaps().team().Reset(1);
    TheObjectHeaps().team_type().Reset(1);
    ThePlayer() = base::NewPooled<HouseClass>(HOUSE_USSR);
    TheGameState().active() = true;
  }

  // Brings one jeep onto the map as a team whose script is the single line
  // `mission`, and runs the team until it has started on that line.
  static TeamClass* StartTeam(const TeamMission mission) {
    auto* team_type = base::NewPooled<TeamType>();
    EXPECT_NE(team_type, nullptr);
    team_type->set_house(HOUSE_USSR);
    team_type->AddMember(
        {.quantity = 1, .type = &UnitTypeClass::As_Reference(UNIT_JEEP)});
    team_type->AddMission(mission);
    EXPECT_TRUE(DeliverReinforcements(team_type));
    EXPECT_EQ(TheObjectHeaps().team().Count(), 1);
    TeamClass* team = TheObjectHeaps().team().ActiveAt(0);
    team->AI();
    EXPECT_TRUE(team->IsMoving);
    return team;
  }
};

TEST_F(TeamTest, ATeamMovingToAWaypointOffTheRadarIsLeavingTheMap) {
  base::At(TheScenario().Waypoint, 3) = Cell::None();

  EXPECT_TRUE(StartTeam({TeamMissionType::kMove, 3})->Is_Leaving_Map());
}

TEST_F(TeamTest, ATeamMovingToAWaypointOnTheRadarIsNotLeavingTheMap) {
  base::At(TheScenario().Waypoint, 3) = Cell::FromXY(10, 10);

  EXPECT_FALSE(StartTeam({TeamMissionType::kMove, 3})->Is_Leaving_Map());
}

TEST_F(TeamTest, ATeamMovingToAWaypointPastTheLastIsNotLeavingTheMap) {
  EXPECT_FALSE(StartTeam({TeamMissionType::kMove, 500})->Is_Leaving_Map());
}

TEST_F(TeamTest, ATeamMovingToANegativeWaypointIsNotLeavingTheMap) {
  EXPECT_FALSE(StartTeam({TeamMissionType::kMove, -1})->Is_Leaving_Map());
}

// The jump leaves the team past the end of its script until its next turn.
TEST_F(TeamTest, AJumpPastTheEndOfTheScriptEndsTheTeam) {
  TeamClass* const team = StartTeam({TeamMissionType::kLoop, 5});
  EXPECT_FALSE(team->Is_Leaving_Map());

  team->AI();

  EXPECT_EQ(TheObjectHeaps().team().Count(), 0);
}

TEST_F(TeamTest, AJumpToANegativeLineEndsTheTeam) {
  TeamClass* const team = StartTeam({TeamMissionType::kLoop, -3});
  EXPECT_FALSE(team->Is_Leaving_Map());

  team->AI();

  EXPECT_EQ(TheObjectHeaps().team().Count(), 0);
}

// The original game adds the best vehicle so far on every pass of its search,
// not once after it, so a single round of recruiting takes more than the
// nearest one.
TEST_F(TeamTest, RecruitsTheFirstVehicleFoundAndEachNearerOneAfterIt) {
  const Cell origin = Cell::FromXY(40, 50);
  base::At(TheScenario().Waypoint, 0) = origin;
  auto* team_type = base::NewPooled<TeamType>();
  ASSERT_NE(team_type, nullptr);
  team_type->set_house(HOUSE_USSR);
  team_type->set_origin(0);
  team_type->AddMember(
      {.quantity = 3, .type = &UnitTypeClass::As_Reference(UNIT_JEEP)});
  team_type->AddMission({TeamMissionType::kMove, 0});

  // In the order the search meets them: six cells away, nine, then three.
  auto* first = base::NewPooled<UnitClass>(UNIT_JEEP, HOUSE_USSR);
  auto* farther = base::NewPooled<UnitClass>(UNIT_JEEP, HOUSE_USSR);
  auto* nearer = base::NewPooled<UnitClass>(UNIT_JEEP, HOUSE_USSR);
  ASSERT_NE(nearer, nullptr);
  // With no rules loaded a vehicle is only placed while a scenario is read.
  TheWorld().scenario_init()++;
  ASSERT_TRUE(first->Unlimbo(Cell::FromXY(46, 50).Center()));
  ASSERT_TRUE(farther->Unlimbo(Cell::FromXY(49, 50).Center()));
  ASSERT_TRUE(nearer->Unlimbo(Cell::FromXY(43, 50).Center()));
  TheWorld().scenario_init()--;

  auto* team = base::NewPooled<TeamClass>(team_type, ThePlayer());
  ASSERT_NE(team, nullptr);
  team->AI();

  EXPECT_EQ(first->Team, team);
  EXPECT_EQ(farther->Team, nullptr);
  EXPECT_EQ(nearer->Team, team);
}

}  // namespace
}  // namespace ra
