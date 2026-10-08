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
#include "ra/screens/credits_roll.h"

#include <array>
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
#include "base/types.h"
#include "engine/file/game_file.h"
#include "engine/gfx/font.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/palette_color.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/wwstd.h"
#include "engine/platform/sleep.h"
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

namespace {

// The font of the credits and the height of its text in rows. A drop shadow
// adds one row below and one column on each side.
constexpr auto kCreditFont = FontType::k6PointGradient;
constexpr int kTextHeight = 14;

// The lines of CREDITS.TXT are drawn this many rows apart.
constexpr int kLinePitch = 16;

// The top and bottom rows of the hidden page that are never shown. The text
// printer does not clip vertically, so a line appears and disappears whole;
// behind these margins it does so out of sight.
constexpr int kMarginRows = 16;

// A line whose top row is above this one is out of sight for good.
constexpr int kGoneRow = -20;

// System ticks (1/60 s) per pixel of scrolling.
constexpr int kTicksPerPixel = 3;

// Frames of scrolling before a credits score that has run out is restarted.
constexpr int kReplayScoreAfterFrames = 1000;

// The slideshow pictures, in the order they are shown; the show loops.
constexpr int kSlideCount = 17;
constexpr std::array<std::string_view, kSlideCount> kSlideNames = {
    "aftr_hi.pcx",  "aly1.pcx",     "apc_hi.pcx",   "aphi0049.pcx",
    "bnhi0020.pcx", "dchi0040.pcx", "frhi0166.pcx", "lab.pcx",
    "landsbrg.pcx", "mahi0107.pcx", "mig_hi.pcx",   "mtfacthi.pcx",
    "needle.pcx",   "sov2.pcx",     "spy.pcx",      "stalin.pcx",
    "tent.pcx"};

// A new slide is copied in strips of this height, one strip per frame, so
// that the copy does not make the scroll stutter. The strips cover the 400
// rows of a slide.
constexpr int kStripHeight = 100;
constexpr int kStripCount = 4;

// A slide's timetable, in slide frames. One slide frame passes per pixel of
// scrolling, and the count restarts at 0 with every slide:
//
//   1-4      copy the slide to the background page, a strip per frame
//   5-8      copy the background page to the hidden page the same way
//   9        put the slide's colours into the combo palette
//   11-46    fade the slide's colours up from black
//   151-186  fade them back down to black
//   200      the last frame; the screen has been black since 186
//
// Nothing of a slide is displayed until its fade starts, because its palette
// entries are black while its pixels are copied in.
constexpr int kCopyToBackgroundFrame = 1;
constexpr int kCopyToPageFrame = kCopyToBackgroundFrame + kStripCount;
constexpr int kLoadColorsFrame = kCopyToPageFrame + kStripCount;
constexpr int kFadeInFrame = 10;
constexpr int kFadeOutFrame = 150;
constexpr int kLastSlideFrame = 200;

// A fade takes the steps 1 to kFadeFrames - 1, one per frame, each this much
// further from the starting colours.
//
// TODO: The integer division makes a step 6/255, so the last one reaches
// 216/255: a slide is shown at 85% of its brightness, and the fade down has
// to black out the colours at the end.
constexpr int kFadeFrames = 37;
constexpr int kFadeStep = 255 / kFadeFrames;

// The flags the text printer draws a string of the credits with.
constexpr TextPrintType PrintFlags(const CreditAlignment alignment) {
  constexpr TextPrintType flags = TPF_USE_GRAD_PAL | TPF_DROPSHADOW;
  if (alignment == CreditAlignment::kCenter) {
    return flags | TPF_CENTER;
  }
  if (alignment == CreditAlignment::kRight) {
    return flags | TPF_RIGHT;
  }
  return flags;
}

// One string of credits text on its way up the screen.
class CreditLine {
 public:
  // Places `string` on a page of the given size in pixels, as far below the
  // bottom of the page as its line is down the file.
  CreditLine(const CreditString& string, int page_width, int page_height);

  // Moves the text up one pixel. Returns true once it has left the top of
  // the screen for good.
  bool ScrollUp();

