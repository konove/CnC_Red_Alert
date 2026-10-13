// Tests for the team types in a real Game with no game data: how one reads
// and writes its scenario INI entry, which one the computer is offered, the
// teams it creates and destroys, how it survives a saved game, and the text
// the scenario editor lists for it and for a line of its script.

#include "ra/ai/team_type.h"

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/strings/str_cat.h"
#include "base/array.h"
#include "base/pooled.h"
#include "engine/file/ini_file.h"
#include "engine/gfx/palette_index.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/stream/archive.h"
#include "engine/stream/span_source.h"
#include "engine/stream/vector_sink.h"
#include "game/ui/standard_colors.h"
#include "gtest/gtest.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/scenario.h"
#include "ra/ai/trigger_action.h"
#include "ra/ai/trigger_type.h"
#include "ra/assets/palettes.h"
#include "ra/core/config.h"
#include "ra/core/const.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/map/world.h"
#include "ra/objects/house.h"
#include "ra/objects/mission.h"
#include "ra/objects/type.h"
#include "ra/startup/ui_test_util.h"
#include "ra/ui/color_scheme.h"

namespace ra {
namespace {

constexpr const char* kIniSection = "TeamTypes";

// True in the builds that have the scenario editor's descriptions.
bool HasDescriptions() {
  return config::kCheatKeysEnabled || config::kScenarioEditorEnabled;
}

// A Game with every type a team can have as a member, the Soviet house,
// reading the current scenario format, and one team type to work on.
class TeamTypeTest : public UiFixture {
 protected:
  void SetUp() override {
    InitAircraftTypes();
    InitInfantryTypes();
    InitVesselTypes();
    TheObjectHeaps().team().Reset(4);
    TheObjectHeaps().team_type().Reset(4);
    TheObjectHeaps().trigger_type().Reset(4);
    TheWorld().new_ini_format() = 3;
    house_ = base::NewPooled<HouseClass>(HOUSE_USSR);
    team_type_ = base::NewPooled<TeamType>();
  }

  // The INI name of a type each kind of member can be.
  static std::string Infantry() {
    return InfantryTypeClass::As_Reference(INFANTRY_E1).IniName;
  }
  static std::string Unit() {
    return UnitTypeClass::As_Reference(UNIT_JEEP).IniName;
  }
  static std::string Aircraft() {
    return AircraftTypeClass::As_Reference(AIRCRAFT_HIND).IniName;
  }
  static std::string Vessel() {
    return VesselTypeClass::As_Reference(VESSEL_SS).IniName;
  }

  // An entry in the current format for a Soviet team type of three riflemen
  // with no script, with the given option bits and the most teams allowed.
  static std::string SovietEntry(const int flags, const int max_teams) {
    return absl::StrCat(static_cast<int>(HOUSE_USSR), ",", flags, ",7,0,",
                        max_teams, ",-1,-1,1,", Infantry(), ":3,0");
  }

