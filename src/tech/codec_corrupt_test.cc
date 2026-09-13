// Tests that the block-based LCW, LZW and LZO decoders stop cleanly on corrupt
// block headers and payloads instead of reading or writing past their buffers.

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

#include "gtest/gtest.h"
#include "tech/lcwpipe.h"
#include "tech/lcwstraw.h"
#include "tech/lzopipe.h"
#include "tech/lzostraw.h"
#include "tech/lzwpipe.h"
#include "tech/lzwstraw.h"
#include "tech/pipe.h"
#include "tech/straw.h"
#include "tech/xstraw.h"

namespace {

constexpr int kBlockSize = 128;

class ByteSink : public Pipe {
 public:
  std::vector<uint8_t> bytes;
  int Put(const void* data, int length) override {
    const auto* first = static_cast<const uint8_t*>(data);
    bytes.insert(bytes.end(), first, first + length);
    return length;
  }
};

std::vector<uint8_t> Drain(Straw& straw) {
  std::vector<uint8_t> result;
  std::array<uint8_t, 7> chunk{};
  for (int count = straw.Get(chunk.data(), static_cast<int>(chunk.size()));
       count != 0;
       count = straw.Get(chunk.data(), static_cast<int>(chunk.size()))) {
    result.insert(result.end(), chunk.begin(), chunk.begin() + count);
  }
  return result;
}

// Returns a block: little-endian compressed and uncompressed counts, then the
// payload.
std::vector<uint8_t> Block(int comp_count, int uncomp_count,
                           const std::vector<uint8_t>& payload) {
  std::vector<uint8_t> block = {static_cast<uint8_t>(comp_count & 0xff),
                                static_cast<uint8_t>(comp_count >> 8),
                                static_cast<uint8_t>(uncomp_count & 0xff),
                                static_cast<uint8_t>(uncomp_count >> 8)};
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
  ByteSink sink;
  PipeType pipe(PipeType::COMPRESS, kBlockSize);
  pipe.SetSink(sink);
  pipe.Put(plain.data(), static_cast<int>(plain.size()));
  pipe.Flush();
  return sink.bytes;
}

// LZW codes are little-endian 16-bit values.
std::vector<uint8_t> LzwCodes(const std::vector<int>& codes) {
  std::vector<uint8_t> bytes;
  for (const int code : codes) {
    bytes.push_back(static_cast<uint8_t>(code & 0xff));
    bytes.push_back(static_cast<uint8_t>(code >> 8));
  }
  return bytes;
}

// Decodes `encoded` through both the pipe and the straw of one codec.
template <class PipeType, class StrawType>
void ExpectDecodes(const std::vector<uint8_t>& encoded,
                   const std::vector<uint8_t>& expected) {
  ByteSink sink;
  PipeType pipe(PipeType::DECOMPRESS, kBlockSize);
  pipe.SetSink(sink);
  pipe.Put(encoded.data(), static_cast<int>(encoded.size()));
  pipe.Flush();
  EXPECT_EQ(sink.bytes, expected) << "pipe";

  BufferStraw source(encoded.data(), static_cast<int>(encoded.size()));
  StrawType straw(StrawType::DECOMPRESS, kBlockSize);
  straw.SetSource(source);
  EXPECT_EQ(Drain(straw), expected) << "straw";
}

TEST(CodecCorruptTest, LcwRejectsOversizedCompressedCount) {
  ExpectDecodes<LCWPipe, LCWStraw>(OversizedBlock(), {});
}

TEST(CodecCorruptTest, LzwRejectsOversizedCompressedCount) {
  ExpectDecodes<LZWPipe, LZWStraw>(OversizedBlock(), {});
}

TEST(CodecCorruptTest, LzoRejectsOversizedCompressedCount) {
  ExpectDecodes<LZOPipe, LZOStraw>(OversizedBlock(), {});
}

TEST(CodecCorruptTest, LcwKeepsValidBlockBeforeCorruptOne) {
  ExpectDecodes<LCWPipe, LCWStraw>(Concat(LcwAbc(), OversizedBlock()),
                                   {'a', 'b', 'c'});
}

TEST(CodecCorruptTest, LzwKeepsValidBlockBeforeCorruptOne) {
  ExpectDecodes<LZWPipe, LZWStraw>(
      Concat(Compress<LZWPipe>(Plain()), OversizedBlock()), Plain());
}

TEST(CodecCorruptTest, LzoKeepsValidBlockBeforeCorruptOne) {
  ExpectDecodes<LZOPipe, LZOStraw>(
      Concat(Compress<LZOPipe>(Plain()), OversizedBlock()), Plain());
}

TEST(CodecCorruptTest, RejectsOversizedUncompressedCount) {
  // 0x9c40 = 40000 bytes, far beyond every decoder's output buffer.
  std::vector<uint8_t> lcw = LcwAbc();
  lcw[2] = 0x40;
  lcw[3] = 0x9c;
  ExpectDecodes<LCWPipe, LCWStraw>(lcw, {});

  std::vector<uint8_t> lzw = Compress<LZWPipe>(Plain());
  lzw[2] = 0x40;
  lzw[3] = 0x9c;
  ExpectDecodes<LZWPipe, LZWStraw>(lzw, {});

  std::vector<uint8_t> lzo = Compress<LZOPipe>(Plain());
  lzo[2] = 0x40;
  lzo[3] = 0x9c;
  ExpectDecodes<LZOPipe, LZOStraw>(lzo, {});
}

TEST(CodecCorruptTest, RejectsEmptyCompressedBlock) {
  const std::vector<uint8_t> empty = Block(0, 16, {});
  ExpectDecodes<LCWPipe, LCWStraw>(empty, {});
  ExpectDecodes<LZWPipe, LZWStraw>(empty, {});
  ExpectDecodes<LZOPipe, LZOStraw>(empty, {});
}

TEST(CodecCorruptTest, LcwRejectsRunPastCapacity) {
  // A 40000-byte run from a 5-byte payload with a plausible header.
  ExpectDecodes<LCWPipe, LCWStraw>(Block(5, 16, {0xfe, 0x40, 0x9c, 'x', 0x80}),
                                   {});
}

TEST(CodecCorruptTest, LcwRejectsCopyFromBeforeOutput) {
  // A short copy 5 bytes back when nothing has been written yet.
  ExpectDecodes<LCWPipe, LCWStraw>(Block(3, 3, {0x00, 0x05, 0x80}), {});
}

TEST(CodecCorruptTest, LcwRejectsMissingEndMarker) {
  ExpectDecodes<LCWPipe, LCWStraw>(Block(2, 1, {0x81, 'a'}), {});
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
  ExpectDecodes<LZWPipe, LZWStraw>(
      Block(static_cast<int>(payload.size()), 200, payload), {});
}

TEST(CodecCorruptTest, LzwRejectsCodeBeyondDictionary) {
  const std::vector<uint8_t> payload = LzwCodes({'x', 30000, 258, 256});
  ExpectDecodes<LZWPipe, LZWStraw>(
      Block(static_cast<int>(payload.size()), 3, payload), {});
}

TEST(CodecCorruptTest, LzoRejectsMatchBeforeOutputStart) {
  // A one-byte literal, then an M2 match reaching 136 bytes back, then the end
  // marker. The header gives the length the unchecked decoder produced while
  // copying bytes from before its output buffer.
  ExpectDecodes<LZOPipe, LZOStraw>(
      Block(7, 5, {18, 'x', 0x7c, 0x10, 0x11, 0x00, 0x00}), {});
}

TEST(CodecCorruptTest, LzoRejectsExpansionPastCapacity) {
  // A one-byte literal, then an M3 match repeating it about 61000 times: the
  // header and payload fit the buffers but the output does not.
  std::vector<uint8_t> payload = {18, 'x', 32};
  payload.insert(payload.end(), 240, 0x00);
  payload.insert(payload.end(), {0x01, 0x00, 0x00, 0x11, 0x00, 0x00});
  ExpectDecodes<LZOPipe, LZOStraw>(
      Block(static_cast<int>(payload.size()), 16, payload), {});
}

}  // namespace
