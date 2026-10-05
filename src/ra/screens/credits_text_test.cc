// Tests for ParseCredits(): how the text of CREDITS.TXT is cut into strings
// and how each string's columns decide where it is drawn.

#include "ra/screens/credits_text.h"

#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "gtest/gtest.h"

namespace ra {

// Lets a failed expectation show the strings instead of their bytes.
// GoogleTest finds it by the argument's namespace.
static void PrintTo(const CreditString& string, std::ostream* os) {
  *os << '"' << string.text << "\" line " << string.line << " column "
      << string.column << " alignment " << static_cast<int>(string.alignment);
}

namespace {

CreditString Left(std::string_view text, int line, int column) {
  return {.text = text,
          .line = line,
          .column = column,
          .alignment = CreditAlignment::kLeft};
}

CreditString Right(std::string_view text, int line, int column) {
  return {.text = text,
          .line = line,
          .column = column,
          .alignment = CreditAlignment::kRight};
}

// A centred string is always anchored at the middle column.
CreditString Centered(std::string_view text, int line) {
  return {.text = text,
          .line = line,
          .column = kCreditColumns / 2,
          .alignment = CreditAlignment::kCenter};
}

using Strings = std::vector<CreditString>;

TEST(ParseCreditsTest, EmptyTextHasNoStrings) {
  EXPECT_EQ(ParseCredits(""), Strings{});
}

TEST(ParseCreditsTest, WhitespaceAloneHasNoStrings) {
  EXPECT_EQ(ParseCredits(" \t \r\n\r\n  "), Strings{});
}

TEST(ParseCreditsTest, AStringLeftOfTheMiddleEndsAtItsLastColumn) {
  EXPECT_EQ(ParseCredits("  Programmer\r\n"),
            (Strings{Right("Programmer", 0, 12)}));
}

TEST(ParseCreditsTest, AStringRightOfTheMiddleStartsAtItsFirstColumn) {
  // Five tabs reach column 40 and the spaces carry on from there.
  EXPECT_EQ(ParseCredits("\t\t\t\t\t  Steve Tall\r\n"),
            (Strings{Left("Steve Tall", 0, 42)}));
}

TEST(ParseCreditsTest, AStringAcrossTheMiddleIsCentered) {
  EXPECT_EQ(ParseCredits("\t\t\t\tCommand & Conquer\r\n"),
            (Strings{Centered("Command & Conquer", 0)}));
}

TEST(ParseCreditsTest, AStringThatOnlyReachesTheMiddleIsNotCentered) {
  // Columns 36 to 39: the string ends where the middle column begins.
  const std::string text = std::string(36, ' ') + "Lead\r\n";
  EXPECT_EQ(ParseCredits(text), (Strings{Right("Lead", 0, 40)}));
}

TEST(ParseCreditsTest, AStringOneColumnPastTheMiddleIsCentered) {
  const std::string text = std::string(36, ' ') + "Leads\r\n";
  EXPECT_EQ(ParseCredits(text), (Strings{Centered("Leads", 0)}));
}

TEST(ParseCreditsTest, AStringStartingOnTheMiddleColumnIsLeftJustified) {
  const std::string text = std::string(40, ' ') + "Name\r\n";
  EXPECT_EQ(ParseCredits(text), (Strings{Left("Name", 0, 40)}));
}

TEST(ParseCreditsTest, ATabSplitsALineIntoTwoColumns) {
  EXPECT_EQ(
      ParseCredits("\t\tProducer\t\tEd Del Castillo\r\n"),
      (Strings{Right("Producer", 0, 24), Left("Ed Del Castillo", 0, 40)}));
}

TEST(ParseCreditsTest, ATabAfterAStringAdvancesToTheNextStop) {
  // "Art" ends at column 3 and "Director" at 7 + 8; each tab goes on to the
  // next multiple of eight, even from a column just short of one.
  EXPECT_EQ(ParseCredits("Art\tDir\r\n1234567\tDirector\r\n"),
            (Strings{Right("Art", 0, 3), Right("Dir", 0, 11),
                     Right("1234567", 1, 7), Right("Director", 1, 16)}));
}

TEST(ParseCreditsTest, TwoSpacesSplitALine) {
  EXPECT_EQ(ParseCredits("Job  Name\r\n"),
            (Strings{Right("Job", 0, 3), Right("Name", 0, 9)}));
}

TEST(ParseCreditsTest, OneSpaceStaysInsideAString) {
  EXPECT_EQ(ParseCredits("Joseph B. Hewitt IV\r\n"),
            (Strings{Right("Joseph B. Hewitt IV", 0, 19)}));
}

TEST(ParseCreditsTest, EachCarriageReturnStartsALine) {
  EXPECT_EQ(ParseCredits("One\r\n\r\n\r\nFour\r\n"),
            (Strings{Right("One", 0, 3), Right("Four", 3, 4)}));
}

TEST(ParseCreditsTest, ALineFeedAloneDoesNotStartALine) {
  // Only the carriage return counts; the line feed ends the string and is
  // then skipped without moving the column.
  EXPECT_EQ(ParseCredits("One\nTwo\r\n"),
            (Strings{Right("One", 0, 3), Right("Two", 0, 6)}));
}

TEST(ParseCreditsTest, OneTrailingSpaceDoesNotAddALine) {
  EXPECT_EQ(ParseCredits("Map Designers: \r\nNext\r\n"),
            (Strings{Right("Map Designers:", 0, 14), Right("Next", 1, 4)}));
}

TEST(ParseCreditsTest, OneTrailingSpaceBeforeATabIsNotPartOfTheString) {
  EXPECT_EQ(ParseCredits("Job \tName\r\n"),
            (Strings{Right("Job", 0, 3), Right("Name", 0, 12)}));
}

TEST(ParseCreditsTest, ATrailingSpaceAtTheEndOfTheTextIsNotPartOfTheString) {
  EXPECT_EQ(ParseCredits("Job "), (Strings{Right("Job", 0, 3)}));
}

TEST(ParseCreditsTest, KeepsAStringThatRunsToTheEndOfTheText) {
  EXPECT_EQ(ParseCredits("Programmer\r\n\t\t\t\t\t  Steve Tall"),
            (Strings{Right("Programmer", 0, 10), Left("Steve Tall", 1, 42)}));
}

TEST(ParseCreditsTest, KeepsAStringEndedByTheLastByteOfTheText) {
  EXPECT_EQ(ParseCredits("Programmer\r"),
            (Strings{Right("Programmer", 0, 10)}));
}

TEST(ParseCreditsTest, TheStringsPointIntoTheText) {
  const std::string text = "Job\tName\r\n";
  const Strings strings = ParseCredits(text);
  ASSERT_EQ(strings.size(), 2U);
  EXPECT_EQ(strings.back().text.data(),
            std::string_view(text).substr(4).data());
}

}  // namespace

}  // namespace ra
