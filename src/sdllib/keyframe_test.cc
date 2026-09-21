#include "ra/keyframe.h"

#include <SDL_events.h>
#include <SDL_pixels.h>
#include <SDL_render.h>
#include <SDL_surface.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

#include "base/installed.h"
#include "gtest/gtest.h"
#include "port/unaligned.h"
#include "sdllib/display.h"
#include "sdllib/pixel_buffer.h"
#include "sdllib/ww_win.h"

// The SDL event loop delegates to the application; these decoder tests do not
// pump events or draw into game windows.
void SDL_Event_Handler(SDL_Event* /*event*/) {}

namespace {
std::vector<std::byte> FrameFile() {
  // Three 2x2 frames: an LCW key frame, a key delta, and a chained delta.
  std::vector<std::byte> data(64);
  port::WriteUnaligned<uint16_t>(data, 3);
  port::WriteUnaligned<uint16_t>(std::span(data).subspan(6), 2);
  port::WriteUnaligned<uint16_t>(std::span(data).subspan(8), 2);
  port::WriteUnaligned<uint32_t>(std::span(data).subspan(14), 0x80000028U);
  port::WriteUnaligned<uint32_t>(std::span(data).subspan(22), 0x4000002eU);
  port::WriteUnaligned<uint32_t>(std::span(data).subspan(26), 40);
  port::WriteUnaligned<uint32_t>(std::span(data).subspan(30), 0x20000036U);
  port::WriteUnaligned<uint32_t>(std::span(data).subspan(34), 1);
  const std::array<uint8_t, 22> stream{
      0x84, 1, 2, 3, 4, 0x80,         // LCW literal and stop.
      4,    1, 1, 1, 1, 0x80, 0, 0,   // XOR key delta.
      4,    2, 2, 2, 2, 0x80, 0, 0};  // XOR chained delta.
  for (size_t i = 0; i < stream.size(); ++i) {
    data.at(40 + i) = std::byte{stream.at(i)};
  }
  return data;
}

TEST(KeyFrameBoundsTest, BuildsKeyAndChainedDeltaFrames) {
  const auto data = FrameFile();
  std::array<uint8_t, 6> output{};
  output.fill(0xa5);
  ASSERT_EQ(Build_Frame(data, 0, output).size(), 4);
  EXPECT_EQ(output, (std::array<uint8_t, 6>{1, 2, 3, 4, 0xa5, 0xa5}));
  ASSERT_EQ(Build_Frame(data, 1, output).size(), 4);
  EXPECT_EQ(output, (std::array<uint8_t, 6>{0, 3, 2, 5, 0xa5, 0xa5}));
  ASSERT_EQ(Build_Frame(data, 2, output).size(), 4);
  EXPECT_EQ(output, (std::array<uint8_t, 6>{2, 1, 0, 7, 0xa5, 0xa5}));
}

TEST(KeyFrameBoundsTest, RejectsSmallDestinationsAndInvalidOffsets) {
  auto data = FrameFile();
  std::array<uint8_t, 3> output{0xa5, 0xa5, 0xa5};
  EXPECT_TRUE(Build_Frame(data, 0, output).empty());
  EXPECT_EQ(output.front(), 0xa5);
  std::array<uint8_t, 4> full{};
  EXPECT_TRUE(Build_Frame(std::span(data).first(15), 0, full).empty());
  port::WriteUnaligned<uint32_t>(std::span(data).subspan(14), 0x80ffffffU);
  EXPECT_TRUE(Build_Frame(data, 0, full).empty());
}
// Invalid frames must return before touching a pending timer or SDL surface.
// A plain surface record suffices because rejected frames never access it.
class RejectedFrameBuffer : public PixelBuffer {
 public:
  explicit RejectedFrameBuffer(bool have_surface) {
    palette_surface_ = have_surface ? &surface_ : nullptr;
    redraw_timer_ = 1;
  }
  // The surface and timer are fakes; the base destructor must not release
  // them.
  ~RejectedFrameBuffer() {
    palette_surface_ = nullptr;
    redraw_timer_ = 0;
  }
  RejectedFrameBuffer(const RejectedFrameBuffer&) = delete;
  RejectedFrameBuffer& operator=(const RejectedFrameBuffer&) = delete;
  RejectedFrameBuffer(RejectedFrameBuffer&&) = delete;
  RejectedFrameBuffer& operator=(RejectedFrameBuffer&&) = delete;
  [[nodiscard]] int PendingTimer() const { return redraw_timer_; }

