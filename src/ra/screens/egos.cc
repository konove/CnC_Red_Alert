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
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/base/attributes.h"
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
#include "ra/screens/credits_text.h"
#include "ra/screens/goptions.h"
#include "ra/startup/conquer.h"
#include "ra/ui/color_scheme.h"
#include "ra/ui/dialog.h"

namespace ra {

using enum engine::window::KeyNumber;

namespace {

// The font of the credits. Its text is 14 rows high, and the lines of the
// file are drawn this many rows apart.
constexpr FontType kCreditFont = FontType::k6PointGradient;
constexpr int kLinePitch = 16;

// The top and bottom rows of the hidden page that are never shown. The text
// printer does not clip vertically, so a line appears and disappears whole;
// behind these margins it does so out of sight.
constexpr int kMarginRows = 16;

// System ticks (1/60 s) per pixel of scrolling.
constexpr int kTicksPerPixel = 3;

// Number of slideshow pictures.
constexpr int kSlideCount = 17;

// Slide frames from the start of a slide to the start of its fade-out. One
// slide frame passes per pixel of scrolling.
constexpr int kSlideHoldFrames = 150;

// Slide frames a fade in or out takes.
constexpr int kFadeFrames = 37;

// Slide frames the screen stays black between two slides, fade-out included.
constexpr int kSlideGapFrames = 50;

// A new slide is copied in strips of this height, one strip per frame, so
// that the copy does not make the scroll stutter. Four strips cover the 400
// rows of a slide.
constexpr int kStripHeight = 100;

// The slideshow pictures, in the order they are shown; the show loops.
constexpr std::array<std::string_view, kSlideCount> kSlideNames = {
    "aftr_hi.pcx",  "aly1.pcx",     "apc_hi.pcx",   "aphi0049.pcx",
    "bnhi0020.pcx", "dchi0040.pcx", "frhi0166.pcx", "lab.pcx",
    "landsbrg.pcx", "mahi0107.pcx", "mig_hi.pcx",   "mtfacthi.pcx",
    "needle.pcx",   "sov2.pcx",     "spy.pcx",      "stalin.pcx",
    "tent.pcx"};

// One string of credits text on its way up the screen.
class CreditLine {
 public:
  // `x` and `y` are where the text printer is given the string, in pixels on
  // the hidden page: `x` is the left edge, the centre or the right edge of
  // the text according to `flags`, `y` its top row.
  CreditLine(int x, int y, std::string_view text, TextPrintType flags);

  // Moves the text up one pixel. Returns true once it has left the top of
  // the screen for good.
  bool ScrollUp();

  // Draws the text on `view`, unless it is partly or wholly off the page.
  void Render(PixelView& view) const;

  // Erases the text from `view` by copying the rectangle it covers from
  // `background`, a page of the same size.
  void Erase(PixelView& view, PixelView& background) const;

  std::string text_;
  int x_;
  int y_;  // Negative once the text has started to leave the page.
  TextPrintType flags_;
};

CreditLine::CreditLine(int x, int y, std::string_view text, TextPrintType flags)
    : text_(text), x_(x), y_(y), flags_(flags) {}

bool CreditLine::ScrollUp() {
  y_--;
  // A line is 16 pixels high at most, and the top 16 rows of the hidden page
  // are never shown, so by now the line is long out of sight.
  return y_ < -20;
}

void CreditLine::Render(PixelView& view) const {
  // The text printer does not clip vertically, so a line is only drawn while
  // all of it is on the page.
  if (y_ < view.height() && y_ > -kMarginRows) {
    Fancy_Text_Print(view, text_.c_str(), x_, y_, Get_Color_Scheme(), kTBlack,
                     kCreditFont, flags_);
  }
}

void CreditLine::Erase(PixelView& view, PixelView& background) const {
  const int width =
      StringPixelWidth(TextFontStyle(kCreditFont, flags_), text_.c_str());
  // Find the left edge the way the text printer does.
  int x = x_;
  if (base::Any(flags_ & TPF_RIGHT)) {
    x -= width;
  } else if (base::Any(flags_ & TPF_CENTER)) {
    x -= width / 2;
  }

  // One pixel wider on each side and one row taller than the 14-pixel text,
  // for the drop shadow. The blit clips, so a line that is still below the
  // page or already above it costs nothing.
  background.BlitTo(view, x - 1, y_, x - 1, y_, width + 2, 14 + 1);
}

// The slideshow behind the text: the pictures, the page the text is erased
// with, and the palettes that fade each picture in and out while the colours
// of the text stay as they are.
//
// The show draws on the hidden page and fades by way of the title palette,
// which it takes over for as long as it lives.
class SlideShow {
 public:
  // Loads the pictures and shows the credits palette with every slide colour
  // black. `text_scheme` is the colour scheme the text is drawn in; its
  // colours never fade. The screen must be black already: loading takes a
  // moment, and the page for erasing text starts as a copy of it.
  explicit SlideShow(const RemapControlType& text_scheme);

