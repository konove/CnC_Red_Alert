#include "ra/mission_id.h"

#include <string>

#include "gtest/gtest.h"

namespace {

TEST(IsMissionCounterstrikeTest, TwoDigitNumberAbove24) {
  EXPECT_TRUE(IsMissionCounterstrike("SCM25EA.INI"));
  EXPECT_TRUE(IsMissionCounterstrike("SCM99EA.INI"));
}

TEST(IsMissionCounterstrikeTest, TwoDigitNumberAtOrBelow24) {
  EXPECT_FALSE(IsMissionCounterstrike("SCM24EA.INI"));
  EXPECT_FALSE(IsMissionCounterstrike("SCM01EA.INI"));
}

TEST(IsMissionCounterstrikeTest, ThreeDigitNumber) {
  EXPECT_TRUE(IsMissionCounterstrike("SCM100.INI"));
  EXPECT_TRUE(IsMissionCounterstrike("SCM025EA.INI"));
  EXPECT_FALSE(IsMissionCounterstrike("SCM012.INI"));
}

TEST(IsMissionCounterstrikeTest, PrefixIsCaseSensitive) {
  // Legacy behavior: the sscanf-based parser only matched uppercase "SCM".
  EXPECT_FALSE(IsMissionCounterstrike("scm25ea.ini"));
}

TEST(IsMissionCounterstrikeTest, NonMatchingNames) {
  EXPECT_FALSE(IsMissionCounterstrike(""));
  EXPECT_FALSE(IsMissionCounterstrike("SCM"));
  EXPECT_FALSE(IsMissionCounterstrike("SCMJGEA.INI"));
  EXPECT_FALSE(IsMissionCounterstrike("SCG01EA.INI"));
  EXPECT_FALSE(IsMissionCounterstrike("XYZ25.INI"));
}

TEST(IsMissionCounterstrikeTest, AcceptsStdString) {
  EXPECT_TRUE(IsMissionCounterstrike(std::string("SCM25EA.INI")));
}

TEST(IsMissionAftermathTest, AlphabeticalName) {
  EXPECT_TRUE(IsMissionAftermath("SCMJGEA.INI"));
  EXPECT_TRUE(IsMissionAftermath("scmjgea.ini"));
}

TEST(IsMissionAftermathTest, TwoDigitsThenNonDigit) {
  EXPECT_TRUE(IsMissionAftermath("SCM25EA.INI"));
  EXPECT_TRUE(IsMissionAftermath("scm01ea.ini"));
}

TEST(IsMissionAftermathTest, NonMatchingNames) {
  EXPECT_FALSE(IsMissionAftermath(""));
  EXPECT_FALSE(IsMissionAftermath("SCM"));
  EXPECT_FALSE(IsMissionAftermath("SCM123.INI"));  // Three digits.
  EXPECT_FALSE(IsMissionAftermath("SCM12"));       // Nothing after digits.
  EXPECT_FALSE(IsMissionAftermath("SCM1A.INI"));   // Single digit.
  EXPECT_FALSE(IsMissionAftermath("SCG01EA.INI"));
}

}  // namespace
