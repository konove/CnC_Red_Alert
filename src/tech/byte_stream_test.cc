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
#include <vector>

#include "base/array.h"
#include "base/buffer.h"
#include "base/seek_origin.h"
#include "gtest/gtest.h"
#include "sdllib/file_access.h"

#ifdef __linux__
#include <linux/prctl.h>
#include <sys/prctl.h>
#endif

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

TEST_F(DiskStreamTest, ReadWriteKeepsExistingContents) {
  {
    const std::unique_ptr<DiskStream> stream =
        DiskStream::Open(path(), FileAccess::kReadWrite);
    ASSERT_NE(stream, nullptr);
    EXPECT_EQ(stream->Seek(0, SeekOrigin::kEnd), 5);
    EXPECT_EQ(stream->Write(Bytes("!")), 1);
  }
  const std::unique_ptr<DiskStream> stream =
      DiskStream::Open(path(), FileAccess::kRead);
  ASSERT_NE(stream, nullptr);
  EXPECT_EQ(ReadAll(*stream, 10), "xabcd!");
}

TEST_F(DiskStreamTest, FlushedWritesAreVisibleToLaterWrites) {
  const std::unique_ptr<DiskStream> stream =
      DiskStream::Open(path(), FileAccess::kReadWrite);
  ASSERT_NE(stream, nullptr);
  stream->Seek(0, SeekOrigin::kEnd);
  EXPECT_EQ(stream->Write(Bytes("!")), 1);
  EXPECT_TRUE(stream->Flush());
  EXPECT_TRUE(stream->ok());
  EXPECT_EQ(stream->Write(Bytes("?")), 1);
  EXPECT_TRUE(stream->Flush());
  EXPECT_EQ(stream->Seek(0, SeekOrigin::kBegin), 0);
  EXPECT_EQ(ReadAll(*stream, 16), "xabcd!?");
}

// TD's super-record mode keeps RECORD.BIN open for the whole game and flushes
// it every frame, so a crash still leaves the frames written so far on disk.
TEST_F(DiskStreamTest, FlushMakesWrittenBytesVisible) {
  const std::unique_ptr<DiskStream> out =
      DiskStream::Open(path(), FileAccess::kWrite);
  ASSERT_NE(out, nullptr);
  EXPECT_EQ(out->Write(Bytes("frame")), 5);
  EXPECT_TRUE(out->Flush());
  // Read back through a second handle while the first is still open.
  const std::unique_ptr<DiskStream> in =
      DiskStream::Open(path(), FileAccess::kRead);
  ASSERT_NE(in, nullptr);
  EXPECT_EQ(ReadAll(*in, 10), "frame");
}

TEST_F(DiskStreamTest, ReadWriteCreatesMissingFile) {
  const std::string missing = path() + ".new";
  EXPECT_NE(DiskStream::Open(missing, FileAccess::kReadWrite), nullptr);
  EXPECT_TRUE(std::filesystem::exists(missing));
  std::filesystem::remove(missing);
}

TEST_F(DiskStreamTest, SeekBeforeTheStartKeepsThePosition) {
  const std::unique_ptr<DiskStream> stream =
      DiskStream::Open(path(), FileAccess::kRead);
  ASSERT_NE(stream, nullptr);
  EXPECT_EQ(stream->Seek(2, SeekOrigin::kBegin), 2);
  EXPECT_EQ(stream->Seek(-10, SeekOrigin::kCurrent), 2);
  EXPECT_EQ(ReadAll(*stream, 1), "b");
}

TEST_F(DiskStreamTest, SizePreservesThePosition) {
  const std::unique_ptr<DiskStream> stream =
      DiskStream::Open(path(), FileAccess::kRead);
  ASSERT_NE(stream, nullptr);
  EXPECT_EQ(ReadAll(*stream, 2), "xa");
  EXPECT_EQ(stream->Size(), 5);
  EXPECT_EQ(ReadAll(*stream, 1), "b");
}

TEST_F(DiskStreamTest, FailedSeekWithBufferedReadDataKeepsThePosition) {
  // Filling the read buffer (fixture is only 5 bytes, so one byte fills it)
  // exercises the same failed-seek path as SeekBeforeTheStartKeepsThePosition
  // once the filebuf is no longer positioned where the stream last reported.
  const std::unique_ptr<DiskStream> stream =
      DiskStream::Open(path(), FileAccess::kRead);
  ASSERT_NE(stream, nullptr);
  EXPECT_EQ(ReadAll(*stream, 1), "x");
  EXPECT_EQ(stream->Seek(-10, SeekOrigin::kCurrent), 1);
  EXPECT_EQ(ReadAll(*stream, 1), "a");
}

TEST(DiskStreamErrorTest, OpenRefusesDirectory) {
  EXPECT_EQ(DiskStream::Open(std::filesystem::temp_directory_path().string(),
                             FileAccess::kRead),
            nullptr);
}

TEST(DiskStreamErrorTest, WriteToFullDeviceFails) {
  if (!std::filesystem::exists("/dev/full")) {
    GTEST_SKIP() << "no /dev/full on this platform";
  }
  const std::unique_ptr<DiskStream> stream =
      DiskStream::Open("/dev/full", FileAccess::kWrite);
  ASSERT_NE(stream, nullptr);
  // Larger than any filebuf buffer, so the failure surfaces in Write itself.
  const std::vector<std::byte> block(1 << 20);
  stream->Write(block);
  EXPECT_FALSE(stream->ok());
}

