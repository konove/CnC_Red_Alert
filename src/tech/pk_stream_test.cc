// Tests the public-key stream factories: a random Blowfish key sealed with
// one half of a key pair opens with the other half.

#include <array>
#include <cstdint>
#include <vector>

#include "gtest/gtest.h"
#include "tech/pipe.h"
#include "tech/pk.h"
#include "tech/pkpipe.h"
#include "tech/pkstraw.h"
#include "tech/rndstraw.h"
#include "tech/straw.h"
#include "tech/xstraw.h"

namespace {

class VectorPipe : public Pipe {
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
  std::array<uint8_t, 100> chunk{};
  for (int count = straw.Get(chunk.data(), chunk.size()); count != 0;
       count = straw.Get(chunk.data(), chunk.size())) {
    result.insert(result.end(), chunk.begin(), chunk.begin() + count);
  }
  return result;
}

void Seed(RandomStraw& rng) {
  for (int32_t value = 1; rng.Seed_Bits_Needed() > 0; ++value) {
    rng.Seed_Long(value * 40503);
  }
}

class PkStreamTest : public testing::Test {
 protected:
  static void SetUpTestSuite() {
    RandomStraw rng;
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

  RandomStraw rng;
  Seed(rng);
  VectorPipe sink;
  const auto pipe = MakePKEncryptPipe(sink, fast_key_, rng);
  ASSERT_NE(pipe, nullptr);
  pipe->Put(plain.data(), static_cast<int>(plain.size()));
  pipe->Flush();
  const int header_size =
      fast_key_.Block_Count(56) * fast_key_.Crypt_Block_Size();
  ASSERT_EQ(std::ssize(sink.bytes), header_size + std::ssize(plain));

  BufferStraw source(sink.bytes.data(), static_cast<int>(sink.bytes.size()));
  const auto straw = MakePKDecryptStraw(source, slow_key_);
  ASSERT_NE(straw, nullptr);
  EXPECT_EQ(Drain(*straw), plain);
}

TEST_F(PkStreamTest, DecryptStrawRejectsShortHeader) {
  const std::array<uint8_t, 10> truncated{};
  BufferStraw source(truncated.data(), truncated.size());
  EXPECT_EQ(MakePKDecryptStraw(source, slow_key_), nullptr);
}

}  // namespace