  HouseClass* house_ = nullptr;
  TeamType* team_type_ = nullptr;
};

TEST_F(TeamTypeTest, ANewTeamTypeHasNoHouseNoOriginAndNothingInIt) {
  EXPECT_TRUE(team_type_->is_active());
  EXPECT_EQ(team_type_->house(), HOUSE_NONE);
  EXPECT_EQ(team_type_->origin(), -1);
  EXPECT_EQ(team_type_->trigger(), nullptr);
  EXPECT_EQ(team_type_->member_count(), 0);
  EXPECT_EQ(team_type_->mission_count(), 0);
  EXPECT_EQ(team_type_->CountTeams(), 0);
  EXPECT_EQ(team_type_->recruit_priority(), 7);
  EXPECT_TRUE(team_type_->is_prebuilt());
  EXPECT_TRUE(team_type_->is_reinforceable());
}

TEST_F(TeamTypeTest, PrebuiltAndReinforceableCanBeTurnedOff) {
  team_type_->set_is_prebuilt(false);
  team_type_->set_is_reinforceable(false);

  EXPECT_FALSE(team_type_->is_prebuilt());
  EXPECT_FALSE(team_type_->is_reinforceable());
}

TEST_F(TeamTypeTest, ReadIniEntrySetsEveryField) {
  // Options 21: roundabout, autocreate and reinforceable. Trigger slot 1.
  team_type_->ReadIniEntry(
      "tanks", absl::StrCat("2,21,9,1,3,5,1,2,", Infantry(), ":3,", Unit(),
                            ":1,2,3:5,0:2"));

  EXPECT_STREQ(team_type_->IniName, "tanks");
  EXPECT_EQ(team_type_->house(), HousesType{2});
  EXPECT_TRUE(team_type_->is_roundabout());
  EXPECT_FALSE(team_type_->is_suicide());
  EXPECT_TRUE(team_type_->is_autocreate());
  EXPECT_FALSE(team_type_->is_prebuilt());
  EXPECT_TRUE(team_type_->is_reinforceable());
  EXPECT_EQ(team_type_->recruit_priority(), 9);
  EXPECT_EQ(team_type_->max_teams(), 3);
  EXPECT_EQ(team_type_->origin(), 5);
  EXPECT_EQ(team_type_->trigger(), TheObjectHeaps().trigger_type().Slot(1));
  ASSERT_EQ(team_type_->member_count(), 2);
  EXPECT_EQ(base::At(team_type_->members(), 0).type,
            &InfantryTypeClass::As_Reference(INFANTRY_E1));
  EXPECT_EQ(base::At(team_type_->members(), 0).quantity, 3);
  EXPECT_EQ(base::At(team_type_->members(), 1).type,
            &UnitTypeClass::As_Reference(UNIT_JEEP));
  EXPECT_EQ(base::At(team_type_->members(), 1).quantity, 1);
  ASSERT_EQ(team_type_->mission_count(), 2);
  EXPECT_EQ(base::At(team_type_->missions(), 0).mission(),
            TeamMissionType::kMove);
  EXPECT_EQ(base::At(team_type_->missions(), 0).value(), 5);
  EXPECT_EQ(base::At(team_type_->missions(), 1).mission(),
            TeamMissionType::kAttack);
  EXPECT_EQ(base::At(team_type_->missions(), 1).quarry(), QuarryType{2});
}

TEST_F(TeamTypeTest, ACountAboveWhatATeamTypeStoresIsTheMost) {
  // Initial number 999, maximum 256.
  team_type_->ReadIniEntry("tanks", "2,0,7,999,256,-1,-1,0,0");

  EXPECT_EQ(team_type_->max_teams(), 255);
  EXPECT_EQ(team_type_->ToIniEntry(), "2,0,7,255,255,-1,-1,0,0");
}

TEST_F(TeamTypeTest, ANegativeCountIsTheMost) {
  team_type_->ReadIniEntry("tanks", "2,0,7,-1,-1,-1,-1,0,0");

  EXPECT_EQ(team_type_->max_teams(), 255);
  EXPECT_EQ(team_type_->ToIniEntry(), "2,0,7,255,255,-1,-1,0,0");
}

TEST_F(TeamTypeTest, TheOtherOptionBitsAreSuicideAndPrebuilt) {
  team_type_->ReadIniEntry("tanks", "2,10,7,0,1,-1,-1,0,0");

  EXPECT_FALSE(team_type_->is_roundabout());
  EXPECT_TRUE(team_type_->is_suicide());
  EXPECT_FALSE(team_type_->is_autocreate());
  EXPECT_TRUE(team_type_->is_prebuilt());
  EXPECT_FALSE(team_type_->is_reinforceable());
  EXPECT_EQ(team_type_->trigger(), nullptr);
}

TEST_F(TeamTypeTest, AMemberMayBeAnAircraftOrAVessel) {
  team_type_->ReadIniEntry(
      "fleet",
      absl::StrCat("2,0,7,0,1,-1,-1,2,", Aircraft(), ":2,", Vessel(), ":4,0"));

  ASSERT_EQ(team_type_->member_count(), 2);
  EXPECT_EQ(base::At(team_type_->members(), 0).type,
            &AircraftTypeClass::As_Reference(AIRCRAFT_HIND));
  EXPECT_EQ(base::At(team_type_->members(), 1).type,
            &VesselTypeClass::As_Reference(VESSEL_SS));
  EXPECT_EQ(base::At(team_type_->members(), 1).quantity, 4);
}

TEST_F(TeamTypeTest, ReadIniEntrySkipsAnUnknownFirstMember) {
  team_type_->ReadIniEntry("tanks", absl::StrCat("2,0,7,0,1,-1,-1,2,BOGUS:1,",
                                                 Infantry(), ":3,1,3:5"));

  ASSERT_EQ(team_type_->member_count(), 1);
  EXPECT_EQ(base::At(team_type_->members(), 0).type,
            &InfantryTypeClass::As_Reference(INFANTRY_E1));
  EXPECT_EQ(base::At(team_type_->members(), 0).quantity, 3);
  ASSERT_EQ(team_type_->mission_count(), 1);
  EXPECT_EQ(base::At(team_type_->missions(), 0).mission(),
            TeamMissionType::kMove);
  EXPECT_EQ(base::At(team_type_->missions(), 0).value(), 5);
}

TEST_F(TeamTypeTest, ReadIniEntryTakesAnOriginThatIsNoWaypointAsNone) {
  team_type_->ReadIniEntry("tanks", "2,0,7,0,1,500,-1,0,0");

  EXPECT_EQ(team_type_->origin(), -1);
}

TEST_F(TeamTypeTest, AnEntryThatStopsBeforeTheMembersHasNeither) {
  team_type_->ReadIniEntry("tanks", "2,0,7,0,1,-1,-1");

  EXPECT_EQ(team_type_->member_count(), 0);
  EXPECT_EQ(team_type_->mission_count(), 0);
}

TEST_F(TeamTypeTest, MoreKindsOfMemberThanATeamTypeHoldsLeaveItEmpty) {
  team_type_->ReadIniEntry(
      "tanks", absl::StrCat("2,0,7,0,1,-1,-1,6,", Infantry(), ":1,1,3:5"));

  EXPECT_EQ(team_type_->member_count(), 0);
  EXPECT_EQ(team_type_->mission_count(), 0);
}

TEST_F(TeamTypeTest, AMemberListShorterThanItsCountLeavesTheTeamTypeEmpty) {
  team_type_->ReadIniEntry(
      "tanks", absl::StrCat("2,0,7,0,1,-1,-1,2,", Infantry(), ":1"));

  EXPECT_EQ(team_type_->member_count(), 0);
  EXPECT_EQ(team_type_->mission_count(), 0);
}

TEST_F(TeamTypeTest, MoreMissionsThanATeamTypeHoldsLeaveItEmpty) {
  team_type_->ReadIniEntry(
      "tanks", absl::StrCat("2,0,7,0,1,-1,-1,1,", Infantry(), ":1,21,3:5"));

  EXPECT_EQ(team_type_->member_count(), 0);
  EXPECT_EQ(team_type_->mission_count(), 0);
}

// The formats before 2 list the five options one by one, have a field that
// is not used where the trigger is now, and end with the trigger.
TEST_F(TeamTypeTest, TheOldFormatsListTheOptionsAndEndWithTheTrigger) {
  TheWorld().new_ini_format() = 1;

  team_type_->ReadIniEntry("tanks", absl::StrCat("2,1,0,1,0,1,9,1,3,5,3,1,",
                                                 Infantry(), ":3,1,3:5,2"));

  EXPECT_TRUE(team_type_->is_roundabout());
  EXPECT_FALSE(team_type_->is_suicide());
  EXPECT_TRUE(team_type_->is_autocreate());
  EXPECT_FALSE(team_type_->is_prebuilt());
  EXPECT_TRUE(team_type_->is_reinforceable());
  EXPECT_EQ(team_type_->recruit_priority(), 9);
  EXPECT_EQ(team_type_->max_teams(), 3);
  EXPECT_EQ(team_type_->origin(), 5);
  EXPECT_EQ(team_type_->member_count(), 1);
  EXPECT_EQ(team_type_->mission_count(), 1);
  EXPECT_EQ(team_type_->trigger(), TheObjectHeaps().trigger_type().Slot(2));
}

TEST_F(TeamTypeTest, ToIniEntryWritesWhatReadIniEntryReads) {
  auto* const trigger_type = base::NewPooled<TriggerType>();
  const std::string entry = absl::StrCat("2,21,9,1,3,5,0,2,", Infantry(), ":3,",
                                         Unit(), ":1,2,3:5,0:2");
  team_type_->ReadIniEntry("tanks", entry);
  ASSERT_EQ(team_type_->trigger(), trigger_type);

  EXPECT_EQ(team_type_->ToIniEntry(), entry);
}

TEST_F(TeamTypeTest, ToIniEntryWritesEveryOptionBitAndNoTriggerAsMinusOne) {
  const std::string entry = "2,10,7,0,1,-1,-1,0,0";
  team_type_->ReadIniEntry("tanks", entry);

  EXPECT_EQ(team_type_->ToIniEntry(), entry);
}

TEST_F(TeamTypeTest, ReadIniCreatesATeamTypePerEntry) {
  TeamType::FreeAll();
  engine::file::IniFile ini;
  ASSERT_TRUE(ini.PutString(kIniSection, "wave", SovietEntry(0, 2)));
  ASSERT_TRUE(ini.PutString(kIniSection, "dawn", SovietEntry(4, 5)));

  TeamType::ReadIni(ini);

  ASSERT_EQ(TheObjectHeaps().team_type().Count(), 2);
  const TeamType* const wave = TeamType::FromName("wave");
  const TeamType* const dawn = TeamType::FromName("dawn");
  ASSERT_NE(wave, nullptr);
  ASSERT_NE(dawn, nullptr);
  EXPECT_EQ(wave->max_teams(), 2);
  EXPECT_EQ(dawn->max_teams(), 5);
  EXPECT_TRUE(dawn->is_autocreate());
}

TEST_F(TeamTypeTest, ReadIniEntryRejectsAHouseThatIsNoHouse) {
  EXPECT_FALSE(team_type_->ReadIniEntry("tanks", "20,0,7,0,1,-1,-1,0,0"));
  EXPECT_FALSE(team_type_->ReadIniEntry("tanks", "-1,0,7,0,1,-1,-1,0,0"));
  EXPECT_TRUE(team_type_->ReadIniEntry("tanks", "19,0,7,0,1,-1,-1,0,0"));
}

TEST_F(TeamTypeTest, ReadIniEntryRejectsAMissionThatIsNoMission) {
  const auto entry = [](const std::string_view script) {
    return absl::StrCat("2,0,7,0,1,-1,-1,1,", Infantry(), ":1,", script);
  };

  EXPECT_FALSE(team_type_->ReadIniEntry("tanks", entry("2,3:5,17:0")));
  EXPECT_FALSE(team_type_->ReadIniEntry("tanks", entry("1,-1:0")));
  EXPECT_TRUE(team_type_->ReadIniEntry("tanks", entry("2,3:5,16:0")));
}

TEST_F(TeamTypeTest, ReadIniDropsTheEntriesItRejects) {
  TeamType::FreeAll();
  engine::file::IniFile ini;
  ASSERT_TRUE(ini.PutString(kIniSection, "wave", SovietEntry(0, 2)));
  ASSERT_TRUE(ini.PutString(kIniSection, "lost", "99,0,7,0,1,-1,-1,0,0"));
  ASSERT_TRUE(ini.PutString(kIniSection, "dawn", SovietEntry(4, 5)));

  TeamType::ReadIni(ini);

  EXPECT_EQ(TheObjectHeaps().team_type().Count(), 2);
  EXPECT_NE(TeamType::FromName("wave"), nullptr);
  EXPECT_EQ(TeamType::FromName("lost"), nullptr);
  EXPECT_NE(TeamType::FromName("dawn"), nullptr);
}

// A scenario numbers its team types by their place in the section, so an
// entry that is dropped must not move the later ones down a slot.
TEST_F(TeamTypeTest, ReadIniLeavesTheSlotOfARejectedEntryEmpty) {
  TeamType::FreeAll();
  engine::file::IniFile ini;
  ASSERT_TRUE(ini.PutString(kIniSection, "wave", SovietEntry(0, 2)));
  ASSERT_TRUE(ini.PutString(kIniSection, "lost", "99,0,7,0,1,-1,-1,0,0"));
  ASSERT_TRUE(ini.PutString(kIniSection, "dawn", SovietEntry(4, 5)));

  TeamType::ReadIni(ini);

  const auto& heap = TheObjectHeaps().team_type();
  EXPECT_EQ(heap.SlotIndexOf(TeamType::FromName("wave")), 0);
  EXPECT_FALSE(heap.IsAllocated(1));
  EXPECT_EQ(heap.SlotIndexOf(TeamType::FromName("dawn")), 2);
}

// The scenario editor leaves a hole in a heap when it deletes a type, and the
// next type it creates fills the hole but is listed last. The references are
// written as places in the list, which is the order of the entries and so the
// slots a scenario is read into.
TEST_F(TeamTypeTest, ReferencesSurviveWritingAndReadingHeapsWithHoles) {
  TeamType::FreeAll();
  auto* const gone_team = base::NewPooled<TeamType>();
  auto* const boats = base::NewPooled<TeamType>();
  auto* const tanks = base::NewPooled<TeamType>();
  auto* const gone_trigger = base::NewPooled<TriggerType>();
  auto* const alarm = base::NewPooled<TriggerType>();
  auto* const siren = base::NewPooled<TriggerType>();
  base::DeletePooled(gone_team);
  base::DeletePooled(gone_trigger);
  // These take slot 0 of their heaps and the last place in the lists.
  auto* const jeeps = base::NewPooled<TeamType>();
  auto* const flare = base::NewPooled<TriggerType>();
  ASSERT_EQ(TheObjectHeaps().team_type().SlotIndexOf(jeeps), 0);
  ASSERT_EQ(TheObjectHeaps().trigger_type().SlotIndexOf(flare), 0);

  ASSERT_TRUE(boats->ReadIniEntry("boats", SovietEntry(0, 1)));
  ASSERT_TRUE(tanks->ReadIniEntry("tanks", SovietEntry(0, 1)));
  ASSERT_TRUE(jeeps->ReadIniEntry("jeeps", SovietEntry(0, 1)));
  alarm->Set_Name("alrm");
  siren->Set_Name("sirn");
  flare->Set_Name("flar");
  boats->set_trigger(flare);
  tanks->set_trigger(alarm);
  jeeps->set_trigger(siren);
  alarm->event1.Team = jeeps;
  siren->event1.Team = boats;
  flare->event1.Team = tanks;
  alarm->action1 = MakeTriggerAction(
      {.type = TriggerActionType::kCreateTeam, .team_type = tanks});
  alarm->action2 = MakeTriggerAction(
      {.type = TriggerActionType::kForceTrigger, .trigger_type = flare});
  flare->action1 = MakeTriggerAction(
      {.type = TriggerActionType::kCreateTeam, .team_type = jeeps});
  flare->action2 = MakeTriggerAction(
      {.type = TriggerActionType::kForceTrigger, .trigger_type = siren});

  engine::file::IniFile ini;
  TeamType::WriteIni(ini);
  TriggerType::WriteIni(ini);
  TeamType::FreeAll();
  TriggerType::DestroyAll();
  TeamType::ReadIni(ini);
  TriggerType::ReadIni(ini);

  const auto team = [](const char* name) { return TeamType::FromName(name); };
  const auto trigger = [](const char* name) {
    return TriggerType::FromName(name);
  };
  ASSERT_EQ(TheObjectHeaps().team_type().Count(), 3);
  ASSERT_EQ(TheObjectHeaps().trigger_type().Count(), 3);
  EXPECT_EQ(team("boats")->trigger(), trigger("flar"));
  EXPECT_EQ(team("tanks")->trigger(), trigger("alrm"));
  EXPECT_EQ(team("jeeps")->trigger(), trigger("sirn"));
  EXPECT_EQ(trigger("alrm")->event1.Team, team("jeeps"));
  EXPECT_EQ(trigger("sirn")->event1.Team, team("boats"));
  EXPECT_EQ(trigger("flar")->event1.Team, team("tanks"));
  EXPECT_EQ(trigger("alrm")->action1->ToRecord().team_type, team("tanks"));
  EXPECT_EQ(trigger("alrm")->action2->ToRecord().trigger_type, trigger("flar"));
  EXPECT_EQ(trigger("flar")->action1->ToRecord().team_type, team("jeeps"));
  EXPECT_EQ(trigger("flar")->action2->ToRecord().trigger_type, trigger("sirn"));
}

TEST_F(TeamTypeTest, ReadIniDropsTheEntriesTheHeapHasNoRoomFor) {
  TeamType::FreeAll();
  engine::file::IniFile ini;
  for (const char* const name : {"a", "b", "c", "d", "e"}) {
    ASSERT_TRUE(ini.PutString(kIniSection, name, SovietEntry(0, 1)));
  }

  TeamType::ReadIni(ini);

  EXPECT_EQ(TheObjectHeaps().team_type().Count(), 4);
  EXPECT_EQ(TeamType::FromName("e"), nullptr);
}

TEST_F(TeamTypeTest, WriteIniReplacesTheSectionWithEveryTeamType) {
  team_type_->ReadIniEntry("tanks", SovietEntry(4, 2));
  base::NewPooled<TeamType>()->ReadIniEntry("boats", SovietEntry(0, 1));
  engine::file::IniFile ini;
  ASSERT_TRUE(ini.PutString(kIniSection, "stale", "1"));

  TeamType::WriteIni(ini);

  ASSERT_EQ(ini.EntryCount(kIniSection), 2);
  EXPECT_EQ(ini.GetString(kIniSection, "tanks", ""), SovietEntry(4, 2));
  EXPECT_EQ(ini.GetString(kIniSection, "boats", ""), SovietEntry(0, 1));
}

TEST_F(TeamTypeTest, FreeAllEmptiesTheHeap) {
  base::NewPooled<TeamType>();

  TeamType::FreeAll();

  EXPECT_EQ(TheObjectHeaps().team_type().Count(), 0);
}

TEST_F(TeamTypeTest, AFullHeapGivesNoTeamType) {
  for (int i = 0; i < 3; i++) {
    ASSERT_NE(base::NewPooled<TeamType>(), nullptr);
  }

  EXPECT_EQ(base::NewPooled<TeamType>(), nullptr);
}

TEST_F(TeamTypeTest, FromNameIgnoresCaseAndFindsALaterTeamType) {
  team_type_->Set_Name("tanks");
  auto* const boats = base::NewPooled<TeamType>();
  boats->Set_Name("boats");

  EXPECT_EQ(TeamType::FromName("TANKS"), team_type_);
  EXPECT_EQ(TeamType::FromName("Boats"), boats);
}

TEST_F(TeamTypeTest, FromNameGivesNullForAnUnknownOrEmptyName) {
  team_type_->Set_Name("tanks");

  EXPECT_EQ(TeamType::FromName("planes"), nullptr);
  EXPECT_EQ(TeamType::FromName(""), nullptr);
}

TEST_F(TeamTypeTest, DetachDropsTheTriggerTypeThatIsGoing) {
  auto* const trigger_type = base::NewPooled<TriggerType>();
  team_type_->set_trigger(trigger_type);

  team_type_->Detach(trigger_type->As_Target());

  EXPECT_EQ(team_type_->trigger(), nullptr);
}

TEST_F(TeamTypeTest, DetachLeavesAnotherTriggerTypeAndOtherObjectsAlone) {
  auto* const mine = base::NewPooled<TriggerType>();
  const auto* const other = base::NewPooled<TriggerType>();
  team_type_->set_trigger(mine);

  team_type_->Detach(other->As_Target());
  team_type_->Detach(team_type_->As_Target());

  EXPECT_EQ(team_type_->trigger(), mine);
}

TEST_F(TeamTypeTest, DetachWithNoTriggerDoesNothing) {
  const auto* const trigger_type = base::NewPooled<TriggerType>();

  team_type_->Detach(trigger_type->As_Target());

  EXPECT_EQ(team_type_->trigger(), nullptr);
}

TEST_F(TeamTypeTest, AddMissionAndAddMemberAppend) {
  team_type_->AddMission({TeamMissionType::kMove, 3});
  team_type_->AddMission({TeamMissionType::kUnload, 0});
  team_type_->AddMember(
      {.quantity = 2, .type = &UnitTypeClass::As_Reference(UNIT_JEEP)});

  ASSERT_EQ(team_type_->mission_count(), 2);
  EXPECT_EQ(base::At(team_type_->missions(), 1).mission(),
            TeamMissionType::kUnload);
  ASSERT_EQ(team_type_->member_count(), 1);
  EXPECT_EQ(base::At(team_type_->members(), 0).quantity, 2);

  team_type_->ClearMembers();
  EXPECT_EQ(team_type_->member_count(), 0);
}

TEST_F(TeamTypeTest, OneScriptLineOrMemberTooManyEndsTheGame) {
  for (int i = 0; i < TeamType::kMaxMissions; i++) {
    team_type_->AddMission({TeamMissionType::kUnload, 0});
  }
  for (int i = 0; i < TeamType::kMaxMemberTypes; i++) {
    team_type_->AddMember({});
  }

  // The switches and formatted diagnostics are inside GoogleTest's macro.
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(team_type_->AddMission({TeamMissionType::kUnload, 0}),
               "Check failed");
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(team_type_->AddMember({}), "Check failed");
}

TEST_F(TeamTypeTest, SetOriginTakesAWaypointOfTheScenarioOrNone) {
  team_type_->set_origin(100);
  EXPECT_EQ(team_type_->origin(), 100);
  team_type_->set_origin(101);
  EXPECT_EQ(team_type_->origin(), -1);
  team_type_->set_origin(0);
  EXPECT_EQ(team_type_->origin(), 0);
  team_type_->set_origin(-7);
  EXPECT_EQ(team_type_->origin(), -1);
}

TEST_F(TeamTypeTest, ATeamTypeBelowItsLimitCreatesATeamOfItsHouse) {
  team_type_->ReadIniEntry("tanks", SovietEntry(0, 1));

  const TeamClass* const team = team_type_->CreateTeam();

  ASSERT_NE(team, nullptr);
  EXPECT_EQ(TheObjectHeaps().team().Count(), 1);
  EXPECT_EQ(team_type_->CountTeams(), 1);
}

// Only its own teams, and only the ones that still exist.
TEST_F(TeamTypeTest, TheTeamCountFollowsTheTeamsInTheField) {
  team_type_->ReadIniEntry("tanks", SovietEntry(0, 2));
  auto* const other = base::NewPooled<TeamType>();
  other->ReadIniEntry("more", SovietEntry(0, 2));
  const TeamClass* const team = team_type_->CreateTeam();
  ASSERT_NE(team, nullptr);
  ASSERT_NE(team_type_->CreateTeam(), nullptr);
  ASSERT_NE(other->CreateTeam(), nullptr);
  EXPECT_EQ(team_type_->CountTeams(), 2);

  base::DeletePooled(team);

  EXPECT_EQ(team_type_->CountTeams(), 1);
  EXPECT_EQ(other->CountTeams(), 1);
}

TEST_F(TeamTypeTest, ATeamTypeAtItsLimitCreatesNoTeam) {
  team_type_->ReadIniEntry("tanks", SovietEntry(0, 1));
  ASSERT_NE(team_type_->CreateTeam(), nullptr);

  EXPECT_EQ(team_type_->CreateTeam(), nullptr);
  EXPECT_EQ(team_type_->CountTeams(), 1);
}

TEST_F(TeamTypeTest, ScenarioSetUpCreatesATeamPastTheLimit) {
  team_type_->ReadIniEntry("tanks", SovietEntry(0, 0));
  TheWorld().scenario_init()++;

  const TeamClass* const team = team_type_->CreateTeam();

  TheWorld().scenario_init()--;
  EXPECT_NE(team, nullptr);
}

TEST_F(TeamTypeTest, DestroyTeamsDeletesItsOwnTeamsOnly) {
  team_type_->ReadIniEntry("tanks", SovietEntry(0, 2));
  auto* const other = base::NewPooled<TeamType>();
  other->ReadIniEntry("boats", SovietEntry(0, 1));
  ASSERT_NE(team_type_->CreateTeam(), nullptr);
  const TeamClass* const kept = other->CreateTeam();
  ASSERT_NE(team_type_->CreateTeam(), nullptr);

  team_type_->DestroyTeams();

  ASSERT_EQ(TheObjectHeaps().team().Count(), 1);
  EXPECT_EQ(TheObjectHeaps().team().ActiveAt(0), kept);
}

TEST_F(TeamTypeTest, AHouseAtPeaceIsOfferedATeamTypeThatIsNotAutocreate) {
  team_type_->ReadIniEntry("auto", SovietEntry(4, 1));
  auto* const plain = base::NewPooled<TeamType>();
  plain->ReadIniEntry("plain", SovietEntry(0, 1));

  EXPECT_EQ(TeamType::SuggestNewTeam(house_, false), plain);
}

TEST_F(TeamTypeTest, AnAlertedHouseIsOfferedAnAutocreateTeamType) {
  team_type_->ReadIniEntry("auto", SovietEntry(4, 1));
  base::NewPooled<TeamType>()->ReadIniEntry("plain", SovietEntry(0, 1));

  EXPECT_EQ(TeamType::SuggestNewTeam(house_, true), team_type_);
}

TEST_F(TeamTypeTest, ATeamTypeAtItsLimitIsNotOffered) {
  team_type_->ReadIniEntry("tanks", SovietEntry(0, 1));
  ASSERT_NE(team_type_->CreateTeam(), nullptr);

  EXPECT_EQ(TeamType::SuggestNewTeam(house_, false), nullptr);
}

TEST_F(TeamTypeTest, AnotherHousesTeamTypeIsNotOffered) {
  team_type_->ReadIniEntry("greek", absl::StrCat(static_cast<int>(HOUSE_GREECE),
                                                 ",0,7,0,1,-1,-1,0,0"));

  EXPECT_EQ(TeamType::SuggestNewTeam(house_, false), nullptr);
}

TEST_F(TeamTypeTest, NoTeamTypesOfferNone) {
  TeamType::FreeAll();

  EXPECT_EQ(TeamType::SuggestNewTeam(house_, false), nullptr);
}

// The original game drew among the first twenty candidates only.
TEST_F(TeamTypeTest, EveryCandidateTakesPartInTheDraw) {
  TeamType::FreeAll();
  TheObjectHeaps().team_type().Reset(21);
  TeamType* last = nullptr;
  for (int i = 0; i < 21; i++) {
    last = base::NewPooled<TeamType>();
    ASSERT_NE(last, nullptr);
    last->ReadIniEntry("t", SovietEntry(0, 1));
  }

  bool last_was_offered = false;
  for (int draw = 0; draw < 2000 && !last_was_offered; draw++) {
    const TeamType* const offered = TeamType::SuggestNewTeam(house_, false);
    ASSERT_NE(offered, nullptr);
    last_was_offered = offered == last;
  }
  EXPECT_TRUE(last_was_offered);
}

TEST_F(TeamTypeTest, ASavedHeapLoadsEveryTeamTypeIntoItsOldSlot) {
  auto* const trigger_type = base::NewPooled<TriggerType>();
  // Slots 0 and 2 are in use and slot 1 is not.
  auto* const gap = base::NewPooled<TeamType>();
  auto* const second = base::NewPooled<TeamType>();
  base::DeletePooled(gap);
  const std::string entry = absl::StrCat("2,21,9,1,3,5,0,2,", Infantry(), ":3,",
                                         Vessel(), ":1,2,3:5,11:2");
  team_type_->ReadIniEntry("tanks", entry);
  second->ReadIniEntry("boats", SovietEntry(4, 2));
  std::vector<std::byte> saved;
  {
    VectorSink sink(saved);
    ArchiveWriter writer(sink);
    ASSERT_TRUE(TheObjectHeaps().team_type().Save(writer));
  }
  TeamType::FreeAll();

  SpanSource source{std::span<const std::byte>(saved)};
  ArchiveReader reader(source);
  ASSERT_TRUE(TheObjectHeaps().team_type().Load(reader)) << reader.error();

  ASSERT_EQ(TheObjectHeaps().team_type().Count(), 2);
  const TeamType* const loaded = TeamType::FromName("tanks");
  ASSERT_EQ(loaded, team_type_);
  EXPECT_EQ(TeamType::FromName("boats"), second);
  EXPECT_TRUE(loaded->is_active());
  EXPECT_EQ(loaded->ToIniEntry(), entry);
  EXPECT_EQ(loaded->trigger(), trigger_type);
  EXPECT_EQ(second->ToIniEntry(), SovietEntry(4, 2));
}

TEST_F(TeamTypeTest, TheEditorNamesEveryTeamMission) {
  for (const TeamMissionType mission :
       magic_enum::enum_values<TeamMissionType>()) {
    EXPECT_FALSE(std::string_view(TeamType::MissionName(mission)).empty());
  }
  EXPECT_STREQ(TeamType::MissionName(TeamMissionType::kUnload), "Unload");
}

TEST_F(TeamTypeTest, EachTeamMissionNeedsItsKindOfArgument) {
  EXPECT_EQ(TeamMissionNeeds(TeamMissionType::kFormation), NEED_FORMATION);
  EXPECT_EQ(TeamMissionNeeds(TeamMissionType::kAttack), NEED_QUARRY);
  EXPECT_EQ(TeamMissionNeeds(TeamMissionType::kMoveCell), NEED_HEX_NUMBER);
  EXPECT_EQ(TeamMissionNeeds(TeamMissionType::kDo), NEED_MISSION);
  for (const TeamMissionType mission :
       {TeamMissionType::kSetGlobal, TeamMissionType::kGuard,
        TeamMissionType::kLoop}) {
    EXPECT_EQ(TeamMissionNeeds(mission), NEED_NUMBER);
  }
  for (const TeamMissionType mission :
       {TeamMissionType::kPatrol, TeamMissionType::kMove,
        TeamMissionType::kAttackWaypoint, TeamMissionType::kSpy}) {
    EXPECT_EQ(TeamMissionNeeds(mission), NEED_WAYPOINT);
  }
  for (const TeamMissionType mission :
       {TeamMissionType::kNone, TeamMissionType::kAttackTarcom,
        TeamMissionType::kUnload, TeamMissionType::kDeploy,
        TeamMissionType::kHoundDog, TeamMissionType::kInvulnerable,
        TeamMissionType::kLoad}) {
    EXPECT_EQ(TeamMissionNeeds(mission), NEED_NONE);
  }
}

TEST_F(TeamTypeTest, TheDescriptionsAreEmptyWithoutTheEditor) {
  if (HasDescriptions()) {
    GTEST_SKIP() << "this build has the editor's descriptions";
  }
  team_type_->ReadIniEntry("tanks", SovietEntry(0, 1));

  EXPECT_TRUE(team_type_->Description().empty());
  EXPECT_TRUE(team_type_->MemberDescription().empty());
  EXPECT_TRUE(TeamMission(TeamMissionType::kMove, 3).Description(0).empty());
}

TEST_F(TeamTypeTest, TheDescriptionListsNameHouseOriginMissionsAndMembers) {
  if (!HasDescriptions()) {
    GTEST_SKIP() << "this build has no editor descriptions";
  }
  // Autocreate, origin waypoint 27, one mission.
  team_type_->ReadIniEntry(
      "tanks", absl::StrCat(static_cast<int>(HOUSE_USSR), ",4,7,0,1,27,-1,1,",
                            Infantry(), ":3,1,3:5"));

  EXPECT_EQ(
      team_type_->Description(),
      absl::StrCat("tanks\t", HouseTypeClass::As_Reference(HOUSE_USSR).Suffix,
                   "\t*AB\t1\t", Infantry(), ":3"));
}

TEST_F(TeamTypeTest, TheDescriptionOfANewTeamTypeHasNoHouseAndNoOrigin) {
  if (!HasDescriptions()) {
    GTEST_SKIP() << "this build has no editor descriptions";
  }
  team_type_->Set_Name("new");

  EXPECT_EQ(team_type_->Description(), "new\t\t \t0\t");
}

TEST_F(TeamTypeTest, TheMemberDescriptionListsEachKindAndItsCount) {
  if (!HasDescriptions()) {
    GTEST_SKIP() << "this build has no editor descriptions";
  }
  team_type_->ReadIniEntry(
      "tanks",
      absl::StrCat("2,0,7,0,1,-1,-1,2,", Infantry(), ":3,", Unit(), ":1,0"));

  EXPECT_EQ(team_type_->MemberDescription(),
            absl::StrCat(Infantry(), ":3,", Unit(), ":1"));
}

TEST_F(TeamTypeTest, ALongMemberDescriptionIsCutToTwentyFiveCharacters) {
  if (!HasDescriptions()) {
    GTEST_SKIP() << "this build has no editor descriptions";
  }
  team_type_->ReadIniEntry(
      "tanks", absl::StrCat("2,0,7,0,1,-1,-1,5,", Infantry(), ":100,", Unit(),
                            ":100,", Aircraft(), ":100,", Vessel(), ":100,",
                            Infantry(), ":100,0"));

  const std::string text = team_type_->MemberDescription();

  EXPECT_EQ(text.size(), 25U);
  EXPECT_TRUE(text.ends_with("..."));
  EXPECT_TRUE(text.starts_with(absl::StrCat(Infantry(), ":100,")));
}

TEST_F(TeamTypeTest, AScriptLineIsDescribedByItsNumberMissionAndArgument) {
  if (!HasDescriptions()) {
    GTEST_SKIP() << "this build has no editor descriptions";
  }
  const auto line = [](const TeamMissionType mission, const int value) {
    return TeamMission(mission, value).Description(4);
  };
  const auto name = [](const TeamMissionType mission) {
    return absl::StrCat("4\t", TeamType::MissionName(mission));
  };

  // No argument, a number, a hex cell number and a waypoint's letters.
  EXPECT_EQ(line(TeamMissionType::kUnload, 0), name(TeamMissionType::kUnload));
  EXPECT_EQ(line(TeamMissionType::kGuard, 30),
            absl::StrCat(name(TeamMissionType::kGuard), "30"));
  EXPECT_EQ(line(TeamMissionType::kMoveCell, 0x1a2),
            absl::StrCat(name(TeamMissionType::kMoveCell), "1a2"));
  EXPECT_EQ(line(TeamMissionType::kMove, 27),
            absl::StrCat(name(TeamMissionType::kMove), "AB"));
}

TEST_F(TeamTypeTest, AScriptLineNamesItsQuarryFormationOrUnitMission) {
  if (!HasDescriptions()) {
    GTEST_SKIP() << "this build has no editor descriptions";
  }
  TeamMission attack(TeamMissionType::kAttack, 0);
  attack.set_quarry(QUARRY_BUILDINGS);
  TeamMission formation(TeamMissionType::kFormation, 0);
  formation.set_formation(FORMATION_LOOSE);
  TeamMission order(TeamMissionType::kDo, 0);
  order.set_unit_mission(MISSION_GUARD);

  EXPECT_EQ(attack.Description(0),
            absl::StrCat("0\t", TeamType::MissionName(TeamMissionType::kAttack),
                         QuarryName.at(QUARRY_BUILDINGS)));
  EXPECT_EQ(
      formation.Description(1),
      absl::StrCat("1\t", TeamType::MissionName(TeamMissionType::kFormation),
                   FormationName.at(FORMATION_LOOSE)));
  EXPECT_EQ(order.Description(2),
            absl::StrCat("2\t", TeamType::MissionName(TeamMissionType::kDo),
                         MissionClass::Mission_Name(MISSION_GUARD)));
  EXPECT_EQ(formation.formation(), FORMATION_LOOSE);
  EXPECT_EQ(order.unit_mission(), MISSION_GUARD);
}

// Draws a team type and a script line as list entries on a page of their own.
// Each colour scheme of UiFixture draws in one colour, so the page tells which
// scheme drew.
class TeamTypeDrawTest : public TeamTypeTest {
 protected:
  static constexpr int kWidth = 600;
  static constexpr int kHeight = 16;

