// Tests for the scenario editor's team type dialog, TeamType::Edit(), played
// by a script on a headless screen.

#include "base/array.h"
#include "base/pooled.h"
#include "engine/window/keyboard.h"
#include "gtest/gtest.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/team_type.h"
#include "ra/ai/trigger_type.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/core/type_heaps.h"
#include "ra/map/world.h"
#include "ra/objects/house.h"
#include "ra/objects/type.h"
#include "ra/startup/ui_test_util.h"

namespace ra {
namespace {

// A Game with every type a team can have as a member, reading the current
// scenario format, and one team type to work on.
class TeamTypeDialogTest : public UiFixture {
 protected:
  void SetUp() override {
    TheTypeHeaps().aircraft().Reset(magic_enum::enum_count<AircraftType>());
    TheTypeHeaps().infantry().Reset(magic_enum::enum_count<InfantryType>());
    TheTypeHeaps().vessel().Reset(magic_enum::enum_count<VesselType>());
    AircraftTypeClass::Init_Heap();
    InfantryTypeClass::Init_Heap();
    VesselTypeClass::Init_Heap();
    TheObjectHeaps().team_type().Reset(4);
    TheObjectHeaps().trigger_type().Reset(4);
    TheWorld().new_ini_format() = 3;
    team_type_ = base::NewPooled<TeamType>();
    trigger_type_ = base::NewPooled<TriggerType>();
    trigger_type_->Set_Name("alrm");
  }

  // Double-clicks the first line of the script, which loads it into the work
  // mission, and appends the work mission; then accepts the dialog.
  void DoubleClickTheFirstLineAppendAndAccept();

  // Runs the dialog on a team type whose script is `line` alone, copying the
  // line as DoubleClickTheFirstLineAppendAndAccept() does, and returns the
  // copy.
  TeamMission CopyOf(const TeamMission& line);

