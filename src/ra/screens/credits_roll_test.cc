// Tests for the scrolling credits, run from start to finish on a headless
// screen with a clock that moves as fast as the loop asks for time.

#include "ra/screens/credits_roll.h"

#include <cstdint>
#include <string>
#include <vector>

#include "base/fixed.h"
#include "base/numeric.h"
#include "engine/file/disk_file.h"
#include "engine/file/file_access.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/palette_color.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/keyboard.h"
#include "gtest/gtest.h"
#include "ra/assets/filepcx.h"
#include "ra/assets/palette.h"
#include "ra/assets/palettes.h"
#include "ra/assets/screen.h"
#include "ra/core/defines.h"
#include "ra/screens/goptions.h"
#include "ra/startup/ui_test_util.h"
#include "ra/ui/color_scheme.h"

namespace ra {
namespace {

// The colour every slide in these tests is painted in, and the first of the
// slideshow's pictures.
constexpr uint8_t kSlideColor = 200;
constexpr const char* kFirstSlide = "aftr_hi.pcx";

// One credit, on the first line of the file. It starts at the top of the
// hidden page's bottom margin and goes up a row a frame.
constexpr const char* kOneCredit = "Producer\r";

class CreditsRollTest : public UiFixture {
 protected:
  // The colour the credits are drawn in: the green scheme's.
  static constexpr int kTextColor = SchemeColor(PCOLOR_GREEN);

  CreditsRollTest() {
    // A few lines take about 470 frames to scroll off a 400-row screen.
    player().set_frame_budget(5000);
  }

  // Writes a slide that is kSlideColor all over, with that colour white in
  // its palette, as the picture file `name`.
  void WriteSlide(const std::string& name) {
    PixelBuffer picture(Screen::kWidth, Screen::kHeight);
    picture.view().Clear(kSlideColor);
    engine::gfx::Palette palette;
    palette.at(kSlideColor) = engine::gfx::PaletteColor(255, 255, 255);
    const auto file =
        OpenDiskFile(GameFilePath(name).string(), FileAccess::kWrite);
    ASSERT_NE(file, nullptr);
    Write_PCX_File(*file, picture, palette);
  }

  // Writes the palette of the text, EGOPAL.PAL, with the text colour white.
  void WriteTextPalette() {
    std::string palette(768, '\0');
    for (int gun = 0; gun < 3; ++gun) {
      palette.at(base::ToSize((kTextColor * 3) + gun)) = 63;
    }
    WriteGameFile("EGOPAL.PAL", palette);
  }

  // The rows of the visible page that hold text, top to bottom.
  static std::vector<int> TextRows() {
    PixelView& view = TheScreen().visible_view();
    std::vector<int> rows;
    for (int y = 0; y < view.height(); ++y) {
      for (int x = 0; x < view.width(); ++x) {
        if (view.GetPixel(x, y) == kTextColor) {
          rows.push_back(y);
          break;
        }
      }
    }
    return rows;
  }

  // The first and last columns of the visible page that hold text, or -1
  // for both when there is none.
  struct TextSpan {
    int left = -1;
    int right = -1;
  };
  static TextSpan TextColumns() {
    PixelView& view = TheScreen().visible_view();
    TextSpan span;
    for (int x = 0; x < view.width(); ++x) {
      for (int y = 0; y < view.height(); ++y) {
        if (view.GetPixel(x, y) == kTextColor) {
          span.left = span.left < 0 ? x : span.left;
          span.right = x;
          break;
        }
      }
    }
    return span;
  }

  // Rolls credits that are the one line `line` and returns where its text
  // was across the page when it was half way up.
  TextSpan ColumnsOfTheLine(const std::string& line) {
    WriteGameFile("credits.txt", line + "\r");
    TextSpan span;
    player().At(250).Do([&span] { span = TextColumns(); });
    ShowCredits();
    return span;
  }

