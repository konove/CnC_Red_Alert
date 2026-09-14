// Tests for the Blowfish block cipher against the published test vectors.

#include "tech/blowfish.h"

#include <array>

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
    engine.Submit_Key(vector.key.data(), static_cast<int>(vector.key.size()));

    Block cypher{};
    EXPECT_EQ(engine.Encrypt(vector.plain.data(),
                             static_cast<int>(vector.plain.size()),
                             cypher.data()),
              8);
    EXPECT_EQ(cypher, vector.cypher);
  }
}

TEST(BlowfishEngineTest, DecryptsReferenceVectors) {
  for (const Vector& vector : kVectors) {
    BlowfishEngine engine;
    engine.Submit_Key(vector.key.data(), static_cast<int>(vector.key.size()));

    Block plain{};
    EXPECT_EQ(engine.Decrypt(vector.cypher.data(),
                             static_cast<int>(vector.cypher.size()),
                             plain.data()),
              8);
    EXPECT_EQ(plain, vector.plain);
  }
}

}  // namespace
