// Tests for DiskStream, MemoryStream and RangeStream.

#include "tech/byte_stream.h"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "gtest/gtest.h"
#include "sdllib/file_access.h"
#include "tech/file.h"

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

}  // namespace
