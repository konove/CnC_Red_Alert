// Tests for PixelView::DrawStampLocked: reading an icon set's header and
// tables, clipping the tile, and refusing sets it cannot draw.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <span>
#include <vector>

#include "base/unaligned.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/tile.h"
#include "gtest/gtest.h"

namespace {

// A 2x2 tile:
//   1 2
//   3 4
constexpr std::array<uint8_t, 4> kSquare{1, 2, 3, 4};

// Lays out a Red Alert icon set of one tile, `width` by `height`: the header,
// the tile's pixels, its transparency flag and a one-cell map showing it.
std::vector<std::byte> MakeIconSet(const int16_t width, const int16_t height,
                                   std::span<const uint8_t> pixels,
                                   const bool transparent = false) {
  IControl_Type header{};
  header.Width = width;
  header.Height = height;
  header.Count = 1;
  header.MapWidth = 1;
  header.MapHeight = 1;
  header.Icons = sizeof(IControl_Type);
  header.TransFlag = header.Icons + static_cast<int32_t>(pixels.size());
  header.Map = header.TransFlag + 1;
  header.Size = header.Map + 1;

  std::vector<std::byte> set(static_cast<size_t>(header.Size));
  base::WriteUnaligned(set, header);
  std::ranges::transform(pixels, set.begin() + header.Icons,
                         [](const uint8_t pixel) { return std::byte{pixel}; });
  set.at(static_cast<size_t>(header.TransFlag)) = std::byte{transparent};
  set.at(static_cast<size_t>(header.Map)) = std::byte{0};
  return set;
}

// Draws on a 4x4 page whose pixels start out as 0.
class DrawStampTest : public testing::Test {
 protected:
  // Draws cell 0 of `set` at x,y, clipped to the whole page.
  void Draw(const std::span<const std::byte> set, const int x = 0,
            const int y = 0) {
    page_.view().DrawStampLocked(set, 0, x, y, {}, 0, 0, 4, 4);
  }