 private:
  SDL_Surface surface_{};
};

TEST(GraphicBufferRenderTest, RejectsInvalidDimensionsBeforeSdlAccess) {
  RejectedFrameBuffer buffer(true);
  const std::array<uint8_t, 4> pixels{};
  buffer.PresentScaledFrame(pixels, 0, 2);
  buffer.PresentScaledFrame(pixels, 2, 0);
  buffer.PresentScaledFrame(pixels, -1, 2);
  buffer.PresentScaledFrame(pixels, 2, -1);
  EXPECT_EQ(buffer.PendingTimer(), 1);
}

TEST(GraphicBufferRenderTest, RejectsShortFramesAndDimensionOverflow) {
  RejectedFrameBuffer buffer(true);
  const std::array<uint8_t, 3> pixels{};
  buffer.PresentScaledFrame(pixels, 2, 2);
  buffer.PresentScaledFrame(pixels, std::numeric_limits<int>::max(),
                            std::numeric_limits<int>::max());
  EXPECT_EQ(buffer.PendingTimer(), 1);
}

TEST(GraphicBufferRenderTest, RejectsMissingDisplaySurfaceBeforeSdlAccess) {
  RejectedFrameBuffer buffer(false);
  const std::array<uint8_t, 4> pixels{};
  buffer.PresentScaledFrame(pixels, 2, 2);
  EXPECT_EQ(buffer.PendingTimer(), 1);
}
// Draws scaled frames through a software renderer onto a 2x2 target, so the
// colors that reach the screen can be read back without a window.
class ScaledFrameTest : public ::testing::Test {
 protected:
  class Buffer : public PixelBuffer {
   public:
    // The base destructor frees the surface and the scaled-frame texture.
    Buffer() {
      palette_surface_ = SDL_CreateRGBSurface(0, 2, 2, 8, 0, 0, 0, 0);
    }
  };

  void SetUp() override {
    target_ =
        SDL_CreateRGBSurfaceWithFormat(0, 2, 2, 32, SDL_PIXELFORMAT_RGBA32);
    ASSERT_NE(target_, nullptr);
    renderer_ = SDL_CreateSoftwareRenderer(target_);
    ASSERT_NE(renderer_, nullptr);
    // The Display borrows the renderer; TearDown() still owns it.
    display_.emplace(nullptr, renderer_);
    display_scope_.emplace(*display_);
  }

  void TearDown() override {
    display_scope_.reset();
    display_.reset();
    SDL_DestroyRenderer(renderer_);
    renderer_ = nullptr;
    SDL_FreeSurface(target_);
  }

  // Returns a 6-bit VGA palette that is black but for `color` at `index`.
  static std::array<uint8_t, 768> PaletteWith(size_t index, uint8_t red,
                                              uint8_t green, uint8_t blue) {
    std::array<uint8_t, 768> palette{};
    palette.at(index * 3) = red;
    palette.at((index * 3) + 1) = green;
    palette.at((index * 3) + 2) = blue;
    return palette;
  }

  // Returns the top-left pixel on screen as {red, green, blue}.
  [[nodiscard]] static std::array<uint8_t, 3> ScreenColor() {
    std::array<uint8_t, 16> pixels{};
    EXPECT_EQ(SDL_RenderReadPixels(
                  static_cast<SDL_Renderer*>(TheDisplay().renderer()), nullptr,
                  SDL_PIXELFORMAT_RGBA32, pixels.data(), 8),
              0);
    return {pixels.at(0), pixels.at(1), pixels.at(2)};
  }

  SDL_Surface* target_ = nullptr;

  SDL_Renderer* renderer_ = nullptr;

  std::optional<Display> display_;

  std::optional<base::Installed<Display>::Scope> display_scope_;
};

TEST_F(ScaledFrameTest, ShowsTheFrameInTheCurrentPalette) {
  Buffer buffer;
  buffer.UpdatePalette(PaletteWith(1, 63, 0, 0));
  buffer.PresentScaledFrame(std::array<uint8_t, 4>{1, 1, 1, 1}, 2, 2);
  EXPECT_EQ(ScreenColor(), (std::array<uint8_t, 3>{255, 0, 0}));
}

// The texture holds baked colors, so a palette-cycling screen (the mission
// map's pulsing hotspots) depends on a palette change converting them again.
TEST_F(ScaledFrameTest, FollowsALaterPaletteChange) {
  Buffer buffer;
  buffer.UpdatePalette(PaletteWith(1, 63, 0, 0));
  buffer.PresentScaledFrame(std::array<uint8_t, 4>{1, 1, 1, 1}, 2, 2);

  buffer.UpdatePalette(PaletteWith(1, 0, 63, 0));
  buffer.Present(/*end_frame=*/true);
  EXPECT_EQ(ScreenColor(), (std::array<uint8_t, 3>{0, 255, 0}));
}
}  // namespace
