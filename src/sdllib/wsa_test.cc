// Tests for opening and animating WSA animations. The game normally provides
// the file layer; an in-memory image stands in for it.

#include "sdllib/wsa.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <span>
#include <string_view>
#include <vector>

#include "base/buffer.h"
#include "gtest/gtest.h"
#include "sdllib/file.h"
#include "sdllib/file_access.h"
#include "sdllib/gbuffer.h"
#include "sdllib/iff.h"
#include "sdllib/ww_win.h"

namespace {

// Contents of the single file the stubs below serve, and the read cursor.
std::vector<char> file_image;
int64_t file_pos = 0;
// Handles opened and not yet closed.
int open_handles = 0;

}  // namespace

// Link-time stubs for the file layer the game supplies.
// Every name but "MISSING.WSA" opens the file image.
int OpenFileHandle(std::string_view file_name, FileAccess /*mode*/) {
  if (file_name == "MISSING.WSA") {
    return -1;
  }
  file_pos = 0;
  ++open_handles;
  return 1;
}

void CloseFileHandle(int /*handle*/) { --open_handles; }

int32_t ReadFileHandle(int /*handle*/, std::span<std::byte> buffer) {
  const int64_t available =
      std::max<int64_t>(0, std::ssize(file_image) - file_pos);
  const int64_t count =
      std::min(static_cast<int64_t>(buffer.size()), available);
  base::CopyBytes(
      buffer,
      std::as_bytes(
          std::span(file_image).subspan(static_cast<size_t>(file_pos))),
      count);
  file_pos += count;
  return static_cast<int32_t>(count);
}

int32_t SeekFileHandle(int /*handle*/, int32_t offset, int origin) {
  if (origin == SEEK_SET) {
    file_pos = offset;
  } else if (origin == SEEK_CUR) {
    file_pos += offset;
  } else {
    file_pos = std::ssize(file_image) + offset;
  }
  return static_cast<int32_t>(file_pos);
}

// ww_win.cc, pulled in through gbuffer, dispatches events to the app.
void SDL_Event_Handler(SDL_Event* /*event*/) {}

// LCW decoding is not under test: frames are "stored", so the compressed bytes
// are the delta itself. `source` is the back of `dest`, which a forward copy
// handles.
int32_t LCW_Uncompress(std::span<const std::byte> source,
                       std::span<std::byte> dest) {
  if (source.size() > dest.size()) {
    return 0;
  }
  std::ranges::copy(source, dest.begin());
  return static_cast<int32_t>(source.size());
}

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

// Builds a one-frame WSA of the given size. largest_frame_size is the raw
// header value, which includes the 37 bytes ANIMATE adds.
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
  out.insert(out.end(), static_cast<size_t>(frame_size), '\0');
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
  file_image = MakeWsa(16, 16);
  WsaAnimation animation("TEST.WSA");
  ASSERT_TRUE(animation.is_open());

  animation.Close();
  EXPECT_FALSE(animation.is_open());
  EXPECT_EQ(animation.frame_count(), 0);

  std::vector<uint8_t> page(size_t{16} * 16, '\0');
  GraphicBufferClass view(16, 16, page);
  EXPECT_FALSE(animation.DrawFrame(view, 0));
}

TEST(WsaTest, ClosesFileAfterLoading) {
  file_image = MakeWsa(16, 16);
  open_handles = 0;
  EXPECT_TRUE(WsaAnimation("TEST.WSA").is_open());
  EXPECT_EQ(open_handles, 0);

  // A rejected file is closed again too.
  file_image = MakeWsa(16, 16, 10, 4);
  EXPECT_FALSE(WsaAnimation("TEST.WSA").is_open());
  EXPECT_EQ(open_handles, 0);
}

