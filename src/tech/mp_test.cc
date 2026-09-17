// Tests for encoding multi-precision numbers into fixed-length buffers.

#include "tech/mp.h"

#include <array>
#include <cstdint>

#include "gtest/gtest.h"
#include "tech/digit_cursor.h"
#include "tech/int.h"
#include "tech/random_source.h"

namespace {

constexpr int kPrecision = 2;

TEST(IntAccessTest, ReadsBitsAcrossDigitBoundaries) {
  const Int<2> value(0x80000001U);
  EXPECT_TRUE(value.at(0));
  EXPECT_TRUE(value.at(31));
  EXPECT_FALSE(value.at(32));
  EXPECT_FALSE(value.at(63));
}

TEST(IntAccessDeathTest, RejectsNegativeAndPastEndBits) {
  const Int<2> value(1U);
  // GoogleTest's death-test macro formats subprocess diagnostics with libc.
  // NOLINTBEGIN(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH((void)value.at(-1), "Check failed");
  EXPECT_DEATH((void)value.at(64), "Check failed");
  // NOLINTEND(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
}


TEST(XmpEncodeTest, ShortBufferKeepsLowOrderBytes) {
  const std::array<uint32_t, kPrecision> number = {0x04030201, 0x08070605};
  std::array<unsigned char, 5> buffer{};
  buffer.fill(0xaa);

  // Before the fix, the padding count wrapped and the loop overran the buffer.
  EXPECT_EQ(XMP_Encode(buffer, 3, number, kPrecision), 3U);

  EXPECT_EQ(buffer, (std::array<unsigned char, 5>{0x03, 0x02, 0x01, 0xaa,
                                                  0xaa}));
}

TEST(XmpEncodeTest, ExactBufferIsBigEndian) {
  const std::array<uint32_t, kPrecision> number = {0x04030201, 0x08070605};
  std::array<unsigned char, 8> buffer{};

  EXPECT_EQ(XMP_Encode(buffer, 8, number, kPrecision), 8U);

  EXPECT_EQ(buffer, (std::array<unsigned char, 8>{0x08, 0x07, 0x06, 0x05, 0x04,
                                                  0x03, 0x02, 0x01}));
}

TEST(XmpEncodeTest, LongBufferIsSignExtended) {
  const std::array<uint32_t, kPrecision> positive = {0x04030201, 0x08070605};
  std::array<unsigned char, 10> buffer{};
  buffer.fill(0xaa);

  EXPECT_EQ(XMP_Encode(buffer, 10, positive, kPrecision), 10U);
  EXPECT_EQ(buffer, (std::array<unsigned char, 10>{0x00, 0x00, 0x08, 0x07,
                                                   0x06, 0x05, 0x04, 0x03,
                                                   0x02, 0x01}));

  const std::array<uint32_t, kPrecision> negative = {0x00000001, 0x80000000};
  EXPECT_EQ(XMP_Encode(buffer, 10, negative, kPrecision), 10U);
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

  XMP_Randomize(digits, rng, kPrecision * 32, kPrecision);

  EXPECT_EQ(digits.at(kPrecision), 0xa5a5a5a5U);
  EXPECT_NE(digits.at(0) | digits.at(1), 0U);
}

TEST(XmpRandomizeTest, ClearsBitsAboveTheRequestedCount) {
  RandomSource rng;
  Seed(rng);
  for (const int bits : {1, 7, 8, 12, 32, 45, 63}) {
    std::array<uint32_t, kPrecision + 1> digits{};
    digits.fill(0xffffffff);

    XMP_Randomize(digits, rng, bits, kPrecision);

    const uint64_t value = digits.at(0) | (uint64_t{digits.at(1)} << 32);
    EXPECT_EQ(value >> static_cast<unsigned>(bits), 0U) << bits;
    EXPECT_EQ(digits.at(kPrecision), 0xffffffffU) << bits;
  }
}

TEST(DigitCursorTest, HalfDigitWritesPreserveNeighbors) {
  std::array<uint32_t, 2> digits = {0x12345678U, 0x90abcdefU};
  DigitCursor<uint32_t> cursor(digits);
  const auto halves = cursor.Rebind<uint16_t>();
  EXPECT_EQ(static_cast<uint16_t>(halves.at(0)), 0x5678);
  halves.at(1) = 0x4321;
  EXPECT_EQ(digits.at(0), 0x43215678U);
  EXPECT_EQ(digits.at(1), 0x90abcdefU);
  cursor += 2;
  EXPECT_EQ(static_cast<uint32_t>(*--cursor), 0x90abcdefU);
  EXPECT_EQ(static_cast<uint32_t>(*--cursor), 0x43215678U);
}

TEST(DigitCursorTest, RejectsAccessOutsideOriginalStorage) {
  std::array<uint32_t, 2> digits{};
  const DigitCursor<uint32_t> cursor(digits);
  // GoogleTest's death-test macro formats subprocess diagnostics with libc.
  // NOLINTBEGIN(clang-diagnostic-unsafe-buffer-usage-in-libc-call,clang-diagnostic-switch-default)
  EXPECT_DEATH(static_cast<void>(cursor.at(-1)), "Check failed");
  EXPECT_DEATH(static_cast<void>(cursor.at(2)), "Check failed");
  // NOLINTEND(clang-diagnostic-unsafe-buffer-usage-in-libc-call,clang-diagnostic-switch-default)
}

TEST(XmpDerDecodeTest, TruncatedInputLeavesNumberUnchanged) {
  std::array<uint32_t, 2> digits = {7, 0};
  const std::array<unsigned char, 3> truncated = {2, 3, 1};
  XMP_DER_Decode(digits, truncated, 2);
  EXPECT_EQ(digits.at(0), 7U);
  EXPECT_EQ(digits.at(1), 0U);
}

}  // namespace
