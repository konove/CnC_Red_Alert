// Holds the Rust .AUD block decoders in src/rust to the C++ ones they
// replaced: the same samples and the same state for valid, truncated and
// random blocks.

#include <cstddef>
#include <cstdint>
#include <vector>

#include "base/lcg_test_util.h"
#include "base/numeric.h"
#include "engine/codec/aud_decoder.h"
#include "engine/codec/aud_decoder_reference_test_util.h"
#include "gtest/gtest.h"

namespace {

TEST(AudDecoderDifferentialTest, AdpcmMatchesReferenceFromEveryState) {
  base::Lcg lcg(1);
  for (int round = 0; round < 4000; ++round) {
    // Every step index, and predictors near both ends of the range.
    AdpcmState want_state{
        .predictor = static_cast<int16_t>(
            lcg.Below(4) == 0 ? (lcg.Below(2) == 0 ? 32767 : -32768) +
                                    (lcg.Below(200) - 100)
                              : lcg.Below(65536) - 32768),
        .step_index = static_cast<int8_t>(lcg.Below(89))};
    AdpcmState got_state = want_state;
    // Several blocks in a row, so the state is carried.
    for (int block = 0; block < 3; ++block) {
      const auto bytes = lcg.Bytes(lcg.Below(300));
      const auto want =
          aud_decoder_reference::DecodeAdpcmBlock(want_state, bytes);
      const auto got = DecodeAdpcmBlock(got_state, bytes);
      ASSERT_TRUE(got == want) << "round " << round;
      ASSERT_EQ(got_state.predictor, want_state.predictor);
      ASSERT_EQ(got_state.step_index, want_state.step_index);
    }
  }
}

TEST(AudDecoderDifferentialTest, AdpcmCoversEveryCodeAtEveryStep) {
  for (int step_index = 0; step_index <= 88; ++step_index) {
    for (int byte = 0; byte < 256; ++byte) {
      for (const int predictor : {-32768, -1000, 0, 1000, 32767}) {
        AdpcmState want_state{.predictor = static_cast<int16_t>(predictor),
                              .step_index = static_cast<int8_t>(step_index)};
        AdpcmState got_state = want_state;
        const std::vector<std::byte> block{static_cast<std::byte>(byte)};
        ASSERT_EQ(DecodeAdpcmBlock(got_state, block),
                  aud_decoder_reference::DecodeAdpcmBlock(want_state, block));
        ASSERT_EQ(got_state.predictor, want_state.predictor);
        ASSERT_EQ(got_state.step_index, want_state.step_index);
      }
    }
  }
}

// A block of well-formed Westwood commands.
std::vector<std::byte> WestwoodBlock(base::Lcg& lcg) {
  std::vector<std::byte> block;
  for (int commands = lcg.Below(60); commands > 0; --commands) {
    const uint8_t command = lcg.Byte();
    block.push_back(std::byte{command});
    const int kind = command >> 6U;
    const bool has_payload =
        kind == 0 || kind == 1 || (kind == 2 && (command & 0x20U) == 0);
    if (has_payload) {
      const auto payload = lcg.Bytes((command & 0x3fU) + 1);
      block.insert(block.end(), payload.begin(), payload.end());
    }
  }
  return block;
}

void ExpectWestwoodMatch(const std::vector<std::byte>& block) {
  const auto want = aud_decoder_reference::DecodeWestwoodBlock(block);
  const auto got = DecodeWestwoodBlock(block);
  ASSERT_TRUE(got == want);
}

TEST(AudDecoderDifferentialTest, WestwoodValidBlocksMatchReference) {
  base::Lcg lcg(2);
  for (int round = 0; round < 4000; ++round) {
    ExpectWestwoodMatch(WestwoodBlock(lcg));
  }
}

TEST(AudDecoderDifferentialTest, WestwoodTruncatedBlocksMatchReference) {
  base::Lcg lcg(3);
  for (int round = 0; round < 4000; ++round) {
    auto block = WestwoodBlock(lcg);
    if (!block.empty()) {
      block.resize(base::ToSize(lcg.Below(static_cast<int>(block.size()))));
    }
    ExpectWestwoodMatch(block);
  }
}

TEST(AudDecoderDifferentialTest, WestwoodRandomBytesMatchReference) {
  base::Lcg lcg(4);
  for (int round = 0; round < 8000; ++round) {
    ExpectWestwoodMatch(lcg.Bytes(lcg.Below(400)));
  }
  ExpectWestwoodMatch({});
}

}  // namespace
