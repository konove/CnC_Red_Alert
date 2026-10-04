// Tests the reinforcements against a real Game with no game data: one house
// and the team types are built by hand.

#include "td/ai/reinf.h"

#include "base/array.h"
#include "base/pooled.h"
#include "gtest/gtest.h"
#include "td/ai/teamtype.h"
#include "td/core/cell.h"
#include "td/core/defines.h"
#include "td/core/object_heaps.h"
#include "td/core/target.h"
#include "td/map/display.h"
#include "td/map/layer.h"
#include "td/map/map.h"
#include "td/map/world.h"
#include "td/objects/house.h"
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
    // Do_Reinforcements() announces the player's own reinforcements.
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
  EXPECT_EQ(Create_Air_Reinforcement(house_, AIRCRAFT_A10, 2, MISSION_HUNT,
                                     Target{}, Target{}),
            0);
  EXPECT_EQ(ObjectCount(), 0);
}

TEST_F(ReinforcementTest, ATeamThatCannotBePlacedAtTheEdgeIsDeleted) {
  house_->Edge = SOURCE_WEST;
  TeamTypeClass& team_type = MakeTeamType();
  AddMembers(team_type, UnitTypeClass::As_Reference(UNIT_JEEP), 2);

  EXPECT_TRUE(Do_Reinforcements(&team_type));

  EXPECT_EQ(ObjectCount(), 0);
  EXPECT_EQ(TheWorld().scenario_init(), 0);
}

TEST_F(ReinforcementTest, AVehicleGroupEntersFromTheHouseEdge) {
  TheGameState().active() = true;
  house_->Edge = SOURCE_WEST;
  TeamTypeClass& team_type = MakeTeamType();
  AddMembers(team_type, UnitTypeClass::As_Reference(UNIT_JEEP), 2);

  EXPECT_TRUE(Do_Reinforcements(&team_type));

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

  Do_Reinforcements(&team_type);

  EXPECT_EQ(ObjectCount(), 0);
}

TEST_F(ReinforcementTest, AGroupForAHouseEdgeThatIsNoEdgeIsDeleted) {
  house_->Edge = SOURCE_VISIBLE;
  TeamTypeClass& team_type = MakeTeamType();
  AddMembers(team_type, UnitTypeClass::As_Reference(UNIT_JEEP), 2);

  Do_Reinforcements(&team_type);

  EXPECT_EQ(ObjectCount(), 0);
}

TEST_F(ReinforcementTest, TheGroundMembersOfAnAirGroupAreDeleted) {
  // No transport, so the soldier cannot ride in the A-10.
  TeamTypeClass& team_type = MakeTeamType();
  AddMembers(team_type, AircraftTypeClass::As_Reference(AIRCRAFT_A10), 1);
  AddMembers(team_type, InfantryTypeClass::As_Reference(INFANTRY_E1), 1);

  Do_Reinforcements(&team_type);

  EXPECT_EQ(ObjectCount(), 0);
}

TEST_F(ReinforcementTest, EveryGunboatThatCannotBePlacedIsDeleted) {
  TeamTypeClass& team_type = MakeTeamType();
  AddMembers(team_type, UnitTypeClass::As_Reference(UNIT_GUNBOAT), 2);

  EXPECT_FALSE(Do_Reinforcements(&team_type));

  EXPECT_EQ(ObjectCount(), 0);
}

TEST_F(ReinforcementTest, AFailedSpecialReinforcementLeavesNoTeamType) {
  // No room for the harvester, so the delivery fails after making its team.
  TheObjectHeaps().unit().Reset(0);

  EXPECT_FALSE(Create_Special_Reinforcement(
      house_, &UnitTypeClass::As_Reference(UNIT_HARVESTER), nullptr));

  EXPECT_EQ(TheObjectHeaps().team_type().Count(), 0);
  EXPECT_EQ(TheObjectHeaps().team().Count(), 0);
}

TEST_F(ReinforcementTest, ATeamTypeWithoutMembersDeliversNothing) {
  const TeamTypeClass& team_type = MakeTeamType();

  EXPECT_FALSE(Do_Reinforcements(&team_type));
  EXPECT_FALSE(Do_Reinforcements(nullptr));

  EXPECT_EQ(ObjectCount(), 0);
}

}  // namespace
}  // namespace td
