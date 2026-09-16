// Tests the public-key stream factories: a random Blowfish key sealed with
// one half of a key pair opens with the other half.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <vector>

#include "base/types.h"
#include "gtest/gtest.h"
#include "tech/byte_sink.h"
#include "tech/byte_source.h"
#include "tech/pk.h"
#include "tech/pk_sink.h"
#include "tech/pk_source.h"
#include "tech/random_source.h"
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
  std::array<uint8_t, 100> chunk{};
  for (base::ssize count = straw.Read(std::as_writable_bytes(std::span(chunk)));
       count != 0;
       count = straw.Read(std::as_writable_bytes(std::span(chunk)))) {
    std::ranges::copy(std::span(chunk).first(static_cast<std::size_t>(count)),
                      std::back_inserter(result));
  }
  return result;
}

void Seed(RandomSource& rng) {
  for (int32_t value = 1; rng.Seed_Bits_Needed() > 0; ++value) {
    rng.Seed_Long(value * 40503);
  }
}

class PkStreamTest : public testing::Test {
 protected:
  static void SetUpTestSuite() {
    RandomSource rng;
    Seed(rng);
    // Small primes keep the test fast; the modulus still spans several
    // Blowfish key blocks.
    PKey::Generate(rng, 128, fast_key_, slow_key_);
  }

  static PKey fast_key_;
  static PKey slow_key_;
};

PKey PkStreamTest::fast_key_;
PKey PkStreamTest::slow_key_;

TEST_F(PkStreamTest, EncryptPipeRoundTripsThroughDecryptStraw) {
  std::vector<uint8_t> plain;
  plain.reserve(1003);
  for (int i = 0; i < 1003; ++i) {
    plain.push_back(static_cast<uint8_t>(i * 13));
  }

  RandomSource rng;
  Seed(rng);
  RecordingSink sink;
  const auto pipe = MakePkEncryptSink(sink, fast_key_, rng);
  ASSERT_NE(pipe, nullptr);
  pipe->Write(std::as_bytes(std::span(plain)));
  pipe->Flush();
  const int header_size =
      fast_key_.Block_Count(56) * fast_key_.Crypt_Block_Size();
  ASSERT_EQ(std::ssize(sink.bytes), header_size + std::ssize(plain));

  SpanSource source(std::as_bytes(std::span(sink.bytes)));
  const auto straw = MakePkDecryptSource(source, slow_key_);
  ASSERT_NE(straw, nullptr);
  EXPECT_EQ(Drain(*straw), plain);
}

TEST_F(PkStreamTest, DecryptStrawRejectsShortHeader) {
  const std::array<uint8_t, 10> truncated{};
  SpanSource source(std::as_bytes(std::span(truncated)));
  EXPECT_EQ(MakePkDecryptSource(source, slow_key_), nullptr);
}

}  // namespace
