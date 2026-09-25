#include "base/strings/tokenizer.h"

#include <cstring>

#include "base/strings/safe_string.h"
#include "gtest/gtest.h"

namespace {

TEST(TokenizerTest, SplitsFieldsAndTerminatesEachInPlace) {
  char text[] = "GDI,E1,256";
  base::Tokenizer tokens(text, ",");
  EXPECT_STREQ(tokens.Next(), "GDI");
  EXPECT_STREQ(tokens.Next(), "E1");
  EXPECT_STREQ(tokens.Next(), "256");
  EXPECT_EQ(tokens.Next(), nullptr);
  EXPECT_EQ(tokens.Next(), nullptr);
  EXPECT_STREQ(tokens.Remaining(), "");
}

TEST(TokenizerTest, SkipsRunsOfDelimitersLikeStrtok) {
  char text[] = ",,a,\r\n,b,,";
  base::Tokenizer tokens(text, ",\r\n");
  EXPECT_STREQ(tokens.Next(), "a");
  EXPECT_STREQ(tokens.Next(), "b");
  EXPECT_EQ(tokens.Next(), nullptr);
}

TEST(TokenizerTest, ReturnsNoTokenForEmptyOrAllDelimiterText) {
  char empty[] = "";
  EXPECT_EQ(base::Tokenizer(empty, ",").Next(), nullptr);
  char blanks[] = "   ";
  EXPECT_EQ(base::Tokenizer(blanks, " ").Next(), nullptr);
}

TEST(TokenizerTest, ChangesDelimitersPerCall) {
  char text[] = "2,E1:3,E2:4";
  base::Tokenizer tokens(text, ",");
  EXPECT_STREQ(tokens.Next(), "2");
  EXPECT_STREQ(tokens.Next(",:"), "E1");
  EXPECT_STREQ(tokens.Next(",:"), "3");
  EXPECT_STREQ(tokens.Next(",:"), "E2");
  EXPECT_STREQ(tokens.Next(), "4");
  EXPECT_EQ(tokens.Next(), nullptr);
}

TEST(TokenizerTest, RemainingStartsAfterTheLastTokensDelimiter) {
  char text[] = "005 hello world 7";
  base::Tokenizer tokens(text, " ");
  EXPECT_STREQ(tokens.Next(), "005");
  EXPECT_STREQ(tokens.Remaining(), "hello world 7");
  // A length-prefixed field is read from Remaining() and parsing resumes
  // after it with a fresh tokenizer.
  tokens = base::Tokenizer(
      base::MutableCString(tokens.Remaining()).subspan(5).data(), " ");
  EXPECT_STREQ(tokens.Next(), "world");
  EXPECT_STREQ(tokens.Remaining(), "7");
  EXPECT_STREQ(tokens.Next(), "7");
  EXPECT_STREQ(tokens.Remaining(), "");
}

TEST(TokenizerTest, TwoTokenizersDoNotInterfere) {
  char outer[] = "a,b";
  char inner[] = "x,y";
  base::Tokenizer outer_tokens(outer, ",");
  EXPECT_STREQ(outer_tokens.Next(), "a");
  base::Tokenizer inner_tokens(inner, ",");
  EXPECT_STREQ(inner_tokens.Next(), "x");
  EXPECT_STREQ(inner_tokens.Next(), "y");
  EXPECT_STREQ(outer_tokens.Next(), "b");
}

}  // namespace
