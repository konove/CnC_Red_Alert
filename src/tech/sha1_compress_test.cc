#include "tech/sha1_compress.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/strings/str_format.h"
#include "gtest/gtest.h"
#include "tech/sha.h"

namespace {

std::string HexDigest(const Sha1Digest& digest) {
  std::string hex;
  for (const std::byte b : digest) {
    absl::StrAppendFormat(&hex, "%02x", std::to_integer<int>(b));
  }
  return hex;
}

std::string ShaHex(std::string_view text) {
  SHAEngine engine;
  engine.Hash(std::as_bytes(std::span(text)));
  return HexDigest(engine.Digest());
}

// Deterministic, non-repeating input so every block differs.
std::vector<std::byte> TestBytes(int size) {
  std::vector<std::byte> bytes(static_cast<std::size_t>(size));
  uint32_t value = 0x12345678U;
  for (std::byte& b : bytes) {
    value = (value * 1664525U) + 1013904223U;
    b = static_cast<std::byte>(value >> 24U);
  }
  return bytes;
}

// FIPS 180 test vectors, through whichever path this CPU uses.
TEST(Sha1CompressTest, KnownDigests) {
  EXPECT_EQ(ShaHex(""), "da39a3ee5e6b4b0d3255bfef95601890afd80709");
  EXPECT_EQ(ShaHex("abc"), "a9993e364706816aba3e25717850c26c9cd0d89d");
  EXPECT_EQ(ShaHex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
            "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
  EXPECT_EQ(ShaHex(std::string(1'000'000, 'a')),
            "34aa973cd4c4daa4f61eeb2bdbad27316534016f");
}

TEST(Sha1CompressTest, HardwareMatchesPortable) {
  if (!tech::Sha1HardwareAvailable()) {
    GTEST_SKIP() << "CPU has no SHA extensions";
  }
  for (const int blocks : {1, 2, 3, 17, 1000}) {
    const std::vector<std::byte> input =
        TestBytes(blocks * tech::kSha1BlockSize);
    tech::Sha1State portable = {0x67452301U, 0xefcdab89U, 0x98badcfeU,
                                0x10325476U, 0xc3d2e1f0U};
    tech::Sha1State hardware = portable;
    tech::Sha1CompressPortable(portable, input);
    tech::Sha1CompressHardware(hardware, input);
    EXPECT_EQ(hardware, portable) << blocks << " blocks";
  }
}

// Hash() splits input into a partial head, whole blocks and a partial tail;
// every split must give the digest of the whole input at once.
TEST(Sha1CompressTest, ChunkedHashingMatchesOneShot) {
  const std::vector<std::byte> input = TestBytes(1000);
  SHAEngine one_shot;
  one_shot.Hash(input);
  const Sha1Digest expected = one_shot.Digest();

  for (const std::size_t chunk : {1U, 7U, 63U, 64U, 65U, 200U}) {
    SHAEngine chunked;
    for (std::size_t offset = 0; offset < input.size(); offset += chunk) {
      chunked.Hash(std::span(input).subspan(
          offset, std::min(chunk, input.size() - offset)));
    }
    EXPECT_EQ(chunked.Digest(), expected) << "chunk " << chunk;
  }
}

}  // namespace
