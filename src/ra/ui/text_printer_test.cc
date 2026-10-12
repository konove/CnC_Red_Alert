// Tests TextPrinter: where a text lands for each justification, what With()
// and Plain() change, and how PrintClipped() cuts and tabs.

#include <algorithm>
#include <string>

#include "absl/base/attributes.h"
#include "engine/gfx/font.h"
#include "engine/gfx/palette_index.h"
#include "engine/gfx/pixel_buffer.h"
#include "game/ui/standard_colors.h"
#include "gtest/gtest.h"
#include "ra/assets/palettes.h"
#include "ra/core/defines.h"
#include "ra/data/text_ids.h"
#include "ra/startup/ui_test_util.h"
#include "ra/ui/dialog.h"

namespace ra {
namespace {

// The box around the pixels of one colour on a page.
struct Extent {
  int left = -1;  // -1 when the page has no such pixel.
  int top = -1;
  int right = -1;  // Inclusive.
  int count = 0;
};

// UiFixture's fonts are all one 6x10 block font and each of its colour
// schemes draws in one colour, so a printed text is a row of solid blocks in
// the scheme's colour.
class TextPrinterTest : public UiFixture {
 protected:
  static constexpr int kWidth = 300;
  static constexpr int kHeight = 20;
  static constexpr FontType kFont = FontType::k6Point;
  static constexpr engine::gfx::PaletteIndex kGreen = SchemeColor(PCOLOR_GREEN);

  // The printer the tests start from: left justified, in the green scheme.
  TextPrinter Printer(const TextPrintType flags = TPF_NOSHADOW)
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return {page_.view(),
            {&ThePalettes().color_remaps().at(PCOLOR_GREEN),
             game::kTransparentBlack, kFont, flags}};
  }

  // The pixel width the font gives `text`.
  static int WidthOf(const char* text) {
    return StringPixelWidth(TextFontStyle(kFont, TPF_NOSHADOW), text);
  }

  Extent ExtentOf(const engine::gfx::PaletteIndex color) {
    PixelView& view = page_.view();
    Extent extent;
    for (int y = 0; y < kHeight; ++y) {
      for (int x = 0; x < kWidth; ++x) {
        if (view.GetPixel(x, y) != color) {
          continue;
        }
        extent.left = extent.count == 0 ? x : std::min(extent.left, x);
        extent.top = extent.count == 0 ? y : std::min(extent.top, y);
        extent.right = std::max(extent.right, x);
        ++extent.count;
      }
    }
    return extent;
  }

