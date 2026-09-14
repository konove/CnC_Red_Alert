// Tests for CCFileClass over cached and uncached mixfiles, and for the
// integer-handle file API built on it.

#include "ra/ccfile.h"

#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "ra/conquer.h"
#include "ra/externs.h"
#include "ra/jshell.h"
#include "ra/startup.h"
#include "sdllib/file.h"
#include "sdllib/file_access.h"
#include "tech/crc.h"
#include "tech/rawfile.h"
#include "tech/wwfile.h"

// The real definitions live in the game, which would drag all of it in. The
// CD is always present, so CCFileClass::Error() always returns.
int RequiredCD = -1;
bool Force_CD_Available(int /*cd_desired*/) { return true; }
int Get_CD_Index(int /*cd_drive*/, int /*timeout*/) { return -1; }
void Emergency_Exit(int code) { std::exit(code); }
void* Load_Alloc_Data(FileClass& /*file*/) { return nullptr; }

namespace {

// Name of the one file packed in the test mixfile. Unusual enough that no loose
// file by this name sits in the working directory.
constexpr const char* kPackedName = "CCFILE_TEST.BIN";

void PutInt16(std::vector<char>& out, int value) {
  out.push_back(static_cast<char>(value & 0xff));
  out.push_back(static_cast<char>((value >> 8) & 0xff));
}

void PutInt32(std::vector<char>& out, int64_t value) {
  for (int shift = 0; shift < 32; shift += 8) {
    out.push_back(static_cast<char>((value >> shift) & 0xff));
  }
}

// A plain-format mixfile whose data section is "xabcd", with kPackedName
// covering "abcd". The leading byte makes the embedded file start past the
// data section, so a wrong bias reads 'x'.
std::vector<char> MixImage() {
  std::vector<char> out;
  PutInt16(out, 1);
  PutInt32(out, 5);
  PutInt32(out, std::bit_cast<int32_t>(CrcEngine::Compute(kPackedName)));
  PutInt32(out, 1);
  PutInt32(out, 4);
  for (const char byte : std::string("xabcd")) {
    out.push_back(byte);
  }
  return out;
}

void WriteFile(const std::filesystem::path& path,
               const std::vector<char>& bytes) {
  std::ofstream file(path, std::ios::binary);
  file.write(bytes.data(), std::ssize(bytes));
  ASSERT_TRUE(file.good()) << path;
}

class CCFileTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const std::string test_name =
        ::testing::UnitTest::GetInstance()->current_test_info()->name();
    mix_path_ = std::filesystem::temp_directory_path() /
                ("ccfile_test_" + test_name + ".mix");
    loose_path_ = std::filesystem::temp_directory_path() /
                  ("ccfile_test_" + test_name + ".txt");
    WriteFile(mix_path_, MixImage());
    ASSERT_NE(MFCD::Register(mix_path_.string()), nullptr);
  }

  void TearDown() override {
    MFCD::Free_All();
    std::filesystem::remove(mix_path_);
    std::filesystem::remove(loose_path_);
  }

  // Cache() finds mixfiles by basename, not by the path they were registered
  // under.
  void CacheMixfile() const {
    ASSERT_TRUE(MFCD::Cache(mix_path_.filename().string()));
  }

  [[nodiscard]] const std::filesystem::path& loose_path() const {
    return loose_path_;
  }

 private:
  std::filesystem::path mix_path_;
  std::filesystem::path loose_path_;
};

TEST_F(CCFileTest, CachedFileReadsAndSeeksWithinItsImage) {
  CacheMixfile();
  CCFileClass file(kPackedName);
  ASSERT_TRUE(file.Open());
  EXPECT_TRUE(file.Is_Resident());
  EXPECT_EQ(file.Size(), 4);

  char buffer[8] = {};
  EXPECT_EQ(file.Read(buffer, 2), 2);
  EXPECT_EQ(std::string(buffer, 2), "ab");

  EXPECT_EQ(file.Seek(10, SEEK_SET), 4);
  EXPECT_EQ(file.Read(buffer, 1), 0);
  EXPECT_EQ(file.Seek(-10, SEEK_CUR), 0);
  EXPECT_EQ(file.Seek(-1, SEEK_END), 3);
  EXPECT_EQ(file.Read(buffer, 8), 1);
  EXPECT_EQ(buffer[0], 'd');
}

TEST_F(CCFileTest, WriteToCachedFileWritesNothing) {
  CacheMixfile();
  CCFileClass file(kPackedName);
  ASSERT_TRUE(file.Open());
  ASSERT_TRUE(file.Is_Resident());

  // Used to fall through to the base class and write through a null handle.
  EXPECT_EQ(file.Write("zz", 2), 0);
}

TEST_F(CCFileTest, UncachedFileReadsOnlyItsBytesOfTheMixfile) {
  CCFileClass file(kPackedName);
  ASSERT_TRUE(file.Open());
  EXPECT_FALSE(file.Is_Resident());
  EXPECT_EQ(file.Size(), 4);

  char buffer[8] = {};
  EXPECT_EQ(file.Read(buffer, 8), 4);
  EXPECT_EQ(std::string(buffer, 4), "abcd");
}

TEST_F(CCFileTest, SizeOfUnopenedPackedFileIsItsOwnSize) {
  CCFileClass file(kPackedName);
  EXPECT_EQ(file.Size(), 4);
}

TEST_F(CCFileTest, DeleteRefusesPackedFile) {
  CCFileClass file(kPackedName);
  EXPECT_FALSE(file.Delete());
  EXPECT_TRUE(file.Is_Available());
}

TEST_F(CCFileTest, DeleteRemovesLooseFile) {
  WriteFile(loose_path(), {'h', 'i'});
  CCFileClass file(loose_path().c_str());
  EXPECT_TRUE(file.Delete());
  EXPECT_FALSE(std::filesystem::exists(loose_path()));
}

TEST_F(CCFileTest, HandleApiReadsPackedFile) {
  CacheMixfile();
  const int handle = Open_File(kPackedName, FileAccess::kRead);
  ASSERT_NE(handle, WWERROR);
  EXPECT_EQ(File_Size(handle), 4);
  EXPECT_EQ(Seek_File(handle, 1, SEEK_SET), 1);

  char buffer[8] = {};
  EXPECT_EQ(Read_File(handle, buffer, 8), 3);
  EXPECT_EQ(std::string(buffer, 3), "bcd");
  Close_File(handle);
  EXPECT_EQ(File_Size(handle), 0);
}

TEST_F(CCFileTest, HandleApiIgnoresInvalidHandles) {
  char buffer[4] = {};
  // Each used to index Handles[] unchecked; only WWERROR (-1) was rejected.
  for (const int handle : {WWERROR, -2, 10, 1000}) {
    EXPECT_EQ(Read_File(handle, buffer, 4), 0) << handle;
    EXPECT_EQ(Write_File(handle, buffer, 4), 0) << handle;
    EXPECT_EQ(File_Size(handle), 0) << handle;
    EXPECT_EQ(Seek_File(handle, 0, SEEK_SET), 0) << handle;
    Close_File(handle);
  }
}

}  // namespace
