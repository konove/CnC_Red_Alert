// Tests for how a trigger action's data reads the values scenario INI files
// store, and for carrying the actions out in a real Game with no game data.

#include "ra/ai/taction.h"

#include <string_view>

#include "base/array.h"
#include "base/pooled.h"
#include "base/strings/tokenizer.h"
#include "gtest/gtest.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/scenario.h"
#include "ra/ai/team.h"
#include "ra/ai/teamtype.h"
#include "ra/ai/trigger.h"
#include "ra/ai/trigtype.h"
#include "ra/core/cell.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/core/type_heaps.h"
#include "ra/data/rules.h"
#include "ra/map/world.h"
#include "ra/objects/building.h"
#include "ra/objects/cell.h"
#include "ra/objects/house.h"
#include "ra/objects/type.h"
#include "ra/objects/unit.h"
#include "ra/startup/game_test_util.h"

namespace ra {

namespace {

// The shipped scenarios store a special weapon as its value in the low byte
// and 0xFF in the three above it: SCG12EA's "nuke" trigger holds -255, which
// the original game read, through a one-byte enum, as SPC_NUCLEAR_BOMB.
TEST(TriggerActionDataTest, SpecialWeaponReadsTheLowByteOfTheIniValue) {
  TriggerAction action;
  action.data.value = -255;
  EXPECT_EQ(action.data.special, SPC_NUCLEAR_BOMB);
  action.data.value = -252;
  EXPECT_EQ(action.data.special, SPC_PARA_INFANTRY);
  action.data.value = -1;
  EXPECT_EQ(action.data.special, SPC_NONE);
}

// SCG10EA's "lch3" trigger plays movie -170, which the original read as 86.
TEST(TriggerActionDataTest, MovieReadsTheLowByteOfTheIniValue) {
  TriggerAction action;
  action.data.value = -170;
  EXPECT_EQ(action.data.movie, static_cast<VQType>(86));
  action.data.value = -1;
  EXPECT_EQ(action.data.movie, VQ_NONE);
}

// A Game whose only house heap is empty: no house the actions name exists.
class TriggerActionTest : public GameFixture {
 protected:
  // Carries out `action` as a trigger of `house` that is attached to nothing.
  static bool Perform(TriggerAction& action,
                      const HousesType house = HOUSE_USSR) {
    return action.Perform(house, nullptr, -1, Cell{});
  }
};

// A trigger may belong to a house the scenario does not have.
TEST_F(TriggerActionTest, ActionsOnTheTriggersHouseSkipAMissingHouse) {
  TriggerAction action;
  action.type = TACTION_BASE_BUILDING;
  action.data.flag = true;
  EXPECT_TRUE(Perform(action));

  action.type = TACTION_1_SPECIAL;
  action.data.special = SPC_NUCLEAR_BOMB;
  EXPECT_TRUE(Perform(action));

  action.type = TACTION_FULL_SPECIAL;
  EXPECT_TRUE(Perform(action));
}

TEST_F(TriggerActionTest, ActionsOnTheNamedHouseSkipAMissingHouse) {
  for (const TriggerActionType type :
       {TACTION_ALL_HUNT, TACTION_BEGIN_PRODUCTION, TACTION_FIRE_SALE,
        TACTION_AUTOCREATE}) {
    TriggerAction action;
    action.type = type;
    action.data.house = HOUSE_GREECE;
    EXPECT_TRUE(Perform(action));
    action.data.house = HOUSE_NONE;
    EXPECT_TRUE(Perform(action));
  }
}

// Read_INI() leaves Team null when the scenario names a team that does not
// exist.
TEST_F(TriggerActionTest, TeamActionsSkipAMissingTeam) {
  TriggerAction action;
  action.type = TACTION_CREATE_TEAM;
  EXPECT_TRUE(Perform(action));
  action.type = TACTION_DESTROY_TEAM;
  EXPECT_TRUE(Perform(action));
  action.type = TACTION_REINFORCEMENTS;
  EXPECT_FALSE(Perform(action));
}

TEST_F(TriggerActionTest, ForceTriggerDoesNothingWhenNoTriggerCanBeMade) {
  TheObjectHeaps().trigger_type().Reset(1);
  TheObjectHeaps().trigger().Reset(0);
  TriggerAction action;
  action.type = TACTION_FORCE_TRIGGER;
  action.trigger_type = base::NewPooled<TriggerTypeClass>();
  ASSERT_NE(action.trigger_type, nullptr);
  EXPECT_TRUE(Perform(action));
}

TEST_F(TriggerActionTest, TextTriggerSkipsAnIdWithNoText) {
  TriggerAction action;
  action.type = TACTION_TEXT_TRIGGER;
  action.data.value = -1;
  EXPECT_TRUE(Perform(action));
  action.data.value = 0;
  EXPECT_TRUE(Perform(action));
}

TEST(TriggerActionNameTest, EveryActionHasAnEditorName) {
  EXPECT_STREQ(ActionName(TACTION_NONE), "-No Action-");
  EXPECT_STREQ(ActionName(TACTION_CREATE_TEAM), "Create Team...");
  EXPECT_STREQ(ActionName(TACTION_LAUNCH_NUKES), "Launch Nukes");
}

TEST(TriggerActionNeedTest, EachActionAsksForTheValueItUses) {
  EXPECT_EQ(ActionNeed(TACTION_1_SPECIAL), NEED_SPECIAL);
  EXPECT_EQ(ActionNeed(TACTION_FULL_SPECIAL), NEED_SPECIAL);
  for (const TriggerActionType type :
       {TACTION_FIRE_SALE, TACTION_WIN, TACTION_LOSE, TACTION_ALL_HUNT,
        TACTION_BEGIN_PRODUCTION, TACTION_AUTOCREATE}) {
    EXPECT_EQ(ActionNeed(type), NEED_HOUSE);
  }
  EXPECT_EQ(ActionNeed(TACTION_BASE_BUILDING), NEED_BOOL);
  for (const TriggerActionType type :
       {TACTION_CREATE_TEAM, TACTION_DESTROY_TEAM, TACTION_REINFORCEMENTS}) {
    EXPECT_EQ(ActionNeed(type), NEED_TEAM);
  }
  EXPECT_EQ(ActionNeed(TACTION_FORCE_TRIGGER), NEED_TRIGGER);
  EXPECT_EQ(ActionNeed(TACTION_DESTROY_TRIGGER), NEED_TRIGGER);
  for (const TriggerActionType type :
       {TACTION_DZ, TACTION_REVEAL_SOME, TACTION_REVEAL_ZONE}) {
    EXPECT_EQ(ActionNeed(type), NEED_WAYPOINT);
  }
  EXPECT_EQ(ActionNeed(TACTION_PLAY_MUSIC), NEED_THEME);
  EXPECT_EQ(ActionNeed(TACTION_PLAY_MOVIE), NEED_MOVIE);
  EXPECT_EQ(ActionNeed(TACTION_PLAY_SOUND), NEED_SOUND);
  EXPECT_EQ(ActionNeed(TACTION_PLAY_SPEECH), NEED_SPEECH);
  for (const TriggerActionType type :
       {TACTION_TEXT_TRIGGER, TACTION_ADD_TIMER, TACTION_SUB_TIMER,
        TACTION_SET_TIMER, TACTION_SET_GLOBAL, TACTION_CLEAR_GLOBAL}) {
    EXPECT_EQ(ActionNeed(type), NEED_NUMBER);
  }
  EXPECT_EQ(ActionNeed(TACTION_PREFERRED_TARGET), NEED_QUARRY);
  for (const TriggerActionType type :
       {TACTION_NONE, TACTION_WINLOSE, TACTION_ALLOWWIN, TACTION_REVEAL_ALL,
        TACTION_START_TIMER, TACTION_STOP_TIMER, TACTION_CREEP_SHADOW,
        TACTION_DESTROY_OBJECT, TACTION_LAUNCH_NUKES}) {
    EXPECT_EQ(ActionNeed(type), NEED_NONE);
  }
}

// A Game with the player's house, a team type and a trigger type.
class TriggerActionGameTest : public TriggerActionTest {
 protected:
  TriggerActionGameTest() {
    TheObjectHeaps().team_type().Reset(2);
    TheObjectHeaps().team().Reset(2);
    TheObjectHeaps().trigger_type().Reset(2);
    TheObjectHeaps().trigger().Reset(4);
    house_ = base::NewPooled<HouseClass>(HOUSE_USSR);
    ThePlayer() = house_;
    // The heaps have to be sized before anything is allocated from them.
    // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
    team_type_ = base::NewPooled<TeamTypeClass>();
    team_type_->Set_Name("tanks");
    team_type_->House = HOUSE_USSR;
    // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
    trigger_type_ = base::NewPooled<TriggerTypeClass>();
    trigger_type_->Set_Name("alarm");
  }

