// Regression coverage for initialized codec buffers and block headers.
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <vector>

#include "gtest/gtest.h"
#include "tech/b64straw.h"
#include "tech/blwstraw.h"
#include "tech/lcw.h"
#include "tech/lcwpipe.h"
#include "tech/lcwstraw.h"
#include "tech/lzopipe.h"
#include "tech/lzostraw.h"
#include "tech/lzwpipe.h"
#include "tech/lzwstraw.h"
#include "tech/mp.h"
#include "tech/pipe.h"
#include "tech/sha.h"
#include "tech/shastraw.h"
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
  for (int i = 0; auto& byte : source) {
    byte = static_cast<uint8_t>(i++ % 7);
  }
  ByteSink encoded;
  CodecPipe compressor(CodecPipe::COMPRESS, 128);
  compressor.SetSink(encoded);
  for (const auto& byte : source) {
    compressor.Put(&byte, 1);
  }
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
  for (const auto& byte : straw_encoded) {
    decompressing_pipe.Put(&byte, 1);
  }
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
  for (const auto& byte : encoded) {
    pipe.Put(&byte, 1);
  }
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
  for (const int length : {1, 2, 3, 4, 5}) {
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
    ASSERT_EQ(std::ssize(decoded), length);
    EXPECT_EQ(
        std::memcmp(decoded.data(), input, static_cast<std::size_t>(length)),
        0);
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

TEST(CodecStateTest, LcwLongRunsRespectTheirLengthAtEveryAlignment) {
  alignas(uint32_t) std::array<uint8_t, 48> output{};
  for (int offset = 0; offset < 4; ++offset) {
    for (int length = 4; length <= 20; ++length) {
      output.fill(0xa5);
      const std::array<uint8_t, 5> encoded = {
          0xfe, static_cast<uint8_t>(length), 0, 0x6b, 0x80};
      EXPECT_EQ(LCW_Uncomp(encoded.data(), output.data() + offset, length),
                length);
      for (int i = 0; const uint8_t byte : output) {
        EXPECT_EQ(byte, i >= offset && i < offset + length ? 0x6b : 0xa5);
        ++i;
      }
    }
  }
}

TEST(CodecStateTest, ModularMultiplicationMatchesIndependentRemainder) {
  // Three 16-bit digits exercise reduction windows with half-word offsets.
  constexpr uint64_t kModulus = 0x10000000f;
  constexpr int kPrecision = 3;
  const std::array<uint32_t, kPrecision> modulus = {15, 1, 0};
  ASSERT_EQ(XMP_Prepare_Modulus(modulus.data(), kPrecision), 0);
  uint64_t seed = 17;
  for (int trial = 0; trial < 256; ++trial) {
    seed = ((seed * 1664525) + 1013904223) % kModulus;
    const uint64_t a = seed;
    seed = ((seed * 1664525) + 1013904223) % kModulus;
    const uint64_t b = seed;
    const std::array<uint32_t, kPrecision> left = {
        static_cast<uint32_t>(a), static_cast<uint32_t>(a >> 32), 0};
    const std::array<uint32_t, kPrecision> right = {
        static_cast<uint32_t>(b), static_cast<uint32_t>(b >> 32), 0};
    std::array<uint32_t, kPrecision> result{};
    ASSERT_EQ(
        XMP_Mod_Mult(result.data(), left.data(), right.data(), kPrecision), 0);
    // Repeated doubling avoids overflowing a native 64-bit product.
    uint64_t expected = 0;
    uint64_t addend = a;
    for (uint64_t multiplier = b; multiplier; multiplier >>= 1) {
      if (multiplier & 1) {
        expected = (expected + addend) % kModulus;
      }
      addend = (addend * 2) % kModulus;
    }
    EXPECT_EQ(result[0], static_cast<uint32_t>(expected));
    EXPECT_EQ(result[1], static_cast<uint32_t>(expected >> 32));
    EXPECT_EQ(result[2], 0);
  }
  XMP_Mod_Mult_Clear(kPrecision);
}

TEST(CodecStateTest, LcwHandlesFragmentedHeadersAndPartialBlocks) {
  CheckBlocks<LCWPipe, LCWStraw>();
}

TEST(CodecStateTest, LzwRoundTripsIncompressibleBlocks) {
  // 97 is coprime with 256, so every byte differs and no pair ever repeats:
  // the encoder emits a code per byte, the worst case for its output size.
  std::array<uint8_t, 256> source{};
  for (int i = 0; auto& byte : source) {
    byte = static_cast<uint8_t>((i++ * 97) % 256);
  }
  const std::vector<uint8_t> expected(source.begin(), source.end());

  ByteSink encoded;
  LZWPipe compressor(LZWPipe::COMPRESS, 128);
  compressor.SetSink(encoded);
  compressor.Put(source.data(), static_cast<int>(source.size()));
  compressor.Flush();
  BufferStraw compressed(encoded.bytes.data(),
                         static_cast<int>(encoded.bytes.size()));
  LZWStraw decompressor(LZWStraw::DECOMPRESS, 128);
  decompressor.SetSource(compressed);
  EXPECT_EQ(Drain(decompressor), expected);

  BufferStraw plain(source.data(), static_cast<int>(source.size()));
  LZWStraw compressing_straw(LZWStraw::COMPRESS, 128);
  compressing_straw.SetSource(plain);
  const std::vector<uint8_t> straw_encoded = Drain(compressing_straw);
  EXPECT_EQ(straw_encoded, encoded.bytes);
  ByteSink decoded;
  LZWPipe decompressing_pipe(LZWPipe::DECOMPRESS, 128);
  decompressing_pipe.SetSink(decoded);
  decompressing_pipe.Put(straw_encoded.data(),
                         static_cast<int>(straw_encoded.size()));
  decompressing_pipe.Flush();
  EXPECT_EQ(decoded.bytes, expected);
}

// A pass-through straw link must not read ahead of its caller: the mixfile
// header decode leaves the file positioned for whatever reads it next.
TEST(CodecStateTest, UnkeyedBlowStrawReadsOnlyWhatWasRequested) {
  std::array<uint8_t, 32> data{};
  for (int i = 0; auto& byte : data) {
    byte = static_cast<uint8_t>(i++);
  }
  BufferStraw source(data.data(), static_cast<int>(data.size()));
  BlowStraw straw(BlowStraw::DECRYPT);
  straw.SetSource(source);
  std::array<uint8_t, 5> head{};
  ASSERT_EQ(straw.Get(head.data(), static_cast<int>(head.size())), 5);
  EXPECT_TRUE(std::equal(head.begin(), head.end(), data.begin()));

  std::array<uint8_t, 64> rest{};
  ASSERT_EQ(source.Get(rest.data(), static_cast<int>(rest.size())), 27);
  EXPECT_TRUE(std::equal(data.begin() + 5, data.end(), rest.begin()));
}

TEST(CodecStateTest, ShaStrawHashesOnlyTheBytesItReturned) {
  std::array<uint8_t, 32> data{};
  for (int i = 0; auto& byte : data) {
    byte = static_cast<uint8_t>(i++ * 3);
  }
  BufferStraw source(data.data(), static_cast<int>(data.size()));
  SHAStraw sha;
  sha.SetSource(source);
  std::array<uint8_t, 5> head{};
  ASSERT_EQ(sha.Get(head.data(), static_cast<int>(head.size())), 5);

  SHAEngine expected_engine;
  expected_engine.Hash(data.data(), 5);
  std::array<uint8_t, 20> expected{};
  expected_engine.Result(expected.data());
  std::array<uint8_t, 20> actual{};
  sha.Result(actual.data());
  EXPECT_EQ(actual, expected);

  std::array<uint8_t, 64> rest{};
  ASSERT_EQ(source.Get(rest.data(), static_cast<int>(rest.size())), 27);
  EXPECT_TRUE(std::equal(data.begin() + 5, data.end(), rest.begin()));
}
