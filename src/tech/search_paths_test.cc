// Tests for SearchPaths and FindExistingFile.

#include "tech/search_paths.h"

#include <filesystem>
#include <fstream>
#include <ios>
#include <optional>
#include <string>

#include "absl/strings/ascii.h"
#include "gtest/gtest.h"
#include "tech/disk_file.h"

// SearchPaths resolves "?:" through the game's CD probe, declared only in
// search_paths.cc. The tests set what it reports.
namespace {
int cd_index = -1;
}  // namespace
// NOLINTBEGIN(misc-use-internal-linkage): satisfies search_paths.cc's extern.
int Get_CD_Index(int cd_drive, int timeout);
int Get_CD_Index(int /*cd_drive*/, int /*timeout*/) { return cd_index; }
// NOLINTEND(misc-use-internal-linkage)

namespace {

void WriteFile(const std::filesystem::path& path, const std::string& bytes) {
  std::ofstream file(path, std::ios::binary);
  file.write(bytes.data(), std::ssize(bytes));
  ASSERT_TRUE(file.good()) << path;
}

class SearchPathsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const std::string test_name =
        ::testing::UnitTest::GetInstance()->current_test_info()->name();
    // Lowercased because the case-insensitive lookup lowercases the whole
    // path, directories included.
    root_ = std::filesystem::temp_directory_path() /
            absl::AsciiStrToLower("search_paths_test_" + test_name);
    first_ = root_ / "first";
    second_ = root_ / "second";
    std::filesystem::create_directories(first_);
    std::filesystem::create_directories(second_);
    SearchPaths::Clear();
    cd_index = -1;
  }

  void TearDown() override {
    SearchPaths::Clear();
    std::filesystem::remove_all(root_);
  }

  static constexpr const char* kName = "SEARCH_PATHS_TEST.BIN";

  [[nodiscard]] std::string first() const { return first_.string(); }
  [[nodiscard]] std::string second() const { return second_.string(); }
  [[nodiscard]] std::string in_first() const {
    return (first_ / kName).string();
  }
  [[nodiscard]] std::string in_second() const {
    return (second_ / kName).string();
  }

 private:
  std::filesystem::path root_;
  std::filesystem::path first_;
  std::filesystem::path second_;
};

TEST_F(SearchPathsTest, ExistingNameResolvesToItselfBeforeAnyDirectory) {
  WriteFile(in_first(), "a");
  WriteFile(in_second(), "b");
  SearchPaths::Add(second());
  EXPECT_EQ(SearchPaths::Resolve(in_first()), in_first());
}

TEST_F(SearchPathsTest, DirectoriesAreSearchedInTheOrderAdded) {
  WriteFile(in_first(), "a");
  WriteFile(in_second(), "b");

  EXPECT_EQ(SearchPaths::Add(first() + ";" + second()), 0);
  EXPECT_EQ(SearchPaths::Resolve(kName), in_first());

  SearchPaths::Clear();
  SearchPaths::Add(second() + ";" + first());
  EXPECT_EQ(SearchPaths::Resolve(kName), in_second());
}

TEST_F(SearchPathsTest, MissingAndEmptyNamesResolveToNothing) {
  SearchPaths::Add(first());
  EXPECT_EQ(SearchPaths::Resolve(kName), std::nullopt);
  EXPECT_EQ(SearchPaths::Resolve(""), std::nullopt);
}

TEST_F(SearchPathsTest, LowercaseFileIsFoundUnderItsUppercaseName) {
  const std::filesystem::path lower =
      std::filesystem::path(first()) / "search_paths_test_lower.bin";
  WriteFile(lower, "x");
  const std::string upper = absl::AsciiStrToUpper(lower.filename().string());
  if (std::filesystem::exists(std::filesystem::path(first()) / upper)) {
    GTEST_SKIP() << "case-insensitive filesystem";
  }
  SearchPaths::Add(first());
  EXPECT_EQ(SearchPaths::Resolve(upper), lower.string());
}

TEST_F(SearchPathsTest, CdPlaceholderNeedsARecognizedCd) {
  SearchPaths::SetCdDrive(3);
  EXPECT_EQ(SearchPaths::Add("?:\\"), 1);
  EXPECT_FALSE(SearchPaths::HasAny());

  cd_index = 0;
  SearchPaths::Refresh();
  EXPECT_TRUE(SearchPaths::HasAny());
  EXPECT_EQ(SearchPaths::current_cd_drive(), 3);
  EXPECT_EQ(SearchPaths::last_cd_drive(), 0);
}

TEST_F(SearchPathsTest, RefreshRestoresClearedDirectories) {
  WriteFile(in_first(), "a");
  SearchPaths::Add(first());
  SearchPaths::Clear();
  EXPECT_EQ(SearchPaths::Resolve(kName), std::nullopt);
  SearchPaths::Refresh();
  EXPECT_EQ(SearchPaths::Resolve(kName), in_first());
}

TEST(FindExistingFileTest, ReturnsThePathThatExists) {
  const std::filesystem::path path =
      std::filesystem::temp_directory_path() / "find_existing_file_test.bin";
  WriteFile(path, "x");
  EXPECT_EQ(FindExistingFile(path.string()), path.string());
  EXPECT_EQ(FindExistingFile(path.string() + ".missing"), std::nullopt);
  std::filesystem::remove(path);
}

}  // namespace
