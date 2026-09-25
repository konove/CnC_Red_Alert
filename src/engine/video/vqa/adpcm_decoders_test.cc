#include "engine/video/vqa/adpcm_decoders.h"

#include <array>
#include <cstdint>
#include <span>

#include "base/numeric.h"
#include "base/unaligned.h"
#include "gtest/gtest.h"

namespace {

// Returns the 16-bit sample at index in a decoded buffer.
int16_t SampleAt(std::span<const uint8_t> samples, int index) {
  return base::ReadUnaligned<int16_t>(
      std::as_bytes(samples.subspan(base::ToSize(index * 2), 2)));
}

TEST(ImaAdpcmDecoderTest, SupportsOnly16BitMono) {
  EXPECT_TRUE(ImaAdpcmDecoder::Supports(1, 16));
  EXPECT_FALSE(ImaAdpcmDecoder::Supports(2, 16));
  EXPECT_FALSE(ImaAdpcmDecoder::Supports(1, 8));
}

TEST(ImaAdpcmDecoderTest, DecodesTheLowNibbleFirst) {
  // Code 7 at step 7 adds (7 * 2 + 1) * 7 / 8 = 13 and moves up 8 steps, to
  // step 16; code 0 then adds 16 / 8 = 2.
  const std::array<uint8_t, 1> source{0x07};
  std::array<uint8_t, 4> dest{};
  ImaAdpcmDecoder decoder;

  ASSERT_TRUE(decoder.Decode(source, dest));
  EXPECT_EQ(SampleAt(dest, 0), 13);
  EXPECT_EQ(SampleAt(dest, 1), 15);
}

TEST(ImaAdpcmDecoderTest, CarriesItsStateFromCallToCall) {
  const std::array<uint8_t, 1> source{0x07};
  std::array<uint8_t, 4> first{};
  std::array<uint8_t, 4> second{};
  ImaAdpcmDecoder decoder;

  ASSERT_TRUE(decoder.Decode(source, first));
  ASSERT_TRUE(decoder.Decode(source, second));
  // From 15 at step index 7 (step 14): code 7 adds 15 * 14 / 8 = 26.
  EXPECT_EQ(SampleAt(second, 0), 15 + 26);
}

TEST(ImaAdpcmDecoderTest, NegativeCodesSubtract) {
  // Code 15 is code 7 with the sign bit.
  const std::array<uint8_t, 1> source{0x0F};
  std::array<uint8_t, 4> dest{};
  ImaAdpcmDecoder decoder;

  ASSERT_TRUE(decoder.Decode(source, dest));
  EXPECT_EQ(SampleAt(dest, 0), -13);
}

TEST(ImaAdpcmDecoderTest, RefusesASourceTooShortToFillDest) {
  const std::array<uint8_t, 1> source{0x07};
  std::array<uint8_t, 8> dest{};
  ImaAdpcmDecoder decoder;

  EXPECT_FALSE(decoder.Decode(source, dest));
  EXPECT_EQ(dest, (std::array<uint8_t, 8>{}));
}

}  // namespace
