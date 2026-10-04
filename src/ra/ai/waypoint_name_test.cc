#include "ra/ai/waypoint_name.h"

#include "gtest/gtest.h"

namespace ra {

namespace {

TEST(WaypointNameTest, TheFirstTwentySixWaypointsAreOneLetter) {
  EXPECT_EQ(WaypointLetters(0), "A");
  EXPECT_EQ(WaypointLetters(1), "B");
  EXPECT_EQ(WaypointLetters(25), "Z");
}

TEST(WaypointNameTest, LaterWaypointsAreTwoLetters) {
  EXPECT_EQ(WaypointLetters(26), "AA");
  EXPECT_EQ(WaypointLetters(27), "AB");
  EXPECT_EQ(WaypointLetters(51), "AZ");
  EXPECT_EQ(WaypointLetters(52), "BA");
  // The scenario's last waypoint, the special one.
  EXPECT_EQ(WaypointLetters(100), "CW");
  EXPECT_EQ(WaypointLetters(701), "ZZ");
}

// An action with no value holds -1, which the editor shows as the character
// before 'A'.
TEST(WaypointNameTest, NoWaypointIsTheCharacterBeforeA) {
  EXPECT_EQ(WaypointLetters(-1), "@");
}

TEST(WaypointNameTest, LettersNameTheWaypointInEitherCase) {
  EXPECT_EQ(WaypointFromLetters("A"), 0);
  EXPECT_EQ(WaypointFromLetters("z"), 25);
  EXPECT_EQ(WaypointFromLetters("AA"), 26);
  EXPECT_EQ(WaypointFromLetters("cw"), 100);
  EXPECT_EQ(WaypointFromLetters("Zz"), 701);
}

TEST(WaypointNameTest, OnlyTheFirstTwoLettersCount) {
  EXPECT_EQ(WaypointFromLetters("ABC"), 27);
}

TEST(WaypointNameTest, NoLettersNameNoWaypoint) {
  EXPECT_EQ(WaypointFromLetters(""), -1);
}

TEST(WaypointNameTest, EveryWaypointSurvivesTheRoundTrip) {
  for (int waypoint = 0; waypoint <= 701; ++waypoint) {
    EXPECT_EQ(WaypointFromLetters(WaypointLetters(waypoint)), waypoint);
  }
}

}  // namespace

}  // namespace ra
