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
#include <vector>

#include "base/array.h"
#include "base/buffer.h"
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
// order. Empty outside Show_Who_Was_Responsible().
static std::vector<std::unique_ptr<EgoClass>> EgoList;

// Number of slideshow pictures.
#define NUM_SLIDES 17

// Slide frames from the start of a slide to the start of its fade-out. One
// slide frame passes per pixel of scrolling.
#define FRAME_DELAY 150

// Slide frames a fade in or out takes.
#define FADE_DELAY 37

// The slideshow pictures, in the order they are shown; the show loops.
static char SlideNames[NUM_SLIDES][13] = {

    "aftr_hi.pcx",  "aly1.pcx",     "apc_hi.pcx",   "aphi0049.pcx",
    "bnhi0020.pcx", "dchi0040.pcx", "frhi0166.pcx", "lab.pcx",
    "landsbrg.pcx", "mahi0107.pcx", "mig_hi.pcx",   "mtfacthi.pcx",
    "needle.pcx",   "sov2.pcx",     "spy.pcx",      "stalin.pcx",
    "tent.pcx"};

// Each slide's own palette, as loaded from its picture.
static engine::gfx::Palette SlidePals[NUM_SLIDES];

// The slides, decoded to screen-sized pages. All are loaded up front so that
// no disk access stalls the scroll.
static std::unique_ptr<PixelBuffer> SlideBuffers[NUM_SLIDES];

// The current slide without the text on it: what a line of text is erased
// with before it is drawn one pixel higher.
static std::unique_ptr<PixelBuffer> BackgroundPage;

// The font's colours plus the current slide's colours: the palette a slide
// fades up to.
static PaletteClass ComboPalette;

// Non-zero for the palette entries that belong to the slides and fade with
// them; zero for the ones the text is drawn in, which never fade.
static char PaletteLUT[256];

// A new slide is copied in strips of this height, one strip per frame, so
// that the copy does not make the scroll stutter. Four strips cover the 400
// rows of a slide.
#define CHUNK_HEIGHT 100

EgoClass::EgoClass(int x, int y, std::string_view text, FontType font,
                   TextPrintType flags)
    : Text(text), XPos(x), YPos(y), TextFont(font), Flags(flags) {}

bool EgoClass::Scroll(int distance) {
  YPos -= distance;
  // A line is 16 pixels high at most, and the top 16 rows of the hidden page
  // are never shown, so by now the line is long out of sight.
  return YPos < -20;
}

void EgoClass::Render(PixelView& view) const {
  // The text printer does not clip vertically, so a line is only drawn while
  // all of it is on the page.
  if (YPos < view.height() && YPos > -16) {
    Fancy_Text_Print(view, Text.c_str(), XPos, YPos, Get_Color_Scheme(),
                     kTBlack, TextFont, Flags);
  }
}

void EgoClass::Wipe(PixelView& view, PixelBuffer* background) const {
  const int width =
      StringPixelWidth(TextFontStyle(TextFont, Flags), Text.c_str());
  // Find the left edge the way the text printer does.
  int x = XPos;

  if (base::Any(Flags & TPF_RIGHT)) {
    x -= width;
  } else {
    if (base::Any(Flags & TPF_CENTER)) {
      x -= width / 2;
    }
  }

  // One pixel wider on each side and one row taller than the 14-pixel text,
  // for the drop shadow. The blit clips, so a line that is still below the
  // page or already above it costs nothing.
  background->view().BlitTo(view, x - 1, YPos, x - 1, YPos, width + 2, 14 + 1);
}

// Makes `palette` the displayed palette at once, with no fade.
static void Set_Pal(const PaletteClass* palette) { Set_Palette(*palette); }

