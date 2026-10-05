/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// The scrolling credits: CREDITS.TXT rolls up the screen like a film's end
// titles while a slideshow of stills fades in and out behind it.
#include "ra/screens/egos.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <memory>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "base/array.h"
#include "base/fixed.h"
#include "base/numeric.h"
#include "base/types.h"
#include "engine/file/game_file.h"
#include "engine/gfx/font.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/palette_color.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/wwstd.h"
#include "engine/platform/timer.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/ui/input.h"
#include "ra/assets/graphics_loader.h"
#include "ra/assets/palette.h"
#include "ra/assets/palettes.h"
#include "ra/assets/screen.h"
#include "ra/assets/theme.h"
#include "ra/core/defines.h"
#include "ra/screens/goptions.h"
#include "ra/startup/conquer.h"
#include "ra/ui/color_scheme.h"
#include "ra/ui/dialog.h"

namespace ra {

using enum engine::window::KeyNumber;

// One entry per string of credits text still on its way up the screen, in file
// order. Empty outside ShowCredits().
static std::vector<std::unique_ptr<CreditLine>> credit_lines;

// Number of slideshow pictures.
constexpr int kSlideCount = 17;

// Slide frames from the start of a slide to the start of its fade-out. One
// slide frame passes per pixel of scrolling.
constexpr int kSlideHoldFrames = 150;

// Slide frames a fade in or out takes.
constexpr int kFadeFrames = 37;

// The slideshow pictures, in the order they are shown; the show loops.
static constexpr std::array<std::string_view, kSlideCount> kSlideNames = {
    "aftr_hi.pcx",  "aly1.pcx",     "apc_hi.pcx",   "aphi0049.pcx",
    "bnhi0020.pcx", "dchi0040.pcx", "frhi0166.pcx", "lab.pcx",
    "landsbrg.pcx", "mahi0107.pcx", "mig_hi.pcx",   "mtfacthi.pcx",
    "needle.pcx",   "sov2.pcx",     "spy.pcx",      "stalin.pcx",
    "tent.pcx"};

// Each slide's own palette, as loaded from its picture.
static std::array<engine::gfx::Palette, kSlideCount> slide_palettes;

// The slides, decoded to screen-sized pages. All are loaded up front so that
// no disk access stalls the scroll.
static std::array<std::unique_ptr<PixelBuffer>, kSlideCount> slide_pages;

// The current slide without the text on it: what a line of text is erased
// with before it is drawn one pixel higher.
static std::unique_ptr<PixelBuffer> background_page;

// The font's colours plus the current slide's colours: the palette a slide
// fades up to.
static PaletteClass combo_palette;

// Non-zero for the palette entries that belong to the slides and fade with
// them; zero for the ones the text is drawn in, which never fade.
static std::array<char, 256> fades_with_slide;

// A new slide is copied in strips of this height, one strip per frame, so
// that the copy does not make the scroll stutter. Four strips cover the 400
// rows of a slide.
constexpr int kStripHeight = 100;

CreditLine::CreditLine(int x, int y, std::string_view text, FontType font,
                       TextPrintType flags)
    : text_(text), x_(x), y_(y), font_(font), flags_(flags) {}

bool CreditLine::Scroll(int pixels) {
  y_ -= pixels;
  // A line is 16 pixels high at most, and the top 16 rows of the hidden page
  // are never shown, so by now the line is long out of sight.
  return y_ < -20;
}

void CreditLine::Render(PixelView& view) const {
  // The text printer does not clip vertically, so a line is only drawn while
  // all of it is on the page.
  if (y_ < view.height() && y_ > -16) {
    Fancy_Text_Print(view, text_.c_str(), x_, y_, Get_Color_Scheme(), kTBlack,
                     font_, flags_);
  }
}

void CreditLine::Erase(PixelView& view, PixelView& background) const {
  const int width =
      StringPixelWidth(TextFontStyle(font_, flags_), text_.c_str());
  // Find the left edge the way the text printer does.
  int x = x_;

  if (base::Any(flags_ & TPF_RIGHT)) {
    x -= width;
  } else {
    if (base::Any(flags_ & TPF_CENTER)) {
      x -= width / 2;
    }
  }

  // One pixel wider on each side and one row taller than the 14-pixel text,
  // for the drop shadow. The blit clips, so a line that is still below the
  // page or already above it costs nothing.
  background.BlitTo(view, x - 1, y_, x - 1, y_, width + 2, 14 + 1);
}

// Makes `palette` the displayed palette at once, with no fade.
static void Set_Pal(const PaletteClass& palette) { Set_Palette(palette); }

// Advances the slideshow behind the text by one step. `slide` is the picture
// and `slide_frame` how long it has been current, which decides what happens:
//
//   1-4      copy the slide to the background page, a strip per frame
//   5-8      copy the background page to the hidden page the same way
//   9        put the slide's colours into the combo palette
//   11-46    fade the slide's colours up from black
//   151-186  fade them back down to black
//
// Nothing is displayed until the fade starts, because the slide's palette
// entries are black while its pixels are copied in.
static void StepSlideShow(int slide, int slide_frame) {
  if (slide_frame >= 1 && slide_frame <= 4) {
    PixelView& slide_view = slide_pages.at(base::ToSize(slide))->view();
    slide_view.BlitTo(background_page->view(), 0,
                      (slide_frame - 1) * kStripHeight, 0,
                      (slide_frame - 1) * kStripHeight,
                      TheScreen().visible_view().width(), kStripHeight);
    return;
  }

  if (slide_frame >= 5 && slide_frame <= 8) {
    background_page->view().BlitTo(
        TheScreen().hidden_view(), 0, (slide_frame - 5) * kStripHeight, 0,
        (slide_frame - 5) * kStripHeight, TheScreen().visible_view().width(),
        kStripHeight);
    return;
  }

  if (slide_frame == 9) {
    for (int index = 0; index < 256; index++) {
      if (fades_with_slide.at(base::ToSize(index))) {
        combo_palette.at(index) =
            slide_palettes.at(base::ToSize(slide)).at(index);
      }
    }
    return;
  }

  if (slide_frame > 10 && slide_frame < kFadeFrames + 10) {
    // Each step is computed from the unfaded title palette, which is why it
    // is put back afterwards; only the last step keeps its result.
    //
    // TODO: The integer division makes a step 6/255, so the last one reaches
    // 216/255 and the slide is shown at 85% of its brightness.
    const PaletteClass unfaded_palette = ThePalettes().title_palette();
    ThePalettes().title_palette().Partial_Adjust(
        std::min(255 / kFadeFrames * (slide_frame - 10), 255), combo_palette,
        fades_with_slide);
    Set_Pal(ThePalettes().title_palette());
    if (slide_frame != 9 + kFadeFrames) {
      ThePalettes().title_palette() = unfaded_palette;
    } else {
      ThePalettes().title_palette() = PaletteClass::CurrentPalette;
    }
    return;
  }

  if (slide_frame > kSlideHoldFrames &&
      slide_frame < kSlideHoldFrames + kFadeFrames) {
    const PaletteClass unfaded_palette = ThePalettes().title_palette();
    ThePalettes().title_palette().Partial_Adjust(
        std::min(255 / kFadeFrames * (slide_frame - kSlideHoldFrames), 255),
        fades_with_slide);
    if (slide_frame != kSlideHoldFrames + kFadeFrames - 1) {
      Set_Pal(ThePalettes().title_palette());
      ThePalettes().title_palette() = unfaded_palette;
    } else {
      // The steps stop short of black (see the fade up), so the last one
      // blacks out the slide's entries outright. The next slide's pixels can
      // then be copied in unseen.
      for (int index = 0; index < 256; index++) {
        if (fades_with_slide.at(base::ToSize(index))) {
          ThePalettes().title_palette().at(index) = engine::gfx::kBlackColor;
        }
      }
      Set_Pal(ThePalettes().title_palette());
    }
  }
}

void ShowCredits() {
  engine::window::KeyNumber key = KN_NONE;

  // System ticks (1/60 s) per pixel of scrolling.
  static constexpr int kTicksPerPixel = 3;

  // CREDITS.TXT is laid out as an 80-column page, and a string's place on
  // that page decides how it is drawn:
  //
  //   it straddles column 40      centred on the screen
  //   it ends before column 40    right-justified at its last column
  //   it starts after column 40   left-justified at its first column
  //
  // which gives the two-column "job  name" look. A string ends at a tab, at
  // the end of the line or at two spaces in a row.
  const auto credits_file = OpenGameFile("credits.txt");
  if (!credits_file) {
    return;
  }
  std::string credits(base::ToSize(credits_file->Size()), '\0');
  if (credits_file->Read(std::as_writable_bytes(std::span(credits))) <= 0) {
    return;
  }

  Set_Color_Scheme(&ThePalettes().color_remaps().at(PCOLOR_GREEN));

  // The credits are drawn to the hidden page and blitted forward a frame at a
  // time, so that the text never appears half-scrolled on screen.
  PixelView& view = TheScreen().hidden_view();

  // `bytes_left` counts the bytes from `cursor` to the end of the file. Columns
  // and lines count from zero.
  int bytes_left = static_cast<int>(credits_file->Size());
  int line = 0;
  int column = 0;
  auto cursor = credits.begin();
  char previous_char = 0;
  auto string_begin = credits.begin();
  auto string_end = credits.begin();
  bool string_ended = false;
  int start_column = 0;
  int end_column = 0;
  int x = 0;
  const int y = TheScreen().visible_view().height() + 2;
  // Every line starts two rows below the bottom of the screen, 16 rows
  // apart, and scrolls up from there.
  do {
    char ch = *cursor++;
    bytes_left--;

    switch (ch) {
      case 13:
        line++;
        column = 0;
        break;

      case 10:
        // The carriage return already ended the line.
        break;

      case 32:
        column++;
        break;

        // Tab stops are every 8 columns.
      case 9:
        column += 8;
        column -= column % 8;
        break;

      default:
        // The start of a string. Find where it ends: `string_end` stops one
        // past the character that ended it, which is left in `ch`.
        previous_char = ch;
        string_begin = cursor - 1;
        string_end = cursor - 1;
        end_column = start_column = column;
        string_ended = false;

        do {
          ch = *string_end++;
          switch (ch) {
            case 9:
            case 10:
            case 13:
              string_ended = true;
              break;

            case 32:
              if (previous_char == 32) {
                string_ended = true;
              }
              end_column++;
              break;

            default:
              end_column++;
          }
          if (string_end >= cursor + bytes_left) {
            string_ended = true;
          }

          previous_char = ch;
        } while (!string_ended);

        // TODO: A string that runs into the end of the file is dropped, so
        // the last line of credits is lost unless a CR and LF follow it.
        if (string_end >= cursor + bytes_left) {
          break;
        }

        // Ended by two spaces: step back so that neither is in the string.
        //
        // TODO: This also fires for one space before a tab or line end. The
        // end column is then one too low, and after a CR the line counter
        // advances twice, because `cursor` is left on the CR and the outer loop
        // counts it again.
        if (*(string_end - 2) == 32) {
          string_end--;
          end_column -= 2;
        }

        TextPrintType flags = TPF_USE_GRAD_PAL | TPF_DROPSHADOW;

        if (start_column < 40 && end_column > 40) {
          flags = flags | TPF_CENTER;
          x = TheScreen().visible_view().width() / 2;
        } else {
          if (start_column < 40) {
            flags = flags | TPF_RIGHT;
            x = end_column * TheScreen().visible_view().width() / 80;
          } else {
            x = start_column * TheScreen().visible_view().width() / 80;
          }
        }

        // The string is everything before the character that ended it.
        credit_lines.push_back(std::make_unique<CreditLine>(
            x, y + (line * 16), std::string_view(string_begin, string_end - 1),
            FontType::k6PointGradient, flags));

        // Carry on after the string. The outer loop has already counted its
        // first character.
        cursor = string_end;
        column = static_cast<int>(column + (string_end - string_begin));
        bytes_left =
            static_cast<int>(bytes_left - (string_end - string_begin - 1));

        // The terminator was consumed with the string, so do here what the
        // outer loop would have done for it.
        if (ch == 13) {
          line++;
          column = 0;
        } else {
          if (ch == 9) {
            column += 7;
            column -= column % 8;
          }
        }
        break;
    }

  } while (bytes_left > 0);

  // Everything fades with the slides except the colours the text is drawn
  // in: the six gradient entries of the font and the green scheme's own.
  fades_with_slide.fill(1);
  const PlayerColorType pcolor = PCOLOR_GREEN;

  for (int index = 0; index < 6; index++) {
    fades_with_slide.at(base::At(
        ThePalettes().color_remaps().at(pcolor).FontRemap, 10 + index)) = 0;
  }
  fades_with_slide.at(ThePalettes().color_remaps().at(pcolor).Color) = 0;
  fades_with_slide.at(ThePalettes().color_remaps().at(pcolor).Shadow) = 0;
  fades_with_slide.at(ThePalettes().color_remaps().at(pcolor).Background) = 0;
  fades_with_slide.at(ThePalettes().color_remaps().at(pcolor).Corners) = 0;
  fades_with_slide.at(ThePalettes().color_remaps().at(pcolor).Highlight) = 0;
  fades_with_slide.at(ThePalettes().color_remaps().at(pcolor).Bright) = 0;
  fades_with_slide.at(ThePalettes().color_remaps().at(pcolor).Underline) = 0;
  fades_with_slide.at(ThePalettes().color_remaps().at(pcolor).Bar) = 0;
  fades_with_slide.at(ThePalettes().color_remaps().at(pcolor).Box) = 0;

  TheTheme().Stop();

  ThePalettes().black_palette().Set(kTimerSecond * 2, ServiceRealTime);

  // EGOPAL.PAL holds the colours of the text; the slides were drawn to leave
  // those entries alone.
  if (const auto file = OpenGameFile("EGOPAL.PAL")) {
    std::array<uint8_t, engine::gfx::Palette::kFileSize> vga{};
    if (file->ReadObject(vga)) {
      ThePalettes().title_palette() = engine::gfx::Palette::FromVga(vga);
    }
  }

  // Start with the slide entries black, so the text can scroll on a black
  // screen until the first slide fades up.
  combo_palette = ThePalettes().title_palette();

  for (int index = 0; index < 256; index++) {
    if (fades_with_slide.at(base::ToSize(index))) {
      combo_palette.at(index) = engine::gfx::kBlackColor;
    }
  }

  // The scroll never touches the top and bottom 16 rows of the visible page.
  TheScreen().visible_view().Clear();
  TheScreen().hidden_view().Clear();

  ThePalettes().title_palette() = combo_palette;
  ThePalettes().title_palette().Set();

  for (int index = 0; index < kSlideCount; index++) {
    auto buffer = std::make_unique<PixelBuffer>();
    buffer->Init(TheScreen().visible_view().width(),
                 TheScreen().visible_view().height(), {}, 0);
    Load_Title_Screen(kSlideNames.at(base::ToSize(index)), &buffer->view(),
                      slide_palettes.at(base::ToSize(index)));
    slide_pages.at(base::ToSize(index)) = std::move(buffer);
  }

  // The background page starts as a copy of the cleared screen, so the text
  // can be erased from the first frame on.
  background_page = std::make_unique<PixelBuffer>();
  background_page->Init(TheScreen().visible_view().width(),
                        TheScreen().visible_view().height(), {}, 0);

  TheScreen().visible_view().BlitTo(background_page->view());

  // Drop the keys pressed so far; one of them would end the credits.
  game::TheKeyboard().Clear();

  // The credits always have music: with the score turned off, play it at
  // 40% until they end.
  const Fixed saved_score_volume = TheOptions().ScoreVolume;
  if (saved_score_volume == 0) {
    TheOptions().Set_Score_Volume(Fixed(4, 10), false);
  }
  TheTheme().Queue_Song(THEME_CREDITS);

  // `frame` counts pixels scrolled since `start_ticks`; `slide_frame` restarts
  // with every slide.
  const int64_t start_ticks = engine::platform::SystemTicks();
  int frame = 0;
  int slide_frame = 0;
  int slide = 0;

  game::TheMouse().Hide();

  // Scroll one pixel per pass until every line has left the top of the
  // screen or Esc is pressed.
  while (!credit_lines.empty()) {
    frame++;

    // A slide stays for 150 frames, fades for 37 and leaves the screen black
    // until frame 200. Then the next one starts, the first following the
    // last.
    slide_frame++;

    if (slide_frame > kSlideHoldFrames + 50) {
      if (slide < kSlideCount - 1) {
        slide++;
        slide_frame = 0;
      } else {
        slide = 0;
        slide_frame = 0;
      }
    }

    StepSlideShow(slide, slide_frame);

    // Erase every line where it was drawn and move it up. A line that has
    // left the screen is removed, which ends the pass early: the lines not
    // reached are the ones above it, which are off the screen as well and
    // go the same way in the next passes.
    for (base::ssize i = std::ssize(credit_lines) - 1; i >= 0; i--) {
      credit_lines.at(base::ToSize(i))->Erase(view, background_page->view());
      if (credit_lines.at(base::ToSize(i))->Scroll(1)) {
        credit_lines.erase(std::next(credit_lines.begin(), i));
        break;
      }
    }

    if (view.Lock()) {
      for (base::ssize i = std::ssize(credit_lines) - 1; i >= 0; i--) {
        credit_lines.at(base::ToSize(i))->Render(view);
      }
      view.Unlock();
    }

    // Play the credits score again when it runs out.
    if (frame > 1000 && !TheTheme().Still_Playing()) {
      TheTheme().Queue_Song(THEME_CREDITS);
    }

    ServiceRealTime();

    // Wait for this frame's time. The deadline is measured from the start,
    // so a slow frame is made up for by the next ones. Any key cuts the wait
    // short.
    while (engine::platform::SystemTicks() - start_ticks <
               static_cast<int64_t>(frame) * kTicksPerPixel &&
           !game::TheKeyboard().Peek()) {
    }

    // Show all but the top and bottom 16 rows of the hidden page. The text
    // printer does not clip vertically, so a line appears and disappears
    // whole; behind these margins it does so out of sight.
    TheScreen().hidden_view().BlitTo(TheScreen().visible_view(), 0, 16, 0, 16,
                                     TheScreen().visible_view().width(),
                                     TheScreen().visible_view().height() - 32);

    key = KN_NONE;
    if (game::TheKeyboard().Peek()) {
      key = game::TheKeyboard().Read().key;
      if (key == KN_ESC) {
        break;
      }
    }
  }

  if (key == KN_ESC) {
    TheTheme().Fade_Out();
    ThePalettes().black_palette().Set(kTimerSecond * 2, ServiceRealTime);
  } else {
    // The text is gone: let the slide fade out. One that is being shown
    // skips straight to its fade; one that is still fading up finishes that
    // first.
    while (slide_frame <= kFadeFrames + kSlideHoldFrames) {
      if (slide_frame < kSlideHoldFrames && slide_frame > 10 + kFadeFrames) {
        slide_frame = kSlideHoldFrames;
      }
      frame++;
      slide_frame++;

      StepSlideShow(slide, slide_frame);

      ServiceRealTime();

      while (engine::platform::SystemTicks() - start_ticks <
                 static_cast<int64_t>(frame) * kTicksPerPixel &&
             !game::TheKeyboard().Peek()) {
      }
    }
  }

  TheScreen().visible_view().Clear();

  game::TheMouse().Show();

  Set_Color_Scheme(&ThePalettes().color_remaps().at(PCOLOR_DIALOG_BLUE));

  TheTheme().Stop();
  TheOptions().Set_Score_Volume(saved_score_volume, false);

  slide_pages = {};

  background_page.reset();

  credit_lines.clear();
}

}  // namespace ra
