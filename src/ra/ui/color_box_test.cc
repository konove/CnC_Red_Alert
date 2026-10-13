// Tests DrawColorBox: the colour reaches every pixel inside the bevel and none
// outside it, and the bevel is sunken for the selected box only.

#include "base/rect.h"
#include "engine/gfx/palette_index.h"
#include "engine/gfx/pixel_buffer.h"
#include "gtest/gtest.h"
#include "ra/startup/ui_test_util.h"
#include "ra/ui/dialog.h"

namespace ra {
namespace {

class ColorBoxTest : public UiFixture {
 protected:
  static constexpr engine::gfx::PaletteIndex kColor{200};
  static constexpr base::Rect kBox{3, 2, 20, 18};

  // The number of pixels of the page that are in the swatch colour.
  int ColoredPixels() {
    int count = 0;
    for (int y = 0; y < page_.view().height(); ++y) {
      for (int x = 0; x < page_.view().width(); ++x) {
        count += page_.view().GetPixel(x, y) == kColor ? 1 : 0;
      }
    }
    return count;
  }

  PixelBuffer page_{30, 24};
};

TEST_F(ColorBoxTest, FillsExactlyTheInsideOfTheBevel) {
  DrawColorBox(page_.view(), kBox, kColor, false);

  for (int y = kBox.y + 1; y < kBox.bottom() - 1; ++y) {
    for (int x = kBox.x + 1; x < kBox.right() - 1; ++x) {
      ASSERT_EQ(page_.view().GetPixel(x, y), kColor) << x << "," << y;
    }
  }
  EXPECT_EQ(ColoredPixels(), (kBox.width - 2) * (kBox.height - 2));
}

TEST_F(ColorBoxTest, SelectedBoxSwapsTheBevelColors) {
  DrawColorBox(page_.view(), kBox, kColor, false);
  const engine::gfx::PaletteIndex raised_top =
      page_.view().GetPixel(kBox.x + 1, kBox.y);
  const engine::gfx::PaletteIndex raised_bottom =
      page_.view().GetPixel(kBox.x + 1, kBox.bottom() - 1);

  DrawColorBox(page_.view(), kBox, kColor, true);

  EXPECT_EQ(page_.view().GetPixel(kBox.x + 1, kBox.y), raised_bottom);
  EXPECT_EQ(page_.view().GetPixel(kBox.x + 1, kBox.bottom() - 1), raised_top);
  EXPECT_EQ(ColoredPixels(), (kBox.width - 2) * (kBox.height - 2));
}

}  // namespace
}  // namespace ra
