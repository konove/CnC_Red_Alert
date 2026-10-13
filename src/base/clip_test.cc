#include "base/clip.h"

#include "base/rect.h"
#include "gtest/gtest.h"

namespace base {
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
  EXPECT_EQ(ClipRect(Rect{10, 20, 5, 6}, kWidth, kHeight),
            (Rect{10, 20, 5, 6}));
}

TEST(ClipRectTest, RectanglePastAnEdgeLeavesNothing) {
  EXPECT_FALSE(ClipRect(Rect{100, 0, 10, 10}, kWidth, kHeight).IsValid());
  EXPECT_FALSE(ClipRect(Rect{-20, 0, 10, 10}, kWidth, kHeight).IsValid());
  EXPECT_FALSE(ClipRect(Rect{0, -20, 10, 10}, kWidth, kHeight).IsValid());
  EXPECT_FALSE(ClipRect(Rect{0, 100, 10, 10}, kWidth, kHeight).IsValid());
}

// The far corner is one past the last pixel, so a rectangle that ends on the
// window's left or top edge, or starts on its right or bottom one, passes the
// outcode test although no pixel of it is inside. Nothing left is always the
// default Rect, whichever way it came about.
TEST(ClipRectTest, RectangleOnlyTouchingAnEdgeLeavesNothing) {
  EXPECT_EQ(ClipRect(Rect{-10, 5, 10, 10}, kWidth, kHeight), Rect{});
  EXPECT_EQ(ClipRect(Rect{5, -10, 10, 10}, kWidth, kHeight), Rect{});
  EXPECT_EQ(ClipRect(Rect{kWidth, 5, 0, 10}, kWidth, kHeight), Rect{});
  EXPECT_EQ(ClipRect(Rect{5, kHeight, 10, 0}, kWidth, kHeight), Rect{});
}

TEST(ClipRectTest, EmptyRectangleLeavesNothing) {
  EXPECT_EQ(ClipRect(Rect{10, 10, 0, 5}, kWidth, kHeight), Rect{});
  EXPECT_EQ(ClipRect(Rect{10, 10, 5, 0}, kWidth, kHeight), Rect{});
  EXPECT_EQ(ClipRect(Rect{10, 10, -3, 5}, kWidth, kHeight), Rect{});
}

TEST(ClipRectTest, RectangleTouchingTheFarEdgeIsInside) {
  EXPECT_EQ(ClipRect(Rect{40, 40, 10, 10}, kWidth, kHeight),
            (Rect{40, 40, 10, 10}));
}

TEST(ClipRectTest, OverhangIsTrimmedFromTheFarEdges) {
  EXPECT_EQ(ClipRect(Rect{45, 45, 10, 20}, kWidth, kHeight),
            (Rect{45, 45, 5, 5}));
}

TEST(ClipRectTest, OverhangIsTrimmedFromTheNearEdges) {
  EXPECT_EQ(ClipRect(Rect{-5, -8, 10, 20}, kWidth, kHeight),
            (Rect{0, 0, 5, 12}));
}

TEST(ClipRectTest, RectangleLargerThanTheWindowBecomesTheWindow) {
  EXPECT_EQ(ClipRect(Rect{-10, -10, 100, 100}, kWidth, kHeight),
            (Rect{0, 0, kWidth, kHeight}));
}

// The map clips a 24 by 24 cell against the tactical view; a cell straddling
// the right edge keeps only the columns still on screen.
TEST(ClipRectTest, CellStraddlingTheTacticalEdge) {
  EXPECT_EQ(ClipRect(Rect{36, 12, 24, 24}, kWidth, kHeight),
            (Rect{36, 12, 14, 24}));
}

// Checks where ConfineRect() puts `rect` in the test window and whether it
// reports having bounded it.
void ExpectConfined(const Rect& rect, const int x, const int y,
                    const bool bounded) {
  const ConfinedRect confined = ConfineRect(rect, kWidth, kHeight);

  EXPECT_EQ(confined.position.x, x);
  EXPECT_EQ(confined.position.y, y);
  EXPECT_EQ(confined.bounded, bounded);
}

TEST(ConfineRectTest, RectangleInsideDoesNotMove) {
  ExpectConfined(Rect{10, 20, 5, 6}, 10, 20, false);
}

TEST(ConfineRectTest, RectangleTouchingTheFarEdgeDoesNotMove) {
  ExpectConfined(Rect{40, 40, 10, 10}, 40, 40, false);
}

// The rectangle keeps its size and slides back inside, unlike ClipRect(),
// which would have trimmed the overhang off instead.
TEST(ConfineRectTest, OverhangSlidesBackAndKeepsItsSize) {
  ExpectConfined(Rect{45, 48, 10, 20}, 40, 30, true);
}

TEST(ConfineRectTest, NegativeOriginMovesToZero) {
  ExpectConfined(Rect{-5, -8, 10, 20}, 0, 0, true);
}

// Each axis is bounded on its own.
TEST(ConfineRectTest, OneAxisCanBeBoundedAlone) {
  ExpectConfined(Rect{-5, 20, 10, 10}, 0, 20, true);
  ExpectConfined(Rect{10, 48, 10, 10}, 10, 40, true);
}

TEST(ConfineRectTest, RectangleLargerThanTheWindowPinsToTheOrigin) {
  ExpectConfined(Rect{30, 30, 100, 100}, 0, 0, true);
}

// A rectangle too big for the window reports as bounded even though it was
// already at the origin and does not move. Both games' map scrolling depends
// on that: it reads the result as "the scroll hit the edge of the world".
TEST(ConfineRectTest, OversizedRectangleAtTheOriginStillReportsBounded) {
  ExpectConfined(Rect{0, 0, 100, 100}, 0, 0, true);
}

}  // namespace
}  // namespace base
