#include "engine/video/vqa/adpcm_decoders.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

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

// The movie loader's layout: the stream at the end of the buffer it decodes
// into, which the output never catches up with.
TEST(ImaAdpcmDecoderTest, DecodesAStreamAtTheEndOfItsDestination) {
  std::vector<uint8_t> buffer(400);
  for (std::size_t i = 300; i < buffer.size(); ++i) {
    buffer.at(i) = static_cast<uint8_t>((i * 37) + 11);
  }
  const std::vector<uint8_t> source(buffer.begin() + 300, buffer.end());
  std::vector<uint8_t> want(buffer.size());
  ImaAdpcmDecoder separate;
  ASSERT_TRUE(separate.Decode(source, want));

  ImaAdpcmDecoder in_place;
  ASSERT_TRUE(in_place.Decode(std::span(buffer).subspan(300), buffer));
  EXPECT_EQ(buffer, want);
}

// When the output overtakes the stream, later codes are read from the samples
// already written over it, as the C++ decoder did.
TEST(ImaAdpcmDecoderTest, ReadsWhatItWroteWhenTheOutputOvertakesTheStream) {
  std::array<uint8_t, 8> buffer{0x07, 0x70};
  ImaAdpcmDecoder decoder;

  ASSERT_TRUE(decoder.Decode(std::span(buffer).first(2), buffer));
  // 0x07 gives 13 and 15, written as 0d 00 0f 00, so the second code byte is
  // 0x00, not 0x70: code 0 at step 14 adds 1, then at step 13 adds 1.
  EXPECT_EQ(SampleAt(buffer, 0), 13);
  EXPECT_EQ(SampleAt(buffer, 1), 15);
  EXPECT_EQ(SampleAt(buffer, 2), 16);
  EXPECT_EQ(SampleAt(buffer, 3), 17);
}

TEST(ImaAdpcmDeathTest, RejectsAStreamOverlappingTheFront) {
  std::vector<uint8_t> buffer(32);
  ImaAdpcmDecoder decoder;
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(decoder.Decode(std::span<const uint8_t>(buffer).first(8),
                              std::span(buffer).subspan(4, 16)),
               "overlaps the front");
}

}  // namespace
