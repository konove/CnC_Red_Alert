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
  EXPECT_TRUE(IsMissionCounterstrike("SCM25EA.INI"));
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

TEST(MissionWithNumberTest, ReplacesTheTwoDigitNumber) {
  EXPECT_EQ(MissionWithNumber("SCG05EA.INI", 6), "SCG06EA.INI");
  EXPECT_EQ(MissionWithNumber("SCU09EB.INI", 10), "SCU10EB.INI");
  EXPECT_EQ(MissionWithNumber("SCA01EA.INI", 2), "SCA02EA.INI");
}

TEST(MissionWithNumberTest, LeavesShortNamesAlone) {
  EXPECT_EQ(MissionWithNumber("", 6), "");
  EXPECT_EQ(MissionWithNumber("SCG0", 6), "SCG0");
}

TEST(MissionWithVariantTest, ReplacesTheVariantLetter) {
  EXPECT_EQ(MissionWithVariant("SCG05EA.INI", 'B'), "SCG05EB.INI");
  EXPECT_EQ(MissionWithVariant("SCU13EA.INI", 'C'), "SCU13EC.INI");
}

TEST(MissionWithVariantTest, LeavesShortNamesAlone) {
  EXPECT_EQ(MissionWithVariant("", 'B'), "");
  EXPECT_EQ(MissionWithVariant("SCG05E", 'B'), "SCG05E");
}

}  // namespace
