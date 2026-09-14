// Tests how failures travel through pipes and straws: short writes, read
// errors below a file straw, truncated compressed data, and the
// flush-then-write-directly sequence the save game relies on.

#include <array>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "base/seek_origin.h"
#include "base/types.h"
#include "gtest/gtest.h"
#include "sdllib/file_access.h"
#include "tech/blowfish.h"
#include "tech/blowfish_sink.h"
#include "tech/byte_sink.h"
#include "tech/byte_stream.h"
#include "tech/codec_block.h"
#include "tech/disk_file.h"
#include "tech/file.h"
#include "tech/file_source.h"
#include "tech/lzo_sink.h"
#include "tech/lzo_source.h"
#include "tech/span_sink.h"

namespace {

std::span<const std::byte> Bytes(std::string_view text) {
  return std::as_bytes(std::span(text));
}

class RecordingSink : public ByteSink {
 public:
  std::vector<std::byte> bytes;
  bool Write(std::span<const std::byte> data) override {
    bytes.insert(bytes.end(), data.begin(), data.end());
    return true;
  }
};

// A file that hands out `data` and then, if `fail_after` is set, reports a
// read error once that many bytes have been read.
class ScriptedFile : public File {
 public:
  ScriptedFile(std::string_view data, base::ssize fail_after)
      : data_(data), fail_after_(fail_after) {}

  [[nodiscard]] std::string_view FileName() const override { return "x"; }
  void SetName(std::string_view /*filename*/) override {}
  bool Create() override { return false; }
  bool Delete() override { return false; }
  bool IsAvailable() override { return true; }
  [[nodiscard]] bool IsOpen() const override { return open_; }
  bool Open(std::string_view /*filename*/, FileAccess rights) override {
    return Open(rights);
  }
  bool Open(FileAccess /*rights*/) override {
    open_ = true;
    return true;
  }
  using File::Read;
  using File::Write;
  base::ssize Read(std::span<std::byte> buffer) override {
    base::ssize count = 0;
    while (count < std::ssize(buffer) && position_ < std::ssize(data_)) {
      if (position_ == fail_after_) {
        failed_ = true;
        break;
      }
      buffer[static_cast<std::size_t>(count++)] =
          static_cast<std::byte>(data_[static_cast<std::size_t>(position_++)]);
    }
    return count;
  }
  base::ssize Write(std::span<const std::byte> /*buffer*/) override {
    return 0;
  }
  [[nodiscard]] bool ok() const override { return !failed_; }
  base::ssize Seek(base::ssize /*offset*/, SeekOrigin /*origin*/) override {
    return position_;
  }
  base::ssize Size() override { return std::ssize(data_); }
  void Close() override { open_ = false; }

