// Tests for opening MIX archives: plain and PK-encrypted, single file and
// several, and one archive nested inside another - the shape the shipped
// data has and that tools/mixdump reads.

#include "tech/mix_archive.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <ios>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "engine/base/array.h"
#include "engine/crypto/blowfish_sink.h"
#include "engine/crypto/crc.h"
#include "engine/crypto/pk.h"
#include "engine/crypto/pk_sink.h"
#include "engine/crypto/random_source.h"
#include "engine/stream/byte_sink.h"
#include "gtest/gtest.h"

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

// A file to pack into a test archive.
struct MixEntry {
  std::string name;
  std::string contents;
};

// The bytes a lookup returned, as a string to compare against what was packed.
std::string Contents(std::span<const std::byte> data) {
  std::string out;
  for (const std::byte byte : data) {
    out.push_back(static_cast<char>(byte));
  }
  return out;
}

// Collects what a sink is given, so an encrypted header can be built in memory.
class VectorSink : public ByteSink {
 public:
  std::vector<char> bytes;

  bool Write(std::span<const std::byte> data) override {
    for (const std::byte byte : data) {
      bytes.push_back(static_cast<char>(byte));
    }
    return true;
  }
};

// The index is searched with lower_bound, so it has to be sorted by CRC. The
// entries and the data must agree on offsets, hence one pass to lay the files
// out and a second to emit them in index order.
struct IndexEntry {
  int32_t crc;
  int64_t offset;
  int64_t size;
};

std::vector<IndexEntry> BuildIndex(std::span<const MixEntry> files) {
  std::vector<IndexEntry> index;
  int64_t offset = 0;
  for (const MixEntry& file : files) {
    index.push_back(
        {.crc = std::bit_cast<int32_t>(CrcEngine::Compute(file.name)),
         .offset = offset,
         .size = std::ssize(file.contents)});
    offset += std::ssize(file.contents);
  }
  std::ranges::sort(index, {}, &IndexEntry::crc);
  return index;
}

int64_t DataSize(std::span<const MixEntry> files) {
  int64_t total = 0;
  for (const MixEntry& file : files) {
    total += std::ssize(file.contents);
  }
  return total;
}

// Appends the files in the order the index describes them.
// LLVM 23 mistakes element invalidation for invalidating the vector reference;
// no element reference or iterator is retained across these appends.
// NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-invalidation)
void PutData(std::vector<char>& out, std::span<const MixEntry> files) {
  for (const MixEntry& file : files) {
    for (const char byte : file.contents) {
      out.push_back(byte);
    }
  }
}

// Plain format: count, data size, index, then the data.
std::vector<char> PlainMix(std::span<const MixEntry> files) {
  std::vector<char> out;
  PutInt16(out, static_cast<int>(files.size()));
  PutInt32(out, DataSize(files));
  for (const IndexEntry& entry : BuildIndex(files)) {
    PutInt32(out, entry.crc);
    PutInt32(out, entry.offset);
    PutInt32(out, entry.size);
  }
  PutData(out, files);
  return out;
}

// Extended format: a zero marker and the flags, then the count, data size and
// index PK-encrypted as one block, then the plain data. Blowfish pads the
// encrypted block out to its own block size, and the data begins after the
// padding - which is what lets MixArchive find the data by asking the file
// where reading the index left it.
std::vector<char> EncryptedMix(std::span<const MixEntry> files, const PKey& key,
                               RandomSource& rng) {
  std::vector<char> index;
  PutInt16(index, static_cast<int>(files.size()));
  PutInt32(index, DataSize(files));
  for (const IndexEntry& entry : BuildIndex(files)) {
    PutInt32(index, entry.crc);
    PutInt32(index, entry.offset);
    PutInt32(index, entry.size);
  }

  // Blowfish works in eight-byte blocks, so reading the index back pulls whole
  // blocks from the file. Padding the index up to a block boundary is what
  // leaves the file positioned exactly at the data once the index is read.
  while (index.size() % 8 != 0) {
    index.push_back(0);
  }

  VectorSink encrypted;
  {
    const std::unique_ptr<BlowfishSink> sink =
        MakePkEncryptSink(encrypted, key, rng);
    EXPECT_TRUE(sink->Write(std::as_bytes(std::span(index))));
    EXPECT_TRUE(sink->Flush());
  }

  std::vector<char> out;
  PutInt16(out, 0);  // marks the extended format
  PutInt16(out, 2);  // the encrypted-index flag
  for (const char byte : encrypted.bytes) {
    out.push_back(byte);
  }
  PutData(out, files);
  return out;
}

class MixFileTest : public ::testing::Test {
 protected:
  // Writes bytes to a uniquely named temp file and registers it. Several
  // archives can be live at once, which is what a nested archive needs.
  // The name the archive is registered and looked up under.
  static std::string ArchiveName(std::string_view stem) {
    return std::string(stem) + "_" +
           ::testing::UnitTest::GetInstance()->current_test_info()->name() +
           ".mix";
  }

  Mix* RegisterBytes(std::string_view stem, std::span<const char> bytes,
                     const PKey* key = nullptr) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / ArchiveName(stem);
    std::ofstream file(path, std::ios::binary);
    EXPECT_TRUE(file.is_open());
    file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    file.close();
    extra_.push_back(path);
    return Mix::Register(path.string(), key);
  }

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
    for (const std::filesystem::path& path : extra_) {
      std::filesystem::remove(path);
    }
  }

 private:
  std::filesystem::path path_;
  std::vector<std::filesystem::path> extra_;
};