  // Advances the show by one step. `slide` is the picture and `slide_frame`
  // how long it has been current, which decides what happens:
  //
  //   1-4      copy the slide to the background page, a strip per frame
  //   5-8      copy the background page to the hidden page the same way
  //   9        put the slide's colours into the combo palette
  //   11-46    fade the slide's colours up from black
  //   151-186  fade them back down to black
  //
  // Nothing is displayed until the fade starts, because the slide's palette
  // entries are black while its pixels are copied in.
  void Step(int slide, int slide_frame);

  // The current slide without the text on it: what a line of text is erased
  // with before it is drawn one pixel higher.
  PixelView& background() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return background_page_.view();
  }

  // Non-zero for the palette entries that belong to the slides and fade with
  // them; zero for the ones the text is drawn in, which never fade.
  std::array<char, 256> fades_with_slide_{};

  // The font's colours plus the current slide's colours: the palette a slide
  // fades up to.
  PaletteClass combo_palette_;

  // Each slide's own palette, as loaded from its picture.
  std::array<engine::gfx::Palette, kSlideCount> slide_palettes_;

  // The slides, decoded to screen-sized pages. All are loaded up front so
  // that no disk access stalls the scroll.
  std::array<PixelBuffer, kSlideCount> slide_pages_;

  PixelBuffer background_page_;
};

SlideShow::SlideShow(const RemapControlType& text_scheme) {
  // Everything fades with the slides except the colours the text is drawn
  // in: the six gradient entries of the font and the scheme's own.
  fades_with_slide_.fill(1);
  for (int index = 0; index < 6; index++) {
    fades_with_slide_.at(base::At(text_scheme.FontRemap, 10 + index)) = 0;
  }
  fades_with_slide_.at(text_scheme.Color) = 0;
  fades_with_slide_.at(text_scheme.Shadow) = 0;
  fades_with_slide_.at(text_scheme.Background) = 0;
  fades_with_slide_.at(text_scheme.Corners) = 0;
  fades_with_slide_.at(text_scheme.Highlight) = 0;
  fades_with_slide_.at(text_scheme.Bright) = 0;
  fades_with_slide_.at(text_scheme.Underline) = 0;
  fades_with_slide_.at(text_scheme.Bar) = 0;
  fades_with_slide_.at(text_scheme.Box) = 0;

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
  combo_palette_ = ThePalettes().title_palette();
  for (int index = 0; index < 256; index++) {
    if (fades_with_slide_.at(base::ToSize(index))) {
      combo_palette_.at(index) = engine::gfx::kBlackColor;
    }
  }
  ThePalettes().title_palette() = combo_palette_;
  ThePalettes().title_palette().Set();

  const int width = TheScreen().visible_view().width();
  const int height = TheScreen().visible_view().height();
  for (std::size_t index = 0; index < kSlideNames.size(); index++) {
    slide_pages_.at(index).Init(width, height, {}, 0);
    Load_Title_Screen(kSlideNames.at(index), &slide_pages_.at(index).view(),
                      slide_palettes_.at(index));
  }

  // The background page starts as a copy of the black screen, so the text
  // can be erased from the first frame on.
  background_page_.Init(width, height, {}, 0);
  TheScreen().visible_view().BlitTo(background_page_.view());
}

void SlideShow::Step(int slide, int slide_frame) {
  if (slide_frame >= 1 && slide_frame <= 4) {
    PixelView& slide_view = slide_pages_.at(base::ToSize(slide)).view();
    slide_view.BlitTo(background_page_.view(), 0,
                      (slide_frame - 1) * kStripHeight, 0,
                      (slide_frame - 1) * kStripHeight,
                      TheScreen().visible_view().width(), kStripHeight);
    return;
  }

  if (slide_frame >= 5 && slide_frame <= 8) {
    background_page_.view().BlitTo(
        TheScreen().hidden_view(), 0, (slide_frame - 5) * kStripHeight, 0,
        (slide_frame - 5) * kStripHeight, TheScreen().visible_view().width(),
        kStripHeight);
    return;
  }

  if (slide_frame == 9) {
    for (int index = 0; index < 256; index++) {
      if (fades_with_slide_.at(base::ToSize(index))) {
        combo_palette_.at(index) =
            slide_palettes_.at(base::ToSize(slide)).at(index);
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
        std::min(255 / kFadeFrames * (slide_frame - 10), 255), combo_palette_,
        fades_with_slide_);
    Set_Palette(ThePalettes().title_palette());
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
        fades_with_slide_);
    if (slide_frame != kSlideHoldFrames + kFadeFrames - 1) {
      Set_Palette(ThePalettes().title_palette());
      ThePalettes().title_palette() = unfaded_palette;
    } else {
      // The steps stop short of black (see the fade up), so the last one
      // blacks out the slide's entries outright. The next slide's pixels can
      // then be copied in unseen.
      for (int index = 0; index < 256; index++) {
        if (fades_with_slide_.at(base::ToSize(index))) {
          ThePalettes().title_palette().at(index) = engine::gfx::kBlackColor;
        }
      }
      Set_Palette(ThePalettes().title_palette());
    }
  }
}

// Waits until `frame` frames of scrolling have passed since `start_ticks`.
// The deadline is measured from the start, so a slow frame is made up for by
// the next ones. Any key cuts the wait short.
void WaitForFrame(int64_t start_ticks, int frame) {
  while (engine::platform::SystemTicks() - start_ticks <
             static_cast<int64_t>(frame) * kTicksPerPixel &&
         !game::TheKeyboard().Peek()) {
  }
}

}  // namespace

