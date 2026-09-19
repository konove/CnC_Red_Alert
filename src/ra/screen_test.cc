// Tests Screen without a window: its state before Init() and TheScreen().

#include "ra/screen.h"

#include "base/installed.h"
#include "gtest/gtest.h"
#include "sdllib/gbuffer.h"
#include "sdllib/ww_win.h"

// ww_win.cc, pulled in through gbuffer, dispatches events to the app.
void SDL_Event_Handler(SDL_Event* /*event*/) {}

namespace {

TEST(ScreenTest, AsksForA400LineModeByDefault) {
  const Screen screen;
  EXPECT_EQ(screen.mode_height(), Screen::kHeight);
  EXPECT_FALSE(screen.is_vq640());
}

TEST(ScreenTest, MoviePagesAreSizedBeforeInit) {
  Screen screen;
  EXPECT_EQ(screen.sys_mem_page().Get_Width(), 320);
  EXPECT_EQ(screen.sys_mem_page().Get_Height(), 200);
  EXPECT_EQ(screen.vq640().Get_Width(), Screen::kWidth);
  EXPECT_EQ(screen.vq640().Get_Height(), Screen::kHeight);
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
  EXPECT_EQ(screen.visible_view().Get_Graphic_Buffer(), &screen.visible_page());
  EXPECT_EQ(screen.hidden_view().Get_Graphic_Buffer(), &screen.hidden_page());
}

TEST(ScreenTest, TheScreenReturnsTheInstalledScreen) {
  Screen screen;
  const base::Installed<Screen>::Scope scope(screen);
  EXPECT_EQ(&TheScreen(), &screen);
}

TEST(ScreenTest, SettersStick) {
  Screen screen;
  screen.set_mode_height(480);
  screen.set_is_vq640(true);
  EXPECT_EQ(screen.mode_height(), 480);
  EXPECT_TRUE(screen.is_vq640());
}

}  // namespace
