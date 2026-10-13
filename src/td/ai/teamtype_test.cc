// Tests for the team types in a real Game with no game data: how one reads
// and writes its scenario INI entry.

#include "td/ai/teamtype.h"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include "base/pooled.h"
#include "engine/file/ini_file.h"
#include "engine/stream/span_source.h"
#include "gtest/gtest.h"
#include "td/ai/team.h"
#include "td/core/defines.h"
#include "td/core/object_heaps.h"
#include "td/map/world.h"
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
  team_type->House = HOUSE_GOOD;
  team_type->MaxAllowed = static_cast<uint8_t>(max_teams);
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
  team_type.IsTransient = true;
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

}  // namespace
}  // namespace td
