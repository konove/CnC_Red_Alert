// Tests legacy INI defaults and team count validation through game readers.
#include <string>

#include "gtest/gtest.h"
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

TEST(TdNumberParseTest, RejectsInvalidTeamClassAndMissionCounts) {
  char name[] = "TestTeam";
  for (const char* count : {"-1", "999999", "2147483648", "1tail"}) {
    TeamTypeClass team;
    std::string entry = std::string{"GoodGuy,0,0,0,0,0,7,2,1,0,"} + count;
    team.Fill_In(name, entry.data());
    EXPECT_EQ(team.ClassCount, 0) << count;
    EXPECT_EQ(team.MissionCount, 0) << count;

    entry = std::string{"GoodGuy,0,0,0,0,0,7,2,1,0,0,"} + count;
    team.Fill_In(name, entry.data());
    EXPECT_EQ(team.ClassCount, 0) << count;
    EXPECT_EQ(team.MissionCount, 0) << count;
  }
}

TEST(TdNumberParseTest, PreservesValidEmptyTeamAndRejectsTruncatedMembers) {
  char name[] = "TestTeam";
  char entry[] = "GoodGuy,0,0,0,0,0,7,2,1,0,0,0";
  TeamTypeClass team;
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
}

}  // namespace
