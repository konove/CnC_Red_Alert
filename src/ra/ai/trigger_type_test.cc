// Tests for the trigger type in a real Game with no game data: its heap slot,
// what it drops when a type it refers to is destroyed, how it reads and writes
// its scenario INI entry, how it survives a saved game, the summary the
// scenario editor lists and how that summary is drawn as a list entry.

#include "ra/ai/trigger_type.h"

#include <array>
#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include "absl/strings/str_cat.h"
#include "base/pooled.h"
#include "engine/file/disk_file.h"
#include "engine/file/file_access.h"
#include "engine/file/ini_file.h"
#include "engine/gfx/palette_index.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/platform/env.h"
#include "engine/stream/archive.h"
#include "engine/stream/span_source.h"
#include "engine/stream/vector_sink.h"
#include "game/ui/standard_colors.h"
#include "gtest/gtest.h"
#include "ra/ai/team_type.h"
#include "ra/ai/tevent.h"
#include "ra/ai/trigger_action.h"
#include "ra/assets/palettes.h"
#include "ra/core/config.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/map/world.h"
#include "ra/objects/type.h"
#include "ra/startup/game_test_util.h"
#include "ra/startup/ui_test_util.h"
#include "ra/ui/color_scheme.h"

namespace ra {

namespace {

// A Game with one team type and one trigger type, each in slot 0 of its heap,
// reading the scenario INI's current format.
class TriggerTypeTest : public GameFixture {
 protected:
  void SetUp() override {
    TheObjectHeaps().team_type().Reset(4);
    TheObjectHeaps().trigger_type().Reset(4);
    team_type_ = base::NewPooled<TeamType>();
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

  TeamType* team_type_ = nullptr;
  TriggerType* trigger_type_ = nullptr;
};

TEST_F(TriggerTypeTest, ANewTriggerTypeIsVolatileWithOneEventAndOneAction) {
  EXPECT_TRUE(trigger_type_->is_active);
  EXPECT_EQ(trigger_type_->persistence, TriggerType::VOLATILE);
  EXPECT_EQ(trigger_type_->event_control, MULTI_ONLY);
  EXPECT_EQ(trigger_type_->action_control, MULTI_ONLY);
  EXPECT_EQ(trigger_type_->event1.type(), TriggerEventType::kNone);
  EXPECT_EQ(trigger_type_->action1->type(), TriggerActionType::kNone);
  EXPECT_EQ(trigger_type_->ID, 0);
}

TEST_F(TriggerTypeTest, AFullHeapGivesNoTriggerType) {
  TriggerType::DestroyAll();
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
  trigger_type_->event1.set_team_type(team_type_);
  trigger_type_->event2.set_team_type(team_type_);
  trigger_type_->action1 = MakeTriggerAction(
      {.type = TriggerActionType::kCreateTeam, .team_type = team_type_});
  trigger_type_->action2 = MakeTriggerAction(
      {.type = TriggerActionType::kCreateTeam, .team_type = team_type_});

  trigger_type_->Detach(team_type_->As_Target());

  EXPECT_EQ(trigger_type_->event1.team_type(), nullptr);
  EXPECT_EQ(trigger_type_->event2.team_type(), nullptr);
  EXPECT_EQ(trigger_type_->action1->ToRecord().team_type, nullptr);
  EXPECT_EQ(trigger_type_->action2->ToRecord().team_type, nullptr);
}

TEST_F(TriggerTypeTest, DetachLeavesAnotherTeamTypeAlone) {
  trigger_type_->event1.set_team_type(team_type_);
  trigger_type_->event2.set_team_type(team_type_);

  trigger_type_->Detach(base::NewPooled<TeamType>()->As_Target());

  EXPECT_EQ(trigger_type_->event1.team_type(), team_type_);
  EXPECT_EQ(trigger_type_->event2.team_type(), team_type_);
}

TEST_F(TriggerTypeTest, DetachLeavesItsNamesakesAlone) {
  trigger_type_->event1.set_team_type(team_type_);
  trigger_type_->action1 =
      MakeTriggerAction({.type = TriggerActionType::kForceTrigger,
                         .trigger_type = trigger_type_});

  // A trigger type in the same slot number as the team type.
  trigger_type_->Detach(trigger_type_->As_Target());

  EXPECT_EQ(trigger_type_->event1.team_type(), team_type_);
  EXPECT_EQ(trigger_type_->action1->ToRecord().trigger_type, nullptr);
}

TEST_F(TriggerTypeTest, OneEventAttachesWhereThatEventDoes) {
  trigger_type_->event1.set_type(TriggerEventType::kPlayerEntered);
  trigger_type_->event2.set_type(TriggerEventType::kTime);
  ASSERT_NE(EventAttachment(TriggerEventType::kPlayerEntered),
            EventAttachment(TriggerEventType::kPlayerEntered) |
                EventAttachment(TriggerEventType::kTime));

  EXPECT_EQ(trigger_type_->AttachesTo(),
            EventAttachment(TriggerEventType::kPlayerEntered));
}

TEST_F(TriggerTypeTest, TwoEventsAttachWhereEitherDoes) {
  trigger_type_->event1.set_type(TriggerEventType::kPlayerEntered);
  trigger_type_->event2.set_type(TriggerEventType::kTime);
  for (const MultiStyle control : {MULTI_AND, MULTI_OR, MULTI_LINKED}) {
    trigger_type_->event_control = control;
    EXPECT_EQ(trigger_type_->AttachesTo(),
              EventAttachment(TriggerEventType::kPlayerEntered) |
                  EventAttachment(TriggerEventType::kTime));
  }
}

// persistence, house, event control, action control, then the two events
// (event, team, value) and the two actions (action, team, trigger, value).
constexpr std::string_view kEntry =
    "2,1,2,1,13,-1,50,23,0,0,4,0,-1,-1,22,-1,0,-1";

TEST_F(TriggerTypeTest, ReadIniEntrySetsEveryField) {
  auto* const type = base::NewPooled<TriggerType>();
  type->ReadIniEntry("wave", kEntry);

  EXPECT_EQ(type->Name(), std::string_view("wave"));
  EXPECT_EQ(type->persistence, TriggerType::PERSISTANT);
  EXPECT_EQ(type->house, static_cast<HousesType>(1));
  EXPECT_EQ(type->event_control, MULTI_OR);
  EXPECT_EQ(type->action_control, MULTI_AND);
  EXPECT_EQ(type->event1.type(), TriggerEventType::kTime);
  EXPECT_EQ(type->event1.team_type(), nullptr);
  EXPECT_EQ(type->event1.value(), 50);
  EXPECT_EQ(type->event2.type(), TriggerEventType::kLeavesMap);
  EXPECT_EQ(type->event2.team_type(), team_type_);
  EXPECT_EQ(type->action1->type(), TriggerActionType::kCreateTeam);
  EXPECT_EQ(type->action1->ToRecord().team_type, team_type_);
  EXPECT_EQ(type->action1->ToRecord().trigger_type, nullptr);
  // A team action takes no value.
  EXPECT_EQ(type->action1->ToRecord().value, -1);
  EXPECT_EQ(type->action2->type(), TriggerActionType::kForceTrigger);
  EXPECT_EQ(type->action2->ToRecord().team_type, nullptr);
  EXPECT_EQ(type->action2->ToRecord().trigger_type, trigger_type_);
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
  EXPECT_EQ(trigger_type_->event1.type(), TriggerEventType::kNone);
  EXPECT_EQ(trigger_type_->action1->type(), TriggerActionType::kNone);
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
  EXPECT_EQ(wave->event1.value(), 50);
  // The trigger in slot 0, which is the first entry: itself.
  EXPECT_EQ(wave->action2->ToRecord().trigger_type, wave);
  EXPECT_EQ(dawn->action1->type(), TriggerActionType::kWin);
  EXPECT_EQ(dawn->event1.value(), 5);
}

// From format 2 an action gives its trigger type's heap slot, which may be a
// later entry's.
TEST_F(TriggerTypeTest, ReadIniResolvesALaterSlotInTheCurrentFormat) {
  TriggerType::DestroyAll();
  engine::file::IniFile ini;
  ASSERT_TRUE(ini.PutString(TriggerType::kIniSection, "frst",
                            "0,1,0,1,13,-1,50,0,-1,0,22,-1,1,-1,22,-1,2,-1"));
  ASSERT_TRUE(ini.PutString(TriggerType::kIniSection, "last",
                            "0,1,0,0,13,-1,60,0,-1,0,1,-1,-1,1,0,-1,-1,-1"));

  TriggerType::ReadIni(ini);

  const TriggerType* const first = TriggerType::FromName("frst");
  ASSERT_NE(first, nullptr);
  EXPECT_EQ(first->action1->ToRecord().trigger_type,
            TriggerType::FromName("last"));
  // Slot 2 exists but the scenario never filled it.
  EXPECT_EQ(first->action2->ToRecord().trigger_type, nullptr);
}

TEST_F(TriggerTypeTest, ReadIniDropsTheEntriesTheHeapHasNoRoomFor) {
  TriggerType::DestroyAll();
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
  EXPECT_EQ(first->action1->ToRecord().trigger_type, last);
  // A name no trigger type has resolves to none.
  EXPECT_EQ(first->action2->ToRecord().trigger_type, nullptr);
  EXPECT_EQ(last->action1->ToRecord().trigger_type, nullptr);
}

// The two action records of every trigger type, in heap order.
std::vector<std::array<TriggerActionRecord, 2>> ActionRecords() {
  std::vector<std::array<TriggerActionRecord, 2>> records;
  for (int index = 0; index < TheObjectHeaps().trigger_type().Count();
       index++) {
    const TriggerType* type = TheObjectHeaps().trigger_type().ActiveAt(index);
    records.push_back({type->action1->ToRecord(), type->action2->ToRecord()});
  }
  return records;
}

// The shipped scenarios hold values the actions rewrite (-255 for the nuke,
// 0xFFFF00xx sounds); what WriteIni() writes of them must read back as the
// same actions. The tree does not carry the scenarios: set RA_SCENARIO_DIR to
// a directory of their INI files, which mixdump extracts from the game's MIX
// archives.
TEST_F(TriggerTypeTest, TheShippedTriggersAreReadBackAsWritten) {
  const auto directory = engine::platform::GetEnv("RA_SCENARIO_DIR");
  if (!directory) {
    GTEST_SKIP() << "RA_SCENARIO_DIR is not set";
  }
  // Room for the largest scenario, and a team type in every slot an action
  // may name.
  TriggerType::DestroyAll();
  TheObjectHeaps().trigger_type().Reset(128);
  constexpr int kTeamTypes = 64;
  TheObjectHeaps().team_type().Reset(kTeamTypes);
  for (int index = 0; index < kTeamTypes; index++) {
    base::NewPooled<TeamType>();
  }

  int trigger_count = 0;
  for (const auto& file : std::filesystem::directory_iterator(*directory)) {
    SCOPED_TRACE(file.path().string());
    engine::file::IniFile ini;
    const auto stream = OpenDiskFile(file.path().string(), FileAccess::kRead);
    ASSERT_NE(stream, nullptr);
    ASSERT_TRUE(ini.Load(*stream));

    TriggerType::DestroyAll();
    TheWorld().new_ini_format() = ini.GetInt("Basic", "NewINIFormat", 0);
    TriggerType::ReadIni(ini);
    const auto read = ActionRecords();
    trigger_count += static_cast<int>(read.size());

    engine::file::IniFile written;
    TriggerType::WriteIni(written);
    TriggerType::DestroyAll();
    TheWorld().new_ini_format() = 3;
    TriggerType::ReadIni(written);
    EXPECT_EQ(ActionRecords(), read);
  }
  EXPECT_GT(trigger_count, 0);
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
  trigger_type_->event1.set_type(TriggerEventType::kCredits);
  trigger_type_->event1.set_value(50);
  EXPECT_EQ(
      trigger_type_->Description(),
      DescriptionPrefix(*trigger_type_, "V..") + "Credits exceed (x100)...50");
}

// The events that are about a kind of object show its name, which comes from
// the text table under the number the type keeps.
class TriggerTypeObjectNameTest : public TriggerTypeTest {
 protected:
  void SetUp() override {
    TriggerTypeTest::SetUp();
    InitAircraftTypes();
    InitBuildingTypes();
    InitInfantryTypes();
  }

