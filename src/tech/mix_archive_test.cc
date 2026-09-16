// Tests for opening plain MIX archives, including corrupt headers.

#include "tech/mix_archive.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <ios>
#include <span>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "tech/crc.h"

namespace {

using Mix = MixArchive;

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

// Builds a plain-format MIX holding "abcd" as A.BIN, with the header count,
// data size and entry fields overridable to simulate corruption.
struct MixImage {
  int count = 1;
  int64_t size = 4;
  int64_t entry_offset = 0;
  int64_t entry_size = 4;
  bool truncate_index = false;

  [[nodiscard]] std::vector<char> Bytes() const {
    std::vector<char> out;
    PutInt16(out, count);
    PutInt32(out, size);
    PutInt32(out, std::bit_cast<int32_t>(CrcEngine::Compute("A.BIN")));
    PutInt32(out, entry_offset);
    PutInt32(out, entry_size);
    if (truncate_index) {
      out.resize(out.size() - 4);
      return out;
    }
    for (const char byte : std::string("abcd")) {
      out.push_back(byte);
    }
    return out;
  }
};

class MixFileTest : public ::testing::Test {
 protected:
  // Writes image to a uniquely named temp file and registers it.
  Mix* Register(const MixImage& image) {
    const std::string name =
        std::string("mix_archive_test_") +
        ::testing::UnitTest::GetInstance()->current_test_info()->name() +
        ".mix";
    path_ = std::filesystem::temp_directory_path() / name;
    const std::vector<char> bytes = image.Bytes();
    std::ofstream file(path_, std::ios::binary);
    EXPECT_TRUE(file.is_open());
    file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    file.close();
    return Mix::Register(path_.string());
  }

  void TearDown() override {
    Mix::Free_All();
    std::filesystem::remove(path_);
  }

 private:
  std::filesystem::path path_;
};

TEST_F(MixFileTest, ValidArchiveServesItsFile) {
  ASSERT_NE(Register(MixImage{}), nullptr);
  ASSERT_TRUE(Mix::Cache("mix_archive_test_ValidArchiveServesItsFile.mix"));

  const std::span<const std::byte> data = Mix::RetrieveData("a.bin");
  ASSERT_EQ(data.size(), 4U);
  EXPECT_EQ(static_cast<char>(data[0]), 'a');
  EXPECT_EQ(static_cast<char>(data[3]), 'd');
}

TEST_F(MixFileTest, NegativeCountFailsOpen) {
  // Before the fix, resize() threw std::length_error.
  EXPECT_EQ(Register(MixImage{.count = -1}), nullptr);
}

TEST_F(MixFileTest, NegativeDataSizeFailsOpen) {
  EXPECT_EQ(Register(MixImage{.size = -4}), nullptr);
}

TEST_F(MixFileTest, DataSizeBeyondFileFailsOpen) {
  EXPECT_EQ(Register(MixImage{.size = 5, .entry_size = 5}), nullptr);
}

TEST_F(MixFileTest, EntryOutsideDataFailsOpen) {
  EXPECT_EQ(Register(MixImage{.entry_offset = 2}), nullptr);
  EXPECT_EQ(Register(MixImage{.entry_offset = -1}), nullptr);
}

TEST_F(MixFileTest, TruncatedIndexFailsOpen) {
  EXPECT_EQ(Register(MixImage{.size = 0, .truncate_index = true}), nullptr);
}

}  // namespace
