// Tests for Fixed: parsing and formatting its INI text, and serialization.

#include "base/fixed.h"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "engine/stream/archive.h"
#include "engine/stream/span_source.h"
#include "engine/stream/vector_sink.h"
#include "gtest/gtest.h"

namespace {

// FromString: null and empty input.

TEST(FixedFromStringTest, NullReturnsZero) {
  const Fixed f = Fixed::FromString(nullptr);
  EXPECT_EQ(f.whole(), 0);
  EXPECT_EQ(f.fraction(), 0);
}

TEST(FixedFromStringTest, EmptyStringReturnsZero) {
  const Fixed f = Fixed::FromString("");
  EXPECT_EQ(f.whole(), 0);
  EXPECT_EQ(f.fraction(), 0);
}

// FromString: whole numbers.

TEST(FixedFromStringTest, WholeNumber) {
  const Fixed f = Fixed::FromString("3");
  EXPECT_EQ(f.whole(), 3);
  EXPECT_EQ(f.fraction(), 0);
}

TEST(FixedFromStringTest, Zero) {
  const Fixed f = Fixed::FromString("0");
  EXPECT_EQ(f.whole(), 0);
  EXPECT_EQ(f.fraction(), 0);
}

// FromString: decimal values.

TEST(FixedFromStringTest, DecimalHalf) {
  const Fixed f = Fixed::FromString(".5");
  EXPECT_EQ(f.whole(), 0);
  EXPECT_EQ(f.fraction(), 128);  // 0.5 * 256 = 128
}

TEST(FixedFromStringTest, DecimalOnePointFive) {
  const Fixed f = Fixed::FromString("1.5");
  EXPECT_EQ(f.whole(), 1);
  EXPECT_EQ(f.fraction(), 128);
}

TEST(FixedFromStringTest, DecimalSmallFraction) {
  // ".016" → whole=0, frac = 256 * 16 / 1000 = 4 (truncated)
  const Fixed f = Fixed::FromString(".016");
  EXPECT_EQ(f.whole(), 0);
  EXPECT_EQ(f.fraction(), 4);
}

TEST(FixedFromStringTest, DecimalThreeDigitFraction) {
  // ".083" → whole=0, frac = 256 * 83 / 1000 = 21 (truncated)
  const Fixed f = Fixed::FromString(".083");
  EXPECT_EQ(f.whole(), 0);
  EXPECT_EQ(f.fraction(), 21);
}

TEST(FixedFromStringTest, WholeAndFraction) {
  // "1.25" → whole=1, frac = 256 * 25 / 100 = 64
  const Fixed f = Fixed::FromString("1.25");
  EXPECT_EQ(f.whole(), 1);
  EXPECT_EQ(f.fraction(), 64);
}

// FromString: percentage values.

TEST(FixedFromStringTest, Percentage100) {
  // "100%" → 100 * 256 / 100 = 256 → whole=1, frac=0
  const Fixed f = Fixed::FromString("100%");
  EXPECT_EQ(f.whole(), 1);
  EXPECT_EQ(f.fraction(), 0);
}

TEST(FixedFromStringTest, Percentage50) {
  // "50%" → 50 * 256 / 100 = 128 → whole=0, frac=128
  const Fixed f = Fixed::FromString("50%");
  EXPECT_EQ(f.whole(), 0);
  EXPECT_EQ(f.fraction(), 128);
}

TEST(FixedFromStringTest, Percentage75) {
  // "75%" → 75 * 256 / 100 = 192 → whole=0, frac=192
  const Fixed f = Fixed::FromString("75%");
  EXPECT_EQ(f.whole(), 0);
  EXPECT_EQ(f.fraction(), 192);
}

TEST(FixedFromStringTest, Percentage0) {
  const Fixed f = Fixed::FromString("0%");
  EXPECT_EQ(f.whole(), 0);
  EXPECT_EQ(f.fraction(), 0);
}

TEST(FixedFromStringTest, Percentage200) {
  // "200%" → 200 * 256 / 100 = 512 → whole=2, frac=0
  const Fixed f = Fixed::FromString("200%");
  EXPECT_EQ(f.whole(), 2);
  EXPECT_EQ(f.fraction(), 0);
}

// FromString: leading whitespace.

TEST(FixedFromStringTest, LeadingWhitespace) {
  const Fixed f = Fixed::FromString("  75%");
  EXPECT_EQ(f.whole(), 0);
  EXPECT_EQ(f.fraction(), 192);
}

// FromString: a value from the game rules.

TEST(FixedFromStringTest, GameValuePointTwelve) {
  // ".12" is used for AirstripRatio, HelipadRatio
  const Fixed f = Fixed::FromString(".12");
  EXPECT_EQ(f.whole(), 0);
  // 256 * 12 / 100 = 30 (truncated)
  EXPECT_EQ(f.fraction(), 30);
}

// FromString: long inputs.

TEST(FixedFromStringTest, SevenFractionDigits) {
  // 256 * 9999999 overflows int if computed directly.
  const Fixed f = Fixed::FromString(".9999999");
  EXPECT_EQ(f.whole(), 0);
  EXPECT_EQ(f.fraction(), 255);
}

TEST(FixedFromStringTest, DigitsBeyondEightAreIgnored) {
  // 1/256 is exact in eight decimal places, so later digits cannot change the
  // truncated result, however many there are.
  EXPECT_EQ(Fixed::FromString("0.00390625"), Fixed(1, 256));
  EXPECT_EQ(Fixed::FromString("0.003906249999999"), Fixed(0, 1));
  EXPECT_EQ(Fixed::FromString(".99999999999999999999").fraction(), 255);
}

TEST(FixedFromStringTest, LargeWholePartKeepsLowByte) {
  // 99999999 is 0x5F5E0FF; like the original's (unsigned char) cast, only the
  // low byte is kept.
  EXPECT_EQ(Fixed::FromString("99999999").whole(), 0xFF);
}

// FromString: a view that is not NUL-terminated, and edge cases.

TEST(FixedFromStringTest, StringViewSubstr) {
  // Verify non-null-terminated string_view works correctly.
  constexpr std::string_view full = "1.5extra";
  constexpr std::string_view view = full.substr(0, 3);  // "1.5"
  const Fixed f = Fixed::FromString(view);
  EXPECT_EQ(f.whole(), 1);
  EXPECT_EQ(f.fraction(), 128);
}

TEST(FixedFromStringTest, WhitespaceOnly) {
  const Fixed f = Fixed::FromString("   ");
  EXPECT_EQ(f.whole(), 0);
  EXPECT_EQ(f.fraction(), 0);
}

TEST(FixedFromStringTest, TrailingDotNoFraction) {
  // "3." has a dot but no fractional digits after it.
  const Fixed f = Fixed::FromString("3.");
  EXPECT_EQ(f.whole(), 3);
  EXPECT_EQ(f.fraction(), 0);
}

// ToString: whole numbers.

TEST(FixedToStringTest, Zero) { EXPECT_EQ(Fixed().ToString(), "0"); }

TEST(FixedToStringTest, WholeNumber) {
  EXPECT_EQ(Fixed(uint8_t{3}).ToString(), "3");
}

// ToString: common constants.

TEST(FixedToStringTest, OneHalf) { EXPECT_EQ(Fixed::kHalf.ToString(), "0.5"); }

TEST(FixedToStringTest, OneQuarter) {
  EXPECT_EQ(Fixed::kQuarter.ToString(), "0.25");
}

TEST(FixedToStringTest, ThreeQuarters) {
  EXPECT_EQ(Fixed::kThreeQuarters.ToString(), "0.75");
}

TEST(FixedToStringTest, OneThird) {
  // 1/3 in 8.8 is 85/256, exactly 0.33203125.
  EXPECT_EQ(Fixed::kThird.ToString(), "0.33203125");
}

TEST(FixedToStringTest, TwoThirds) {
  // 2/3 in 8.8 is 170/256, exactly 0.6640625.
  EXPECT_EQ(Fixed(2, 3).ToString(), "0.6640625");
}

// ToString: small fractions keep their leading zeros.

TEST(FixedToStringTest, SmallFraction) {
  EXPECT_EQ(Fixed(12, 256).ToString(), "0.046875");
  EXPECT_EQ(Fixed(1, 256).ToString(), "0.00390625");
}

// ToString: FromString reads every value back unchanged, as the options INI
// needs when it saves and reloads the volumes.

TEST(FixedToStringTest, RoundTripsThroughFromString) {
  for (int whole = 0; whole < 256; ++whole) {
    for (int fraction = 0; fraction < 256; ++fraction) {
      const Fixed value((whole * 256) + fraction, 256);
      ASSERT_EQ(Fixed::FromString(value.ToString()), value) << value.ToString();
    }
  }
}

// Arithmetic with large ints: the int is scaled by 256, which must not
// overflow int.

TEST(FixedIntArithmeticTest, LargeFractionComponents) {
  EXPECT_EQ(Fixed(10000000, 20000000), Fixed(1, 2));
}

TEST(FixedIntArithmeticTest, MultiplyLargeInt) {
  EXPECT_EQ(Fixed(1, 1) * 10000000, 10000000);
  EXPECT_EQ(10000000 * Fixed(1, 2), 5000000);
}

TEST(FixedIntArithmeticTest, DivideLargeInt) {
  EXPECT_EQ(50000000 / Fixed(2, 1), 25000000);
}

TEST(FixedIntArithmeticTest, DivideRoundsToNearest) {
  EXPECT_EQ(1 / Fixed(3, 4), 1);  // 1.33
  EXPECT_EQ(2 / Fixed(3, 4), 3);  // 2.67
  EXPECT_EQ(3 / Fixed(2, 1), 2);  // 1.5 rounds up
  EXPECT_EQ(100 / Fixed(1, 16), 1600);
}

TEST(FixedIntArithmeticTest, DivideByZeroOrOneReturnsLhs) {
  EXPECT_EQ(7 / Fixed(), 7);
  EXPECT_EQ(7 / Fixed(1), 7);
}

TEST(FixedIntArithmeticTest, CompareWithLargeInt) {
  EXPECT_TRUE(Fixed(1, 2) < 10000000);
  EXPECT_FALSE(Fixed(1, 2) > 10000000);
  EXPECT_FALSE(Fixed(1, 2) >= 10000000);
  EXPECT_FALSE(Fixed(1, 2) == 10000000);
  EXPECT_TRUE(Fixed(1, 2) != 10000000);
}

// Serialize: the raw 8.8 pattern round-trips through the archive.

TEST(FixedSerializeTest, RoundTripsRawBits) {
  for (const Fixed value :
       {Fixed(0, 1), Fixed(3, 4), Fixed(255, 1), Fixed(65535, 256)}) {
    std::vector<std::byte> bytes;
    VectorSink sink(bytes);
    ArchiveWriter writer(sink);
    Fixed out = value;
    out.Serialize(writer);
    EXPECT_EQ(bytes.size(), 2U);

    SpanSource source(bytes);
    ArchiveReader reader(source);
    Fixed in;
    in.Serialize(reader);
    EXPECT_TRUE(reader.ok());
    EXPECT_EQ(in, value);
  }
}

}  // namespace
