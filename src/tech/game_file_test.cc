// Tests for GameFile over loose files and cached, uncached and nested
// mixfiles.

#include "tech/game_file.h"

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
#include "tech/crc.h"
#include "tech/file.h"
#include "tech/mixfile.h"
#include "tech/search_paths.h"

namespace {

using MFCD = MixFileClass<GameFile>;

// Names of the file packed in the test mixfile and of the mixfile packed inside
// the outer one for the nesting tests. Unusual enough that no loose file by
// these names sits in the working directory.
constexpr const char* kPackedName = "GAME_FILE_TEST.BIN";
constexpr const char* kInnerName = "GAME_FILE_TEST_INNER.MIX";

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

class GameFileTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const std::string test_name =
        ::testing::UnitTest::GetInstance()->current_test_info()->name();
    mix_path_ = std::filesystem::temp_directory_path() /
                ("game_file_test_" + test_name + ".mix");
    loose_path_ = std::filesystem::temp_directory_path() /
                  ("game_file_test_" + test_name + ".txt");
    search_dir_ = std::filesystem::temp_directory_path() /
                  ("game_file_test_" + test_name + ".dir");
    WriteFile(mix_path_, MixImage());
    ASSERT_NE(MFCD::Register(mix_path_.string()), nullptr);
  }

  void TearDown() override {
    MFCD::Free_All();
    SearchPaths::Clear();
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
    SearchPaths::Add(search_dir_.string());
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

TEST_F(GameFileTest, CachedFileReadsAndSeeksWithinItsImage) {
  CacheMixfile();
  GameFile file(kPackedName);
  ASSERT_TRUE(file.Open());
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

TEST_F(GameFileTest, WriteToCachedFileWritesNothing) {
  CacheMixfile();
  GameFile file(kPackedName);
  ASSERT_TRUE(file.Open());

  // Used to fall through to the base class and write through a null handle.
  EXPECT_EQ(file.Write("zz", 2), 0);
}

TEST_F(GameFileTest, UncachedFileReadsOnlyItsBytesOfTheMixfile) {
  GameFile file(kPackedName);
  ASSERT_TRUE(file.Open());
  EXPECT_EQ(file.Size(), 4);

  char buffer[8] = {};
  EXPECT_EQ(file.Read(buffer, 8), 4);
  EXPECT_EQ(std::string(buffer, 4), "abcd");
}

TEST_F(GameFileTest, SizeOfUnopenedPackedFileIsItsOwnSize) {
  GameFile file(kPackedName);
  EXPECT_EQ(file.Size(), 4);
}

TEST_F(GameFileTest, DeleteRefusesPackedFile) {
  GameFile file(kPackedName);
  EXPECT_FALSE(file.Delete());
  EXPECT_TRUE(file.IsAvailable());
}

TEST_F(GameFileTest, DeleteRemovesLooseFile) {
  WriteFile(loose_path(), {'h', 'i'});
  GameFile file(loose_path().string());
  EXPECT_TRUE(file.Delete());
  EXPECT_FALSE(std::filesystem::exists(loose_path()));
}

TEST_F(GameFileTest, UncachedMixfileInsideUncachedMixfileReadsItsBytes) {
  RegisterNestedMixfiles();
  GameFile file(kPackedName);
  ASSERT_TRUE(file.Open());
  EXPECT_EQ(file.Size(), 4);

  char buffer[8] = {};
  EXPECT_EQ(file.Read(buffer, 8), 4);
  EXPECT_EQ(std::string(buffer, 4), "abcd");
}

TEST_F(GameFileTest, CachedMixfileInsideUncachedMixfileReadsItsBytes) {
  RegisterNestedMixfiles();
  ASSERT_TRUE(MFCD::Cache(kInnerName));
  GameFile file(kPackedName);
  ASSERT_TRUE(file.Open());

  char buffer[8] = {};
  EXPECT_EQ(file.Read(buffer, 8), 4);
  EXPECT_EQ(std::string(buffer, 4), "abcd");
}

TEST_F(GameFileTest, LooseFileOnSearchPathOverridesPackedCopy) {
  CacheMixfile();
  WriteLooseCopy("LOOSE");
  GameFile file(kPackedName);
  ASSERT_TRUE(file.Open());
  EXPECT_EQ(file.Size(), 5);

  char buffer[8] = {};
  EXPECT_EQ(file.Read(buffer, 8), 5);
  EXPECT_EQ(std::string(buffer, 5), "LOOSE");
}

TEST_F(GameFileTest, OpenOfNameFoundNowhereFails) {
  GameFile file("GAME_FILE_TEST_MISSING.BIN");
  EXPECT_FALSE(file.IsAvailable());
  EXPECT_FALSE(file.Open());
  EXPECT_EQ(file.Size(), 0);
  char buffer[4] = {};
  EXPECT_EQ(file.Read(buffer, 4), 0);
}

}  // namespace
