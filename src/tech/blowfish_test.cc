// Tests for the Blowfish block cipher against the published test vectors.

#include "tech/blowfish.h"

#include <algorithm>
#include <array>
#include <span>

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

}  // namespace
