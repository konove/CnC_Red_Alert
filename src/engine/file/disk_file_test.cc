// Tests for FindExistingFile's lowercase-name retry and OpenDiskFile, the
// free functions that replace the DiskFile class.

#include "engine/file/disk_file.h"

#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <memory>
#include <span>
#include <string>
#include <string_view>

#include "absl/strings/ascii.h"
#include "engine/file/disk_stream.h"
#include "engine/file/file_access.h"
#include "gtest/gtest.h"

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

// FindExistingFile lowercases the whole path it is given, so a test that
// relies on its retry finding a sibling file needs the directory part of the
// path to already be all lowercase (true of temp_directory_path() on Linux,
// not guaranteed elsewhere, e.g. a TMPDIR with mixed-case components).
bool DirectoryIsAllLowercase(const std::filesystem::path& directory) {
  const std::string generic = directory.generic_string();
  return generic == absl::AsciiStrToLower(generic);
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

TEST_F(DiskFileTest, FindExistingFileRetriesLowercaseName) {
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

  EXPECT_EQ(FindExistingFile(upper.string()), lower.string());

  // The exact name still wins when it exists too.
  EXPECT_EQ(FindExistingFile(lower.string()), lower.string());
  std::filesystem::remove(lower);
}

TEST_F(DiskFileTest, OpenDiskFileWritesTheExistingLowercaseFile) {
  // FindExistingFile lowercases the whole path, so only an all-lowercase
  // file is found; temp_directory_path() is lowercase on Linux.
  const std::filesystem::path lower = std::filesystem::temp_directory_path() /
                                      "disk_file_test_lowercase_write.bin";
  if (!DirectoryIsAllLowercase(lower.parent_path())) {
    GTEST_SKIP() << "TMPDIR has upper-case components";
  }
  const std::filesystem::path upper =
      lower.parent_path() / absl::AsciiStrToUpper(lower.filename().string());
  WriteFile(lower, "old");
  if (std::filesystem::exists(upper)) {
    std::filesystem::remove(lower);
    GTEST_SKIP() << "case-insensitive filesystem";
  }
  {
    const std::unique_ptr<DiskStream> out =
        OpenDiskFile(upper.string(), FileAccess::kWrite);
    ASSERT_NE(out, nullptr);
    out->Write(std::as_bytes(std::span(std::string_view("new"))));
  }
  EXPECT_FALSE(std::filesystem::exists(upper));
  EXPECT_EQ(ReadFile(lower), "new");
  std::filesystem::remove(lower);
}

TEST_F(DiskFileTest, RawDiskStreamOpenDoesNotFallBackToLowercaseTwin) {
  // Only FindExistingFile/OpenDiskFile retry under the lowercased name;
  // DiskStream::Open by itself must fail rather than silently reading a
  // different file than the one it was asked to open.
  const std::filesystem::path lower = std::filesystem::temp_directory_path() /
                                      "disk_file_test_lowercase_raw.bin";
  if (!DirectoryIsAllLowercase(lower.parent_path())) {
    GTEST_SKIP() << "TMPDIR has upper-case components";
  }
  const std::filesystem::path upper =
      lower.parent_path() / absl::AsciiStrToUpper(lower.filename().string());
  WriteFile(lower, "x");
  if (std::filesystem::exists(upper)) {
    std::filesystem::remove(lower);
    GTEST_SKIP() << "case-insensitive filesystem";
  }
  EXPECT_EQ(DiskStream::Open(upper.string(), FileAccess::kRead), nullptr);
  std::filesystem::remove(lower);
}

TEST_F(DiskFileTest, OpenDiskFileForReadOfMissingFileIsNull) {
  EXPECT_EQ(OpenDiskFile(path() + ".missing"), nullptr);
  const std::unique_ptr<DiskStream> created =
      OpenDiskFile(path() + ".new", FileAccess::kWrite);
  EXPECT_NE(created, nullptr);
  std::filesystem::remove(path() + ".new");
}

}  // namespace
