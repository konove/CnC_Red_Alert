// Tests for MixAwareFile over cached and uncached mixfiles, and for the
// integer-handle file API built on it.

#include "ra/mix_aware_file.h"

#include <bit>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <string_view>
#include <vector>

#include "absl/base/attributes.h"
#include "gtest/gtest.h"
#include "ra/externs.h"
#include "sdllib/file.h"
#include "sdllib/file_access.h"
#include "tech/crc.h"
#include "tech/disk_file.h"
#include "tech/file.h"

// The real definition lives in the game, which would drag all of it in. No
// CD drive is ever current here, so it is never called.
int Get_CD_Index(int /*cd_drive*/, int /*timeout*/) { return -1; }

namespace {

// Names of the file packed in the test mixfile and of the mixfile packed inside
// the outer one for the nesting tests. Unusual enough that no loose file by
// these names sits in the working directory.
constexpr const char* kPackedName = "MIX_AWARE_FILE_TEST.BIN";
constexpr const char* kInnerName = "MIX_AWARE_FILE_TEST_INNER.MIX";

void PutInt16(std::vector<char>& out, int value) {
  out.push_back(static_cast<char>(value & 0xff));
  out.push_back(static_cast<char>((value >> 8) & 0xff));
}

void PutInt32(std::vector<char>& out, int64_t value) {
  for (int shift = 0; shift < 32; shift += 8) {
    out.push_back(static_cast<char>((value >> shift) & 0xff));
  }
}

// A plain-format mixfile holding one file, name, whose bytes are payload. The
// data section starts with a leading 'x' so that a wrong bias reads it instead
// of the file.
std::vector<char> MixImageHolding(const std::string_view name,
                                  const std::vector<char>& payload) {
  std::vector<char> out;
  PutInt16(out, 1);
  PutInt32(out, std::ssize(payload) + 1);
  PutInt32(out, std::bit_cast<int32_t>(CrcEngine::Compute(name)));
  PutInt32(out, 1);
  PutInt32(out, std::ssize(payload));
  out.push_back('x');
  out.insert(out.end(), payload.begin(), payload.end());
  return out;
}

// The test mixfile: kPackedName covering "abcd".
std::vector<char> MixImage() {
  return MixImageHolding(kPackedName, {'a', 'b', 'c', 'd'});
}

void WriteFile(const std::filesystem::path& path,
               const std::vector<char>& bytes) {
  std::ofstream file(path, std::ios::binary);
  file.write(bytes.data(), std::ssize(bytes));
  ASSERT_TRUE(file.good()) << path;
}

class MixAwareFileTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const std::string test_name =
        ::testing::UnitTest::GetInstance()->current_test_info()->name();
    mix_path_ = std::filesystem::temp_directory_path() /
                ("mix_aware_file_test_" + test_name + ".mix");
    loose_path_ = std::filesystem::temp_directory_path() /
                  ("mix_aware_file_test_" + test_name + ".txt");
    search_dir_ = std::filesystem::temp_directory_path() /
                  ("mix_aware_file_test_" + test_name + ".dir");
    WriteFile(mix_path_, MixImage());
    ASSERT_NE(MFCD::Register(mix_path_.string()), nullptr);
  }

  void TearDown() override {
    MFCD::Free_All();
    MixAwareFile::ClearSearchPaths();
    std::filesystem::remove(mix_path_);
    std::filesystem::remove(loose_path_);
    std::filesystem::remove_all(search_dir_);
  }

  // Cache() finds mixfiles by basename, not by the path they were registered
  // under.
  void CacheMixfile() const {
    ASSERT_TRUE(MFCD::Cache(mix_path_.filename().string()));
  }

  // Replaces the flat fixture with the test mixfile packed inside an outer
  // mixfile on disk, registered as kInnerName through the outer one. Neither
  // is cached afterwards.
  void RegisterNestedMixfiles() {
    MFCD::Free_All();
    WriteFile(mix_path_, MixImageHolding(kInnerName, MixImage()));
    ASSERT_NE(MFCD::Register(mix_path_.string()), nullptr);
    ASSERT_NE(MFCD::Register(kInnerName), nullptr);
  }

  // Puts a loose copy of kPackedName holding bytes on the search path.
  void WriteLooseCopy(const std::string& bytes) {
    std::filesystem::create_directories(search_dir_);
    WriteFile(search_dir_ / kPackedName, {bytes.begin(), bytes.end()});
    MixAwareFile::AddSearchPaths(search_dir_.string());
  }

  [[nodiscard]] const std::filesystem::path& loose_path() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return loose_path_;
  }

 private:
  std::filesystem::path mix_path_;
  std::filesystem::path loose_path_;
  std::filesystem::path search_dir_;
};

TEST_F(MixAwareFileTest, CachedFileReadsAndSeeksWithinItsImage) {
  CacheMixfile();
  MixAwareFile file(kPackedName);
  ASSERT_TRUE(file.Open());
  EXPECT_TRUE(file.IsResident());
  EXPECT_EQ(file.Size(), 4);

  char buffer[8] = {};
  EXPECT_EQ(file.Read(buffer, 2), 2);
  EXPECT_EQ(std::string(buffer, 2), "ab");

  EXPECT_EQ(file.Seek(10, SeekOrigin::kBegin), 4);
  EXPECT_EQ(file.Read(buffer, 1), 0);
  EXPECT_EQ(file.Seek(-10, SeekOrigin::kCurrent), 0);
  EXPECT_EQ(file.Seek(-1, SeekOrigin::kEnd), 3);
  EXPECT_EQ(file.Read(buffer, 8), 1);
  EXPECT_EQ(buffer[0], 'd');
}

