// Tests for opening WSA animations into a caller-supplied buffer. The game
// normally provides the file layer; an in-memory image stands in for it.

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

}  // namespace

// Link-time stubs for the file layer the game supplies.
int OpenFileHandle(std::string_view /*file_name*/, FileAccess /*mode*/) {
  file_pos = 0;
  return 1;
}

void CloseFileHandle(int /*handle*/) {}

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

// Frame 0 decoding is not under test; leave the delta buffer alone.
int32_t LCW_Uncompress(std::span<const unsigned char> /*source*/,
                       std::span<unsigned char> /*dest*/) {
  return 0;
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

TEST(WsaTest, OpenClearsWholeTargetBufferForLargeFrames) {
  constexpr int kWidth = 320;
  constexpr int kHeight = 240;
  file_image = MakeWsa(kWidth, kHeight);

  // Generously sized; Open_Animation only needs the header, target and delta
  // buffers. new[] storage satisfies SysAnimHeaderType's alignment.
  constexpr int kBufferSize = 128 + (kWidth * kHeight) + 1024;
  std::vector<uint8_t> buffer(kBufferSize, '\x5a');

  void* handle =
      Open_Animation("TEST.WSA", buffer, kBufferSize, WSA_OPEN_FROM_DISK);
  ASSERT_NE(handle, nullptr);

  // The target frame starts right after the system header (under 64 bytes)
  // and runs for width * height bytes. The old 16-bit clear stopped after
  // 76800 % 65536 bytes.
  constexpr std::ptrdiff_t kHeaderBound = 128;
  constexpr std::ptrdiff_t kFrameBytes = std::ptrdiff_t{kWidth} * kHeight;
  const auto dirty = std::find_if(
      buffer.begin() + kHeaderBound, buffer.begin() + kFrameBytes,
      [](char byte) { return byte != '\0'; });
  EXPECT_EQ(dirty - buffer.begin(), kFrameBytes);

  Close_Animation(handle);
}


TEST(WsaTest, OpenRejectsFirstFrameLargerThanDeltaBuffer) {
  // largest_frame_size 40 leaves room for 3 bytes of frame data after the 37
  // header bytes ANIMATE counts, but frame 0 is 16 bytes.
  file_image = MakeWsa(16, 16, 40, 16);
  std::vector<uint8_t> buffer(1024, '\0');

  EXPECT_EQ(Open_Animation("TEST.WSA", buffer, 1024, WSA_OPEN_FROM_DISK),
            nullptr);
}

TEST(WsaTest, OpenRejectsLargestFrameSmallerThanAnimateHeader) {
  // Before the check, 10 - 37 wrapped to 65509 and frame 0 was read that far
  // past the delta buffer.
  file_image = MakeWsa(16, 16, 10, 4);
  std::vector<uint8_t> buffer(1024, '\0');

  EXPECT_EQ(Open_Animation("TEST.WSA", buffer, 1024, WSA_OPEN_FROM_DISK),
            nullptr);
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

// Opens the corrupt animation in `buffer_size` bytes surrounded by guard
// bytes, animates to frame 1, and checks that the oversized frame landed
// neither in front of the delta buffer nor outside the caller's buffer.
void ExpectOversizedFrameIsNotLoaded(int32_t buffer_size, WSAOpenType flags) {
  file_image = MakeTwoFrameWsa(600);
  constexpr int kGuard = 1024;
  std::vector<uint8_t> storage(
      static_cast<size_t>(kGuard + buffer_size + kGuard), '\x5a');
  const auto user_buffer =
      std::span(storage).subspan(kGuard, static_cast<size_t>(buffer_size));

  void* handle = Open_Animation("TEST.WSA", user_buffer, buffer_size, flags);
  ASSERT_NE(handle, nullptr);

  std::vector<uint8_t> page(size_t{16} * 16, '\0');
  GraphicBufferClass view(16, 16, page);
  Animate_Frame(handle, view, 1);

  const auto guard_end = storage.begin() + kGuard;
  EXPECT_EQ(std::count(storage.begin(), guard_end, '\x5a'), kGuard);
  // The system header and target frame occupy the first 256+ bytes. The
  // header holds heap pointers, any byte of which may happen to be 0xee, so
  // look for a run of frame bytes rather than a single one.
  EXPECT_EQ(std::search_n(guard_end, guard_end + 256, 8, '\xee'),
            guard_end + 256);
  EXPECT_EQ(std::count(page.begin(), page.end(), '\xee'), 0);

  Close_Animation(handle);
}

TEST(WsaTest, AnimateRejectsOversizedFrameReadFromFile) {
  // Less than the resident size, so frames are read from the file.
  ExpectOversizedFrameIsNotLoaded(1024, WSA_OPEN_FROM_DISK);
}

TEST(WsaTest, AnimateRejectsOversizedFrameCopiedFromMemory) {
  // Enough for the whole file, so frames are copied from the resident buffer.
  ExpectOversizedFrameIsNotLoaded(4096, WSA_OPEN_FROM_MEM);
}

}  // namespace

TEST(WsaDeltaTest, BoundsRunsSkipsAndTruncatedCommands) {
  std::array<uint8_t, 6> output{1, 2, 3, 4, 5, 6};
  const std::array<uint8_t, 6> run{0, 3, 0x10, 0x80, 0, 0};
  Apply_XOR_Delta(std::span(output).first(3), run);
  EXPECT_EQ(output, (std::array<uint8_t, 6>{0x11, 0x12, 0x13, 4, 5, 6}));
  const auto before = output;
  const std::array<uint8_t, 3> excessive_run{0, 7, 0xff};
  Apply_XOR_Delta(output, excessive_run);
  EXPECT_EQ(output, before);
  const std::array<uint8_t, 2> truncated{0x80, 0};
  Apply_XOR_Delta(output, truncated);
  EXPECT_EQ(output, before);
  const std::array<uint8_t, 4> skip{0x86, 1, 0xff, 0};
  Apply_XOR_Delta(output, skip);
  EXPECT_EQ(output, before);
}

TEST(WsaDeltaTest, CopiesRowsWithoutTouchingPadding) {
  std::array<uint8_t, 8> output{};
  output.fill(0xa5);
  const std::array<uint8_t, 8> literal{4, 1, 2, 3, 4, 0x80, 0, 0};
  Apply_XOR_Delta_To_Page_Or_Viewport(output, literal, 2, 4, 1);
  EXPECT_EQ(output,
            (std::array<uint8_t, 8>{1, 2, 0xa5, 0xa5, 3, 4, 0xa5, 0xa5}));
}
