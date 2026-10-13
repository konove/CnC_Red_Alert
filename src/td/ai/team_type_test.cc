// Tests for the team types in a real Game with no game data: how one reads
// and writes its scenario INI entry, which one the computer is offered, and
// the teams it creates, counts and destroys.

#include "td/ai/team_type.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "base/array.h"
#include "base/numeric.h"
#include "base/pooled.h"
#include "engine/file/ini_file.h"
#include "engine/stream/archive.h"
#include "engine/stream/span_source.h"
#include "engine/stream/vector_sink.h"
#include "gtest/gtest.h"
#include "td/ai/team.h"
#include "td/ai/trigger.h"
#include "td/core/defines.h"
#include "td/core/object_heaps.h"
#include "td/core/target.h"
#include "td/map/world.h"
#include "td/objects/house.h"
#include "td/objects/type.h"
#include "td/startup/game_state.h"
#include "td/startup/game_test_util.h"

namespace td {
namespace {

using engine::file::IniFile;

// A Game with room for four team types and four teams.
class TeamTypeTest : public GameFixture {
 protected:
  TeamTypeTest() {
    TheObjectHeaps().team().Reset(4);
    TheObjectHeaps().team_type().Reset(4);
  }

  // Loads `text` into `ini`, as the scenario reader does.
  static void Parse(const std::string_view text, IniFile& ini) {
    SpanSource source(std::as_bytes(std::span(text)));
    ASSERT_TRUE(ini.Load(source));
  }
};

TEST_F(TeamTypeTest, EntriesPastTheHeapsCapacityAreDropped) {
  TheObjectHeaps().team_type().Reset(2);
  IniFile ini;
  Parse(
      "[TeamTypes]\r\n"
      "one=GoodGuy,0,0,0,0,0,7,1,0,0,0,0\r\n"
      "two=GoodGuy,0,0,0,0,0,7,1,0,0,0,0\r\n"
      "three=GoodGuy,0,0,0,0,0,7,1,0,0,0,0\r\n",
      ini);

  TeamType::ReadIni(ini);

  EXPECT_EQ(TheObjectHeaps().team_type().Count(), 2);
  EXPECT_NE(TeamType::FromName("two"), nullptr);
  EXPECT_EQ(TeamType::FromName("three"), nullptr);
}

TEST_F(TeamTypeTest, OldFormatEntriesPastTheHeapsCapacityAreDropped) {
  TheObjectHeaps().team_type().Reset(1);
  IniFile ini;
  Parse(
      "[Teams]\r\n"
      "one=GoodGuy,0,0,0,0,0,7,1,0,Hunt,E1:2\r\n"
      "two=GoodGuy,0,0,0,0,0,7,1,0,Hunt,E1:2\r\n",
      ini);

  TeamType::ReadIni(ini);

  EXPECT_EQ(TheObjectHeaps().team_type().Count(), 1);
}

// The scenario need not have the house a team type is written for.
TEST_F(TeamTypeTest, ATeamTypeIsWrittenWithItsHousesNameWithoutThatHouse) {
  IniFile ini;
  Parse(
      "[TeamTypes]\r\n"
      "one=BadGuy,0,0,0,0,0,7,1,0,0,0,0,1,1\r\n",
      ini);
  TeamType::ReadIni(ini);
  ASSERT_EQ(TheObjectHeaps().team_type().Count(), 1);

  IniFile written;
  TeamType::WriteIni(written, true);

  EXPECT_EQ(written.GetString("TeamTypes", "one", ""),
            "BadGuy,0,0,0,0,0,7,1,0,0,0,0,1,1");
}

// Twenty lines with long names and arguments make an entry of more than 500
// characters.
TEST_F(TeamTypeTest, TheLongestEntryIsWrittenWhole) {
  std::string entry =
      "GoodGuy,1,1,1,1,1,15,255,255,255,5,E1:255,E2:255,E3:255,E4:255,E5:255,"
      "20";
  for (int line = 0; line < 20; line++) {
    entry += ",Attack Civil.:1000000";
  }
  entry += ",1,1";
  ASSERT_GT(entry.size(), 500U);
  IniFile ini;
  Parse("[TeamTypes]\r\nbig=" + entry + "\r\n", ini);
  TeamType::ReadIni(ini);
  ASSERT_EQ(TheObjectHeaps().team_type().Count(), 1);

  IniFile written;
  TeamType::WriteIni(written, true);

  EXPECT_EQ(written.GetString("TeamTypes", "big", ""), entry);
}

// A team type of GDI's with the most teams allowed set to `max_teams`.
TeamType& MakeTeamType(const int max_teams) {
  auto* team_type = base::NewPooled<TeamType>();
  EXPECT_NE(team_type, nullptr);
  team_type->set_house(HOUSE_GOOD);
  team_type->set_max_teams(static_cast<uint8_t>(max_teams));
  return *team_type;
}

TEST_F(TeamTypeTest, ATeamTypeBelowItsLimitCreatesATeam) {
  const TeamType& team_type = MakeTeamType(1);

  const TeamClass* const team = team_type.CreateTeam();

  ASSERT_NE(team, nullptr);
  EXPECT_EQ(team->Class, &team_type);
  EXPECT_EQ(team_type.CountTeams(), 1);
}

TEST_F(TeamTypeTest, ATeamTypeAtItsLimitCreatesNoTeam) {
  const TeamType& team_type = MakeTeamType(1);
  ASSERT_NE(team_type.CreateTeam(), nullptr);

  EXPECT_EQ(team_type.CreateTeam(), nullptr);
  EXPECT_EQ(team_type.CountTeams(), 1);
}

TEST_F(TeamTypeTest, ScenarioSetUpCreatesATeamPastTheLimit) {
  const TeamType& team_type = MakeTeamType(0);
  TheWorld().scenario_init()++;

  EXPECT_NE(team_type.CreateTeam(), nullptr);

  TheWorld().scenario_init()--;
}

// Only its own teams, and only the ones that still exist.
TEST_F(TeamTypeTest, TheTeamCountFollowsTheTeamsInTheField) {
  const TeamType& team_type = MakeTeamType(2);
  const TeamType& other = MakeTeamType(2);
  const TeamClass* const team = team_type.CreateTeam();
  ASSERT_NE(team, nullptr);
  ASSERT_NE(team_type.CreateTeam(), nullptr);
  ASSERT_NE(other.CreateTeam(), nullptr);
  EXPECT_EQ(team_type.CountTeams(), 2);

  base::DeletePooled(team);

  EXPECT_EQ(team_type.CountTeams(), 1);
  EXPECT_EQ(other.CountTeams(), 1);
}

TEST_F(TeamTypeTest, DestroyTeamsDeletesTheTeamsOfThisTypeOnly) {
  const TeamType& team_type = MakeTeamType(2);
  const TeamType& other = MakeTeamType(2);
  ASSERT_NE(team_type.CreateTeam(), nullptr);
  ASSERT_NE(other.CreateTeam(), nullptr);
  ASSERT_NE(team_type.CreateTeam(), nullptr);

  team_type.DestroyTeams();

  EXPECT_EQ(team_type.CountTeams(), 0);
  EXPECT_EQ(other.CountTeams(), 1);
}

// A transient team type exists for one special reinforcement.
TEST_F(TeamTypeTest, ATransientTeamTypeIsDeletedWithItsLastTeam) {
  TeamType& team_type = MakeTeamType(2);
  team_type.set_is_transient(true);
  const TeamClass* const first = team_type.CreateTeam();
  const TeamClass* const second = team_type.CreateTeam();
  ASSERT_NE(second, nullptr);
  // A team only cleans up after itself while a game is being played.
  TheGameState().active() = true;

  base::DeletePooled(first);
  EXPECT_EQ(TheObjectHeaps().team_type().Count(), 1);
  base::DeletePooled(second);

  EXPECT_EQ(TheObjectHeaps().team_type().Count(), 0);
}

// An entry of GDI's with every option on, three kinds of member and a script.
constexpr std::string_view kFullEntry =
    "GoodGuy,1,1,1,1,1,9,3,2,4,3,E1:3,JEEP:2,TRAN:1,"
    "2,Move:3,Attack Base:10,0,0";

TEST_F(TeamTypeTest, ANewTeamTypeHasNoHouseAndNothingInIt) {
  const auto* const team_type = base::NewPooled<TeamType>();
  ASSERT_NE(team_type, nullptr);

  EXPECT_TRUE(team_type->is_active());
  EXPECT_EQ(team_type->house(), HOUSE_NONE);
  EXPECT_EQ(team_type->member_count(), 0);
  EXPECT_EQ(team_type->mission_count(), 0);
  EXPECT_EQ(team_type->recruit_priority(), 7);
  EXPECT_TRUE(team_type->is_prebuilt());
  EXPECT_TRUE(team_type->is_reinforceable());
  EXPECT_FALSE(team_type->is_transient());
}

TEST_F(TeamTypeTest, AnEntryIsReadFieldByField) {
  auto* const team_type = base::NewPooled<TeamType>();
  ASSERT_NE(team_type, nullptr);

  team_type->ReadIniEntry("alpha", kFullEntry);

  EXPECT_STREQ(team_type->IniName, "alpha");
  EXPECT_EQ(team_type->house(), HOUSE_GOOD);
  EXPECT_TRUE(team_type->is_roundabout());
  EXPECT_TRUE(team_type->is_learning());
  EXPECT_TRUE(team_type->is_suicide());
  EXPECT_TRUE(team_type->is_autocreate());
  EXPECT_TRUE(team_type->is_mercenary());
  EXPECT_EQ(team_type->recruit_priority(), 9);
  EXPECT_EQ(team_type->max_teams(), 3);
  EXPECT_EQ(team_type->initial_count(), 2);
  EXPECT_EQ(team_type->fear(), 4);
  EXPECT_FALSE(team_type->is_reinforceable());
  EXPECT_FALSE(team_type->is_prebuilt());

  ASSERT_EQ(team_type->member_count(), 3);
  const std::span<const TeamMember> members = team_type->members();
  EXPECT_EQ(members.front().type,
            &InfantryTypeClass::As_Reference(INFANTRY_E1));
  EXPECT_EQ(members.front().quantity, 3);
  EXPECT_EQ(base::At(members, 1).type, &UnitTypeClass::As_Reference(UNIT_JEEP));
  EXPECT_EQ(base::At(members, 1).quantity, 2);
  EXPECT_EQ(members.back().type,
            &AircraftTypeClass::As_Reference(AIRCRAFT_TRANSPORT));

  ASSERT_EQ(team_type->mission_count(), 2);
  const std::span<const TeamMission> missions = team_type->missions();
  EXPECT_EQ(missions.front().mission, TeamMissionType::kMove);
  EXPECT_EQ(missions.front().argument, 3);
  EXPECT_EQ(missions.back().mission, TeamMissionType::kAttackBase);
  EXPECT_EQ(missions.back().argument, 10);
}

TEST_F(TeamTypeTest, AnEntryIsWrittenAsItWasRead) {
  auto* const team_type = base::NewPooled<TeamType>();
  ASSERT_NE(team_type, nullptr);
  team_type->ReadIniEntry("alpha", kFullEntry);

  EXPECT_EQ(team_type->ToIniEntry(), kFullEntry);
}

// The two options at the end of an entry came later than the rest.
TEST_F(TeamTypeTest, AnEntryWithoutItsLastTwoFieldsLeavesBothOptionsOn) {
  auto* const team_type = base::NewPooled<TeamType>();
  ASSERT_NE(team_type, nullptr);

  team_type->ReadIniEntry("alpha", "GoodGuy,0,0,0,0,0,7,1,0,0,0,0");

  EXPECT_TRUE(team_type->is_reinforceable());
  EXPECT_TRUE(team_type->is_prebuilt());
}

TEST_F(TeamTypeTest, AMemberOfAnUnknownTypeIsSkipped) {
  auto* const team_type = base::NewPooled<TeamType>();
  ASSERT_NE(team_type, nullptr);

  team_type->ReadIniEntry("alpha",
                          "GoodGuy,0,0,0,0,0,7,1,0,0,2,XYZZY:4,JEEP:2,0");

  ASSERT_EQ(team_type->member_count(), 1);
  EXPECT_EQ(team_type->members().front().type,
            &UnitTypeClass::As_Reference(UNIT_JEEP));
}

TEST_F(TeamTypeTest, AMemberListShorterThanItsCountLeavesNoMembers) {
  auto* const team_type = base::NewPooled<TeamType>();
  ASSERT_NE(team_type, nullptr);

  team_type->ReadIniEntry("alpha", "GoodGuy,0,0,0,0,0,7,1,0,0,2,E1:3");

  EXPECT_EQ(team_type->member_count(), 0);
  EXPECT_EQ(team_type->mission_count(), 0);
}

TEST_F(TeamTypeTest, AScriptShorterThanItsCountLeavesNothing) {
  auto* const team_type = base::NewPooled<TeamType>();
  ASSERT_NE(team_type, nullptr);

  team_type->ReadIniEntry("alpha", "GoodGuy,0,0,0,0,0,7,1,0,0,1,E1:3,2,Move:3");

  EXPECT_EQ(team_type->member_count(), 0);
  EXPECT_EQ(team_type->mission_count(), 0);
}

TEST_F(TeamTypeTest, AnUnknownMissionNameIsReadAsNoMission) {
  auto* const team_type = base::NewPooled<TeamType>();
  ASSERT_NE(team_type, nullptr);

  team_type->ReadIniEntry("alpha", "GoodGuy,0,0,0,0,0,7,1,0,0,0,1,Dance:3");

  ASSERT_EQ(team_type->mission_count(), 1);
  EXPECT_EQ(team_type->missions().front().mission, TeamMissionType::kNone);
  EXPECT_EQ(team_type->missions().front().argument, 3);
}

// The counts are bytes: a scenario's -1 has always read as 255.
TEST_F(TeamTypeTest, ATeamCountThatDoesNotFitAByteWraps) {
  auto* const team_type = base::NewPooled<TeamType>();
  ASSERT_NE(team_type, nullptr);

  team_type->ReadIniEntry("alpha", "GoodGuy,0,0,0,0,0,7,-1,300,0,0,0");

  EXPECT_EQ(team_type->max_teams(), 255);
  EXPECT_EQ(team_type->initial_count(), 44);
}

TEST_F(TeamTypeTest, TheOldFormatIsReadWhenThereAreNoTeamTypes) {
  IniFile ini;
  Parse(
      "[Teams]\r\n"
      "old=BadGuy,1,0,1,0,1,5,2,1,Hunt,E1:2,XYZZY:9,JEEP:1,TRAN:1,E2:1,E3:1,"
      "E4:1\r\n",
      ini);

  TeamType::ReadIni(ini);

  const TeamType* const team_type = TeamType::FromName("OLD");
  ASSERT_NE(team_type, nullptr);
  EXPECT_EQ(team_type->house(), HOUSE_BAD);
  EXPECT_TRUE(team_type->is_roundabout());
  EXPECT_FALSE(team_type->is_learning());
  EXPECT_TRUE(team_type->is_suicide());
  EXPECT_FALSE(team_type->is_autocreate());
  EXPECT_TRUE(team_type->is_mercenary());
  EXPECT_EQ(team_type->recruit_priority(), 5);
  EXPECT_EQ(team_type->max_teams(), 2);
  EXPECT_EQ(team_type->initial_count(), 1);
  EXPECT_EQ(team_type->mission_count(), 0);
  // The unknown type is skipped and the sixth known one does not fit.
  ASSERT_EQ(team_type->member_count(), TeamType::kMaxMemberTypes);
  EXPECT_EQ(team_type->members().front().quantity, 2);
  EXPECT_EQ(team_type->members().back().type,
            &InfantryTypeClass::As_Reference(INFANTRY_E3));
}

TEST_F(TeamTypeTest, WritingDeletesTheOldFormatSection) {
  IniFile ini;
  Parse("[Teams]\r\nold=BadGuy,0,0,0,0,0,5,2,1,Hunt,E1:2\r\n", ini);
  TeamType::ReadIni(ini);

  TeamType::WriteIni(ini, true);

  EXPECT_FALSE(ini.HasSection("Teams"));
  EXPECT_EQ(ini.GetString("TeamTypes", "old", ""),
            "BadGuy,0,0,0,0,0,5,2,1,0,1,E1:2,0,1,1");
}

TEST_F(TeamTypeTest, ASavedHeapLoadsEveryTeamTypeIntoItsOldSlot) {
  // Slots 0 and 2 are in use and slot 1 is not.
  auto* const first = base::NewPooled<TeamType>();
  auto* const gap = base::NewPooled<TeamType>();
  auto* const second = base::NewPooled<TeamType>();
  ASSERT_NE(second, nullptr);
  base::DeletePooled(gap);
  first->ReadIniEntry("alpha", kFullEntry);
  second->ReadIniEntry("beta", "BadGuy,0,0,0,0,0,7,1,0,0,0,0,1,1");
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
  EXPECT_EQ(TeamType::FromName("alpha"), first);
  EXPECT_EQ(TeamType::FromName("beta"), second);
  EXPECT_TRUE(first->is_active());
  EXPECT_EQ(first->ToIniEntry(), kFullEntry);
  EXPECT_EQ(second->ToIniEntry(), "BadGuy,0,0,0,0,0,7,1,0,0,0,0,1,1");
}

TEST_F(TeamTypeTest, FromNameFindsATeamTypeWhateverTheCase) {
  auto* const team_type = base::NewPooled<TeamType>();
  ASSERT_NE(team_type, nullptr);
  team_type->Set_Name("alpha");

  EXPECT_EQ(TeamType::FromName("ALPHA"), team_type);
  EXPECT_EQ(TeamType::FromName("beta"), nullptr);
  EXPECT_EQ(TeamType::FromName(""), nullptr);
}

TEST_F(TeamTypeTest, FreeAllLeavesNoTeamTypes) {
  ASSERT_NE(base::NewPooled<TeamType>(), nullptr);
  ASSERT_NE(base::NewPooled<TeamType>(), nullptr);

  TeamType::FreeAll();

  EXPECT_EQ(TheObjectHeaps().team_type().Count(), 0);
}

TEST_F(TeamTypeTest, AMissionHasANameAndTheNameAMission) {
  EXPECT_STREQ(TeamType::MissionName(TeamMissionType::kAttackCivilians),
               "Attack Civil.");
  EXPECT_STREQ(TeamType::MissionName(TeamMissionType::kNone), "None");
  EXPECT_EQ(TeamType::MissionFromName("attack civil."),
            TeamMissionType::kAttackCivilians);
  EXPECT_EQ(TeamType::MissionFromName("None"), TeamMissionType::kNone);
  EXPECT_EQ(TeamType::MissionFromName(""), TeamMissionType::kNone);
}

TEST_F(TeamTypeTest, TheTargetOfATeamTypeNamesItsSlot) {
  ASSERT_NE(base::NewPooled<TeamType>(), nullptr);
  const auto* const second = base::NewPooled<TeamType>();
  ASSERT_NE(second, nullptr);

  const Target target = second->As_Target();

  EXPECT_EQ(target.kind(), KIND_TEAMTYPE);
  EXPECT_EQ(target.value(), 1);
}

TEST_F(TeamTypeTest, DeleteClearsTheTriggersThatNameTheTeamType) {
  TheObjectHeaps().trigger().Reset(2);
  auto* const team_type = base::NewPooled<TeamType>();
  auto* const other = base::NewPooled<TeamType>();
  auto* const trigger = base::NewPooled<TriggerClass>();
  auto* const other_trigger = base::NewPooled<TriggerClass>();
  ASSERT_NE(other_trigger, nullptr);
  trigger->Team = team_type;
  other_trigger->Team = other;

  team_type->Delete();

  EXPECT_EQ(TheObjectHeaps().team_type().Count(), 1);
  EXPECT_EQ(trigger->Team, nullptr);
  EXPECT_EQ(other_trigger->Team, other);
  base::DeletePooled(trigger);
  base::DeletePooled(other_trigger);
}

TEST_F(TeamTypeTest, TheSettersSetTheOptions) {
  TeamType& team_type = MakeTeamType(1);

  team_type.set_is_roundabout(true);
  team_type.set_is_learning(true);
  team_type.set_is_suicide(true);
  team_type.set_is_autocreate(true);
  team_type.set_is_mercenary(true);
  team_type.set_is_prebuilt(false);
  team_type.set_is_reinforceable(false);
  team_type.set_recruit_priority(12);
  team_type.set_initial_count(3);

  EXPECT_EQ(team_type.ToIniEntry(), "GoodGuy,1,1,1,1,1,12,1,3,0,0,0,0,0");
}

TEST_F(TeamTypeTest, MembersAndLinesCanBeAddedAndCleared) {
  TeamType& team_type = MakeTeamType(1);
  team_type.AddMember(
      {.quantity = 2, .type = &UnitTypeClass::As_Reference(UNIT_JEEP)});
  team_type.AddMission({.mission = TeamMissionType::kGuard, .argument = 5});
  EXPECT_EQ(team_type.ToIniEntry(),
            "GoodGuy,0,0,0,0,0,7,1,0,0,1,JEEP:2,1,Guard:5,1,1");

  team_type.ClearMembers();
  team_type.ClearMissions();

  EXPECT_EQ(team_type.member_count(), 0);
  EXPECT_EQ(team_type.mission_count(), 0);
}

// A Game with GDI in it, for the team types the house's own AI picks among.
class SuggestNewTeamTest : public TeamTypeTest {
 protected:
  SuggestNewTeamTest() {
    TheObjectHeaps().house().Reset(1);
    house_ = base::NewPooled<HouseClass>(HOUSE_GOOD);
  }

