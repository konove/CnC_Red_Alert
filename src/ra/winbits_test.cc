#include "ra/winbits.h"

#include <cstddef>
#include <cstdint>
#include <vector>

#include "base/types.h"
#include "gtest/gtest.h"
#include "ra/defines.h"
#include "ra/dib.h"
#include "sdllib/drawbuff.h"
#include "sdllib/gbuffer.h"
#include "sdllib/ww_win.h"

namespace {

constexpr int kWidth = 8;
constexpr int kHeight = 4;

}  // namespace

// The real table lives in ra/globals.cc, which would drag the whole game in.
// One window, covering the whole buffer, is all these tests need.
int WindowList[][8] = {
    {0, 0, kWidth, kHeight, 0, 0, 0, 0},
};

// sdllib leaves this to the application. No test here pumps the event loop.
void SDL_Event_Handler(SDL_Event* /*event*/) {}

namespace {

// A GraphicBufferClass over plain memory, installed as LogicPage for the
// length of one test and taken out again afterwards.
class TestScreen {
 public:
  TestScreen()
      : pixels_(std::size_t{kWidth} * kHeight, 0),
        buffer_(kWidth, kHeight, pixels_.data()) {
    previous_ = Set_Logic_Page(&buffer_);
  }

  TestScreen(const TestScreen&) = delete;
  TestScreen& operator=(const TestScreen&) = delete;

  ~TestScreen() { Set_Logic_Page(previous_); }

  std::uint8_t Pixel(int x, int y) const { return pixels_[Offset(x, y)]; }
  void SetPixel(int x, int y, std::uint8_t value) {
    pixels_[Offset(x, y)] = value;
  }

 private:
  static std::size_t Offset(int x, int y) {
    return static_cast<std::size_t>((base::ssize{y} * kWidth) + x);
  }

  std::vector<std::uint8_t> pixels_;
  GraphicBufferClass buffer_;
  GraphicViewPortClass* previous_;
};

// An 8-bit BMP whose pixel at column x of the bottom-up row y is
// `first + (y * width) + x`.
std::vector<std::uint8_t> MakeBmp(int width, int height, std::uint8_t first) {
  const int stride = (width + 3) & ~3;
  const std::uint32_t bits_offset = 14 + 40 + (2 * 4);

  std::vector<std::uint8_t> bmp;
  auto put16 = [&bmp](std::uint16_t v) {
    bmp.push_back(static_cast<std::uint8_t>(v & 0xFFU));
    bmp.push_back(static_cast<std::uint8_t>(v >> 8U));
  };
  auto put32 = [&bmp](std::uint32_t v) {
    for (int i = 0; i < 4; ++i) {
      bmp.push_back(static_cast<std::uint8_t>((v >> (8U * i)) & 0xFFU));
    }
  };

  put16(0x4D42);
  put32(bits_offset + static_cast<std::uint32_t>(stride * height));
  put16(0);
  put16(0);
  put32(bits_offset);

  put32(40);
  put32(static_cast<std::uint32_t>(width));
  put32(static_cast<std::uint32_t>(height));
  put16(1);
  put16(8);
  put32(0);
  put32(0);
  put32(0);
  put32(0);
  put32(2);
  put32(0);
  for (int i = 0; i < 2 * 4; ++i) {
    bmp.push_back(0);
  }

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < stride; ++x) {
      bmp.push_back(
          x < width ? static_cast<std::uint8_t>(first + (y * width) + x) : 0);
    }
  }
  return bmp;
}

TEST(WinBitsTest, SaveAndRestoreRoundTripsARectangle) {
  TestScreen screen;
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      screen.SetPixel(x, y, static_cast<std::uint8_t>((y * kWidth) + x + 1));
    }
  }

  std::uint8_t saved[2 * 3] = {};
  ASSERT_TRUE(SaveSurfaceRect(2, 1, 2, 3, saved, WINDOW_MAIN));

  // Top-down, no padding: the first row saved is the one at y == 1.
  EXPECT_EQ(saved[0], screen.Pixel(2, 1));
  EXPECT_EQ(saved[1], screen.Pixel(3, 1));
  EXPECT_EQ(saved[4], screen.Pixel(2, 3));

  for (int y = 1; y < 4; ++y) {
    screen.SetPixel(2, y, 0xFF);
    screen.SetPixel(3, y, 0xFF);
  }
  ASSERT_TRUE(RestoreSurfaceRect(2, 1, 2, 3, saved, WINDOW_MAIN));

  EXPECT_EQ(screen.Pixel(2, 1), 2 * 1 + 8 + 1);
  EXPECT_EQ(screen.Pixel(3, 3), 3 * 8 + 3 + 1);
  EXPECT_EQ(screen.Pixel(4, 1), 8 + 4 + 1)
      << "the column beside it is untouched";
}

TEST(WinBitsTest, DrawDibTurnsTheImageRightWayUp) {
  TestScreen screen;
  const auto image = dib::Image::FromBmp(MakeBmp(2, 2, 10));
  ASSERT_TRUE(image.has_value());

  DrawDib(*image, 1, 1, 100, WINDOW_MAIN);

  // Row 0 of the image is its bottom row, so it lands on the lower line.
  EXPECT_EQ(screen.Pixel(1, 2), 10);
  EXPECT_EQ(screen.Pixel(2, 2), 11);
  EXPECT_EQ(screen.Pixel(1, 1), 12);
  EXPECT_EQ(screen.Pixel(2, 1), 13);
  EXPECT_EQ(screen.Pixel(0, 1), 0) << "nothing spills to the left";
}

TEST(WinBitsTest, DrawDibClipsEachRowToTheGivenWidth) {
  TestScreen screen;
  const auto image = dib::Image::FromBmp(MakeBmp(3, 1, 20));
  ASSERT_TRUE(image.has_value());

  DrawDib(*image, 0, 0, 2, WINDOW_MAIN);

  EXPECT_EQ(screen.Pixel(0, 0), 20);
  EXPECT_EQ(screen.Pixel(1, 0), 21);
  EXPECT_EQ(screen.Pixel(2, 0), 0) << "the third column is clipped away";
}

TEST(WinBitsTest, DrawDibDrawsNothingForANegativeWidth) {
  TestScreen screen;
  const auto image = dib::Image::FromBmp(MakeBmp(2, 2, 30));
  ASSERT_TRUE(image.has_value());

  DrawDib(*image, 0, 0, -1, WINDOW_MAIN);

  EXPECT_EQ(screen.Pixel(0, 0), 0);
  EXPECT_EQ(screen.Pixel(1, 1), 0);
}

}  // namespace
