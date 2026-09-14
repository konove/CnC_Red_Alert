// Characterization tests for DiskFile: implicit open on Read/Write, the
// Bias() window, the lowercase-name retry, and Open() never reporting failure.
// They pin current behaviour so the file I/O refactor can prove equivalence
// (docs/FILE_IO_REFACTOR_PLAN.md).

#include "tech/disk_file.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>

#include "absl/strings/ascii.h"
#include "gtest/gtest.h"
#include "tech/file.h"

namespace {

void WriteFile(const std::filesystem::path& path, const std::string& bytes) {
  std::ofstream file(path, std::ios::binary);
  file.write(bytes.data(), std::ssize(bytes));
  ASSERT_TRUE(file.good()) << path;
}

std::string ReadFile(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(file), {}};
}

class DiskFileTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const std::string test_name =
        ::testing::UnitTest::GetInstance()->current_test_info()->name();
    path_ = std::filesystem::temp_directory_path() /
            ("disk_file_test_" + test_name + ".bin");
    WriteFile(path_, "xabcd");
  }

  void TearDown() override { std::filesystem::remove(path_); }

  [[nodiscard]] std::string path() const { return path_.string(); }

 private:
  std::filesystem::path path_;
};

TEST_F(DiskFileTest, ReadOpensAndClosesImplicitly) {
  DiskFile file(path());
  char buffer[8] = {};
  EXPECT_EQ(file.Read(buffer, 8), 5);
  EXPECT_EQ(std::string(buffer, 5), "xabcd");
  EXPECT_FALSE(file.IsOpen());
}

TEST_F(DiskFileTest, WriteOpensAndClosesImplicitly) {
  DiskFile file(path());
  EXPECT_EQ(file.Write("hi", 2), 2);
  EXPECT_FALSE(file.IsOpen());
  EXPECT_EQ(ReadFile(path()), "hi");
}

TEST_F(DiskFileTest, BiasWindowLimitsSizeSeekAndRead) {
  DiskFile file(path());
  file.Bias(1, 4);
  EXPECT_EQ(file.Size(), 4);

  file.Open();
  EXPECT_EQ(file.Seek(10, SeekOrigin::kBegin), 4);
  EXPECT_EQ(file.Seek(0, SeekOrigin::kBegin), 0);
  char buffer[8] = {};
  EXPECT_EQ(file.Read(buffer, 8), 4);
  EXPECT_EQ(std::string(buffer, 4), "abcd");
  EXPECT_EQ(file.Seek(-1, SeekOrigin::kEnd), 3);
  // A seek to before the start of the file is ignored: the position stays
  // where it was (a resident MixAwareFile clamps to 0 instead).
  EXPECT_EQ(file.Seek(-10, SeekOrigin::kCurrent), 3);
}

TEST_F(DiskFileTest, BiasAccumulatesAndSetNameClearsIt) {
  DiskFile file(path());
  file.Bias(1, 4);
  file.Bias(1, 2);
  EXPECT_EQ(file.bias_start(), 2);
  EXPECT_EQ(file.Size(), 2);

  file.SetName(path());
  EXPECT_EQ(file.bias_start(), 0);
  EXPECT_EQ(file.Size(), 5);
}

TEST_F(DiskFileTest, IsAvailableRetriesLowercaseNameAndRenames) {
  // The retry lowercases the whole name, so it only finds all-lowercase files.
  const std::filesystem::path lower =
      std::filesystem::temp_directory_path() / "disk_file_test_lowercase.bin";
  const std::filesystem::path upper =
      lower.parent_path() / absl::AsciiStrToUpper(lower.filename().string());
  WriteFile(lower, "x");
  if (std::filesystem::exists(upper)) {
    std::filesystem::remove(lower);
    GTEST_SKIP() << "case-insensitive filesystem";
  }

  DiskFile file(upper.string());
  EXPECT_TRUE(file.IsAvailable());
  EXPECT_EQ(file.FileName(), lower.string());

  // Open() alone does not retry.
  DiskFile direct(upper.string());
  direct.Open();
  EXPECT_FALSE(direct.IsOpen());
  std::filesystem::remove(lower);
}

TEST_F(DiskFileTest, OpenOfMissingFileReturnsTrueButIsNotOpen) {
  DiskFile file(path() + ".missing");
  EXPECT_TRUE(file.Open());
  EXPECT_FALSE(file.IsOpen());
  EXPECT_FALSE(file.IsAvailable());
}

}  // namespace
