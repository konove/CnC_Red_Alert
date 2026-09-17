#include "port/profile_buffer.h"

#include <array>
#include <span>

#include "gtest/gtest.h"

namespace {

TEST(ProfileBufferTest, ReadsCaseInsensitiveTrimmedValuesWithEitherLineEnding) {
  for (const auto* const text : {"[Section]\r\nKey = value \t\r\n",
                         "[Section]\nKey = value \t\n",
                         "[Section]\nKey = value \t"}) {
    char output[16];
    ASSERT_TRUE(port::ReadProfile(text, "section", "KEY", "missing", output));
    EXPECT_STREQ(output, "value");
  }
}

TEST(ProfileBufferTest, ShortAndMalformedInputsDoNotOverread) {
  for (const auto* const text : {"", "[", "[S]", "[S]\n", "[S]\nK", "[S]\nOther=x"}) {
    char output[8];
    EXPECT_FALSE(port::ReadProfile(text, "S", "Key", "default", output));
    EXPECT_STREQ(output, "default");
  }
  char output[8];
  EXPECT_TRUE(port::ReadProfile("[S]\nKey=", "S", "Key", "default", output));
  EXPECT_STREQ(output, "default");
}

TEST(ProfileBufferTest, DefaultMayAliasOutputAndSmallOutputsKeepTheirBounds) {
  char output[] = "default";
  EXPECT_FALSE(port::ReadProfile("", "S", "K", output, output));
  EXPECT_STREQ(output, "default");
  std::array<char, 3> guarded{'L', 'x', 'R'};
  port::ReadProfile("[S]\nK=value", "S", "K", nullptr,
                    std::span(guarded).subspan(1, 1));
  EXPECT_EQ(guarded.at(0), 'L');
  EXPECT_EQ(guarded.at(1), '\0');
  EXPECT_EQ(guarded.at(2), 'R');
  EXPECT_TRUE(port::ReadProfile("[S]\nK=value", "S", "K", nullptr, {}));
}

TEST(ProfileBufferTest, EnumeratesWholeKeysWithDoubleNullTermination) {
  char output[16];
  port::ReadProfile("[S]\r\nA=1\r\nB=2\r\n[T]\r\nC=3\r\n", "S",
                    nullptr, nullptr, output);
  EXPECT_STREQ(output, "A");
  EXPECT_STREQ(std::span(output).subspan(2).data(), "B");
  EXPECT_EQ(output[4], '\0');
  char short_output[3];
  port::ReadProfile("[S]\nA=1\nB=2", "S", nullptr, nullptr, short_output);
  EXPECT_STREQ(short_output, "A");
  EXPECT_EQ(short_output[2], '\0');
}

TEST(ProfileBufferTest, UpdatesAndDeletesWithoutChangingOtherSections) {
  char text[128] = "[S]\r\nA=1\r\nB=2\r\n[T]\r\nC=3\r\n";
  ASSERT_TRUE(port::WriteProfile(text, "s", "a", "new"));
  EXPECT_STREQ(text, "[S]\r\na=new\r\nB=2\r\n[T]\r\nC=3\r\n");
  ASSERT_TRUE(port::WriteProfile(text, "S", "B", nullptr));
  EXPECT_STREQ(text, "[S]\r\na=new\r\n[T]\r\nC=3\r\n");
  ASSERT_TRUE(port::WriteProfile(text, "S", nullptr, nullptr));
  EXPECT_STREQ(text, "[T]\r\nC=3\r\n");
  ASSERT_TRUE(port::WriteProfile(text, "Missing", "K", nullptr));
  EXPECT_STREQ(text, "[T]\r\nC=3\r\n");
}

TEST(ProfileBufferTest, CapacityFailureIsAtomicAndRejectsUnterminatedStorage) {
  char text[] = "[S]\r\nA=1\r\n";
  EXPECT_FALSE(port::WriteProfile(text, "S", "A", "a much longer value"));
  EXPECT_STREQ(text, "[S]\r\nA=1\r\n");
  char unterminated[] = {'x', 'y'};
  EXPECT_FALSE(port::WriteProfile(unterminated, "S", "A", "1"));
  EXPECT_EQ(unterminated[0], 'x');
  EXPECT_EQ(unterminated[1], 'y');
}

TEST(ProfileBufferTest, AppendsSectionAndAcceptsAliasedValue) {
  char text[64] = "";
  ASSERT_TRUE(port::WriteProfile(text, "S", "A", "value"));
  EXPECT_STREQ(text, "\r\n[S]\r\nA=value\r\n");
  char aliased[64] = "value";
  ASSERT_TRUE(port::WriteProfile(aliased, "S", "A", aliased));
  char output[16];
  port::ReadProfile(aliased, "S", "A", nullptr, output);
  EXPECT_STREQ(output, "value");
}

TEST(ProfileBufferTest, InsertsIntoSectionWithoutTrailingNewline) {
  char text[32] = "[S]";
  ASSERT_TRUE(port::WriteProfile(text, "S", "A", "1"));
  EXPECT_STREQ(text, "[S]\r\nA=1\r\n");
}

}  // namespace
