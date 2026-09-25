// Tests how failures travel through pipes and straws: short writes, read
// errors below a stream straw, truncated compressed data, and the
// flush-then-write-directly sequence the save game relies on.

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "engine/base/array.h"
#include "engine/base/types.h"
#include "engine/codec/block_codec.h"
#include "engine/codec/lzo_sink.h"
#include "engine/codec/lzo_source.h"
#include "engine/stream/byte_sink.h"
#include "engine/stream/byte_stream.h"
#include "engine/stream/seek_origin.h"
#include "engine/stream/span_sink.h"
#include "engine/stream/stream_sink.h"
#include "engine/stream/stream_source.h"
#include "gtest/gtest.h"
#include "tech/blowfish.h"
#include "tech/blowfish_sink.h"

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

// A stream that hands out `data` and then, if `fail_after` is set, reports a
// read error once that many bytes have been read.
class ScriptedStream : public ByteStream {
 public:
  ScriptedStream(std::string_view data, base::ssize fail_after)
      : data_(data), fail_after_(fail_after) {}

  using ByteStream::Read;
  using ByteStream::Write;
  base::ssize Read(std::span<std::byte> buffer) override {
    base::ssize count = 0;
    while (count < std::ssize(buffer) && position_ < std::ssize(data_)) {
      if (position_ == fail_after_) {
        failed_ = true;
        break;
      }
      base::At(buffer, static_cast<std::size_t>(count++)) =
          static_cast<std::byte>(
              data_.at(static_cast<std::size_t>(position_++)));
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

 private:
  std::string data_;
  base::ssize fail_after_;  // -1 means reads never fail.
  base::ssize position_ = 0;
  bool failed_ = false;
};

// A stream whose Write stores at most `accept` bytes and reports that count,
// so StreamSink sees a short write without needing a real full device.
class ShortWriteStream : public ByteStream {
 public:
  explicit ShortWriteStream(base::ssize accept) : accept_(accept) {}

  using ByteStream::Read;
  using ByteStream::Write;
  base::ssize Read(std::span<std::byte> /*buffer*/) override { return 0; }
  base::ssize Write(std::span<const std::byte> buffer) override {
    return std::min(accept_, std::ssize(buffer));
  }
  base::ssize Seek(base::ssize /*offset*/, SeekOrigin /*origin*/) override {
    return 0;
  }
  base::ssize Size() override { return 0; }

 private:
  base::ssize accept_;
};

// A stream that accepts every write but whose Flush always fails, e.g. a
// pubsync that reports EIO after the bytes were already accepted by Write.
class FailingFlushStream : public ByteStream {
 public:
  using ByteStream::Read;
  using ByteStream::Write;
  base::ssize Read(std::span<std::byte> /*buffer*/) override { return 0; }
  base::ssize Write(std::span<const std::byte> buffer) override {
    return std::ssize(buffer);
  }
  bool Flush() override { return false; }
  base::ssize Seek(base::ssize /*offset*/, SeekOrigin /*origin*/) override {
    return 0;
  }
  base::ssize Size() override { return 0; }
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
  blow.Key(std::as_bytes(std::span(key)));
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

TEST(StreamErrorTest, StreamSourceTellsEndOfStreamFromReadError) {
  {
    ScriptedStream stream("abc", -1);
    StreamSource source(stream);
    std::array<std::byte, 8> buffer{};
    EXPECT_EQ(source.Read(buffer), 3);
    EXPECT_EQ(source.Read(buffer), 0);
    EXPECT_TRUE(source.ok());
  }
  {
    // Two bytes arrive, then the stream fails.
    ScriptedStream stream("abcdef", 2);
    StreamSource source(stream);
    std::array<std::byte, 8> buffer{};
    EXPECT_EQ(source.Read(buffer), 2);
    EXPECT_FALSE(source.ok());
    EXPECT_EQ(source.Read(buffer), 0);
    EXPECT_FALSE(source.ok());
  }
}

TEST(StreamErrorTest, ReadErrorIsStickyThroughTransformStreamSource) {
  RecordingSink encoded;
  LzoSink compressor(CodecMode::kCompress, encoded, 16);
  compressor.Write(Bytes("0123456789abcdefghijklmnopqrstuvwxyz"));
  compressor.Finish();
  std::string stored(static_cast<std::size_t>(std::ssize(encoded.bytes)), '\0');
  for (std::size_t i = 0; i < stored.size(); ++i) {
    stored.at(i) = static_cast<char>(encoded.bytes.at(i));
  }

  // Blocks of 16, 16 and 4 bytes; the read error lands inside the last.
  ScriptedStream stream(stored, std::ssize(stored) - 3);
  StreamSource source(stream);
  LzoSource decompressor(CodecMode::kDecompress, source, 16);
  std::array<std::byte, 64> buffer{};
  EXPECT_EQ(decompressor.Read(buffer), 32);
  EXPECT_FALSE(decompressor.ok());
  EXPECT_EQ(decompressor.Read(buffer), 0);
  EXPECT_FALSE(decompressor.ok());
}

TEST(StreamSinkTest, StreamSinkFailsOnShortWrite) {
  ShortWriteStream stream(/*accept=*/3);
  StreamSink sink(stream);
  EXPECT_FALSE(sink.Write(Bytes("abcdef")));
  EXPECT_FALSE(sink.ok());
  // The failure is sticky: a later write does not get a second chance.
  EXPECT_FALSE(sink.Write(Bytes("g")));
  EXPECT_FALSE(sink.ok());
}

TEST(StreamSinkTest, FlushPropagatesAStreamFlushFailure) {
  FailingFlushStream stream;
  StreamSink sink(stream);
  EXPECT_TRUE(sink.Write(Bytes("ok")));
  EXPECT_FALSE(sink.Flush());
  EXPECT_FALSE(sink.ok());
}

TEST(StreamSinkTest, FinishPropagatesAStreamFlushFailure) {
  FailingFlushStream stream;
  StreamSink sink(stream);
  EXPECT_FALSE(sink.Finish());
  EXPECT_FALSE(sink.ok());
}

}  // namespace