  TeamType* team_type_ = nullptr;
  TriggerType* trigger_type_ = nullptr;
};

// The dialog is 400 by 250 in the screen's top left corner. A point inside
// the first line of the script list, the Append button and the OK button.
constexpr int kScriptX = 95 + 20;
constexpr int kScriptRow0Y = 125 + 4;
constexpr int kAppendX = 35 + 5;
constexpr int kAppendY = 119;
constexpr int kOkX = 264 + 5;
constexpr int kOkY = 221 + 4;

// The maximum and origin fields, in the row of fields along the top.
constexpr int kMaxX = 202 + 3;
constexpr int kOriginX = 300 + 3;
constexpr int kFieldRowY = 27 + 3;

// The other mission editing buttons, below Append, the second line of the
// script, the Cancel button, the first check box and the name and priority
// fields.
constexpr int kInsertY = 129;
constexpr int kDeleteY = 139;
constexpr int kReplaceY = 149;
constexpr int kScriptRow1Y = 125 + 9 + 4;
constexpr int kCancelX = 315 + 5;
constexpr int kRoundaboutX = 35 + 3;
constexpr int kRoundaboutY = 52 + 3;
constexpr int kNameX = 35 + 3;
constexpr int kPriorityX = 153 + 3;

void TeamTypeDialogTest::DoubleClickTheFirstLineAppendAndAccept() {
  player()
      .At(3)
      .Click(kScriptX, kScriptRow0Y)
      .At(6)
      .Click(kScriptX, kScriptRow0Y)
      .At(9)
      .Click(kAppendX, kAppendY)
      .At(12)
      .Click(kOkX, kOkY);
}

TeamMission TeamTypeDialogTest::CopyOf(const TeamMission& line) {
  team_type_->AddMission(line);
  DoubleClickTheFirstLineAppendAndAccept();

  EXPECT_TRUE(team_type_->Edit());

  EXPECT_EQ(team_type_->mission_count(), 2);
  return base::At(team_type_->missions(), 1);
}

TEST_F(TeamTypeDialogTest, OkAcceptsItAndNamesABlankTeamTypeWithDashes) {
  player().At(3).Click(kOkX, kOkY);

  EXPECT_TRUE(team_type_->Edit());

  EXPECT_STREQ(team_type_->IniName, "----");
  EXPECT_EQ(team_type_->house(), HOUSE_GOOD);
  EXPECT_EQ(team_type_->origin(), -1);
  EXPECT_EQ(team_type_->trigger(), nullptr);
  EXPECT_EQ(team_type_->mission_count(), 0);
}

TEST_F(TeamTypeDialogTest, CancelLeavesTheTeamTypeAsItWas) {
  team_type_->Set_Name("tanks");
  team_type_->AddMission({TeamMissionType::kMove, 3});
  player()
      .At(3)
      .Click(kRoundaboutX, kRoundaboutY)
      .At(6)
      .Click(kAppendX, kAppendY)
      .At(9)
      .Click(kCancelX, kOkY);

  EXPECT_FALSE(team_type_->Edit());

  EXPECT_STREQ(team_type_->IniName, "tanks");
  EXPECT_FALSE(team_type_->is_roundabout());
  EXPECT_EQ(team_type_->mission_count(), 1);
  EXPECT_EQ(team_type_->house(), HOUSE_NONE);
}

TEST_F(TeamTypeDialogTest, ACheckBoxSetsItsOption) {
  player().At(3).Click(kRoundaboutX, kRoundaboutY).At(6).Click(kOkX, kOkY);

  EXPECT_TRUE(team_type_->Edit());

  EXPECT_TRUE(team_type_->is_roundabout());
  EXPECT_FALSE(team_type_->is_suicide());
  EXPECT_TRUE(team_type_->is_prebuilt());
}

TEST_F(TeamTypeDialogTest, TheOptionsATeamTypeHasAreShownAndKept) {
  // Suicide and prebuilt, of the Soviet house, with a recruit priority of 9.
  team_type_->ReadIniEntry("tanks", "2,10,9,0,1,-1,-1,0,0");
  player().At(3).Click(kOkX, kOkY);

  EXPECT_TRUE(team_type_->Edit());

  EXPECT_STREQ(team_type_->IniName, "tanks");
  EXPECT_EQ(team_type_->house(), HousesType{2});
  EXPECT_FALSE(team_type_->is_roundabout());
  EXPECT_TRUE(team_type_->is_suicide());
  EXPECT_FALSE(team_type_->is_autocreate());
  EXPECT_TRUE(team_type_->is_prebuilt());
  EXPECT_FALSE(team_type_->is_reinforceable());
  EXPECT_EQ(team_type_->recruit_priority(), 9);
  EXPECT_EQ(team_type_->max_teams(), 1);
}

TEST_F(TeamTypeDialogTest, TheNameAndThePriorityAreTyped) {
  player()
      .At(3)
      .Click(kNameX, kFieldRowY)
      .At(5)
      .Type("tank")
      .At(12)
      .Click(kPriorityX, kFieldRowY)
      .At(14)
      .Key(engine::window::KN_BACKSPACE)
      .At(16)
      .Type("12")
      .At(22)
      .Click(kOkX, kOkY);

  EXPECT_TRUE(team_type_->Edit());

  EXPECT_STRCASEEQ(team_type_->IniName, "tank");
  EXPECT_EQ(team_type_->recruit_priority(), 12);
}

TEST_F(TeamTypeDialogTest, TheTriggerATeamTypeHasIsSelectedAndKept) {
  team_type_->set_trigger(trigger_type_);
  player().At(3).Click(kOkX, kOkY);

  EXPECT_TRUE(team_type_->Edit());

  EXPECT_EQ(team_type_->trigger(), trigger_type_);
}

TEST_F(TeamTypeDialogTest, TheOriginATeamTypeHasIsShownAndKept) {
  team_type_->set_origin(27);
  player().At(3).Click(kOkX, kOkY);

  EXPECT_TRUE(team_type_->Edit());

  EXPECT_EQ(team_type_->origin(), 27);
}

// A single click selects a line and loads nothing: the work mission stays the
// first mission of the list with the first quarry.
TEST_F(TeamTypeDialogTest, ASingleClickOnALineDoesNotLoadIt) {
  team_type_->AddMission({TeamMissionType::kMove, 3});
  player()
      .At(3)
      .Click(kScriptX, kScriptRow0Y)
      .At(6)
      .Click(kAppendX, kAppendY)
      .At(9)
      .Click(kOkX, kOkY);

  EXPECT_TRUE(team_type_->Edit());

  ASSERT_EQ(team_type_->mission_count(), 2);
  EXPECT_EQ(base::At(team_type_->missions(), 1).mission(),
            TeamMissionType::kAttack);
  EXPECT_EQ(base::At(team_type_->missions(), 1).quarry(), QuarryType{0});
}

TEST_F(TeamTypeDialogTest, ALineWithoutAnArgumentIsCopiedWithNone) {
  const TeamMission copy = CopyOf({TeamMissionType::kUnload, 0});

  EXPECT_EQ(copy.mission(), TeamMissionType::kUnload);
  EXPECT_EQ(copy.value(), 0);
}

TEST_F(TeamTypeDialogTest, ALineWithANumberIsCopiedWithIt) {
  const TeamMission copy = CopyOf({TeamMissionType::kGuard, 30});

  EXPECT_EQ(copy.mission(), TeamMissionType::kGuard);
  EXPECT_EQ(copy.value(), 30);
}

// A cell number is shown and read in hex.
TEST_F(TeamTypeDialogTest, ALineWithACellIsCopiedWithIt) {
  const TeamMission copy = CopyOf({TeamMissionType::kMoveCell, 0x1AF2});

  EXPECT_EQ(copy.mission(), TeamMissionType::kMoveCell);
  EXPECT_EQ(copy.value(), 0x1AF2);
}

// Typed digits may be of either case.
TEST_F(TeamTypeDialogTest, ACellIsTypedInHex) {
  constexpr int kArgumentX = 35 + 170 + 15 + 3;
  constexpr int kArgumentY = 103 + 3;
  team_type_->AddMission({TeamMissionType::kMoveCell, 0x1A});
  player()
      .At(3)
      .Click(kScriptX, kScriptRow0Y)
      .At(6)
      .Click(kScriptX, kScriptRow0Y)
      .At(9)
      .Click(kArgumentX, kArgumentY)
      .At(11)
      .Type("F")
      .At(15)
      .Click(kAppendX, kAppendY)
      .At(18)
      .Click(kOkX, kOkY);

  EXPECT_TRUE(team_type_->Edit());

  ASSERT_EQ(team_type_->mission_count(), 2);
  EXPECT_EQ(base::At(team_type_->missions(), 1).value(), 0x1AF);
}

// The field takes letters that are no hex digits.
TEST_F(TeamTypeDialogTest, ACellWithAStrayCharacterIsZero) {
  constexpr int kArgumentX = 35 + 170 + 15 + 3;
  constexpr int kArgumentY = 103 + 3;
  team_type_->AddMission({TeamMissionType::kMoveCell, 0x1A});
  player()
      .At(3)
      .Click(kScriptX, kScriptRow0Y)
      .At(6)
      .Click(kScriptX, kScriptRow0Y)
      .At(9)
      .Click(kArgumentX, kArgumentY)
      .At(11)
      .Type("Z")
      .At(15)
      .Click(kAppendX, kAppendY)
      .At(18)
      .Click(kOkX, kOkY);

  EXPECT_TRUE(team_type_->Edit());

  ASSERT_EQ(team_type_->mission_count(), 2);
  EXPECT_EQ(base::At(team_type_->missions(), 1).value(), 0);
}

TEST_F(TeamTypeDialogTest, ALineWithAWaypointIsCopiedWithIt) {
  const TeamMission copy = CopyOf({TeamMissionType::kMove, 27});

  EXPECT_EQ(copy.mission(), TeamMissionType::kMove);
  EXPECT_EQ(copy.value(), 27);
}

// The home, reinforcement and special waypoints have letters ("CU" to "CW")
// that the argument field does not take.
TEST_F(TeamTypeDialogTest, ACopiedWaypointPastTheLetteredOnesBecomesTheFirst) {
  const TeamMission copy = CopyOf({TeamMissionType::kMove, 98});

  EXPECT_EQ(copy.mission(), TeamMissionType::kMove);
  EXPECT_EQ(copy.value(), 0);
}

TEST_F(TeamTypeDialogTest, ALineWithAFormationIsCopiedWithIt) {
  TeamMission line(TeamMissionType::kFormation, 0);
  line.set_formation(FORMATION_LOOSE);

  const TeamMission copy = CopyOf(line);

  EXPECT_EQ(copy.mission(), TeamMissionType::kFormation);
  EXPECT_EQ(copy.formation(), FORMATION_LOOSE);
}

TEST_F(TeamTypeDialogTest, ALineWithAUnitMissionIsCopiedWithIt) {
  TeamMission line(TeamMissionType::kDo, 0);
  line.set_unit_mission(MISSION_GUARD);

  const TeamMission copy = CopyOf(line);

  EXPECT_EQ(copy.mission(), TeamMissionType::kDo);
  EXPECT_EQ(copy.unit_mission(), MISSION_GUARD);
}

// Loads the second line into the work mission, selects the first and presses
// the button at `button_y`.
class TeamTypeScriptEditTest : public TeamTypeDialogTest {
 protected:
  void SetUp() override {
    TeamTypeDialogTest::SetUp();
    team_type_->AddMission({TeamMissionType::kUnload, 0});
    team_type_->AddMission({TeamMissionType::kMove, 3});
  }

