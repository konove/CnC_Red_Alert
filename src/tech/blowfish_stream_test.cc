// Tests the Blowfish stream links: whole 8-byte blocks are encrypted, a
// shorter tail passes through unchanged, and no key means no change.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "base/types.h"
#include "gtest/gtest.h"
#include "tech/blowfish.h"
#include "tech/blowpipe.h"
#include "tech/blwstraw.h"
#include "tech/pipe.h"
#include "tech/straw.h"
#include "tech/xstraw.h"

namespace {

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

std::vector<uint8_t> Plain(int size) {
  std::vector<uint8_t> bytes;
  bytes.reserve(static_cast<std::size_t>(size));
  for (int i = 0; i < size; ++i) {
    bytes.push_back(static_cast<uint8_t>((i * 29) + 3));
  }
  return bytes;
}

constexpr std::array<uint8_t, 16> kKey = {1, 2,  3,  4,  5,  6,  7,  8,
                                          9, 10, 11, 12, 13, 14, 15, 16};

std::vector<uint8_t> EncryptWithPipe(const std::vector<uint8_t>& plain) {
  VectorPipe sink;
  BlowPipe pipe(BlowPipe::ENCRYPT);
  pipe.Key(kKey.data(), kKey.size());
  pipe.SetSink(sink);
  pipe.Put(std::as_bytes(std::span(plain)));
  pipe.Flush();
  return sink.bytes;
}

std::vector<uint8_t> DecryptWithStraw(const std::vector<uint8_t>& cipher,
                                      int chunk_size) {
  BufferStraw source(std::as_bytes(std::span(cipher)));
  BlowStraw straw(BlowStraw::DECRYPT);
  straw.Key(kKey.data(), kKey.size());
  straw.SetSource(source);
  return Drain(straw, chunk_size);
}

TEST(BlowfishStreamTest, PipeToStrawRoundTrip) {
  const std::vector<uint8_t> plain = Plain(1003);
  const std::vector<uint8_t> cipher = EncryptWithPipe(plain);
  ASSERT_EQ(cipher.size(), plain.size());
  EXPECT_NE(cipher, plain);
  EXPECT_EQ(DecryptWithStraw(cipher, 100), plain);
}

TEST(BlowfishStreamTest, EncryptsEachWholeBlockIndependently) {
  const std::vector<uint8_t> plain = Plain(16);
  const std::vector<uint8_t> cipher = EncryptWithPipe(plain);

  BlowfishEngine engine;
  engine.Submit_Key(kKey.data(), kKey.size());
  std::array<uint8_t, 16> expected{};
  engine.Encrypt(plain.data(), 8, expected.data());
  engine.Encrypt(plain.data() + 8, 8, expected.data() + 8);
  EXPECT_EQ(cipher, std::vector<uint8_t>(expected.begin(), expected.end()));
}

TEST(BlowfishStreamTest, ShortTailPassesThroughBothWays) {
  const std::vector<uint8_t> plain = Plain(21);
  const std::vector<uint8_t> cipher = EncryptWithPipe(plain);
  // The last five bytes do not fill a block.
  EXPECT_TRUE(
      std::equal(cipher.begin() + 16, cipher.end(), plain.begin() + 16));

  BufferStraw source(std::as_bytes(std::span(plain)));
  BlowStraw straw(BlowStraw::ENCRYPT);
  straw.Key(kKey.data(), kKey.size());
  straw.SetSource(source);
  EXPECT_EQ(Drain(straw, 7), cipher);

  EXPECT_EQ(DecryptWithStraw(cipher, 7), plain);

  VectorPipe decrypted;
  BlowPipe pipe(BlowPipe::DECRYPT);
  pipe.Key(kKey.data(), kKey.size());
  pipe.SetSink(decrypted);
  pipe.Put(std::as_bytes(std::span(cipher)));
  pipe.Flush();
  EXPECT_EQ(decrypted.bytes, plain);

  const std::vector<uint8_t> tiny = Plain(5);
  EXPECT_EQ(EncryptWithPipe(tiny), tiny);
  EXPECT_EQ(DecryptWithStraw(tiny, 3), tiny);
}

TEST(BlowfishStreamTest, WithoutKeyPassesThrough) {
  const std::vector<uint8_t> plain = Plain(37);
  VectorPipe sink;
  BlowPipe pipe(BlowPipe::ENCRYPT);
  pipe.SetSink(sink);
  pipe.Put(std::as_bytes(std::span(plain)));
  pipe.Flush();
  EXPECT_EQ(sink.bytes, plain);

  BufferStraw source(std::as_bytes(std::span(plain)));
  BlowStraw straw(BlowStraw::DECRYPT);
  straw.SetSource(source);
  EXPECT_EQ(Drain(straw, 10), plain);
}

TEST(BlowfishStreamTest, OneByteAtATimeMatchesBulk) {
  const std::vector<uint8_t> plain = Plain(45);
  const std::vector<uint8_t> bulk = EncryptWithPipe(plain);

  VectorPipe sink;
  BlowPipe pipe(BlowPipe::ENCRYPT);
  pipe.Key(kKey.data(), kKey.size());
  pipe.SetSink(sink);
  for (const uint8_t& byte : plain) {
    pipe.WriteObject(byte);
  }
  pipe.Flush();
  EXPECT_EQ(sink.bytes, bulk);

  EXPECT_EQ(DecryptWithStraw(bulk, 1), plain);
}

}  // namespace
