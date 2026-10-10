// Holds the Rust SHA-1 block compression in src/rust to the C++ one it
// replaced: the same state from any starting state, for one block and for
// many, in the portable code and with the SHA extensions.

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

TEST(Sha1CompressDifferentialTest, PortableMatchesReference) {
  base::Lcg lcg(1);
  for (int round = 0; round < 2000; ++round) {
    const auto blocks = lcg.Bytes(lcg.Below(9) * kSha1BlockSize);
    Sha1State want = RandomState(lcg);
    Sha1State got = want;
    sha1_reference::Sha1CompressPortable(want, blocks);
    engine::crypto::Sha1CompressPortable(got, blocks);
    ASSERT_EQ(got, want) << "round " << round;
  }
}

TEST(Sha1CompressDifferentialTest, HardwareMatchesReference) {
  if (!engine::crypto::Sha1HardwareAvailable()) {
    GTEST_SKIP() << "CPU has no SHA extensions";
  }
  base::Lcg lcg(2);
  for (int round = 0; round < 2000; ++round) {
    const auto blocks = lcg.Bytes(lcg.Below(9) * kSha1BlockSize);
    Sha1State want = RandomState(lcg);
    Sha1State got = want;
    // The portable reference, so that this also runs where the C++ hardware
    // path cannot.
    sha1_reference::Sha1CompressPortable(want, blocks);
    engine::crypto::Sha1CompressHardware(got, blocks);
    ASSERT_EQ(got, want) << "round " << round;
  }
}

TEST(Sha1CompressDifferentialTest, DispatchMatchesReference) {
  EXPECT_EQ(engine::crypto::Sha1HardwareAvailable(),
            sha1_reference::Sha1HardwareAvailable());
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
