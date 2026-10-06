// Tests for CC_Draw_Shape()'s default ghost and fading tables.

#include "td/startup/conquer.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <vector>

#include "base/array.h"
#include "base/unaligned.h"
#include "engine/gfx/fading_table.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/shape.h"
#include "engine/gfx/text_window.h"
#include "gtest/gtest.h"
#include "td/core/defines.h"
#include "td/core/jshell.h"
#include "td/map/display.h"

namespace td {
namespace {

using engine::gfx::ShapeFlags;

// The page and the window drawn into: Tiberian Dawn's 320x200.
constexpr int kPageWidth = 320;
constexpr int kPageHeight = 200;

// A shape file of one 2x2 frame, LCW-compressed, of colors 1 to 4.
std::vector<std::byte> ShapeFile() {
  std::vector<std::byte> data(36);
  base::WriteUnaligned<uint16_t>(data, 1);                        // Frames.
  base::WriteUnaligned<uint16_t>(std::span(data).subspan(6), 2);  // Width.
  base::WriteUnaligned<uint16_t>(std::span(data).subspan(8), 2);  // Height.
  // The frame's offset, flagged as an LCW key frame, and where it ends.
  base::WriteUnaligned<uint32_t>(std::span(data).subspan(14), 0x8000001eU);
  base::WriteUnaligned<uint32_t>(std::span(data).subspan(22), 36);
  constexpr std::array<uint8_t, 6> kStream{0x84, 1, 2, 3, 4, 0x80};
  std::ranges::transform(kStream, data.begin() + 30,
                         [](const uint8_t value) { return std::byte{value}; });
  return data;
}

// The main window's row of WindowList.
int (&MainWindow())[8] {
  return base::At(WindowList, static_cast<int>(WINDOW_MAIN));
}

// Opens the main window over the whole page, and puts it and the display's
// shared tables a test replaced back afterwards.
class CcDrawShapeTest : public testing::Test {
 public:
  CcDrawShapeTest() {
    Set_Window(static_cast<int>(WINDOW_MAIN), 0, 0, kPageWidth, kPageHeight);
  }
  CcDrawShapeTest(const CcDrawShapeTest&) = delete;
  CcDrawShapeTest& operator=(const CcDrawShapeTest&) = delete;
  CcDrawShapeTest(CcDrawShapeTest&&) = delete;
  CcDrawShapeTest& operator=(CcDrawShapeTest&&) = delete;
  ~CcDrawShapeTest() override {
    DisplayClass::FadingShade = saved_shade_;
    DisplayClass::SpecialGhost = saved_ghost_;
    std::ranges::copy(saved_window_, std::begin(MainWindow()));
  }

 protected:
  // Returns the page's top left 2x2 pixels after drawing the shape there
  // with `flags` and no tables of the caller's.
  static std::vector<int> Draw(const ShapeFlags flags) {
    PixelBuffer page(kPageWidth, kPageHeight);
    page.view().Clear();
    const auto shape_file = ShapeFile();
    CC_Draw_Shape(page.view(), shape_file, 0, 0, 0, WINDOW_MAIN, flags);
    return {page.view().GetPixel(0, 0), page.view().GetPixel(1, 0),
            page.view().GetPixel(0, 1), page.view().GetPixel(1, 1)};
  }

 private:
  std::array<int, 8> saved_window_ = std::to_array(MainWindow());
  FadingTable saved_shade_ = DisplayClass::FadingShade;
  TranslucencyTable<1> saved_ghost_ = DisplayClass::SpecialGhost;
};

TEST_F(CcDrawShapeTest, DrawsTheFrameUnchangedWithoutEffects) {
  EXPECT_EQ(Draw(ShapeFlags::kNone), (std::vector<int>{1, 2, 3, 4}));
}

TEST_F(CcDrawShapeTest, FadingWithoutATableUsesFadingShade) {
  for (int color = 0; color < 256; ++color) {
    DisplayClass::FadingShade.at(static_cast<size_t>(color)) =
        static_cast<uint8_t>(color + 10);
  }

  EXPECT_EQ(Draw(ShapeFlags::kFading), (std::vector<int>{11, 12, 13, 14}));
}

TEST_F(CcDrawShapeTest, GhostWithoutATableUsesSpecialGhost) {
  // Every color takes the table's one row, which turns the page's 0 into 7.
  DisplayClass::SpecialGhost = {};
  DisplayClass::SpecialGhost.at(256) = 7;

  EXPECT_EQ(Draw(ShapeFlags::kGhost), (std::vector<int>{7, 7, 7, 7}));
}

}  // namespace
}  // namespace td
