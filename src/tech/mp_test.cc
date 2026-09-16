// Tests for encoding multi-precision numbers into fixed-length buffers.

#include "tech/mp.h"

#include <array>
#include <cstdint>

#include "gtest/gtest.h"
#include "tech/random_source.h"

namespace {

constexpr int kPrecision = 2;

TEST(XmpEncodeTest, ShortBufferKeepsLowOrderBytes) {
  const std::array<uint32_t, kPrecision> number = {0x04030201, 0x08070605};
  std::array<unsigned char, 5> buffer{};
  buffer.fill(0xaa);

  // Before the fix, the padding count wrapped and the loop overran the buffer.
  EXPECT_EQ(XMP_Encode(buffer.data(), 3, number.data(), kPrecision), 3U);

  EXPECT_EQ(buffer, (std::array<unsigned char, 5>{0x03, 0x02, 0x01, 0xaa,
                                                  0xaa}));
}

TEST(XmpEncodeTest, ExactBufferIsBigEndian) {
  const std::array<uint32_t, kPrecision> number = {0x04030201, 0x08070605};
  std::array<unsigned char, 8> buffer{};

  EXPECT_EQ(XMP_Encode(buffer.data(), 8, number.data(), kPrecision), 8U);

  EXPECT_EQ(buffer, (std::array<unsigned char, 8>{0x08, 0x07, 0x06, 0x05, 0x04,
                                                  0x03, 0x02, 0x01}));
}

TEST(XmpEncodeTest, LongBufferIsSignExtended) {
  const std::array<uint32_t, kPrecision> positive = {0x04030201, 0x08070605};
  std::array<unsigned char, 10> buffer{};
  buffer.fill(0xaa);

  EXPECT_EQ(XMP_Encode(buffer.data(), 10, positive.data(), kPrecision), 10U);
  EXPECT_EQ(buffer, (std::array<unsigned char, 10>{0x00, 0x00, 0x08, 0x07,
                                                   0x06, 0x05, 0x04, 0x03,
                                                   0x02, 0x01}));

  const std::array<uint32_t, kPrecision> negative = {0x00000001, 0x80000000};
  EXPECT_EQ(XMP_Encode(buffer.data(), 10, negative.data(), kPrecision), 10U);
  EXPECT_EQ(buffer, (std::array<unsigned char, 10>{0xff, 0xff, 0x80, 0x00,
                                                   0x00, 0x00, 0x00, 0x00,
                                                   0x00, 0x01}));
}

// Seeds a generator so that its bytes are neither all zero nor all ones.
void Seed(RandomSource& rng) {
  for (int32_t value = 1; rng.Seed_Bits_Needed() > 0; ++value) {
    rng.Seed_Long(value * 7919);
  }
}

TEST(XmpRandomizeTest, FullPrecisionStaysInsideTheDigits) {
  RandomSource rng;
  Seed(rng);
  // A guard digit after the number catches a write past its end.
  std::array<uint32_t, kPrecision + 1> digits{};
  digits.fill(0xa5a5a5a5);

  XMP_Randomize(digits.data(), rng, kPrecision * 32, kPrecision);

  EXPECT_EQ(digits[kPrecision], 0xa5a5a5a5U);
  EXPECT_NE(digits[0] | digits[1], 0U);
}

TEST(XmpRandomizeTest, ClearsBitsAboveTheRequestedCount) {
  RandomSource rng;
  Seed(rng);
  for (const int bits : {1, 7, 8, 12, 32, 45, 63}) {
    std::array<uint32_t, kPrecision + 1> digits{};
    digits.fill(0xffffffff);

    XMP_Randomize(digits.data(), rng, bits, kPrecision);

    const uint64_t value = digits[0] | (uint64_t{digits[1]} << 32);
    EXPECT_EQ(value >> static_cast<unsigned>(bits), 0U) << bits;
    EXPECT_EQ(digits[kPrecision], 0xffffffffU) << bits;
  }
}

}  // namespace
