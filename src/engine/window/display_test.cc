// Tests for the Displays: the paletted surface it lends the window page, the
// palette it shows it through, the stretched movie frames it presents
// instead of it, and the event loop that hands events to its client.

#include "engine/window/display.h"

#include <SDL.h>
#include <SDL_events.h>
#include <SDL_pixels.h>
#include <SDL_render.h>
#include <SDL_stdinc.h>
#include <SDL_surface.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <span>

#include "absl/base/attributes.h"
#include "base/array.h"
#include "base/installed.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/palette_color.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/pixel_surface.h"
#include "engine/window/display_client.h"
#include "engine/window/display_observer.h"
#include "engine/window/headless_display.h"
#include "engine/window/window_display.h"
#include "gtest/gtest.h"

namespace engine::window {
namespace {

// Returns a palette that is black but for the color with VGA guns `red`,
// `green` and `blue` at `index`.
engine::gfx::Palette PaletteWith(int index, int red, int green, int blue) {
  engine::gfx::Palette palette;
  palette.at(index) = engine::gfx::PaletteColor(engine::gfx::ExpandGun(red),
                                                engine::gfx::ExpandGun(green),
                                                engine::gfx::ExpandGun(blue));
  return palette;
}

// Counts the frames a Display ends.
class RecordingFrames final : public DisplayObserver {
 public:
  void OnFrameEnd() override { ++ended; }

  int ended = 0;
};

// The palette and present paths run before Screen::Init() sets a video mode
// and after the game tears it down, so a Display without a surface has to
// take every call and do nothing. This one has no renderer either, so any
// SDL call that reached for one would fail the test by crashing.
TEST(DisplayWithoutVideoModeTest, HasNoSurfaceToLock) {
  WindowDisplay display;

  EXPECT_FALSE(display.Lock().has_value());
  EXPECT_EQ(display.palette(), nullptr);
}

TEST(DisplayWithoutVideoModeTest, IgnoresPalettesFramesAndPresents) {
  WindowDisplay display;
  const base::Installed<Display>::Scope scope(display);

  display.SetPalette(PaletteWith(1, 63, 0, 0));
  display.PresentScaledFrame(std::array<uint8_t, 4>{1, 1, 1, 1}, 2, 2);
  display.EndFrame();

  EXPECT_EQ(display.palette(), nullptr);
  EXPECT_FALSE(display.redraw_pending());
}

TEST(DisplayWithoutVideoModeTest, PictureScaleIsOneWithoutARenderer) {
  const WindowDisplay display;

  EXPECT_EQ(display.PictureScale(), 1.0F);
}

// SDL's integer scaling draws a 640x400 picture at the largest whole factor
// that fits: 1400x900 fits twice (2.19 across, 2.25 down), not two and a
// quarter times.
TEST(DisplayPictureScaleTest, IsTheWholeFactorThePictureIsDrawnAt) {
  SDL_Surface* target =
      SDL_CreateRGBSurfaceWithFormat(0, 1400, 900, 32, SDL_PIXELFORMAT_RGBA32);
  ASSERT_NE(target, nullptr);
  SDL_Renderer* renderer = SDL_CreateSoftwareRenderer(target);
  ASSERT_NE(renderer, nullptr);
  SDL_RenderSetLogicalSize(renderer, 640, 400);
  SDL_RenderSetIntegerScale(renderer, SDL_TRUE);

  {
    const WindowDisplay display(nullptr, renderer);
    EXPECT_EQ(display.PictureScale(), 2.0F);
  }

  SDL_DestroyRenderer(renderer);
  SDL_FreeSurface(target);
}

TEST(DisplayWithoutVideoModeTest, SetVideoModeNeedsARenderer) {
  WindowDisplay display;

  EXPECT_FALSE(display.SetVideoMode(2, 2));
  EXPECT_FALSE(display.Lock().has_value());
}

// Draws through a software renderer onto a 2x2 target, so the colors that
// reach the screen can be read back without a window.
template <class DisplayType>
class RendererDisplayTest : public ::testing::Test {
 protected:
  void SetUp() override {
    target_ =
        SDL_CreateRGBSurfaceWithFormat(0, 2, 2, 32, SDL_PIXELFORMAT_RGBA32);
    ASSERT_NE(target_, nullptr);
    renderer_ = SDL_CreateSoftwareRenderer(target_);
    ASSERT_NE(renderer_, nullptr);
    // The Display borrows the renderer; TearDown() still owns it.
    display_ = std::make_unique<DisplayType>(nullptr, renderer_);
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

  DisplayType& display() ABSL_ATTRIBUTE_LIFETIME_BOUND { return *display_; }

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
  std::unique_ptr<DisplayType> display_;
  std::optional<base::Installed<Display>::Scope> display_scope_;
};

using DisplayTest = RendererDisplayTest<WindowDisplay>;
// The window a script plays the game in, which keeps no time of its own.
using ScriptedWindowDisplayTest = RendererDisplayTest<ScriptedWindowDisplay>;

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
  display().SetPalette(PaletteWith(1, 63, 0, 0));
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
  display().SetPalette(PaletteWith(1, 0, 0, 63));