TEST(WsaTest, OpenRejectsFirstFrameLargerThanDeltaBuffer) {
  // largest_frame_size 40 leaves room for 3 bytes of frame data after the 37
  // header bytes ANIMATE counts, but frame 0 is 16 bytes.
  file_image = MakeWsa(16, 16, 40, 16);

  EXPECT_FALSE(WsaAnimation("TEST.WSA").is_open());
}

TEST(WsaTest, OpenRejectsLargestFrameSmallerThanAnimateHeader) {
  // Before the check, 10 - 37 wrapped to 65509 and frame 0 was read that far
  // past the delta buffer.
  file_image = MakeWsa(16, 16, 10, 4);

  EXPECT_FALSE(WsaAnimation("TEST.WSA").is_open());
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
  file_image = MakeTwoFrameWsa(600);

  WsaAnimation animation("TEST.WSA");
  ASSERT_TRUE(animation.is_open());

  std::vector<uint8_t> page(size_t{16} * 16, '\0');
  GraphicBufferClass view(16, 16, page);
  animation.DrawFrame(view, 1);

  EXPECT_EQ(std::count(page.begin(), page.end(), '\xee'), 0);
}

// Returns a delta that XORs the first pixel of the frame with `value`.
std::vector<uint8_t> XorFirstPixel(uint8_t value) {
  return {1, value, 0x80, 0, 0};
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
  file_image = MakeDeltaWsa({XorFirstPixel(0x01), XorFirstPixel(0x02)},
                            /*palette=*/true);

  WsaAnimation animation("TEST.WSA");
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
  file_image =
      MakeDeltaWsa({XorFirstPixel(0x01), std::vector<uint8_t>(600, 0xee)},
                   /*palette=*/false);

  WsaAnimation animation("TEST.WSA");
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

TEST(WsaDeltaTest, BoundsRunsSkipsAndTruncatedCommands) {
  std::array<uint8_t, 6> output{1, 2, 3, 4, 5, 6};
  const std::array<uint8_t, 6> run{0, 3, 0x10, 0x80, 0, 0};
  ApplyXorDelta(std::span(output).first(3), std::as_bytes(std::span(run)));
  EXPECT_EQ(output, (std::array<uint8_t, 6>{0x11, 0x12, 0x13, 4, 5, 6}));
  const auto before = output;
  const std::array<uint8_t, 3> excessive_run{0, 7, 0xff};
  ApplyXorDelta(output, std::as_bytes(std::span(excessive_run)));
  EXPECT_EQ(output, before);
  const std::array<uint8_t, 2> truncated{0x80, 0};
  ApplyXorDelta(output, std::as_bytes(std::span(truncated)));
  EXPECT_EQ(output, before);
  const std::array<uint8_t, 4> skip{0x86, 1, 0xff, 0};
  ApplyXorDelta(output, std::as_bytes(std::span(skip)));
  EXPECT_EQ(output, before);
}

TEST(WsaDeltaTest, CopiesRowsWithoutTouchingPadding) {
  std::array<uint8_t, 8> output{};
  output.fill(0xa5);
  const std::array<uint8_t, 8> literal{4, 1, 2, 3, 4, 0x80, 0, 0};
  ApplyXorDeltaToView(output, std::as_bytes(std::span(literal)), 2, 4,
                      /*copy=*/true);
  EXPECT_EQ(output,
            (std::array<uint8_t, 8>{1, 2, 0xa5, 0xa5, 3, 4, 0xa5, 0xa5}));
}

TEST(WsaDeltaTest, XorsRowsWithoutTouchingPadding) {
  std::array<uint8_t, 8> output{};
  output.fill(0xa5);
  const std::array<uint8_t, 8> literal{4, 1, 2, 3, 4, 0x80, 0, 0};
  ApplyXorDeltaToView(output, std::as_bytes(std::span(literal)), 2, 4,
                      /*copy=*/false);
  EXPECT_EQ(output, (std::array<uint8_t, 8>{0xa4, 0xa7, 0xa5, 0xa5, 0xa6, 0xa1,
                                            0xa5, 0xa5}));
}
