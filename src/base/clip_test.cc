#include "base/clip.h"

#include "gtest/gtest.h"

namespace {

// A 50 by 50 window, the shape the map code clips cell rectangles against.
constexpr int kWidth = 50;
constexpr int kHeight = 50;

TEST(OutCodeTest, InsideWindowIsEmpty) {
  EXPECT_EQ(OutCodeOf(0, 0, kWidth, kHeight), OutCode::kInside);
  EXPECT_EQ(OutCodeOf(49, 49, kWidth, kHeight), OutCode::kInside);
}

TEST(OutCodeTest, EachEdgeGetsItsOwnBit) {
  EXPECT_EQ(OutCodeOf(-1, 10, kWidth, kHeight), OutCode::kLeft);
  EXPECT_EQ(OutCodeOf(kWidth, 10, kWidth, kHeight), OutCode::kRight);
  EXPECT_EQ(OutCodeOf(10, -1, kWidth, kHeight), OutCode::kAbove);
  EXPECT_EQ(OutCodeOf(10, kHeight, kWidth, kHeight), OutCode::kBelow);
}

TEST(OutCodeTest, CornerSetsTwoBits) {
  EXPECT_EQ(OutCodeOf(-1, -1, kWidth, kHeight),
            OutCode::kLeft | OutCode::kAbove);
  EXPECT_EQ(OutCodeOf(kWidth, kHeight, kWidth, kHeight),
            OutCode::kRight | OutCode::kBelow);
}

// The widened window callers use for the corner just past the bottom right is
// what lets an end coordinate equal to the width count as inside.
TEST(OutCodeTest, WidenedWindowAdmitsTheEndCoordinate) {
  EXPECT_EQ(OutCodeOf(kWidth, kHeight, kWidth + 1, kHeight + 1),
            OutCode::kInside);
}

TEST(ClipRectTest, RectangleInsideIsUnchanged) {
  int x = 10;
  int y = 20;
  int width = 5;
  int height = 6;

  EXPECT_TRUE(ClipRect(x, y, width, height, kWidth, kHeight));
  EXPECT_EQ(x, 10);
  EXPECT_EQ(y, 20);
  EXPECT_EQ(width, 5);
  EXPECT_EQ(height, 6);
}

TEST(ClipRectTest, RectanglePastAnEdgeIsRejected) {
  int x = 100;
  int y = 0;
  int width = 10;
  int height = 10;
  EXPECT_FALSE(ClipRect(x, y, width, height, kWidth, kHeight));

  x = -20;
  EXPECT_FALSE(ClipRect(x, y, width, height, kWidth, kHeight));

  x = 0;
  y = -20;
  EXPECT_FALSE(ClipRect(x, y, width, height, kWidth, kHeight));

  y = 100;
  EXPECT_FALSE(ClipRect(x, y, width, height, kWidth, kHeight));
}

TEST(ClipRectTest, RectangleTouchingTheFarEdgeIsInside) {
  int x = 40;
  int y = 40;
  int width = 10;
  int height = 10;

  EXPECT_TRUE(ClipRect(x, y, width, height, kWidth, kHeight));
  EXPECT_EQ(x, 40);
  EXPECT_EQ(y, 40);
  EXPECT_EQ(width, 10);
  EXPECT_EQ(height, 10);
}

TEST(ClipRectTest, OverhangIsTrimmedFromTheFarEdges) {
  int x = 45;
  int y = 45;
  int width = 10;
  int height = 20;

  EXPECT_TRUE(ClipRect(x, y, width, height, kWidth, kHeight));
  EXPECT_EQ(x, 45);
  EXPECT_EQ(y, 45);
  EXPECT_EQ(width, 5);
  EXPECT_EQ(height, 5);
}

TEST(ClipRectTest, OverhangIsTrimmedFromTheNearEdges) {
  int x = -5;
  int y = -8;
  int width = 10;
  int height = 20;

  EXPECT_TRUE(ClipRect(x, y, width, height, kWidth, kHeight));
  EXPECT_EQ(x, 0);
  EXPECT_EQ(y, 0);
  EXPECT_EQ(width, 5);
  EXPECT_EQ(height, 12);
}

TEST(ClipRectTest, RectangleLargerThanTheWindowBecomesTheWindow) {
  int x = -10;
  int y = -10;
  int width = 100;
  int height = 100;

  EXPECT_TRUE(ClipRect(x, y, width, height, kWidth, kHeight));
  EXPECT_EQ(x, 0);
  EXPECT_EQ(y, 0);
  EXPECT_EQ(width, kWidth);
  EXPECT_EQ(height, kHeight);
}

// The map clips a 24 by 24 cell against the tactical view; a cell straddling
// the right edge keeps only the columns still on screen.
TEST(ClipRectTest, CellStraddlingTheTacticalEdge) {
  int x = 36;
  int y = 12;
  int width = 24;
  int height = 24;

  EXPECT_TRUE(ClipRect(x, y, width, height, kWidth, kHeight));
  EXPECT_EQ(x, 36);
  EXPECT_EQ(width, 14);
  EXPECT_EQ(y, 12);
  EXPECT_EQ(height, 24);
}

TEST(ConfineRectTest, RectangleInsideDoesNotMove) {
  int x = 10;
  int y = 20;

  EXPECT_FALSE(ConfineRect(x, y, 5, 6, kWidth, kHeight));
  EXPECT_EQ(x, 10);
  EXPECT_EQ(y, 20);
}

TEST(ConfineRectTest, RectangleTouchingTheFarEdgeDoesNotMove) {
  int x = 40;
  int y = 40;

  EXPECT_FALSE(ConfineRect(x, y, 10, 10, kWidth, kHeight));
  EXPECT_EQ(x, 40);
  EXPECT_EQ(y, 40);
}

// The rectangle keeps its size and slides back inside, unlike ClipRect(),
// which would have trimmed the overhang off instead.
TEST(ConfineRectTest, OverhangSlidesBackAndKeepsItsSize) {
  int x = 45;
  int y = 48;

  EXPECT_TRUE(ConfineRect(x, y, 10, 20, kWidth, kHeight));
  EXPECT_EQ(x, 40);
  EXPECT_EQ(y, 30);
}

TEST(ConfineRectTest, NegativeOriginMovesToZero) {
  int x = -5;
  int y = -8;

  EXPECT_TRUE(ConfineRect(x, y, 10, 20, kWidth, kHeight));
  EXPECT_EQ(x, 0);
  EXPECT_EQ(y, 0);
}

TEST(ConfineRectTest, RectangleLargerThanTheWindowPinsToTheOrigin) {
  int x = 30;
  int y = 30;

  EXPECT_TRUE(ConfineRect(x, y, 100, 100, kWidth, kHeight));
  EXPECT_EQ(x, 0);
  EXPECT_EQ(y, 0);
}

// A rectangle too big for the window reports as bounded even though it was
// already at the origin and does not move. Both games' map scrolling depends
// on that: it reads the result as "the scroll hit the edge of the world".
TEST(ConfineRectTest, OversizedRectangleAtTheOriginStillReportsBounded) {
  int x = 0;
  int y = 0;

  EXPECT_TRUE(ConfineRect(x, y, 100, 100, kWidth, kHeight));
  EXPECT_EQ(x, 0);
  EXPECT_EQ(y, 0);
}

}  // namespace
