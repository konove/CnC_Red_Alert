// Regression coverage for initialized codec buffers and block headers.
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <span>
#include <string_view>
#include <vector>

#include "base/buffer.h"
#include "base/types.h"
#include "gtest/gtest.h"
#include "tech/base64.h"
#include "tech/base64_source.h"
#include "tech/block_codec.h"
#include "tech/blowfish.h"
#include "tech/blowfish_source.h"
#include "tech/byte_sink.h"
#include "tech/byte_source.h"
#include "tech/lcw.h"
#include "tech/lcw_sink.h"
#include "tech/lcw_source.h"
#include "tech/lzo_sink.h"
#include "tech/lzo_source.h"
#include "tech/lzw_sink.h"
#include "tech/lzw_source.h"
#include "tech/mp.h"
#include "tech/sha.h"
#include "tech/sha1_source.h"
#include "tech/span_source.h"

namespace {
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
    std::ranges::copy(std::span(chunk).first(static_cast<std::size_t>(count)),
                      std::back_inserter(result));
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
  RecordingSink encoded;
  CodecPipe compressor(CodecMode::kCompress, encoded, 128);
  for (const auto& byte : source) {
    compressor.WriteObject(byte);
  }
  compressor.Flush();
  ASSERT_FALSE(encoded.bytes.empty());
  SpanSource compressed(std::as_bytes(std::span(encoded.bytes)));
  CodecStraw decompressor(CodecMode::kDecompress, compressed, 128);
  const std::vector<uint8_t> expected(source.begin(), source.end());
  EXPECT_EQ(Drain(decompressor), expected);

  SpanSource plain(std::as_bytes(std::span(source)));
  CodecStraw compressing_straw(CodecMode::kCompress, plain, 128);
  const auto straw_encoded = Drain(compressing_straw);
  EXPECT_EQ(straw_encoded, encoded.bytes);
  RecordingSink decoded;
  CodecPipe decompressing_pipe(CodecMode::kDecompress, decoded, 128);
  for (const auto& byte : straw_encoded) {
    decompressing_pipe.WriteObject(byte);
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
  SpanSource source(std::as_bytes(std::span(encoded)));
  LcwSource straw(CodecMode::kDecompress, source, 128);
  EXPECT_EQ(Drain(straw), expected);
  RecordingSink decoded;
  LcwSink pipe(CodecMode::kDecompress, decoded, 128);
  for (const auto& byte : encoded) {
    pipe.WriteObject(byte);
  }
  pipe.Flush();
  EXPECT_EQ(decoded.bytes, expected);
}
TEST(CodecStateTest, LzoHandlesFragmentedHeadersAndPartialBlocks) {
  CheckBlocks<LzoSink, LzoSource>();
}
TEST(CodecStateTest, LzwHandlesFragmentedHeadersAndPartialBlocks) {
  CheckBlocks<LzwSink, LzwSource>();
}

TEST(CodecStateTest, Base64HandlesShortFinalGroups) {
  constexpr std::array<std::string_view, 5> expected = {"YQ==", "YWI=", "YWJj",
                                                        "YWJjZA==", "YWJjZGU="};
  for (const int length : {1, 2, 3, 4, 5}) {
    constexpr char input[] = "abcde";
    SpanSource plain(std::as_bytes(
        std::span(input).first(static_cast<std::size_t>(length))));
    Base64Source encoder(Base64Mode::kEncode, plain);
    const auto bytes = Drain(encoder);
    ASSERT_EQ(bytes.size(),
              expected[static_cast<std::size_t>(length - 1)].size());
    EXPECT_EQ(
        base::CompareBytes(std::as_bytes(std::span(bytes)),
                           std::as_bytes(std::span(
                               expected[static_cast<std::size_t>(length - 1)])),
                           bytes.size()),
        0);
    SpanSource encoded(std::as_bytes(std::span(bytes)));
    Base64Source decoder(Base64Mode::kDecode, encoded);
    const auto decoded = Drain(decoder);
    ASSERT_EQ(std::ssize(decoded), length);
    EXPECT_EQ(base::CompareBytes(std::as_bytes(std::span(decoded)),
                                 base::ObjectBytes(input), length),
              0);
  }
}

TEST(CodecStateTest, ShaResetRestoresKnownDigestAfterPartialInput) {
  SHAEngine hash;
  hash.Hash(std::as_bytes(std::span("discard this partial block")).first(26));
  static_cast<void>(hash.Digest());
  hash.Init();
  hash.Hash(std::as_bytes(std::span("a")).first(1));
  hash.Hash(std::as_bytes(std::span("bc")).first(2));
  const Sha1Digest actual = hash.Digest();
  EXPECT_EQ(base::CompareBytes(actual, base::ObjectBytes(SHA_DIGEST1a),
                               actual.size()),
            0);
  EXPECT_EQ(hash.Digest(), actual);
}
}  // namespace

