// Tests the seeded random byte generator used for key generation.

#include "tech/random_source.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "gtest/gtest.h"

namespace {

void Seed(RandomSource& rng, int32_t base) {
  for (int32_t value = base; rng.Seed_Bits_Needed() > 0; ++value) {
    rng.Seed_Long(value);
  }
}

TEST(RandomSourceTest, SameSeedGivesSameBytes) {
  RandomSource first;
  RandomSource second;
  Seed(first, 7);
  Seed(second, 7);
  std::array<uint8_t, 1000> a{};
  std::array<uint8_t, 1000> b{};
  EXPECT_EQ(first.Read(std::as_writable_bytes(std::span(a))), 1000);
  EXPECT_EQ(second.Read(std::as_writable_bytes(std::span(b))), 1000);
  EXPECT_EQ(a, b);
}

TEST(RandomSourceTest, DifferentSeedGivesDifferentBytes) {
  RandomSource first;
  RandomSource second;
  Seed(first, 7);
  Seed(second, 8);
  std::array<uint8_t, 64> a{};
  std::array<uint8_t, 64> b{};
  first.Read(std::as_writable_bytes(std::span(a)));
  second.Read(std::as_writable_bytes(std::span(b)));
  EXPECT_NE(a, b);
}

TEST(RandomSourceTest, FillsEveryRequestedByte) {
  RandomSource rng;
  Seed(rng, 3);
  std::array<uint8_t, 4096> bytes{};
  EXPECT_EQ(rng.Read(std::as_writable_bytes(std::span(bytes).first(4000))),
            4000);
  // Bytes past the request stay untouched.
  for (int i = 4000; i < 4096; ++i) {
    EXPECT_EQ(bytes[static_cast<std::size_t>(i)], 0);
  }
  int zeros = 0;
  for (int i = 0; i < 4000; ++i) {
    zeros += bytes[static_cast<std::size_t>(i)] == 0 ? 1 : 0;
  }
  EXPECT_LT(zeros, 100);
}

}  // namespace