  std::vector<uint8_t> pixels_ = std::vector<uint8_t>(size_t{4} * 4, 0);
  PixelBuffer page_{4, 4, pixels_};
};

// The page as it was before any drawing.
std::vector<uint8_t> BlankPage() {
  return std::vector<uint8_t>(size_t{4} * 4, 0);
}

TEST_F(DrawStampTest, DrawsTheCellsTile) {
  Draw(MakeIconSet(2, 2, kSquare));

  EXPECT_EQ(pixels_, (std::vector<uint8_t>{1, 2, 0, 0,  //
                                           3, 4, 0, 0,  //
                                           0, 0, 0, 0,  //
                                           0, 0, 0, 0}));
}

TEST_F(DrawStampTest, ClipsTheTileAtTheTopLeft) {
  Draw(MakeIconSet(2, 2, kSquare), -1, -1);

  EXPECT_EQ(pixels_, (std::vector<uint8_t>{4, 0, 0, 0,  //
                                           0, 0, 0, 0,  //
                                           0, 0, 0, 0,  //
                                           0, 0, 0, 0}));
}

TEST_F(DrawStampTest, ClipsTheTileAtTheBottomRight) {
  Draw(MakeIconSet(2, 2, kSquare), 3, 3);

  EXPECT_EQ(pixels_, (std::vector<uint8_t>{0, 0, 0, 0,  //
                                           0, 0, 0, 0,  //
                                           0, 0, 0, 0,  //
                                           0, 0, 0, 1}));
}

TEST_F(DrawStampTest, ATileEndingOnTheClipEdgeDrawsNothing) {
  Draw(MakeIconSet(2, 2, kSquare), -2, 0);
  Draw(MakeIconSet(2, 2, kSquare), 0, 4);

  EXPECT_EQ(pixels_, BlankPage());
}

// x,y are relative to the clip rectangle, and the rectangle clips the tile
// even where the page would not.
TEST_F(DrawStampTest, PositionsAndClipsWithinTheClipRectangle) {
  // A 3x3 tile, numbered 1 to 9.
  std::array<uint8_t, 9> tile{};
  std::ranges::iota(tile, uint8_t{1});

  page_.view().DrawStampLocked(MakeIconSet(3, 3, tile), 0, -1, 0, {}, 1, 1, 2,
                               2);

  EXPECT_EQ(pixels_, (std::vector<uint8_t>{0, 0, 0, 0,  //
                                           0, 2, 3, 0,  //
                                           0, 5, 6, 0,  //
                                           0, 0, 0, 0}));
}

TEST_F(DrawStampTest, ColorZeroIsTransparentOnlyInATransparentTile) {
  constexpr std::array<uint8_t, 4> kHoles{0, 2, 3, 0};
  std::ranges::fill(pixels_, 9);

  Draw(MakeIconSet(2, 2, kHoles, /*transparent=*/true), 0, 0);
  Draw(MakeIconSet(2, 2, kHoles, /*transparent=*/false), 2, 2);

  EXPECT_EQ(pixels_, (std::vector<uint8_t>{9, 2, 9, 9,  //
                                           3, 9, 9, 9,  //
                                           9, 9, 0, 2,  //
                                           9, 9, 3, 0}));
}

// A remapped tile is transparent wherever the remapped color is 0, whatever
// its flag says.
TEST_F(DrawStampTest, RemapsAndTreatsRemappedZeroAsTransparent) {
  std::array<uint8_t, 256> remap{};
  std::ranges::iota(remap, uint8_t{0});
  remap.at(1) = 0;
  remap.at(4) = 40;
  std::ranges::fill(pixels_, 9);

  page_.view().DrawStampLocked(MakeIconSet(2, 2, kSquare), 0, 0, 0, remap, 0, 0,
                               4, 4);

  EXPECT_EQ(pixels_, (std::vector<uint8_t>{9, 2, 9, 9,   //
                                           3, 40, 9, 9,  //
                                           9, 9, 9, 9,   //
                                           9, 9, 9, 9}));
}

TEST_F(DrawStampTest, ARemapTableShorterThan256DrawsNothing) {
  constexpr std::array<uint8_t, 16> remap{};

  page_.view().DrawStampLocked(MakeIconSet(2, 2, kSquare), 0, 0, 0, remap, 0, 0,
                               4, 4);

  EXPECT_EQ(pixels_, BlankPage());
}

// Sets are loaded out of the theater's MIX archive, which is freed and
// replaced when the theater changes, so a new set can sit where an old one
// was. It used to be recognized by its address alone and drawn with the old
// set's header: here, as a 2x2 tile.
TEST_F(DrawStampTest, ReadsANewSetAtTheAddressOfAnOldOne) {
  constexpr std::array<uint8_t, 4> kRow{5, 6, 7, 8};
  std::vector<std::byte> set = MakeIconSet(2, 2, kSquare);
  Draw(set);

  std::ranges::fill(pixels_, 0);
  const std::vector<std::byte> row = MakeIconSet(4, 1, kRow);
  ASSERT_EQ(row.size(), set.size());
  std::ranges::copy(row, set.begin());
  Draw(set);

  EXPECT_EQ(pixels_, (std::vector<uint8_t>{5, 6, 7, 8,  //
                                           0, 0, 0, 0,  //
                                           0, 0, 0, 0,  //
                                           0, 0, 0, 0}));
}

// Too short to hold a header. It used to leave the previous set's tables in
// place, and drew that set's tile.
TEST_F(DrawStampTest, ASetTooShortForItsHeaderDrawsNothing) {
  Draw(MakeIconSet(2, 2, kSquare));

  std::ranges::fill(pixels_, 0);
  constexpr std::array<std::byte, 8> short_set{};
  Draw(short_set);

  EXPECT_EQ(pixels_, BlankPage());
}

// A width and height both negative multiply to a positive tile size, which
// passed the size check; the clipped width then stayed negative and the row
// loops ran off the end of the page.
TEST_F(DrawStampTest, ASetWithNegativeTileSizeDrawsNothing) {
  Draw(MakeIconSet(-2, -2, kSquare), 2, 2);

  EXPECT_EQ(pixels_, BlankPage());
}

}  // namespace
