#include "base/clip.h"

#include "base/rect.h"
#include "gtest/gtest.h"

namespace base {
namespace {

// A 50 by 50 window, the shape the map code clips cell rectangles against.
constexpr int kWidth = 50;
constexpr int kHeight = 50;

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

// The window most of the tests below confine to.
constexpr Rect kWindow{0, 0, kWidth, kHeight};

TEST(ConfineRectTest, RectangleInsideDoesNotMove) {
  EXPECT_EQ(ConfineRect(Rect{10, 20, 5, 6}, kWindow),
            (ConfinedRect{.position = {10, 20}, .bounded = false}));
}

TEST(ConfineRectTest, RectangleTouchingTheFarEdgeDoesNotMove) {
  EXPECT_EQ(ConfineRect(Rect{40, 40, 10, 10}, kWindow),
            (ConfinedRect{.position = {40, 40}, .bounded = false}));
}

// The rectangle keeps its size and slides back inside, unlike ClipRect(),
// which would have trimmed the overhang off instead.
TEST(ConfineRectTest, OverhangSlidesBackAndKeepsItsSize) {
  EXPECT_EQ(ConfineRect(Rect{45, 48, 10, 20}, kWindow),
            (ConfinedRect{.position = {40, 30}, .bounded = true}));
}

TEST(ConfineRectTest, NegativeOriginMovesToZero) {
  EXPECT_EQ(ConfineRect(Rect{-5, -8, 10, 20}, kWindow),
            (ConfinedRect{.position = {0, 0}, .bounded = true}));
}

// Each axis is bounded on its own.
TEST(ConfineRectTest, OneAxisCanBeBoundedAlone) {
  EXPECT_EQ(ConfineRect(Rect{-5, 20, 10, 10}, kWindow),
            (ConfinedRect{.position = {0, 20}, .bounded = true}));
  EXPECT_EQ(ConfineRect(Rect{10, 48, 10, 10}, kWindow),
            (ConfinedRect{.position = {10, 40}, .bounded = true}));
}

TEST(ConfineRectTest, RectangleLargerThanTheWindowPinsToTheOrigin) {
  EXPECT_EQ(ConfineRect(Rect{30, 30, 100, 100}, kWindow),
            (ConfinedRect{.position = {0, 0}, .bounded = true}));
}

// Bounded although it does not move; see ConfineRect().
TEST(ConfineRectTest, OversizedRectangleAtTheOriginStillReportsBounded) {
  EXPECT_EQ(ConfineRect(Rect{0, 0, 100, 100}, kWindow),
            (ConfinedRect{.position = {0, 0}, .bounded = true}));
}

// The map's playable area starts some cells in from the corner of the world,
// so the window the games confine to is never at the origin.
constexpr Rect kOffsetBounds{100, 200, kWidth, kHeight};

TEST(ConfineRectTest, WindowAwayFromTheOriginLeavesARectangleInsideAlone) {
  EXPECT_EQ(ConfineRect(Rect{110, 220, 5, 6}, kOffsetBounds),
            (ConfinedRect{.position = {110, 220}, .bounded = false}));
}

TEST(ConfineRectTest, WindowAwayFromTheOriginBoundsAtItsNearEdges) {
  EXPECT_EQ(ConfineRect(Rect{95, 0, 10, 20}, kOffsetBounds),
            (ConfinedRect{.position = {100, 200}, .bounded = true}));
}

TEST(ConfineRectTest, WindowAwayFromTheOriginBoundsAtItsFarEdges) {
  EXPECT_EQ(ConfineRect(Rect{145, 248, 10, 20}, kOffsetBounds),
            (ConfinedRect{.position = {140, 230}, .bounded = true}));
}

// Pinned to the window's corner, not to (0, 0).
TEST(ConfineRectTest, WindowAwayFromTheOriginPinsAnOversizedRectangle) {
  EXPECT_EQ(ConfineRect(Rect{130, 230, 100, 100}, kOffsetBounds),
            (ConfinedRect{.position = {100, 200}, .bounded = true}));
}

// What a failed comparison shows, in place of the struct's bytes.
TEST(ConfinedRectTest, PrintsItsPositionAndWhetherItWasBounded) {
  EXPECT_EQ(
      testing::PrintToString(ConfinedRect{.position = {3, 4}, .bounded = true}),
      "{(3, 4), bounded}");
  EXPECT_EQ(testing::PrintToString(ConfinedRect{.position = {3, 4}}),
            "{(3, 4), not bounded}");
}

}  // namespace
}  // namespace base
