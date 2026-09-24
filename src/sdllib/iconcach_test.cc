// Tests for PixelView::DrawStampLocked: reading an icon set's header and
// tables, and refusing sets it cannot draw.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "base/buffer.h"
#include "gtest/gtest.h"
#include "sdllib/pixel_buffer.h"
#include "sdllib/tile.h"

namespace {

// Lays out a Red Alert icon set of `count` tiles, `width` by `height`: the
// header, the tiles' pixels, a transparency flag per tile (all opaque) and a
// map whose cell n shows tile n.
std::vector<std::byte> MakeIconSet(int16_t width, int16_t height, int16_t count,
                                   std::span<const uint8_t> pixels) {
  const auto pixel_bytes = static_cast<int32_t>(pixels.size());
  IControl_Type header{};
  header.Width = width;
  header.Height = height;
  header.Count = count;
  header.MapWidth = count;
  header.MapHeight = 1;
  header.Icons = sizeof(IControl_Type);
  header.TransFlag = header.Icons + pixel_bytes;
  header.Map = header.TransFlag + count;
  header.Size = header.Map + count;

  std::vector<std::byte> set(static_cast<size_t>(header.Size));
  base::CopyBytes(set, base::ObjectBytes(header), sizeof(header));
  std::ranges::transform(pixels, set.begin() + header.Icons,
                         [](uint8_t pixel) { return std::byte{pixel}; });
  for (int tile = 0; tile < count; ++tile) {
    set.at(static_cast<size_t>(header.Map) + static_cast<size_t>(tile)) =
        static_cast<std::byte>(tile);
  }
  return set;
}

// Draws cell 0 of `set` at the top left of a 4x4 page, clipped to the page.
void DrawAtTopLeft(PixelBuffer& page, std::span<const std::byte> set) {
  page.view().DrawStampLocked(set, 0, 0, 0, {}, 0, 0, 4, 4);
}

TEST(DrawStampTest, DrawsTheCellsTile) {
  constexpr std::array<uint8_t, 4> kPixels{1, 2, 3, 4};
  const std::vector<std::byte> set = MakeIconSet(2, 2, 1, kPixels);
  std::vector<uint8_t> pixels(size_t{4} * 4, 0);
  PixelBuffer page(4, 4, pixels);

  DrawAtTopLeft(page, set);

  EXPECT_EQ(pixels, (std::vector<uint8_t>{1, 2, 0, 0,  //
                                          3, 4, 0, 0,  //
                                          0, 0, 0, 0,  //
                                          0, 0, 0, 0}));
}

// Sets are loaded out of the theater's MIX archive, which is freed and
// replaced when the theater changes, so a new set can sit where an old one
// was. It used to be recognized by its address alone and drawn with the old
// set's header: here, as a 2x2 tile.
TEST(DrawStampTest, ReadsANewSetAtTheAddressOfAnOldOne) {
  constexpr std::array<uint8_t, 4> kSquare{1, 2, 3, 4};
  constexpr std::array<uint8_t, 4> kRow{5, 6, 7, 8};
  std::vector<std::byte> set = MakeIconSet(2, 2, 1, kSquare);
  std::vector<uint8_t> pixels(size_t{4} * 4, 0);
  PixelBuffer page(4, 4, pixels);
  DrawAtTopLeft(page, set);

  std::ranges::fill(pixels, 0);
  const std::vector<std::byte> row = MakeIconSet(4, 1, 1, kRow);
  ASSERT_EQ(row.size(), set.size());
  std::ranges::copy(row, set.begin());
  DrawAtTopLeft(page, set);

  EXPECT_EQ(pixels, (std::vector<uint8_t>{5, 6, 7, 8,  //
                                          0, 0, 0, 0,  //
                                          0, 0, 0, 0,  //
                                          0, 0, 0, 0}));
}

// Too short to hold a header. It used to leave the previous set's tables in
// place, and drew that set's tile.
TEST(DrawStampTest, ASetTooShortForItsHeaderDrawsNothing) {
  constexpr std::array<uint8_t, 4> kPixels{1, 2, 3, 4};
  const std::vector<std::byte> set = MakeIconSet(2, 2, 1, kPixels);
  std::vector<uint8_t> pixels(size_t{4} * 4, 0);
  PixelBuffer page(4, 4, pixels);
  DrawAtTopLeft(page, set);

  std::ranges::fill(pixels, 0);
  const std::array<std::byte, 8> short_set{};
  DrawAtTopLeft(page, short_set);

  EXPECT_EQ(pixels, std::vector<uint8_t>(size_t{4} * 4, 0));
}

// A width and height both negative multiply to a positive tile size, which
// passed the size check; the clipped width then stayed negative and the row
// loops ran off the end of the page.
TEST(DrawStampTest, ASetWithNegativeTileSizeDrawsNothing) {
  constexpr std::array<uint8_t, 4> kPixels{1, 2, 3, 4};
  const std::vector<std::byte> set = MakeIconSet(-2, -2, 1, kPixels);
  std::vector<uint8_t> pixels(size_t{4} * 4, 0);
  PixelBuffer page(4, 4, pixels);

  page.view().DrawStampLocked(set, 0, 2, 2, {}, 0, 0, 4, 4);

  EXPECT_EQ(pixels, std::vector<uint8_t>(size_t{4} * 4, 0));
}

}  // namespace