  // How bright the slides' colour is being shown, 0 to 255.
  static int SlideBrightness() {
    return PaletteClass::CurrentPalette.at(kSlideColor).red();
  }
};

TEST_F(CreditsRollTest, WithoutACreditsFileThereIsNothingToShow) {
  ShowCredits();

  EXPECT_EQ(player().frame(), 0);
}

TEST_F(CreditsRollTest, AnEmptyCreditsFileShowsNothingEither) {
  WriteGameFile("credits.txt", "");

  ShowCredits();

  EXPECT_EQ(player().frame(), 0);
}

TEST_F(CreditsRollTest, RollsToTheEndOnItsOwn) {
  WriteGameFile("credits.txt", "Producer\tEd\rArtist\tJoe\r");

  ShowCredits();

  EXPECT_GT(player().frame(), 400);
}

TEST_F(CreditsRollTest, TheTextGoesUpARowAFrame) {
  WriteGameFile("credits.txt", kOneCredit);
  std::vector<int> early;
  std::vector<int> later;
  player().At(250).Do([&early] { early = TextRows(); }).At(260).Do([&later] {
    later = TextRows();
  });

  ShowCredits();

  // The block font is 10 rows tall.
  ASSERT_EQ(early.size(), 10U);
  ASSERT_EQ(later.size(), 10U);
  EXPECT_EQ(later.front(), early.front() - 10);
}

// The page is 80 columns of 8 pixels. The credits font is set a pixel
// tighter than its glyphs are wide, so the 6-wide block font advances 5 a
// character and a string of n is 5n + 1 pixels across.
constexpr int kAdvance = 5;

// A string that starts in the right half starts at its column.
TEST_F(CreditsRollTest, AStringInTheRightHalfStartsAtItsColumn) {
  const TextSpan span = ColumnsOfTheLine(std::string(45, ' ') + "Name");

  EXPECT_EQ(span.left, 45 * 8);
  EXPECT_EQ(span.right, (45 * 8) + (4 * kAdvance));
}

// A string in the left half ends at its last column, so that the job titles
// line up against the names.
TEST_F(CreditsRollTest, AStringInTheLeftHalfEndsAtItsLastColumn) {
  const TextSpan span = ColumnsOfTheLine("          Producer");

  EXPECT_EQ(span.right - span.left, 8 * kAdvance);
  EXPECT_LT(span.right, Screen::kWidth / 2);
  EXPECT_GT(span.left, 10 * 8);
}

// A string that crosses the middle is centred on the screen, wherever it
// started.
TEST_F(CreditsRollTest, AStringAcrossTheMiddleIsCentred) {
  const TextSpan span =
      ColumnsOfTheLine(std::string(10, ' ') + std::string(50, 'x'));

  EXPECT_EQ(span.right - span.left, 50 * kAdvance);
  EXPECT_EQ(span.left, (Screen::kWidth - (50 * kAdvance)) / 2);
}

// Credits longer than their score start it again.
TEST_F(CreditsRollTest, LongCreditsRollPastTheEndOfTheirScore) {
  std::string credits;
  for (int line = 0; line < 45; ++line) {
    credits += "Tester\r";
  }
  WriteGameFile("credits.txt", credits);

  ShowCredits();

  EXPECT_GT(player().frame(), 1000);
}

// A line is erased where it was before it is drawn a row higher, so it never
// leaves a trail: there are always exactly its own ten rows of text.
TEST_F(CreditsRollTest, AMovingLineLeavesNoTrail) {
  WriteGameFile("credits.txt", kOneCredit);
  WriteSlide(kFirstSlide);
  std::vector<int> rows;
  player().At(300).Do([&rows] { rows = TextRows(); });

  ShowCredits();

  ASSERT_EQ(rows.size(), 10U);
  EXPECT_EQ(rows.back(), rows.front() + 9);
}

// The text printer does not clip, so a line shows and goes whole, behind the
// 16 rows at the top and bottom that are never copied to the screen.
TEST_F(CreditsRollTest, TheMarginRowsAreNeverDrawnOn) {
  WriteGameFile("credits.txt", "One\rTwo\rThree\rFour\r");
  WriteSlide(kFirstSlide);
  bool margin_drawn = false;
  const auto check = [&margin_drawn] {
    PixelView& view = TheScreen().visible_view();
    for (const int y : {0, 15, view.height() - 16, view.height() - 1}) {
      for (int x = 0; x < view.width(); ++x) {
        margin_drawn = margin_drawn || view.GetPixel(x, y) != 0;
      }
    }
  };
  for (int frame = 10; frame <= 430; frame += 30) {
    player().At(frame).Do(check);
  }

  ShowCredits();

  EXPECT_FALSE(margin_drawn);
}

TEST_F(CreditsRollTest, ASlideIsBehindTheText) {
  WriteGameFile("credits.txt", kOneCredit);
  WriteSlide(kFirstSlide);
  int behind = -1;
  player().At(100).Do(
      [&behind] { behind = TheScreen().visible_view().GetPixel(5, 200); });

  ShowCredits();

  EXPECT_EQ(behind, kSlideColor);
}

// A slide's pixels are copied in while its colours are black, fade up, hold,
// and fade back to black before the next slide's pixels arrive.
TEST_F(CreditsRollTest, ASlideFadesUpHoldsAndFadesOut) {
  WriteGameFile("credits.txt", kOneCredit);
  WriteSlide(kFirstSlide);
  int copied_in = -1;
  int fading_up = -1;
  int held = -1;
  int fading_out = -1;
  int gone = -1;
  player()
      .At(10)
      .Do([&copied_in] { copied_in = SlideBrightness(); })
      .At(30)
      .Do([&fading_up] { fading_up = SlideBrightness(); })
      .At(100)
      .Do([&held] { held = SlideBrightness(); })
      .At(170)
      .Do([&fading_out] { fading_out = SlideBrightness(); })
      .At(195)
      .Do([&gone] { gone = SlideBrightness(); });

  ShowCredits();

  EXPECT_EQ(copied_in, 0);
  EXPECT_GT(fading_up, 0);
  EXPECT_LT(fading_up, held);
  // The fade's steps stop at 216 of 255; see kFadeStep.
  EXPECT_GT(held, 200);
  EXPECT_LT(fading_out, held);
  EXPECT_GT(fading_out, 0);
  EXPECT_EQ(gone, 0);
}

TEST_F(CreditsRollTest, TheTextKeepsItsColourThroughTheFades) {
  WriteGameFile("credits.txt", kOneCredit);
  WriteSlide(kFirstSlide);
  WriteTextPalette();
  std::vector<int> text_brightness;
  for (const int frame : {10, 30, 100, 170, 195}) {
    player().At(frame).Do([&text_brightness] {
      text_brightness.push_back(
          PaletteClass::CurrentPalette.at(kTextColor).red());
    });
  }

  ShowCredits();

  EXPECT_EQ(text_brightness, (std::vector<int>{255, 255, 255, 255, 255}));
}

// When the text has gone the slide on screen is faded out at once rather
// than left to finish its time, and the screen is cleared.
TEST_F(CreditsRollTest, EndsOnABlackScreen) {
  WriteGameFile("credits.txt", kOneCredit);
  WriteSlide(kFirstSlide);
  WriteSlide("aly1.pcx");
  WriteSlide("apc_hi.pcx");

  ShowCredits();

  EXPECT_EQ(SlideBrightness(), 0);
  EXPECT_EQ(TheScreen().visible_view().GetPixel(5, 200), 0);
  EXPECT_TRUE(TextRows().empty());
}

TEST_F(CreditsRollTest, EscEndsItEarly) {
  WriteGameFile("credits.txt", "Producer\tEd\rArtist\tJoe\r");
  WriteSlide(kFirstSlide);
  player().At(100).Key(engine::window::KN_ESC);

  ShowCredits();

  // The roll stops at once; what follows is the two-second fade to black.
  EXPECT_LT(player().frame(), 100 + 150);
  EXPECT_EQ(SlideBrightness(), 0);
  EXPECT_EQ(TheScreen().visible_view().GetPixel(5, 200), 0);
}

TEST_F(CreditsRollTest, AnyOtherKeyOnlyHurriesAFrame) {
  WriteGameFile("credits.txt", kOneCredit);
  player().At(100).Key(engine::window::KN_SPACE);

  ShowCredits();

  EXPECT_GT(player().frame(), 400);
}

TEST_F(CreditsRollTest, PlaysItsMusicAtFortyPercentWhenTheScoreIsOff) {
  WriteGameFile("credits.txt", kOneCredit);
  TheOptions().ScoreVolume = Fixed(0);
  // A value neither test expects.
  Fixed during(1);
  player().At(100).Do([&during] { during = TheOptions().ScoreVolume; });

  ShowCredits();

  EXPECT_EQ(during, Fixed(4, 10));
  EXPECT_EQ(TheOptions().ScoreVolume, Fixed(0));
}

TEST_F(CreditsRollTest, LeavesAScoreVolumeThatIsOnAlone) {
  WriteGameFile("credits.txt", kOneCredit);
  TheOptions().ScoreVolume = Fixed(1, 2);
  // A value neither test expects.
  Fixed during(1);
  player().At(100).Do([&during] { during = TheOptions().ScoreVolume; });

  ShowCredits();

  EXPECT_EQ(during, Fixed(1, 2));
  EXPECT_EQ(TheOptions().ScoreVolume, Fixed(1, 2));
}

TEST_F(CreditsRollTest, DrawsInGreenAndHandsBackTheDialogScheme) {
  WriteGameFile("credits.txt", kOneCredit);
  const RemapControlType* during = nullptr;
  player().At(100).Do([&during] { during = Get_Color_Scheme(); });

  ShowCredits();

  EXPECT_EQ(during, &ThePalettes().color_remaps().at(PCOLOR_GREEN));
  EXPECT_EQ(Get_Color_Scheme(),
            &ThePalettes().color_remaps().at(PCOLOR_DIALOG_BLUE));
}

}  // namespace
}  // namespace ra