  page.view().Clear(1);
  display().EndFrame();

  EXPECT_EQ(ScreenColor(), (std::array<uint8_t, 3>{0, 0, 255}));
}

// Palettes hold 8-bit components, and the screen shows them unchanged.
TEST_F(DisplayTest, ShowsEveryEightBitComponent) {
  PixelBuffer page;
  page.Init(2, 2, display());
  engine::gfx::Palette palette;
  palette.at(1) = engine::gfx::PaletteColor(1, 129, 254);
  display().SetPalette(palette);

  page.view().Clear(1);
  display().EndFrame();

  EXPECT_EQ(ScreenColor(), (std::array<uint8_t, 3>{1, 129, 254}));
}

TEST_F(DisplayTest, ResetVideoModeTakesTheSurfaceAway) {
  display().ResetVideoMode();

  EXPECT_FALSE(display().Lock().has_value());
  EXPECT_EQ(display().palette(), nullptr);
}

TEST_F(DisplayTest, ShowsAScaledFrameInTheCurrentPalette) {
  display().SetPalette(PaletteWith(1, 63, 0, 0));
  display().PresentScaledFrame(std::array<uint8_t, 4>{1, 1, 1, 1}, 2, 2);
  EXPECT_EQ(ScreenColor(), (std::array<uint8_t, 3>{255, 0, 0}));
}

// The texture holds baked colors, so a palette-cycling screen (the mission
// map's pulsing hotspots) depends on a palette change converting them again.
TEST_F(DisplayTest, AScaledFrameFollowsALaterPaletteChange) {
  display().SetPalette(PaletteWith(1, 63, 0, 0));
  display().PresentScaledFrame(std::array<uint8_t, 4>{1, 1, 1, 1}, 2, 2);

  display().SetPalette(PaletteWith(1, 0, 63, 0));
  display().EndFrame();
  EXPECT_EQ(ScreenColor(), (std::array<uint8_t, 3>{0, 255, 0}));
}

// As soon as the game draws on the surface again, it replaces the frame.
TEST_F(DisplayTest, DrawingOnTheSurfaceDropsTheScaledFrame) {
  display().SetPalette(PaletteWith(1, 63, 0, 0));
  display().PresentScaledFrame(std::array<uint8_t, 4>{1, 1, 1, 1}, 2, 2);

  ASSERT_TRUE(display().Lock().has_value());
  display().Unlock();
  display().EndFrame();

  EXPECT_EQ(ScreenColor(), (std::array<uint8_t, 3>{0, 0, 0}));
}

// A backdrop is art stretched up to the window with captions drawn over it at
// full resolution: wherever the surface holds color 0, the backdrop shows.
TEST_F(DisplayTest, ABackdropShowsThroughColorZeroOfTheSurface) {
  display().SetPalette(PaletteWith(1, 63, 0, 0));
  display().PresentBackdrop(std::array<uint8_t, 4>{1, 1, 1, 1}, 2, 2);

  EXPECT_EQ(ScreenColor(), (std::array<uint8_t, 3>{255, 0, 0}));
}

// Drawing on the surface keeps a backdrop and is presented over it, at the end
// of the frame like any other drawing.
TEST_F(DisplayTest, TheSurfaceIsDrawnOverABackdrop) {
  PixelBuffer page;
  page.Init(2, 2, display());
  engine::gfx::Palette palette = PaletteWith(1, 63, 0, 0);
  palette.at(2) = engine::gfx::PaletteColor(0, 0, 255);
  display().SetPalette(palette);
  display().PresentBackdrop(std::array<uint8_t, 4>{1, 1, 1, 1}, 2, 2);

  page.view().Clear(2);
  EXPECT_TRUE(display().redraw_pending());
  display().EndFrame();

  EXPECT_EQ(ScreenColor(), (std::array<uint8_t, 3>{0, 0, 255}));
}

TEST_F(DisplayTest, DrawingColorZeroOnTheSurfaceKeepsABackdrop) {
  display().SetPalette(PaletteWith(1, 63, 0, 0));
  display().PresentBackdrop(std::array<uint8_t, 4>{1, 1, 1, 1}, 2, 2);

  ASSERT_TRUE(display().Lock().has_value());
  display().Unlock();
  display().EndFrame();

  EXPECT_EQ(ScreenColor(), (std::array<uint8_t, 3>{255, 0, 0}));
}

TEST_F(DisplayTest, ClearBackdropLeavesTheSurfaceAlone) {
  display().SetPalette(PaletteWith(1, 63, 0, 0));
  display().PresentBackdrop(std::array<uint8_t, 4>{1, 1, 1, 1}, 2, 2);

  display().ClearBackdrop();
  display().EndFrame();

  EXPECT_EQ(ScreenColor(), (std::array<uint8_t, 3>{0, 0, 0}));
}

// A presentation that ends while a movie is up must not take the movie's frame
// down with it.
TEST_F(DisplayTest, ClearBackdropLeavesAMovieFrame) {
  display().SetPalette(PaletteWith(1, 63, 0, 0));
  display().PresentScaledFrame(std::array<uint8_t, 4>{1, 1, 1, 1}, 2, 2);

  display().ClearBackdrop();
  display().EndFrame();

  EXPECT_EQ(ScreenColor(), (std::array<uint8_t, 3>{255, 0, 0}));
}

// A movie frame shows alone, even over a surface that a backdrop showed under.
TEST_F(DisplayTest, AMovieFrameReplacesABackdrop) {
  PixelBuffer page;
  page.Init(2, 2, display());
  engine::gfx::Palette palette = PaletteWith(1, 63, 0, 0);
  palette.at(2) = engine::gfx::PaletteColor(0, 0, 255);
  palette.at(3) = engine::gfx::PaletteColor(0, 255, 0);
  display().SetPalette(palette);
  display().PresentBackdrop(std::array<uint8_t, 4>{1, 1, 1, 1}, 2, 2);
  page.view().Clear(2);

  display().PresentScaledFrame(std::array<uint8_t, 4>{3, 3, 3, 3}, 2, 2);

  EXPECT_EQ(ScreenColor(), (std::array<uint8_t, 3>{0, 255, 0}));
}

// A frame with no pixels, or with fewer pixels than its size says, is
// rejected without presenting anything; the surface (all color 0, black)
// stays what an end of frame shows.
TEST_F(DisplayTest, RejectsFramesThatDoNotMatchTheirSize) {
  display().SetPalette(PaletteWith(1, 63, 0, 0));
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

// Nothing may depend on real time, so the next pump presents what the redraw
// timer would have, and no timer is armed.
TEST_F(ScriptedWindowDisplayTest, ThePumpPresentsWhatWasDrawn) {
  display().SetPalette(PaletteWith(1, 63, 0, 0));
  PixelBuffer page;
  page.Init(2, 2, display());

  page.view().Clear(1);
  EXPECT_TRUE(display().redraw_pending());
  EXPECT_EQ(ScreenColor(), (std::array<uint8_t, 3>{0, 0, 0}));

  display().PumpEvents();

  EXPECT_FALSE(display().redraw_pending());
  EXPECT_EQ(ScreenColor(), (std::array<uint8_t, 3>{255, 0, 0}));
}

TEST_F(ScriptedWindowDisplayTest, APumpWithNothingDrawnEndsNoFrame) {
  RecordingFrames frames;
  display().set_observer(&frames);

  display().PumpEvents();

  EXPECT_EQ(frames.ended, 0);
  display().set_observer(nullptr);
}

// The 70-a-second limit would make a scripted run take as long as playing.
TEST_F(ScriptedWindowDisplayTest, PresentingDoesNotWait) {
  const auto start = std::chrono::steady_clock::now();

  for (int frame = 0; frame < 200; ++frame) {
    display().EndFrame();
  }

  // 200 paced frames take close to three seconds.
  EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::seconds(1));
}

// Counts what the event loop hands it.
class RecordingClient final : public DisplayClient {
 public:
  void HandleEvent(const SDL_Event& event) override {
    ++events_seen;
    last_event_type = event.type;
  }
  void Pump() override { ++pumps_seen; }

