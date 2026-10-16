// Tests for the Blowfish block cipher against the published test vectors.

#include "engine/crypto/blowfish.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "gtest/gtest.h"

namespace {

using Block = std::array<unsigned char, 8>;

struct Vector {
  Block key;
  Block plain;
  Block cypher;
};

// Eric Young's reference vectors. The engine keeps its words in uint32_t, so
// the round function must still reduce S-box sums modulo 2^32.
constexpr std::array<Vector, 3> kVectors = {{
    {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x4E, 0xF9, 0x97, 0x45, 0x61, 0x98, 0xDD, 0x78}},
    {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
     {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
     {0x51, 0x86, 0x6F, 0xD5, 0xB8, 0x5E, 0xCB, 0x8A}},
    {{0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
     {0x7D, 0x85, 0x6F, 0x9A, 0x61, 0x30, 0x63, 0xF2}},
}};

TEST(BlowfishEngineTest, EncryptsReferenceVectors) {
  for (const Vector& vector : kVectors) {
    BlowfishEngine engine;
    engine.Submit_Key(std::as_bytes(std::span(vector.key)));

    Block cypher{};
    EXPECT_EQ(engine.Encrypt(std::as_bytes(std::span(vector.plain)),
                             std::as_writable_bytes(std::span(cypher))),
              8);
    EXPECT_EQ(cypher, vector.cypher);
  }
}

TEST(BlowfishEngineTest, DecryptsReferenceVectors) {
  for (const Vector& vector : kVectors) {
    BlowfishEngine engine;
    engine.Submit_Key(std::as_bytes(std::span(vector.key)));

    Block plain{};
    EXPECT_EQ(engine.Decrypt(std::as_bytes(std::span(vector.cypher)),
                             std::as_writable_bytes(std::span(plain))),
              8);
    EXPECT_EQ(plain, vector.plain);
  }
}

// In-place operation is the same non-const buffer passed as source and
// destination; the trailing partial block travels through untouched.
TEST(BlowfishEngineTest, RoundTripsInPlaceThroughOneBuffer) {
  const Vector& vector = kVectors.at(2);
  BlowfishEngine engine;
  engine.Submit_Key(std::as_bytes(std::span(vector.key)));

  std::array<unsigned char, 11> data{};
  std::ranges::copy(vector.plain, data.begin());
  data.at(8) = 0xAA;
  data.at(9) = 0xBB;
  data.at(10) = 0xCC;
  const std::array<unsigned char, 11> original = data;

  EXPECT_EQ(engine.Encrypt(std::as_bytes(std::span(data)),
                           std::as_writable_bytes(std::span(data))),
            8);
  EXPECT_TRUE(
      std::equal(vector.cypher.begin(), vector.cypher.end(), data.begin()));
  EXPECT_EQ(data.at(8), 0xAA);
  EXPECT_EQ(data.at(10), 0xCC);

  EXPECT_EQ(engine.Decrypt(std::as_bytes(std::span(data)),
                           std::as_writable_bytes(std::span(data))),
            8);
  EXPECT_EQ(data, original);
}

// The key whose first n bytes key the engine in KeysOfEveryLengthEncrypt.
// Its first 24 bytes are Eric Young's set_key test key; the rest are any.
std::vector<std::byte> SetKeyTestKey() {
  constexpr std::array<uint8_t, 24> kYoung = {
      0xF0, 0xE1, 0xD2, 0xC3, 0xB4, 0xA5, 0x96, 0x87, 0x78, 0x69, 0x5A, 0x4B,
      0x3C, 0x2D, 0x1E, 0x0F, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
  std::vector<std::byte> key(BlowfishEngine::kMaxKeyLength);
  for (std::size_t i = 0; i < key.size(); ++i) {
    key.at(i) =
        static_cast<std::byte>(i < kYoung.size() ? kYoung.at(i) : (i * 29) + 7);
  }
  return key;
}

// FEDCBA9876543210 under the first 1 to 56 bytes of SetKeyTestKey(), as one
// big-endian number. Lengths 1 to 24 are Eric Young's published set_key
// vectors; 25 to 56 are what the original C++ engine gave, recorded before it
// was deleted (docs/RUST_BLOWFISH_PLAN.md).
constexpr std::array<uint64_t, BlowfishEngine::kMaxKeyLength> kSetKeyCiphers = {
    0xF9AD597C49DB005EU, 0xE91D21C1D961A6D6U, 0xE9C2B70A1BC65CF3U,
    0xBE1E639408640F05U, 0xB39E44481BDB1E6EU, 0x9457AA83B1928C0DU,
    0x8BB77032F960629DU, 0xE87A244E2CC85E82U, 0x15750E7A4F4EC577U,
    0x122BA70B3AB64AE0U, 0x3A833C9AFFC537F6U, 0x9409DA87A90F6BF2U,
    0x884F80625060B8B4U, 0x1F85031C19E11968U, 0x79D9373A714CA34FU,
    0x93142887EE3BE15CU, 0x03429E838CE2D14BU, 0xA4299E27469FF67BU,
    0xAFD5AED1C1BC96A8U, 0x10851C0E3858DA9FU, 0xE6F51ED79B9DB21FU,
    0x64A6E14AFD36B46FU, 0x80C7D7D45A5479ADU, 0x05044B62FA52D080U,
    0x905A7E141058B05EU, 0xDD8BB3406DF19E9AU, 0x9F84F402A7DB2106U,
    0x1270F055F183D2B9U, 0xDA22769BD573D636U, 0x7EF8137FA4736A67U,
    0xAB26DBA8AFB5F68FU, 0x7907CDA93923CCD4U, 0x17EB23881F780A35U,
    0x2F401C06588C62DEU, 0xB4F627044AFB0233U, 0xB0FB895AB38CFEF6U,
    0x8CDB4656617E0A58U, 0x1EE853904AAA5E3FU, 0xF405082ECFEF3EE4U,
    0x64B9D65BF720D4D8U, 0xA03A9639D3F48E3AU, 0x1452704A1AA8B0B0U,
    0x49B8D636C7AE8D30U, 0x4B2AAC17517B7B6DU, 0xFF43625B552337A3U,
    0x602B0EC4335D28B2U, 0x81B7573DB52EBC5CU, 0x24FEDE49923B70ADU,
    0x6B400BEF7AD9FE57U, 0xDE334B6224D1390BU, 0x80FF9F1B1375922DU,
    0xE9C3FD6E9E8C017CU, 0x1C756618C35D232AU, 0x2EED8EEB9199D296U,
    0x13C7A207783E783EU, 0x47E930A5488C264AU,
};

TEST(BlowfishEngineTest, KeysOfEveryLengthEncrypt) {
  const auto key = SetKeyTestKey();
  constexpr Block kPlain = {0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10};
  for (std::size_t length = 1; length <= key.size(); ++length) {
    BlowfishEngine engine;
    engine.Submit_Key(std::span(key).first(length));
    Block want{};
    for (std::size_t i = 0; i < want.size(); ++i) {
      want.at(i) = static_cast<unsigned char>(kSetKeyCiphers.at(length - 1) >>
                                              (56 - (8 * i)));
    }

    Block cypher{};
    ASSERT_EQ(engine.Encrypt(std::as_bytes(std::span(kPlain)),
                             std::as_writable_bytes(std::span(cypher))),
              8);
    EXPECT_EQ(cypher, want) << length << "-byte key";
    Block plain{};
    ASSERT_EQ(engine.Decrypt(std::as_bytes(std::span(cypher)),
                             std::as_writable_bytes(std::span(plain))),
              8);
    EXPECT_EQ(plain, kPlain) << length << "-byte key";
  }
}

// A new key replaces the old one whole, and an empty key leaves none: the
// data is then copied, and the bytes past it in the destination left alone.
TEST(BlowfishEngineTest, ANewKeyReplacesTheOldAndAnEmptyOneRemovesIt) {
  const Vector& first = kVectors.at(1);
  const Vector& second = kVectors.at(2);
  BlowfishEngine engine;
  const auto encrypt = [&engine](const Block& plain) {
    Block cypher{};
    EXPECT_EQ(engine.Encrypt(std::as_bytes(std::span(plain)),
                             std::as_writable_bytes(std::span(cypher))),
              8);
    return cypher;
  };
  engine.Submit_Key(std::as_bytes(std::span(first.key)));
  EXPECT_EQ(encrypt(first.plain), first.cypher);
  engine.Submit_Key(std::as_bytes(std::span(second.key)));
  EXPECT_EQ(encrypt(second.plain), second.cypher);

  engine.Submit_Key({});
  const std::array<unsigned char, 5> data{1, 2, 3, 4, 5};
  std::array<unsigned char, 7> dest{};
  dest.fill(0xcc);
  EXPECT_EQ(engine.Encrypt(std::as_bytes(std::span(data)),
                           std::as_writable_bytes(std::span(dest))),
            5);
  EXPECT_EQ(dest, (std::array<unsigned char, 7>{1, 2, 3, 4, 5, 0xcc, 0xcc}));
}

TEST(BlowfishDeathTest, RejectsBuffersThatPartlyOverlap) {
  std::vector<std::byte> buffer(32);
  BlowfishEngine engine;
  engine.Submit_Key(std::span(buffer).first(8));
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(engine.Encrypt(std::span(buffer).first(16),
                              std::span(buffer).subspan(8, 16)),
               "without being one buffer");
}

TEST(BlowfishDeathTest, RejectsAKeyThatIsTooLong) {
  const std::vector<std::byte> key(BlowfishEngine::kMaxKeyLength + 1);
  BlowfishEngine engine;
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(engine.Submit_Key(key), "");
}

}  // namespace
