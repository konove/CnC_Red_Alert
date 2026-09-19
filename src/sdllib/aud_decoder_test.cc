#include "sdllib/aud_decoder.h"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <vector>

#include "gtest/gtest.h"

namespace {

std::vector<std::byte> Bytes(std::initializer_list<int> values) {
  std::vector<std::byte> bytes;
  for (const int value : values) {
    bytes.push_back(static_cast<std::byte>(value));
  }
  return bytes;
}

std::vector<uint8_t> DecodeWestwood(std::initializer_list<int> block) {
  return DecodeWestwoodBlock(Bytes(block)).value_or(std::vector<uint8_t>{});
}

TEST(AudDecoderTest, AdpcmDecodesLowNibbleFirst) {
  AdpcmState state;
  // Code 7 at step 7 adds 15/8 of it; that moves the index to 8, where code 0
  // adds an eighth of step 16.
  EXPECT_EQ(DecodeAdpcmBlock(state, Bytes({0x07})),
            (std::vector<int16_t>{13, 15}));
  EXPECT_EQ(state.predictor, 15);
  EXPECT_EQ(state.step_index, 7);
}

TEST(AudDecoderTest, AdpcmSignBitSubtracts) {
  AdpcmState state;
  EXPECT_EQ(DecodeAdpcmBlock(state, Bytes({0x0F})),
            (std::vector<int16_t>{-13, -11}));
}

TEST(AudDecoderTest, AdpcmStateCarriesAcrossBlocks) {
  AdpcmState split_state;
  std::vector<int16_t> split =
      DecodeAdpcmBlock(split_state, Bytes({0x37, 0x9A}));
  const auto second = DecodeAdpcmBlock(split_state, Bytes({0x5C}));
  split.insert(split.end(), second.begin(), second.end());

  AdpcmState whole_state;
  EXPECT_EQ(split, DecodeAdpcmBlock(whole_state, Bytes({0x37, 0x9A, 0x5C})));
  EXPECT_EQ(split.size(), 6U);
}

TEST(AudDecoderTest, AdpcmSaturatesSampleAndStepIndex) {
  AdpcmState state{.predictor = 32767, .step_index = 88};
  EXPECT_EQ(DecodeAdpcmBlock(state, Bytes({0x77})),
            (std::vector<int16_t>{32767, 32767}));
  EXPECT_EQ(state.step_index, 88);

  state = {.predictor = -32768, .step_index = 0};
  EXPECT_EQ(DecodeAdpcmBlock(state, Bytes({0x88})),
            (std::vector<int16_t>{-32768, -32768}));
  EXPECT_EQ(state.step_index, 0);
}

TEST(AudDecoderTest, WestwoodRepeatsTheMidpoint) {
  EXPECT_EQ(DecodeWestwood({0xC2}), (std::vector<uint8_t>{0x80, 0x80, 0x80}));
}

TEST(AudDecoderTest, WestwoodRawRunSetsTheCurrentSample) {
  EXPECT_EQ(DecodeWestwood({0x81, 0x10, 0x20, 0xC0}),
            (std::vector<uint8_t>{0x10, 0x20, 0x20}));
}

TEST(AudDecoderTest, WestwoodFiveBitDeltaIsSignedAndWraps) {
  EXPECT_EQ(DecodeWestwood({0xA3, 0xBF}), (std::vector<uint8_t>{0x83, 0x82}));
  EXPECT_EQ(DecodeWestwood({0x80, 0xFE, 0xA5}),
            (std::vector<uint8_t>{0xFE, 0x03}));
}

TEST(AudDecoderTest, WestwoodFourBitDeltasLowNibbleFirst) {
  EXPECT_EQ(DecodeWestwood({0x40, 0xF0}), (std::vector<uint8_t>{0x77, 0x7F}));
}

TEST(AudDecoderTest, WestwoodTwoBitDeltasLowestBitsFirst) {
  EXPECT_EQ(DecodeWestwood({0x00, 0xE4}),
            (std::vector<uint8_t>{0x7E, 0x7D, 0x7D, 0x7E}));
}

TEST(AudDecoderTest, WestwoodPackedDeltasSaturate) {
  EXPECT_EQ(DecodeWestwood({0x80, 0xFF, 0x40, 0xFF}),
            (std::vector<uint8_t>{0xFF, 0xFF, 0xFF}));
  EXPECT_EQ(DecodeWestwood({0x80, 0x01, 0x00, 0x00}),
            (std::vector<uint8_t>{0x01, 0x00, 0x00, 0x00, 0x00}));
}

TEST(AudDecoderTest, WestwoodRejectsCommandsPastTheEnd) {
  EXPECT_TRUE(DecodeWestwoodBlock(Bytes({0xC0, 0x41, 0x00, 0x00})));
  EXPECT_FALSE(DecodeWestwoodBlock(Bytes({0xC0, 0x41, 0x00})));
  EXPECT_FALSE(DecodeWestwoodBlock(Bytes({0x82, 0x01})));
  EXPECT_FALSE(DecodeWestwoodBlock(Bytes({0x03, 0x00})));
}

}  // namespace
