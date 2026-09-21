// Tests for Display: the window page it hands out and the lifetime of the
// pointer to it.

#include "sdllib/display.h"

#include <SDL_events.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "base/installed.h"
#include "gtest/gtest.h"
#include "sdllib/pixel_buffer.h"

// The event loop calls back into the app; sdllib's tests do not have one.
void SDL_Event_Handler(SDL_Event* /*event*/) {}

namespace {

// A Display that never opened a window still answers window_page(), because
// the palette and present paths run before Screen::Init() and after it tears
// the pages down.
TEST(DisplayTest, HasNoWindowPageBeforeOneIsAttached) {
  const Display display;
  EXPECT_EQ(display.window_page(), nullptr);
}

TEST(DisplayTest, AttachWindowPageRecordsThePage) {
  std::vector<uint8_t> pixels(size_t{4} * 4);
  PixelBuffer buffer(4, 4, pixels);
  Display display;

  display.AttachWindowPage(buffer);

  EXPECT_EQ(display.window_page(), &buffer);
}

TEST(DisplayTest, DetachWindowPageForgetsThePage) {
  std::vector<uint8_t> pixels(size_t{4} * 4);
  PixelBuffer buffer(4, 4, pixels);
  Display display;
  display.AttachWindowPage(buffer);

  display.DetachWindowPage();

  EXPECT_EQ(display.window_page(), nullptr);
}

// The game's pages are destroyed with the Game in ShutDown(), before the
// static destructors run; nothing may be left pointing at them. This replaces
// the WindowBuffer lifetime test that pixel_buffer_test.cc used to carry.
TEST(DisplayTest, DestroyingTheWindowPageDetachesIt) {
  std::vector<uint8_t> pixels(size_t{4} * 4);
  Display display;
  const base::Installed<Display>::Scope scope(display);
  {
    PixelBuffer buffer(4, 4, pixels);
    display.AttachWindowPage(buffer);
  }
  EXPECT_EQ(display.window_page(), nullptr);
}

// SetScreenPalette() ran before the window existed and checked WindowBuffer
// for null; the same call must still be harmless.
TEST(DisplayTest, SetPaletteWithoutAWindowPageDoesNothing) {
  const std::vector<uint8_t> palette(size_t{256} * 3);
  Display display;

  display.SetPalette(palette);

  EXPECT_EQ(display.window_page(), nullptr);
}

}  // namespace