  // What Description() puts after the prefix: the event's name and `value`.
  std::string DescriptionEnding() const {
    const std::string description = trigger_type_->Description();
    const std::string prefix = DescriptionPrefix(*trigger_type_, "V..");
    EXPECT_TRUE(description.starts_with(prefix)) << description;
    return description.substr(prefix.size());
  }
};

TEST_F(TriggerTypeObjectNameTest, TheDescriptionNamesTheUnitAnEventTakes) {
  if (!HasDescriptions()) {
    GTEST_SKIP() << "this build has no editor descriptions";
  }
  SetText(UnitTypeClass::As_Reference(UNIT_JEEP).Full_Name(), "Ranger");
  trigger_type_->event1.set_type(TriggerEventType::kBuildUnit);
  trigger_type_->event1.set_unit(UNIT_JEEP);

  EXPECT_TRUE(DescriptionEnding().ends_with("Ranger")) << DescriptionEnding();
}

TEST_F(TriggerTypeObjectNameTest, TheDescriptionNamesTheAircraftAnEventTakes) {
  if (!HasDescriptions()) {
    GTEST_SKIP() << "this build has no editor descriptions";
  }
  SetText(AircraftTypeClass::As_Reference(AIRCRAFT_HIND).Full_Name(), "Hind");
  trigger_type_->event1.set_type(TriggerEventType::kBuildAircraft);
  trigger_type_->event1.set_aircraft(AIRCRAFT_HIND);

  EXPECT_TRUE(DescriptionEnding().ends_with("Hind")) << DescriptionEnding();
}

TEST_F(TriggerTypeObjectNameTest, TheDescriptionNamesTheBuildingAnEventTakes) {
  if (!HasDescriptions()) {
    GTEST_SKIP() << "this build has no editor descriptions";
  }
  SetText(BuildingTypeClass::As_Reference(STRUCT_POWER).Full_Name(),
          "Power Plant");
  trigger_type_->event1.set_type(TriggerEventType::kBuildBuilding);
  trigger_type_->event1.set_structure(STRUCT_POWER);

  EXPECT_TRUE(DescriptionEnding().ends_with("Power Plant"))
      << DescriptionEnding();
}

TEST_F(TriggerTypeObjectNameTest, TheDescriptionNamesTheInfantryAnEventTakes) {
  if (!HasDescriptions()) {
    GTEST_SKIP() << "this build has no editor descriptions";
  }
  SetText(InfantryTypeClass::As_Reference(INFANTRY_E1).Full_Name(),
          "Rifle Infantry");
  trigger_type_->event1.set_type(TriggerEventType::kBuildInfantry);
  trigger_type_->event1.set_infantry(INFANTRY_E1);

  EXPECT_TRUE(DescriptionEnding().ends_with("Rifle Infantry"))
      << DescriptionEnding();
}

// A saved game holds the trigger type heap as each type's slot and contents;
// loading constructs each type in its old slot.
TEST_F(TriggerTypeTest, ASavedHeapLoadsEveryTriggerTypeIntoItsOldSlot) {
  // Slots 0 and 2 are in use and slot 1 is not, so a load that filled the
  // slots in order would put "wave" where nothing points.
  auto* const gap = base::NewPooled<TriggerType>();
  auto* const second = base::NewPooled<TriggerType>();
  second->Set_Name("wave");
  base::DeletePooled(gap);
  trigger_type_->house = HOUSE_USSR;
  trigger_type_->persistence = TriggerType::PERSISTANT;
  trigger_type_->event_control = MULTI_AND;
  trigger_type_->action_control = MULTI_AND;
  trigger_type_->event1.set_type(TriggerEventType::kCredits);
  trigger_type_->event1.set_value(50);
  trigger_type_->event2.set_type(TriggerEventType::kLeavesMap);
  trigger_type_->event2.set_team_type(team_type_);
  trigger_type_->action1 = MakeTriggerAction(
      {.type = TriggerActionType::kCreateTeam, .team_type = team_type_});
  trigger_type_->action2 = MakeTriggerAction(
      {.type = TriggerActionType::kForceTrigger, .trigger_type = second});
  const std::string entry = trigger_type_->ToIniEntry();
  std::vector<std::byte> saved;
  {
    VectorSink sink(saved);
    ArchiveWriter writer(sink);
    ASSERT_TRUE(TheObjectHeaps().trigger_type().Save(writer));
  }
  TriggerType::DestroyAll();

  SpanSource source{std::span<const std::byte>(saved)};
  ArchiveReader reader(source);
  ASSERT_TRUE(TheObjectHeaps().trigger_type().Load(reader)) << reader.error();

  ASSERT_EQ(TheObjectHeaps().trigger_type().Count(), 2);
  TriggerType* const loaded = TriggerType::FromName("alrm");
  ASSERT_EQ(loaded, trigger_type_);
  EXPECT_EQ(TriggerType::FromName("wave"), second);
  EXPECT_TRUE(loaded->is_active);
  EXPECT_EQ(loaded->ID, 0);
  EXPECT_EQ(second->ID, 2);
  EXPECT_EQ(loaded->ToIniEntry(), entry);
  EXPECT_EQ(loaded->event2.team_type(), team_type_);
  EXPECT_EQ(loaded->action2->ToRecord().trigger_type, second);
}

// Draws a trigger type as a list entry on a page of its own. Each colour
// scheme of UiFixture draws in one colour, so the page tells which scheme
// drew.
class TriggerTypeDrawTest : public UiFixture {
 protected:
  static constexpr int kWidth = 600;
  static constexpr int kHeight = 16;

