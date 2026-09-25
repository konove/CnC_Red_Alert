// Tests for Display: the paletted surface it lends the window page, the
// palette it shows it through, and the stretched movie frames it presents
// instead of it.

#include "engine/window/display.h"

#include <SDL_pixels.h>
#include <SDL_render.h>
#include <SDL_surface.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <span>

#include "absl/base/attributes.h"
#include "base/installed.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/pixel_surface.h"
#include "gtest/gtest.h"

namespace engine::window {
namespace {

// Returns a 6-bit VGA palette that is black but for `color` at `index`.
std::array<uint8_t, 768> PaletteWith(size_t index, uint8_t red, uint8_t green,
                                     uint8_t blue) {
  std::array<uint8_t, 768> palette{};
  palette.at(index * 3) = red;
  palette.at((index * 3) + 1) = green;
  palette.at((index * 3) + 2) = blue;
  return palette;
}

// The palette and present paths run before Screen::Init() sets a video mode
// and after the game tears it down, so a Display without a surface has to
// take every call and do nothing. This one has no renderer either, so any
// SDL call that reached for one would fail the test by crashing.
TEST(DisplayWithoutVideoModeTest, HasNoSurfaceToLock) {
  Display display;

  EXPECT_FALSE(display.Lock().has_value());
  EXPECT_EQ(display.palette(), nullptr);
}

TEST(DisplayWithoutVideoModeTest, IgnoresPalettesFramesAndPresents) {
  Display display;
  const base::Installed<Display>::Scope scope(display);

  display.SetPalette(PaletteWith(1, 63, 0, 0));
  display.PresentScaledFrame(std::array<uint8_t, 4>{1, 1, 1, 1}, 2, 2);
  display.EndFrame();

  EXPECT_EQ(display.palette(), nullptr);
  EXPECT_FALSE(display.redraw_pending());
}

TEST(DisplayWithoutVideoModeTest, SetVideoModeNeedsARenderer) {
  Display display;

  EXPECT_FALSE(display.SetVideoMode(2, 2));
  EXPECT_FALSE(display.Lock().has_value());
}

// Draws through a software renderer onto a 2x2 target, so the colors that
// reach the screen can be read back without a window.
class DisplayTest : public ::testing::Test {
 protected:
  void SetUp() override {
    target_ =
        SDL_CreateRGBSurfaceWithFormat(0, 2, 2, 32, SDL_PIXELFORMAT_RGBA32);
    ASSERT_NE(target_, nullptr);
    renderer_ = SDL_CreateSoftwareRenderer(target_);
    ASSERT_NE(renderer_, nullptr);
    // The Display borrows the renderer; TearDown() still owns it.
    display_ = std::make_unique<Display>(nullptr, renderer_);
    display_scope_.emplace(*display_);
    ASSERT_TRUE(display_->SetVideoMode(2, 2));
  }

  void TearDown() override {
    display_scope_.reset();
    display_.reset();
    SDL_DestroyRenderer(renderer_);
    renderer_ = nullptr;
    SDL_FreeSurface(target_);
  }

  Display& display() ABSL_ATTRIBUTE_LIFETIME_BOUND { return *display_; }

  // Returns the top-left pixel on screen as {red, green, blue}.
  [[nodiscard]] std::array<uint8_t, 3> ScreenColor() const {
    std::array<uint8_t, 16> pixels{};
    EXPECT_EQ(SDL_RenderReadPixels(renderer_, nullptr, SDL_PIXELFORMAT_RGBA32,
                                   pixels.data(), 8),
              0);
    return {pixels.at(0), pixels.at(1), pixels.at(2)};
  }

