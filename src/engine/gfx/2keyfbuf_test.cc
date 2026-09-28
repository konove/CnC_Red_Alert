// Tests for DrawShapeFrame: clipping, transparency, the ghost and fading
// tables, and the predator (cloak) effect's sampling of the page.

#include "engine/gfx/2keyfbuf.h"

#include <cstddef>
#include <cstdint>
#include <span>
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

  // Draws the width x height frame of `colors` at x,y.
  void Draw(const std::vector<uint8_t>& colors, const int width,
            const int height, const int x, const int y,
            const ShapeFlags_Type flags = SHAPE_NORMAL,
            const ShapeEffects& effects = {}) {
    DrawShapeFrame(x, y, width, height, std::as_bytes(std::span(colors)),
                   page_.view(), flags, effects);
  }

  // Draws a one-pixel shape of color 99 at x,y with the predator effect
  // starting at `predator_offset`.
  void DrawPredator(const int x, const int y, const int predator_offset) {
    ShapeEffects effects;
    effects.predator_offset = predator_offset;
    Draw({99}, 1, 1, x, y, SHAPE_PREDATOR, effects);
  }

  [[nodiscard]] uint8_t PixelAt(const int x, const int y) const {
    return pixels_.at(base::ToSize((y * kPageWidth) + x));
  }

  std::vector<uint8_t> pixels_ =
      std::vector<uint8_t>(std::size_t{kPageWidth} * kPageHeight);
  PixelBuffer page_{kPageWidth, kPageHeight, pixels_};
};

TEST_F(DrawShapeFrameTest, CopiesEveryColorWithoutEffects) {
  Draw({0, 50, 51, 52}, 2, 2, 1, 0);

  EXPECT_EQ(PixelAt(1, 0), 0);
  EXPECT_EQ(PixelAt(2, 0), 50);
  EXPECT_EQ(PixelAt(1, 1), 51);
  EXPECT_EQ(PixelAt(2, 1), 52);
  EXPECT_EQ(PixelAt(3, 0), 3);
}

TEST_F(DrawShapeFrameTest, ColorZeroIsTransparentUnderShapeTrans) {
  Draw({0, 50}, 2, 1, 1, 0, SHAPE_TRANS);

  EXPECT_EQ(PixelAt(1, 0), 1);
  EXPECT_EQ(PixelAt(2, 0), 50);
}

TEST_F(DrawShapeFrameTest, ClipsAtTheTopLeft) {
  Draw({50, 51, 52, 53}, 2, 2, -1, -1);

  EXPECT_EQ(PixelAt(0, 0), 53);
  EXPECT_EQ(PixelAt(1, 0), 1);
  EXPECT_EQ(PixelAt(0, 1), 8);
}

TEST_F(DrawShapeFrameTest, ClipsAtTheBottomRight) {
  Draw({50, 51, 52, 53}, 2, 2, 7, 2);

  EXPECT_EQ(PixelAt(7, 2), 50);
  EXPECT_EQ(PixelAt(6, 2), 22);
  EXPECT_EQ(PixelAt(7, 1), 15);
}

TEST_F(DrawShapeFrameTest, CentersTheFrameOnXY) {
  Draw({50, 51, 52, 53}, 2, 2, 2, 2, SHAPE_CENTER);

  EXPECT_EQ(PixelAt(1, 1), 50);
  EXPECT_EQ(PixelAt(2, 2), 53);
}

TEST_F(DrawShapeFrameTest, DrawsNothingFromAFrameShorterThanItsSize) {
  Draw({50, 51, 52}, 2, 2, 0, 0);

  EXPECT_EQ(PixelAt(0, 0), 0);
}

// Color 50 is translucent through row 0, which adds 100 to the page pixel;
// color 51 is opaque.
TEST_F(DrawShapeFrameTest, GhostColorsRemapThePageUnderThem) {
  std::vector<uint8_t> ghost(256 + 256, 0xFF);
  for (int page_color = 0; page_color < 256; ++page_color) {
    ghost.at(base::ToSize(256 + page_color)) =
        static_cast<uint8_t>(page_color + 100);
  }
  ghost.at(50) = 0;
  ShapeEffects effects;
  effects.ghost_table = ghost;

  Draw({50, 51}, 2, 1, 3, 1, SHAPE_GHOST, effects);

  EXPECT_EQ(PixelAt(3, 1), 111);
  EXPECT_EQ(PixelAt(4, 1), 51);
}

TEST_F(DrawShapeFrameTest, DrawsNothingWithAGhostRowMissing) {
  std::vector<uint8_t> ghost(256, 0xFF);
  ghost.at(50) = 0;
  ShapeEffects effects;
  effects.ghost_table = ghost;

  Draw({50}, 1, 1, 0, 0, SHAPE_GHOST, effects);

  EXPECT_EQ(PixelAt(0, 0), 0);
}

TEST_F(DrawShapeFrameTest, FadingRemapsTheColorOnce) {
  std::vector<uint8_t> fade(256);
  for (std::size_t color = 0; color < fade.size(); ++color) {
    fade.at(color) = static_cast<uint8_t>(255 - color);
  }
  ShapeEffects effects;
  effects.fading_table = fade;

  Draw({10}, 1, 1, 0, 0, SHAPE_FADING, effects);

  EXPECT_EQ(PixelAt(0, 0), 245);
}

// Offset 0 walks 1, 3, 2 pixels to the right, reading each sample before
// this shape overwrites it.
TEST_F(DrawShapeFrameTest, ThePredatorWalkAdvancesPerPixel) {
  Draw({99, 99, 99}, 3, 1, 0, 0, SHAPE_PREDATOR);

  EXPECT_EQ(PixelAt(0, 0), 1);
  EXPECT_EQ(PixelAt(1, 0), 4);
  EXPECT_EQ(PixelAt(2, 0), 4);
}

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
