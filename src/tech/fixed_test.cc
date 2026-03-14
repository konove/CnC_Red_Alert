#include "tech/fixed.h"

#include <string>
#include <string_view>

#include "gtest/gtest.h"

namespace {

// FromString: null and empty input.

TEST(FixedFromStringTest, NullReturnsZero) {
  const fixed f = fixed::FromString(nullptr);
  EXPECT_EQ(f.whole(), 0);
  EXPECT_EQ(f.fraction(), 0);
}

TEST(FixedFromStringTest, EmptyStringReturnsZero) {
  const fixed f = fixed::FromString("");
  EXPECT_EQ(f.whole(), 0);
  EXPECT_EQ(f.fraction(), 0);
}

// FromString: whole numbers.

TEST(FixedFromStringTest, WholeNumber) {
  const fixed f = fixed::FromString("3");
  EXPECT_EQ(f.whole(), 3);
  EXPECT_EQ(f.fraction(), 0);
}

TEST(FixedFromStringTest, Zero) {
  const fixed f = fixed::FromString("0");
  EXPECT_EQ(f.whole(), 0);
  EXPECT_EQ(f.fraction(), 0);
}

// FromString: decimal values.

TEST(FixedFromStringTest, DecimalHalf) {
  const fixed f = fixed::FromString(".5");
  EXPECT_EQ(f.whole(), 0);
  EXPECT_EQ(f.fraction(), 128);  // 0.5 * 256 = 128
}

TEST(FixedFromStringTest, DecimalOnePointFive) {
  const fixed f = fixed::FromString("1.5");
  EXPECT_EQ(f.whole(), 1);
  EXPECT_EQ(f.fraction(), 128);
}

TEST(FixedFromStringTest, DecimalSmallFraction) {
  // ".016" → whole=0, frac = 256 * 16 / 1000 = 4 (truncated)
  const fixed f = fixed::FromString(".016");
  EXPECT_EQ(f.whole(), 0);
  EXPECT_EQ(f.fraction(), 4);
}

TEST(FixedFromStringTest, DecimalThreeDigitFraction) {
  // ".083" → whole=0, frac = 256 * 83 / 1000 = 21 (truncated)
  const fixed f = fixed::FromString(".083");
  EXPECT_EQ(f.whole(), 0);
  EXPECT_EQ(f.fraction(), 21);
}

TEST(FixedFromStringTest, WholeAndFraction) {
  // "1.25" → whole=1, frac = 256 * 25 / 100 = 64
  const fixed f = fixed::FromString("1.25");
  EXPECT_EQ(f.whole(), 1);
  EXPECT_EQ(f.fraction(), 64);
}

// FromString: percentage values.

TEST(FixedFromStringTest, Percentage100) {
  // "100%" → 100 * 256 / 100 = 256 → whole=1, frac=0
  const fixed f = fixed::FromString("100%");
  EXPECT_EQ(f.whole(), 1);
  EXPECT_EQ(f.fraction(), 0);
}

TEST(FixedFromStringTest, Percentage50) {
  // "50%" → 50 * 256 / 100 = 128 → whole=0, frac=128
  const fixed f = fixed::FromString("50%");
  EXPECT_EQ(f.whole(), 0);
  EXPECT_EQ(f.fraction(), 128);
}

TEST(FixedFromStringTest, Percentage75) {
  // "75%" → 75 * 256 / 100 = 192 → whole=0, frac=192
  const fixed f = fixed::FromString("75%");
  EXPECT_EQ(f.whole(), 0);
  EXPECT_EQ(f.fraction(), 192);
}

TEST(FixedFromStringTest, Percentage0) {
  const fixed f = fixed::FromString("0%");
  EXPECT_EQ(f.whole(), 0);
  EXPECT_EQ(f.fraction(), 0);
}

TEST(FixedFromStringTest, Percentage200) {
  // "200%" → 200 * 256 / 100 = 512 → whole=2, frac=0
  const fixed f = fixed::FromString("200%");
  EXPECT_EQ(f.whole(), 2);
  EXPECT_EQ(f.fraction(), 0);
}

// FromString: leading whitespace.

TEST(FixedFromStringTest, LeadingWhitespace) {
  const fixed f = fixed::FromString("  75%");
  EXPECT_EQ(f.whole(), 0);
  EXPECT_EQ(f.fraction(), 192);
}

// FromString: values used in the actual game code.

TEST(FixedFromStringTest, GameValuePatrolTime) {
  // ".016" is used for PatrolTime, RepairRate, Rate, AARate, etc.
  const fixed f = fixed::FromString(".016");
  EXPECT_EQ(f, fixed::FromString(".016"));
  EXPECT_GT(f, fixed(0, 1));
}

TEST(FixedFromStringTest, GameValuePointFive) {
  // ".5" is used for DefenseRatio
  const fixed f = fixed::FromString(".5");
  EXPECT_EQ(f, fixed(1, 2));
}

TEST(FixedFromStringTest, GameValuePointTwelve) {
  // ".12" is used for AirstripRatio, HelipadRatio
  const fixed f = fixed::FromString(".12");
  EXPECT_EQ(f.whole(), 0);
  // 256 * 12 / 100 = 30 (truncated)
  EXPECT_EQ(f.fraction(), 30);
}

// FromString: accepts std::string_view and std::string.

TEST(FixedFromStringTest, StringView) {
  constexpr std::string_view sv = "1.5";
  const fixed f = fixed::FromString(sv);
  EXPECT_EQ(f.whole(), 1);
  EXPECT_EQ(f.fraction(), 128);
}

TEST(FixedFromStringTest, StringViewSubstr) {
  // Verify non-null-terminated string_view works correctly.
  constexpr std::string_view full = "1.5extra";
  constexpr std::string_view sv = full.substr(0, 3);  // "1.5"
  const fixed f = fixed::FromString(sv);
  EXPECT_EQ(f.whole(), 1);
  EXPECT_EQ(f.fraction(), 128);
}

TEST(FixedFromStringTest, StdString) {
  const std::string s = "75%";
  const fixed f = fixed::FromString(s);
  EXPECT_EQ(f.whole(), 0);
  EXPECT_EQ(f.fraction(), 192);
}

TEST(FixedFromStringTest, WhitespaceOnly) {
  const fixed f = fixed::FromString("   ");
  EXPECT_EQ(f.whole(), 0);
  EXPECT_EQ(f.fraction(), 0);
}

TEST(FixedFromStringTest, TrailingDotNoFraction) {
  // "3." has a dot but no fractional digits after it.
  const fixed f = fixed::FromString("3.");
  EXPECT_EQ(f.whole(), 3);
  EXPECT_EQ(f.fraction(), 0);
}

TEST(FixedFromStringTest, WholeNumberOnly) {
  // "7" has no dot at all — exercises the dot==npos branch.
  const fixed f = fixed::FromString("7");
  EXPECT_EQ(f.whole(), 7);
  EXPECT_EQ(f.fraction(), 0);
}

// AsString: whole numbers.

TEST(FixedAsStringTest, Zero) { EXPECT_EQ(fixed().AsString(), "0"); }

TEST(FixedAsStringTest, WholeNumber) {
  EXPECT_EQ(fixed(uint8_t{3}).AsString(), "3");
}

TEST(FixedAsStringTest, WholeOne) {
  EXPECT_EQ(fixed(uint8_t{1}).AsString(), "1");
}

// AsString: common constants.

TEST(FixedAsStringTest, OneHalf) { EXPECT_EQ(fixed::_1_2.AsString(), "0.5"); }

TEST(FixedAsStringTest, OneQuarter) {
  EXPECT_EQ(fixed::_1_4.AsString(), "0.25");
}

TEST(FixedAsStringTest, ThreeQuarters) {
  EXPECT_EQ(fixed::_3_4.AsString(), "0.75");
}

TEST(FixedAsStringTest, OneThird) {
  // 1/3 in 8.8 = 85/256 ≈ 0.332
  EXPECT_EQ(fixed::_1_3.AsString(), "0.332");
}

TEST(FixedAsStringTest, TwoThirds) {
  // 2/3 in 8.8 = 170/256 ≈ 0.664
  EXPECT_EQ(fixed::_2_3.AsString(), "0.664");
}

// AsString: values with whole and fractional parts.

TEST(FixedAsStringTest, OnePointFive) {
  EXPECT_EQ(fixed::FromString("1.5").AsString(), "1.5");
}

// AsString: trailing zeros are stripped.

TEST(FixedAsStringTest, StripsTrailingZeros) {
  // fixed(1, 4) has fraction 64 → 64*1000/256 = 250 → "0.250" → "0.25"
  EXPECT_EQ(fixed(1, 4).AsString(), "0.25");
}

}  // namespace
