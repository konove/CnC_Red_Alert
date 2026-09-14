// Tests for the low-level IO_* file routines.

#include "sdllib/file.h"

#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>

#include "absl/base/attributes.h"
#include "gtest/gtest.h"
#include "sdllib/file_access.h"

namespace {

std::string ReadAll(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(file), {}};
}

class IoOpenFileTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const std::string test_name =
        ::testing::UnitTest::GetInstance()->current_test_info()->name();
    path_ = std::filesystem::temp_directory_path() /
            ("sdllib_file_test_" + test_name + ".bin");
    std::filesystem::remove(path_);
  }

  void TearDown() override { std::filesystem::remove(path_); }

  [[nodiscard]] const std::filesystem::path& path() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return path_;
  }

 private:
  std::filesystem::path path_;
};

TEST_F(IoOpenFileTest, ReadWriteKeepsExistingContents) {
  {
    std::ofstream file(path(), std::ios::binary);
    file << "hello";
  }

  void* const handle = IO_Open_File(path().c_str(), FileAccess::kReadWrite);
  ASSERT_NE(handle, nullptr);
  // Used to open with "w+b", which truncated the file to nothing.
  EXPECT_EQ(IO_Seek_File(handle, 0, SEEK_END), 5);
  size_t written = 0;
  ASSERT_TRUE(IO_Write_File(handle, "!", 1, written));
  IO_Close_File(handle);

  EXPECT_EQ(ReadAll(path()), "hello!");
}

TEST_F(IoOpenFileTest, ReadWriteCreatesMissingFile) {
  void* const handle = IO_Open_File(path().c_str(), FileAccess::kReadWrite);
  ASSERT_NE(handle, nullptr);
  IO_Close_File(handle);
  EXPECT_TRUE(std::filesystem::exists(path()));
}

}  // namespace
