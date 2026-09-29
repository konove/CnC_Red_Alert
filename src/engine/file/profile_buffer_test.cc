#include "engine/file/profile_buffer.h"

#include <array>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "gtest/gtest.h"

namespace {

TEST(ProfileBufferTest, ReadsCaseInsensitiveTrimmedValuesWithEitherLineEnding) {
  for (const auto* const text : {"[Section]\r\nKey = value \t\r\n",
                         "[Section]\nKey = value \t\n",
                         "[Section]\nKey = value \t"}) {
    char output[16];
    ASSERT_TRUE(
        engine::file::ReadProfile(text, "section", "KEY", "missing", output));
    EXPECT_STREQ(output, "value");
  }
}

TEST(ProfileBufferTest, ShortAndMalformedInputsDoNotOverread) {
  for (const auto* const text : {"", "[", "[S]", "[S]\n", "[S]\nK", "[S]\nOther=x"}) {
    char output[8];
    EXPECT_FALSE(
        engine::file::ReadProfile(text, "S", "Key", "default", output));
    EXPECT_STREQ(output, "default");
  }
  char output[8];
  EXPECT_TRUE(
      engine::file::ReadProfile("[S]\nKey=", "S", "Key", "default", output));
  EXPECT_STREQ(output, "default");
}

TEST(ProfileBufferTest, DefaultMayAliasOutputAndSmallOutputsKeepTheirBounds) {
  char output[] = "default";
  EXPECT_FALSE(engine::file::ReadProfile("", "S", "K", output, output));
  EXPECT_STREQ(output, "default");
  std::array<char, 3> guarded{'L', 'x', 'R'};
  engine::file::ReadProfile("[S]\nK=value", "S", "K", nullptr,
                            std::span(guarded).subspan(1, 1));
  EXPECT_EQ(guarded.at(0), 'L');
  EXPECT_EQ(guarded.at(1), '\0');
  EXPECT_EQ(guarded.at(2), 'R');
  EXPECT_TRUE(engine::file::ReadProfile("[S]\nK=value", "S", "K", nullptr, {}));
}

TEST(ProfileBufferTest, EnumeratesWholeKeysWithDoubleNullTermination) {
  char output[16];
  engine::file::ReadProfile("[S]\r\nA=1\r\nB=2\r\n[T]\r\nC=3\r\n", "S", nullptr,
                            nullptr, output);
  EXPECT_STREQ(output, "A");
  EXPECT_STREQ(std::span(output).subspan(2).data(), "B");
  EXPECT_EQ(output[4], '\0');
  char short_output[3];
  engine::file::ReadProfile("[S]\nA=1\nB=2", "S", nullptr, nullptr,
                            short_output);
  EXPECT_STREQ(short_output, "A");
  EXPECT_EQ(short_output[2], '\0');
}

TEST(ProfileBufferTest, UpdatesAndDeletesWithoutChangingOtherSections) {
  std::string text = "[S]\r\nA=1\r\nB=2\r\n[T]\r\nC=3\r\n";
  engine::file::WriteProfile(text, "s", "a", "new");
  EXPECT_EQ(text, "[S]\r\na=new\r\nB=2\r\n[T]\r\nC=3\r\n");
  engine::file::WriteProfile(text, "S", "B", nullptr);
  EXPECT_EQ(text, "[S]\r\na=new\r\n[T]\r\nC=3\r\n");
  engine::file::WriteProfile(text, "S", nullptr, nullptr);
  EXPECT_EQ(text, "[T]\r\nC=3\r\n");
  engine::file::WriteProfile(text, "Missing", "K", nullptr);
  EXPECT_EQ(text, "[T]\r\nC=3\r\n");
}

TEST(ProfileBufferTest, GrowsWithoutACapacity) {
  std::string text = "[S]\r\nA=1\r\n";
  const std::string long_value(100000, 'v');
  engine::file::WriteProfile(text, "S", "A", long_value.c_str());
  EXPECT_EQ(text, "[S]\r\nA=" + long_value + "\r\n");
}

TEST(ProfileBufferTest, AppendsSectionAndAcceptsAliasedValue) {
  std::string text;
  engine::file::WriteProfile(text, "S", "A", "value");
  EXPECT_EQ(text, "\r\n[S]\r\nA=value\r\n");
  std::string aliased = "value";
  engine::file::WriteProfile(aliased, "S", "A", aliased.c_str());
  char output[16];
  engine::file::ReadProfile(aliased, "S", "A", nullptr, output);
  EXPECT_STREQ(output, "value");
}

TEST(ProfileBufferTest, InsertsIntoSectionWithoutTrailingNewline) {
  std::string text = "[S]";
  engine::file::WriteProfile(text, "S", "A", "1");
  EXPECT_EQ(text, "[S]\r\nA=1\r\n");
}

// A loose file in the current directory, where writes go and reads look first.
constexpr const char* kFileName = "PROFILE_BUFFER_TEST.INI";

class ProfileFileTest : public testing::Test {
 protected:
  void TearDown() override { std::filesystem::remove(kFileName); }
};

TEST_F(ProfileFileTest, ReadOfMissingFileIsNullopt) {
  EXPECT_EQ(engine::file::ReadProfileFile(kFileName), std::nullopt);
}

TEST_F(ProfileFileTest, WritesAndReadsBackTheWholeText) {
  const std::string text = "[S]\r\nA=" + std::string(100000, 'v') + "\r\n";
  ASSERT_TRUE(engine::file::WriteProfileFile(kFileName, text));
  EXPECT_EQ(engine::file::ReadProfileFile(kFileName), text);
}

TEST_F(ProfileFileTest, ReadStopsAtNul) {
  constexpr std::string_view kText("[S]\r\nA=1\r\n\0stale", 16);
  ASSERT_TRUE(engine::file::WriteProfileFile(kFileName, kText));
  EXPECT_EQ(std::filesystem::file_size(kFileName), kText.size());
  EXPECT_EQ(engine::file::ReadProfileFile(kFileName), "[S]\r\nA=1\r\n");
}

}  // namespace
