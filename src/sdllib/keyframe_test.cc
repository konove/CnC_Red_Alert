#include "ra/keyframe.h"

#include <SDL_events.h>
#include <SDL_surface.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string_view>
#include <vector>

#include "gtest/gtest.h"
#include "port/unaligned.h"
#include "sdllib/file.h"
#include "sdllib/file_access.h"
#include "sdllib/gbuffer.h"
#include "sdllib/ww_win.h"

// The SDL event loop delegates to the application; these decoder tests do not
// pump events or draw into game windows.
int WindowList[7][8]{};
void SDL_Event_Handler(SDL_Event* /*event*/) {}
int OpenFileHandle(std::string_view /*name*/, FileAccess /*mode*/) {
  return -1;
}
void CloseFileHandle(int /*handle*/) {}
int32_t ReadFileHandle(int /*handle*/, std::span<std::byte> /*buffer*/) {
  return 0;
}
int32_t SeekFileHandle(int /*handle*/, int32_t /*offset*/, int /*origin*/) {
  return -1;
}

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
class RejectedFrameBuffer : public GraphicBufferClass {
 public:
  explicit RejectedFrameBuffer(bool have_surface) {
    PaletteSurface = have_surface ? &surface_ : nullptr;
    RedrawTimer = 1;
  }
  [[nodiscard]] int PendingTimer() const { return RedrawTimer; }

 private:
  SDL_Surface surface_{};
};

TEST(GraphicBufferRenderTest, RejectsInvalidDimensionsBeforeSdlAccess) {
  RejectedFrameBuffer buffer(true);
  const std::array<uint8_t, 4> pixels{};
  buffer.Render_Scaled_Frame(pixels, 0, 2);
  buffer.Render_Scaled_Frame(pixels, 2, 0);
  buffer.Render_Scaled_Frame(pixels, -1, 2);
  buffer.Render_Scaled_Frame(pixels, 2, -1);
  EXPECT_EQ(buffer.PendingTimer(), 1);
}

TEST(GraphicBufferRenderTest, RejectsShortFramesAndDimensionOverflow) {
  RejectedFrameBuffer buffer(true);
  const std::array<uint8_t, 3> pixels{};
  buffer.Render_Scaled_Frame(pixels, 2, 2);
  buffer.Render_Scaled_Frame(pixels, std::numeric_limits<int>::max(),
                             std::numeric_limits<int>::max());
  EXPECT_EQ(buffer.PendingTimer(), 1);
}

TEST(GraphicBufferRenderTest, RejectsMissingDisplaySurfaceBeforeSdlAccess) {
  RejectedFrameBuffer buffer(false);
  const std::array<uint8_t, 4> pixels{};
  buffer.Render_Scaled_Frame(pixels, 2, 2);
  EXPECT_EQ(buffer.PendingTimer(), 1);
}
}  // namespace