// Advances the slideshow behind the text by one step. `slide` is the picture
// and `frame` how long it has been current, which decides what happens:
//
//   1-4      copy the slide to the background page, a strip per frame
//   5-8      copy the background page to the hidden page the same way
//   9        put the slide's colours into the combo palette
//   11-46    fade the slide's colours up from black
//   151-186  fade them back down to black
//
// Nothing is displayed until the fade starts, because the slide's palette
// entries are black while its pixels are copied in.
static void Slide_Show(int slide, int frame) {
  if (frame >= 1 && frame <= 4) {
    PixelView& slide_view = base::At(SlideBuffers, slide)->view();
    slide_view.BlitTo(BackgroundPage->view(), 0, (frame - 1) * CHUNK_HEIGHT, 0,
                      (frame - 1) * CHUNK_HEIGHT,
                      TheScreen().visible_view().width(), CHUNK_HEIGHT);
    return;
  }

  if (frame >= 5 && frame <= 8) {
    BackgroundPage->view().BlitTo(
        TheScreen().hidden_view(), 0, (frame - 5) * CHUNK_HEIGHT, 0,
        (frame - 5) * CHUNK_HEIGHT, TheScreen().visible_view().width(),
        CHUNK_HEIGHT);
    return;
  }

  if (frame == 9) {
    for (int index = 0; index < 256; index++) {
      if (base::At(PaletteLUT, index)) {
        ComboPalette.at(index) = base::At(SlidePals, slide).at(index);
      }
    }
    return;
  }

  if (frame > 10 && frame < FADE_DELAY + 10) {
    // Each step is computed from the unfaded title palette, which is why it
    // is put back afterwards; only the last step keeps its result.
    //
    // TODO: The integer division makes a step 6/255, so the last one reaches
    // 216/255 and the slide is shown at 85% of its brightness.
    const PaletteClass save_palette = ThePalettes().title_palette();
    ThePalettes().title_palette().Partial_Adjust(
        std::min(255 / FADE_DELAY * (frame - 10), 255), ComboPalette,
        PaletteLUT);
    Set_Pal(&ThePalettes().title_palette());
    if (frame != 9 + FADE_DELAY) {
      ThePalettes().title_palette() = save_palette;
    } else {
      ThePalettes().title_palette() = PaletteClass::CurrentPalette;
    }
    return;
  }

  if (frame > FRAME_DELAY && frame < FRAME_DELAY + FADE_DELAY) {
    const PaletteClass save_palette = ThePalettes().title_palette();
    ThePalettes().title_palette().Partial_Adjust(
        std::min(255 / FADE_DELAY * (frame - FRAME_DELAY), 255), PaletteLUT);
    if (frame != FRAME_DELAY + FADE_DELAY - 1) {
      Set_Pal(&ThePalettes().title_palette());
      ThePalettes().title_palette() = save_palette;
    } else {
      // The steps stop short of black (see the fade up), so the last one
      // blacks out the slide's entries outright. The next slide's pixels can
      // then be copied in unseen.
      for (int index = 0; index < 256; index++) {
        if (base::At(PaletteLUT, index)) {
          ThePalettes().title_palette().at(index) = engine::gfx::kBlackColor;
        }
      }
      Set_Pal(&ThePalettes().title_palette());
    }
  }
}

