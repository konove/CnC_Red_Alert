// Tests for Rect.

#include "engine/gfx/rect.h"

#include "gtest/gtest.h"

namespace {

TEST(RectTest, DefaultIsTheEmptyRectangle) {
  constexpr Rect rect;

  EXPECT_EQ(rect, (Rect{0, 0, 0, 0}));
  EXPECT_FALSE(rect.IsValid());
}

TEST(RectTest, IsValidNeedsAPositiveWidthAndHeight) {
  EXPECT_TRUE((Rect{-5, -5, 1, 1}).IsValid());
  EXPECT_FALSE((Rect{0, 0, 0, 4}).IsValid());
  EXPECT_FALSE((Rect{0, 0, 4, 0}).IsValid());
  EXPECT_FALSE((Rect{0, 0, -1, 4}).IsValid());
  EXPECT_FALSE((Rect{0, 0, 4, -1}).IsValid());
}

TEST(RectTest, RightAndBottomAreOnePastTheLastPixel) {
  constexpr Rect rect{2, 3, 10, 20};

  EXPECT_EQ(rect.right(), 12);
  EXPECT_EQ(rect.bottom(), 23);
}

TEST(RectTest, FromCornersIncludesBothCorners) {
  EXPECT_EQ(Rect::FromCorners(2, 3, 5, 7), (Rect{2, 3, 4, 5}));
  EXPECT_EQ(Rect::FromCorners(4, 4, 4, 4), (Rect{4, 4, 1, 1}));
}

TEST(RectTest, FromCornersTakesTheCornersInEitherOrder) {
  constexpr Rect expected{2, 3, 4, 5};

  EXPECT_EQ(Rect::FromCorners(5, 7, 2, 3), expected);
  EXPECT_EQ(Rect::FromCorners(5, 3, 2, 7), expected);
  EXPECT_EQ(Rect::FromCorners(2, 7, 5, 3), expected);
}

}  // namespace
