#include "engine/platform/file_system.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "absl/base/attributes.h"
#include "gtest/gtest.h"

TEST(MatchesPatternTest, StarAndQuestionMark) {
  EXPECT_TRUE(MatchesPattern("SC*.MIX", "SC01.MIX"));
  EXPECT_TRUE(MatchesPattern("SC*.MIX", "SC.MIX"));
  EXPECT_TRUE(MatchesPattern("SAVEGAME.*", "SAVEGAME.001"));
  EXPECT_TRUE(MatchesPattern("*.PKT", "MISSIONS.PKT"));
  EXPECT_TRUE(MatchesPattern("A?C", "ABC"));
  EXPECT_FALSE(MatchesPattern("A?C", "AC"));
  EXPECT_FALSE(MatchesPattern("SC*.MIX", "SC01.MIXX"));
  EXPECT_FALSE(MatchesPattern("SC*.MIX", "XSC01.MIX"));
  EXPECT_TRUE(MatchesPattern("*", ""));
  EXPECT_FALSE(MatchesPattern("", "A"));
}

TEST(MatchesPatternTest, MatchesPatternIgnoresCase) {
  EXPECT_TRUE(MatchesPattern("SC*.MIX", "sc01.mix"));
  EXPECT_TRUE(MatchesPattern("savegame.*", "SAVEGAME.001"));
}

class FindFilesTest : public ::testing::Test {
 protected:
  void SetUp() override {
    dir_ =
        std::filesystem::temp_directory_path() /
        ("file_system_test_" +
         std::string(
             ::testing::UnitTest::GetInstance()->current_test_info()->name()));
    std::filesystem::remove_all(dir_);
    std::filesystem::create_directory(dir_);
  }
  void TearDown() override { std::filesystem::remove_all(dir_); }
  void Touch(const std::string& name) const {
    std::ofstream(dir_ / name) << "x";
  }
  [[nodiscard]] const std::filesystem::path& dir() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return dir_;
  }

 private:
  std::filesystem::path dir_;
};

TEST_F(FindFilesTest, FindFilesMatchesLowercaseNames) {
  Touch("sc02.mix");
  Touch("SC01.MIX");
  Touch("SS01.MIX");
  const std::vector<FoundFile> found = FindFiles("SC*.MIX", dir());
  ASSERT_EQ(found.size(), 2U);
  EXPECT_EQ(found.at(0).name, "SC01.MIX");  // sorted by name
  EXPECT_EQ(found.at(1).name, "sc02.mix");
}

TEST_F(FindFilesTest, FindFilesSkipsDirectories) {
  std::filesystem::create_directory(dir() / "SAVEGAME.002");
  Touch("SAVEGAME.001");
  const std::vector<FoundFile> found = FindFiles("SAVEGAME.*", dir());
  ASSERT_EQ(found.size(), 1U);
  EXPECT_EQ(found.at(0).name, "SAVEGAME.001");
}

TEST_F(FindFilesTest, ModifiedIsSecondsSinceTheEpoch) {
  Touch("A.BIN");
  const std::vector<FoundFile> found = FindFiles("A.BIN", dir());
  ASSERT_EQ(found.size(), 1U);
  const int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
  EXPECT_NEAR(static_cast<double>(found.at(0).modified),
              static_cast<double>(now), 60.0);
}

TEST(FreeDiskSpaceTest, IsPositive) { EXPECT_GT(FreeDiskSpace(), 0); }
