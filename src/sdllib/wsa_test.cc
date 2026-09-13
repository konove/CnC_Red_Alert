// Tests for opening WSA animations into a caller-supplied buffer. The game
// normally provides the file layer; an in-memory image stands in for it.

#include "sdllib/wsa.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

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
int Open_File(const char* /*file_name*/, FileAccess /*mode*/) {
  file_pos = 0;
  return 1;
}

void Close_File(int /*handle*/) {}

long Read_File(int /*handle*/, void* buf, unsigned long bytes) {
  const int64_t available =
      std::max<int64_t>(0, std::ssize(file_image) - file_pos);
  const int64_t count = std::min(static_cast<int64_t>(bytes), available);
  std::memcpy(buf, file_image.data() + file_pos, static_cast<size_t>(count));
  file_pos += count;
  return count;
}

unsigned long Seek_File(int /*handle*/, long offset, int starting) {
  if (starting == SEEK_SET) {
    file_pos = offset;
  } else if (starting == SEEK_CUR) {
    file_pos += offset;
  } else {
    file_pos = std::ssize(file_image) + offset;
  }
  return static_cast<unsigned long>(file_pos);
}

// ww_win.cc, pulled in through gbuffer, dispatches events to the app.
void SDL_Event_Handler(SDL_Event* /*event*/) {}

// Frame 0 decoding is not under test; leave the delta buffer alone.
unsigned long LCW_Uncompress(void* /*source*/, void* /*dest*/,
                             unsigned long /*length*/) {
  return 0;
}

namespace {

void PutUint16(std::vector<char>& out, int value) {
  out.push_back(static_cast<char>(value & 0xff));
  out.push_back(static_cast<char>((value >> 8) & 0xff));
}

void PutUint32(std::vector<char>& out, int64_t value) {
  for (int shift = 0; shift < 32; shift += 8) {
    out.push_back(static_cast<char>((value >> shift) & 0xff));
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
  std::vector<char> buffer(kBufferSize, '\x5a');

  void* handle = Open_Animation("TEST.WSA", buffer.data(), kBufferSize,
                                WSA_OPEN_FROM_DISK);
  ASSERT_NE(handle, nullptr);

  // The target frame starts right after the system header (under 64 bytes)
  // and runs for width * height bytes. The old 16-bit clear stopped after
  // 76800 % 65536 bytes.
  constexpr std::ptrdiff_t kHeaderBound = 64;
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
  std::vector<char> buffer(1024, '\0');

  EXPECT_EQ(Open_Animation("TEST.WSA", buffer.data(), 1024, WSA_OPEN_FROM_DISK),
            nullptr);
}

TEST(WsaTest, OpenRejectsLargestFrameSmallerThanAnimateHeader) {
  // Before the check, 10 - 37 wrapped to 65509 and frame 0 was read that far
  // past the delta buffer.
  file_image = MakeWsa(16, 16, 10, 4);
  std::vector<char> buffer(1024, '\0');

  EXPECT_EQ(Open_Animation("TEST.WSA", buffer.data(), 1024, WSA_OPEN_FROM_DISK),
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
void ExpectOversizedFrameIsNotLoaded(long buffer_size, WSAOpenType flags) {
  file_image = MakeTwoFrameWsa(600);
  constexpr int kGuard = 1024;
  std::vector<char> storage(static_cast<size_t>(kGuard + buffer_size + kGuard),
                            '\x5a');
  char* const user_buffer = storage.data() + kGuard;

  void* handle = Open_Animation("TEST.WSA", user_buffer, buffer_size, flags);
  ASSERT_NE(handle, nullptr);

  std::vector<char> page(size_t{16} * 16, '\0');
  GraphicBufferClass view(16, 16, page.data());
  Animate_Frame(handle, view, 1);

  const auto guard_end = storage.begin() + kGuard;
  EXPECT_EQ(std::count(storage.begin(), guard_end, '\x5a'), kGuard);
  // The system header and target frame occupy the first 256+ bytes.
  EXPECT_EQ(std::count(guard_end, guard_end + 256, '\xee'), 0);
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
