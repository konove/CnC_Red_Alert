// Tests for Point.

#include "base/point.h"

#include "gtest/gtest.h"

namespace {

TEST(PointTest, DefaultIsTheOrigin) {
  constexpr base::Point point;

  EXPECT_EQ(point.x(), 0);
  EXPECT_EQ(point.y(), 0);
}

TEST(PointTest, KeepsTheCoordinatesItWasBuiltWith) {
  constexpr base::Point point{-3, 7};

  EXPECT_EQ(point.x(), -3);
  EXPECT_EQ(point.y(), 7);
}

TEST(PointTest, EqualWhenBothCoordinatesMatch) {
  EXPECT_EQ((base::Point{2, 3}), (base::Point{2, 3}));
  EXPECT_NE((base::Point{2, 3}), (base::Point{3, 3}));
  EXPECT_NE((base::Point{2, 3}), (base::Point{2, 4}));
}

TEST(PointTest, ChangesByAssigningANewOne) {
  base::Point point{1, 2};

  point = base::Point{point.x() + 10, point.y()};

  EXPECT_EQ(point, (base::Point{11, 2}));
}

}  // namespace
