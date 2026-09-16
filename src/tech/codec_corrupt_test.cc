// Tests that the block-based LCW, LZW and LZO decoders stop cleanly on corrupt
// block headers and payloads instead of reading or writing past their buffers.

#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <string_view>
#include <vector>

#include "base/types.h"
#include "gtest/gtest.h"
#include "tech/block_codec.h"
#include "tech/byte_sink.h"
#include "tech/byte_source.h"
#include "tech/lcw_sink.h"
#include "tech/lcw_source.h"
#include "tech/lzo_sink.h"
#include "tech/lzo_source.h"
#include "tech/lzw_sink.h"
#include "tech/lzw_source.h"
#include "tech/span_source.h"

namespace {

constexpr int kBlockSize = 128;

class RecordingSink : public ByteSink {
 public:
  std::vector<uint8_t> bytes;
  bool Write(std::span<const std::byte> data) override {
    for (const std::byte byte : data) {
      bytes.push_back(std::to_integer<uint8_t>(byte));
    }
    return true;
  }
};

std::vector<uint8_t> Drain(ByteSource& straw) {
  std::vector<uint8_t> result;
  std::array<uint8_t, 7> chunk{};
  for (base::ssize count = straw.Read(std::as_writable_bytes(std::span(chunk)));
       count != 0;
       count = straw.Read(std::as_writable_bytes(std::span(chunk)))) {
    result.insert(result.end(), chunk.begin(), chunk.begin() + count);
  }
  return result;
}

// Returns a block: little-endian compressed and uncompressed counts, then the
// payload.
std::vector<uint8_t> Block(int comp_count, int uncomp_count,
                           const std::vector<uint8_t>& payload) {
  std::vector<uint8_t> block = {static_cast<uint8_t>(comp_count % 256),
                                static_cast<uint8_t>(comp_count / 256),
                                static_cast<uint8_t>(uncomp_count % 256),
                                static_cast<uint8_t>(uncomp_count / 256)};
  block.insert(block.end(), payload.begin(), payload.end());
  return block;
}

std::vector<uint8_t> Concat(std::vector<uint8_t> first,
                            const std::vector<uint8_t>& second) {
  first.insert(first.end(), second.begin(), second.end());
  return first;
}

// A header claiming far more compressed bytes than any decoder buffer holds.
std::vector<uint8_t> OversizedBlock() {
  return Block(40000, 16, std::vector<uint8_t>(40000, 0x11));
}

// Sixteen bytes of plain data for the codecs with a working compressor.
std::vector<uint8_t> Plain() {
  constexpr std::string_view kText = "0123456789abcdef";
  return {kText.begin(), kText.end()};
}

// Hand-encoded so these tests do not depend on the LCW encoder: "abc".
std::vector<uint8_t> LcwAbc() {
  return Block(5, 3, {0x83, 'a', 'b', 'c', 0x80});
}

template <class PipeType>
std::vector<uint8_t> Compress(const std::vector<uint8_t>& plain) {
  RecordingSink sink;
  PipeType pipe(CodecMode::kCompress, sink, kBlockSize);
  pipe.Write(std::as_bytes(std::span(plain)));
  pipe.Flush();
  return sink.bytes;
}

// LZW codes are little-endian 16-bit values.
std::vector<uint8_t> LzwCodes(const std::vector<int>& codes) {
  std::vector<uint8_t> bytes;
  for (const int code : codes) {
    bytes.push_back(static_cast<uint8_t>(code % 256));
    bytes.push_back(static_cast<uint8_t>(code / 256));
  }
  return bytes;
}

// Decodes `encoded` through both the pipe and the straw of one codec.
template <class PipeType, class StrawType>
void ExpectDecodes(const std::vector<uint8_t>& encoded,
                   const std::vector<uint8_t>& expected) {
  RecordingSink sink;
  PipeType pipe(CodecMode::kDecompress, sink, kBlockSize);
  pipe.Write(std::as_bytes(std::span(encoded)));
  EXPECT_FALSE(pipe.Finish()) << "pipe";
  EXPECT_EQ(sink.bytes, expected) << "pipe";

  SpanSource source(std::as_bytes(std::span(encoded)));
  StrawType straw(CodecMode::kDecompress, source, kBlockSize);
  EXPECT_EQ(Drain(straw), expected) << "straw";
  EXPECT_FALSE(straw.ok()) << "straw";
}

// Every truncation of a valid stream keeps the whole blocks before the cut,
// drops the partial one and fails, in the pipe and the straw alike.
template <class PipeType, class StrawType>
void ExpectTruncationsFail() {
  std::vector<uint8_t> plain(300);
  for (int i = 0; auto& byte : plain) {
    byte = static_cast<uint8_t>(i++ * 7);
  }
  const std::vector<uint8_t> encoded = Compress<PipeType>(plain);
  const auto count_at = [&encoded](base::ssize at) {
    return encoded[static_cast<std::size_t>(at)] +
           (encoded[static_cast<std::size_t>(at + 1)] * 256);
  };
  const base::ssize first_block_end = 4 + count_at(0);
  for (const base::ssize cut : {base::ssize{2}, base::ssize{4}, base::ssize{9},
                                first_block_end + 1, std::ssize(encoded) - 1}) {
    const std::vector<uint8_t> truncated(encoded.begin(),
                                         encoded.begin() + cut);
    // Only blocks that end before the cut decode.
    base::ssize decoded = 0;
    for (base::ssize at = 0; at + 4 <= cut;) {
      const base::ssize block_end = at + 4 + count_at(at);
      if (block_end > cut) {
        break;
      }
      decoded += count_at(at + 2);
      at = block_end;
    }
    const std::vector<uint8_t> expected(plain.begin(), plain.begin() + decoded);
    SCOPED_TRACE(cut);
    ExpectDecodes<PipeType, StrawType>(truncated, expected);
  }
}

TEST(CodecCorruptTest, TruncatedStreamsFailInBothDirections) {
  ExpectTruncationsFail<LcwSink, LcwSource>();
  ExpectTruncationsFail<LzwSink, LzwSource>();
  ExpectTruncationsFail<LzoSink, LzoSource>();
}

TEST(CodecCorruptTest, LcwRejectsOversizedCompressedCount) {
  ExpectDecodes<LcwSink, LcwSource>(OversizedBlock(), {});
}

TEST(CodecCorruptTest, LzwRejectsOversizedCompressedCount) {
  ExpectDecodes<LzwSink, LzwSource>(OversizedBlock(), {});
}

TEST(CodecCorruptTest, LzoRejectsOversizedCompressedCount) {
  ExpectDecodes<LzoSink, LzoSource>(OversizedBlock(), {});
}

TEST(CodecCorruptTest, LcwKeepsValidBlockBeforeCorruptOne) {
  ExpectDecodes<LcwSink, LcwSource>(Concat(LcwAbc(), OversizedBlock()),
                                    {'a', 'b', 'c'});
}

TEST(CodecCorruptTest, LzwKeepsValidBlockBeforeCorruptOne) {
  ExpectDecodes<LzwSink, LzwSource>(
      Concat(Compress<LzwSink>(Plain()), OversizedBlock()), Plain());
}

TEST(CodecCorruptTest, LzoKeepsValidBlockBeforeCorruptOne) {
  ExpectDecodes<LzoSink, LzoSource>(
      Concat(Compress<LzoSink>(Plain()), OversizedBlock()), Plain());
}

TEST(CodecCorruptTest, RejectsOversizedUncompressedCount) {
  // 0x9c40 = 40000 bytes, far beyond every decoder's output buffer.
  std::vector<uint8_t> lcw = LcwAbc();
  lcw[2] = 0x40;
  lcw[3] = 0x9c;
  ExpectDecodes<LcwSink, LcwSource>(lcw, {});

  std::vector<uint8_t> lzw = Compress<LzwSink>(Plain());
  lzw[2] = 0x40;
  lzw[3] = 0x9c;
  ExpectDecodes<LzwSink, LzwSource>(lzw, {});

  std::vector<uint8_t> lzo = Compress<LzoSink>(Plain());
  lzo[2] = 0x40;
  lzo[3] = 0x9c;
  ExpectDecodes<LzoSink, LzoSource>(lzo, {});
}

TEST(CodecCorruptTest, RejectsEmptyCompressedBlock) {
  const std::vector<uint8_t> empty = Block(0, 16, {});
  ExpectDecodes<LcwSink, LcwSource>(empty, {});
  ExpectDecodes<LzwSink, LzwSource>(empty, {});
  ExpectDecodes<LzoSink, LzoSource>(empty, {});
}

TEST(CodecCorruptTest, LcwRejectsRunPastCapacity) {
  // A 40000-byte run from a 5-byte payload with a plausible header.
  ExpectDecodes<LcwSink, LcwSource>(Block(5, 16, {0xfe, 0x40, 0x9c, 'x', 0x80}),
                                    {});
}

TEST(CodecCorruptTest, LcwRejectsCopyFromBeforeOutput) {
  // A short copy 5 bytes back when nothing has been written yet.
  ExpectDecodes<LcwSink, LcwSource>(Block(3, 3, {0x00, 0x05, 0x80}), {});
}

TEST(CodecCorruptTest, LcwRejectsMissingEndMarker) {
  ExpectDecodes<LcwSink, LcwSource>(Block(2, 1, {0x81, 'a'}), {});
}

TEST(CodecCorruptTest, LzwRejectsExpansionPastCapacity) {
  // Each code names the entry about to be defined, so the decoded strings
  // grow by one byte per code: 254 bytes of codes expand to about 8000.
  std::vector<int> codes = {'x'};
  for (int code = 257; code < 382; ++code) {
    codes.push_back(code);
  }
  codes.push_back(256);  // End of stream.
  const std::vector<uint8_t> payload = LzwCodes(codes);
  ExpectDecodes<LzwSink, LzwSource>(
      Block(static_cast<int>(payload.size()), 200, payload), {});
}

TEST(CodecCorruptTest, LzwRejectsCodeBeyondDictionary) {
  const std::vector<uint8_t> payload = LzwCodes({'x', 30000, 258, 256});
  ExpectDecodes<LzwSink, LzwSource>(
      Block(static_cast<int>(payload.size()), 3, payload), {});
}

TEST(CodecCorruptTest, LzoRejectsMatchBeforeOutputStart) {
  // A one-byte literal, then an M2 match reaching 136 bytes back, then the end
  // marker. The header gives the length the unchecked decoder produced while
  // copying bytes from before its output buffer.
  ExpectDecodes<LzoSink, LzoSource>(
      Block(7, 5, {18, 'x', 0x7c, 0x10, 0x11, 0x00, 0x00}), {});
}

TEST(CodecCorruptTest, LzoRejectsExpansionPastCapacity) {
  // A one-byte literal, then an M3 match repeating it about 61000 times: the
  // header and payload fit the buffers but the output does not.
  std::vector<uint8_t> payload = {18, 'x', 32};
  payload.insert(payload.end(), 240, 0x00);
  payload.insert(payload.end(), {0x01, 0x00, 0x00, 0x11, 0x00, 0x00});
  ExpectDecodes<LzoSink, LzoSource>(
      Block(static_cast<int>(payload.size()), 16, payload), {});
}

}  // namespace