// A key pair for the encrypted fixtures. Small primes keep it quick; the
// modulus still spans several Blowfish key blocks, as the shipped one does.
//
// The archive is written with the private half and read with the public one,
// the way the shipped data is: the game only ever holds the public key.
class MixKeys {
 public:
  static const PKey& Public() { return Instance().public_key_; }
  static const PKey& Private() { return Instance().private_key_; }
  static RandomSource& Rng() { return Instance().rng_; }

 private:
  static MixKeys& Instance() {
    static MixKeys keys;
    return keys;
  }

  MixKeys() {
    for (int32_t value = 1; rng_.Seed_Bits_Needed() > 0; ++value) {
      rng_.Seed_Long(value * 40503);
    }
    PKey::Generate(rng_, 128, public_key_, private_key_);
  }

  RandomSource rng_;
  PKey public_key_;
  PKey private_key_;
};

TEST_F(MixFileTest, ValidArchiveServesItsFile) {
  ASSERT_NE(Register(MixImage{}), nullptr);
  ASSERT_TRUE(Mix::Cache("mix_archive_test_ValidArchiveServesItsFile.mix"));

  const std::span<const std::byte> data = Mix::RetrieveData("a.bin");
  ASSERT_EQ(data.size(), 4U);
  EXPECT_EQ(static_cast<char>(base::At(data, 0)), 'a');
  EXPECT_EQ(static_cast<char>(base::At(data, 3)), 'd');
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

TEST_F(MixFileTest, PlainArchiveServesEveryFileItHolds) {
  const std::vector<MixEntry> files = {
      {.name = "FIRST.BIN", .contents = "one"},
      {.name = "SECOND.BIN", .contents = "two!"},
      {.name = "THIRD.BIN", .contents = "three"}};
  const std::vector<char> bytes = PlainMix(files);
  ASSERT_NE(RegisterBytes("plain", bytes), nullptr);
  ASSERT_TRUE(Mix::Cache(ArchiveName("plain")));

  for (const MixEntry& file : files) {
    const std::span<const std::byte> data = Mix::RetrieveData(file.name);
    EXPECT_EQ(Contents(data), file.contents) << file.name;
  }
}

TEST_F(MixFileTest, EncryptedIndexRoundTrips) {
  const std::vector<MixEntry> files = {
      {.name = "ALPHA.BIN", .contents = "aaaa"},
      {.name = "BETA.BIN", .contents = "bbbbbb"}};
  const std::vector<char> bytes =
      EncryptedMix(files, MixKeys::Private(), MixKeys::Rng());
  ASSERT_NE(RegisterBytes("crypt", bytes, &MixKeys::Public()), nullptr);
  ASSERT_TRUE(Mix::Cache(ArchiveName("crypt")));

  for (const MixEntry& file : files) {
    const std::span<const std::byte> data = Mix::RetrieveData(file.name);
    EXPECT_EQ(Contents(data), file.contents) << file.name;
  }
}

// The shape the shipped data has: the mission INIs live in an archive that is
// itself a file inside another archive, and registering the outer one is what
// makes the inner one openable by name. tools/mixdump depends on this.
TEST_F(MixFileTest, ArchiveNestedInsideAnotherOpensByName) {
  const std::vector<MixEntry> inner_files = {
      {.name = "BURIED.BIN", .contents = "deep"}};
  const std::vector<char> inner = PlainMix(inner_files);

  const std::vector<MixEntry> outer_files = {
      {.name = "INNER.MIX",
       .contents = std::string(inner.begin(), inner.end())}};
  const std::vector<char> outer = PlainMix(outer_files);

  ASSERT_NE(RegisterBytes("outer", outer), nullptr);
  // Only reachable because the archive holding it is registered.
  ASSERT_NE(Mix::Register("INNER.MIX"), nullptr);
  ASSERT_TRUE(Mix::Cache("INNER.MIX"));

  EXPECT_EQ(Contents(Mix::RetrieveData("BURIED.BIN")), "deep");
}

// The index is what a tool inspecting an archive has to work from, since the
// names themselves are not stored - only their CRCs.
TEST_F(MixFileTest, IndexReportsEveryEntrySortedByCrc) {
  const std::vector<MixEntry> files = {
      {.name = "ONE.BIN", .contents = "1"},
      {.name = "TWO.BIN", .contents = "22"},
      {.name = "THREE.BIN", .contents = "333"}};
  const Mix* archive = RegisterBytes("index", PlainMix(files));
  ASSERT_NE(archive, nullptr);

  const std::span<const Mix::FileEntry> index = archive->index();
  ASSERT_EQ(index.size(), files.size());
  EXPECT_TRUE(std::ranges::is_sorted(index, {}, &Mix::FileEntry::crc));

  int64_t total = 0;
  for (const Mix::FileEntry& entry : index) {
    total += entry.size;
  }
  EXPECT_EQ(total, 1 + 2 + 3);

  // Every packed name is in there, found by its CRC the way a lookup does it.
  for (const MixEntry& file : files) {
    const auto crc = std::bit_cast<int32_t>(CrcEngine::Compute(file.name));
    EXPECT_TRUE(std::ranges::any_of(index, [crc](const Mix::FileEntry& e) {
      return e.crc == crc;
    })) << file.name;
  }
}

}  // namespace