TEST(DiskStreamErrorTest, FlushToFullDeviceFails) {
  if (!std::filesystem::exists("/dev/full")) {
    GTEST_SKIP() << "no /dev/full on this platform";
  }
  const std::unique_ptr<DiskStream> stream =
      DiskStream::Open("/dev/full", FileAccess::kWrite);
  ASSERT_NE(stream, nullptr);
  // One byte fits in the filebuf's own buffer, so sputn buffers it and
  // Write reports success; the failure only surfaces on Flush's pubsync.
  EXPECT_EQ(stream->Write(Bytes("!")), 1);
  EXPECT_FALSE(stream->Flush());
  EXPECT_FALSE(stream->ok());
}

TEST(DiskStreamErrorTest, SeekFlushesAPendingWriteAndReportsItsFailure) {
  if (!std::filesystem::exists("/dev/full")) {
    GTEST_SKIP() << "no /dev/full on this platform";
  }
  const std::unique_ptr<DiskStream> stream =
      DiskStream::Open("/dev/full", FileAccess::kWrite);
  ASSERT_NE(stream, nullptr);
  // A few bytes fit in the filebuf's own buffer, so Write itself reports
  // success; without Seek flushing first, pubseekoff's own failed flush
  // would be read as "seek before the start" and Seek would silently keep
  // reporting the old position instead of the write failure.
  EXPECT_EQ(stream->Write(Bytes("abc")), 3);
  stream->Seek(0, SeekOrigin::kBegin);
  EXPECT_FALSE(stream->ok());
}

#ifdef __linux__
// gtest_main.cc marks the whole test process undumpable (PR_SET_DUMPABLE 0)
// so a death test's forked child leaves no systemd-coredump behind, but the
// kernel also denies opening /proc/self/mem to an undumpable process, even
// for self-access. Only RealReadErrorSetsOkFalseWithoutThrowing needs
// dumpability, so it restores it for its own scope rather than weakening
// gtest_main.cc's default for every other test.
class ScopedDumpable {
 public:
  // prctl declares its optional arguments with a trailing "...".
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  ScopedDumpable() : was_dumpable_(prctl(PR_GET_DUMPABLE)) {
    prctl(PR_SET_DUMPABLE, 1);  // NOLINT(cppcoreguidelines-pro-type-vararg)
  }
  ~ScopedDumpable() {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    prctl(PR_SET_DUMPABLE, was_dumpable_);
  }

  ScopedDumpable(const ScopedDumpable&) = delete;
  ScopedDumpable& operator=(const ScopedDumpable&) = delete;
  ScopedDumpable(ScopedDumpable&&) = delete;
  ScopedDumpable& operator=(ScopedDumpable&&) = delete;

 private:
  int was_dumpable_;
};
#endif  // __linux__

TEST(DiskStreamErrorTest, RealReadErrorSetsOkFalseWithoutThrowing) {
#ifdef __linux__
  // Reading a process's own memory map at offset 0 fails with EIO; unlike a
  // directory, the open itself succeeds, so this is the one read error this
  // test suite can provoke without a failing disk.
  const ScopedDumpable dumpable;
  if (!std::filesystem::exists("/proc/self/mem")) {
    GTEST_SKIP() << "no /proc/self/mem on this platform";
  }
  const std::unique_ptr<DiskStream> stream =
      DiskStream::Open("/proc/self/mem", FileAccess::kRead);
  if (stream == nullptr) {
    GTEST_SKIP() << "could not open /proc/self/mem";
  }
  std::array<std::byte, 8> buffer{};
  EXPECT_EQ(stream->Read(buffer), 0);
  EXPECT_FALSE(stream->ok());
#else
  GTEST_SKIP() << "/proc/self/mem is Linux-only";
#endif
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
  const std::unique_ptr<DiskStream> stream =
      DiskStream::Open(path(), FileAccess::kReadWrite);
  ASSERT_NE(stream, nullptr);

  const Header written{.magic = 0x52415356, .version = 7, .flags = -2};
  const std::array<int16_t, 3> array_written = {1, 2, 3};
  EXPECT_TRUE(stream->WriteObject(written));
  EXPECT_TRUE(stream->WriteObject(array_written));
  EXPECT_TRUE(stream->WriteObject(int32_t{-1}));
  EXPECT_EQ(stream->Seek(0, SeekOrigin::kBegin), 0);

  Header read{};
  std::array<int16_t, 3> array_read{};
  int32_t trailer = 0;
  EXPECT_TRUE(stream->ReadObject(read));
  EXPECT_TRUE(stream->ReadObject(array_read));
  EXPECT_TRUE(stream->ReadObject(trailer));
  EXPECT_EQ(read.magic, written.magic);
  EXPECT_EQ(read.version, written.version);
  EXPECT_EQ(read.flags, written.flags);
  EXPECT_EQ(array_read, array_written);
  EXPECT_EQ(trailer, -1);
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

TEST(MemoryStreamTest, TypedViewCountStopsMidElement) {
  MemoryStream stream(Bytes("abcd"));
  std::array<uint16_t, 2> words{};
  EXPECT_EQ(stream.Read(std::span(words), 3), 3);  // 3 bytes: 1 full, 1 half
  const auto bytes = std::as_bytes(std::span(words));
  EXPECT_EQ(base::At(bytes, 0), std::byte{'a'});
  EXPECT_EQ(base::At(bytes, 1), std::byte{'b'});
  EXPECT_EQ(base::At(bytes, 2), std::byte{'c'});
  EXPECT_EQ(base::At(bytes, 3), std::byte{0});
}

}  // namespace
