// Tests for the view locking in pixel_buffer.h.

#include "sdllib/pixel_buffer.h"

#include <SDL_events.h>

#include <cstddef>
#include <cstdint>
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

}  // namespace
