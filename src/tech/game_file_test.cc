// Tests for OpenGameFile/GameFileExists/GameFileSize/DeleteGameFile, the
// free functions that replace the GameFile class, over loose files and
// cached, uncached and nested mixfiles.

#include "tech/game_file.h"

#include <bit>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/base/attributes.h"
#include "absl/strings/ascii.h"
#include "base/seek_origin.h"
#include "gtest/gtest.h"
#include "sdllib/file_access.h"
#include "tech/byte_stream.h"
#include "tech/crc.h"
#include "tech/mix_archive.h"
#include "tech/search_paths.h"

namespace {

// Names of the file packed in the test mixfile and of the mixfile packed inside
// the outer one for the nesting tests. Unusual enough that no loose file by
// these names sits in the working directory.
constexpr const char* kPackedName = "GAME_FILE_TEST.BIN";
constexpr const char* kInnerName = "GAME_FILE_TEST_INNER.MIX";

// LLVM 23 mistakes element invalidation for invalidating the vector reference;
// no element reference or iterator is retained across these appends.
// NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-invalidation)
void PutInt16(std::vector<char>& out, int value) {
  const auto bits = static_cast<uint32_t>(value);
  out.push_back(static_cast<char>(bits & 0xff));
  out.push_back(static_cast<char>((bits >> 8) & 0xff));
}

// LLVM 23 mistakes element invalidation for invalidating the vector reference;
// no element reference or iterator is retained across these appends.
// NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-invalidation)
void PutInt32(std::vector<char>& out, int64_t value) {
  const auto bits = static_cast<uint64_t>(value);
  for (unsigned shift = 0; shift < 32; shift += 8) {
    out.push_back(static_cast<char>((bits >> shift) & 0xff));
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
    ASSERT_NE(MixArchive::Register(mix_path_.string()), nullptr);
  }

  void TearDown() override {
    MixArchive::Free_All();
    SearchPaths::Clear();
    std::filesystem::remove(mix_path_);
    std::filesystem::remove(loose_path_);
    std::filesystem::remove_all(search_dir_);
  }

  // Cache() finds mixfiles by basename, not by the path they were registered
  // under.
  void CacheMixfile() const {
    ASSERT_TRUE(MixArchive::Cache(mix_path_.filename().string()));
  }

  // Replaces the flat fixture with the test mixfile packed inside an outer
  // mixfile on disk, registered as kInnerName through the outer one. Neither
  // is cached afterwards.
  void RegisterNestedMixfiles() {
    MixArchive::Free_All();
    WriteFile(mix_path_, MixImageHolding(kInnerName, MixImage()));
    ASSERT_NE(MixArchive::Register(mix_path_.string()), nullptr);
    ASSERT_NE(MixArchive::Register(kInnerName), nullptr);
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

TEST_F(GameFileTest, OpenGameFileOfNameFoundNowhereIsNull) {
  EXPECT_EQ(OpenGameFile("GAME_FILE_TEST_MISSING.BIN"), nullptr);
  EXPECT_FALSE(GameFileExists("GAME_FILE_TEST_MISSING.BIN"));
  EXPECT_EQ(GameFileSize("GAME_FILE_TEST_MISSING.BIN"), 0);
}

TEST_F(GameFileTest, SizeOfPackedFileNeedsNoOpen) {
  EXPECT_EQ(GameFileSize(kPackedName), 4);
}

TEST_F(GameFileTest, DeleteGameFileRefusesPackedFile) {
  EXPECT_FALSE(DeleteGameFile(kPackedName));
  EXPECT_TRUE(GameFileExists(kPackedName));
}

TEST_F(GameFileTest, OpenGameFileReadsCachedFileAndSeeksWithinItsImage) {
  CacheMixfile();
  const std::unique_ptr<ByteStream> file = OpenGameFile(kPackedName);
  ASSERT_NE(file, nullptr);
  EXPECT_EQ(file->Size(), 4);

  char buffer[8] = {};
  EXPECT_EQ(file->Read(buffer, 2), 2);
  EXPECT_EQ(std::string(buffer, 2), "ab");

  EXPECT_EQ(file->Seek(10, SeekOrigin::kBegin), 4);
  EXPECT_EQ(file->Read(buffer, 1), 0);
  EXPECT_EQ(file->Seek(-10, SeekOrigin::kCurrent), 0);
  EXPECT_EQ(file->Seek(-1, SeekOrigin::kEnd), 3);
  EXPECT_EQ(file->Read(buffer, 8), 1);
  EXPECT_EQ(buffer[0], 'd');
}

TEST_F(GameFileTest, OpenGameFileReadsUncachedFileOnlyItsBytesOfTheMixfile) {
  const std::unique_ptr<ByteStream> file = OpenGameFile(kPackedName);
  ASSERT_NE(file, nullptr);
  EXPECT_EQ(file->Size(), 4);

  char buffer[8] = {};
  EXPECT_EQ(file->Read(buffer, 8), 4);
  EXPECT_EQ(std::string(buffer, 4), "abcd");
}

TEST_F(GameFileTest,
       OpenGameFileReadsUncachedMixfileInsideUncachedMixfileBytes) {
  RegisterNestedMixfiles();
  const std::unique_ptr<ByteStream> file = OpenGameFile(kPackedName);
  ASSERT_NE(file, nullptr);
  EXPECT_EQ(file->Size(), 4);

  char buffer[8] = {};
  EXPECT_EQ(file->Read(buffer, 8), 4);
  EXPECT_EQ(std::string(buffer, 4), "abcd");
}

TEST_F(GameFileTest, OpenGameFileReadsCachedMixfileInsideUncachedMixfile) {
  RegisterNestedMixfiles();
  ASSERT_TRUE(MixArchive::Cache(kInnerName));
  const std::unique_ptr<ByteStream> file = OpenGameFile(kPackedName);
  ASSERT_NE(file, nullptr);

  char buffer[8] = {};
  EXPECT_EQ(file->Read(buffer, 8), 4);
  EXPECT_EQ(std::string(buffer, 4), "abcd");
}

TEST_F(GameFileTest, OpenGameFileForLooseFileOverridesPackedCopy) {
  CacheMixfile();
  WriteLooseCopy("LOOSE");
  EXPECT_TRUE(GameFileExists(kPackedName));
  const std::unique_ptr<ByteStream> file = OpenGameFile(kPackedName);
  ASSERT_NE(file, nullptr);
  EXPECT_EQ(file->Size(), 5);

  char buffer[8] = {};
  EXPECT_EQ(file->Read(buffer, 8), 5);
  EXPECT_EQ(std::string(buffer, 5), "LOOSE");
}

TEST_F(GameFileTest, OpenGameFileWriteToCachedFileWritesNothing) {
  CacheMixfile();
  const std::unique_ptr<ByteStream> file = OpenGameFile(kPackedName);
  ASSERT_NE(file, nullptr);

  // Backed by a MemoryStream over the cached mixfile image, which is
  // read-only.
  EXPECT_EQ(file->Write("zz", 2), 0);
}

TEST_F(GameFileTest, OpenGameFileWriteToUncachedFileWritesNothing) {
  const std::unique_ptr<ByteStream> file = OpenGameFile(kPackedName);
  ASSERT_NE(file, nullptr);

  // Backed by a RangeStream over the mixfile on disk, which is read-only.
  EXPECT_EQ(file->Write("zz", 2), 0);
}

TEST_F(GameFileTest, OpenGameFileWriteReplacesExistingLowercaseFile) {
  // A write never searches (Findings), so it opens the name in the current
  // working directory the same way OpenDiskFile does, and picks up the same
  // lowercase-twin fallback: it must update an existing lowercase file
  // (e.g. conquer.ini) rather than create a new upper-case one beside it.
  const std::string lower_name = "game_file_test_write_lowercase.bin";
  const std::string upper_name = absl::AsciiStrToUpper(lower_name);
  WriteFile(lower_name, {'o', 'l', 'd'});
  if (std::filesystem::exists(upper_name)) {
    std::filesystem::remove(lower_name);
    GTEST_SKIP() << "case-insensitive filesystem";
  }
  {
    const std::unique_ptr<ByteStream> out =
        OpenGameFile(upper_name, FileAccess::kWrite);
    ASSERT_NE(out, nullptr);
    out->Write(std::as_bytes(std::span(std::string_view("new"))));
  }
  EXPECT_FALSE(std::filesystem::exists(upper_name));
  std::ifstream in(lower_name, std::ios::binary);
  EXPECT_EQ(std::string(std::istreambuf_iterator<char>(in), {}), "new");
  std::filesystem::remove(lower_name);
}

TEST_F(GameFileTest, DeleteGameFileRemovesLooseFile) {
  WriteFile(loose_path(), {'h', 'i'});
  EXPECT_TRUE(GameFileExists(loose_path().string()));
  EXPECT_TRUE(DeleteGameFile(loose_path().string()));
  EXPECT_FALSE(std::filesystem::exists(loose_path()));
  EXPECT_FALSE(GameFileExists(loose_path().string()));
}

}  // namespace
