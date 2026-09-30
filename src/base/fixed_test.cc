// Tests for fixed: parsing and formatting its INI text, and serialization.

#include "base/fixed.h"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "engine/stream/archive.h"
#include "engine/stream/byte_sink.h"
#include "engine/stream/span_source.h"
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

// FromString: long inputs.

TEST(FixedFromStringTest, SevenFractionDigits) {
  // 256 * 9999999 overflows int if computed directly.
  const fixed f = fixed::FromString(".9999999");
  EXPECT_EQ(f.whole(), 0);
  EXPECT_EQ(f.fraction(), 255);
}

TEST(FixedFromStringTest, DigitsBeyondEightAreIgnored) {
  // 1/256 is exact in eight decimal places, so later digits cannot change the
  // truncated result, however many there are.
  EXPECT_EQ(fixed::FromString("0.00390625"), fixed(1, 256));
  EXPECT_EQ(fixed::FromString("0.003906249999999"), fixed(0, 1));
  EXPECT_EQ(fixed::FromString(".99999999999999999999").fraction(), 255);
}

TEST(FixedFromStringTest, LargeWholePartKeepsLowByte) {
  // 99999999 is 0x5F5E0FF; like the original's (unsigned char) cast, only the
  // low byte is kept.
  EXPECT_EQ(fixed::FromString("99999999").whole(), 0xFF);
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
  // 1/3 in 8.8 is 85/256, exactly 0.33203125.
  EXPECT_EQ(fixed::_1_3.AsString(), "0.33203125");
}

TEST(FixedAsStringTest, TwoThirds) {
  // 2/3 in 8.8 is 170/256, exactly 0.6640625.
  EXPECT_EQ(fixed::_2_3.AsString(), "0.6640625");
}

// AsString: small fractions keep their leading zeros.

TEST(FixedAsStringTest, SmallFraction) {
  EXPECT_EQ(fixed(12, 256).AsString(), "0.046875");
  EXPECT_EQ(fixed(1, 256).AsString(), "0.00390625");
}

// AsString: FromString reads every value back unchanged, as the options INI
// needs when it saves and reloads the volumes.

TEST(FixedAsStringTest, RoundTripsThroughFromString) {
  for (int whole = 0; whole < 256; ++whole) {
    for (int fraction = 0; fraction < 256; ++fraction) {
      const fixed value((whole * 256) + fraction, 256);
      ASSERT_EQ(fixed::FromString(value.AsString()), value) << value.AsString();
    }
  }
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

// Arithmetic with large ints: the int is scaled by 256, which must not
// overflow int.

TEST(FixedIntArithmeticTest, LargeFractionComponents) {
  EXPECT_EQ(fixed(10000000, 20000000), fixed(1, 2));
}

TEST(FixedIntArithmeticTest, MultiplyLargeInt) {
  EXPECT_EQ(fixed(1, 1) * 10000000, 10000000);
  EXPECT_EQ(10000000 * fixed(1, 2), 5000000);
}

TEST(FixedIntArithmeticTest, DivideLargeInt) {
  EXPECT_EQ(50000000 / fixed(2, 1), 25000000);
}

TEST(FixedIntArithmeticTest, CompareWithLargeInt) {
  EXPECT_TRUE(fixed(1, 2) < 10000000);
  EXPECT_FALSE(fixed(1, 2) > 10000000);
  EXPECT_FALSE(fixed(1, 2) >= 10000000);
  EXPECT_FALSE(fixed(1, 2) == 10000000);
  EXPECT_TRUE(fixed(1, 2) != 10000000);
}

// Serialize: the raw 8.8 pattern round-trips through the archive.

class RecordingSink : public ByteSink {
 public:
  bool Write(std::span<const std::byte> data) override {
    for (const std::byte byte : data) {
      bytes.push_back(std::to_integer<uint8_t>(byte));
    }
    return true;
  }
  std::vector<uint8_t> bytes;
};

TEST(FixedSerializeTest, RoundTripsRawBits) {
  for (const fixed value : {fixed(0, 1), fixed(3, 4), fixed(255, 1),
                            fixed(65535, 256)}) {
    RecordingSink sink;
    ArchiveWriter writer(sink);
    fixed out = value;
    out.Serialize(writer);
    EXPECT_EQ(sink.bytes.size(), 2U);

    SpanSource straw(std::as_bytes(std::span(sink.bytes)));
    ArchiveReader reader(straw);
    fixed in;
    in.Serialize(reader);
    EXPECT_TRUE(reader.ok());
    EXPECT_EQ(in, value);
  }
}

}  // namespace
