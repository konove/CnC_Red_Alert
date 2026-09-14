// Pins the exact bytes the codec pipes and straws produce today, so the
// stream refactor can prove that saved games, mixfile decryption and INI
// binary blocks stay byte-identical.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "absl/strings/escaping.h"
#include "base/types.h"
#include "gtest/gtest.h"
#include "tech/b64pipe.h"
#include "tech/b64straw.h"
#include "tech/blowfish.h"
#include "tech/blowpipe.h"
#include "tech/blwstraw.h"
#include "tech/lcwpipe.h"
#include "tech/lcwstraw.h"
#include "tech/lzopipe.h"
#include "tech/lzostraw.h"
#include "tech/lzwpipe.h"
#include "tech/lzwstraw.h"
#include "tech/pipe.h"
#include "tech/sha.h"
#include "tech/shapipe.h"
#include "tech/straw.h"
#include "tech/xpipe.h"
#include "tech/xstraw.h"

namespace {

// Not a multiple of any block size, so every codec ends on a partial block
// and Blowfish leaves a three-byte tail.
constexpr int kInputSize = 100003;

// SAVE_BLOCK_SIZE in ra/saveload.cc.
constexpr int kSaveBlockSize = 4096;

class VectorPipe : public Pipe {
 public:
  std::vector<uint8_t> bytes;
  base::ssize Put(std::span<const std::byte> data) override {
    for (const std::byte byte : data) {
      bytes.push_back(std::to_integer<uint8_t>(byte));
    }
    return std::ssize(data);
  }
};

std::vector<uint8_t> Drain(Straw& straw, int chunk_size) {
  std::vector<uint8_t> result;
  std::vector<uint8_t> chunk(static_cast<std::size_t>(chunk_size));
  for (base::ssize count = straw.Get(std::as_writable_bytes(std::span(chunk)));
       count != 0;
       count = straw.Get(std::as_writable_bytes(std::span(chunk)))) {
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
void PutInPieces(Pipe& pipe, const std::vector<uint8_t>& bytes) {
  for (std::size_t at = 0; at < bytes.size(); at += 997) {
    const std::size_t piece = std::min<std::size_t>(997, bytes.size() - at);
    pipe.Put(std::as_bytes(std::span(bytes).subspan(at, piece)));
  }
  pipe.Flush();
}

std::string Sha1Hex(const std::vector<uint8_t>& bytes) {
  SHAEngine sha;
  sha.Hash(bytes.data(), static_cast<int32_t>(bytes.size()));
  std::array<char, 20> digest{};
  sha.Result(digest.data());
  return absl::BytesToHexString({digest.data(), digest.size()});
}

std::array<uint8_t, BlowfishEngine::MAX_KEY_LENGTH> Key() {
  std::array<uint8_t, BlowfishEngine::MAX_KEY_LENGTH> key{};
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
  VectorPipe encoded;
  CodecPipe compressor(CodecPipe::COMPRESS, block_size);
  compressor.SetSink(encoded);
  PutInPieces(compressor, input);
  EXPECT_EQ(Sha1Hex(encoded.bytes), golden);

  BufferStraw plain(std::as_bytes(std::span(input)));
  CodecStraw compressing_straw(CodecStraw::COMPRESS, block_size);
  compressing_straw.SetSource(plain);
  EXPECT_EQ(Drain(compressing_straw, 1000), encoded.bytes);

  BufferStraw compressed(std::as_bytes(std::span(encoded.bytes)));
  CodecStraw decompressor(CodecStraw::DECOMPRESS, block_size);
  decompressor.SetSource(compressed);
  EXPECT_EQ(Drain(decompressor, 1000), input);

  VectorPipe decoded;
  CodecPipe decompressing_pipe(CodecPipe::DECOMPRESS, block_size);
  decompressing_pipe.SetSink(decoded);
  PutInPieces(decompressing_pipe, encoded.bytes);
  EXPECT_EQ(decoded.bytes, input);
}

TEST(StreamGoldenTest, LzoCompressesSaveBlocks) {
  ExpectCompresses<LZOPipe, LZOStraw>(
      kSaveBlockSize, "24f6acf295db6c5e04d63ba3a3c4769d81fa6d7e");
}

// The map and overlay packs use the default 8192-byte block.
TEST(StreamGoldenTest, LcwCompressesDefaultBlocks) {
  ExpectCompresses<LCWPipe, LCWStraw>(
      8192, "391841f830f9870d42d09cdde75435e7aa71d3fd");
}

TEST(StreamGoldenTest, LzwCompressesSaveBlocks) {
  ExpectCompresses<LZWPipe, LZWStraw>(
      kSaveBlockSize, "366f846b06db2bff6969bf91aa632f586501c8a5");
}

TEST(StreamGoldenTest, BlowfishEncryptsWithFixedKey) {
  const std::vector<uint8_t> input = Input();
  const auto key = Key();
  VectorPipe encrypted;
  BlowPipe encryptor(BlowPipe::ENCRYPT);
  encryptor.Key(key.data(), static_cast<int>(key.size()));
  encryptor.SetSink(encrypted);
  PutInPieces(encryptor, input);
  EXPECT_EQ(Sha1Hex(encrypted.bytes),
            "2fea48225f216a5c044f307a870ae4b2a88a8afc");

  BufferStraw plain(std::as_bytes(std::span(input)));
  BlowStraw encrypting_straw(BlowStraw::ENCRYPT);
  encrypting_straw.Key(key.data(), static_cast<int>(key.size()));
  encrypting_straw.SetSource(plain);
  EXPECT_EQ(Drain(encrypting_straw, 1000), encrypted.bytes);

  BufferStraw cipher(std::as_bytes(std::span(encrypted.bytes)));
  BlowStraw decryptor(BlowStraw::DECRYPT);
  decryptor.Key(key.data(), static_cast<int>(key.size()));
  decryptor.SetSource(cipher);
  EXPECT_EQ(Drain(decryptor, 1000), input);
}

// The save-game body: LZO, then Blowfish, then a SHA-1 tap before the file.
TEST(StreamGoldenTest, SaveGameChainProducesPinnedBytesAndDigest) {
  const std::vector<uint8_t> input = Input();
  const auto key = Key();
  VectorPipe file;
  SHAPipe sha;
  BlowPipe blow(BlowPipe::ENCRYPT);
  LZOPipe lzo(LZOPipe::COMPRESS, kSaveBlockSize);
  blow.Key(key.data(), static_cast<int>(key.size()));
  sha.SetSink(file);
  blow.SetSink(sha);
  lzo.SetSink(blow);
  PutInPieces(lzo, input);
  EXPECT_EQ(Sha1Hex(file.bytes), "2338b443f754c7b2e6e587dab7c29deff557d0b1");

  // The digest written into the save covers the stream exactly as stored.
  std::array<char, 20> digest{};
  sha.Result(digest.data());
  EXPECT_EQ(absl::BytesToHexString({digest.data(), digest.size()}),
            Sha1Hex(file.bytes));

  BufferStraw stored(std::as_bytes(std::span(file.bytes)));
  BlowStraw decrypt(BlowStraw::DECRYPT);
  LZOStraw decompress(LZOStraw::DECOMPRESS, kSaveBlockSize);
  decrypt.Key(key.data(), static_cast<int>(key.size()));
  decrypt.SetSource(stored);
  decompress.SetSource(decrypt);
  EXPECT_EQ(Drain(decompress, 1000), input);
}

// INIClass::Put_UUBlock draws 70 characters at a time for each entry, and
// Get_UUBlock pushes the entries back through a decoding pipe.
TEST(StreamGoldenTest, Base64EncodesInUuBlockLines) {
  const std::vector<uint8_t> input = Input();
  BufferStraw plain(std::as_bytes(std::span(input)));
  Base64Straw encoder(Base64Straw::ENCODE);
  encoder.SetSource(plain);
  std::vector<std::vector<uint8_t>> lines;
  std::vector<uint8_t> joined;
  for (;;) {
    std::array<uint8_t, 70> line{};
    const auto length =
        static_cast<int>(encoder.Get(std::as_writable_bytes(std::span(line))));
    if (length == 0) {
      break;
    }
    lines.emplace_back(line.begin(), line.begin() + length);
    joined.insert(joined.end(), line.begin(), line.begin() + length);
    joined.push_back('\n');
  }
  EXPECT_EQ(Sha1Hex(joined), "228aa04caf47a421738b2d0186b6a379b1b23fbd");

  VectorPipe pipe_encoded;
  Base64Pipe encoding_pipe(Base64Pipe::ENCODE);
  encoding_pipe.SetSink(pipe_encoded);
  PutInPieces(encoding_pipe, input);
  std::vector<uint8_t> unsplit;
  for (const auto& line : lines) {
    unsplit.insert(unsplit.end(), line.begin(), line.end());
  }
  EXPECT_EQ(pipe_encoded.bytes, unsplit);

  std::vector<uint8_t> block(input.size() + 16);
  BufferPipe block_pipe(std::as_writable_bytes(std::span(block)));
  Base64Pipe decoding_pipe(Base64Pipe::DECODE);
  decoding_pipe.SetSink(&block_pipe);
  base::ssize total = 0;
  for (const auto& line : lines) {
    total += decoding_pipe.Put(std::as_bytes(std::span(line)));
  }
  total += decoding_pipe.End();
  ASSERT_EQ(total, kInputSize);
  block.resize(kInputSize);
  EXPECT_EQ(block, input);

  BufferStraw encoded(std::as_bytes(std::span(unsplit)));
  Base64Straw decoder(Base64Straw::DECODE);
  decoder.SetSource(encoded);
  EXPECT_EQ(Drain(decoder, 1000), input);
}

}  // namespace
