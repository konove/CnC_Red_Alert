#include "port/format.h"

#include <cstdint>
#include <string>

#include "absl/strings/str_format.h"
#include "absl/types/span.h"
#include "gtest/gtest.h"

namespace {

enum Owner { OWNER_NONE = -1, OWNER_GOOD = 0, OWNER_BAD = 1 };

TEST(FormatRuntimeTest, FormatsLikePrintf) {
  EXPECT_EQ(port::FormatRuntime("%s has %d units", "GDI", 12),
            "GDI has 12 units");
  EXPECT_EQ(port::FormatRuntime("%5d|%-5d|%08X|%c", 42, 42, 255, 'x'),
            "   42|42   |000000FF|x");
  EXPECT_EQ(port::FormatRuntime("%*d|%.*s", 4, 7, 2, "abcdef"), "   7|ab");
}

TEST(FormatRuntimeTest, AcceptsEnumsAndWideIntegers) {
  const int64_t frame = 1234567890123;
  EXPECT_EQ(port::FormatRuntime("%d %d %lu", OWNER_BAD, OWNER_NONE, frame),
            "1 -1 1234567890123");
  EXPECT_EQ(port::FormatRuntime("%s", std::string("owned")), "owned");
}

TEST(FormatRuntimeTest, CollapsesEscapedPercentWithoutArguments) {
  EXPECT_EQ(port::FormatRuntime("100%% done"), "100% done");
}

TEST(FormatRuntimeTest, ReturnsTextThatIsNotAFormatUnchanged) {
  EXPECT_EQ(port::FormatRuntime("100% done"), "100% done");
  EXPECT_EQ(port::FormatRuntime("Score: %d"), "Score: %d");
}

TEST(FormatRuntimeTest, ReturnsFormatUnchangedOnArgumentMismatch) {
  EXPECT_EQ(port::FormatRuntime("%d", "text"), "%d");
  EXPECT_EQ(port::FormatRuntime("%s", 5), "%s");
  EXPECT_EQ(port::FormatRuntime("%d and %d", 1), "%d and %d");
}

TEST(FormatRuntimeTest, IgnoresSurplusArguments) {
  EXPECT_EQ(port::FormatRuntime("just text", 1, 2), "just text");
}

TEST(FormatRuntimeTest, SpanOverloadTakesPackedArguments) {
  const auto packed = port::MakeFormatArgs("x", 3);
  EXPECT_EQ(port::FormatRuntime("%s=%d", absl::MakeConstSpan(packed)), "x=3");
  EXPECT_EQ(port::FormatRuntime("plain", absl::Span<const absl::FormatArg>()),
            "plain");
}

}  // namespace
