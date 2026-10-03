#include "base/strings/tokenizer.h"

#include <string>
#include <string_view>

#include "gtest/gtest.h"

namespace {

TEST(TokenizerTest, SplitsFieldsAndLeavesTheTextUntouched) {
  const std::string text = "GDI,E1,256";
  base::Tokenizer tokens(text, ",");
  EXPECT_EQ(tokens.Next(), "GDI");
  EXPECT_EQ(tokens.Next(), "E1");
  EXPECT_EQ(tokens.Next(), "256");
  EXPECT_TRUE(tokens.Next().empty());
  EXPECT_TRUE(tokens.Next().empty());
  EXPECT_EQ(tokens.Remaining(), "");
  EXPECT_EQ(text, "GDI,E1,256");
}

TEST(TokenizerTest, TokensAreViewsIntoTheText) {
  const std::string_view text = "a,bc";
  base::Tokenizer tokens(text, ",");
  tokens.Next();
  const std::string_view token = tokens.Next();
  EXPECT_EQ(token.data(), text.substr(2).data());
  EXPECT_EQ(token.size(), 2U);
}

TEST(TokenizerTest, SkipsRunsOfDelimitersLikeStrtok) {
  base::Tokenizer tokens(",,a,\r\n,b,,", ",\r\n");
  EXPECT_EQ(tokens.Next(), "a");
  EXPECT_EQ(tokens.Next(), "b");
  EXPECT_TRUE(tokens.Next().empty());
}

TEST(TokenizerTest, ReturnsNoTokenForEmptyOrAllDelimiterText) {
  EXPECT_TRUE(base::Tokenizer("", ",").Next().empty());
  EXPECT_TRUE(base::Tokenizer("   ", " ").Next().empty());
  EXPECT_TRUE(base::Tokenizer(std::string_view(), ",").Next().empty());
}

TEST(TokenizerTest, ChangesDelimitersPerCall) {
  base::Tokenizer tokens("2,E1:3,E2:4", ",");
  EXPECT_EQ(tokens.Next(), "2");
  EXPECT_EQ(tokens.Next(",:"), "E1");
  EXPECT_EQ(tokens.Next(",:"), "3");
  EXPECT_EQ(tokens.Next(",:"), "E2");
  EXPECT_EQ(tokens.Next(), "4");
  EXPECT_TRUE(tokens.Next().empty());
}

TEST(TokenizerTest, RemainingStartsAfterTheLastTokensDelimiter) {
  base::Tokenizer tokens("005 hello world 7", " ");
  EXPECT_EQ(tokens.Next(), "005");
  EXPECT_EQ(tokens.Remaining(), "hello world 7");
  // A length-prefixed field is read from Remaining() and parsing resumes
  // after it with a fresh tokenizer.
  tokens = base::Tokenizer(tokens.Remaining().substr(5), " ");
  EXPECT_EQ(tokens.Next(), "world");
  EXPECT_EQ(tokens.Remaining(), "7");
  EXPECT_EQ(tokens.Next(), "7");
  EXPECT_EQ(tokens.Remaining(), "");
}

TEST(TokenizerTest, RemainingKeepsDelimitersAfterTheOneThatEndedTheToken) {
  base::Tokenizer tokens("a,,b", ",");
  EXPECT_EQ(tokens.Next(), "a");
  EXPECT_EQ(tokens.Remaining(), ",b");
}

TEST(TokenizerTest, TwoTokenizersDoNotInterfere) {
  base::Tokenizer outer_tokens("a,b", ",");
  EXPECT_EQ(outer_tokens.Next(), "a");
  base::Tokenizer inner_tokens("x,y", ",");
  EXPECT_EQ(inner_tokens.Next(), "x");
  EXPECT_EQ(inner_tokens.Next(), "y");
  EXPECT_EQ(outer_tokens.Next(), "b");
}

}  // namespace
