// Regression coverage for initialized codec buffers and block headers.
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

#include "gtest/gtest.h"
#include "tech/b64straw.h"
#include "tech/lcwpipe.h"
#include "tech/lcwstraw.h"
#include "tech/lzopipe.h"
#include "tech/lzostraw.h"
#include "tech/lzwpipe.h"
#include "tech/lzwstraw.h"
#include "tech/pipe.h"
#include "tech/sha.h"
#include "tech/straw.h"
#include "tech/xstraw.h"

namespace {
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

template <class CodecPipe, class CodecStraw>
void CheckBlocks() {
  // Multiple full blocks and a one-byte tail exercise both header states.
  std::array<uint8_t, 513> source{};
  for (int i = 0; i < static_cast<int>(source.size()); ++i) source[i] = i % 7;
  ByteSink encoded;
  CodecPipe compressor(CodecPipe::COMPRESS, 128);
  compressor.SetSink(encoded);
  for (const auto& byte : source) compressor.Put(&byte, 1);
  compressor.Flush();
  ASSERT_FALSE(encoded.bytes.empty());
  BufferStraw compressed(encoded.bytes.data(), static_cast<int>(encoded.bytes.size()));
  CodecStraw decompressor(CodecStraw::DECOMPRESS, 128);
  decompressor.SetSource(compressed);
  const std::vector<uint8_t> expected(source.begin(), source.end());
  EXPECT_EQ(Drain(decompressor), expected);

  BufferStraw plain(source.data(), static_cast<int>(source.size()));
  CodecStraw compressing_straw(CodecStraw::COMPRESS, 128);
  compressing_straw.SetSource(plain);
  const auto straw_encoded = Drain(compressing_straw);
  EXPECT_EQ(straw_encoded, encoded.bytes);
  ByteSink decoded;
  CodecPipe decompressing_pipe(CodecPipe::DECOMPRESS, 128);
  decompressing_pipe.SetSink(decoded);
  for (const auto& byte : straw_encoded) decompressing_pipe.Put(&byte, 1);
  decompressing_pipe.Flush();
  EXPECT_EQ(decoded.bytes, expected);
}

TEST(CodecStateTest, LcwDecodesLiteralAndRunBlocksWithFragmentedHeaders) {
  // LCW compression is a stub; exercise both decoders with independent blocks.
  const std::array<uint8_t, 18> encoded = {
      5, 0, 3, 0, 0x83, 'a', 'b', 'c', 0x80,
      5, 0, 5, 0, 0xfe, 5, 0, 'x', 0x80};
  const std::vector<uint8_t> expected = {'a', 'b', 'c', 'x', 'x', 'x', 'x', 'x'};
  BufferStraw source(encoded.data(), static_cast<int>(encoded.size()));
  LCWStraw straw(LCWStraw::DECOMPRESS, 128);
  straw.SetSource(source);
  EXPECT_EQ(Drain(straw), expected);
  ByteSink decoded;
  LCWPipe pipe(LCWPipe::DECOMPRESS, 128);
  pipe.SetSink(decoded);
  for (const auto& byte : encoded) pipe.Put(&byte, 1);
  pipe.Flush();
  EXPECT_EQ(decoded.bytes, expected);
}
TEST(CodecStateTest, LzoHandlesFragmentedHeadersAndPartialBlocks) {
  CheckBlocks<LZOPipe, LZOStraw>();
}
TEST(CodecStateTest, LzwHandlesFragmentedHeadersAndPartialBlocks) {
  CheckBlocks<LZWPipe, LZWStraw>();
}

TEST(CodecStateTest, Base64HandlesShortFinalGroups) {
  constexpr const char* expected[] = {"YQ==", "YWI=", "YWJj", "YWJjZA==", "YWJjZGU="};
  for (int length : {1, 2, 3, 4, 5}) {
    constexpr char input[] = "abcde";
    BufferStraw plain(input, length);
    Base64Straw encoder(Base64Straw::ENCODE);
    encoder.SetSource(plain);
    const auto bytes = Drain(encoder);
    ASSERT_EQ(bytes.size(), std::strlen(expected[length - 1]));
    EXPECT_EQ(std::memcmp(bytes.data(), expected[length - 1], bytes.size()), 0);
    BufferStraw encoded(bytes.data(), static_cast<int>(bytes.size()));
    Base64Straw decoder(Base64Straw::DECODE);
    decoder.SetSource(encoded);
    const auto decoded = Drain(decoder);
    ASSERT_EQ(decoded.size(), length);
    EXPECT_EQ(std::memcmp(decoded.data(), input, length), 0);
  }
}

TEST(CodecStateTest, ShaResetRestoresKnownDigestAfterPartialInput) {
  SHAEngine hash;
  hash.Hash("discard this partial block", 26);
  std::array<uint8_t, 20> discarded{};
  hash.Result(discarded.data());
  hash.Init();
  hash.Hash("a", 1);
  hash.Hash("bc", 2);
  std::array<uint8_t, 20> actual{};
  EXPECT_EQ(hash.Result(actual.data()), 20);
  EXPECT_EQ(std::memcmp(actual.data(), SHA_DIGEST1a, actual.size()), 0);
  std::array<uint8_t, 20> cached{};
  hash.Result(cached.data());
  EXPECT_EQ(cached, actual);
}
}  // namespace
