#include "ra/assets/shape_draw.h"

#include "gtest/gtest.h"

namespace ra {
namespace {

// Window widths are in pixels: the tactical window is 480 wide at 640x400.
constexpr int kWidth = 480;

TEST(PredatorOffsetTest, LeftHalfWalksWithTheFrame) {
  EXPECT_EQ(PredatorOffset(7, 0, kWidth), 7);
  EXPECT_EQ(PredatorOffset(7, kWidth / 2, kWidth), 7);
}

TEST(PredatorOffsetTest, RightHalfWalksTheOtherWay) {
  EXPECT_EQ(PredatorOffset(7, (kWidth / 2) + 1, kWidth), -7);
  EXPECT_EQ(PredatorOffset(7, kWidth - 1, kWidth), -7);
}

}  // namespace
}  // namespace ra
