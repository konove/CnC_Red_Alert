// Tests for opening and animating WSA animations, read from an in-memory image
// of the file.

#include "tech/wsa_animation.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <vector>

#include "gtest/gtest.h"
#include "sdllib/gbuffer.h"
#include "sdllib/ww_win.h"
#include "tech/memory_file.h"

// ww_win.cc, pulled in through gbuffer, dispatches events to the app.
void SDL_Event_Handler(SDL_Event* /*event*/) {}

namespace {

// LLVM 23 mistakes element invalidation for invalidating the vector reference;
// no element reference or iterator is retained across these appends.
// NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-invalidation)
void PutUint16(std::vector<char>& out, int value) {
  const auto bits = static_cast<uint32_t>(value);
  out.push_back(static_cast<char>(bits & 0xff));
  out.push_back(static_cast<char>((bits >> 8) & 0xff));
}

// LLVM 23 mistakes element invalidation for invalidating the vector reference;
// no element reference or iterator is retained across these appends.
// NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-invalidation)
void PutUint32(std::vector<char>& out, int64_t value) {
  const auto bits = static_cast<uint64_t>(value);
  for (unsigned shift = 0; shift < 32; shift += 8) {
    out.push_back(static_cast<char>((bits >> shift) & 0xff));
  }
}

// Loads the WSA file in `image`.
WsaAnimation LoadWsa(std::vector<char> image) {
  MemoryFile file(std::as_writable_bytes(std::span(image)));
  file.Open();
  return WsaAnimation(file);
}

// Builds a one-frame WSA of the given size, whose frame 0 is `frame_size` LCW
// end markers and so decodes to nothing. largest_frame_size is the raw header
// value, which includes the 37 bytes ANIMATE adds.
std::vector<char> MakeWsa(int width, int height, int largest_frame_size = 256,
                          int frame_size = 4) {
  constexpr int kHeaderSize = 14;
  constexpr int kOffsetsSize = 3 * 4;  // total_frames + 2 offsets.
  const int frame0 = kHeaderSize + kOffsetsSize;

  std::vector<char> out;
  PutUint16(out, 1);       // total_frames
  PutUint16(out, 0);       // pixel_x
  PutUint16(out, 0);       // pixel_y
  PutUint16(out, width);   // pixel_width
  PutUint16(out, height);  // pixel_height
  PutUint16(out, largest_frame_size);
  PutUint16(out, 0);       // flags
  PutUint32(out, frame0);  // frame 0 offset
  PutUint32(out, frame0 + frame_size);
  PutUint32(out, 0);  // no loop frame
  out.insert(out.end(), static_cast<size_t>(frame_size), '\x80');
  return out;
}

TEST(WsaTest, ClosedAnimationDrawsNothing) {
  WsaAnimation animation("MISSING.WSA");
  EXPECT_FALSE(animation.is_open());
  EXPECT_EQ(animation.frame_count(), 0);

  std::vector<uint8_t> page(size_t{16} * 16, '\x5a');
  GraphicBufferClass view(16, 16, page);
  EXPECT_FALSE(animation.DrawFrame(view, 0));
  EXPECT_EQ(std::count(page.begin(), page.end(), '\x5a'), std::ssize(page));
}

TEST(WsaTest, CloseMakesAnimationInert) {
  WsaAnimation animation = LoadWsa(MakeWsa(16, 16));
  ASSERT_TRUE(animation.is_open());

  animation.Close();
  EXPECT_FALSE(animation.is_open());
  EXPECT_EQ(animation.frame_count(), 0);

  std::vector<uint8_t> page(size_t{16} * 16, '\0');
  GraphicBufferClass view(16, 16, page);
  EXPECT_FALSE(animation.DrawFrame(view, 0));
}

TEST(WsaTest, OpenRejectsFirstFrameLargerThanDeltaBuffer) {
  // largest_frame_size 40 leaves room for 3 bytes of frame data after the 37
  // header bytes ANIMATE counts, but frame 0 is 16 bytes.
  EXPECT_FALSE(LoadWsa(MakeWsa(16, 16, 40, 16)).is_open());
}

TEST(WsaTest, OpenRejectsLargestFrameSmallerThanAnimateHeader) {
  // Before the check, 10 - 37 wrapped to 65509 and frame 0 was read that far
  // past the delta buffer.
  EXPECT_FALSE(LoadWsa(MakeWsa(16, 16, 10, 4)).is_open());
}

// Builds a 16x16 two-frame WSA. Frame 0 is on the page; the offset table
// claims frame 1 is `claimed_size` bytes, and the file stores that many bytes
// of 0xee after the table.
std::vector<char> MakeTwoFrameWsa(int claimed_size) {
  constexpr int kHeaderSize = 14;
  constexpr int kOffsetsSize = 4 * 4;  // total_frames + 2 offsets.
  const int frame1 = kHeaderSize + kOffsetsSize;

  std::vector<char> out;
  PutUint16(out, 2);    // total_frames
  PutUint16(out, 0);    // pixel_x
  PutUint16(out, 0);    // pixel_y
  PutUint16(out, 16);   // pixel_width
  PutUint16(out, 16);   // pixel_height
  PutUint16(out, 256);  // largest_frame_size: 219 bytes of frame data.
  PutUint16(out, 0);    // flags
  PutUint32(out, 0);    // Frame 0 is on the page.
  PutUint32(out, frame1);
  PutUint32(out, frame1 + claimed_size);
  PutUint32(out, 0);  // No loop frame.
  out.insert(out.end(), static_cast<size_t>(claimed_size), '\xee');
  return out;
}

// Run under ASan, this also catches the frame landing outside the delta buffer.
TEST(WsaTest, AnimateRejectsOversizedFrame) {
  WsaAnimation animation = LoadWsa(MakeTwoFrameWsa(600));
  ASSERT_TRUE(animation.is_open());

  std::vector<uint8_t> page(size_t{16} * 16, '\0');
  GraphicBufferClass view(16, 16, page);
  animation.DrawFrame(view, 1);

  EXPECT_EQ(std::count(page.begin(), page.end(), '\xee'), 0);
}

// Returns a delta that XORs the first pixel of the frame with `value`, LCW
// "compressed" as one five-byte literal and the end marker.
std::vector<uint8_t> XorFirstPixel(uint8_t value) {
  return {0x85, 1, value, 0x80, 0, 0, 0x80};
}

// Builds a 4x1 animation with no loop frame. Frame 0 is on the page and
// `deltas` produce frames 1 and up. With `palette` set, a 768-byte palette
// follows the offset table; the table's offsets do not count it.
std::vector<char> MakeDeltaWsa(const std::vector<std::vector<uint8_t>>& deltas,
                               bool palette) {
  constexpr int kHeaderSize = 14;
  const int total_frames = static_cast<int>(deltas.size()) + 1;
  int offset = kHeaderSize + ((total_frames + 2) * 4);

  std::vector<char> out;
  PutUint16(out, total_frames);
  PutUint16(out, 0);    // pixel_x
  PutUint16(out, 0);    // pixel_y
  PutUint16(out, 4);    // pixel_width
  PutUint16(out, 1);    // pixel_height
  PutUint16(out, 256);  // largest_frame_size: 219 bytes of frame data.
  PutUint16(out, palette ? 1 : 0);  // flags
  PutUint32(out, 0);                // Frame 0 is on the page.
  for (const auto& delta : deltas) {
    PutUint32(out, offset);
    offset += static_cast<int>(delta.size());
  }
  PutUint32(out, offset);  // End of the last frame.
  PutUint32(out, 0);       // No loop frame.
  if (palette) {
    out.insert(out.end(), 768, '\0');
  }
  for (const auto& delta : deltas) {
    out.insert(out.end(), delta.begin(), delta.end());
  }
  return out;
}

TEST(WsaTest, AnimationWithPaletteDoesNotWrapWithoutLoopFrame) {
  WsaAnimation animation =
      LoadWsa(MakeDeltaWsa({XorFirstPixel(0x01), XorFirstPixel(0x02)},
                           /*palette=*/true));
  ASSERT_TRUE(animation.is_open());

  std::vector<uint8_t> page(4, 0);
  GraphicBufferClass view(4, 1, page);
  ASSERT_TRUE(animation.DrawFrame(view, 0));

  // Going backwards through a loop frame would reach frame 2 in one step, but
  // there is none, so both deltas have to be applied.
  EXPECT_TRUE(animation.DrawFrame(view, 2));
  EXPECT_EQ(page.front(), 0x03);
}

TEST(WsaTest, AnimateStopsAtFrameItCannotLoad) {
  // Frame 2 claims more data than the delta buffer holds.
  WsaAnimation animation = LoadWsa(
      MakeDeltaWsa({XorFirstPixel(0x01), std::vector<uint8_t>(600, 0xee)},
                   /*palette=*/false));
  ASSERT_TRUE(animation.is_open());

  std::vector<uint8_t> page(4, 0);
  GraphicBufferClass view(4, 1, page);
  EXPECT_FALSE(animation.DrawFrame(view, 2));
  EXPECT_EQ(page.front(), 0x01);

  // The animation knows it is showing frame 1, so asking for it is no work.
  EXPECT_TRUE(animation.DrawFrame(view, 1));
  EXPECT_EQ(page.front(), 0x01);
}

}  // namespace