  void LoadTheSecondLineSelectTheFirstAndPress(const int button_y) {
    player()
        .At(3)
        .Click(kScriptX, kScriptRow1Y)
        .At(6)
        .Click(kScriptX, kScriptRow1Y)
        .At(9)
        .Click(kScriptX, kScriptRow0Y)
        .At(12)
        .Click(kAppendX, button_y)
        .At(15)
        .Click(kOkX, kOkY);
  }
};

TEST_F(TeamTypeScriptEditTest, InsertPutsTheWorkMissionBeforeTheSelectedLine) {
  LoadTheSecondLineSelectTheFirstAndPress(kInsertY);

  EXPECT_TRUE(team_type_->Edit());

  ASSERT_EQ(team_type_->mission_count(), 3);
  EXPECT_EQ(base::At(team_type_->missions(), 0).mission(),
            TeamMissionType::kMove);
  EXPECT_EQ(base::At(team_type_->missions(), 0).value(), 3);
  EXPECT_EQ(base::At(team_type_->missions(), 1).mission(),
            TeamMissionType::kUnload);
  EXPECT_EQ(base::At(team_type_->missions(), 2).mission(),
            TeamMissionType::kMove);
}

TEST_F(TeamTypeScriptEditTest, ReplaceOverwritesTheSelectedLine) {
  LoadTheSecondLineSelectTheFirstAndPress(kReplaceY);

  EXPECT_TRUE(team_type_->Edit());

  ASSERT_EQ(team_type_->mission_count(), 2);
  EXPECT_EQ(base::At(team_type_->missions(), 0).mission(),
            TeamMissionType::kMove);
  EXPECT_EQ(base::At(team_type_->missions(), 0).value(), 3);
  EXPECT_EQ(base::At(team_type_->missions(), 1).mission(),
            TeamMissionType::kMove);
}

TEST_F(TeamTypeScriptEditTest, DeleteRemovesTheSelectedLine) {
  LoadTheSecondLineSelectTheFirstAndPress(kDeleteY);

  EXPECT_TRUE(team_type_->Edit());

  ASSERT_EQ(team_type_->mission_count(), 1);
  EXPECT_EQ(base::At(team_type_->missions(), 0).mission(),
            TeamMissionType::kMove);
  EXPECT_EQ(base::At(team_type_->missions(), 0).value(), 3);
}

// The Members button, below the name field, and in the members dialog, which
// fills the 640 by 400 screen, the first member picture (the first infantry
// type) and the OK button.
constexpr int kMembersX = 35 + 5;
constexpr int kMembersY = 39 + 3;
constexpr int kFirstMemberX = 32 + 8;
constexpr int kFirstMemberY = 21 + 8;
constexpr int kMembersOkX = 265 + 5;
constexpr int kMembersOkY = 369 + 3;

// The members dialog redraws the map when it closes, sidebar and radar
// included, and those are drawn for the player's house.
class TeamTypeMembersDialogTest : public TeamTypeDialogTest {
 protected:
  TeamTypeMembersDialogTest() {
    ThePlayer() = base::NewPooled<HouseClass>(HOUSE_GREECE);
  }
};

TEST_F(TeamTypeMembersDialogTest, TheMembersDialogEditsThisTeamType) {
  player()
      .At(3)
      .Click(kMembersX, kMembersY)
      .At(6)
      .MoveTo(kFirstMemberX, kFirstMemberY)
      .At(9)
      .Click(kFirstMemberX, kFirstMemberY)
      .At(12)
      .Click(kMembersOkX, kMembersOkY)
      .At(15)
      .Click(kOkX, kOkY);

  EXPECT_TRUE(team_type_->Edit());

  ASSERT_EQ(team_type_->member_count(), 1);
  EXPECT_EQ(base::At(team_type_->members(), 0).type,
            &InfantryTypeClass::As_Reference(InfantryType{0}));
  EXPECT_EQ(base::At(team_type_->members(), 0).quantity, 1);
}

// The members dialog writes to the team type when it closes, before the team
// type dialog is answered.
TEST_F(TeamTypeMembersDialogTest,
       CancelPutsBackTheMembersTheMembersDialogChanged) {
  const TechnoTypeClass* const type =
      &AircraftTypeClass::As_Reference(AircraftType{0});
  team_type_->AddMember({.quantity = 2, .type = type});
  player()
      .At(3)
      .Click(kMembersX, kMembersY)
      .At(6)
      .MoveTo(kFirstMemberX, kFirstMemberY)
      .At(9)
      .Click(kFirstMemberX, kFirstMemberY)
      .At(12)
      .Click(kMembersOkX, kMembersOkY)
      .At(15)
      .Click(kCancelX, kOkY);

  EXPECT_FALSE(team_type_->Edit());

  ASSERT_EQ(team_type_->member_count(), 1);
  EXPECT_EQ(base::At(team_type_->members(), 0).type, type);
  EXPECT_EQ(base::At(team_type_->members(), 0).quantity, 2);
}

TEST_F(TeamTypeDialogTest, ReplaceAndDeleteDoNothingToAnEmptyScript) {
  player()
      .At(3)
      .Click(kAppendX, kReplaceY)
      .At(6)
      .Click(kAppendX, kDeleteY)
      .At(9)
      .Click(kOkX, kOkY);

  EXPECT_TRUE(team_type_->Edit());

  EXPECT_EQ(team_type_->mission_count(), 0);
}

TEST_F(TeamTypeDialogTest, AFullScriptTakesNoMoreLines) {
  for (int i = 0; i < TeamType::kMaxMissions; i++) {
    team_type_->AddMission({TeamMissionType::kUnload, 0});
  }
  player()
      .At(3)
      .Click(kAppendX, kAppendY)
      .At(6)
      .Click(kAppendX, kInsertY)
      .At(9)
      .Click(kOkX, kOkY);

  EXPECT_TRUE(team_type_->Edit());

  EXPECT_EQ(team_type_->mission_count(), TeamType::kMaxMissions);
}

// The second click on a script line loads it into the work mission, which
// Append then adds to the script.
TEST_F(TeamTypeDialogTest, ADoubleClickedAttackLineIsAppendedWithItsQuarry) {
  TeamMission attack(TeamMissionType::kAttack, 0);
  attack.set_quarry(QUARRY_BUILDINGS);
  team_type_->AddMission(attack);
  player()
      .At(3)
      .Click(kScriptX, kScriptRow0Y)
      .At(6)
      .Click(kScriptX, kScriptRow0Y)
      .At(9)
      .Click(kAppendX, kAppendY)
      .At(12)
      .Click(kOkX, kOkY);

  EXPECT_TRUE(team_type_->Edit());

  ASSERT_EQ(team_type_->mission_count(), 2);
  EXPECT_EQ(base::At(team_type_->missions(), 1).mission(),
            TeamMissionType::kAttack);
  EXPECT_EQ(base::At(team_type_->missions(), 1).quarry(), QUARRY_BUILDINGS);
}

TEST_F(TeamTypeDialogTest, TheDialogReadsTheOriginAsWaypointLetters) {
  player()
      .At(3)
      .Click(kOriginX, kFieldRowY)
      .At(5)
      .Type("AB")
      .At(12)
      .Click(kOkX, kOkY);

  EXPECT_TRUE(team_type_->Edit());

  EXPECT_EQ(team_type_->origin(), 27);
}

// "ZZ" names waypoint 701; the scenario has 101.
TEST_F(TeamTypeDialogTest,
       TheDialogTakesOriginLettersPastTheLastWaypointAsNone) {
  player()
      .At(3)
      .Click(kOriginX, kFieldRowY)
      .At(5)
      .Type("ZZ")
      .At(12)
      .Click(kOkX, kOkY);

  EXPECT_TRUE(team_type_->Edit());

  EXPECT_EQ(team_type_->origin(), -1);
}

// The field takes a space after the first letter, and letters followed by a
// space come out below zero.
TEST_F(TeamTypeDialogTest, TheDialogTakesOriginLettersThatAreNoWaypointAsNone) {
  player()
      .At(3)
      .Click(kOriginX, kFieldRowY)
      .At(5)
      .Type("A ")
      .At(12)
      .Click(kOkX, kOkY);

  EXPECT_TRUE(team_type_->Edit());

  EXPECT_EQ(team_type_->origin(), -1);
}

// The count fields take three digits and the team type stores a byte.
TEST_F(TeamTypeDialogTest, TheDialogCapsTheMaximumAtWhatTheTeamTypeHolds) {
  player()
      .At(3)
      .Click(kMaxX, kFieldRowY)
      .At(5)
      .Key(engine::window::KN_BACKSPACE)
      .At(7)
      .Type("999")
      .At(14)
      .Click(kOkX, kOkY);

  EXPECT_TRUE(team_type_->Edit());

  EXPECT_EQ(team_type_->max_teams(), 255);
}

}  // namespace
}  // namespace ra
