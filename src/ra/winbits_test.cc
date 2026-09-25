#include "ra/winbits.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "absl/base/attributes.h"
#include "engine/base/buffer.h"
#include "engine/base/types.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/text_window.h"
#include "engine/gfx/tile.h"
#include "gtest/gtest.h"
#include "ra/compat.h"
#include "ra/defines.h"
#include "ra/dib.h"

namespace {

constexpr int kWidth = 8;
constexpr int kHeight = 4;

}  // namespace

namespace {

// A PixelBuffer over plain memory for the length of one test. The winbits
// functions take the view to draw on, so the test hands them view().
class TestScreen {
 public:
  // engine_gfx holds the window rows and the game fills them in; one window
  // covering the whole buffer is all these tests need.
  TestScreen()
      : pixels_(std::size_t{kWidth} * kHeight, 0),
        buffer_(kWidth, kHeight, pixels_) {
    WindowList[0][kWindowX] = 0;
    WindowList[0][kWindowY] = 0;
    WindowList[0][kWindowWidth] = kWidth;
    WindowList[0][kWindowHeight] = kHeight;
  }

  ~TestScreen() = default;
  TestScreen(const TestScreen&) = delete;
  TestScreen& operator=(const TestScreen&) = delete;
  TestScreen(TestScreen&&) = delete;
  TestScreen& operator=(TestScreen&&) = delete;

  [[nodiscard]] PixelView& view() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return buffer_.view();
  }

  [[nodiscard]] std::uint8_t Pixel(int x, int y) const {
    return pixels_.at(Offset(x, y));
  }
  void SetPixel(int x, int y, std::uint8_t value) {
    pixels_.at(Offset(x, y)) = value;
  }

 private:
  static std::size_t Offset(int x, int y) {
    return static_cast<std::size_t>((base::ssize{y} * kWidth) + x);
  }

  std::vector<std::uint8_t> pixels_;
  PixelBuffer buffer_;
};

// An 8-bit BMP whose pixel at column x of the bottom-up row y is
// `first + (y * width) + x`.
std::vector<std::uint8_t> MakeBmp(int width, int height, std::uint8_t first) {
  const int stride = (width + 3) / 4 * 4;
  const std::uint32_t bits_offset = 14 + 40 + (2 * 4);

  std::vector<std::uint8_t> bmp;
  const auto put16 = [&bmp](std::uint16_t v) {
    bmp.push_back(static_cast<std::uint8_t>(v & 0xFFU));
    bmp.push_back(static_cast<std::uint8_t>(v >> 8U));
  };
  const auto put32 = [&bmp](std::uint32_t v) {
    for (unsigned shift = 0; shift < 32; shift += 8) {
      bmp.push_back(static_cast<std::uint8_t>((v >> shift) & 0xFFU));
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
  ASSERT_TRUE(SaveSurfaceRect(screen.view(), 2, 1, 2, 3, saved, WINDOW_MAIN));

  // Top-down, no padding: the first row saved is the one at y == 1.
  EXPECT_EQ(saved[0], screen.Pixel(2, 1));
  EXPECT_EQ(saved[1], screen.Pixel(3, 1));
  EXPECT_EQ(saved[4], screen.Pixel(2, 3));

  for (int y = 1; y < 4; ++y) {
    screen.SetPixel(2, y, 0xFF);
    screen.SetPixel(3, y, 0xFF);
  }
  ASSERT_TRUE(
      RestoreSurfaceRect(screen.view(), 2, 1, 2, 3, saved, WINDOW_MAIN));

  EXPECT_EQ(screen.Pixel(2, 1), (2 * 1) + 8 + 1);
  EXPECT_EQ(screen.Pixel(3, 3), (3 * 8) + 3 + 1);
  EXPECT_EQ(screen.Pixel(4, 1), 8 + 4 + 1)
      << "the column beside it is untouched";
}

TEST(WinBitsTest, DrawDibTurnsTheImageRightWayUp) {
  TestScreen screen;
  const auto image = dib::Image::FromBmp(MakeBmp(2, 2, 10));
  ASSERT_TRUE(image.has_value());

  DrawDib(screen.view(), *image, 1, 1, 100, WINDOW_MAIN);

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

  DrawDib(screen.view(), *image, 0, 0, 2, WINDOW_MAIN);

  EXPECT_EQ(screen.Pixel(0, 0), 20);
  EXPECT_EQ(screen.Pixel(1, 0), 21);
  EXPECT_EQ(screen.Pixel(2, 0), 0) << "the third column is clipped away";
}

TEST(WinBitsTest, DrawDibDrawsNothingForANegativeWidth) {
  TestScreen screen;
  const auto image = dib::Image::FromBmp(MakeBmp(2, 2, 30));
  ASSERT_TRUE(image.has_value());

  DrawDib(screen.view(), *image, 0, 0, -1, WINDOW_MAIN);

  EXPECT_EQ(screen.Pixel(0, 0), 0);
  EXPECT_EQ(screen.Pixel(1, 1), 0);
}

}  // namespace

TEST(WinBitsTest, RejectsShortBuffersAndOutOfWindowRectangles) {
  TestScreen screen;
  std::uint8_t short_buffer[3] = {1, 2, 3};
  EXPECT_FALSE(
      SaveSurfaceRect(screen.view(), 0, 0, 2, 2, short_buffer, WINDOW_MAIN));
  EXPECT_FALSE(
      RestoreSurfaceRect(screen.view(), 0, 0, 2, 2, short_buffer, WINDOW_MAIN));
  EXPECT_FALSE(
      SaveSurfaceRect(screen.view(), -1, 0, 1, 1, short_buffer, WINDOW_MAIN));
  EXPECT_FALSE(RestoreSurfaceRect(screen.view(), kWidth, 0, 1, 1, short_buffer,
                                  WINDOW_MAIN));
  EXPECT_EQ(short_buffer[0], 1);
  EXPECT_EQ(screen.Pixel(0, 0), 0);
}

TEST(IconsetViewTest, RejectsTruncatedHeadersAndOutOfRangeSections) {
  const unsigned char short_data[3] = {};
  const IconsetClass truncated(std::as_bytes(std::span(short_data)));
  EXPECT_TRUE(truncated.Map_Data().empty());
  EXPECT_EQ(truncated.Map_Width(), 0);

  IControl_Type header{};
  header.Map = -1;
  const IconsetClass negative(base::ObjectBytes(header));
  EXPECT_TRUE(negative.Map_Data().empty());
  header.Map = sizeof(header) + 1;
  const IconsetClass oversized(base::ObjectBytes(header));
  EXPECT_TRUE(oversized.Map_Data().empty());
  header.Map = sizeof(header);
  const IconsetClass end(base::ObjectBytes(header));
  EXPECT_TRUE(end.Map_Data().empty());
}