  // Draws the text on `view`, unless it is partly or wholly off the page.
  void Render(PixelView& view) const;

  // Erases the text from `view` by copying the rectangle it covers from
  // `background`, a page of the same size.
  void Erase(PixelView& view, PixelView& background) const;

 private:
  std::string text_;
  CreditAlignment alignment_;
  int x_;  // Where `alignment_` anchors the text: its left edge, centre or
           // right edge.
  int y_;  // The top row. Negative once the text has started to leave the
           // page.
};

CreditLine::CreditLine(const CreditString& string, const int page_width,
                       const int page_height)
    : text_(string.text),
      alignment_(string.alignment),
      x_(string.column * page_width / kCreditColumns),
      y_(page_height + 2 + (string.line * kLinePitch)) {}

bool CreditLine::ScrollUp() {
  y_--;
  return y_ < kGoneRow;
}

void CreditLine::Render(PixelView& view) const {
  // The text printer does not clip vertically, so a line is only drawn while
  // all of it is on the page.
  if (y_ < view.height() && y_ > -kMarginRows) {
    Fancy_Text_Print(view, text_.c_str(), x_, y_, Get_Color_Scheme(),
                     kTransparentBlack, kCreditFont, PrintFlags(alignment_));
  }
}

void CreditLine::Erase(PixelView& view, PixelView& background) const {
  // The text and the row of shadow below it.
  constexpr int height = kTextHeight + 1;
  // Most lines are still below the page; do not measure those.
  if (y_ >= view.height() || y_ + height <= 0) {
    return;
  }

  // Find the left edge the way the text printer does.
  const int width = StringPixelWidth(
      TextFontStyle(kCreditFont, PrintFlags(alignment_)), text_.c_str());
  int x = x_;
  if (alignment_ == CreditAlignment::kRight) {
    x -= width;
  } else if (alignment_ == CreditAlignment::kCenter) {
    x -= width / 2;
  }

  // One pixel wider on each side for the shadow.
  background.BlitTo(view, x - 1, y_, x - 1, y_, width + 2, height);
}

// The slideshow behind the text: the pictures, the page the text is erased
// with, and the palettes that fade each picture in and out while the colours
// of the text stay as they are.
//
// The show fades by way of the title palette, which it takes over for as long
// as it lives.
class SlideShow {
 public:
  // Loads the pictures into pages of the given size and shows the credits
  // palette with every slide colour black. `text_scheme` is the colour scheme
  // the text is drawn in; its colours never fade.
  SlideShow(const RemapControlType& text_scheme, int width, int height);

  // Moves the show on by one frame of the timetable above, going to the next
  // slide when the current one has had its time. `page` is the page the
  // slide is drawn on, under the text.
  void Advance(PixelView& page);

  // Brings the current slide's fade-out forward if the slide is being shown.
  // One that is still being copied in or fading up carries on as it was.
  void SkipToFadeOut();

  // True when the current slide has faded out and the screen is black.
  [[nodiscard]] bool faded_out() const {
    return slide_frame_ > kFadeOutFrame + kFadeFrames;
  }

  // The current slide without the text on it: what a line of text is erased
  // with before it is drawn one pixel higher.
  PixelView& background() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return background_page_.view();
  }

 private:
  // Gives every colour of `palette` that belongs to the slides the colour
  // `source` has there.
  void SetSlideColors(engine::gfx::Palette& palette,
                      const engine::gfx::Palette& source) const;

  // Copies one strip of `from` to the same place on `to`.
  static void CopyStrip(PixelView& from, PixelView& to, int strip);

  int slide_ = 0;        // Index into kSlideNames.
  int slide_frame_ = 0;  // Frames the slide has been current.

