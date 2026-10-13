// Tests for Rect.

#include "base/rect.h"

#include "gtest/gtest.h"

namespace {

TEST(RectTest, DefaultIsTheEmptyRectangle) {
  constexpr base::Rect rect;

  EXPECT_EQ(rect, (base::Rect{0, 0, 0, 0}));
  EXPECT_FALSE(rect.IsValid());
}

TEST(RectTest, IsValidNeedsAPositiveWidthAndHeight) {
  EXPECT_TRUE((base::Rect{-5, -5, 1, 1}).IsValid());
  EXPECT_FALSE((base::Rect{0, 0, 0, 4}).IsValid());
  EXPECT_FALSE((base::Rect{0, 0, 4, 0}).IsValid());
  EXPECT_FALSE((base::Rect{0, 0, -1, 4}).IsValid());
  EXPECT_FALSE((base::Rect{0, 0, 4, -1}).IsValid());
}

TEST(RectTest, RightAndBottomAreOnePastTheLastPixel) {
  constexpr base::Rect rect{2, 3, 10, 20};

  EXPECT_EQ(rect.right(), 12);
  EXPECT_EQ(rect.bottom(), 23);
}

TEST(RectTest, FromCornersIncludesBothCorners) {
  EXPECT_EQ(base::Rect::FromCorners(2, 3, 5, 7), (base::Rect{2, 3, 4, 5}));
  EXPECT_EQ(base::Rect::FromCorners(4, 4, 4, 4), (base::Rect{4, 4, 1, 1}));
}

TEST(RectTest, FromCornersTakesTheCornersInEitherOrder) {
  constexpr base::Rect expected{2, 3, 4, 5};

  EXPECT_EQ(base::Rect::FromCorners(5, 7, 2, 3), expected);
  EXPECT_EQ(base::Rect::FromCorners(5, 3, 2, 7), expected);
  EXPECT_EQ(base::Rect::FromCorners(2, 7, 5, 3), expected);
}

}  // namespace
