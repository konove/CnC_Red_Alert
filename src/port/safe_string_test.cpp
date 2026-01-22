#include "port/safe_string.h"

#include <memory>

#include "gtest/gtest.h"

namespace port {
namespace {

// SafeCopy tests

TEST(SafeCopyTest, BasicCopy) {
  char dest[10];
  SafeCopy(dest, "hello", sizeof(dest));
  EXPECT_STREQ(dest, "hello");
}

TEST(SafeCopyTest, TruncatesLongString) {
  char dest[6];
  SafeCopy(dest, "hello world", sizeof(dest));
  EXPECT_STREQ(dest, "hello");
}

TEST(SafeCopyTest, ExactFit) {
  char dest[6];
  SafeCopy(dest, "hello", sizeof(dest));
  EXPECT_STREQ(dest, "hello");
}

TEST(SafeCopyTest, NullDestDoesNothing) {
  SafeCopy(nullptr, "hello", 10);  // Should not crash.
}

TEST(SafeCopyTest, ZeroSizeDoesNothing) {
  char dest[10] = "unchanged";
  SafeCopy(dest, "hello", 0);
  EXPECT_STREQ(dest, "unchanged");
}

TEST(SafeCopyTest, NullSrcSetsEmpty) {
  char dest[10] = "original";
  SafeCopy(dest, nullptr, sizeof(dest));
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
  SafeAppend(dest, " world", sizeof(dest));
  EXPECT_STREQ(dest, "hello world");
}

TEST(SafeAppendTest, TruncatesOnOverflow) {
  char dest[10] = "hello";
  SafeAppend(dest, " world", sizeof(dest));
  EXPECT_STREQ(dest, "hello wor");
}

TEST(SafeAppendTest, AppendToEmpty) {
  char dest[10] = "";
  SafeAppend(dest, "hello", sizeof(dest));
  EXPECT_STREQ(dest, "hello");
}

TEST(SafeAppendTest, NullDestDoesNothing) {
  SafeAppend(nullptr, "hello", 10);  // Should not crash.
}

TEST(SafeAppendTest, NullSrcDoesNothing) {
  char dest[10] = "hello";
  SafeAppend(dest, nullptr, sizeof(dest));
  EXPECT_STREQ(dest, "hello");
}

TEST(SafeAppendTest, ZeroSizeDoesNothing) {
  char dest[10] = "hello";
  SafeAppend(dest, " world", 0);
  EXPECT_STREQ(dest, "hello");
}

TEST(SafeAppendTest, TemplateOverload) {
  char dest[20] = "hello";
  SafeAppend(dest, " world");
  EXPECT_STREQ(dest, "hello world");
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
