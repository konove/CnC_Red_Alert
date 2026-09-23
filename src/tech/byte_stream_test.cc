// Tests for DiskStream, MemoryStream and RangeStream.

#include "tech/byte_stream.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "base/array.h"
#include "base/buffer.h"
#include "base/seek_origin.h"
#include "gtest/gtest.h"
#include "sdllib/file_access.h"

namespace {

std::span<const std::byte> Bytes(const std::string_view text) {
  return std::as_bytes(std::span(text));
}

std::string ReadAll(ByteStream& stream, const int count) {
  std::string out(static_cast<size_t>(count), '\0');
  out.resize(
      static_cast<size_t>(stream.Read(std::as_writable_bytes(std::span(out)))));
  return out;
}

class DiskStreamTest : public ::testing::Test {
 protected:
  void SetUp() override {
    const std::string test_name =
        ::testing::UnitTest::GetInstance()->current_test_info()->name();
    path_ = std::filesystem::temp_directory_path() /
            ("byte_stream_test_" + test_name + ".bin");
    std::ofstream file(path_, std::ios::binary);
    file << "xabcd";
  }

  void TearDown() override { std::filesystem::remove(path_); }

  [[nodiscard]] std::string path() const { return path_.string(); }

 private:
  std::filesystem::path path_;
};

TEST_F(DiskStreamTest, MissingFileDoesNotOpen) {
  EXPECT_EQ(DiskStream::Open(path() + ".missing", FileAccess::kRead), nullptr);
}

TEST_F(DiskStreamTest, ReadsSeeksAndReportsSize) {
  const std::unique_ptr<DiskStream> stream =
      DiskStream::Open(path(), FileAccess::kRead);
  ASSERT_NE(stream, nullptr);
  EXPECT_EQ(stream->Size(), 5);
  EXPECT_EQ(ReadAll(*stream, 2), "xa");
  EXPECT_EQ(stream->Tell(), 2);
  EXPECT_EQ(stream->Seek(-1, SeekOrigin::kEnd), 4);
  EXPECT_EQ(ReadAll(*stream, 8), "d");
  EXPECT_EQ(stream->Seek(1, SeekOrigin::kBegin), 1);
  EXPECT_EQ(ReadAll(*stream, 8), "abcd");
}

TEST_F(DiskStreamTest, WritesAppendAtTheEnd) {
  {
    const std::unique_ptr<DiskStream> stream =
        DiskStream::Open(path(), FileAccess::kReadWrite);
    ASSERT_NE(stream, nullptr);
    stream->Seek(0, SeekOrigin::kEnd);
    EXPECT_EQ(stream->Write(Bytes("!")), 1);
  }
  const std::unique_ptr<DiskStream> stream =
      DiskStream::Open(path(), FileAccess::kRead);
  ASSERT_NE(stream, nullptr);
  EXPECT_EQ(ReadAll(*stream, 16), "xabcd!");
}

TEST(MemoryStreamTest, ReadsWithinTheViewAndClampsSeeks) {
  MemoryStream stream(Bytes("xabcd"));
  EXPECT_EQ(stream.Size(), 5);
  EXPECT_EQ(ReadAll(stream, 3), "xab");
  EXPECT_EQ(stream.Seek(10, SeekOrigin::kCurrent), 5);
  EXPECT_EQ(ReadAll(stream, 3), "");
  EXPECT_EQ(stream.Seek(-100, SeekOrigin::kEnd), 0);
  EXPECT_EQ(stream.Seek(4, SeekOrigin::kBegin), 4);
  EXPECT_EQ(ReadAll(stream, 3), "d");
  EXPECT_EQ(stream.Write(Bytes("z")), 0);
}

TEST(RangeStreamTest, SeesOnlyItsWindow) {
  RangeStream range(std::make_unique<MemoryStream>(Bytes("xabcdy")), 1, 4);
  EXPECT_EQ(range.Size(), 4);
  EXPECT_EQ(ReadAll(range, 2), "ab");
  EXPECT_EQ(range.Tell(), 2);
  EXPECT_EQ(ReadAll(range, 10), "cd");
  EXPECT_EQ(range.Seek(-1, SeekOrigin::kEnd), 3);
  EXPECT_EQ(ReadAll(range, 10), "d");
  EXPECT_EQ(range.Seek(-10, SeekOrigin::kCurrent), 0);
  EXPECT_EQ(range.Seek(10, SeekOrigin::kBegin), 4);
  EXPECT_EQ(range.Write(Bytes("z")), 0);
}

TEST(RangeStreamTest, WindowIsClippedToTheInnerStream) {
  RangeStream range(std::make_unique<MemoryStream>(Bytes("xabcd")), 3, 10);
  EXPECT_EQ(range.Size(), 2);
  EXPECT_EQ(ReadAll(range, 10), "cd");
  RangeStream beyond(std::make_unique<MemoryStream>(Bytes("xabcd")), 9, 1);
  EXPECT_EQ(beyond.Size(), 0);
}

TEST(RangeStreamTest, NestsInsideAnotherRange) {
  // "xabcd" sits at offset 2 of the outer data; "bc" at offset 2 of that.
  auto outer = std::make_unique<RangeStream>(
      std::make_unique<MemoryStream>(Bytes("--xabcd--")), 2, 5);
  RangeStream inner(std::move(outer), 2, 2);
  EXPECT_EQ(inner.Size(), 2);
  EXPECT_EQ(ReadAll(inner, 10), "bc");
  EXPECT_EQ(inner.Seek(1, SeekOrigin::kBegin), 1);
  EXPECT_EQ(ReadAll(inner, 10), "c");
}

TEST_F(DiskStreamTest, RangeOverDiskFileReadsItsBytes) {
  std::unique_ptr<DiskStream> disk =
      DiskStream::Open(path(), FileAccess::kRead);
  ASSERT_NE(disk, nullptr);
  RangeStream range(std::move(disk), 1, 4);
  EXPECT_EQ(ReadAll(range, 10), "abcd");
}

struct Header {
  int32_t magic;
  int16_t version;
  int16_t flags;
};

TEST_F(DiskStreamTest, ObjectsRoundTripThroughWriteObjectAndReadObject) {
  const Header written{.magic = 0x52415356, .version = 7, .flags = -2};
  {
    const std::unique_ptr<DiskStream> out =
        DiskStream::Open(path(), FileAccess::kWrite);
    ASSERT_NE(out, nullptr);
    EXPECT_TRUE(out->WriteObject(written));
  }
  const std::unique_ptr<DiskStream> in =
      DiskStream::Open(path(), FileAccess::kRead);
  ASSERT_NE(in, nullptr);
  Header read{};
  EXPECT_TRUE(in->ReadObject(read));
  EXPECT_EQ(read.magic, written.magic);
  EXPECT_EQ(read.version, written.version);
  EXPECT_EQ(read.flags, written.flags);
}

TEST(MemoryStreamTest, ReadObjectFailsOnShortRead) {
  const std::array<std::byte, 3> bytes{};
  MemoryStream stream(bytes);
  int32_t value = 0;
  EXPECT_FALSE(stream.ReadObject(value));
}

TEST(MemoryStreamTest, ArraysAreObjectsToo) {
  const int16_t written[3] = {1, 2, 3};
  MemoryStream stream(std::as_bytes(std::span(written)));
  int16_t read[3] = {};
  EXPECT_TRUE(stream.ReadObject(read));
  EXPECT_EQ(base::At(read, 2), 3);
}

TEST(MemoryStreamTest, ReadBytesAndReadStringStopAtEndOfStream) {
  const std::string_view text = "abcde";
  MemoryStream stream(std::as_bytes(std::span(text)));
  EXPECT_EQ(stream.ReadString(3), "abc");
  EXPECT_EQ(std::ssize(stream.ReadBytes(10)), 2);
}

TEST(MemoryStreamTest, SpanAndRawPointerReadsAgree) {
  MemoryStream stream(Bytes("xyz"));
  std::array<std::byte, 2> first{};
  EXPECT_EQ(stream.Read(std::span(first)), 2);
  EXPECT_EQ(static_cast<char>(first.at(1)), 'y');
  char last = 0;
  EXPECT_EQ(stream.Read(base::ObjectBytes(last)), 1);
  EXPECT_EQ(last, 'z');
}

TEST(MemoryStreamTest, TypedViewCountIsBytesRatherThanElements) {
  const std::array<uint16_t, 2> words{0x0102, 0x0304};
  MemoryStream stream(std::as_bytes(std::span(words)));
  std::array<uint16_t, 2> read{};
  EXPECT_EQ(stream.Read(std::span(read), 2), 2);  // 2 bytes = 1 element
  EXPECT_EQ(read.at(0), words.at(0));
  EXPECT_EQ(read.at(1), 0);
}

}  // namespace
