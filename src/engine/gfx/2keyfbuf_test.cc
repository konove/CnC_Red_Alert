// Tests for DrawShapeFrame: the predator (cloak) effect's sampling of
// the page.

#include "engine/gfx/2keyfbuf.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "base/numeric.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/shape.h"
#include "gtest/gtest.h"

namespace {

constexpr int kPageWidth = 8;
constexpr int kPageHeight = 3;

// Draws on an 8x3 page whose pixels are numbered 0 to 23, so a drawn pixel
// shows which page pixel it sampled.
class DrawShapeFrameTest : public testing::Test {
 protected:
  DrawShapeFrameTest() {
    for (std::size_t i = 0; i < pixels_.size(); ++i) {
      pixels_.at(i) = static_cast<uint8_t>(i);
    }
  }

  // Draws a one-pixel shape of color 99 at x,y with the predator effect
  // starting at `predator_offset`.
  void DrawPredator(const int x, const int y, const int predator_offset) {
    std::array<std::byte, 1> shape{std::byte{99}};
    ShapeEffects effects;
    effects.predator_offset = predator_offset;
    DrawShapeFrame(x, y, 1, 1, shape, page_.view(), SHAPE_PREDATOR, effects);
  }

  [[nodiscard]] uint8_t PixelAt(const int x, const int y) const {
    return pixels_.at(base::ToSize((y * kPageWidth) + x));
  }

  std::vector<uint8_t> pixels_ =
      std::vector<uint8_t>(std::size_t{kPageWidth} * kPageHeight);
  PixelBuffer page_{kPageWidth, kPageHeight, pixels_};
};

// Offset 1 starts the walk at the second entry, 3 pixels to the right.
TEST_F(DrawShapeFrameTest, APositiveOffsetSamplesToTheRight) {
  DrawPredator(2, 1, 1);

  EXPECT_EQ(PixelAt(2, 1), PixelAt(5, 1));
}

// Offset -1 starts the walk at the second of the down-and-left entries: one
// row below and 3 pixels to the left.
TEST_F(DrawShapeFrameTest, ANegativeOffsetSamplesBelowAndToTheLeft) {
  DrawPredator(4, 1, -1);

  EXPECT_EQ(PixelAt(4, 1), PixelAt(1, 2));
}

}  // namespace
