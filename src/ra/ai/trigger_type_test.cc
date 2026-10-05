// Tests for the trigger type in a real Game with no game data: its heap slot,
// what it drops when a type it refers to is destroyed, how it reads and writes
// its scenario INI entry, and the summary the scenario editor lists.

#include "ra/ai/trigger_type.h"

#include <string>

#include "absl/strings/str_cat.h"
#include "base/pooled.h"
#include "engine/file/ini_file.h"
#include "gtest/gtest.h"
#include "ra/ai/teamtype.h"
#include "ra/ai/tevent.h"
#include "ra/ai/trigger_action.h"
#include "ra/core/config.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/map/world.h"
#include "ra/objects/type.h"
#include "ra/startup/game_test_util.h"

namespace ra {

namespace {

// A Game with one team type and one trigger type, each in slot 0 of its heap,
// reading the scenario INI's current format.
class TriggerTypeTest : public GameFixture {
 protected:
  void SetUp() override {
    TheObjectHeaps().team_type().Reset(4);
    TheObjectHeaps().trigger_type().Reset(4);
    team_type_ = base::NewPooled<TeamTypeClass>();
    team_type_->Set_Name("tanks");
    trigger_type_ = base::NewPooled<TriggerType>();
    trigger_type_->Set_Name("alrm");
    TheWorld().new_ini_format() = 2;
  }

  // True in the builds that have the scenario editor's descriptions.
  static bool HasDescriptions() {
    return config::kCheatKeysEnabled || config::kScenarioEditorEnabled;
  }

  // The start of every description of `type`: name, house and the three
  // marks `marks`, up to the event's name.
  static std::string DescriptionPrefix(const TriggerType& type,
                                       const std::string_view marks) {
    return absl::StrCat(type.Name(), "\t ",
                        HouseTypeClass::As_Reference(type.house).Suffix, " ",
                        marks, "  ");
  }

