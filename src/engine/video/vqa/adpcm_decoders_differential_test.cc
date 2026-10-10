// Holds the Rust IMA ADPCM decoder behind ImaAdpcmDecoder to the C++ one it
// replaced: the same destination bytes and result, with the stream apart from
// its destination and inside it.

#include <algorithm>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "base/lcg_test_util.h"
#include "base/numeric.h"
#include "engine/video/vqa/adpcm_decoders.h"
#include "engine/video/vqa/adpcm_decoders_reference_test_util.h"
#include "gtest/gtest.h"

namespace {

std::vector<uint8_t> RandomBuffer(base::Lcg& lcg, const int size) {
  std::vector<uint8_t> buffer(base::ToSize(size));
  std::ranges::generate(buffer, [&lcg] { return lcg.Byte(); });
  return buffer;
}

TEST(ImaAdpcmDifferentialTest, SeparateBuffersMatchReference) {
  base::Lcg lcg(1);
  for (int round = 0; round < 3000; ++round) {
    ImaAdpcmDecoder decoder;
    adpcm_decoders_reference::ImaAdpcmDecoder reference;
    // Several chunks through one decoder, so the state is carried; some
    // sources are too short and must be refused.
    for (int chunk = 0; chunk < 3; ++chunk) {
      const auto source = RandomBuffer(lcg, lcg.Below(200));
      auto want = RandomBuffer(lcg, lcg.Below(900));
      auto got = want;
      ASSERT_EQ(decoder.Decode(source, got), reference.Decode(source, want));
      ASSERT_TRUE(got == want) << "round " << round;
    }
  }
}

TEST(ImaAdpcmDifferentialTest, InPlaceMatchesReference) {
  base::Lcg lcg(2);
  for (int round = 0; round < 3000; ++round) {
    ImaAdpcmDecoder decoder;
    adpcm_decoders_reference::ImaAdpcmDecoder reference;
    for (int chunk = 0; chunk < 3; ++chunk) {
      // The stream anywhere in the buffer, the destination at its front: the
      // movie loader's layout (stream at the very end) and every other,
      // including ones where the output overtakes the stream.
      auto want = RandomBuffer(lcg, 1 + lcg.Below(900));
      auto got = want;
      const int size = static_cast<int>(want.size());
      const int source_start = lcg.Below(3) == 0 ? lcg.Below(size) : 0;
      const int source_size =
          lcg.Below(3) == 0 ? lcg.Below(size - source_start + 1)
                            : size - source_start;
      const int dest_size = lcg.Below(size + 1);
      const auto layout = [&](std::vector<uint8_t>& buffer) {
        return std::pair(std::span<const uint8_t>(buffer).subspan(
                             base::ToSize(source_start),
                             base::ToSize(source_size)),
                         std::span(buffer).first(base::ToSize(dest_size)));
      };
      const auto [want_source, want_dest] = layout(want);
      const auto [got_source, got_dest] = layout(got);
      ASSERT_EQ(decoder.Decode(got_source, got_dest),
                reference.Decode(want_source, want_dest));
      ASSERT_TRUE(got == want)
          << "round " << round << ": source " << source_start << "+"
          << source_size << ", dest " << dest_size << " of " << size;
    }
  }
}

TEST(ImaAdpcmDifferentialTest, StreamAtTheEndOfItsDestination) {
  base::Lcg lcg(3);
  for (int round = 0; round < 500; ++round) {
    ImaAdpcmDecoder decoder;
    adpcm_decoders_reference::ImaAdpcmDecoder reference;
    const int source_size = 1 + lcg.Below(300);
    auto want = RandomBuffer(lcg, source_size * 4);
    auto got = want;
    const auto tail = base::ToSize(source_size * 3);
    ASSERT_TRUE(reference.Decode(std::span(want).subspan(tail), want));
    ASSERT_TRUE(decoder.Decode(std::span(got).subspan(tail), got));
    ASSERT_TRUE(got == want);
  }
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
