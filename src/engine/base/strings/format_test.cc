#include "engine/base/strings/format.h"

#include <cstdint>
#include <string>

#include "absl/strings/str_format.h"
#include "absl/types/span.h"
#include "gtest/gtest.h"

namespace {

enum class Owner { OWNER_NONE = -1, OWNER_GOOD = 0, OWNER_BAD = 1 };
using enum Owner;

TEST(FormatRuntimeTest, FormatsLikePrintf) {
  EXPECT_EQ(base::FormatRuntime("%s has %d units", "GDI", 12),
            "GDI has 12 units");
  EXPECT_EQ(base::FormatRuntime("%5d|%-5d|%08X|%c", 42, 42, 255, 'x'),
            "   42|42   |000000FF|x");
  EXPECT_EQ(base::FormatRuntime("%*d|%.*s", 4, 7, 2, "abcdef"), "   7|ab");
}

TEST(FormatRuntimeTest, AcceptsEnumsAndWideIntegers) {
  const int64_t frame = 1234567890123;
  EXPECT_EQ(base::FormatRuntime("%d %d %lu", OWNER_BAD, OWNER_NONE, frame),
            "1 -1 1234567890123");
  EXPECT_EQ(base::FormatRuntime("%s", std::string("owned")), "owned");
}

TEST(FormatRuntimeTest, CollapsesEscapedPercentWithoutArguments) {
  EXPECT_EQ(base::FormatRuntime("100%% done"), "100% done");
}

TEST(FormatRuntimeTest, ReturnsTextThatIsNotAFormatUnchanged) {
  EXPECT_EQ(base::FormatRuntime("100% done"), "100% done");
  EXPECT_EQ(base::FormatRuntime("Score: %d"), "Score: %d");
}

TEST(FormatRuntimeTest, ReturnsFormatUnchangedOnArgumentMismatch) {
  EXPECT_EQ(base::FormatRuntime("%d", "text"), "%d");
  EXPECT_EQ(base::FormatRuntime("%s", 5), "%s");
  EXPECT_EQ(base::FormatRuntime("%d and %d", 1), "%d and %d");
}

TEST(FormatRuntimeTest, IgnoresSurplusArguments) {
  EXPECT_EQ(base::FormatRuntime("just text", 1, 2), "just text");
}

TEST(FormatRuntimeTest, SpanOverloadTakesPackedArguments) {
  const auto packed = base::MakeFormatArgs("x", 3);
  EXPECT_EQ(base::FormatRuntime("%s=%d", absl::MakeConstSpan(packed)), "x=3");
  EXPECT_EQ(base::FormatRuntime("plain", absl::Span<const absl::FormatArg>()),
            "plain");
}

}  // namespace
