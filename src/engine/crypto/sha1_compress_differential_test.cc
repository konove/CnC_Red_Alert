// Holds the Rust SHA-1 block compression in src/rust to the C++ one it
// replaced: the same state from any starting state, for one block and for
// many.

#include <cstddef>
#include <cstdint>
#include <vector>

#include "base/lcg_test_util.h"
#include "engine/crypto/sha1_compress.h"
#include "engine/crypto/sha1_compress_reference_test_util.h"
#include "gtest/gtest.h"

namespace {

using engine::crypto::kSha1BlockSize;
using engine::crypto::Sha1State;

Sha1State RandomState(base::Lcg& lcg) {
  Sha1State state{};
  for (uint32_t& word : state) {
    word = (lcg.Next() << 8U) ^ lcg.Next();
  }
  return state;
}

// Whichever code the sha1 crate runs on this CPU, against the portable C++.
TEST(Sha1CompressDifferentialTest, MatchesPortableReference) {
  base::Lcg lcg(1);
  for (int round = 0; round < 2000; ++round) {
    const auto blocks = lcg.Bytes(lcg.Below(9) * kSha1BlockSize);
    Sha1State want = RandomState(lcg);
    Sha1State got = want;
    sha1_reference::Sha1CompressPortable(want, blocks);
    engine::crypto::Sha1Compress(got, blocks);
    ASSERT_EQ(got, want) << "round " << round;
  }
}

// And against whichever code the C++ ran on it, over longer inputs.
TEST(Sha1CompressDifferentialTest, MatchesDispatchingReference) {
  base::Lcg lcg(3);
  for (int round = 0; round < 500; ++round) {
    const auto blocks = lcg.Bytes(lcg.Below(40) * kSha1BlockSize);
    Sha1State want = RandomState(lcg);
    Sha1State got = want;
    sha1_reference::Sha1Compress(want, blocks);
    engine::crypto::Sha1Compress(got, blocks);
    ASSERT_EQ(got, want) << "round " << round;
  }
}

TEST(Sha1CompressDeathTest, RejectsAPartialBlock) {
  Sha1State state{};
  const std::vector<std::byte> blocks(kSha1BlockSize + 1);
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(engine::crypto::Sha1Compress(state, blocks), "");
}

}  // namespace