 private:
  SDL_Surface* target_ = nullptr;
  SDL_Renderer* renderer_ = nullptr;
  std::unique_ptr<Display> display_;
  std::optional<base::Installed<Display>::Scope> display_scope_;
};

TEST_F(DisplayTest, LendsThePixelsOfTheVideoMode) {
  const std::optional<PixelSurface::Pixels> pixels = display().Lock();
  ASSERT_TRUE(pixels.has_value());
  EXPECT_GE(pixels->pitch, 2);
  EXPECT_GE(pixels->bytes.size(), static_cast<size_t>(pixels->pitch) * 2);
  display().Unlock();
  display().EndFrame();
}

// Giving the pixels back only arms the redraw timer; the frame reaches the
// window when something ends it.
TEST_F(DisplayTest, UnlockDefersThePresentToTheEndOfTheFrame) {
  display().UpdatePalette(PaletteWith(1, 63, 0, 0));
  display().EndFrame();
  ASSERT_TRUE(display().Lock().has_value());
  display().Unlock();

  EXPECT_TRUE(display().redraw_pending());
  display().EndFrame();
  EXPECT_FALSE(display().redraw_pending());
}

// The window page drawn on through the Display: what it draws reaches the
// screen in the current palette.
TEST_F(DisplayTest, PresentsWhatThePageDrewInThePalette) {
  PixelBuffer page;
  page.Init(2, 2, display());
  display().UpdatePalette(PaletteWith(1, 0, 0, 63));

  page.view().Clear(1);
  display().EndFrame();

  EXPECT_EQ(ScreenColor(), (std::array<uint8_t, 3>{0, 0, 255}));
}

TEST_F(DisplayTest, ResetVideoModeTakesTheSurfaceAway) {
  display().ResetVideoMode();

  EXPECT_FALSE(display().Lock().has_value());
  EXPECT_EQ(display().palette(), nullptr);
}

TEST_F(DisplayTest, ShowsAScaledFrameInTheCurrentPalette) {
  display().UpdatePalette(PaletteWith(1, 63, 0, 0));
  display().PresentScaledFrame(std::array<uint8_t, 4>{1, 1, 1, 1}, 2, 2);
  EXPECT_EQ(ScreenColor(), (std::array<uint8_t, 3>{255, 0, 0}));
}

// The texture holds baked colors, so a palette-cycling screen (the mission
// map's pulsing hotspots) depends on a palette change converting them again.
TEST_F(DisplayTest, AScaledFrameFollowsALaterPaletteChange) {
  display().UpdatePalette(PaletteWith(1, 63, 0, 0));
  display().PresentScaledFrame(std::array<uint8_t, 4>{1, 1, 1, 1}, 2, 2);

  display().UpdatePalette(PaletteWith(1, 0, 63, 0));
  display().EndFrame();
  EXPECT_EQ(ScreenColor(), (std::array<uint8_t, 3>{0, 255, 0}));
}

// As soon as the game draws on the surface again, it replaces the frame.
TEST_F(DisplayTest, DrawingOnTheSurfaceDropsTheScaledFrame) {
  display().UpdatePalette(PaletteWith(1, 63, 0, 0));
  display().PresentScaledFrame(std::array<uint8_t, 4>{1, 1, 1, 1}, 2, 2);

  ASSERT_TRUE(display().Lock().has_value());
  display().Unlock();
  display().EndFrame();

  EXPECT_EQ(ScreenColor(), (std::array<uint8_t, 3>{0, 0, 0}));
}

// A frame with no pixels, or with fewer pixels than its size says, is
// rejected without presenting anything; the surface (all color 0, black)
// stays what an end of frame shows.
TEST_F(DisplayTest, RejectsFramesThatDoNotMatchTheirSize) {
  display().UpdatePalette(PaletteWith(1, 63, 0, 0));
  const std::array<uint8_t, 4> pixels{1, 1, 1, 1};

  display().PresentScaledFrame(pixels, 0, 2);
  display().PresentScaledFrame(pixels, 2, 0);
  display().PresentScaledFrame(pixels, -1, 2);
  display().PresentScaledFrame(pixels, 2, -1);
  display().PresentScaledFrame(std::span(pixels).first(3), 2, 2);
  display().PresentScaledFrame(pixels, std::numeric_limits<int>::max(),
                               std::numeric_limits<int>::max());
  display().EndFrame();

  EXPECT_EQ(ScreenColor(), (std::array<uint8_t, 3>{0, 0, 0}));
}

}  // namespace
}  // namespace engine::window