  void SetUp() override {
    TheObjectHeaps().trigger_type().Reset(1);
    trigger_type_ = base::NewPooled<TriggerType>();
    trigger_type_->Set_Name("alrm");
    previous_scheme_ = Get_Color_Scheme();
    Set_Color_Scheme(&ThePalettes().color_remaps().at(PCOLOR_RED));
  }

  void TearDown() override {
    Set_Color_Scheme(previous_scheme_);
    UiFixture::TearDown();
  }

  // Draws the entry at the page's top left corner.
  void Draw(const bool selected, const FontType font,
            const TextPrintType flags = TPF_NONE) {
    PixelView& view = page_.view();
    trigger_type_->Draw_It(view, 0, 0, 0, kWidth, kHeight, selected, font,
                           flags);
  }

  // The number of the page's pixels that are `color`.
  int CountOf(const engine::gfx::PaletteIndex color) {
    PixelView& view = page_.view();
    int count = 0;
    for (int y = 0; y < kHeight; ++y) {
      for (int x = 0; x < kWidth; ++x) {
        count += view.GetPixel(x, y) == color ? 1 : 0;
      }
    }
    return count;
  }

  // The colour of the scheme the dialogs are set to draw in.
  static constexpr engine::gfx::PaletteIndex DialogColor() {
    return SchemeColor(PCOLOR_RED);
  }