  PixelBuffer page_{kWidth, kHeight};
};

TEST_F(TextPrinterTest, ATextStartsAtThePoint) {
  Printer().Print("AB", {10, 3});

  const Extent text = ExtentOf(kGreen);
  EXPECT_EQ(text.left, 10);
  EXPECT_EQ(text.top, 3);
  // A glyph is a pixel wider than the step to the next one.
  EXPECT_LE(text.right, 10 + WidthOf("AB"));
  EXPECT_EQ(text.count + ExtentOf(game::kTransparentBlack).count,
            kWidth * kHeight);
}

TEST_F(TextPrinterTest, ARightJustifiedTextEndsAtThePoint) {
  Printer(TPF_NOSHADOW | TPF_RIGHT).Print("AB", {100, 0});

  EXPECT_EQ(ExtentOf(kGreen).left, 100 - WidthOf("AB"));
}

TEST_F(TextPrinterTest, ACenteredTextHasThePointInItsMiddle) {
  Printer(TPF_NOSHADOW | TPF_CENTER).Print("ABCD", {100, 0});

  EXPECT_EQ(ExtentOf(kGreen).left, 100 - (WidthOf("ABCD") / 2));
}

TEST_F(TextPrinterTest, WithAddsFlagsToACopy) {
  const TextPrinter left = Printer();
  const TextPrinter right = left.With(TPF_RIGHT);

  right.Print("AB", {100, 0});
  EXPECT_EQ(ExtentOf(kGreen).left, 100 - WidthOf("AB"));

  // The printer it was made from still prints from the point on.
  page_.view().Clear();
  left.Print("AB", {100, 0});
  EXPECT_EQ(ExtentOf(kGreen).left, 100);
}

TEST_F(TextPrinterTest, ATextIsPrintedAsItIs) {
  Printer().Print("%d%%", {0, 0});
  const int printed = ExtentOf(kGreen).count;
  ASSERT_GT(printed, 0);

  page_.view().Clear();
  Printer().Print("ABCD", {0, 0});
  EXPECT_EQ(ExtentOf(kGreen).count, printed);
}

TEST_F(TextPrinterTest, AStringPrintsLikeItsCharacters) {
  Printer().Print(std::string("AB"), {10, 3});

  const Extent text = ExtentOf(kGreen);
  EXPECT_EQ(text.left, 10);
  EXPECT_EQ(text.top, 3);
}

TEST_F(TextPrinterTest, ATextNumberPrintsTheTableText) {
  SetText(TXT_OK, "AB");
  Printer(TPF_NOSHADOW | TPF_RIGHT).Print(TXT_OK, {100, 0});

  EXPECT_EQ(ExtentOf(kGreen).left, 100 - WidthOf("AB"));
}

TEST_F(TextPrinterTest, NoTextPrintsNothing) {
  const char* const none = nullptr;
  Printer().Print(none, {10, 3});
  Printer().Print("", {10, 3});
  Printer().Print(TXT_NONE, {10, 3});
  Printer().PrintClipped(none, {10, 3}, 100);

  EXPECT_EQ(ExtentOf(game::kTransparentBlack).count, kWidth * kHeight);
}

TEST_F(TextPrinterTest, ATextBeyondThePageIsNotPrinted) {
  Printer().Print("AB", {kWidth, 0});
  Printer().Print("AB", {0, kHeight});

  EXPECT_EQ(ExtentOf(game::kTransparentBlack).count, kWidth * kHeight);
}

TEST_F(TextPrinterTest, APrinterWithoutASchemePrintsInTheRedOne) {
  TextPrinter(page_.view(), {.font = kFont, .flags = TPF_NOSHADOW})
      .Print("AB", {0, 0});

  EXPECT_GT(ExtentOf(SchemeColor(PCOLOR_RED)).count, 0);
}

TEST_F(TextPrinterTest, APlainPrinterPrintsInItsOneColour) {
  constexpr engine::gfx::PaletteIndex kColor{200};
  // With() copies the printer; the copy must paint from its own scheme, not
  // from the one inside the temporary it was copied from.
  const TextPrinter plain =
      TextPrinter::Plain(page_.view(), kColor, game::kTransparentBlack, kFont,
                         TPF_NOSHADOW)
          .With(TPF_RIGHT);

  plain.Print("AB", {100, 0});

  const Extent text = ExtentOf(kColor);
  EXPECT_EQ(text.left, 100 - WidthOf("AB"));
  EXPECT_EQ(text.count + ExtentOf(game::kTransparentBlack).count,
            kWidth * kHeight);
}

TEST_F(TextPrinterTest, AClippedTextStopsAtTheWidth) {
  Printer().PrintClipped("ABCDEFGHIJ", {10, 0}, WidthOf("ABC") + 1);

  const Extent text = ExtentOf(kGreen);
  EXPECT_EQ(text.left, 10);
  EXPECT_LT(text.right, 10 + WidthOf("ABC") + 1);
  EXPECT_GE(text.right, 10 + WidthOf("AB"));
}

TEST_F(TextPrinterTest, ATabMovesOnToTheNextTabStop) {
  static constexpr int kTabs[] = {40, 90};
  Printer().PrintClipped("\tA\tB", {10, 0}, 200, kTabs);

  const Extent text = ExtentOf(kGreen);
  EXPECT_EQ(text.left, 10 + 40);
  EXPECT_GE(text.right, 10 + 90);
  // A glyph is a pixel wider than the step to the next one.
  EXPECT_LE(text.right, 10 + 90 + WidthOf("B"));
}

TEST_F(TextPrinterTest, ATabWithoutTabStopsMovesToTheNextFifty) {
  Printer().PrintClipped("\tA", {10, 0}, 200);

  EXPECT_EQ(ExtentOf(kGreen).left, 10 + 50);
}

TEST_F(TextPrinterTest, TextPastTheLastTabStopStaysOnIt) {
  static constexpr int kTabs[] = {40};
  Printer().PrintClipped("\tA\tB", {10, 0}, 200, kTabs);

  // Both columns land on the one stop.
  const Extent text = ExtentOf(kGreen);
  EXPECT_EQ(text.left, 10 + 40);
  // A glyph is a pixel wider than the step to the next one.
  EXPECT_LE(text.right, 10 + 40 + WidthOf("B"));
}

}  // namespace
}  // namespace ra
