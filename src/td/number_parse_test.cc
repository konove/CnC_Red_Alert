// Tests legacy INI defaults and team count validation through game readers.
#include <string>

#include "gtest/gtest.h"
#include "td/config.h"
#include "td/externs.h"
#include "td/profile.h"
#include "td/teamtype.h"

namespace {

TEST(TdNumberParseTest, InvalidProfileNumbersKeepTheDefault) {
  // The legacy profile reader expects DOS line endings, as in the game data.
  char profile[] =
      "[Values]\r\nBad=12tail\r\nBig=2147483648\r\nValid=-42\r\nHex=ff\r\n"
      "BadHex=fftail\r\n";
  EXPECT_EQ(WWGetPrivateProfileInt("Values", "Bad", 73, profile), 73);
  EXPECT_EQ(WWGetPrivateProfileInt("Values", "Big", 73, profile), 73);
  EXPECT_EQ(WWGetPrivateProfileInt("Values", "Valid", 73, profile), -42);
  EXPECT_EQ(WWGetPrivateProfileHex("Values", "Hex", profile), 255U);
  EXPECT_EQ(WWGetPrivateProfileHex("Values", "BadHex", profile), 0U);
}

// Team types validate that they live in the TeamTypes heap, so the tests
// allocate them there rather than on the stack.
class TdTeamTypeParseTest : public testing::Test {
 protected:
  static void SetUpTestSuite() { TeamTypes.Set_Heap(4); }
  static void TearDownTestSuite() { TeamTypes.Set_Heap(0); }
};

TEST_F(TdTeamTypeParseTest, RejectsInvalidTeamClassAndMissionCounts) {
  char name[] = "TestTeam";
  for (const char* count : {"-1", "999999", "2147483648", "1tail"}) {
    auto* heap_team = new TeamTypeClass();
    ASSERT_NE(heap_team, nullptr);
    TeamTypeClass& team = *heap_team;
    std::string entry = std::string{"GoodGuy,0,0,0,0,0,7,2,1,0,"} + count;
    team.Fill_In(name, entry.data());
    EXPECT_EQ(team.ClassCount, 0) << count;
    EXPECT_EQ(team.MissionCount, 0) << count;

    entry = std::string{"GoodGuy,0,0,0,0,0,7,2,1,0,0,"} + count;
    team.Fill_In(name, entry.data());
    EXPECT_EQ(team.ClassCount, 0) << count;
    EXPECT_EQ(team.MissionCount, 0) << count;
    delete heap_team;
  }
}

TEST_F(TdTeamTypeParseTest, PreservesValidEmptyTeamAndRejectsTruncatedMembers) {
  char name[] = "TestTeam";
  char entry[] = "GoodGuy,0,0,0,0,0,7,2,1,0,0,0";
  auto* heap_team = new TeamTypeClass();
  ASSERT_NE(heap_team, nullptr);
  TeamTypeClass& team = *heap_team;
  team.Fill_In(name, entry);
  EXPECT_EQ(team.RecruitPriority, 7);
  EXPECT_EQ(team.MaxAllowed, 2);
  EXPECT_EQ(team.InitNum, 1);
  EXPECT_EQ(team.ClassCount, 0);
  EXPECT_EQ(team.MissionCount, 0);

  char truncated[] = "GoodGuy,0,0,0,0,0,7,2,1,0,1";
  team.Fill_In(name, truncated);
  EXPECT_EQ(team.ClassCount, 0);
  EXPECT_EQ(team.MissionCount, 0);
  delete heap_team;
}

TEST_F(TdTeamTypeParseTest, TeamTypeOutsideItsHeapFailsValidation) {
  if (!config::kCheatKeysEnabled) {
    GTEST_SKIP() << "Object validation only runs with cheat keys enabled.";
  }
  // Validation used to exit(0), which ended the test binary as a success.
  char name[] = "TestTeam";
  char entry[] = "GoodGuy,0,0,0,0,0,7,2,1,0,0,0";
  TeamTypeClass stack_team;
  // The switch is inside GoogleTest's macro.
  // NOLINTNEXTLINE(clang-diagnostic-switch-default)
  EXPECT_DEATH(stack_team.Fill_In(name, entry), "TEAMTYPE object error");
}

}  // namespace