  TeamTypeClass* team_type_ = nullptr;
  TriggerType* trigger_type_ = nullptr;
};

TEST_F(TriggerTypeTest, ANewTriggerTypeIsVolatileWithOneEventAndOneAction) {
  EXPECT_TRUE(trigger_type_->is_active);
  EXPECT_EQ(trigger_type_->persistence, TriggerType::VOLATILE);
  EXPECT_EQ(trigger_type_->event_control, MULTI_ONLY);
  EXPECT_EQ(trigger_type_->action_control, MULTI_ONLY);
  EXPECT_EQ(trigger_type_->event1.Event, TEVENT_NONE);
  EXPECT_EQ(trigger_type_->action1.type, TriggerActionType::kNone);
  EXPECT_EQ(trigger_type_->ID, 0);
}

TEST_F(TriggerTypeTest, AFullHeapGivesNoTriggerType) {
  TheObjectHeaps().trigger_type().Reset(1);
  ASSERT_NE(base::NewPooled<TriggerType>(), nullptr);
  EXPECT_EQ(base::NewPooled<TriggerType>(), nullptr);
}

TEST_F(TriggerTypeTest, DeletingATriggerTypeFreesItsSlot) {
  base::DeletePooled(trigger_type_);

  EXPECT_EQ(TheObjectHeaps().trigger_type().Count(), 0);
  EXPECT_EQ(TriggerType::FromName("alrm"), nullptr);
}

TEST_F(TriggerTypeTest, DestroyAllEmptiesTheHeap) {
  ASSERT_NE(base::NewPooled<TriggerType>(), nullptr);

  TriggerType::DestroyAll();

  EXPECT_EQ(TheObjectHeaps().trigger_type().Count(), 0);
}

TEST_F(TriggerTypeTest, FromNameIgnoresCase) {
  EXPECT_EQ(TriggerType::FromName("alrm"), trigger_type_);
  EXPECT_EQ(TriggerType::FromName("ALRM"), trigger_type_);
}

TEST_F(TriggerTypeTest, FromNameFindsALaterTriggerType) {
  auto* const second = base::NewPooled<TriggerType>();
  second->Set_Name("wave");

  EXPECT_EQ(TriggerType::FromName("wave"), second);
}

TEST_F(TriggerTypeTest, FromNameGivesNullForAnUnknownOrEmptyName) {
  EXPECT_EQ(TriggerType::FromName("none"), nullptr);
  EXPECT_EQ(TriggerType::FromName(""), nullptr);
  // An unnamed trigger type is not found by the empty name either.
  base::NewPooled<TriggerType>()->Set_Name("");
  EXPECT_EQ(TriggerType::FromName(""), nullptr);
}

TEST_F(TriggerTypeTest, DetachClearsTheTeamTypeOfItsEventsAndActions) {
  trigger_type_->event1.Team = team_type_;
  trigger_type_->event2.Team = team_type_;
  trigger_type_->action1.team_type = team_type_;
  trigger_type_->action2.team_type = team_type_;

  trigger_type_->Detach(team_type_->As_Target());

  EXPECT_EQ(trigger_type_->event1.Team, nullptr);
  EXPECT_EQ(trigger_type_->event2.Team, nullptr);
  EXPECT_EQ(trigger_type_->action1.team_type, nullptr);
  EXPECT_EQ(trigger_type_->action2.team_type, nullptr);
}

TEST_F(TriggerTypeTest, DetachLeavesAnotherTeamTypeAlone) {
  trigger_type_->event1.Team = team_type_;
  trigger_type_->event2.Team = team_type_;

  trigger_type_->Detach(base::NewPooled<TeamTypeClass>()->As_Target());

  EXPECT_EQ(trigger_type_->event1.Team, team_type_);
  EXPECT_EQ(trigger_type_->event2.Team, team_type_);
}

TEST_F(TriggerTypeTest, DetachLeavesItsNamesakesAlone) {
  trigger_type_->event1.Team = team_type_;
  trigger_type_->action1.trigger_type = trigger_type_;

  // A trigger type in the same slot number as the team type.
  trigger_type_->Detach(trigger_type_->As_Target());

  EXPECT_EQ(trigger_type_->event1.Team, team_type_);
  EXPECT_EQ(trigger_type_->action1.trigger_type, nullptr);
}

TEST_F(TriggerTypeTest, OneEventAttachesWhereThatEventDoes) {
  trigger_type_->event1.Event = TEVENT_PLAYER_ENTERED;
  trigger_type_->event2.Event = TEVENT_TIME;
  ASSERT_NE(Attaches_To(TEVENT_PLAYER_ENTERED),
            Attaches_To(TEVENT_PLAYER_ENTERED) | Attaches_To(TEVENT_TIME));

  EXPECT_EQ(trigger_type_->AttachesTo(), Attaches_To(TEVENT_PLAYER_ENTERED));
}

TEST_F(TriggerTypeTest, TwoEventsAttachWhereEitherDoes) {
  trigger_type_->event1.Event = TEVENT_PLAYER_ENTERED;
  trigger_type_->event2.Event = TEVENT_TIME;
  for (const MultiStyle control : {MULTI_AND, MULTI_OR, MULTI_LINKED}) {
    trigger_type_->event_control = control;
    EXPECT_EQ(trigger_type_->AttachesTo(),
              Attaches_To(TEVENT_PLAYER_ENTERED) | Attaches_To(TEVENT_TIME));
  }
}

// persistence, house, event control, action control, then the two events
// (event, team, value) and the two actions (action, team, trigger, value).
constexpr std::string_view kEntry =
    "2,1,2,1,13,-1,50,23,0,0,4,0,-1,7,22,-1,0,-1";

TEST_F(TriggerTypeTest, ReadIniEntrySetsEveryField) {
  auto* const type = base::NewPooled<TriggerType>();
  type->ReadIniEntry("wave", kEntry);

  EXPECT_EQ(type->Name(), std::string_view("wave"));
  EXPECT_EQ(type->persistence, TriggerType::PERSISTANT);
  EXPECT_EQ(type->house, static_cast<HousesType>(1));
  EXPECT_EQ(type->event_control, MULTI_OR);
  EXPECT_EQ(type->action_control, MULTI_AND);
  EXPECT_EQ(type->event1.Event, TEVENT_TIME);
  EXPECT_EQ(type->event1.Team, nullptr);
  EXPECT_EQ(type->event1.Data.Value, 50);
  EXPECT_EQ(type->event2.Event, TEVENT_LEAVES_MAP);
  EXPECT_EQ(type->event2.Team, team_type_);
  EXPECT_EQ(type->action1.type, TriggerActionType::kCreateTeam);
  EXPECT_EQ(type->action1.team_type, team_type_);
  EXPECT_EQ(type->action1.trigger_type, nullptr);
  EXPECT_EQ(type->action1.data.value, 7);
  EXPECT_EQ(type->action2.type, TriggerActionType::kForceTrigger);
  EXPECT_EQ(type->action2.team_type, nullptr);
  EXPECT_EQ(type->action2.trigger_type, trigger_type_);
}

TEST_F(TriggerTypeTest, ToIniEntryWritesWhatReadIniEntryReads) {
  auto* const type = base::NewPooled<TriggerType>();
  type->ReadIniEntry("wave", kEntry);

  EXPECT_EQ(type->ToIniEntry(), kEntry);
}

TEST_F(TriggerTypeTest, MissingFieldsReadAsZero) {
  trigger_type_->persistence = TriggerType::PERSISTANT;
  trigger_type_->event_control = MULTI_LINKED;
  trigger_type_->action_control = MULTI_AND;

  trigger_type_->ReadIniEntry("bare", "");

  EXPECT_EQ(trigger_type_->persistence, TriggerType::VOLATILE);
  EXPECT_EQ(trigger_type_->house, static_cast<HousesType>(0));
  EXPECT_EQ(trigger_type_->event_control, MULTI_ONLY);
  EXPECT_EQ(trigger_type_->action_control, MULTI_ONLY);
  EXPECT_EQ(trigger_type_->event1.Event, TEVENT_NONE);
  EXPECT_EQ(trigger_type_->action1.type, TriggerActionType::kNone);
}

TEST_F(TriggerTypeTest, ReadIniCreatesATriggerTypePerEntry) {
  TriggerType::DestroyAll();
  engine::file::IniFile ini;
  ASSERT_TRUE(ini.PutString(TriggerType::kIniSection, "wave", kEntry));
  ASSERT_TRUE(ini.PutString(TriggerType::kIniSection, "dawn",
                            "0,2,0,0,13,-1,5,0,-1,0,1,-1,-1,-1,0,-1,-1,-1"));

  TriggerType::ReadIni(ini);

  ASSERT_EQ(TheObjectHeaps().trigger_type().Count(), 2);
  const TriggerType* const wave = TriggerType::FromName("wave");
  const TriggerType* const dawn = TriggerType::FromName("dawn");
  ASSERT_NE(wave, nullptr);
  ASSERT_NE(dawn, nullptr);
  EXPECT_EQ(wave->event1.Data.Value, 50);
  // The trigger in slot 0, which is the first entry: itself.
  EXPECT_EQ(wave->action2.trigger_type, wave);
  EXPECT_EQ(dawn->action1.type, TriggerActionType::kWin);
  EXPECT_EQ(dawn->event1.Data.Value, 5);
}

TEST_F(TriggerTypeTest, ReadIniDropsTheEntriesTheHeapHasNoRoomFor) {
  TheObjectHeaps().trigger_type().Reset(1);
  engine::file::IniFile ini;
  ASSERT_TRUE(ini.PutString(TriggerType::kIniSection, "wave", kEntry));
  ASSERT_TRUE(ini.PutString(TriggerType::kIniSection, "dawn", kEntry));

  TriggerType::ReadIni(ini);

  EXPECT_EQ(TheObjectHeaps().trigger_type().Count(), 1);
  EXPECT_NE(TriggerType::FromName("wave"), nullptr);
  EXPECT_EQ(TriggerType::FromName("dawn"), nullptr);
}

TEST_F(TriggerTypeTest, ReadIniOfAnEmptySectionCreatesNothing) {
  TriggerType::DestroyAll();
  const engine::file::IniFile ini;

  TriggerType::ReadIni(ini);

  EXPECT_EQ(TheObjectHeaps().trigger_type().Count(), 0);
}

// Before format 2 an action names its trigger type, which may come later in
// the section.
TEST_F(TriggerTypeTest, ReadIniResolvesTriggerNamesInTheOldFormats) {
  TriggerType::DestroyAll();
  TheWorld().new_ini_format() = 1;
  engine::file::IniFile ini;
  ASSERT_TRUE(
      ini.PutString(TriggerType::kIniSection, "frst",
                    "0,1,0,1,13,50,0,0,22,None,last,-1,22,None,gone,3"));
  ASSERT_TRUE(ini.PutString(TriggerType::kIniSection, "last",
                            "0,1,0,0,13,60,0,0,1,None,None,-1,0,None,None,-1"));

  TriggerType::ReadIni(ini);

  const TriggerType* const first = TriggerType::FromName("frst");
  const TriggerType* const last = TriggerType::FromName("last");
  ASSERT_NE(first, nullptr);
  ASSERT_NE(last, nullptr);
  EXPECT_EQ(first->action1.trigger_type, last);
  EXPECT_TRUE(first->action1.pending_trigger_name.empty());
  // A name no trigger type has resolves to none.
  EXPECT_EQ(first->action2.trigger_type, nullptr);
  EXPECT_TRUE(first->action2.pending_trigger_name.empty());
  EXPECT_EQ(last->action1.trigger_type, nullptr);
}

TEST_F(TriggerTypeTest, WriteIniReplacesTheSectionAndClearsTheOldOne) {
  trigger_type_->ReadIniEntry("alrm", kEntry);
  engine::file::IniFile ini;
  ASSERT_TRUE(ini.PutString("Triggers", "old", "1"));
  ASSERT_TRUE(ini.PutString(TriggerType::kIniSection, "stale", "1"));

  TriggerType::WriteIni(ini);

  EXPECT_EQ(ini.EntryCount("Triggers"), 0);
  ASSERT_EQ(ini.EntryCount(TriggerType::kIniSection), 1);
  EXPECT_EQ(ini.GetString(TriggerType::kIniSection, "alrm", ""), kEntry);
}

TEST_F(TriggerTypeTest, WriteIniWithNoTriggerTypesLeavesTheSectionEmpty) {
  TriggerType::DestroyAll();
  engine::file::IniFile ini;
  ASSERT_TRUE(ini.PutString(TriggerType::kIniSection, "stale", "1"));

  TriggerType::WriteIni(ini);

  EXPECT_EQ(ini.EntryCount(TriggerType::kIniSection), 0);
}

TEST_F(TriggerTypeTest, TheDescriptionIsEmptyWithoutTheEditor) {
  if (HasDescriptions()) {
    GTEST_SKIP() << "this build has the editor's descriptions";
  }
  EXPECT_TRUE(trigger_type_->Description().empty());
}

TEST_F(TriggerTypeTest, TheDescriptionNamesTheTriggerItsHouseAndItsEvent) {
  if (!HasDescriptions()) {
    GTEST_SKIP() << "this build has no editor descriptions";
  }
  EXPECT_EQ(trigger_type_->Description(),
            DescriptionPrefix(*trigger_type_, "V..") + "-No Event-");
}

TEST_F(TriggerTypeTest, TheDescriptionShowsOnlyFourLettersOfTheName) {
  if (!HasDescriptions()) {
    GTEST_SKIP() << "this build has no editor descriptions";
  }
  trigger_type_->Set_Name("alarms");
  EXPECT_TRUE(trigger_type_->Description().starts_with("alar\t "));
}

TEST_F(TriggerTypeTest, TheDescriptionMarksPersistence) {
  if (!HasDescriptions()) {
    GTEST_SKIP() << "this build has no editor descriptions";
  }
  trigger_type_->persistence = TriggerType::SEMIPERSISTANT;
  EXPECT_EQ(trigger_type_->Description(),
            DescriptionPrefix(*trigger_type_, "S..") + "-No Event-");
  trigger_type_->persistence = TriggerType::PERSISTANT;
  EXPECT_EQ(trigger_type_->Description(),
            DescriptionPrefix(*trigger_type_, "P..") + "-No Event-");
}

TEST_F(TriggerTypeTest, TheDescriptionMarksTheSecondEventAndAction) {
  if (!HasDescriptions()) {
    GTEST_SKIP() << "this build has no editor descriptions";
  }
  trigger_type_->event_control = MULTI_AND;
  EXPECT_EQ(trigger_type_->Description(),
            DescriptionPrefix(*trigger_type_, "V&.") + "-No Event-");
  trigger_type_->event_control = MULTI_OR;
  EXPECT_EQ(trigger_type_->Description(),
            DescriptionPrefix(*trigger_type_, "V|.") + "-No Event-");
  trigger_type_->event_control = MULTI_LINKED;
  EXPECT_EQ(trigger_type_->Description(),
            DescriptionPrefix(*trigger_type_, "V=.") + "-No Event-");
  trigger_type_->action_control = MULTI_AND;
  EXPECT_EQ(trigger_type_->Description(),
            DescriptionPrefix(*trigger_type_, "V=&") + "-No Event-");
}

TEST_F(TriggerTypeTest, TheDescriptionAddsTheNumberAnEventTakes) {
  if (!HasDescriptions()) {
    GTEST_SKIP() << "this build has no editor descriptions";
  }
  trigger_type_->event1.Event = TEVENT_CREDITS;
  trigger_type_->event1.Data.Value = 50;
  EXPECT_EQ(
      trigger_type_->Description(),
      DescriptionPrefix(*trigger_type_, "V..") + "Credits exceed (x100)...50");
}

}  // namespace

}  // namespace ra
