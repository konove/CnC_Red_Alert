// Tests for how a trigger action's data reads the values scenario INI files
// store, and for carrying the actions out in a real Game with no game data.

#include "ra/ai/trigger_action.h"

#include <string_view>

#include "base/array.h"
#include "base/pooled.h"
#include "base/strings/tokenizer.h"
#include "gtest/gtest.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/scenario.h"
#include "ra/ai/team.h"
#include "ra/ai/team_type.h"
#include "ra/ai/trigger.h"
#include "ra/ai/trigger_type.h"
#include "ra/assets/assets.h"
#include "ra/core/cell.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/core/type_heaps.h"
#include "ra/data/rules.h"
#include "ra/map/world.h"
#include "ra/net/session.h"
#include "ra/objects/building.h"
#include "ra/objects/cell.h"
#include "ra/objects/house.h"
#include "ra/objects/type.h"
#include "ra/objects/unit.h"
#include "ra/startup/game_test_util.h"
#include "ra/ui/msglist.h"

namespace ra {

namespace {

// The record of an action of `type` that takes `value`.
template <class Value = int>
TriggerActionRecord Record(const TriggerActionType type,
                           const Value value = -1) {
  return {.type = type, .value = static_cast<int>(value)};
}

// The value of the action MakeTriggerAction() makes of `type` and `value`.
int KeptValue(const TriggerActionType type, const int value) {
  return MakeTriggerAction(Record(type, value))->ToRecord().value;
}

// The shipped scenarios store a special weapon as its value in the low byte
// and 0xFF in the three above it: SCG12EA's "nuke" trigger holds -255, which
// the original game read, through a one-byte enum, as SPC_NUCLEAR_BOMB.
TEST(TriggerActionDataTest, SpecialWeaponReadsTheLowByteOfTheIniValue) {
  for (const TriggerActionType type :
       {TriggerActionType::kOneSpecial, TriggerActionType::kFullSpecial}) {
    EXPECT_EQ(KeptValue(type, -255), static_cast<int>(SPC_NUCLEAR_BOMB));
    EXPECT_EQ(KeptValue(type, -252), static_cast<int>(SPC_PARA_INFANTRY));
    EXPECT_EQ(KeptValue(type, -1), static_cast<int>(SPC_NONE));
  }
}

// SCG10EA's "lch3" trigger plays movie -170, which the original read as 86.
TEST(TriggerActionDataTest, MovieReadsTheLowByteOfTheIniValue) {
  EXPECT_EQ(KeptValue(TriggerActionType::kPlayMovie, -170), 86);
  EXPECT_EQ(KeptValue(TriggerActionType::kPlayMovie, -1),
            static_cast<int>(VQ_NONE));
}

// The editor used to set a house over an unset value of -1 and leave the
// three bytes above it alone; scenarios saved that way are still read.
TEST(TriggerActionDataTest, HouseReadsTheLowByteOfTheIniValue) {
  for (const TriggerActionType type :
       {TriggerActionType::kWin, TriggerActionType::kLose,
        TriggerActionType::kBeginProduction, TriggerActionType::kAllHunt,
        TriggerActionType::kFireSale, TriggerActionType::kAutocreate}) {
    EXPECT_EQ(KeptValue(type, -256 + static_cast<int>(HOUSE_GREECE)),
              static_cast<int>(HOUSE_GREECE));
    EXPECT_EQ(KeptValue(type, -1), static_cast<int>(HOUSE_NONE));
  }
}

TEST(TriggerActionDataTest, SpeechReadsTheLowByteOfTheIniValue) {
  EXPECT_EQ(KeptValue(TriggerActionType::kPlaySpeech,
                      -256 + static_cast<int>(VOX_FAIL)),
            static_cast<int>(VOX_FAIL));
  EXPECT_EQ(KeptValue(TriggerActionType::kPlaySpeech, -1),
            static_cast<int>(VOX_NONE));
}

TEST(TriggerActionDataTest, AFlagIsWhetherTheLowByteIsSet) {
  EXPECT_EQ(KeptValue(TriggerActionType::kBaseBuilding, 0), 0);
  EXPECT_EQ(KeptValue(TriggerActionType::kBaseBuilding, 1), 1);
  EXPECT_EQ(KeptValue(TriggerActionType::kBaseBuilding, -1), 1);
  EXPECT_EQ(KeptValue(TriggerActionType::kBaseBuilding, 256), 0);
}

TEST(TriggerActionDataTest, TheWideValuesAreKeptAsTheyAre) {
  EXPECT_EQ(
      KeptValue(TriggerActionType::kPlayMusic, static_cast<int>(THEME_CRUS)),
      static_cast<int>(THEME_CRUS));
  EXPECT_EQ(KeptValue(TriggerActionType::kPreferredTarget,
                      static_cast<int>(QUARRY_BUILDINGS)),
            static_cast<int>(QUARRY_BUILDINGS));
  EXPECT_EQ(
      KeptValue(TriggerActionType::kPlaySound, static_cast<int>(VOC_GUY_OKAY)),
      static_cast<int>(VOC_GUY_OKAY));
  for (const TriggerActionType type :
       {TriggerActionType::kDropZoneFlare, TriggerActionType::kRevealSome,
        TriggerActionType::kRevealZone, TriggerActionType::kTextTrigger,
        TriggerActionType::kAddTimer, TriggerActionType::kSubTimer,
        TriggerActionType::kSetTimer, TriggerActionType::kSetGlobal,
        TriggerActionType::kClearGlobal}) {
    EXPECT_EQ(KeptValue(type, -65399), -65399);
    EXPECT_EQ(KeptValue(type, 300), 300);
  }
}

// SCU31EA and others store a sound as 0xFFFF00xx.
TEST(TriggerActionDataTest, ASoundOutOfRangeIsReadAsItsLowByte) {
  EXPECT_EQ(KeptValue(TriggerActionType::kPlaySound, -65399), 137);
  EXPECT_EQ(KeptValue(TriggerActionType::kPlaySound, 65673), 137);
  EXPECT_EQ(KeptValue(TriggerActionType::kPlaySound, 137), 137);
}

// 0xFF is no sound either, so the value is kept as it is.
TEST(TriggerActionDataTest, ASoundWhoseLowByteIsNoSoundIsKept) {
  EXPECT_EQ(KeptValue(TriggerActionType::kPlaySound, -1), -1);
}

TEST(TriggerActionDataTest, ANewActionIsNoAction) {
  const auto action = MakeTriggerAction({});
  EXPECT_EQ(action->type(), TriggerActionType::kNone);
  EXPECT_EQ(action->ToRecord(), TriggerActionRecord());
}

// The INI reader casts whatever number the scenario gives.
TEST(TriggerActionDataTest, ATypeNumberThatIsNoActionDoesNothing) {
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  const auto unknown = static_cast<TriggerActionType>(99);
  const auto action = MakeTriggerAction(Record(unknown, 5));
  EXPECT_EQ(action->type(), unknown);
  EXPECT_EQ(action->ToRecord(), Record(unknown));
  EXPECT_TRUE(action->Perform({}));
}

// A Game whose only house heap is empty: no house the actions name exists.
class TriggerActionTest : public GameFixture {
 protected:
  // Carries out the action `record` describes as a trigger of `house` that is
  // attached to nothing.
  static bool Perform(const TriggerActionRecord& record,
                      const HousesType house = HOUSE_USSR) {
    return MakeTriggerAction(record)->Perform({.house = house});
  }
};

// A trigger may belong to a house the scenario does not have.
TEST_F(TriggerActionTest, ActionsOnTheTriggersHouseSkipAMissingHouse) {
  EXPECT_TRUE(Perform(Record(TriggerActionType::kBaseBuilding, 1)));
  EXPECT_TRUE(
      Perform(Record(TriggerActionType::kOneSpecial, SPC_NUCLEAR_BOMB)));
  EXPECT_TRUE(
      Perform(Record(TriggerActionType::kFullSpecial, SPC_NUCLEAR_BOMB)));
}

TEST_F(TriggerActionTest, ActionsOnTheNamedHouseSkipAMissingHouse) {
  for (const TriggerActionType type :
       {TriggerActionType::kAllHunt, TriggerActionType::kBeginProduction,
        TriggerActionType::kFireSale, TriggerActionType::kAutocreate}) {
    EXPECT_TRUE(Perform(Record(type, HOUSE_GREECE)));
    EXPECT_TRUE(Perform(Record(type, HOUSE_NONE)));
  }
}

// The INI reader leaves the team null when the scenario names a team that
// does not exist.
TEST_F(TriggerActionTest, TeamActionsSkipAMissingTeam) {
  EXPECT_TRUE(Perform(Record(TriggerActionType::kCreateTeam)));
  EXPECT_TRUE(Perform(Record(TriggerActionType::kDestroyTeam)));
  EXPECT_FALSE(Perform(Record(TriggerActionType::kReinforcements)));
}

TEST_F(TriggerActionTest, ForceTriggerDoesNothingWhenNoTriggerCanBeMade) {
  TriggerType::DestroyAll();
  TheObjectHeaps().trigger_type().Reset(1);
  TheObjectHeaps().trigger().Reset(0);
  auto* const trigger_type = base::NewPooled<TriggerType>();
  ASSERT_NE(trigger_type, nullptr);
  EXPECT_TRUE(Perform({.type = TriggerActionType::kForceTrigger,
                       .trigger_type = trigger_type}));
}

TEST_F(TriggerActionTest, TextTriggerSkipsAnIdWithNoText) {
  EXPECT_TRUE(Perform(Record(TriggerActionType::kTextTrigger, -1)));
  EXPECT_TRUE(Perform(Record(TriggerActionType::kTextTrigger, 0)));
}

TEST_F(TriggerActionTest, TextTriggerShowsItsTutorialMessage) {
  TheSession().Messages.Init(0, 0, 6, MAX_MESSAGE_LENGTH - 14, 8, 0, 0, 0, 0,
                             0);
  TheAssets().set_tutorial_text(3, "Build a power plant.");

  EXPECT_TRUE(Perform(Record(TriggerActionType::kTextTrigger, 3)));

  ASSERT_EQ(TheSession().Messages.Num_Messages(), 1);
  EXPECT_STREQ(TheSession().Messages.Get_Message(0), "Build a power plant.");
}

TEST(TriggerActionNameTest, EveryActionHasAnEditorName) {
  EXPECT_STREQ(ActionName(TriggerActionType::kNone), "-No Action-");
  EXPECT_STREQ(ActionName(TriggerActionType::kCreateTeam), "Create Team...");
  EXPECT_STREQ(ActionName(TriggerActionType::kLaunchNukes), "Launch Nukes");
}

TEST(TriggerActionNeedTest, EachActionAsksForTheValueItUses) {
  EXPECT_EQ(ActionNeed(TriggerActionType::kOneSpecial), NEED_SPECIAL);
  EXPECT_EQ(ActionNeed(TriggerActionType::kFullSpecial), NEED_SPECIAL);
  for (const TriggerActionType type :
       {TriggerActionType::kFireSale, TriggerActionType::kWin,
        TriggerActionType::kLose, TriggerActionType::kAllHunt,
        TriggerActionType::kBeginProduction, TriggerActionType::kAutocreate}) {
    EXPECT_EQ(ActionNeed(type), NEED_HOUSE);
  }
  EXPECT_EQ(ActionNeed(TriggerActionType::kBaseBuilding), NEED_BOOL);
  for (const TriggerActionType type :
       {TriggerActionType::kCreateTeam, TriggerActionType::kDestroyTeam,
        TriggerActionType::kReinforcements}) {
    EXPECT_EQ(ActionNeed(type), NEED_TEAM);
  }
  EXPECT_EQ(ActionNeed(TriggerActionType::kForceTrigger), NEED_TRIGGER);
  EXPECT_EQ(ActionNeed(TriggerActionType::kDestroyTrigger), NEED_TRIGGER);
  for (const TriggerActionType type :
       {TriggerActionType::kDropZoneFlare, TriggerActionType::kRevealSome,
        TriggerActionType::kRevealZone}) {
    EXPECT_EQ(ActionNeed(type), NEED_WAYPOINT);
  }
  EXPECT_EQ(ActionNeed(TriggerActionType::kPlayMusic), NEED_THEME);
  EXPECT_EQ(ActionNeed(TriggerActionType::kPlayMovie), NEED_MOVIE);
  EXPECT_EQ(ActionNeed(TriggerActionType::kPlaySound), NEED_SOUND);
  EXPECT_EQ(ActionNeed(TriggerActionType::kPlaySpeech), NEED_SPEECH);
  for (const TriggerActionType type :
       {TriggerActionType::kTextTrigger, TriggerActionType::kAddTimer,
        TriggerActionType::kSubTimer, TriggerActionType::kSetTimer,
        TriggerActionType::kSetGlobal, TriggerActionType::kClearGlobal}) {
    EXPECT_EQ(ActionNeed(type), NEED_NUMBER);
  }
  EXPECT_EQ(ActionNeed(TriggerActionType::kPreferredTarget), NEED_QUARRY);
  for (const TriggerActionType type :
       {TriggerActionType::kNone, TriggerActionType::kWinLose,
        TriggerActionType::kAllowWin, TriggerActionType::kRevealAll,
        TriggerActionType::kStartTimer, TriggerActionType::kStopTimer,
        TriggerActionType::kCreepShadow, TriggerActionType::kDestroyObject,
        TriggerActionType::kLaunchNukes}) {
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
    team_type_ = base::NewPooled<TeamType>();
    team_type_->Set_Name("tanks");
    team_type_->set_house(HOUSE_USSR);
    // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
    trigger_type_ = base::NewPooled<TriggerType>();
    trigger_type_->Set_Name("alarm");
  }

  // The record `text` describes in the scenario INI's format `format`.
  static TriggerActionRecord Read(const std::string_view text,
                                  const int format) {
    TheWorld().new_ini_format() = format;
    base::Tokenizer tokens(text);
    return ReadTriggerActionRecord(tokens);
  }

  HouseClass* house_ = nullptr;
  TeamType* team_type_ = nullptr;
  TriggerType* trigger_type_ = nullptr;
};

TEST_F(TriggerActionGameTest, EveryActionIsMadeAgainFromItsRecord) {
  for (const TriggerActionType type :
       magic_enum::enum_values<TriggerActionType>()) {
    const auto action = MakeTriggerAction({.type = type,
                                           .team_type = team_type_,
                                           .trigger_type = trigger_type_,
                                           .value = 3});
    ASSERT_NE(action, nullptr);
    EXPECT_EQ(action->type(), type);
    const TriggerActionRecord record = action->ToRecord();
    EXPECT_EQ(record.type, type);
    EXPECT_EQ(MakeTriggerAction(record)->ToRecord(), record)
        << ActionName(type);
  }
}

TEST_F(TriggerActionGameTest, AnActionKeepsOnlyTheFieldsItsTypeUses) {
  for (const TriggerActionType type :
       magic_enum::enum_values<TriggerActionType>()) {
    const TriggerActionRecord record =
        MakeTriggerAction({.type = type,
                           .team_type = team_type_,
                           .trigger_type = trigger_type_,
                           .value = 3})
            ->ToRecord();
    const NeedType need = ActionNeed(type);
    EXPECT_EQ(record.team_type, need == NEED_TEAM ? team_type_ : nullptr)
        << ActionName(type);
    EXPECT_EQ(record.trigger_type,
              need == NEED_TRIGGER ? trigger_type_ : nullptr)
        << ActionName(type);
    int value = 3;
    if (need == NEED_NONE || need == NEED_TEAM || need == NEED_TRIGGER) {
      value = -1;
    } else if (need == NEED_BOOL) {
      value = 1;
    }
    EXPECT_EQ(record.value, value) << ActionName(type);
  }
}

TEST_F(TriggerActionGameTest, DetachClearsOnlyTheTypeBeingDestroyed) {
  const auto team_action = MakeTriggerAction(
      {.type = TriggerActionType::kCreateTeam, .team_type = team_type_});
  const auto trigger_action =
      MakeTriggerAction({.type = TriggerActionType::kForceTrigger,
                         .trigger_type = trigger_type_});

  // The team type and the trigger type are both in slot 0 of their heaps.
  team_action->Detach(Target{});
  team_action->Detach(trigger_type_->As_Target());
  trigger_action->Detach(Target{});
  trigger_action->Detach(team_type_->As_Target());
  EXPECT_EQ(team_action->ToRecord().team_type, team_type_);
  EXPECT_EQ(trigger_action->ToRecord().trigger_type, trigger_type_);

  team_action->Detach(team_type_->As_Target());
  EXPECT_EQ(team_action->ToRecord().team_type, nullptr);

  trigger_action->Detach(trigger_type_->As_Target());
  EXPECT_EQ(trigger_action->ToRecord().trigger_type, nullptr);
}

TEST_F(TriggerActionGameTest, DetachLeavesAnotherTeamOrTriggerTypeAlone) {
  const auto team_action = MakeTriggerAction(
      {.type = TriggerActionType::kCreateTeam, .team_type = team_type_});
  const auto trigger_action =
      MakeTriggerAction({.type = TriggerActionType::kForceTrigger,
                         .trigger_type = trigger_type_});

  team_action->Detach(base::NewPooled<TeamType>()->As_Target());
  trigger_action->Detach(base::NewPooled<TriggerType>()->As_Target());

  EXPECT_EQ(team_action->ToRecord().team_type, team_type_);
  EXPECT_EQ(trigger_action->ToRecord().trigger_type, trigger_type_);
}

TEST_F(TriggerActionGameTest, AnActionWithNoTeamOrTriggerIgnoresDetach) {
  const auto action =
      MakeTriggerAction(Record(TriggerActionType::kSetTimer, 2));
  action->Detach(team_type_->As_Target());
  action->Detach(trigger_type_->As_Target());
  EXPECT_EQ(action->ToRecord(), Record(TriggerActionType::kSetTimer, 2));
}

TEST_F(TriggerActionGameTest, TheIniEntryNumbersTheTeamAndTheTrigger) {
  EXPECT_EQ(ToIniEntry({.type = TriggerActionType::kCreateTeam,
                        .team_type = team_type_,
                        .value = 7}),
            "4,0,-1,7");
  EXPECT_EQ(ToIniEntry({.type = TriggerActionType::kForceTrigger,
                        .trigger_type = trigger_type_}),
            "22,-1,0,-1");
}

TEST_F(TriggerActionGameTest, TheCurrentFormatReadsWhatToIniEntryWrites) {
  const TriggerActionRecord record = Read("22,0,0,5", 2);

  EXPECT_EQ(record.type, TriggerActionType::kForceTrigger);
  EXPECT_EQ(record.team_type, team_type_);
  EXPECT_EQ(record.trigger_type, trigger_type_);
  EXPECT_EQ(record.value, 5);
  EXPECT_EQ(ToIniEntry(record), "22,0,0,5");
}

TEST_F(TriggerActionGameTest, TheCurrentFormatReadsNoTeamOrTriggerAsNull) {
  const TriggerActionRecord record = Read("1,-1,-1,2", 2);

  EXPECT_EQ(record.type, TriggerActionType::kWin);
  EXPECT_EQ(record.team_type, nullptr);
  EXPECT_EQ(record.trigger_type, nullptr);
  EXPECT_EQ(record.value, 2);
}

// Slot 1 of both heaps exists but holds nothing, and slot 9 does not exist.
TEST_F(TriggerActionGameTest, TheCurrentFormatReadsAnUnfilledSlotAsNull) {
  for (const std::string_view text : {"22,1,1,5", "22,9,9,5"}) {
    const TriggerActionRecord record = Read(text, 2);

    EXPECT_EQ(record.team_type, nullptr);
    EXPECT_EQ(record.trigger_type, nullptr);
    EXPECT_EQ(record.value, 5);
  }
}

TEST_F(TriggerActionGameTest, TheOldFormatsNameTheTeamAndTheTrigger) {
  for (const int format : {0, 1}) {
    const TriggerActionRecord record = Read("4,TANKS,ALARM,3", format);

    EXPECT_EQ(record.type, TriggerActionType::kCreateTeam);
    EXPECT_EQ(record.team_type, team_type_);
    EXPECT_EQ(record.trigger_type, trigger_type_);
    EXPECT_EQ(record.value, 3);
  }
}

TEST_F(TriggerActionGameTest, TheOldFormatsLeaveAMissingTeamOrTriggerNull) {
  const TriggerActionRecord record = Read("4,nobody,None,0", 0);

  EXPECT_EQ(record.type, TriggerActionType::kCreateTeam);
  EXPECT_EQ(record.team_type, nullptr);
  EXPECT_EQ(record.trigger_type, nullptr);
}

// The reader leaves a sound to MakeTriggerAction().
TEST_F(TriggerActionGameTest, TheReaderKeepsTheValueAsWritten) {
  EXPECT_EQ(Read("19,-1,-1,-65399", 2).value, -65399);
  EXPECT_EQ(Read("11,-1,-1,-65399", 2).value, -65399);
}

TEST_F(TriggerActionGameTest, NoActionSucceedsAndChangesNothing) {
  for (const TriggerActionType type :
       {TriggerActionType::kNone, TriggerActionType::kWinLose,
        TriggerActionType::kAllowWin}) {
    EXPECT_TRUE(Perform(Record(type)));
  }
  EXPECT_FALSE(house_->IsToWin);
  EXPECT_FALSE(house_->IsToLose);
}

TEST_F(TriggerActionGameTest, HouseActionsChangeTheTriggersHouse) {
  EXPECT_TRUE(
      Perform(Record(TriggerActionType::kPreferredTarget, QUARRY_HARVESTERS)));
  EXPECT_EQ(house_->PreferredTarget, QUARRY_HARVESTERS);

  EXPECT_TRUE(Perform(Record(TriggerActionType::kBaseBuilding, 1)));
  EXPECT_TRUE(house_->IsBaseBuilding);
  EXPECT_TRUE(Perform(Record(TriggerActionType::kBaseBuilding, 0)));
  EXPECT_FALSE(house_->IsBaseBuilding);
}

TEST_F(TriggerActionGameTest, PreferredTargetSkipsAMissingHouse) {
  EXPECT_TRUE(
      Perform(Record(TriggerActionType::kPreferredTarget, QUARRY_HARVESTERS),
              HOUSE_GREECE));
  EXPECT_EQ(house_->PreferredTarget, QUARRY_ANYTHING);
}

TEST_F(TriggerActionGameTest, NamedHouseActionsChangeThatHouse) {
  EXPECT_TRUE(Perform(Record(TriggerActionType::kBeginProduction, HOUSE_USSR),
                      HOUSE_GREECE));
  EXPECT_TRUE(house_->IsStarted);

  EXPECT_TRUE(
      Perform(Record(TriggerActionType::kFireSale, HOUSE_USSR), HOUSE_GREECE));
  EXPECT_EQ(house_->State, STATE_ENDGAME);

  EXPECT_TRUE(Perform(Record(TriggerActionType::kAutocreate, HOUSE_USSR),
                      HOUSE_GREECE));
  EXPECT_TRUE(house_->IsAlerted);
}

TEST_F(TriggerActionGameTest, GlobalsAreSetAndCleared) {
  EXPECT_TRUE(Perform(Record(TriggerActionType::kSetGlobal, 3)));
  EXPECT_TRUE(base::At(TheScenario().GlobalFlags, 3));

  EXPECT_TRUE(Perform(Record(TriggerActionType::kClearGlobal, 3)));
  EXPECT_FALSE(base::At(TheScenario().GlobalFlags, 3));
}

TEST_F(TriggerActionGameTest, WinAndLoseFlagThePlayerByTheNamedHouse) {
  EXPECT_TRUE(Perform(Record(TriggerActionType::kWin, HOUSE_USSR)));
  EXPECT_TRUE(house_->IsToWin);

  house_->IsToWin = false;
  EXPECT_TRUE(Perform(Record(TriggerActionType::kLose, HOUSE_GREECE)));
  EXPECT_TRUE(house_->IsToWin);
}

TEST_F(TriggerActionGameTest, TheOtherHouseWinningMeansThePlayerLoses) {
  EXPECT_TRUE(Perform(Record(TriggerActionType::kWin, HOUSE_GREECE)));
  EXPECT_TRUE(house_->IsToLose);

  house_->IsToLose = false;
  EXPECT_TRUE(Perform(Record(TriggerActionType::kLose, HOUSE_USSR)));
  EXPECT_TRUE(house_->IsToLose);
}

TEST_F(TriggerActionGameTest, DestroyTriggerDeletesEveryTriggerOfTheType) {
  auto* other_type = base::NewPooled<TriggerType>();
  base::NewPooled<TriggerClass>(trigger_type_);
  const TriggerClass* kept = base::NewPooled<TriggerClass>(other_type);
  base::NewPooled<TriggerClass>(trigger_type_);
  base::NewPooled<TriggerClass>(trigger_type_);
  ASSERT_EQ(TheObjectHeaps().trigger().Count(), 4);

  EXPECT_TRUE(Perform({.type = TriggerActionType::kDestroyTrigger,
                       .trigger_type = trigger_type_}));

  ASSERT_EQ(TheObjectHeaps().trigger().Count(), 1);
  EXPECT_EQ(TheObjectHeaps().trigger().ActiveAt(0), kept);
}

TEST_F(TriggerActionGameTest, DestroyTriggerWithNoTypeDeletesNothing) {
  base::NewPooled<TriggerClass>(trigger_type_);
  EXPECT_TRUE(Perform(Record(TriggerActionType::kDestroyTrigger)));
  EXPECT_EQ(TheObjectHeaps().trigger().Count(), 1);
}

// The forced trigger has no action of its own, so springing it only makes it.
TEST_F(TriggerActionGameTest, ForceTriggerMakesATriggerOfTheType) {
  const auto action =
      MakeTriggerAction({.type = TriggerActionType::kForceTrigger,
                         .trigger_type = trigger_type_});
  EXPECT_TRUE(action->Perform({.house = HOUSE_USSR}));
  EXPECT_TRUE(action->Perform({.house = HOUSE_USSR}));
  // A volatile trigger is deleted once it has sprung.
  EXPECT_LE(TheObjectHeaps().trigger().Count(), 1);
}

TEST_F(TriggerActionGameTest, DestroyTeamRemovesTheTeamsOfTheType) {
  base::NewPooled<TeamClass>(team_type_, house_);
  ASSERT_EQ(TheObjectHeaps().team().Count(), 1);

  EXPECT_TRUE(Perform(
      {.type = TriggerActionType::kDestroyTeam, .team_type = team_type_}));

  EXPECT_EQ(TheObjectHeaps().team().Count(), 0);
}

TEST_F(TriggerActionGameTest, CreateTeamMakesOneAsScenarioSetup) {
  EXPECT_TRUE(Perform(
      {.type = TriggerActionType::kCreateTeam, .team_type = team_type_}));

  EXPECT_EQ(TheObjectHeaps().team().Count(), 1);
  EXPECT_EQ(TheWorld().scenario_init(), 0);
}

// A team type with no members brings nothing on.
TEST_F(TriggerActionGameTest, ReinforcementsReportATeamThatCannotArrive) {
  EXPECT_FALSE(Perform(
      {.type = TriggerActionType::kReinforcements, .team_type = team_type_}));
}

TEST_F(TriggerActionGameTest, DestroyObjectFailsWithNothingToDestroy) {
  EXPECT_FALSE(Perform(Record(TriggerActionType::kDestroyObject)));
}

TEST_F(TriggerActionGameTest, DestroyObjectKillsTheObjectTheTriggerSprangOn) {
  UnitTypeClass::As_Reference(UNIT_JEEP).MaxStrength = 100;
  auto* jeep = base::NewPooled<UnitClass>(UNIT_JEEP, HOUSE_USSR);
  ASSERT_NE(jeep, nullptr);
  ASSERT_GT(jeep->Strength, 0);

  const auto action =
      MakeTriggerAction(Record(TriggerActionType::kDestroyObject));
  EXPECT_TRUE(action->Perform({.house = HOUSE_USSR, .object = jeep}));

  EXPECT_EQ(TheObjectHeaps().unit().Count(), 0);
}

TEST_F(TriggerActionGameTest, DestroyObjectIgnoresAnObjectAlreadyDead) {
  UnitTypeClass::As_Reference(UNIT_JEEP).MaxStrength = 100;
  auto* jeep = base::NewPooled<UnitClass>(UNIT_JEEP, HOUSE_USSR);
  ASSERT_NE(jeep, nullptr);
  jeep->IsActive = false;

  const auto action =
      MakeTriggerAction(Record(TriggerActionType::kDestroyObject));
  EXPECT_FALSE(action->Perform({.house = HOUSE_USSR, .object = jeep}));

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

  const auto action =
      MakeTriggerAction(Record(TriggerActionType::kDestroyObject));
  // Nothing sprang it, so the action itself fails.
  EXPECT_FALSE(action->Perform(
      {.house = HOUSE_USSR,
       .trigger_id = TheObjectHeaps().trigger().SlotIndexOf(trigger)}));

  ASSERT_EQ(TheObjectHeaps().unit().Count(), 1);
  EXPECT_EQ(TheObjectHeaps().unit().ActiveAt(0), bystander);
  EXPECT_EQ(bystander->Trigger, other);
}

TEST_F(TriggerActionGameTest, TheTimerStartsStopsAndIsSetInTenthsOfAMinute) {
  EXPECT_TRUE(Perform(Record(TriggerActionType::kSetTimer, 20)));
  EXPECT_TRUE(TheScenario().MissionTimer.IsRunning());
  EXPECT_EQ(TheScenario().MissionTimer.Remaining(), 2 * kTicksPerMinute);

  EXPECT_TRUE(Perform(Record(TriggerActionType::kStopTimer)));
  EXPECT_FALSE(TheScenario().MissionTimer.IsRunning());
  // Stopping a stopped timer changes nothing.
  EXPECT_TRUE(Perform(Record(TriggerActionType::kStopTimer)));
  EXPECT_FALSE(TheScenario().MissionTimer.IsRunning());

  EXPECT_TRUE(Perform(Record(TriggerActionType::kStartTimer)));
  EXPECT_TRUE(TheScenario().MissionTimer.IsRunning());
  EXPECT_TRUE(Perform(Record(TriggerActionType::kStartTimer)));
  EXPECT_TRUE(TheScenario().MissionTimer.IsRunning());
  EXPECT_EQ(TheScenario().MissionTimer.Remaining(), 2 * kTicksPerMinute);
}

TEST_F(TriggerActionGameTest, TheTimerIsExtendedAndShortenedDownToZero) {
  TheScenario().MissionTimer.Set(kTicksPerMinute);
  TheScenario().MissionTimer.Stop();

  EXPECT_TRUE(Perform(Record(TriggerActionType::kAddTimer, 5)));
  EXPECT_EQ(TheScenario().MissionTimer.Remaining(),
            kTicksPerMinute + (kTicksPerMinute / 2));

  EXPECT_TRUE(Perform(Record(TriggerActionType::kSubTimer, 10)));
  EXPECT_EQ(TheScenario().MissionTimer.Remaining(), kTicksPerMinute / 2);

  EXPECT_TRUE(Perform(Record(TriggerActionType::kSubTimer, 10)));
  EXPECT_EQ(TheScenario().MissionTimer.Remaining(), 0);
}

TEST_F(TriggerActionGameTest, ASpecialWeaponIsGivenOnceOrForGood) {
  // The sidebar button is the player's; give the weapon to another house.
  auto* greece = base::NewPooled<HouseClass>(HOUSE_GREECE);

  EXPECT_TRUE(Perform(Record(TriggerActionType::kOneSpecial, SPC_NUCLEAR_BOMB),
                      HOUSE_GREECE));
  EXPECT_TRUE(greece->SuperWeapon.at(SPC_NUCLEAR_BOMB).Is_Present());
  EXPECT_TRUE(greece->SuperWeapon.at(SPC_NUCLEAR_BOMB).Is_One_Time());

  EXPECT_TRUE(Perform(Record(TriggerActionType::kFullSpecial, SPC_SONAR_PULSE),
                      HOUSE_GREECE));
  EXPECT_TRUE(greece->SuperWeapon.at(SPC_SONAR_PULSE).Is_Present());
  EXPECT_FALSE(greece->SuperWeapon.at(SPC_SONAR_PULSE).Is_One_Time());
  EXPECT_FALSE(house_->SuperWeapon.at(SPC_SONAR_PULSE).Is_Present());
}

// SCG12EA's "nuke" trigger, as the scenario stores it.
TEST_F(TriggerActionGameTest, TheShippedNukeValueGivesTheNuke) {
  auto* greece = base::NewPooled<HouseClass>(HOUSE_GREECE);

  EXPECT_TRUE(
      Perform(Record(TriggerActionType::kOneSpecial, -255), HOUSE_GREECE));
  EXPECT_TRUE(greece->SuperWeapon.at(SPC_NUCLEAR_BOMB).Is_Present());
}

TEST_F(TriggerActionGameTest, AllHuntReachesTheNamedHouse) {
  EXPECT_TRUE(
      Perform(Record(TriggerActionType::kAllHunt, HOUSE_USSR), HOUSE_GREECE));
}

TEST_F(TriggerActionGameTest, LaunchNukesWithNoSilosDoesNothing) {
  EXPECT_TRUE(Perform(Record(TriggerActionType::kLaunchNukes)));
}

TEST_F(TriggerActionGameTest, RevealAllMakesThePlayerSeeTheWholeMap) {
  EXPECT_TRUE(Perform(Record(TriggerActionType::kRevealAll)));
  EXPECT_TRUE(house_->IsVisionary);
  EXPECT_TRUE(TheMap().at(Cell::FromXY(10, 10)).IsMapped);
  // Nothing more to reveal the second time.
  EXPECT_TRUE(Perform(Record(TriggerActionType::kRevealAll)));
}

TEST_F(TriggerActionGameTest, RevealActionsDoNothingForAPlayerWhoSeesAll) {
  house_->IsVisionary = true;
  base::At(TheScenario().Waypoint, 0) = Cell::FromXY(10, 10);
  for (const TriggerActionType type :
       {TriggerActionType::kRevealSome, TriggerActionType::kRevealZone,
        TriggerActionType::kRevealAll}) {
    EXPECT_TRUE(Perform(Record(type, 0)));
  }
  EXPECT_FALSE(TheMap().at(Cell::FromXY(10, 10)).IsMapped);
}

TEST_F(TriggerActionGameTest, RevealZoneMapsTheCellsOfTheWaypointsZone) {
  base::At(TheScenario().Waypoint, 0) = Cell::FromXY(10, 10);
  EXPECT_TRUE(Perform(Record(TriggerActionType::kRevealZone, 0)));
  EXPECT_TRUE(TheMap().at(Cell::FromXY(10, 10)).IsMapped);
}

TEST_F(TriggerActionGameTest, RevealSomeMapsAroundTheWaypoint) {
  base::At(TheScenario().Waypoint, 0) = Cell::FromXY(10, 10);
  TheRules().GapShroudRadius = 3;
  EXPECT_TRUE(Perform(Record(TriggerActionType::kRevealSome, 0)));
  EXPECT_TRUE(TheMap().at(Cell::FromXY(10, 10)).IsMapped);
  EXPECT_FALSE(TheMap().at(Cell::FromXY(40, 40)).IsMapped);
}

TEST_F(TriggerActionGameTest, CreepShadowRuns) {
  EXPECT_TRUE(Perform(Record(TriggerActionType::kCreepShadow)));
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
  const MissionType barracks_orders = barracks->mission_state().queued();

  EXPECT_TRUE(Perform(Record(TriggerActionType::kLaunchNukes)));

  EXPECT_EQ(silo->mission_state().queued(), MISSION_MISSILE);
  EXPECT_EQ(barracks->mission_state().queued(), barracks_orders);
}

TEST_F(TriggerActionGameTest, DestroyObjectOnACellWithNoBridgeStillSucceeds) {
  UnitTypeClass::As_Reference(UNIT_JEEP).MaxStrength = 100;
  auto* jeep = base::NewPooled<UnitClass>(UNIT_JEEP, HOUSE_USSR);

  const auto action =
      MakeTriggerAction(Record(TriggerActionType::kDestroyObject));
  EXPECT_TRUE(action->Perform(
      {.house = HOUSE_USSR, .object = jeep, .cell = Cell::FromXY(10, 10)}));

  EXPECT_EQ(TheObjectHeaps().unit().Count(), 0);
}

}  // namespace

}  // namespace ra
