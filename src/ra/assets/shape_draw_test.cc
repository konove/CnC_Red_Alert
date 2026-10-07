#include "ra/assets/shape_draw.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#include "game/sim/direction.h"
#include "gmock/gmock.h"
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

// An 8 x 6 shape whose pixels are all different letters, so a picture of the
// result shows which source pixel every destination pixel took.
constexpr int kShapeWidth = 8;
constexpr int kShapeHeight = 6;
constexpr std::string_view kShape =
    "ABCDEFGH"
    "IJKLMNOP"
    "QRSTUVWX"
    "YZabcdef"
    "ghijklmn"
    "opqrstuv";

// Returns the rows of a `width` wide image, with '.' for transparent pixels.
std::vector<std::string> Rows(const std::vector<uint8_t>& image,
                              const int width) {
  std::vector<std::string> rows;
  std::string row;
  for (const uint8_t pixel : image) {
    row.push_back(pixel == 0 ? '.' : static_cast<char>(pixel));
    if (std::ssize(row) == width) {
      rows.push_back(row);
      row.clear();
    }
  }
  return rows;
}

// Returns the picture of kShape turned to `rotation` at `scale`.
std::vector<std::string> Draw(const game::DirType rotation,
                              const int32_t scale) {
  const std::vector<uint8_t> pixels(kShape.begin(), kShape.end());
  return Rows(
      ScaleRotateShape(pixels, kShapeWidth, kShapeHeight, rotation, scale),
      kShapeWidth * 2);
}

TEST(ScaleRotateShapeTest, ZeroScaleDrawsNothing) {
  EXPECT_THAT(Draw(game::DirType::kNorth, 0),
              testing::Each(std::string(size_t{kShapeWidth} * 2, '.')));
}

TEST(ScaleRotateShapeTest, UnturnedShapeIsCentredInTheDoubledFrame) {
  EXPECT_THAT(Draw(game::DirType::kNorth, 0x100),
              testing::ElementsAre("................",  //
                                   "................",  //
                                   "................",  //
                                   "....ABCDEFGH....",  //
                                   "....IJKLMNOP....",  //
                                   "....QRSTUVWX....",  //
                                   "....YZabcdef....",  //
                                   "....ghijklmn....",  //
                                   "....opqrstuv....",  //
                                   "................",  //
                                   "................",  //
                                   "................"));
}

TEST(ScaleRotateShapeTest, HalfTurnReversesRowsAndColumns) {
  EXPECT_THAT(Draw(game::DirType::kSouth, 0x100),
              testing::ElementsAre("................",  //
                                   "................",  //
                                   "................",  //
                                   "....vutsrqpo....",  //
                                   "....nmlkjihg....",  //
                                   "....fedcbaZY....",  //
                                   "....XWVUTSRQ....",  //
                                   "....PONMLKJI....",  //
                                   "....HGFEDCBA....",  //
                                   "................",  //
                                   "................",  //
                                   "................"));
}

TEST(ScaleRotateShapeTest, FacingWestIsAQuarterTurnAnticlockwise) {
  EXPECT_THAT(Draw(game::DirType::kWest, 0x100),
              testing::ElementsAre("................",  //
                                   "................",  //
                                   ".....HPXfnv.....",  //
                                   ".....GOWemu.....",  //
                                   ".....FNVdlt.....",  //
                                   ".....EMUcks.....",  //
                                   ".....DLTbjr.....",  //
                                   ".....CKSaiq.....",  //
                                   ".....BJRZhp.....",  //
                                   ".....AIQYgo.....",  //
                                   "................",  //
                                   "................"));
}

TEST(ScaleRotateShapeTest, FacingEastIsAQuarterTurnClockwise) {
  EXPECT_THAT(Draw(game::DirType::kEast, 0x100),
              testing::ElementsAre("................",  //
                                   "................",  //
                                   ".....ogYQIA.....",  //
                                   ".....phZRJB.....",  //
                                   ".....qiaSKC.....",  //
                                   ".....rjbTLD.....",  //
                                   ".....skcUME.....",  //
                                   ".....tldVNF.....",  //
                                   ".....umeWOG.....",  //
                                   ".....vnfXPH.....",  //
                                   "................",  //
                                   "................"));
}