  // Non-zero for the palette entries that belong to the slides and fade with
  // them; zero for the ones the text is drawn in, which never fade.
  std::array<char, engine::gfx::Palette::kColorCount> fades_with_slide_{};

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

SlideShow::SlideShow(const RemapControlType& text_scheme, const int width,
                     const int height) {
  // Everything fades with the slides except the colours the text is drawn
  // in: the six gradient entries of the font and the scheme's own.
  fades_with_slide_.fill(1);
  for (int index = 0; index < 6; index++) {
    base::At(fades_with_slide_, base::At(text_scheme.FontRemap, 10 + index)) =
        0;
  }
  for (const uint8_t color :
       {text_scheme.Color, text_scheme.Shadow, text_scheme.Background,
        text_scheme.Corners, text_scheme.Highlight, text_scheme.Bright,
        text_scheme.Underline, text_scheme.Bar, text_scheme.Box}) {
    base::At(fades_with_slide_, color) = 0;
  }

  // EGOPAL.PAL holds the colours of the text; the slides were drawn to leave
  // those entries alone.
  PaletteClass& title_palette = ThePalettes().title_palette();
  if (const auto file = OpenGameFile("EGOPAL.PAL")) {
    if (const auto palette = engine::gfx::ReadVgaPalette(*file)) {
      title_palette = *palette;
    }
  }

  // Start with the slide entries black, so the text can scroll on a black
  // screen until the first slide fades up.
  combo_palette_ = title_palette;
  SetSlideColors(combo_palette_,
                 engine::gfx::Palette(engine::gfx::kBlackColor));
  title_palette = combo_palette_;
  title_palette.Set();

  for (int index = 0; index < kSlideCount; index++) {
    PixelBuffer& page = base::At(slide_pages_, index);
    page.Init(width, height, {}, 0);
    Load_Title_Screen(base::At(kSlideNames, index), &page.view(),
                      base::At(slide_palettes_, index));
  }

  // Black, so the text can be erased before the first slide is copied in.
  background_page_.Init(width, height, {}, 0);
  background_page_.view().Clear();
}

void SlideShow::Advance(PixelView& page) {
  slide_frame_++;
  if (slide_frame_ > kLastSlideFrame) {
    slide_ = (slide_ + 1) % kSlideCount;
    slide_frame_ = 0;
  }

  if (slide_frame_ >= kCopyToBackgroundFrame &&
      slide_frame_ < kCopyToPageFrame) {
    CopyStrip(base::At(slide_pages_, slide_).view(), background_page_.view(),
              slide_frame_ - kCopyToBackgroundFrame);
    return;
  }

  if (slide_frame_ >= kCopyToPageFrame && slide_frame_ < kLoadColorsFrame) {
    CopyStrip(background_page_.view(), page, slide_frame_ - kCopyToPageFrame);
    return;
  }

  if (slide_frame_ == kLoadColorsFrame) {
    SetSlideColors(combo_palette_, base::At(slide_palettes_, slide_));
    return;
  }

  // Each step of a fade is computed from the title palette as it was when
  // the fade began; only the last step is kept in it.
  PaletteClass& title_palette = ThePalettes().title_palette();
  if (slide_frame_ > kFadeInFrame &&
      slide_frame_ < kFadeInFrame + kFadeFrames) {
    const int step = slide_frame_ - kFadeInFrame;
    PaletteClass faded = title_palette;
    faded.Partial_Adjust(kFadeStep * step, combo_palette_, fades_with_slide_);
    Set_Palette(faded);
    if (step == kFadeFrames - 1) {
      title_palette = faded;
    }
    return;
  }

  if (slide_frame_ > kFadeOutFrame &&
      slide_frame_ < kFadeOutFrame + kFadeFrames) {
    const int step = slide_frame_ - kFadeOutFrame;
    PaletteClass faded = title_palette;
    faded.Partial_Adjust(kFadeStep * step, fades_with_slide_);
    if (step == kFadeFrames - 1) {
      // The steps stop short of black, so the last one blacks out the
      // slide's entries outright. The next slide's pixels can then be copied
      // in unseen.
      SetSlideColors(faded, engine::gfx::Palette(engine::gfx::kBlackColor));
      title_palette = faded;
    }
    Set_Palette(faded);
  }
}

void SlideShow::SkipToFadeOut() {
  if (slide_frame_ > kFadeInFrame + kFadeFrames &&
      slide_frame_ < kFadeOutFrame) {
    slide_frame_ = kFadeOutFrame;
  }
}

void SlideShow::SetSlideColors(engine::gfx::Palette& palette,
                               const engine::gfx::Palette& source) const {
  for (int index = 0; index < engine::gfx::Palette::kColorCount; index++) {
    if (base::At(fades_with_slide_, index)) {
      palette.at(index) = source.at(index);
    }
  }
}

void SlideShow::CopyStrip(PixelView& from, PixelView& to, const int strip) {
  const int y = strip * kStripHeight;
  from.BlitTo(to, 0, y, 0, y, from.width(), kStripHeight);
}

// Waits until `frame` frames of scrolling have passed since `start_ticks`.
// The deadline is measured from the start, so a slow frame is made up for by
// the next ones. Any key cuts the wait short.
void WaitForFrame(const int64_t start_ticks, const int frame) {
  while (engine::platform::SystemTicks() - start_ticks <
             static_cast<int64_t>(frame) * kTicksPerPixel &&
         !game::TheKeyboard().Peek()) {
    engine::platform::SleepMs(1);
  }
}

}  // namespace

