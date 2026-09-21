// Tests Screen without a window: its state before Init() and TheScreen().

#include "td/screen.h"

#include "base/installed.h"
#include "gtest/gtest.h"
#include "sdllib/pixel_buffer.h"
#include "sdllib/ww_win.h"

// ww_win.cc, pulled in through pixel_buffer, dispatches events to the app.
void SDL_Event_Handler(SDL_Event* /*event*/) {}

namespace {

TEST(ScreenTest, AsksForA400LineModeByDefault) {
  const Screen screen;
  EXPECT_EQ(screen.mode_height(), Screen::kHeight);
}

TEST(ScreenTest, MoviePageIsSizedBeforeInit) {
  Screen screen;
  EXPECT_EQ(screen.sys_mem_page().width(), 320);
  EXPECT_EQ(screen.sys_mem_page().height(), 200);
}

TEST(ScreenTest, OnlyTheVisibleViewIsVisible) {
  Screen screen;
  EXPECT_TRUE(screen.IsVisible(&screen.visible_view()));
  EXPECT_FALSE(screen.IsVisible(&screen.hidden_view()));
  EXPECT_FALSE(screen.IsVisible(&screen.visible_page()));
  EXPECT_FALSE(screen.IsVisible(nullptr));
}

TEST(ScreenTest, ViewsBelongToTheirPages) {
  Screen screen;
  EXPECT_EQ(screen.visible_view().buffer(), &screen.visible_page());
  EXPECT_EQ(screen.hidden_view().buffer(), &screen.hidden_page());
}

TEST(ScreenTest, TheScreenReturnsTheInstalledScreen) {
  Screen screen;
  const base::Installed<Screen>::Scope scope(screen);
  EXPECT_EQ(&TheScreen(), &screen);
}

TEST(ScreenTest, SetModeHeightSticks) {
  Screen screen;
  screen.set_mode_height(480);
  EXPECT_EQ(screen.mode_height(), 480);
}

}  // namespace
