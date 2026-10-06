// Tests for the team types: reading and writing their scenario INI entries,
// and the scenario editor's team type dialog played by a script on a headless
// screen.

#include "ra/ai/teamtype.h"

#include "base/pooled.h"
#include "engine/window/keyboard.h"
#include "gtest/gtest.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/core/type_heaps.h"
#include "ra/map/world.h"
#include "ra/objects/type.h"
#include "ra/startup/ui_test_util.h"

namespace ra {
namespace {

// A Game with every type a team can have as a member, reading the current
// scenario format, and one team type to work on.
class TeamTypeTest : public UiFixture {
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
  }

  TeamType* team_type_ = nullptr;
};

// The dialog is 400 by 250 in the screen's top left corner. A point inside
// the first line of the script list, the Append button and the OK button.
constexpr int kScriptX = 95 + 20;
constexpr int kScriptRow0Y = 125 + 4;
constexpr int kAppendX = 35 + 5;
constexpr int kAppendY = 119;
constexpr int kOkX = 264 + 5;
constexpr int kOkY = 221 + 4;

TEST_F(TeamTypeTest, ReadIniEntrySkipsAnUnknownFirstMember) {
  team_type_->ReadIniEntry("tanks", "2,0,7,0,1,-1,-1,2,BOGUS:1,E1:3,1,3:5");

  ASSERT_EQ(team_type_->ClassCount, 1);
  EXPECT_EQ(team_type_->Members[0].type,
            &InfantryTypeClass::As_Reference(INFANTRY_E1));
  EXPECT_EQ(team_type_->Members[0].quantity, 3);
  ASSERT_EQ(team_type_->MissionCount, 1);
  EXPECT_EQ(team_type_->MissionList[0].Mission, TeamMissionType::kMove);
  EXPECT_EQ(team_type_->MissionList[0].Data.Value, 5);
}

TEST_F(TeamTypeTest, ReadIniEntryTakesAnOriginThatIsNoWaypointAsNone) {
  team_type_->ReadIniEntry("tanks", "2,0,7,0,1,500,-1,0,0");

  EXPECT_EQ(team_type_->Origin, -1);
}

// The second click on a script line loads it into the work mission, which
// Append then adds to the script.
TEST_F(TeamTypeTest, ADoubleClickedAttackLineIsAppendedWithItsQuarry) {
  team_type_->MissionCount = 1;
  team_type_->MissionList[0].Mission = TeamMissionType::kAttack;
  team_type_->MissionList[0].Data.Quarry = QUARRY_BUILDINGS;
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

  ASSERT_EQ(team_type_->MissionCount, 2);
  EXPECT_EQ(team_type_->MissionList[1].Mission, TeamMissionType::kAttack);
  EXPECT_EQ(team_type_->MissionList[1].Data.Quarry, QUARRY_BUILDINGS);
}

// The maximum and origin fields, in the row of fields along the top.
constexpr int kMaxX = 202 + 3;
constexpr int kOriginX = 300 + 3;
constexpr int kFieldRowY = 27 + 3;

TEST_F(TeamTypeTest, TheDialogReadsTheOriginAsWaypointLetters) {
  player()
      .At(3)
      .Click(kOriginX, kFieldRowY)
      .At(5)
      .Type("AB")
      .At(12)
      .Click(kOkX, kOkY);

  EXPECT_TRUE(team_type_->Edit());

  EXPECT_EQ(team_type_->Origin, 27);
}

// "ZZ" names waypoint 701; the scenario has 101.
TEST_F(TeamTypeTest, TheDialogTakesOriginLettersPastTheLastWaypointAsNone) {
  player()
      .At(3)
      .Click(kOriginX, kFieldRowY)
      .At(5)
      .Type("ZZ")
      .At(12)
      .Click(kOkX, kOkY);

  EXPECT_TRUE(team_type_->Edit());

  EXPECT_EQ(team_type_->Origin, -1);
}

// The field takes a space after the first letter, and letters followed by a
// space come out below zero.
TEST_F(TeamTypeTest, TheDialogTakesOriginLettersThatAreNoWaypointAsNone) {
  player()
      .At(3)
      .Click(kOriginX, kFieldRowY)
      .At(5)
      .Type("A ")
      .At(12)
      .Click(kOkX, kOkY);

  EXPECT_TRUE(team_type_->Edit());

  EXPECT_EQ(team_type_->Origin, -1);
}

// The count fields take three digits and the team type stores a byte.
TEST_F(TeamTypeTest, TheDialogCapsTheMaximumAtWhatTheTeamTypeHolds) {
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

  EXPECT_EQ(team_type_->MaxAllowed, 255);
}

}  // namespace
}  // namespace ra