void ShowCredits() {
  const auto credits_file = OpenGameFile("credits.txt");
  if (!credits_file) {
    return;
  }
  std::string credits(base::ToSize(credits_file->Size()), '\0');
  if (credits_file->Read(std::as_writable_bytes(std::span(credits))) <= 0) {
    return;
  }

  RemapControlType& text_scheme = ThePalettes().color_remaps().at(PCOLOR_GREEN);
  Set_Color_Scheme(&text_scheme);

  // The credits are drawn to the hidden page and blitted forward a frame at a
  // time, so that the text never appears half-scrolled on screen.
  PixelView& view = TheScreen().hidden_view();
  const int width = TheScreen().visible_view().width();
  const int height = TheScreen().visible_view().height();

  // The strings still on their way up the screen, in file order. Every line
  // of the file starts two rows below the bottom of the screen.
  std::vector<CreditLine> lines;
  for (const CreditString& string : ParseCredits(credits)) {
    TextPrintType flags = TPF_USE_GRAD_PAL | TPF_DROPSHADOW;
    if (string.alignment == CreditAlignment::kCenter) {
      flags = flags | TPF_CENTER;
    } else if (string.alignment == CreditAlignment::kRight) {
      flags = flags | TPF_RIGHT;
    }
    lines.emplace_back(string.column * width / kCreditColumns,
                       height + 2 + (string.line * kLinePitch), string.text,
                       flags);
  }

  TheTheme().Stop();

  ThePalettes().black_palette().Set(kTimerSecond * 2, ServiceRealTime);

  // The scroll never touches the top and bottom rows of the visible page.
  TheScreen().visible_view().Clear();
  TheScreen().hidden_view().Clear();

  SlideShow slides(text_scheme);

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
  bool escaped = false;
  while (!lines.empty() && !escaped) {
    frame++;

    // A slide stays for 150 frames, fades for 37 and leaves the screen black
    // until frame 200. Then the next one starts, the first following the
    // last.
    slide_frame++;
    if (slide_frame > kSlideHoldFrames + kSlideGapFrames) {
      slide = (slide + 1) % kSlideCount;
      slide_frame = 0;
    }
    slides.Step(slide, slide_frame);

    // Erase every line where it was drawn and move it up. A line that has
    // left the screen is removed, which ends the pass early: the lines not
    // reached are the ones above it, which are off the screen as well and
    // go the same way in the next passes.
    for (base::ssize i = std::ssize(lines) - 1; i >= 0; i--) {
      CreditLine& line = lines.at(base::ToSize(i));
      line.Erase(view, slides.background());
      if (line.ScrollUp()) {
        lines.erase(std::next(lines.begin(), i));
        break;
      }
    }

    if (view.Lock()) {
      for (const CreditLine& line : std::views::reverse(lines)) {
        line.Render(view);
      }
      view.Unlock();
    }

    // Play the credits score again when it runs out.
    if (frame > 1000 && !TheTheme().Still_Playing()) {
      TheTheme().Queue_Song(THEME_CREDITS);
    }

    ServiceRealTime();
    WaitForFrame(start_ticks, frame);

    view.BlitTo(TheScreen().visible_view(), 0, kMarginRows, 0, kMarginRows,
                width, height - (2 * kMarginRows));

    if (game::TheKeyboard().Peek()) {
      escaped = game::TheKeyboard().Read().key == KN_ESC;
    }
  }

  if (escaped) {
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

      slides.Step(slide, slide_frame);

      ServiceRealTime();
      WaitForFrame(start_ticks, frame);
    }
  }

  TheScreen().visible_view().Clear();

  game::TheMouse().Show();

  Set_Color_Scheme(&ThePalettes().color_remaps().at(PCOLOR_DIALOG_BLUE));

  TheTheme().Stop();
  TheOptions().Set_Score_Volume(saved_score_volume, false);
}

}  // namespace ra