void Show_Who_Was_Responsible() {
  int key = 0;

  // System ticks (1/60 s) per pixel of scrolling.
  static const int speed = 3;

  // CREDITS.TXT is laid out as an 80-column page, and a string's place on
  // that page decides how it is drawn:
  //
  //   it straddles column 40      centred on the screen
  //   it ends before column 40    right-justified at its last column
  //   it starts after column 40   left-justified at its first column
  //
  // which gives the two-column "job  name" look. A string ends at a tab, at
  // the end of the line or at two spaces in a row.
  //
  // The buffer is one byte longer than the file so that a string can be
  // terminated in place.
  const auto creditsfile = OpenGameFile("credits.txt");
  if (!creditsfile) {
    return;
  }
  std::vector<char> credits(base::ToSize(creditsfile->Size()) + 1);
  if (creditsfile->Read(std::as_writable_bytes(
          std::span(credits).first(credits.size() - 1))) <= 0) {
    return;
  }

  Set_Color_Scheme(&ThePalettes().color_remaps().at(PCOLOR_GREEN));

  // The credits are drawn to the hidden page and blitted forward a frame at a
  // time, so that the text never appears half-scrolled on screen.
  PixelView& view = TheScreen().hidden_view();

  // `length` counts the bytes from `cptr` to the end of the file. Columns
  // and lines count from zero.
  int length = static_cast<int>(creditsfile->Size());
  int line = 0;
  int column = 0;
  auto cptr = credits.begin();
  char lastchar = 0;
  char oldchar = 0;
  auto strstart = credits.begin();
  auto strparse = credits.begin();
  bool gotendstr = false;
  int startcolumn = 0;
  int endcolumn = 0;
  int x = 0;
  const int y = TheScreen().visible_view().height() + 2;
  // Every line starts two rows below the bottom of the screen, 16 rows
  // apart, and scrolls up from there.
  do {
    char ch = *cptr++;
    length--;

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
        // The start of a string. Find where it ends: `strparse` stops one
        // past the character that ended it, which is left in `ch`.
        lastchar = ch;
        strstart = cptr - 1;
        strparse = cptr - 1;
        endcolumn = startcolumn = column;
        gotendstr = false;

        do {
          ch = *strparse++;
          switch (ch) {
            case 9:
            case 10:
            case 13:
              gotendstr = true;
              break;

            case 32:
              if (lastchar == 32) {
                gotendstr = true;
              }
              endcolumn++;
              break;

            default:
              endcolumn++;
          }
          if (strparse >= cptr + length) {
            gotendstr = true;
          }

          lastchar = ch;
        } while (!gotendstr);

        // TODO: A string that runs into the end of the file is dropped, so
        // the last line of credits is lost unless a CR and LF follow it.
        if (strparse >= cptr + length) {
          break;
        }

        // Ended by two spaces: step back so that neither is in the string.
        //
        // TODO: This also fires for one space before a tab or line end. The
        // end column is then one too low, and after a CR the line counter
        // advances twice, because `cptr` is left on the CR and the outer loop
        // counts it again.
        if (*(strparse - 2) == 32) {
          strparse--;
          endcolumn -= 2;
        }

        TextPrintType flags = TPF_USE_GRAD_PAL | TPF_DROPSHADOW;

        if (startcolumn < 40 && endcolumn > 40) {
          flags = flags | TPF_CENTER;
          x = TheScreen().visible_view().width() / 2;
        } else {
          if (startcolumn < 40) {
            flags = flags | TPF_RIGHT;
            x = endcolumn * TheScreen().visible_view().width() / 80;
          } else {
            x = startcolumn * TheScreen().visible_view().width() / 80;
          }
        }

        // Terminate the string in place for as long as it takes to copy it.
        oldchar = *(strparse - 1);
        *(strparse - 1) = 0;

        EgoList.push_back(std::make_unique<EgoClass>(
            x, y + (line * 16), std::to_address(strstart),
            FontType::k6PointGradient, flags));

        *(strparse - 1) = oldchar;

        // Carry on after the string. The outer loop has already counted its
        // first character.
        cptr = strparse;
        column = static_cast<int>(column + (strparse - strstart));
        length = static_cast<int>(length - (strparse - strstart - 1));

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

  } while (length > 0);

  // Everything fades with the slides except the colours the text is drawn
  // in: the six gradient entries of the font and the green scheme's own.
  base::FillBytes(base::ObjectBytes(PaletteLUT), 1, sizeof(PaletteLUT));
  const PlayerColorType pcolor = PCOLOR_GREEN;

  for (int index = 0; index < 6; index++) {
    base::At(PaletteLUT,
             base::At(ThePalettes().color_remaps().at(pcolor).FontRemap,
                      10 + index)) = 0;
  }
  base::At(PaletteLUT, ThePalettes().color_remaps().at(pcolor).Color) = 0;
  base::At(PaletteLUT, ThePalettes().color_remaps().at(pcolor).Shadow) = 0;
  base::At(PaletteLUT, ThePalettes().color_remaps().at(pcolor).Background) = 0;
  base::At(PaletteLUT, ThePalettes().color_remaps().at(pcolor).Corners) = 0;
  base::At(PaletteLUT, ThePalettes().color_remaps().at(pcolor).Highlight) = 0;
  base::At(PaletteLUT, ThePalettes().color_remaps().at(pcolor).Bright) = 0;
  base::At(PaletteLUT, ThePalettes().color_remaps().at(pcolor).Underline) = 0;
  base::At(PaletteLUT, ThePalettes().color_remaps().at(pcolor).Bar) = 0;
  base::At(PaletteLUT, ThePalettes().color_remaps().at(pcolor).Box) = 0;

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
  ComboPalette = ThePalettes().title_palette();

  for (int index = 0; index < 256; index++) {
    if (base::At(PaletteLUT, index)) {
      ComboPalette.at(index) = engine::gfx::kBlackColor;
    }
  }

  // The scroll never touches the top and bottom 16 rows of the visible page.
  TheScreen().visible_view().Clear();
  TheScreen().hidden_view().Clear();

  ThePalettes().title_palette() = ComboPalette;
  ThePalettes().title_palette().Set();

  for (int index = 0; index < NUM_SLIDES; index++) {
    base::At(SlideBuffers, index) = std::make_unique<PixelBuffer>();
    base::At(SlideBuffers, index)
        ->Init(TheScreen().visible_view().width(),
               TheScreen().visible_view().height(), {}, 0);
    Load_Title_Screen(base::Suffix(base::At(SlideNames, index), 0).data(),
                      &base::At(SlideBuffers, index)->view(),
                      base::At(SlidePals, index));
  }

  // The background page starts as a copy of the cleared screen, so the text
  // can be erased from the first frame on.
  BackgroundPage = std::make_unique<PixelBuffer>();
  BackgroundPage->Init(TheScreen().visible_view().width(),
                       TheScreen().visible_view().height(), {}, 0);

  TheScreen().visible_view().BlitTo(BackgroundPage->view());

  // Drop the keys pressed so far; one of them would end the credits.
  game::TheKeyboard().Clear();

  // The credits always have music: with the score turned off, play it at
  // 40% until they end.
  const Fixed oldvolume = TheOptions().ScoreVolume;
  if (oldvolume == 0) {
    TheOptions().Set_Score_Volume(Fixed(4, 10), false);
  }
  TheTheme().Queue_Song(THEME_CREDITS);

  // `frame` counts pixels scrolled since `time`; `picture_frame` restarts
  // with every slide.
  const int time = static_cast<int>(engine::platform::SystemTicks());
  int frame = 0;
  int picture_frame = 0;
  int slide_number = 0;

  game::TheMouse().Hide();

  // Scroll one pixel per pass until every line has left the top of the
  // screen or Esc is pressed.
  while (!EgoList.empty()) {
    frame++;

    // A slide stays for 150 frames, fades for 37 and leaves the screen black
    // until frame 200. Then the next one starts, the first following the
    // last.
    picture_frame++;

    if (picture_frame > FRAME_DELAY + 50) {
      if (slide_number < NUM_SLIDES - 1) {
        slide_number++;
        picture_frame = 0;
      } else {
        slide_number = 0;
        picture_frame = 0;
      }
    }

    Slide_Show(slide_number, picture_frame);

    // Erase every line where it was drawn and move it up. A line that has
    // left the screen is removed, which ends the pass early: the lines not
    // reached are the ones above it, which are off the screen as well and
    // go the same way in the next passes.
    for (base::ssize i = std::ssize(EgoList) - 1; i >= 0; i--) {
      EgoList.at(base::ToSize(i))->Wipe(view, BackgroundPage.get());
      if (EgoList.at(base::ToSize(i))->Scroll(1)) {
        EgoList.erase(std::next(EgoList.begin(), i));
        break;
      }
    }

    if (view.Lock()) {
      for (base::ssize i = std::ssize(EgoList) - 1; i >= 0; i--) {
        EgoList.at(base::ToSize(i))->Render(view);
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
    while (engine::platform::SystemTicks() - time <
               static_cast<int64_t>(frame) * speed &&
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
    while (picture_frame <= FADE_DELAY + FRAME_DELAY) {
      if (picture_frame < FRAME_DELAY && picture_frame > 10 + FADE_DELAY) {
        picture_frame = FRAME_DELAY;
      }
      frame++;
      picture_frame++;

      Slide_Show(slide_number, picture_frame);

      ServiceRealTime();

      while (engine::platform::SystemTicks() - time <
                 static_cast<int64_t>(frame) * speed &&
             !game::TheKeyboard().Peek()) {
      }
    }
  }

  TheScreen().visible_view().Clear();

  game::TheMouse().Show();

  Set_Color_Scheme(&ThePalettes().color_remaps().at(PCOLOR_DIALOG_BLUE));

  TheTheme().Stop();
  TheOptions().Set_Score_Volume(oldvolume, false);

  for (int index = 0; index < NUM_SLIDES; index++) {
    base::At(SlideBuffers, index).reset();
  }

  BackgroundPage.reset();

  EgoList.clear();
}

}  // namespace ra