TEST_F(MixAwareFileTest, WriteToCachedFileWritesNothing) {
  CacheMixfile();
  MixAwareFile file(kPackedName);
  ASSERT_TRUE(file.Open());
  ASSERT_TRUE(file.IsResident());

  // Used to fall through to the base class and write through a null handle.
  EXPECT_EQ(file.Write("zz", 2), 0);
}

TEST_F(MixAwareFileTest, UncachedFileReadsOnlyItsBytesOfTheMixfile) {
  MixAwareFile file(kPackedName);
  ASSERT_TRUE(file.Open());
  EXPECT_FALSE(file.IsResident());
  EXPECT_EQ(file.Size(), 4);

  char buffer[8] = {};
  EXPECT_EQ(file.Read(buffer, 8), 4);
  EXPECT_EQ(std::string(buffer, 4), "abcd");
}

TEST_F(MixAwareFileTest, SizeOfUnopenedPackedFileIsItsOwnSize) {
  MixAwareFile file(kPackedName);
  EXPECT_EQ(file.Size(), 4);
}

TEST_F(MixAwareFileTest, DeleteRefusesPackedFile) {
  MixAwareFile file(kPackedName);
  EXPECT_FALSE(file.Delete());
  EXPECT_TRUE(file.IsAvailable());
}

TEST_F(MixAwareFileTest, DeleteRemovesLooseFile) {
  WriteFile(loose_path(), {'h', 'i'});
  MixAwareFile file(loose_path().string());
  EXPECT_TRUE(file.Delete());
  EXPECT_FALSE(std::filesystem::exists(loose_path()));
}

TEST_F(MixAwareFileTest, UncachedMixfileInsideUncachedMixfileReadsItsBytes) {
  RegisterNestedMixfiles();
  MixAwareFile file(kPackedName);
  ASSERT_TRUE(file.Open());
  EXPECT_FALSE(file.IsResident());
  EXPECT_EQ(file.Size(), 4);

  char buffer[8] = {};
  EXPECT_EQ(file.Read(buffer, 8), 4);
  EXPECT_EQ(std::string(buffer, 4), "abcd");
}

TEST_F(MixAwareFileTest, CachedMixfileInsideUncachedMixfileReadsItsBytes) {
  RegisterNestedMixfiles();
  ASSERT_TRUE(MFCD::Cache(kInnerName));
  MixAwareFile file(kPackedName);
  ASSERT_TRUE(file.Open());
  EXPECT_TRUE(file.IsResident());

  char buffer[8] = {};
  EXPECT_EQ(file.Read(buffer, 8), 4);
  EXPECT_EQ(std::string(buffer, 4), "abcd");
}

TEST_F(MixAwareFileTest, LooseFileOnSearchPathOverridesPackedCopy) {
  CacheMixfile();
  WriteLooseCopy("LOOSE");
  MixAwareFile file(kPackedName);
  ASSERT_TRUE(file.Open());
  EXPECT_FALSE(file.IsResident());
  EXPECT_EQ(file.Size(), 5);

  char buffer[8] = {};
  EXPECT_EQ(file.Read(buffer, 8), 5);
  EXPECT_EQ(std::string(buffer, 5), "LOOSE");
}

TEST_F(MixAwareFileTest, HandleApiOnMissingNameHandsOutAnUnopenedSlot) {
  // Open() never reports failure today, so the caller gets a slot whose reads
  // return nothing. The refactor makes this return WWERROR instead.
  const int handle =
      OpenFileHandle("MIX_AWARE_FILE_TEST_MISSING.BIN", FileAccess::kRead);
  EXPECT_NE(handle, WWERROR);
  EXPECT_EQ(FileHandleSize(handle), 0);
  char buffer[4] = {};
  EXPECT_EQ(ReadFileHandle(handle, buffer, 4), 0);
  CloseFileHandle(handle);
}

TEST_F(MixAwareFileTest, HandleApiReadsPackedFile) {
  CacheMixfile();
  const int handle = OpenFileHandle(kPackedName, FileAccess::kRead);
  ASSERT_NE(handle, WWERROR);
  EXPECT_EQ(FileHandleSize(handle), 4);
  EXPECT_EQ(SeekFileHandle(handle, 1, SEEK_SET), 1);

  char buffer[8] = {};
  EXPECT_EQ(ReadFileHandle(handle, buffer, 8), 3);
  EXPECT_EQ(std::string(buffer, 3), "bcd");
  CloseFileHandle(handle);
  EXPECT_EQ(FileHandleSize(handle), 0);
}

TEST_F(MixAwareFileTest, HandleApiIgnoresInvalidHandles) {
  char buffer[4] = {};
  // Each used to index the handle table unchecked; only WWERROR (-1) was
  // rejected.
  for (const int handle : {WWERROR, -2, 10, 1000}) {
    EXPECT_EQ(ReadFileHandle(handle, buffer, 4), 0) << handle;
    EXPECT_EQ(WriteFileHandle(handle, buffer, 4), 0) << handle;
    EXPECT_EQ(FileHandleSize(handle), 0) << handle;
    EXPECT_EQ(SeekFileHandle(handle, 0, SEEK_SET), 0) << handle;
    CloseFileHandle(handle);
  }
}

}  // namespace