// At one and a half times the size every source pixel is one or two pixels
// wide and high, the edge ones included, and the picture is 12 x 9.
TEST(ScaleRotateShapeTest, ScalesEverySourcePixelEvenly) {
  EXPECT_THAT(Draw(game::DirType::kNorth, 0x180),
              testing::ElementsAre("................",  //
                                   "................",  //
                                   "..AABCCDEEFGGH..",  //
                                   "..IIJKKLMMNOOP..",  //
                                   "..IIJKKLMMNOOP..",  //
                                   "..QQRSSTUUVWWX..",  //
                                   "..YYZaabccdeef..",  //
                                   "..YYZaabccdeef..",  //
                                   "..gghiijkklmmn..",  //
                                   "..oopqqrsstuuv..",  //
                                   "..oopqqrsstuuv..",  //
                                   "................"));
}

TEST(ScaleRotateShapeTest, HalfSizeTakesEveryOtherPixel) {
  EXPECT_THAT(Draw(game::DirType::kNorth, 0x80),
              testing::ElementsAre("................",  //
                                   "................",  //
                                   "................",  //
                                   "................",  //
                                   "................",  //
                                   "......IKMO......",  //
                                   "......Yace......",  //
                                   "......oqsu......",  //
                                   "................",  //
                                   "................",  //
                                   "................",  //
                                   "................"));
}

// A quarter turn only moves pixels, so each one arrives exactly once, and an
// odd-sized shape, whose centre is the middle of a pixel, lands in the same
// columns and rows whichever way it faces.
TEST(ScaleRotateShapeTest, QuarterTurnsOfAnOddShapeKeepEveryPixelInPlace) {
  const std::vector<uint8_t> pixels = {'A', 'B', 'C', 'D', 'E',
                                       'F', 'G', 'H', 'I'};
  EXPECT_THAT(
      Rows(ScaleRotateShape(pixels, 3, 3, game::DirType::kNorth, 0x100), 6),
      testing::ElementsAre("......", "......", "..ABC.", "..DEF.", "..GHI.",
                           "......"));
  EXPECT_THAT(
      Rows(ScaleRotateShape(pixels, 3, 3, game::DirType::kEast, 0x100), 6),
      testing::ElementsAre("......", "......", "..GDA.", "..HEB.", "..IFC.",
                           "......"));
  EXPECT_THAT(
      Rows(ScaleRotateShape(pixels, 3, 3, game::DirType::kSouth, 0x100), 6),
      testing::ElementsAre("......", "......", "..IHG.", "..FED.", "..CBA.",
                           "......"));
  EXPECT_THAT(
      Rows(ScaleRotateShape(pixels, 3, 3, game::DirType::kWest, 0x100), 6),
      testing::ElementsAre("......", "......", "..CFI.", "..BEH.", "..ADG.",
                           "......"));
}

// Off the axes a turn at full size still keeps the shape's area: no row is
// drawn twice and none is dropped.
TEST(ScaleRotateShapeTest, EveryRotationKeepsTheArea) {
  const std::vector<uint8_t> pixels(kShape.begin(), kShape.end());
  for (int rotation = 0; rotation < 256; rotation++) {
    const std::vector<uint8_t> image = ScaleRotateShape(
        pixels, kShapeWidth, kShapeHeight, game::DirType(rotation), 0x100);
    const auto drawn = static_cast<int>(std::ranges::count_if(
        image, [](const uint8_t pixel) { return pixel != 0; }));
    EXPECT_NEAR(drawn, kShapeWidth * kShapeHeight, 4) << "at " << rotation;
  }
}

}  // namespace
}  // namespace ra
