// Tests team type count validation through the game readers.
#include <string>

#include "base/pooled.h"
#include "gtest/gtest.h"
#include "td/ai/teamtype.h"
#include "td/core/config.h"
#include "td/core/object_heaps.h"

namespace td {

namespace {

// Team types validate that they live in the TeamTypes heap, so the tests
// allocate them there rather than on the stack.
class TdTeamTypeParseTest : public testing::Test {
 protected:
  static void SetUpTestSuite() { TheObjectHeaps().team_type().Reset(4); }
  static void TearDownTestSuite() { TheObjectHeaps().team_type().Reset(0); }
};

TEST_F(TdTeamTypeParseTest, RejectsInvalidTeamClassAndMissionCounts) {
  char name[] = "TestTeam";
  for (const char* count : {"-1", "999999", "2147483648", "1tail"}) {
    auto* heap_team = base::NewPooled<TeamType>();
    ASSERT_NE(heap_team, nullptr);
    TeamType& team = *heap_team;
    std::string entry = std::string{"GoodGuy,0,0,0,0,0,7,2,1,0,"} + count;
    team.ReadIniEntry(name, entry);
    EXPECT_EQ(team.ClassCount, 0) << count;
    EXPECT_EQ(team.MissionCount, 0) << count;

    entry = std::string{"GoodGuy,0,0,0,0,0,7,2,1,0,0,"} + count;
    team.ReadIniEntry(name, entry);
    EXPECT_EQ(team.ClassCount, 0) << count;
    EXPECT_EQ(team.MissionCount, 0) << count;
    base::DeletePooled(heap_team);
  }
}

TEST_F(TdTeamTypeParseTest, PreservesValidEmptyTeamAndRejectsTruncatedMembers) {
  char name[] = "TestTeam";
  char entry[] = "GoodGuy,0,0,0,0,0,7,2,1,0,0,0";
  auto* heap_team = base::NewPooled<TeamType>();
  ASSERT_NE(heap_team, nullptr);
  TeamType& team = *heap_team;
  team.ReadIniEntry(name, entry);
  EXPECT_EQ(team.RecruitPriority, 7);
  EXPECT_EQ(team.MaxAllowed, 2);
  EXPECT_EQ(team.InitNum, 1);
  EXPECT_EQ(team.ClassCount, 0);
  EXPECT_EQ(team.MissionCount, 0);

  char truncated[] = "GoodGuy,0,0,0,0,0,7,2,1,0,1";
  team.ReadIniEntry(name, truncated);
  EXPECT_EQ(team.ClassCount, 0);
  EXPECT_EQ(team.MissionCount, 0);
  base::DeletePooled(heap_team);
}

TEST_F(TdTeamTypeParseTest, TeamTypeOutsideItsHeapFailsValidation) {
  if (!config::kCheatKeysEnabled) {
    GTEST_SKIP() << "Object validation only runs with cheat keys enabled.";
  }
  // Validation used to exit(0), which ended the test binary as a success.
  char name[] = "TestTeam";
  char entry[] = "GoodGuy,0,0,0,0,0,7,2,1,0,0,0";
  TeamType stack_team;
  // The switch is inside GoogleTest's macro.
  // GoogleTest death-test reporting uses its own bounded format arguments.
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(stack_team.ReadIniEntry(name, entry), "TEAMTYPE object error");
}

}  // namespace

}  // namespace td