TEST(CodecStateTest, LcwLongRunsRespectTheirLengthAtEveryAlignment) {
  alignas(uint32_t) std::array<uint8_t, 48> output{};
  for (int offset = 0; offset < 4; ++offset) {
    for (int length = 4; length <= 20; ++length) {
      output.fill(0xa5);
      const std::array<uint8_t, 5> encoded = {
          0xfe, static_cast<uint8_t>(length), 0, 0x6b, 0x80};
      EXPECT_EQ(
          LcwUncompBounded(std::as_bytes(std::span(encoded)),
                           std::as_writable_bytes(std::span(output))
                               .subspan(static_cast<std::size_t>(offset),
                                        static_cast<std::size_t>(length))),
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
  ASSERT_EQ(XMP_Prepare_Modulus(modulus, kPrecision), 0);
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
    ASSERT_EQ(XMP_Mod_Mult(result, left, right, kPrecision), 0);
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
  CheckBlocks<LcwSink, LcwSource>();
}

TEST(CodecStateTest, LzwRoundTripsIncompressibleBlocks) {
  // 97 is coprime with 256, so every byte differs and no pair ever repeats:
  // the encoder emits a code per byte, the worst case for its output size.
  std::array<uint8_t, 256> source{};
  for (int i = 0; auto& byte : source) {
    byte = static_cast<uint8_t>((i++ * 97) % 256);
  }
  const std::vector<uint8_t> expected(source.begin(), source.end());

  RecordingSink encoded;
  LzwSink compressor(CodecMode::kCompress, encoded, 128);
  compressor.Write(std::as_bytes(std::span(source)));
  compressor.Flush();
  SpanSource compressed(std::as_bytes(std::span(encoded.bytes)));
  LzwSource decompressor(CodecMode::kDecompress, compressed, 128);
  EXPECT_EQ(Drain(decompressor), expected);

  SpanSource plain(std::as_bytes(std::span(source)));
  LzwSource compressing_straw(CodecMode::kCompress, plain, 128);
  const std::vector<uint8_t> straw_encoded = Drain(compressing_straw);
  EXPECT_EQ(straw_encoded, encoded.bytes);
  RecordingSink decoded;
  LzwSink decompressing_pipe(CodecMode::kDecompress, decoded, 128);
  decompressing_pipe.Write(std::as_bytes(std::span(straw_encoded)));
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
  SpanSource source(std::as_bytes(std::span(data)));
  BlowfishSource straw(CipherMode::kDecrypt, source);
  std::array<uint8_t, 5> head{};
  ASSERT_EQ(straw.Read(std::as_writable_bytes(std::span(head))), 5);
  EXPECT_TRUE(std::equal(head.begin(), head.end(), data.begin()));

  std::array<uint8_t, 64> rest{};
  ASSERT_EQ(source.Read(std::as_writable_bytes(std::span(rest))), 27);
  EXPECT_TRUE(std::equal(std::span(data).subspan(5).begin(),
                         std::span(data).subspan(5).end(), rest.begin()));
}

TEST(CodecStateTest, ShaStrawHashesOnlyTheBytesItReturned) {
  std::array<uint8_t, 32> data{};
  for (int i = 0; auto& byte : data) {
    byte = static_cast<uint8_t>(i++ * 3);
  }
  SpanSource source(std::as_bytes(std::span(data)));
  Sha1Source sha(source);
  std::array<uint8_t, 5> head{};
  ASSERT_EQ(sha.Read(std::as_writable_bytes(std::span(head))), 5);

  SHAEngine expected_engine;
  expected_engine.Hash(std::as_bytes(std::span(data)).first(5));
  EXPECT_EQ(sha.digest(), expected_engine.Digest());

  std::array<uint8_t, 64> rest{};
  ASSERT_EQ(source.Read(std::as_writable_bytes(std::span(rest))), 27);
  EXPECT_TRUE(std::equal(std::span(data).subspan(5).begin(),
                         std::span(data).subspan(5).end(), rest.begin()));
}

// A pull link reads only as much of its source as its codec can use, so the
// source position after a partial read is the same whichever straw or
// adapter implements it.
TEST(CodecStateTest, SourcesReadOnlyWhatTheirCodecNeeds) {
  std::vector<uint8_t> plain(300);
  for (int i = 0; auto& byte : plain) {
    byte = static_cast<uint8_t>(i++ * 5);
  }
  {
    // A block header, then that block's compressed bytes.
    RecordingSink encoded;
    LzoSink compressor(CodecMode::kCompress, encoded, 128);
    compressor.Write(std::as_bytes(std::span(plain)));
    compressor.Finish();
    SpanSource source(std::as_bytes(std::span(encoded.bytes)));
    LzoSource decompressor(CodecMode::kDecompress, source, 128);
    std::array<std::byte, 1> one{};
    ASSERT_EQ(decompressor.Read(one), 1);
    const int first_block = 4 + encoded.bytes[0] + (encoded.bytes[1] << 8);
    EXPECT_EQ(source.bytes_remaining(),
              std::ssize(encoded.bytes) - first_block);
  }
  {
    // One 8-byte block for a keyed cipher.
    SpanSource source(std::as_bytes(std::span(plain)));
    BlowfishSource cipher(CipherMode::kDecrypt, source);
    const std::array<char, 4> key = {'k', 'e', 'y', '!'};
    cipher.Key(std::as_bytes(std::span(key)));
    std::array<std::byte, 1> one{};
    ASSERT_EQ(cipher.Read(one), 1);
    EXPECT_EQ(source.bytes_remaining(), std::ssize(plain) - 8);
  }
  {
    // One 4-character group for Base64.
    constexpr std::string_view kEncoded = "YWJjZGVmZ2hp";
    SpanSource source(std::as_bytes(std::span(kEncoded)));
    Base64Source decoder(Base64Mode::kDecode, source);
    std::array<std::byte, 1> one{};
    ASSERT_EQ(decoder.Read(one), 1);
    EXPECT_EQ(source.bytes_remaining(), std::ssize(kEncoded) - 4);
  }
}