  // The action `text` describes in the scenario INI's format `format`.
  static TriggerAction Read(const std::string_view text, const int format) {
    TheWorld().new_ini_format() = format;
    base::Tokenizer tokens(text);
    TriggerAction action;
    action.ReadIni(tokens);
    return action;
  }

  HouseClass* house_ = nullptr;
  TeamTypeClass* team_type_ = nullptr;
  TriggerTypeClass* trigger_type_ = nullptr;
};

TEST_F(TriggerActionGameTest, DetachClearsOnlyTheTypeBeingDestroyed) {
  TriggerAction action;
  action.team_type = team_type_;
  action.trigger_type = trigger_type_;

  action.Detach(Target{});
  EXPECT_EQ(action.team_type, team_type_);
  EXPECT_EQ(action.trigger_type, trigger_type_);

  action.Detach(team_type_->As_Target());
  EXPECT_EQ(action.team_type, nullptr);
  EXPECT_EQ(action.trigger_type, trigger_type_);

  action.Detach(trigger_type_->As_Target());
  EXPECT_EQ(action.trigger_type, nullptr);
}

TEST_F(TriggerActionGameTest, DetachLeavesAnotherTeamOrTriggerTypeAlone) {
  TriggerAction action;
  action.team_type = team_type_;
  action.trigger_type = trigger_type_;

  action.Detach(base::NewPooled<TeamTypeClass>()->As_Target());
  action.Detach(base::NewPooled<TriggerTypeClass>()->As_Target());

  EXPECT_EQ(action.team_type, team_type_);
  EXPECT_EQ(action.trigger_type, trigger_type_);
}

TEST_F(TriggerActionGameTest, TheIniEntryNumbersTheTeamAndTheTrigger) {
  TriggerAction action;
  action.type = TACTION_CREATE_TEAM;
  action.team_type = team_type_;
  action.data.value = 7;
  EXPECT_EQ(action.ToIniEntry(), "4,0,-1,7");

  action.type = TACTION_FORCE_TRIGGER;
  action.team_type = nullptr;
  action.trigger_type = trigger_type_;
  action.data.value = -1;
  EXPECT_EQ(action.ToIniEntry(), "22,-1,0,-1");
}

TEST_F(TriggerActionGameTest, TheCurrentFormatReadsWhatToIniEntryWrites) {
  const TriggerAction action = Read("22,0,0,5", 2);

  EXPECT_EQ(action.type, TACTION_FORCE_TRIGGER);
  EXPECT_EQ(action.team_type, team_type_);
  EXPECT_EQ(action.trigger_type, trigger_type_);
  EXPECT_EQ(action.data.value, 5);
  EXPECT_TRUE(action.pending_trigger_name.empty());
  EXPECT_EQ(Read(action.ToIniEntry(), 2).ToIniEntry(), "22,0,0,5");
}

TEST_F(TriggerActionGameTest, TheCurrentFormatReadsNoTeamOrTriggerAsNull) {
  const TriggerAction action = Read("1,-1,-1,2", 2);

  EXPECT_EQ(action.type, TACTION_WIN);
  EXPECT_EQ(action.team_type, nullptr);
  EXPECT_EQ(action.trigger_type, nullptr);
  EXPECT_EQ(action.data.value, 2);
}

TEST_F(TriggerActionGameTest, TheOldFormatsNameTheTeamAndKeepTheTriggerName) {
  for (const int format : {0, 1}) {
    const TriggerAction action = Read("4,TANKS,alarm,3", format);

    EXPECT_EQ(action.type, TACTION_CREATE_TEAM);
    EXPECT_EQ(action.team_type, team_type_);
    // TriggerTypeClass::Read_INI() resolves the name later.
    EXPECT_EQ(action.trigger_type, nullptr);
    EXPECT_EQ(action.pending_trigger_name, "alarm");
    EXPECT_EQ(action.data.value, 3);
  }
}

TEST_F(TriggerActionGameTest, TheOldFormatsLeaveAMissingTeamNull) {
  const TriggerAction action = Read("4,nobody,None,0", 0);

  EXPECT_EQ(action.type, TACTION_CREATE_TEAM);
  EXPECT_EQ(action.team_type, nullptr);
}

// SCU31EA and others store a sound as 0xFFFF00xx.
TEST_F(TriggerActionGameTest, ASoundOutOfRangeIsReadAsItsLowByte) {
  EXPECT_EQ(Read("19,-1,-1,-65399", 2).data.value, 137);
  EXPECT_EQ(Read("19,-1,-1,65673", 2).data.value, 137);
  EXPECT_EQ(Read("19,-1,-1,137", 2).data.value, 137);
  // Only a sound is corrected.
  EXPECT_EQ(Read("11,-1,-1,-65399", 2).data.value, -65399);
}

// 0xFF is no sound either, so the value is kept as it is.
TEST_F(TriggerActionGameTest, ASoundWhoseLowByteIsNoSoundIsKept) {
  EXPECT_EQ(Read("19,-1,-1,-1", 2).data.value, -1);
}

TEST_F(TriggerActionGameTest, NoActionSucceedsAndChangesNothing) {
  for (const TriggerActionType type :
       {TACTION_NONE, TACTION_WINLOSE, TACTION_ALLOWWIN}) {
    TriggerAction action;
    action.type = type;
    EXPECT_TRUE(Perform(action));
  }
  EXPECT_FALSE(house_->IsToWin);
  EXPECT_FALSE(house_->IsToLose);
}

TEST_F(TriggerActionGameTest, HouseActionsChangeTheTriggersHouse) {
  TriggerAction action;
  action.type = TACTION_PREFERRED_TARGET;
  action.data.quarry = QUARRY_HARVESTERS;
  EXPECT_TRUE(Perform(action));
  EXPECT_EQ(house_->PreferredTarget, QUARRY_HARVESTERS);

  action.type = TACTION_BASE_BUILDING;
  action.data.value = 0;
  action.data.flag = true;
  EXPECT_TRUE(Perform(action));
  EXPECT_TRUE(house_->IsBaseBuilding);
}

TEST_F(TriggerActionGameTest, PreferredTargetSkipsAMissingHouse) {
  TriggerAction action;
  action.type = TACTION_PREFERRED_TARGET;
  action.data.quarry = QUARRY_HARVESTERS;
  EXPECT_TRUE(Perform(action, HOUSE_GREECE));
  EXPECT_EQ(house_->PreferredTarget, QUARRY_ANYTHING);
}

TEST_F(TriggerActionGameTest, NamedHouseActionsChangeThatHouse) {
  TriggerAction action;
  action.data.value = 0;
  action.data.house = HOUSE_USSR;

  action.type = TACTION_BEGIN_PRODUCTION;
  EXPECT_TRUE(Perform(action, HOUSE_GREECE));
  EXPECT_TRUE(house_->IsStarted);

  action.type = TACTION_FIRE_SALE;
  EXPECT_TRUE(Perform(action, HOUSE_GREECE));
  EXPECT_EQ(house_->State, STATE_ENDGAME);

  action.type = TACTION_AUTOCREATE;
  EXPECT_TRUE(Perform(action, HOUSE_GREECE));
  EXPECT_TRUE(house_->IsAlerted);
}

TEST_F(TriggerActionGameTest, GlobalsAreSetAndCleared) {
  TriggerAction action;
  action.type = TACTION_SET_GLOBAL;
  action.data.value = 3;
  EXPECT_TRUE(Perform(action));
  EXPECT_TRUE(base::At(TheScenario().GlobalFlags, 3));

  action.type = TACTION_CLEAR_GLOBAL;
  EXPECT_TRUE(Perform(action));
  EXPECT_FALSE(base::At(TheScenario().GlobalFlags, 3));
}

TEST_F(TriggerActionGameTest, WinAndLoseFlagThePlayerByTheNamedHouse) {
  TriggerAction action;
  action.data.value = 0;
  action.type = TACTION_WIN;
  action.data.house = HOUSE_USSR;
  EXPECT_TRUE(Perform(action));
  EXPECT_TRUE(house_->IsToWin);

  house_->IsToWin = false;
  action.type = TACTION_LOSE;
  action.data.house = HOUSE_GREECE;
  EXPECT_TRUE(Perform(action));
  EXPECT_TRUE(house_->IsToWin);
}

TEST_F(TriggerActionGameTest, TheOtherHouseWinningMeansThePlayerLoses) {
  TriggerAction action;
  action.data.value = 0;
  action.type = TACTION_WIN;
  action.data.house = HOUSE_GREECE;
  EXPECT_TRUE(Perform(action));
  EXPECT_TRUE(house_->IsToLose);

  house_->IsToLose = false;
  action.type = TACTION_LOSE;
  action.data.house = HOUSE_USSR;
  EXPECT_TRUE(Perform(action));
  EXPECT_TRUE(house_->IsToLose);
}

TEST_F(TriggerActionGameTest, DestroyTriggerDeletesEveryTriggerOfTheType) {
  auto* other_type = base::NewPooled<TriggerTypeClass>();
  base::NewPooled<TriggerClass>(trigger_type_);
  const TriggerClass* kept = base::NewPooled<TriggerClass>(other_type);
  base::NewPooled<TriggerClass>(trigger_type_);
  base::NewPooled<TriggerClass>(trigger_type_);
  ASSERT_EQ(TheObjectHeaps().trigger().Count(), 4);

  TriggerAction action;
  action.type = TACTION_DESTROY_TRIGGER;
  action.trigger_type = trigger_type_;
  EXPECT_TRUE(Perform(action));

  ASSERT_EQ(TheObjectHeaps().trigger().Count(), 1);
  EXPECT_EQ(TheObjectHeaps().trigger().ActiveAt(0), kept);
}

TEST_F(TriggerActionGameTest, DestroyTriggerWithNoTypeDeletesNothing) {
  base::NewPooled<TriggerClass>(trigger_type_);
  TriggerAction action;
  action.type = TACTION_DESTROY_TRIGGER;
  EXPECT_TRUE(Perform(action));
  EXPECT_EQ(TheObjectHeaps().trigger().Count(), 1);
}

// The forced trigger has no action of its own, so springing it only makes it.
TEST_F(TriggerActionGameTest, ForceTriggerMakesATriggerOfTheType) {
  TriggerAction action;
  action.type = TACTION_FORCE_TRIGGER;
  action.trigger_type = trigger_type_;
  EXPECT_TRUE(Perform(action));
  EXPECT_TRUE(Perform(action));
  // A volatile trigger is deleted once it has sprung.
  EXPECT_LE(TheObjectHeaps().trigger().Count(), 1);
}

TEST_F(TriggerActionGameTest, DestroyTeamRemovesTheTeamsOfTheType) {
  base::NewPooled<TeamClass>(team_type_, house_);
  ASSERT_EQ(TheObjectHeaps().team().Count(), 1);

  TriggerAction action;
  action.type = TACTION_DESTROY_TEAM;
  action.team_type = team_type_;
  EXPECT_TRUE(Perform(action));

  EXPECT_EQ(TheObjectHeaps().team().Count(), 0);
}

TEST_F(TriggerActionGameTest, CreateTeamMakesOneAsScenarioSetup) {
  TriggerAction action;
  action.type = TACTION_CREATE_TEAM;
  action.team_type = team_type_;
  EXPECT_TRUE(Perform(action));

  EXPECT_EQ(TheObjectHeaps().team().Count(), 1);
  EXPECT_EQ(TheWorld().scenario_init(), 0);
}

// A team type with no members brings nothing on.
TEST_F(TriggerActionGameTest, ReinforcementsReportATeamThatCannotArrive) {
  TriggerAction action;
  action.type = TACTION_REINFORCEMENTS;
  action.team_type = team_type_;
  EXPECT_FALSE(Perform(action));
}

TEST_F(TriggerActionGameTest, DestroyObjectFailsWithNothingToDestroy) {
  TriggerAction action;
  action.type = TACTION_DESTROY_OBJECT;
  EXPECT_FALSE(Perform(action));
}

TEST_F(TriggerActionGameTest, DestroyObjectKillsTheObjectTheTriggerSprangOn) {
  UnitTypeClass::As_Reference(UNIT_JEEP).MaxStrength = 100;
  auto* jeep = base::NewPooled<UnitClass>(UNIT_JEEP, HOUSE_USSR);
  ASSERT_NE(jeep, nullptr);
  ASSERT_GT(jeep->Strength, 0);

  TriggerAction action;
  action.type = TACTION_DESTROY_OBJECT;
  EXPECT_TRUE(action.Perform(HOUSE_USSR, jeep, -1, Cell{}));

  EXPECT_EQ(TheObjectHeaps().unit().Count(), 0);
}

TEST_F(TriggerActionGameTest, DestroyObjectIgnoresAnObjectAlreadyDead) {
  UnitTypeClass::As_Reference(UNIT_JEEP).MaxStrength = 100;
  auto* jeep = base::NewPooled<UnitClass>(UNIT_JEEP, HOUSE_USSR);
  ASSERT_NE(jeep, nullptr);
  jeep->IsActive = false;

  TriggerAction action;
  action.type = TACTION_DESTROY_OBJECT;
  EXPECT_FALSE(action.Perform(HOUSE_USSR, jeep, -1, Cell{}));

  jeep->IsActive = true;
  EXPECT_EQ(jeep->Strength, 100);
}

TEST_F(TriggerActionGameTest, DestroyObjectKillsEverythingTheTriggerIsOn) {
  UnitTypeClass::As_Reference(UNIT_JEEP).MaxStrength = 100;
  auto* trigger = base::NewPooled<TriggerClass>(trigger_type_);
  auto* other = base::NewPooled<TriggerClass>(trigger_type_);
  auto* attached = base::NewPooled<UnitClass>(UNIT_JEEP, HOUSE_USSR);
  auto* bystander = base::NewPooled<UnitClass>(UNIT_JEEP, HOUSE_USSR);
  attached->Trigger = trigger;
  bystander->Trigger = other;

  TriggerAction action;
  action.type = TACTION_DESTROY_OBJECT;
  // Nothing sprang it, so the action itself fails.
  EXPECT_FALSE(action.Perform(HOUSE_USSR, nullptr,
                              TheObjectHeaps().trigger().SlotIndexOf(trigger),
                              Cell{}));

  ASSERT_EQ(TheObjectHeaps().unit().Count(), 1);
  EXPECT_EQ(TheObjectHeaps().unit().ActiveAt(0), bystander);
  EXPECT_EQ(bystander->Trigger, other);
}

TEST_F(TriggerActionGameTest, TheTimerStartsStopsAndIsSetInTenthsOfAMinute) {
  TriggerAction action;
  action.type = TACTION_SET_TIMER;
  action.data.value = 20;
  EXPECT_TRUE(Perform(action));
  EXPECT_TRUE(TheScenario().MissionTimer.IsRunning());
  EXPECT_EQ(TheScenario().MissionTimer.Remaining(), 2 * kTicksPerMinute);

  action.type = TACTION_STOP_TIMER;
  EXPECT_TRUE(Perform(action));
  EXPECT_FALSE(TheScenario().MissionTimer.IsRunning());
  // Stopping a stopped timer changes nothing.
  EXPECT_TRUE(Perform(action));
  EXPECT_FALSE(TheScenario().MissionTimer.IsRunning());

  action.type = TACTION_START_TIMER;
  EXPECT_TRUE(Perform(action));
  EXPECT_TRUE(TheScenario().MissionTimer.IsRunning());
  EXPECT_TRUE(Perform(action));
  EXPECT_TRUE(TheScenario().MissionTimer.IsRunning());
  EXPECT_EQ(TheScenario().MissionTimer.Remaining(), 2 * kTicksPerMinute);
}

TEST_F(TriggerActionGameTest, TheTimerIsExtendedAndShortenedDownToZero) {
  TheScenario().MissionTimer.Set(kTicksPerMinute);
  TheScenario().MissionTimer.Stop();

  TriggerAction action;
  action.type = TACTION_ADD_TIMER;
  action.data.value = 5;
  EXPECT_TRUE(Perform(action));
  EXPECT_EQ(TheScenario().MissionTimer.Remaining(),
            kTicksPerMinute + (kTicksPerMinute / 2));

  action.type = TACTION_SUB_TIMER;
  action.data.value = 10;
  EXPECT_TRUE(Perform(action));
  EXPECT_EQ(TheScenario().MissionTimer.Remaining(), kTicksPerMinute / 2);

  EXPECT_TRUE(Perform(action));
  EXPECT_EQ(TheScenario().MissionTimer.Remaining(), 0);
}

TEST_F(TriggerActionGameTest, ASpecialWeaponIsGivenOnceOrForGood) {
  // The sidebar button is the player's; give the weapon to another house.
  auto* greece = base::NewPooled<HouseClass>(HOUSE_GREECE);
  TriggerAction action;
  action.data.value = 0;
  action.data.special = SPC_NUCLEAR_BOMB;

  action.type = TACTION_1_SPECIAL;
  EXPECT_TRUE(Perform(action, HOUSE_GREECE));
  EXPECT_TRUE(greece->SuperWeapon.at(SPC_NUCLEAR_BOMB).Is_Present());
  EXPECT_TRUE(greece->SuperWeapon.at(SPC_NUCLEAR_BOMB).Is_One_Time());

  action.data.special = SPC_SONAR_PULSE;
  action.type = TACTION_FULL_SPECIAL;
  EXPECT_TRUE(Perform(action, HOUSE_GREECE));
  EXPECT_TRUE(greece->SuperWeapon.at(SPC_SONAR_PULSE).Is_Present());
  EXPECT_FALSE(greece->SuperWeapon.at(SPC_SONAR_PULSE).Is_One_Time());
  EXPECT_FALSE(house_->SuperWeapon.at(SPC_SONAR_PULSE).Is_Present());
}

TEST_F(TriggerActionGameTest, AllHuntReachesTheNamedHouse) {
  TriggerAction action;
  action.type = TACTION_ALL_HUNT;
  action.data.value = 0;
  action.data.house = HOUSE_USSR;
  EXPECT_TRUE(Perform(action, HOUSE_GREECE));
}

TEST_F(TriggerActionGameTest, LaunchNukesWithNoSilosDoesNothing) {
  TriggerAction action;
  action.type = TACTION_LAUNCH_NUKES;
  EXPECT_TRUE(Perform(action));
}

TEST_F(TriggerActionGameTest, RevealAllMakesThePlayerSeeTheWholeMap) {
  TriggerAction action;
  action.type = TACTION_REVEAL_ALL;
  EXPECT_TRUE(Perform(action));
  EXPECT_TRUE(house_->IsVisionary);
  EXPECT_TRUE(TheMap().at(Cell::FromXY(10, 10)).IsMapped);
  // Nothing more to reveal the second time.
  EXPECT_TRUE(Perform(action));
}

TEST_F(TriggerActionGameTest, RevealActionsDoNothingForAPlayerWhoSeesAll) {
  house_->IsVisionary = true;
  base::At(TheScenario().Waypoint, 0) = Cell::FromXY(10, 10);
  TriggerAction action;
  action.data.value = 0;
  for (const TriggerActionType type :
       {TACTION_REVEAL_SOME, TACTION_REVEAL_ZONE, TACTION_REVEAL_ALL}) {
    action.type = type;
    EXPECT_TRUE(Perform(action));
  }
  EXPECT_FALSE(TheMap().at(Cell::FromXY(10, 10)).IsMapped);
}

TEST_F(TriggerActionGameTest, RevealZoneMapsTheCellsOfTheWaypointsZone) {
  base::At(TheScenario().Waypoint, 0) = Cell::FromXY(10, 10);
  TriggerAction action;
  action.type = TACTION_REVEAL_ZONE;
  action.data.value = 0;
  EXPECT_TRUE(Perform(action));
  EXPECT_TRUE(TheMap().at(Cell::FromXY(10, 10)).IsMapped);
}

TEST_F(TriggerActionGameTest, RevealSomeMapsAroundTheWaypoint) {
  base::At(TheScenario().Waypoint, 0) = Cell::FromXY(10, 10);
  TheRules().GapShroudRadius = 3;
  TriggerAction action;
  action.type = TACTION_REVEAL_SOME;
  action.data.value = 0;
  EXPECT_TRUE(Perform(action));
  EXPECT_TRUE(TheMap().at(Cell::FromXY(10, 10)).IsMapped);
  EXPECT_FALSE(TheMap().at(Cell::FromXY(40, 40)).IsMapped);
}

TEST_F(TriggerActionGameTest, CreepShadowRuns) {
  TriggerAction action;
  action.type = TACTION_CREEP_SHADOW;
  EXPECT_TRUE(Perform(action));
}

TEST_F(TriggerActionGameTest, LaunchNukesOrdersEverySiloToFire) {
  TheTypeHeaps().building().Reset(magic_enum::enum_count<StructType>());
  BuildingTypeClass::Init_Heap();
  BuildingTypeClass::As_Reference(STRUCT_MSLO).MaxStrength = 500;
  BuildingTypeClass::As_Reference(STRUCT_BARRACKS).MaxStrength = 500;
  TheObjectHeaps().building().Reset(2);
  auto* silo = base::NewPooled<BuildingClass>(STRUCT_MSLO, HOUSE_USSR);
  auto* barracks = base::NewPooled<BuildingClass>(STRUCT_BARRACKS, HOUSE_USSR);
  ASSERT_NE(silo, nullptr);
  ASSERT_NE(barracks, nullptr);
  const MissionType barracks_orders = barracks->MissionQueue;

  TriggerAction action;
  action.type = TACTION_LAUNCH_NUKES;
  EXPECT_TRUE(Perform(action));

  EXPECT_EQ(silo->MissionQueue, MISSION_MISSILE);
  EXPECT_EQ(barracks->MissionQueue, barracks_orders);
}

TEST_F(TriggerActionGameTest, DestroyObjectOnACellWithNoBridgeStillSucceeds) {
  UnitTypeClass::As_Reference(UNIT_JEEP).MaxStrength = 100;
  auto* jeep = base::NewPooled<UnitClass>(UNIT_JEEP, HOUSE_USSR);

  TriggerAction action;
  action.type = TACTION_DESTROY_OBJECT;
  EXPECT_TRUE(action.Perform(HOUSE_USSR, jeep, -1, Cell::FromXY(10, 10)));

  EXPECT_EQ(TheObjectHeaps().unit().Count(), 0);
}

}  // namespace

}  // namespace ra
