// Tests strict numeric tokens, legacy whitespace, and storage-width limits.
#include "tech/number_parse.h"

#include <cstdint>
#include <limits>

#include "gtest/gtest.h"

namespace {

TEST(ParseIntegerTest, AcceptsDecimalSignsAndWhitespace) {
  EXPECT_EQ(tech::ParseInteger<int>(" \t+0042\r\n"), 42);
  EXPECT_EQ(tech::ParseInteger<int>("-42"), -42);
  EXPECT_EQ(tech::ParseInteger<int>("0"), 0);
  EXPECT_EQ(tech::ParseInteger<int>("2147483647"),
            std::numeric_limits<int>::max());
  EXPECT_EQ(tech::ParseInteger<int>("-2147483648"),
            std::numeric_limits<int>::min());
}

TEST(ParseIntegerTest, RejectsMissingMalformedAndOverflowingTokens) {
  for (const char* text :
       {static_cast<const char*>(nullptr), "", " ", "+", "--1", "1x", "1 2",
        "0x10", "2147483648", "-2147483649", "999999999999999999999999"}) {
    EXPECT_FALSE(tech::ParseInteger<int>(text).has_value())
        << (text ? text : "null");
  }
}

TEST(ParseIntegerTest, ChecksDestinationWidthAndPreservesPackedCoordinates) {
  EXPECT_EQ(tech::ParseInteger<uint32_t>("4294967295"), UINT32_MAX);
  EXPECT_FALSE(tech::ParseInteger<uint32_t>("4294967296").has_value());
  EXPECT_FALSE(tech::ParseInteger<uint32_t>("-1").has_value());
  EXPECT_EQ(tech::ParseInteger<uint16_t>("65535"), UINT16_MAX);
  EXPECT_FALSE(tech::ParseInteger<uint16_t>("65536").has_value());
  EXPECT_EQ(tech::ParseInteger<int>("bad").value_or(-1), -1);
}

TEST(ParseHexTest, AcceptsHexSyntaxAndChecksRange) {
  EXPECT_EQ(tech::ParseHex<uint32_t>(" \t0xFfFFffff\n"), UINT32_MAX);
  EXPECT_EQ(tech::ParseHex<int>("-A"), -10);
  EXPECT_EQ(tech::ParseHex<uint8_t>("ff"), 255);
  for (const char* text : {static_cast<const char*>(nullptr), "", "0x", "12z",
                           "ffh", "100000000", "-1"}) {
    EXPECT_FALSE(tech::ParseHex<uint32_t>(text).has_value())
        << (text ? text : "null");
  }
  EXPECT_FALSE(tech::ParseHex<uint8_t>("100").has_value());
}

TEST(ParseIniIntegerTest, PreservesDecimalAndHexForms) {
  EXPECT_EQ(tech::ParseIniInteger(" +010 "), 10);
  EXPECT_EQ(tech::ParseIniInteger("$ff"), 255);
  EXPECT_EQ(tech::ParseIniInteger("FFh"), 255);
  EXPECT_EQ(tech::ParseIniInteger("0x10H"), 16);
  EXPECT_EQ(tech::ParseIniInteger("$ffffffff"), -1);
  EXPECT_EQ(tech::ParseIniInteger("80000000h"),
            std::numeric_limits<int>::min());
}

TEST(ParseIniIntegerTest, RejectsMalformedAndOverflowingValues) {
  for (const char* text : {"", " ", "$", "h", "12junk", "$ffjunk", "12hmore",
                           "$100000000", "2147483648", "-2147483649"}) {
    EXPECT_FALSE(tech::ParseIniInteger(text).has_value()) << text;
  }
}

TEST(ParseDecimalBitsTest, AcceptsSignedAndUnsignedCoordinateSpellings) {
  EXPECT_EQ(tech::ParseDecimalBits("4294967295"), UINT32_MAX);
  EXPECT_EQ(tech::ParseDecimalBits("-1"), UINT32_MAX);
  EXPECT_EQ(tech::ParseDecimalBits("-2147483648"), 0x80000000U);
  EXPECT_FALSE(tech::ParseDecimalBits("4294967296").has_value());
  EXPECT_FALSE(tech::ParseDecimalBits("-2147483649").has_value());
  EXPECT_FALSE(tech::ParseDecimalBits("123x").has_value());
}

}  // namespace