  // A team type of GDI's with the given recruit priority that wants one of
  // `member_type` and may have one team.
  static TeamType& MakeCandidate(const int priority,
                                 const TechnoTypeClass& member_type) {
    TeamType& team_type = MakeTeamType(1);
    team_type.set_recruit_priority(priority);
    team_type.AddMember({.quantity = 1, .type = &member_type});
    return team_type;
  }

  // The bit a unit or infantry type has in the masks of what a house owns.
  static uint64_t Bit(const int type) { return base::Bit<uint64_t>(type); }

  HouseClass* house_ = nullptr;
};

TEST_F(SuggestNewTeamTest, NoTeamTypesSuggestNone) {
  EXPECT_EQ(TeamType::SuggestNewTeam(house_, 0, 0, false), nullptr);
}

TEST_F(SuggestNewTeamTest, AnotherHousesTeamTypeIsNotSuggested) {
  MakeCandidate(7, UnitTypeClass::As_Reference(UNIT_JEEP)).set_house(HOUSE_BAD);

  EXPECT_EQ(TeamType::SuggestNewTeam(house_, 0, 0, false), nullptr);
}

TEST_F(SuggestNewTeamTest, ATeamTypeAtItsLimitIsNotSuggested) {
  const TeamType& team_type =
      MakeCandidate(7, UnitTypeClass::As_Reference(UNIT_JEEP));
  ASSERT_NE(team_type.CreateTeam(), nullptr);

  EXPECT_EQ(TeamType::SuggestNewTeam(house_, 0, 0, false), nullptr);
}

TEST_F(SuggestNewTeamTest,
       AnAutocreateTeamTypeIsSuggestedOnlyToAnAlertedHouse) {
  TeamType& team_type =
      MakeCandidate(7, UnitTypeClass::As_Reference(UNIT_JEEP));
  team_type.set_is_autocreate(true);

  EXPECT_EQ(TeamType::SuggestNewTeam(house_, 0, 0, false), nullptr);
  EXPECT_EQ(TeamType::SuggestNewTeam(house_, 0, 0, true), &team_type);
}

TEST_F(SuggestNewTeamTest, TheHighestPriorityWinsAndTheFirstOfEquals) {
  const TeamType& first =
      MakeCandidate(5, UnitTypeClass::As_Reference(UNIT_JEEP));
  const TeamType& higher =
      MakeCandidate(9, UnitTypeClass::As_Reference(UNIT_JEEP));
  MakeCandidate(9, UnitTypeClass::As_Reference(UNIT_JEEP));

  EXPECT_NE(TeamType::SuggestNewTeam(house_, 0, 0, false), &first);
  EXPECT_EQ(TeamType::SuggestNewTeam(house_, 0, 0, false), &higher);
}

// A team type the house has no members for counts for half its priority.
TEST_F(SuggestNewTeamTest, ATeamTypeTheHouseHasVehiclesForBeatsAHigherOne) {
  MakeCandidate(9, UnitTypeClass::As_Reference(UNIT_HARVESTER));
  const TeamType& jeeps =
      MakeCandidate(6, UnitTypeClass::As_Reference(UNIT_JEEP));

  EXPECT_EQ(TeamType::SuggestNewTeam(house_, Bit(static_cast<int>(UNIT_JEEP)),
                                     0, false),
            &jeeps);
}

TEST_F(SuggestNewTeamTest, ATeamTypeTheHouseHasInfantryForBeatsAHigherOne) {
  MakeCandidate(9, UnitTypeClass::As_Reference(UNIT_HARVESTER));
  const TeamType& riflemen =
      MakeCandidate(6, InfantryTypeClass::As_Reference(INFANTRY_E1));

  EXPECT_EQ(TeamType::SuggestNewTeam(house_, 0,
                                     Bit(static_cast<int>(INFANTRY_E1)), false),
            &riflemen);
}

// Only vehicles and infantry are matched against what the house has.
TEST_F(SuggestNewTeamTest, AnAircraftTeamTypeAlwaysCountsForHalf) {
  MakeCandidate(9, AircraftTypeClass::As_Reference(AIRCRAFT_TRANSPORT));
  const TeamType& jeeps =
      MakeCandidate(5, UnitTypeClass::As_Reference(UNIT_JEEP));

  EXPECT_EQ(TeamType::SuggestNewTeam(house_, Bit(static_cast<int>(UNIT_JEEP)),
                                     0, false),
            &jeeps);
}

}  // namespace
}  // namespace td