  int events_seen = 0;
  uint32_t last_event_type = 0;
  int pumps_seen = 0;
};

void PushQuitEvent() {
  SDL_Event event{};
  event.type = SDL_QUIT;
  SDL_PushEvent(&event);
}

// Pumps a Display that has no window: the loop needs only SDL's event queue.
class DisplayEventsTest : public ::testing::Test {
 protected:
  void SetUp() override { SDL_Init(SDL_INIT_EVENTS); }

  RecordingClient client_;
};

TEST_F(DisplayEventsTest, HandsEventsToTheClient) {
  WindowDisplay display(client_);
  PushQuitEvent();
  display.PumpEvents();
  EXPECT_EQ(client_.events_seen, 1);
  EXPECT_EQ(client_.last_event_type, static_cast<uint32_t>(SDL_QUIT));
}

TEST_F(DisplayEventsTest, PumpsTheClientOncePerPass) {
  WindowDisplay display(client_);
  display.PumpEvents();
  display.PumpEvents();
  EXPECT_EQ(client_.pumps_seen, 2);
}

TEST_F(DisplayEventsTest, DrainsTheQueueWithoutAClient) {
  WindowDisplay display;
  PushQuitEvent();
  display.PumpEvents();
  EXPECT_EQ(
      SDL_PeepEvents(nullptr, 0, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT),
      0);
}

// Counts what a Display reports, and the client pumps it has seen by the
// time each frame ends.
class RecordingObserver final : public DisplayObserver {
 public:
  explicit RecordingObserver(
      const RecordingClient& client ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : client_(&client) {}

  void OnFrameEnd() override {
    ++frames_ended;
    client_pumps_at_last_frame_end = client_->pumps_seen;
  }
  void OnPump() override { ++pumps; }

  int frames_ended = 0;
  int pumps = 0;
  int client_pumps_at_last_frame_end = -1;

 private:
  const RecordingClient* client_;
};

// A headless video mode has to work in a process that never starts SDL, which
// is what a unit test of a dialog is, so none of these tests calls SDL_Init().
TEST(HeadlessDisplayTest, LendsASurfaceOfTheSizeAsked) {
  HeadlessDisplay display;
  ASSERT_TRUE(display.SetVideoMode(640, 400));

  const std::optional<PixelSurface::Pixels> pixels = display.Lock();
  ASSERT_TRUE(pixels.has_value());
  EXPECT_GE(pixels->pitch, 640);
  EXPECT_EQ(pixels->bytes.size(), static_cast<size_t>(pixels->pitch) * 400);
  display.Unlock();
}

TEST(HeadlessDisplayTest, KeepsWhatAPageDrawsOnIt) {
  HeadlessDisplay display;
  ASSERT_TRUE(display.SetVideoMode(8, 4));
  PixelBuffer page;
  page.Init(8, 4, display);

  page.view().Clear(7);
  display.EndFrame();

  const std::optional<PixelSurface::Pixels> pixels = display.Lock();
  ASSERT_TRUE(pixels.has_value());
  EXPECT_EQ(pixels->bytes.front(), 7);
  display.Unlock();
}

TEST(HeadlessDisplayTest, TakesThePalette) {
  HeadlessDisplay display;
  ASSERT_TRUE(display.SetVideoMode(8, 4));

  display.SetPalette(PaletteWith(1, 63, 0, 0));

  const auto* palette = static_cast<const SDL_Palette*>(display.palette());
  ASSERT_NE(palette, nullptr);
  ASSERT_EQ(palette->ncolors, 256);
  // SDL owns exactly ncolors entries in the palette.
  const auto colors =
      // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
      std::span(palette->colors, static_cast<size_t>(palette->ncolors));
  EXPECT_EQ(base::At(colors, 1).r, 255);
}

TEST(HeadlessDisplayTest, EndingAFrameTellsTheObserverThenPumpsTheClient) {
  RecordingClient client;
  RecordingObserver observer(client);
  HeadlessDisplay display(client);
  ASSERT_TRUE(display.SetVideoMode(8, 4));
  display.set_observer(&observer);

  display.EndFrame();
  display.EndFrame();

  EXPECT_EQ(observer.frames_ended, 2);
  EXPECT_EQ(client.pumps_seen, 2);
  // The second frame ended after the first frame's pump and before its own.
  EXPECT_EQ(observer.client_pumps_at_last_frame_end, 1);
}

TEST(HeadlessDisplayTest, DrawingAloneEndsNoFrame) {
  RecordingClient client;
  RecordingObserver observer(client);
  HeadlessDisplay display(client);
  ASSERT_TRUE(display.SetVideoMode(8, 4));
  display.set_observer(&observer);

  ASSERT_TRUE(display.Lock().has_value());
  display.Unlock();
  display.SetPalette(PaletteWith(1, 63, 0, 0));

  EXPECT_EQ(observer.frames_ended, 0);
  EXPECT_FALSE(display.redraw_pending());
}

TEST(HeadlessDisplayTest, AScaledFrameOnlyEndsTheFrame) {
  RecordingClient client;
  RecordingObserver observer(client);
  HeadlessDisplay display(client);
  ASSERT_TRUE(display.SetVideoMode(8, 4));
  display.set_observer(&observer);

  display.PresentScaledFrame(std::array<uint8_t, 4>{1, 1, 1, 1}, 2, 2);
  display.PresentBackdrop(std::array<uint8_t, 4>{1, 1, 1, 1}, 2, 2);

  EXPECT_EQ(observer.frames_ended, 2);
}

TEST(HeadlessDisplayTest, APumpWithoutAFrameIsReportedAsOne) {
  RecordingClient client;
  RecordingObserver observer(client);
  HeadlessDisplay display(client);
  ASSERT_TRUE(display.SetVideoMode(8, 4));
  display.set_observer(&observer);

  display.PumpEvents();

  EXPECT_EQ(observer.pumps, 1);
  EXPECT_EQ(observer.frames_ended, 0);
  EXPECT_EQ(client.pumps_seen, 1);
}

TEST(HeadlessDisplayTest, ADetachedObserverHearsNothingMore) {
  RecordingClient client;
  RecordingObserver observer(client);
  HeadlessDisplay display(client);
  ASSERT_TRUE(display.SetVideoMode(8, 4));
  display.set_observer(&observer);
  display.EndFrame();

  display.set_observer(nullptr);
  display.EndFrame();

  EXPECT_EQ(observer.frames_ended, 1);
}

TEST(HeadlessDisplayTest, HasTheInputFocus) {
  HeadlessDisplay display;
  ASSERT_TRUE(display.SetVideoMode(8, 4));

  EXPECT_TRUE(display.HasInputFocus());
}

TEST(HeadlessDisplayTest, ResettingTheModeTakesTheSurfaceAway) {
  HeadlessDisplay display;
  ASSERT_TRUE(display.SetVideoMode(8, 4));

  display.ResetVideoMode();

  EXPECT_FALSE(display.Lock().has_value());
}

TEST(MakeDisplayTest, MakesTheKindAsked) {
  RecordingClient client;

  const std::unique_ptr<Display> window =
      MakeDisplay(DisplayKind::kWindow, client);
  const std::unique_ptr<Display> scripted =
      MakeDisplay(DisplayKind::kScriptedWindow, client);
  const std::unique_ptr<Display> headless =
      MakeDisplay(DisplayKind::kHeadless, client);

  EXPECT_NE(dynamic_cast<WindowDisplay*>(window.get()), nullptr);
  EXPECT_EQ(dynamic_cast<ScriptedWindowDisplay*>(window.get()), nullptr);
  EXPECT_NE(dynamic_cast<ScriptedWindowDisplay*>(scripted.get()), nullptr);
  EXPECT_NE(dynamic_cast<HeadlessDisplay*>(headless.get()), nullptr);
}

TEST(MakeDisplayTest, GivesTheDisplayItsClient) {
  RecordingClient client;
  const std::unique_ptr<Display> display =
      MakeDisplay(DisplayKind::kHeadless, client);

  display->PumpEvents();

  EXPECT_EQ(client.pumps_seen, 1);
}

// A window never opened has no window to open either way.
TEST(HeadlessDisplayTest, NeedsNoWindowOpened) {
  HeadlessDisplay display;

  EXPECT_TRUE(display.Init("title", 640, 400));
  EXPECT_EQ(display.renderer(), nullptr);
  EXPECT_EQ(display.PictureScale(), 1.0F);
  EXPECT_FALSE(display.redraw_pending());
}

TEST(HeadlessDisplayTest, EndsNoFrameBeforeAVideoMode) {
  RecordingClient client;
  RecordingObserver observer(client);
  HeadlessDisplay display(client);
  display.set_observer(&observer);

  display.EndFrame();
  display.PresentScaledFrame(std::array<uint8_t, 4>{1, 1, 1, 1}, 2, 2);

  EXPECT_EQ(observer.frames_ended, 0);
  EXPECT_EQ(display.palette(), nullptr);
}

}  // namespace
}  // namespace engine::window