  static bool HasDescriptions() {
    return config::kCheatKeysEnabled || config::kScenarioEditorEnabled;
  }

  PixelBuffer page_{kWidth, kHeight};
  TriggerType* trigger_type_ = nullptr;
  // The scheme to put back, which outlives this test's Game.
  RemapControlType* previous_scheme_ = nullptr;
};

TEST_F(TriggerTypeDrawTest, ASelectedEntryInAGradientFontIsABarOfTheScheme) {
  if (!HasDescriptions()) {
    GTEST_SKIP() << "this build has no editor descriptions";
  }
  Draw(true, FontType::k6PointGradient);

  // The bar is the whole box; the text on it is the scheme's colour too.
  EXPECT_EQ(CountOf(DialogColor()), kWidth * kHeight);
}

TEST_F(TriggerTypeDrawTest, AnUnselectedEntryInAGradientFontHasNoBar) {
  if (!HasDescriptions()) {
    GTEST_SKIP() << "this build has no editor descriptions";
  }
  Draw(false, FontType::k6PointGradient);

  const int text = CountOf(DialogColor());
  EXPECT_GT(text, 0);
  EXPECT_LT(text, kWidth * kHeight);
  EXPECT_EQ(CountOf(game::kTransparentBlack), (kWidth * kHeight) - text);
}

TEST_F(TriggerTypeDrawTest, TheEditorFontIsDrawnLikeAGradientFont) {
  if (!HasDescriptions()) {
    GTEST_SKIP() << "this build has no editor descriptions";
  }
  Draw(true, FontType::kEditor);

  EXPECT_EQ(CountOf(DialogColor()), kWidth * kHeight);
}

// The editor's lists all use the editor font; any other is drawn the same.
TEST_F(TriggerTypeDrawTest, ASelectedEntryInAPlainFontIsABarOfTheSchemeToo) {
  if (!HasDescriptions()) {
    GTEST_SKIP() << "this build has no editor descriptions";
  }
  Draw(true, FontType::k6Point);

  EXPECT_EQ(CountOf(DialogColor()), kWidth * kHeight);
}

TEST_F(TriggerTypeDrawTest, NothingIsDrawnWithoutTheEditor) {
  if (HasDescriptions()) {
    GTEST_SKIP() << "this build has the editor's descriptions";
  }
  Draw(true, FontType::k6PointGradient);

  EXPECT_EQ(CountOf(game::kTransparentBlack), kWidth * kHeight);
}

}  // namespace

}  // namespace ra
