#include "engine/crypto/pk.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "engine/crypto/random_source.h"
#include "gtest/gtest.h"

namespace {

// The modulus of the public key the games' MIX archives are made for, as the
// DER integer their source code holds in base64.
constexpr std::array<uint8_t, 42> kMixModulus = {
    0x02, 0x28, 0x51, 0xbc, 0xda, 0x08, 0x6d, 0x39, 0xfc, 0xe4, 0x56,
    0x51, 0x60, 0xd6, 0x51, 0x71, 0x3f, 0xa2, 0xe8, 0xaa, 0x54, 0xfa,
    0x66, 0x82, 0xb0, 0x4a, 0xab, 0xdd, 0x0e, 0x6a, 0xf8, 0xb0, 0xc1,
    0xe6, 0xd1, 0xfb, 0x4f, 0x3d, 0xaa, 0x43, 0x7f, 0x15};

PKey MixKey() {
  PKey key;
  key.SetFastExponent();
  key.Decode_Modulus(std::as_bytes(std::span(kMixModulus)));
  return key;
}

std::vector<uint8_t> Encoded(const PKey& key, const bool modulus) {
  std::vector<uint8_t> buffer(PKey::kMaxEncodedBytes);
  const auto bytes = std::as_writable_bytes(std::span(buffer));
  buffer.resize(static_cast<std::size_t>(modulus ? key.Encode_Modulus(bytes)
                                                 : key.Encode_Exponent(bytes)));
  return buffer;
}

void Seed(RandomSource& rng) {
  for (int32_t value = 1; rng.Seed_Bits_Needed() > 0; ++value) {
    rng.Seed_Long(value * 40503);
  }
}

TEST(PKeyTest, ANewKeyHasNoBlocks) {
  const PKey key;
  EXPECT_EQ(key.Plain_Block_Size(), 0);
  EXPECT_EQ(key.Crypt_Block_Size(), 1);
  std::array<std::byte, 16> data{};
  std::array<std::byte, 16> out{};
  EXPECT_EQ(key.Encrypt(data, out), 0);
  EXPECT_EQ(key.Decrypt(data, out), 0);
}

TEST(PKeyTest, TheFastExponentIs65537) {
  PKey key;
  key.SetFastExponent();
  EXPECT_EQ(Encoded(key, /*modulus=*/false),
            (std::vector<uint8_t>{2, 3, 1, 0, 1}));
  EXPECT_EQ(PKey::Fast_Exponent(), 65537U);
}

TEST(PKeyTest, BlockSizesFollowTheModulus) {
  const PKey key = MixKey();
  // 319 bits: a block of 39 bytes is always below the modulus.
  EXPECT_EQ(key.Plain_Block_Size(), 39);
  EXPECT_EQ(key.Crypt_Block_Size(), 40);
  EXPECT_EQ(key.Block_Count(56), 2);
  EXPECT_EQ(Encoded(key, /*modulus=*/true),
            std::vector<uint8_t>(kMixModulus.begin(), kMixModulus.end()));
}

// Python's pow(block, 65537, modulus) with the numbers least significant byte
// first, as a block is stored.
TEST(PKeyTest, DecryptsABlockWithTheMixKey) {
  std::array<uint8_t, 40> cipher{};
  for (std::size_t i = 0; i < 39; ++i) {
    cipher.at(i) = static_cast<uint8_t>(i + 1);
  }
  cipher.at(39) = 0x10;
  constexpr std::array<uint8_t, 39> kPlain = {
      0xc3, 0x2a, 0xaf, 0xf0, 0x5b, 0xb0, 0x16, 0x0f, 0x10, 0xe4,
      0xc9, 0xd1, 0x07, 0xa4, 0x1a, 0xa3, 0xaa, 0xbb, 0x30, 0xa9,
      0xfb, 0xc8, 0xf5, 0x0a, 0x6f, 0x24, 0xb4, 0x4e, 0xfa, 0x02,
      0x47, 0x2c, 0x8d, 0xfc, 0x5d, 0x9d, 0x5c, 0x4e, 0xde};
  std::array<uint8_t, 39> plain{};
  EXPECT_EQ(MixKey().Decrypt(std::as_bytes(std::span(cipher)),
                             std::as_writable_bytes(std::span(plain))),
            39);
  EXPECT_EQ(plain, kPlain);
}

TEST(PKeyTest, TruncatedDerLeavesTheKeyUnchanged) {
  PKey key = MixKey();
  // An integer that claims three bytes and has one.
  const std::array<uint8_t, 3> truncated = {2, 3, 1};
  key.Decode_Modulus(std::as_bytes(std::span(truncated)));
  key.Decode_Exponent(std::as_bytes(std::span(truncated)));
  EXPECT_EQ(key.Plain_Block_Size(), 39);
  EXPECT_EQ(Encoded(key, /*modulus=*/true),
            std::vector<uint8_t>(kMixModulus.begin(), kMixModulus.end()));
  EXPECT_EQ(Encoded(key, /*modulus=*/false),
            (std::vector<uint8_t>{2, 3, 1, 0, 1}));
}

TEST(PKeyTest, AnEmptyBufferEncodesNothing) {
  EXPECT_EQ(MixKey().Encode_Modulus({}), 0);
  EXPECT_EQ(MixKey().Encode_Exponent({}), 0);
}

TEST(PKeyTest, GeneratedKeysUndoEachOther) {
  RandomSource rng;
  Seed(rng);
  PKey fast;
  PKey slow;
  PKey::Generate(rng, 96, fast, slow);
  EXPECT_EQ(Encoded(fast, /*modulus=*/true), Encoded(slow, /*modulus=*/true));
  EXPECT_EQ(Encoded(fast, /*modulus=*/false),
            (std::vector<uint8_t>{2, 3, 1, 0, 1}));
  EXPECT_EQ(fast.Plain_Block_Size(), slow.Plain_Block_Size());
  // Two 96-bit primes multiply to 191 or 192 bits.
  EXPECT_EQ(fast.Plain_Block_Size(), 23);

  // Three whole blocks and a part of one, which is left out.
  constexpr std::size_t kPlain = 23;
  constexpr std::size_t kCrypt = 24;
  std::vector<std::byte> plain((kPlain * 3) + 5);
  rng.Read(plain);
  std::vector<std::byte> cipher(kCrypt * 4);
  ASSERT_EQ(fast.Encrypt(plain, cipher), 24 * 3);
  std::vector<std::byte> round_trip(plain.size());
  ASSERT_EQ(slow.Decrypt(std::span(cipher).first(kCrypt * 3), round_trip),
            23 * 3);
  EXPECT_TRUE(std::ranges::equal(std::span(round_trip).first(kPlain * 3),
                                 std::span(plain).first(kPlain * 3)));

  // A destination with room for two blocks takes two.
  EXPECT_EQ(fast.Encrypt(plain, std::span(cipher).first((kCrypt * 3) - 1)),
            24 * 2);
}

// The slow key wins, as it did in C++.
TEST(PKeyTest, GeneratingBothHalvesIntoOneKeyKeepsTheSlowOne) {
  RandomSource rng;
  Seed(rng);
  PKey fast;
  PKey slow;
  PKey::Generate(rng, 64, fast, slow);
  RandomSource same_rng;
  Seed(same_rng);
  PKey both;
  PKey::Generate(same_rng, 64, both, both);
  EXPECT_EQ(Encoded(both, /*modulus=*/true), Encoded(slow, /*modulus=*/true));
  EXPECT_EQ(Encoded(both, /*modulus=*/false), Encoded(slow, /*modulus=*/false));
}

TEST(PKeyTest, DecryptsInTheBufferTheBlocksAreIn) {
  RandomSource rng;
  Seed(rng);
  PKey fast;
  PKey slow;
  PKey::Generate(rng, 64, fast, slow);
  const int plain_size = fast.Plain_Block_Size();
  const int crypt_size = fast.Crypt_Block_Size();
  std::vector<std::byte> plain(static_cast<std::size_t>(plain_size) * 2);
  rng.Read(plain);
  std::vector<std::byte> buffer(static_cast<std::size_t>(crypt_size) * 2);
  ASSERT_EQ(fast.Encrypt(plain, buffer), crypt_size * 2);
  // Each decrypted block is shorter than its source, so the output trails the
  // input through the one buffer.
  ASSERT_EQ(slow.Decrypt(buffer, buffer), plain_size * 2);
  EXPECT_TRUE(
      std::ranges::equal(std::span(buffer).first(plain.size()), plain));
}

TEST(PKeyDeathTest, RejectsPrimesOutOfRange) {
  RandomSource rng;
  Seed(rng);
  PKey fast;
  PKey slow;
  // NOLINTBEGIN(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(PKey::Generate(rng, 31, fast, slow), "Primes of 31 bits");
  EXPECT_DEATH(PKey::Generate(rng, 1025, fast, slow), "Primes of 1025 bits");
  // NOLINTEND(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
}

TEST(PKeyDeathTest, RejectsABufferTooShortForTheEncoding) {
  std::array<std::byte, 10> buffer{};
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(static_cast<void>(MixKey().Encode_Modulus(buffer)),
               "does not fit its buffer");
}

}  // namespace
