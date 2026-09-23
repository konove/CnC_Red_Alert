// Tests for PixelView and PixelBuffer: locking, the lifetime of the
// globals that point at them, and the clipping the drawing primitives do.

#include "sdllib/pixel_buffer.h"

#include <SDL_events.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <vector>

#include "gtest/gtest.h"
#include "port/unaligned.h"
#include "sdllib/font.h"

// ww_win.cc, pulled in through pixel_buffer, dispatches events to the app.
void SDL_Event_Handler(SDL_Event* /*event*/) {}

namespace {

// A viewport that Attach() has not been called on yet. Screen builds its
// views this way and attaches them once the video mode is known, so anything
// that draws in between meets one.
TEST(GraphicViewPortLockTest, AnUnattachedViewportDoesNotLock) {
  PixelView view;

  EXPECT_FALSE(view.Lock());
  EXPECT_EQ(view.lock_count(), 0);
}

// The drawing members all call the primitive only when Lock() succeeded, so
// an unattached viewport has to be a silent no-op rather than a crash.
TEST(GraphicViewPortLockTest, DrawingToAnUnattachedViewportDoesNothing) {
  PixelView view;

  view.Clear();
  view.PutPixel(0, 0, 1);
  view.DrawLine(0, 0, 1, 1, 1);
  view.FillRect(0, 0, 1, 1, 1);

  EXPECT_EQ(view.GetPixel(0, 0), 0);
}

// Screen constructs its views against pages that Init() has not sized yet,
// so Attach() has to cope with a buffer that has no pixels. It used to clamp
// the viewport's corner to width() - 1, that is to -1, and pixels() then
// subspanned an empty span by a negative offset.
TEST(GraphicViewPortLockTest, AttachingToAnEmptyBufferGivesAnEmptyViewport) {
  PixelBuffer page;
  PixelView view(&page, 0, 0, 640, 480);

  EXPECT_EQ(view.width(), 0);
  EXPECT_EQ(view.height(), 0);
  EXPECT_EQ(view.x_pos(), 0);
  EXPECT_EQ(view.y_pos(), 0);
  EXPECT_TRUE(view.pixels().empty());

  view.Clear();
  view.PutPixel(0, 0, 1);
}

// A plain memory buffer has no surface to lock, so Lock() succeeds without
// SDL and nothing is counted.
TEST(GraphicViewPortLockTest, MemoryBufferLocksWithoutCounting) {
  std::vector<uint8_t> pixels(size_t{4} * 4, 0);
  PixelBuffer page(4, 4, pixels);
  PixelView view(&page, 1, 1, 2, 2);

  ASSERT_TRUE(view.Lock());
  ASSERT_TRUE(view.Lock());
  EXPECT_EQ(view.lock_count(), 0);
  view.Unlock();
  view.Unlock();
  EXPECT_EQ(view.lock_count(), 0);
}

// Returns `count` pixels numbered from 1, so that every pixel of a test image
// is distinct and none is the 0 of a cleared destination.
std::vector<uint8_t> NumberedPixels(int count) {
  std::vector<uint8_t> pixels(static_cast<size_t>(count));
  std::ranges::iota(pixels, uint8_t{1});
  return pixels;
}

TEST(CopyFromBufferTest, ClipsRowsAndColumnsOffTheTopLeft) {
  // 1 2
  // 3 4
  // 5 6
  const std::vector<uint8_t> image = NumberedPixels(2 * 3);
  std::vector<uint8_t> page(size_t{4} * 4, 0);
  PixelBuffer dest(4, 4, page);

  dest.view().CopyFromBuffer(-1, -1, 2, 3, image);

  EXPECT_EQ(page, (std::vector<uint8_t>{4, 0, 0, 0,  //
                                        6, 0, 0, 0,  //
                                        0, 0, 0, 0,  //
                                        0, 0, 0, 0}));
}

TEST(CopyToBufferTest, ClipsRowsAndColumnsOffTheTopLeft) {
  //  1  2  3  4
  //  5  6  7  8
  //  ...
  std::vector<uint8_t> page = NumberedPixels(4 * 4);
  PixelBuffer dest(4, 4, page);
  std::array<uint8_t, 9> out{};
  out.fill(0xff);

  // The part of the 3x3 rectangle that lies off the view is left alone.
  dest.view().CopyToBuffer(-1, -1, 3, 3, out);

  EXPECT_EQ(out, (std::array<uint8_t, 9>{0xff, 0xff, 0xff,  //
                                         0xff, 1, 2,        //
                                         0xff, 5, 6}));
}

TEST(BlitTest, ClipsRowsAndColumnsOffTheTopLeft) {
  // 1 2 3
  // 4 5 6
  // 7 8 9
  std::vector<uint8_t> image = NumberedPixels(3 * 3);
  PixelBuffer source(3, 3, image);
  std::vector<uint8_t> page(size_t{4} * 4, 0);
  PixelBuffer dest(4, 4, page);

  source.view().BlitTo(dest.view(), 0, 0, -1, -1, 3, 3);

  EXPECT_EQ(page, (std::vector<uint8_t>{5, 6, 0, 0,  //
                                        8, 9, 0, 0,  //
                                        0, 0, 0, 0,  //
                                        0, 0, 0, 0}));
}

TEST(BlitTest, SourceClippedAtTopLeftKeepsItsPlaceInTheDestination) {
  std::vector<uint8_t> image = NumberedPixels(3 * 3);
  PixelBuffer source(3, 3, image);
  std::vector<uint8_t> page(size_t{4} * 4, 0);
  PixelBuffer dest(4, 4, page);

  // The rectangle's first row and column lie off the source, so the pixels
  // that do exist belong one row down and one column right of (1, 1).
  source.view().BlitTo(dest.view(), -1, -1, 1, 1, 3, 3);

  EXPECT_EQ(page, (std::vector<uint8_t>{0, 0, 0, 0,  //
                                        0, 0, 0, 0,  //
                                        0, 0, 1, 2,  //
                                        0, 0, 4, 5}));
}

TEST(BlitTest, ClipsRowsAndColumnsOffTheBottomRight) {
  std::vector<uint8_t> image = NumberedPixels(3 * 3);
  PixelBuffer source(3, 3, image);
  std::vector<uint8_t> page(size_t{4} * 4, 0);
  PixelBuffer dest(4, 4, page);

  source.view().BlitTo(dest.view(), 0, 0, 2, 2, 3, 3);

  EXPECT_EQ(page, (std::vector<uint8_t>{0, 0, 0, 0,  //
                                        0, 0, 0, 0,  //
                                        0, 0, 1, 2,  //
                                        0, 0, 4, 5}));
}

TEST(BlitTest, RectangleWhollyOffTheDestinationDrawsNothing) {
  std::vector<uint8_t> image = NumberedPixels(3 * 3);
  PixelBuffer source(3, 3, image);
  const std::vector<uint8_t> blank(size_t{4} * 4, 0);
  std::vector<uint8_t> page = blank;
  PixelBuffer dest(4, 4, page);

  source.view().BlitTo(dest.view(), 0, 0, -3, 0, 3, 3);
  EXPECT_EQ(page, blank);
  source.view().BlitTo(dest.view(), 0, 0, 4, 0, 3, 3);
  EXPECT_EQ(page, blank);
  source.view().BlitTo(dest.view(), 0, 0, 0, -3, 3, 3);
  EXPECT_EQ(page, blank);
  source.view().BlitTo(dest.view(), 0, 0, 0, 4, 3, 3);
  EXPECT_EQ(page, blank);
}

// The shape the chronal vortex blits: a source buffer exactly the size of the
// rectangle, landing partly off two edges of the destination at once.
TEST(BlitTest, FullSourceOverhangingTwoEdgesCarriesTheSourceAlong) {
  std::vector<uint8_t> image = NumberedPixels(3 * 3);
  PixelBuffer source(3, 3, image);
  std::vector<uint8_t> page(size_t{4} * 4, 0);
  PixelBuffer dest(4, 4, page);

  // Off the left by one and off the bottom by one.
  source.view().BlitTo(dest.view(), 0, 0, -1, 2, 3, 3);

  EXPECT_EQ(page, (std::vector<uint8_t>{0, 0, 0, 0,  //
                                        0, 0, 0, 0,  //
                                        2, 3, 0, 0,  //
                                        5, 6, 0, 0}));
}

// A three-glyph font, 2 rows tall: glyph 1 is 3 pixels wide and 2 rows,
// glyph 2 is 2 pixels wide and 1 row with 1 blank row above.
std::vector<std::byte> MakePrintFont() {
  std::vector<std::byte> blob(42, std::byte{0});
  const auto word = [&blob](int offset, int value) {
    port::WriteUnaligned(std::span(blob).subspan(static_cast<size_t>(offset)),
                         static_cast<uint16_t>(value));
  };
  const auto byte = [&blob](int offset, int value) {
    blob.at(static_cast<size_t>(offset)) = static_cast<std::byte>(value);
  };
  word(4, 14);   // info block
  word(6, 20);   // offset table
  word(8, 26);   // width table
  word(10, 35);  // glyph data
  word(12, 29);  // height table
  byte(14 + kFontInfoMaxHeight, 2);
  byte(14 + kFontInfoMaxWidth, 3);
  word(22, 35);             // glyph 1 data
  word(24, 39);             // glyph 2 data
  byte(27, 3);              // glyph 1 width
  byte(28, 2);              // glyph 2 width
  word(31, 2 * 256);        // glyph 1: 2 rows
  word(33, (1 * 256) + 1);  // glyph 2: 1 row, 1 blank above
  // Glyph 1 rows: pixel values 1 2 3 and 0 1 2, two per byte, low nibble
  // first. Glyph 2: 1 3.
  byte(35, 0x21);
  byte(36, 0x03);
  byte(37, 0x10);
  byte(38, 0x02);
  byte(39, 0x31);
  return blob;
}

TEST(PrintTest, DrawsThroughTheStylesPaletteAndSpacing) {
  const std::vector<std::byte> blob = MakePrintFont();
  FontStyle style{.font = FontView(blob), .x_spacing = 1};
  style.palette.at(2) = 50;
  style.palette.at(3) = 60;
  std::vector<uint8_t> pixels(size_t{8} * 2, 0);
  PixelBuffer page(8, 2, pixels);

  page.view().Print(style, "\x01\x02", 0, 0, 7, 0);

  // Glyph 1 at x 0-2, one pixel of spacing, glyph 2 at x 4-5 on row 1.
  EXPECT_EQ(pixels, (std::vector<uint8_t>{7, 50, 60, 0, 0, 0, 0, 0,  //
                                          0, 7, 50, 0, 7, 60, 0, 0}));
}

}  // namespace