 private:
  std::string data_;
  base::ssize fail_after_;  // -1 means reads never fail.
  base::ssize position_ = 0;
  bool open_ = false;
  bool failed_ = false;
};

TEST(StreamErrorTest, BufferPipeStoresWhatFitsThenFailsForGood) {
  std::array<char, 4> storage{};
  SpanSink sink(std::as_writable_bytes(std::span(storage)));
  EXPECT_TRUE(sink.Write(Bytes("ab")));
  EXPECT_TRUE(sink.ok());
  EXPECT_FALSE(sink.Write(Bytes("cde")));
  EXPECT_FALSE(sink.ok());
  EXPECT_EQ(sink.bytes_written(), 4);
  EXPECT_EQ(std::string_view(storage.data(), 4), "abcd");
  EXPECT_FALSE(sink.Write(Bytes("")));
  EXPECT_FALSE(sink.Flush());
  EXPECT_FALSE(sink.Finish());
}

TEST(StreamErrorTest, FailureDownstreamReachesEveryLink) {
  std::array<char, 8> storage{};
  SpanSink sink(std::as_writable_bytes(std::span(storage)));
  LzoSink compressor(CodecMode::kCompress, sink, 16);
  // Buffered: nothing has reached the small sink yet.
  EXPECT_TRUE(compressor.Write(Bytes("0123456789")));
  EXPECT_TRUE(compressor.ok());
  // The flushed block does not fit.
  EXPECT_FALSE(compressor.Flush());
  EXPECT_FALSE(compressor.ok());
  EXPECT_FALSE(compressor.Write(Bytes("more")));
}

// Save_Game flushes the chain, writes the digest straight into the file pipe
// behind it, then finishes the chain, which must add nothing.
TEST(StreamErrorTest, FlushEmitsEverythingSoFinishAddsNothing) {
  RecordingSink file;
  BlowfishSink blow(CipherMode::kEncrypt, file);
  LzoSink lzo(CodecMode::kCompress, blow, 64);
  const std::array<char, 8> key = {1, 2, 3, 4, 5, 6, 7, 8};
  blow.Key(key.data(), static_cast<int>(key.size()));
  // 100 bytes: one full block and a partial one, and a Blowfish tail.
  const std::string text(100, 'q');
  EXPECT_TRUE(lzo.Write(Bytes(text)));
  EXPECT_TRUE(lzo.Flush());
  const std::size_t flushed = file.bytes.size();
  EXPECT_GT(flushed, 0U);
  EXPECT_TRUE(file.Write(Bytes("DIGEST")));
  EXPECT_TRUE(lzo.Finish());
  EXPECT_EQ(file.bytes.size(), flushed + 6);
  EXPECT_TRUE(lzo.Flush());
  EXPECT_EQ(file.bytes.size(), flushed + 6);
}

TEST(StreamErrorTest, FileStrawTellsEndOfFileFromReadError) {
  {
    ScriptedFile file("abc", -1);
    FileSource straw(file);
    std::array<std::byte, 8> buffer{};
    EXPECT_EQ(straw.Read(buffer), 3);
    EXPECT_EQ(straw.Read(buffer), 0);
    EXPECT_TRUE(straw.ok());
  }
  {
    // Two bytes arrive, then the file fails.
    ScriptedFile file("abcdef", 2);
    FileSource straw(file);
    std::array<std::byte, 8> buffer{};
    EXPECT_EQ(straw.Read(buffer), 2);
    EXPECT_FALSE(straw.ok());
    EXPECT_EQ(straw.Read(buffer), 0);
    EXPECT_FALSE(straw.ok());
  }
}

TEST(StreamErrorTest, ReadErrorIsStickyThroughTransformStraw) {
  RecordingSink encoded;
  LzoSink compressor(CodecMode::kCompress, encoded, 16);
  compressor.Write(Bytes("0123456789abcdefghijklmnopqrstuvwxyz"));
  compressor.Finish();
  std::string stored(static_cast<std::size_t>(std::ssize(encoded.bytes)), '\0');
  for (std::size_t i = 0; i < stored.size(); ++i) {
    stored[i] = static_cast<char>(encoded.bytes[i]);
  }

  // Blocks of 16, 16 and 4 bytes; the read error lands inside the last.
  ScriptedFile file(stored, std::ssize(stored) - 3);
  FileSource straw(file);
  LzoSource decompressor(CodecMode::kDecompress, straw, 16);
  std::array<std::byte, 64> buffer{};
  EXPECT_EQ(decompressor.Read(buffer), 32);
  EXPECT_FALSE(decompressor.ok());
  EXPECT_EQ(decompressor.Read(buffer), 0);
  EXPECT_FALSE(decompressor.ok());
}

TEST(StreamErrorTest, DiskReadErrorReachesFileAndStraw) {
  // Reading a directory opened as a file fails in the C library.
  const std::string directory = std::filesystem::temp_directory_path().string();
  const std::unique_ptr<DiskStream> stream =
      DiskStream::Open(directory, FileAccess::kRead);
  if (stream == nullptr) {
    GTEST_SKIP() << "this C library does not open directories";
  }
  std::array<std::byte, 8> buffer{};
  EXPECT_EQ(stream->Read(buffer), 0);
  EXPECT_FALSE(stream->ok());

  DiskFile file(directory);
  EXPECT_EQ(file.Read(buffer), 0);
  EXPECT_FALSE(file.ok());
  FileSource straw(file);
  EXPECT_EQ(straw.Read(buffer), 0);
  EXPECT_FALSE(straw.ok());
}

}  // namespace