void ShowCredits() {
  const auto credits_file = OpenGameFile("credits.txt");
  if (!credits_file) {
    return;
  }
  const std::string credits = credits_file->ReadString(credits_file->Size());
  if (credits.empty()) {
    return;
  }

  RemapControlType& text_scheme = ThePalettes().color_remaps().at(PCOLOR_GREEN);
  Set_Color_Scheme(&text_scheme);

  // The credits are drawn to the hidden page and blitted forward a frame at a
  // time, so that the text never appears half-scrolled on screen.
  PixelView& view = TheScreen().hidden_view();
  const int width = TheScreen().visible_view().width();
  const int height = TheScreen().visible_view().height();

  // The strings still on their way up the screen, in file order.
  std::vector<CreditLine> lines;
  for (const CreditString& string : ParseCredits(credits)) {
    lines.emplace_back(string, width, height);
  }

  TheTheme().Stop();

  ThePalettes().black_palette().Set(kTimerSecond * 2, ServiceRealTime);

  // The scroll never touches the top and bottom rows of the visible page.
  TheScreen().visible_view().Clear();
  view.Clear();

  SlideShow slides(text_scheme, width, height);

  // Drop the keys pressed so far; one of them would end the credits.
  game::TheKeyboard().Clear();

  // The credits always have music: with the score turned off, play it at
  // 40% until they end.
  const Fixed saved_score_volume = TheOptions().ScoreVolume;
  if (saved_score_volume == 0) {
    TheOptions().Set_Score_Volume(Fixed(4, 10), false);
  }
  TheTheme().Queue_Song(THEME_CREDITS);

  // `frame` counts the pixels scrolled since `start_ticks`.
  const int64_t start_ticks = engine::platform::SystemTicks();
  int frame = 0;

  game::TheMouse().Hide();

  // Scroll one pixel per pass until every line has left the top of the
  // screen or Esc is pressed.
  bool escaped = false;
  while (!lines.empty() && !escaped) {
    frame++;
    slides.Advance(view);

    // Erase every line where it was drawn and move it up. A line that has
    // left the screen is removed, which ends the pass early: the lines not
    // reached are the ones above it, which are off the screen as well and
    // go the same way in the next passes.
    for (base::ssize i = std::ssize(lines) - 1; i >= 0; i--) {
      CreditLine& line = base::At(lines, i);
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

    if (frame > kReplayScoreAfterFrames && !TheTheme().Still_Playing()) {
      TheTheme().Queue_Song(THEME_CREDITS);
    }

    ServiceRealTime();
    WaitForFrame(start_ticks, frame);

    view.BlitTo(TheScreen().visible_view(), 0, kMarginRows, 0, kMarginRows,
                width, height - (2 * kMarginRows));

    if (game::TheKeyboard().Peek()) {
      escaped = game::TheKeyboard().Read().key == engine::window::KN_ESC;
    }
  }

  if (escaped) {
    TheTheme().Fade_Out();
    ThePalettes().black_palette().Set(kTimerSecond * 2, ServiceRealTime);
  } else {
    // The text is gone: let the slide fade out.
    while (!slides.faded_out()) {
      slides.SkipToFadeOut();
      frame++;
      slides.Advance(view);

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
