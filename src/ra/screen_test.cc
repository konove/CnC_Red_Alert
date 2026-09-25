// Tests Screen without a window: its state before Init() and TheScreen().

#include "ra/screen.h"

#include "engine/base/installed.h"
#include "engine/gfx/pixel_buffer.h"
#include "gtest/gtest.h"

namespace {

TEST(ScreenTest, AsksForA400LineModeByDefault) {
  const Screen screen;
  EXPECT_EQ(screen.mode_height(), Screen::kHeight);
  EXPECT_FALSE(screen.is_vq640());
}

TEST(ScreenTest, MoviePagesAreSizedBeforeInit) {
  Screen screen;
  EXPECT_EQ(screen.sys_mem_page().width(), 320);
  EXPECT_EQ(screen.sys_mem_page().height(), 200);
  EXPECT_EQ(screen.vq640().width(), Screen::kWidth);
  EXPECT_EQ(screen.vq640().height(), Screen::kHeight);
}

TEST(ScreenTest, OnlyTheVisibleViewIsVisible) {
  Screen screen;
  EXPECT_TRUE(screen.IsVisible(&screen.visible_view()));
  EXPECT_FALSE(screen.IsVisible(&screen.hidden_view()));
  EXPECT_FALSE(screen.IsVisible(&screen.visible_page().view()));
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

TEST(ScreenTest, SettersStick) {
  Screen screen;
  screen.set_mode_height(480);
  screen.set_is_vq640(true);
  EXPECT_EQ(screen.mode_height(), 480);
  EXPECT_TRUE(screen.is_vq640());
}

}  // namespace
