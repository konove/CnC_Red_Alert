// Pins the exact bytes the codec sinks and sources produce today, so the
// stream refactor can prove that saved games, mixfile decryption and INI
// binary blocks stay byte-identical.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "absl/strings/str_cat.h"
#include "base/types.h"
#include "gtest/gtest.h"
#include "tech/base64.h"
#include "tech/base64_sink.h"
#include "tech/base64_source.h"
#include "tech/block_codec.h"
#include "tech/blowfish.h"
#include "tech/blowfish_sink.h"
#include "tech/blowfish_source.h"
#include "tech/byte_sink.h"
#include "tech/byte_source.h"
#include "tech/lcw_sink.h"
#include "tech/lcw_source.h"
#include "tech/lzo_sink.h"
#include "tech/lzo_source.h"
#include "tech/lzw_sink.h"
#include "tech/lzw_source.h"
#include "tech/sha.h"
#include "tech/sha1_sink.h"
#include "tech/span_sink.h"
#include "tech/span_source.h"

namespace {

// Not a multiple of any block size, so every codec ends on a partial block
// and Blowfish leaves a three-byte tail.
constexpr int kInputSize = 100003;

// SAVE_BLOCK_SIZE in ra/saveload.cc.
constexpr int kSaveBlockSize = 4096;

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

std::vector<uint8_t> Drain(ByteSource& straw, int chunk_size) {
  std::vector<uint8_t> result;
  std::vector<uint8_t> chunk(static_cast<std::size_t>(chunk_size));
  for (base::ssize count = straw.Read(std::as_writable_bytes(std::span(chunk)));
       count != 0;
       count = straw.Read(std::as_writable_bytes(std::span(chunk)))) {
    result.insert(result.end(), chunk.begin(), chunk.begin() + count);
  }
  return result;
}

// Save-game-like input: runs and repeats that compress, mixed with noise
// that does not.
std::vector<uint8_t> Input() {
  std::vector<uint8_t> bytes;
  bytes.reserve(kInputSize);
  uint32_t seed = 12345;
  while (bytes.size() < kInputSize) {
    seed = (seed * 1664525U) + 1013904223U;
    const auto run = static_cast<std::size_t>((seed >> 8) % 40) + 1;
    if ((seed >> 20) % 3 == 0) {
      bytes.insert(bytes.end(), run, static_cast<uint8_t>(seed >> 24));
    } else {
      for (std::size_t i = 0; i < run; ++i) {
        seed = (seed * 1664525U) + 1013904223U;
        bytes.push_back(static_cast<uint8_t>(seed >> 24));
      }
    }
  }
  bytes.resize(kInputSize);
  return bytes;
}

// Feeds `bytes` to `pipe` in uneven pieces, then flushes it.
void PutInPieces(ByteSink& pipe, const std::vector<uint8_t>& bytes) {
  for (std::size_t at = 0; at < bytes.size(); at += 997) {
    const std::size_t piece = std::min<std::size_t>(997, bytes.size() - at);
    pipe.Write(std::as_bytes(std::span(bytes).subspan(at, piece)));
  }
  pipe.Flush();
}

std::string Sha1Hex(const std::vector<uint8_t>& bytes) {
  SHAEngine sha;
  sha.Hash(bytes.data(), static_cast<int32_t>(bytes.size()));
  std::string hex;
  for (const std::byte byte : sha.Digest()) {
    absl::StrAppend(&hex,
                    absl::Hex(std::to_integer<int>(byte), absl::kZeroPad2));
  }
  return hex;
}

std::array<uint8_t, BlowfishEngine::kMaxKeyLength> Key() {
  std::array<uint8_t, BlowfishEngine::kMaxKeyLength> key{};
  for (int i = 0; auto& byte : key) {
    byte = static_cast<uint8_t>((i++ * 37) + 11);
  }
  return key;
}

TEST(StreamGoldenTest, InputIsStable) {
  EXPECT_EQ(Sha1Hex(Input()), "d70ffc4eff00292de2b8f47a153ed1687b6d6777");
}

template <class CodecPipe, class CodecStraw>
void ExpectCompresses(int block_size, const std::string& golden) {
  const std::vector<uint8_t> input = Input();
  RecordingSink encoded;
  CodecPipe compressor(CodecMode::kCompress, encoded, block_size);
  PutInPieces(compressor, input);
  EXPECT_EQ(Sha1Hex(encoded.bytes), golden);

  SpanSource plain(std::as_bytes(std::span(input)));
  CodecStraw compressing_straw(CodecMode::kCompress, plain, block_size);
  EXPECT_EQ(Drain(compressing_straw, 1000), encoded.bytes);

  SpanSource compressed(std::as_bytes(std::span(encoded.bytes)));
  CodecStraw decompressor(CodecMode::kDecompress, compressed, block_size);
  EXPECT_EQ(Drain(decompressor, 1000), input);

  RecordingSink decoded;
  CodecPipe decompressing_pipe(CodecMode::kDecompress, decoded, block_size);
  PutInPieces(decompressing_pipe, encoded.bytes);
  EXPECT_EQ(decoded.bytes, input);
}

TEST(StreamGoldenTest, LzoCompressesSaveBlocks) {
  ExpectCompresses<LzoSink, LzoSource>(
      kSaveBlockSize, "24f6acf295db6c5e04d63ba3a3c4769d81fa6d7e");
}

// The map and overlay packs use the default 8192-byte block.
TEST(StreamGoldenTest, LcwCompressesDefaultBlocks) {
  ExpectCompresses<LcwSink, LcwSource>(
      8192, "391841f830f9870d42d09cdde75435e7aa71d3fd");
}

TEST(StreamGoldenTest, LzwCompressesSaveBlocks) {
  ExpectCompresses<LzwSink, LzwSource>(
      kSaveBlockSize, "366f846b06db2bff6969bf91aa632f586501c8a5");
}

TEST(StreamGoldenTest, BlowfishEncryptsWithFixedKey) {
  const std::vector<uint8_t> input = Input();
  const auto key = Key();
  RecordingSink encrypted;
  BlowfishSink encryptor(CipherMode::kEncrypt, encrypted);
  encryptor.Key(key.data(), static_cast<int>(key.size()));
  PutInPieces(encryptor, input);
  EXPECT_EQ(Sha1Hex(encrypted.bytes),
            "2fea48225f216a5c044f307a870ae4b2a88a8afc");

  SpanSource plain(std::as_bytes(std::span(input)));
  BlowfishSource encrypting_straw(CipherMode::kEncrypt, plain);
  encrypting_straw.Key(key.data(), static_cast<int>(key.size()));
  EXPECT_EQ(Drain(encrypting_straw, 1000), encrypted.bytes);

  SpanSource cipher(std::as_bytes(std::span(encrypted.bytes)));
  BlowfishSource decryptor(CipherMode::kDecrypt, cipher);
  decryptor.Key(key.data(), static_cast<int>(key.size()));
  EXPECT_EQ(Drain(decryptor, 1000), input);
}

// The save-game body: LZO, then Blowfish, then a SHA-1 tap before the file.
TEST(StreamGoldenTest, SaveGameChainProducesPinnedBytesAndDigest) {
  const std::vector<uint8_t> input = Input();
  const auto key = Key();
  RecordingSink file;
  Sha1Sink sha(file);
  BlowfishSink blow(CipherMode::kEncrypt, sha);
  LzoSink lzo(CodecMode::kCompress, blow, kSaveBlockSize);
  blow.Key(key.data(), static_cast<int>(key.size()));
  PutInPieces(lzo, input);
  EXPECT_EQ(Sha1Hex(file.bytes), "2338b443f754c7b2e6e587dab7c29deff557d0b1");

  // The digest written into the save covers the stream exactly as stored.
  SHAEngine stored_hash;
  stored_hash.Hash(file.bytes.data(), static_cast<int32_t>(file.bytes.size()));
  EXPECT_EQ(sha.digest(), stored_hash.Digest());

  SpanSource stored(std::as_bytes(std::span(file.bytes)));
  BlowfishSource decrypt(CipherMode::kDecrypt, stored);
  LzoSource decompress(CodecMode::kDecompress, decrypt, kSaveBlockSize);
  decrypt.Key(key.data(), static_cast<int>(key.size()));
  EXPECT_EQ(Drain(decompress, 1000), input);
}

// INIClass::Put_UUBlock draws 70 characters at a time for each entry, and
// Get_UUBlock pushes the entries back through a decoding pipe.
TEST(StreamGoldenTest, Base64EncodesInUuBlockLines) {
  const std::vector<uint8_t> input = Input();
  SpanSource plain(std::as_bytes(std::span(input)));
  Base64Source encoder(Base64Mode::kEncode, plain);
  std::vector<std::vector<uint8_t>> lines;
  std::vector<uint8_t> joined;
  for (;;) {
    std::array<uint8_t, 70> line{};
    const auto length =
        static_cast<int>(encoder.Read(std::as_writable_bytes(std::span(line))));
    if (length == 0) {
      break;
    }
    lines.emplace_back(line.begin(), line.begin() + length);
    joined.insert(joined.end(), line.begin(), line.begin() + length);
    joined.push_back('\n');
  }
  EXPECT_EQ(Sha1Hex(joined), "228aa04caf47a421738b2d0186b6a379b1b23fbd");

  RecordingSink pipe_encoded;
  Base64Sink encoding_pipe(Base64Mode::kEncode, pipe_encoded);
  PutInPieces(encoding_pipe, input);
  std::vector<uint8_t> unsplit;
  for (const auto& line : lines) {
    unsplit.insert(unsplit.end(), line.begin(), line.end());
  }
  EXPECT_EQ(pipe_encoded.bytes, unsplit);

  std::vector<uint8_t> block(input.size() + 16);
  SpanSink block_pipe(std::as_writable_bytes(std::span(block)));
  Base64Sink decoding_pipe(Base64Mode::kDecode, block_pipe);
  for (const auto& line : lines) {
    decoding_pipe.Write(std::as_bytes(std::span(line)));
  }
  EXPECT_TRUE(decoding_pipe.Finish());
  ASSERT_EQ(block_pipe.bytes_written(), kInputSize);
  block.resize(kInputSize);
  EXPECT_EQ(block, input);

  SpanSource encoded(std::as_bytes(std::span(unsplit)));
  Base64Source decoder(Base64Mode::kDecode, encoded);
  EXPECT_EQ(Drain(decoder, 1000), input);
}

}  // namespace
