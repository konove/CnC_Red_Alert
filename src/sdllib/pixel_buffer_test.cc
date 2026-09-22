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
// SDL and the nesting count still tracks the calls.
TEST(GraphicViewPortLockTest, MemoryBufferLocksNest) {
  std::vector<uint8_t> pixels(size_t{4} * 4, 0);
  PixelBuffer page(4, 4, pixels);
  PixelView view(&page, 1, 1, 2, 2);

  ASSERT_TRUE(view.Lock());
  ASSERT_TRUE(view.Lock());
  EXPECT_TRUE(view.Unlock());
  EXPECT_TRUE(view.Unlock());
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

  source.view().Blit(dest.view(), 0, 0, -1, -1, 3, 3);

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
  source.view().Blit(dest.view(), -1, -1, 1, 1, 3, 3);

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

  source.view().Blit(dest.view(), 0, 0, 2, 2, 3, 3);

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

  source.view().Blit(dest.view(), 0, 0, -3, 0, 3, 3);
  EXPECT_EQ(page, blank);
  source.view().Blit(dest.view(), 0, 0, 4, 0, 3, 3);
  EXPECT_EQ(page, blank);
  source.view().Blit(dest.view(), 0, 0, 0, -3, 3, 3);
  EXPECT_EQ(page, blank);
  source.view().Blit(dest.view(), 0, 0, 0, 4, 3, 3);
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
  source.view().Blit(dest.view(), 0, 0, -1, 2, 3, 3);

  EXPECT_EQ(page, (std::vector<uint8_t>{0, 0, 0, 0,  //
                                        0, 0, 0, 0,  //
                                        2, 3, 0, 0,  //
                                        5, 6, 0, 0}));
}

}  // namespace
