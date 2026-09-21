// Tests for the clipping in the buffer and viewport copy routines.

#include "sdllib/drawbuff.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <vector>

#include "gtest/gtest.h"
#include "sdllib/graphic_buffer.h"
#include "sdllib/ww_win.h"

// ww_win.cc, pulled in through graphic_buffer, dispatches events to the app.
void SDL_Event_Handler(SDL_Event* /*event*/) {}

namespace {

// Returns `count` pixels numbered from 1, so that every pixel of a test image
// is distinct and none is the 0 of a cleared destination.
std::vector<uint8_t> NumberedPixels(int count) {
  std::vector<uint8_t> pixels(static_cast<size_t>(count));
  std::ranges::iota(pixels, uint8_t{1});
  return pixels;
}

TEST(BufferToPageTest, ClipsRowsAndColumnsOffTheTopLeft) {
  // 1 2
  // 3 4
  // 5 6
  const std::vector<uint8_t> image = NumberedPixels(2 * 3);
  std::vector<uint8_t> page(size_t{4} * 4, 0);
  PixelBuffer view(4, 4, page);

  Buffer_To_Page(-1, -1, 2, 3, image, &view);

  EXPECT_EQ(page, (std::vector<uint8_t>{4, 0, 0, 0,  //
                                        6, 0, 0, 0,  //
                                        0, 0, 0, 0,  //
                                        0, 0, 0, 0}));
}

TEST(BufferToBufferTest, ClipsRowsAndColumnsOffTheTopLeft) {
  //  1  2  3  4
  //  5  6  7  8
  //  ...
  std::vector<uint8_t> page = NumberedPixels(4 * 4);
  PixelBuffer view(4, 4, page);
  std::array<uint8_t, 9> out{};
  out.fill(0xff);

  // The part of the 3x3 rectangle that lies off the view is left alone.
  Buffer_To_Buffer(&view, -1, -1, 3, 3, out, int32_t{out.size()});

  EXPECT_EQ(out, (std::array<uint8_t, 9>{0xff, 0xff, 0xff,  //
                                         0xff, 1, 2,        //
                                         0xff, 5, 6}));
}

TEST(LinearBlitTest, ClipsRowsAndColumnsOffTheTopLeft) {
  // 1 2 3
  // 4 5 6
  // 7 8 9
  std::vector<uint8_t> image = NumberedPixels(3 * 3);
  PixelBuffer source(3, 3, image);
  std::vector<uint8_t> page(size_t{4} * 4, 0);
  PixelBuffer view(4, 4, page);

  Linear_Blit_To_Linear(&source, &view, 0, 0, -1, -1, 3, 3, false);

  EXPECT_EQ(page, (std::vector<uint8_t>{5, 6, 0, 0,  //
                                        8, 9, 0, 0,  //
                                        0, 0, 0, 0,  //
                                        0, 0, 0, 0}));
}

TEST(LinearBlitTest, SourceClippedAtTopLeftKeepsItsPlaceInTheDestination) {
  std::vector<uint8_t> image = NumberedPixels(3 * 3);
  PixelBuffer source(3, 3, image);
  std::vector<uint8_t> page(size_t{4} * 4, 0);
  PixelBuffer view(4, 4, page);

  // The rectangle's first row and column lie off the source, so the pixels
  // that do exist belong one row down and one column right of (1, 1).
  Linear_Blit_To_Linear(&source, &view, -1, -1, 1, 1, 3, 3, false);

  EXPECT_EQ(page, (std::vector<uint8_t>{0, 0, 0, 0,  //
                                        0, 0, 0, 0,  //
                                        0, 0, 1, 2,  //
                                        0, 0, 4, 5}));
}

TEST(LinearBlitTest, ClipsRowsAndColumnsOffTheBottomRight) {
  std::vector<uint8_t> image = NumberedPixels(3 * 3);
  PixelBuffer source(3, 3, image);
  std::vector<uint8_t> page(size_t{4} * 4, 0);
  PixelBuffer view(4, 4, page);

  Linear_Blit_To_Linear(&source, &view, 0, 0, 2, 2, 3, 3, false);

  EXPECT_EQ(page, (std::vector<uint8_t>{0, 0, 0, 0,  //
                                        0, 0, 0, 0,  //
                                        0, 0, 1, 2,  //
                                        0, 0, 4, 5}));
}

// The game's pages are destroyed with the Game in ShutDown(), before the
// static destructors run; nothing may be left pointing at them.
TEST(LifetimeTest, DestroyingTheLogicPageClearsLogicPage) {
  std::vector<uint8_t> pixels(size_t{4} * 4);
  {
    PixelBuffer buffer(4, 4, pixels);
    PixelView view(&buffer, 0, 0, 2, 2);
    SetLogicPage(view);
  }
  EXPECT_EQ(LogicPage, nullptr);
}

TEST(LifetimeTest, DestroyingAnotherViewKeepsLogicPage) {
  std::vector<uint8_t> pixels(size_t{4} * 4);
  PixelBuffer buffer(4, 4, pixels);
  SetLogicPage(buffer);
  {
    const PixelView view(&buffer, 0, 0, 2, 2);
  }
  EXPECT_EQ(LogicPage, &buffer);
  SetLogicPage(nullptr);
}

TEST(LifetimeTest, DestroyingTheWindowBufferClearsWindowBuffer) {
  std::vector<uint8_t> pixels(size_t{4} * 4);
  {
    PixelBuffer buffer(4, 4, pixels);
    // Escaping the scope is the point: the destructor must clear it.
    // NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-use-after-scope)
    WindowBuffer = &buffer;
  }
  EXPECT_EQ(WindowBuffer, nullptr);
}

}  // namespace
