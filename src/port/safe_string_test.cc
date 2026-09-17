#include "port/safe_string.h"

#include <span>
#include <string_view>

#include "base/array.h"
#include "gtest/gtest.h"

namespace port {
namespace {

// SafeCopy tests

TEST(SafeCopyTest, BasicCopy) {
  char dest[10];
  SafeCopy(dest, "hello");
  EXPECT_STREQ(dest, "hello");
}

TEST(SafeCopyTest, TruncatesLongString) {
  char dest[6];
  SafeCopy(dest, "hello world");
  EXPECT_STREQ(dest, "hello");
}

TEST(SafeCopyTest, ExactFit) {
  char dest[6];
  SafeCopy(dest, "hello");
  EXPECT_STREQ(dest, "hello");
}

TEST(SafeCopyTest, NullDestDoesNothing) {
  SafeCopy({}, "hello");  // Should not crash.
}

TEST(SafeCopyTest, ZeroSizeDoesNothing) {
  char dest[10] = "unchanged";
  SafeCopy(std::span(dest).first(0), "hello");
  EXPECT_STREQ(dest, "unchanged");
}

TEST(SafeCopyTest, NullSrcSetsEmpty) {
  char dest[10] = "original";
  SafeCopy(dest, nullptr);
  EXPECT_STREQ(dest, "");
}

TEST(SafeCopyTest, TemplateOverload) {
  char dest[10];
  SafeCopy(dest, "test");
  EXPECT_STREQ(dest, "test");
}

// SafeAppend tests

TEST(SafeAppendTest, BasicAppend) {
  char dest[20] = "hello";
  SafeAppend(dest, " world");
  EXPECT_STREQ(dest, "hello world");
}

TEST(SafeAppendTest, TruncatesOnOverflow) {
  char dest[10] = "hello";
  SafeAppend(dest, " world");
  EXPECT_STREQ(dest, "hello wor");
}

TEST(SafeAppendTest, AppendToEmpty) {
  char dest[10] = "";
  SafeAppend(dest, "hello");
  EXPECT_STREQ(dest, "hello");
}

TEST(SafeAppendTest, NullDestDoesNothing) {
  SafeAppend({}, "hello");  // Should not crash.
}

TEST(SafeAppendTest, NullSrcDoesNothing) {
  char dest[10] = "hello";
  SafeAppend(dest, nullptr);
  EXPECT_STREQ(dest, "hello");
}

TEST(SafeAppendTest, ZeroSizeDoesNothing) {
  char dest[10] = "hello";
  SafeAppend(std::span(dest).first(0), " world");
  EXPECT_STREQ(dest, "hello");
}

TEST(SafeAppendTest, TemplateOverload) {
  char dest[20] = "hello";
  SafeAppend(dest, " world");
  EXPECT_STREQ(dest, "hello world");
}

TEST(SafeCopyTest, PadsUnusedCapacity) {
  char dest[6] = {'x', 'x', 'x', 'x', 'x', 'x'};
  SafeCopy(dest, "a");
  EXPECT_EQ(dest[1], '\0');
  EXPECT_EQ(dest[5], '\0');
}

TEST(SafeAppendTest, TerminatesInitiallyUnterminatedBuffer) {
  char dest[3] = {'a', 'b', 'c'};
  SafeAppend(dest, "d");
  EXPECT_STREQ(dest, "ab");
}

TEST(MutableCStringTest, IncludesTerminatorAndHandlesNull) {
  char text[] = "abc";
  const auto view = MutableCString(text);
  EXPECT_EQ(view.size(), 4U);
  EXPECT_EQ(view.back(), '\0');
  base::At(view, 1) = 'X';
  EXPECT_STREQ(text, "aXc");
  EXPECT_TRUE(MutableCString(nullptr).empty());
}

TEST(SafeCopyTest, CopiesAndAppendsBoundedUnterminatedInput) {
  const char input[] = {'a', 'b', 'c'};
  const std::string_view source{std::span(input)};
  char dest[8];
  SafeCopy(dest, source);
  SafeAppend(dest, source.substr(1));
  EXPECT_STREQ(dest, "abcbc");
}

// CloneString tests

TEST(CloneStringTest, BasicClone) {
  const char* original = "test string";
  char* clone = CloneString(original);
  ASSERT_NE(clone, nullptr);
  EXPECT_STREQ(clone, original);
  EXPECT_NE(clone, original);  // Must be different pointer.
  delete[] clone;
}

TEST(CloneStringTest, NullReturnsNull) {
  EXPECT_EQ(CloneString(nullptr), nullptr);
}

TEST(CloneStringTest, EmptyString) {
  char* clone = CloneString("");
  ASSERT_NE(clone, nullptr);
  EXPECT_STREQ(clone, "");
  delete[] clone;
}

}  // namespace
}  // namespace port