  void SetUp() override {
    TeamTypeTest::SetUp();
    team_type_->ReadIniEntry("tanks", SovietEntry(0, 1));
    previous_scheme_ = Get_Color_Scheme();
    Set_Color_Scheme(&ThePalettes().color_remaps().at(PCOLOR_RED));
  }

  void TearDown() override {
    Set_Color_Scheme(previous_scheme_);
    TeamTypeTest::TearDown();
  }

  // Draws the team type at the page's top left corner.
  void Draw(const bool selected, const FontType font,
            const TextPrintType flags = TPF_NONE) {
    PixelView& view = page_.view();
    team_type_->Draw_It(view, 0, 0, 0, kWidth, kHeight, selected, font, flags);
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

  PixelBuffer page_{kWidth, kHeight};
  // The scheme to put back, which outlives this test's Game.
  RemapControlType* previous_scheme_ = nullptr;
};

TEST_F(TeamTypeDrawTest, ASelectedEntryInTheEditorFontIsABarOfTheScheme) {
  if (!HasDescriptions()) {
    GTEST_SKIP() << "this build has no editor descriptions";
  }
  Draw(true, FontType::kEditor);

  // The bar is the whole box; the text on it is the scheme's colour too.
  EXPECT_EQ(CountOf(DialogColor()), kWidth * kHeight);
}

TEST_F(TeamTypeDrawTest, AnUnselectedEntryInAGradientFontHasNoBar) {
  if (!HasDescriptions()) {
    GTEST_SKIP() << "this build has no editor descriptions";
  }
  Draw(false, FontType::k6PointGradient);

  const int text = CountOf(DialogColor());
  EXPECT_GT(text, 0);
  EXPECT_LT(text, kWidth * kHeight);
  EXPECT_EQ(CountOf(game::kTransparentBlack), (kWidth * kHeight) - text);
}

TEST_F(TeamTypeDrawTest, AnUnselectedEntryThatUsesTheGradientPaletteIsDrawn) {
  if (!HasDescriptions()) {
    GTEST_SKIP() << "this build has no editor descriptions";
  }
  Draw(false, FontType::k6PointGradient, TPF_USE_GRAD_PAL);

  EXPECT_LT(CountOf(game::kTransparentBlack), kWidth * kHeight);
}

// The editor's lists all use the editor font; any other is drawn the same.
TEST_F(TeamTypeDrawTest, ASelectedEntryInAPlainFontIsABarOfTheSchemeToo) {
  if (!HasDescriptions()) {
    GTEST_SKIP() << "this build has no editor descriptions";
  }
  Draw(true, FontType::k6Point);

  EXPECT_EQ(CountOf(DialogColor()), kWidth * kHeight);
}

TEST_F(TeamTypeDrawTest, NothingIsDrawnWithoutTheEditor) {
  if (HasDescriptions()) {
    GTEST_SKIP() << "this build has the editor's descriptions";
  }
  Draw(true, FontType::kEditor);

  EXPECT_EQ(CountOf(game::kTransparentBlack), kWidth * kHeight);
}

TEST_F(TeamTypeDrawTest, AScriptLineIsDrawnAsAListEntryToo) {
  if (!HasDescriptions()) {
    GTEST_SKIP() << "this build has no editor descriptions";
  }
  PixelView& view = page_.view();

  TeamMission(TeamMissionType::kMove, 3)
      .Draw_It(view, 0, 0, 0, kWidth, kHeight, true, FontType::kEditor,
               TPF_NONE);

  EXPECT_EQ(CountOf(DialogColor()), kWidth * kHeight);
}

TEST(ScenarioWaypointTest, AWaypointIsAnIndexIntoTheScenariosWaypoints) {
  EXPECT_FALSE(ScenarioClass::IsWaypoint(-1));
  EXPECT_TRUE(ScenarioClass::IsWaypoint(0));
  EXPECT_TRUE(ScenarioClass::IsWaypoint(ScenarioClass::kSpecialWaypoint));
  EXPECT_FALSE(ScenarioClass::IsWaypoint(ScenarioClass::kWaypointCount));
}

// The home, reinforcement and special waypoints are not placed by letter.
TEST(ScenarioWaypointTest, ALetterWaypointIsBelowTheReservedOnes) {
  EXPECT_FALSE(ScenarioClass::IsLetterWaypoint(-1));
  EXPECT_TRUE(ScenarioClass::IsLetterWaypoint(0));
  EXPECT_TRUE(ScenarioClass::IsLetterWaypoint(97));
  EXPECT_FALSE(ScenarioClass::IsLetterWaypoint(ScenarioClass::kHomeWaypoint));
  EXPECT_FALSE(
      ScenarioClass::IsLetterWaypoint(ScenarioClass::kSpecialWaypoint));
}

}  // namespace
}  // namespace ra
