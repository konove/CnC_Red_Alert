// Tests for the integer-handle file API in file_handles.cc.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>

#include "gtest/gtest.h"
#include "sdllib/file.h"
#include "sdllib/file_access.h"
#include "sdllib/wwstd.h"

namespace {

class FileHandlesTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const std::string test_name =
        ::testing::UnitTest::GetInstance()->current_test_info()->name();
    path_ = std::filesystem::temp_directory_path() /
            ("file_handles_test_" + test_name + ".bin");
    std::ofstream file(path_, std::ios::binary);
    file << "abcd";
  }

  void TearDown() override { std::filesystem::remove(path_); }

  [[nodiscard]] std::string path() const { return path_.string(); }

 private:
  std::filesystem::path path_;
};

TEST_F(FileHandlesTest, HandleReadsSeeksAndSizesAFile) {
  EXPECT_TRUE(FileExists(path()));
  const int handle = OpenFileHandle(path(), FileAccess::kRead);
  ASSERT_NE(handle, kInvalidHandle);
  EXPECT_EQ(FileHandleSize(handle), 4);
  EXPECT_EQ(SeekFileHandle(handle, 1, SEEK_SET), 1);

  char buffer[8] = {};
  EXPECT_EQ(ReadFileHandle(handle, buffer, 8), 3);
  EXPECT_EQ(std::string(buffer, 3), "bcd");
  CloseFileHandle(handle);
  EXPECT_EQ(FileHandleSize(handle), 0);
}

TEST_F(FileHandlesTest, MissingNameGivesNoHandle) {
  EXPECT_FALSE(FileExists(path() + ".missing"));
  EXPECT_EQ(OpenFileHandle(path() + ".missing", FileAccess::kRead),
            kInvalidHandle);
}

TEST_F(FileHandlesTest, InvalidHandlesAreIgnored) {
  char buffer[4] = {};
  // Each used to index the handle table unchecked; only -1 was rejected.
  for (const int handle : {kInvalidHandle, -2, 10, 1000}) {
    EXPECT_EQ(ReadFileHandle(handle, buffer, 4), 0) << handle;
    EXPECT_EQ(WriteFileHandle(handle, buffer, 4), 0) << handle;
    EXPECT_EQ(FileHandleSize(handle), 0) << handle;
    EXPECT_EQ(SeekFileHandle(handle, 0, SEEK_SET), 0) << handle;
    CloseFileHandle(handle);
  }
}

}  // namespace
