#include "ra/screens/credits_text.h"

#include <vector>

#include "gtest/gtest.h"

namespace ra {

namespace {

TEST(ParseCreditsTest, KeepsAStringThatRunsToTheEndOfTheText) {
  const std::vector<CreditString> expected = {
      {.text = "Programmer",
       .line = 0,
       .column = 10,
       .alignment = CreditAlignment::kRight},
      {.text = "Steve Tall",
       .line = 1,
       .column = 42,
       .alignment = CreditAlignment::kLeft},
  };
  EXPECT_EQ(ParseCredits("Programmer\r\n\t\t\t\t\t  Steve Tall"), expected);
}

TEST(ParseCreditsTest, KeepsAStringEndedByTheLastByteOfTheText) {
  const std::vector<CreditString> expected = {
      {.text = "Programmer",
       .line = 0,
       .column = 10,
       .alignment = CreditAlignment::kRight},
  };
  EXPECT_EQ(ParseCredits("Programmer\r"), expected);
}

TEST(ParseCreditsTest, OneTrailingSpaceDoesNotAddALine) {
  const std::vector<CreditString> expected = {
      {.text = "Map Designers:",
       .line = 0,
       .column = 14,
       .alignment = CreditAlignment::kRight},
      {.text = "Next",
       .line = 1,
       .column = 4,
       .alignment = CreditAlignment::kRight},
  };
  EXPECT_EQ(ParseCredits("Map Designers: \r\nNext\r\n"), expected);
}

TEST(ParseCreditsTest, OneTrailingSpaceBeforeATabIsNotPartOfTheString) {
  const std::vector<CreditString> expected = {
      {.text = "Job",
       .line = 0,
       .column = 3,
       .alignment = CreditAlignment::kRight},
      {.text = "Name",
       .line = 0,
       .column = 12,
       .alignment = CreditAlignment::kRight},
  };
  EXPECT_EQ(ParseCredits("Job \tName\r\n"), expected);
}

}  // namespace

}  // namespace ra
