// Characterization tests for CDFileClass search-path resolution, pinned for
// the file I/O refactor (docs/FILE_IO_REFACTOR_PLAN.md).

#include "tech/cdfile.h"

#include <filesystem>
#include <fstream>
#include <ios>
#include <string>

#include "gtest/gtest.h"
#include "sdllib/file_access.h"
#include "tech/search_paths.h"

namespace {

void WriteFile(const std::filesystem::path& path, const std::string& bytes) {
  std::ofstream file(path, std::ios::binary);
  file.write(bytes.data(), std::ssize(bytes));
  ASSERT_TRUE(file.good()) << path;
}

class CDFileTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const std::string test_name =
        ::testing::UnitTest::GetInstance()->current_test_info()->name();
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() / ("cdfile_test_" + test_name);
    first_dir_ = root / "first";
    second_dir_ = root / "second";
    std::filesystem::create_directories(first_dir_);
    std::filesystem::create_directories(second_dir_);
    SearchPaths::Clear();
  }

  void TearDown() override {
    SearchPaths::Clear();
    std::filesystem::remove_all(first_dir_.parent_path());
  }

  static constexpr const char* kName = "CDFILE_TEST.BIN";

  [[nodiscard]] std::string in_first() const {
    return (first_dir_ / kName).string();
  }
  [[nodiscard]] std::string in_second() const {
    return (second_dir_ / kName).string();
  }
  [[nodiscard]] std::string first_dir() const { return first_dir_.string(); }
  [[nodiscard]] std::string second_dir() const { return second_dir_.string(); }

 private:
  std::filesystem::path first_dir_;
  std::filesystem::path second_dir_;
};

TEST_F(CDFileTest, NameFoundWithoutSearchingIsKept) {
  WriteFile(in_first(), "a");
  WriteFile(in_second(), "b");
  SearchPaths::Add(second_dir());

  const CDFileClass file(in_first());
  EXPECT_EQ(file.FileName(), in_first());
}

TEST_F(CDFileTest, SearchPathsAreTriedInRegistrationOrder) {
  WriteFile(in_first(), "a");
  WriteFile(in_second(), "b");

  SearchPaths::Add(first_dir() + ";" + second_dir());
  EXPECT_EQ(CDFileClass(kName).FileName(), in_first());

  SearchPaths::Clear();
  SearchPaths::Add(second_dir() + ";" + first_dir());
  EXPECT_EQ(CDFileClass(kName).FileName(), in_second());
}

TEST_F(CDFileTest, NameFoundNowhereStaysVerbatim) {
  SearchPaths::Add(first_dir());
  CDFileClass file(kName);
  EXPECT_EQ(file.FileName(), kName);
  EXPECT_FALSE(file.IsAvailable());
}

TEST_F(CDFileTest, DisabledSearchTakesNameVerbatim) {
  WriteFile(in_first(), "a");
  SearchPaths::Add(first_dir());

  CDFileClass file;
  file.SetSearchEnabled(false);
  file.SetName(kName);
  EXPECT_EQ(file.FileName(), kName);
}

TEST_F(CDFileTest, EmptyNameIsNotSearched) {
  SearchPaths::Add(first_dir());
  CDFileClass file;
  file.SetName("");
  EXPECT_TRUE(file.FileName().empty());
}

TEST_F(CDFileTest, ReadOpenSearchesButWriteOpenDoesNot) {
  WriteFile(in_first(), "a");
  SearchPaths::Add(first_dir());

  CDFileClass file;
  file.Open(kName, FileAccess::kRead);
  EXPECT_EQ(file.FileName(), in_first());
  file.Close();

  // A write never goes to a search path; it targets the name as given.
  file.Open(kName, FileAccess::kWrite);
  EXPECT_EQ(file.FileName(), kName);
  file.Close();
  std::filesystem::remove(kName);
}

}  // namespace
