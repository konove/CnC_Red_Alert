// Checks the signed fixed-point helpers in ra/coord.h against the original
// unsigned 32-bit implementations, whose results the simulation depends on.

#include <cstdint>

#include <gtest/gtest.h>

#include "ra/coord.h"

namespace {

// The helpers as they were written before they took signed arguments.
constexpr uint32_t LegacyCardinalToFixed(uint32_t base, uint32_t cardinal) {
  if (base == 0) {
    return 0xFFFF;
  }
  return (cardinal << 8) / base;
}

constexpr uint32_t LegacyFixedToCardinal(uint32_t base, uint32_t fixed) {
  const uint32_t ret = (base * fixed) + 0x80;
  if (ret > 0x00FFFFFF) {
    return 0xFFFF;
  }
  return ret >> 8;
}

int ExpectedCardinalToFixed(int base, int cardinal) {
  return static_cast<int>(LegacyCardinalToFixed(static_cast<uint32_t>(base),
                                                static_cast<uint32_t>(cardinal)));
}

int ExpectedFixedToCardinal(int base, int fixed) {
  return static_cast<int>(LegacyFixedToCardinal(static_cast<uint32_t>(base),
                                                static_cast<uint32_t>(fixed)));
}

TEST(FixedToCardinalTest, MatchesLegacyOverGameDomain) {
  for (int base = 0; base <= 0xFFFF; ++base) {
    for (int fixed = 0; fixed <= 0x1FF; ++fixed) {
      ASSERT_EQ(Fixed_To_Cardinal(base, fixed),
                ExpectedFixedToCardinal(base, fixed))
          << "base " << base << " fixed " << fixed;
    }
  }
}

TEST(CardinalToFixedTest, MatchesLegacyOverGameDomain) {
  for (int base = 0; base <= 0x3FF; ++base) {
    for (int cardinal = 0; cardinal <= 0xFFFF; ++cardinal) {
      ASSERT_EQ(Cardinal_To_Fixed(base, cardinal),
                ExpectedCardinalToFixed(base, cardinal))
          << "base " << base << " cardinal " << cardinal;
    }
  }
}

TEST(FixedPointTest, MatchesLegacyOutsideGameDomain) {
  // Large and negative inputs wrap exactly as the unsigned originals did.
  const int kSamples[] = {-65536, -256, -1,       0x10000,   0x7FFF,
                          0xFFFFFF,  0x1000000, 0x7FFFFFFF, 12345};
  for (int a : kSamples) {
    for (int b : kSamples) {
      EXPECT_EQ(Fixed_To_Cardinal(a, b), ExpectedFixedToCardinal(a, b))
          << a << ", " << b;
      EXPECT_EQ(Cardinal_To_Fixed(a, b), ExpectedCardinalToFixed(a, b))
          << a << ", " << b;
    }
  }
}

TEST(FixedPointTest, IsUsableInConstantExpressions) {
  static_assert(Cardinal_To_Fixed(0, 5) == 0xFFFF);
  static_assert(Cardinal_To_Fixed(4, 2) == 0x80);
  static_assert(Fixed_To_Cardinal(200, 0x80) == 100);
}

}  // namespace
