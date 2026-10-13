/*
**	Command & Conquer(tm)
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

/* $Header:   F:\projects\c&c\vcs\code\dialog.cpv   2.17   16 Oct 1995 16:51:50
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : DIALOG.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : September 10, 1993 *
 *                                                                                             *
 *                  Last Update : May 18, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Clip_Text_Print -- Prints text with clipping and <TAB> support.
 ** Dialog_Box -- draws a dialog background box * Display_Place_Building --
 *Displays the "place building" dialog box.                       *
 *   Display_Select_Target -- Displays the "choose target" prompt. *
 *   Display_Status -- Display the player scenario status box. * Draw_Box --
 *Displays a highlighted box. * Fancy_Text_Print -- Prints text with a drop
 *shadow.                                       * Redraw_Needed -- Determine if
 *sidebar needs to be redrawn.                                * Render_Bar_Graph
 *-- Renders a specified bargraph.                                         *
 *   Simple_Text_Print -- Prints text with a drop shadow. * Window_Box -- Draws
 *a fancy box over the specified window.                                *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/ui/dialog.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <span>
#include <string>
#include <string_view>

#include "base/array.h"
#include "base/buffer.h"
#include "base/enum_array.h"
#include "base/flags.h"
#include "base/numeric.h"
#include "base/point.h"
#include "base/rect.h"
#include "base/strings/safe_string.h"
#include "engine/file/mix_archive.h"
#include "engine/gfx/font.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/text_window.h"
#include "game/ui/standard_colors.h"
#include "td/assets/assets.h"
#include "td/assets/screen.h"
#include "td/core/defines.h"
#include "td/data/text.h"
#include "td/startup/conquer.h"
#include "td/startup/game_state.h"

namespace td {

/***********************************************************************************************
 * Dialog_Box -- draws a dialog background box *
 *                                                                                             *
 * INPUT: * x,y,w,h      the usual *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/26/1995 BR : Created. *
 *=============================================================================================*/
void Dialog_Box(PixelView& view, int x, int y, int w, int h) {
  Draw_Box(view, x, y, w, h, BOXSTYLE_GREEN_BORDER, true);
}

/***********************************************************************************************
 * Draw_Box -- Displays a highlighted box. *
 *                                                                                             *
 *    This will draw a highlighted box to the logicpage. It can * optionally
 *fill the box with a color as well. This is a low level * function and thus, it
 *doesn't do any graphic mode color adjustments.                     *
 *                                                                                             *
 * INPUT:   x,y   -- Upper left corner of the box to be drawn (pixels). *
 *                                                                                             *
 *          w,h   -- Width and height of box (in pixels). *
 *                                                                                             *
 *          up    -- Is the box rendered in the "up" stated? *
 *                                                                                             *
 *          filled-- Is the box to be filled. *
 *                                                                                             *
 * OUTPUT:     none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/28/1991 JLB : Created. * 05/30/1992 JLB : Embedded color codes.
 ** 07/31/1992 JLB : Depressed option added. *
 *=============================================================================================*/

void Draw_Box(PixelView& view, int x, int y, int w, int h, BoxStyleEnum up,
              bool filled) {
  static const base::EnumArray<BoxStyleEnum, BoxStyleType, kBoxstyleCount>
      ButtonColors = {{

          // Filler, Shadow, Hilite, Corner colors

          {game::kLightGrey, game::kWhite, game::kGrey,
           game::kLightGrey},  // 0 Button is down.
          {game::kLightGrey, game::kGrey, game::kWhite,
           game::kLightGrey},  // 1 Button is up w/border.
          {game::kLightBlue, game::kBlue, game::kLightCyan,
           game::kLightBlue},  // 2 Raised blue.
          {game::kGrey, game::kWhite, game::kBlack,
           game::kGrey},  // 3 Button is disabled down.
          {game::kGrey, game::kBlack, game::kWhite,
           game::kLightGrey},  // 4 Button is disabled up.
          {game::kLightGrey, game::kGrey, game::kWhite,
           game::kLightGrey},  // 5 Button is up w/arrows.
          //{ CC_GREEN_BKGD, CC_LIGHT_GREEN, CC_GREEN_SHADOW,  CC_GREEN_CORNERS
          //},
          //// 6 Button is down.
          //{ CC_GREEN_BKGD, CC_GREEN_SHADOW, CC_LIGHT_GREEN,  CC_GREEN_CORNERS
          //},
          //// 7 Button is up w/border.
          {kCcGreenBkgd, game::kLightGrey, game::kBlack,
           game::kGrey},  // 6 Button is down.
          {kCcGreenBkgd, game::kBlack, game::kLightGrey,
           game::kGrey},  // 7 Button is up w/border.
          {game::kGrey, game::kWhite, game::kBlack,
           game::kGrey},  // 8 Button is disabled down.
          {game::kGrey, game::kBlack, game::kLightGrey,
           game::kGrey},  // 9 Button is disabled up.
          //{ BLACK,  CC_GREEN_BOX, CC_GREEN_BOX,  BLACK},	// 10 List box.
          //{ BLACK,  CC_GREEN_BOX, CC_GREEN_BOX,  BLACK},	// 11 Menu box.
          {game::kBlack, game::kLightGrey, game::kLightGrey,
           game::kBlack},  // 10 List box.
          {game::kBlack, game::kLightGrey, game::kLightGrey,
           game::kBlack},  // 11 Menu box.
      }};

  w--;
  h--;
  const BoxStyleType& style = ButtonColors.at(up);

  if (filled) {
    if (style.Filler == kCcGreenBkgd) {
      CC_Texture_Fill(view, MixArchive::RetrieveData("BTEXTURE.SHP"),
                      TheGameState().in_main_loop() ? 1 : 0, x, y, w, h);
    } else {
      view.FillRect(base::Rect::FromCorners(x, y, x + w, y + h), style.Filler);
    }
  }

  switch (up) {
    case BOXSTYLE_GREEN_BOX:
      view.DrawRect(base::Rect::FromCorners(x, y, x + w, y + h),
                    style.Highlight);
      break;

    case BOXSTYLE_GREEN_BORDER:
      view.DrawRect(base::Rect::FromCorners(x + 1, y + 1, x + w - 1, y + h - 1),
                    style.Highlight);
      break;

    case BoxStyleEnum::BOXSTYLE_DOWN:
    case BoxStyleEnum::BOXSTYLE_RAISED:
    case BoxStyleEnum::BOXSTYLE_BLUE_UP:
    case BoxStyleEnum::BOXSTYLE_DIS_DOWN:
    case BoxStyleEnum::BOXSTYLE_DIS_RAISED:
    case BoxStyleEnum::BOXSTYLE_RAISED_ARROW:
    case BoxStyleEnum::BOXSTYLE_GREEN_DOWN:
    case BoxStyleEnum::BOXSTYLE_GREEN_RAISED:
    case BoxStyleEnum::BOXSTYLE_GREEN_DIS_DOWN:
    case BoxStyleEnum::BOXSTYLE_GREEN_DIS_RAISED:
    case BoxStyleEnum::BOXSTYLE_COUNT:
    default:
      view.DrawLine(x, y + h, x + w, y + h, style.Shadow);
      view.DrawLine(x + w, y, x + w, y + h, style.Shadow);

      view.DrawLine(x, y, x + w, y, style.Highlight);
      view.DrawLine(x, y, x, y + h, style.Highlight);

      view.PutPixel(x, y + h, style.Corner);
      view.PutPixel(x + w, y, style.Corner);
      break;
  }
}

/***********************************************************************************************
 * Format_Window_String -- Separates a String into Lines. * This function will
 *take a long string and break it up into lines                          * which
 *are not longer then the window width. Any character < ' ' is * considered a
 *new line marker and will be replaced by a NULL.                              *
 *                                                                                             *
 * INPUT:      char *String - string to be formated. * int maxlinelen - Max
 *length of any line in pixels.                              *
 *                                                                                             *
 * OUTPUT:     int - number of lines string is. *
 *                                                                                             *
 * WARNINGS:    The string passed in will be modified - NULLs will be put * into
 *each position that will be a new line.                                  *
 *                                                                                             *
 * HISTORY: * 03/27/1992  SB : Created. * 05/18/1995 JLB : Greatly revised for
 *new font system.                                     *
 *=============================================================================================*/
int Format_Window_String(const FontStyle& font, std::span<char> string,
                         int max_line_len, int& width, int& height) {
  width = 0;
  height = 0;

  if (string.empty()) {
    return 0;
  }

  int lines = 0;
  size_t cursor = 0;
  while (cursor < string.size() && base::At(string, cursor) != '\0') {
    const auto line_start = cursor;
    height += FontLineHeight(font);
    ++lines;
    int line_len = 0;
    while (cursor < string.size() && line_len < max_line_len &&
           base::At(string, cursor) != '\r' &&
           base::At(string, cursor) != '\0') {
      line_len += CharPixelWidth(font, base::At(string, cursor++));
    }
    if (line_len >= max_line_len) {
      const auto overflow = cursor;
      while (cursor > line_start &&
             (cursor == string.size() || base::At(string, cursor) != ' ')) {
        line_len -= CharPixelWidth(font, base::At(string, --cursor));
      }
      if (cursor == line_start) {
        cursor = overflow > line_start ? overflow - 1 : line_start;
        line_len = 0;
        for (auto c = line_start; c < cursor; ++c) {
          line_len += CharPixelWidth(font, base::At(string, c));
        }
      }
    }
    width = std::max(line_len, width);
    if (cursor < string.size() && base::At(string, cursor) != '\0') {
      base::At(string, cursor++) = '\r';
    }
  }

  return lines;
}

/***********************************************************************************************
 * Window_Box -- Draws a fancy box over the specified window. *
 *                                                                                             *
 *    This routine will draw a fancy (shaded) box over the specified * window.
 *This is the effect used to give the polished look to * screen rectangles
 *without having to use art.                                             *
 *                                                                                             *
 * INPUT:   window   -- Specified window to fill and border. *
 *                                                                                             *
 *          style    -- The style to render the window. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   The rendering is done to `view`. *
 *                                                                                             *
 * HISTORY: * 03/03/1992 JLB : Created. * 07/31/1992 JLB : Cool raised border
 *effect.                                               * 06/08/1994 JLB : Takes
 *appropriate enumeration parameters.                                *
 *=============================================================================================*/
void Window_Box(PixelView& view, WindowNumberType window, BoxStyleEnum style) {
  static const base::EnumArray<BoxStyleEnum, int[2], kBoxstyleCount> _border = {
      {
          {0, 0},   // 0 Simple beveled edge.
          {2, 4},   // 1 Wide raised border.
          {1, 1},   // 2 Thick beveled edge.
          {2, 1},   // 3 Thin raised border.
          {0, 0},   // 4 Simple beveled edge.
          {20, 0},  // 5 Simple beveled edge.
          {0, 0},   // 6 Simple beveled edge.
          {2, 4},   // 7 Wide raised border.
          {0, 0},   // 8 Simple beveled edge.
          {20, 0},  // 9 Simple beveled edge.
          {0, 1}    // 10 Simple 1 pixel box.
      }};

  const int x =
      base::At(base::At(WindowList, static_cast<int>(window)), kWindowX) * 8;
  const int y =
      base::At(base::At(WindowList, static_cast<int>(window)), kWindowY);
  const int w =
      base::At(base::At(WindowList, static_cast<int>(window)), kWindowWidth) *
      8;
  const int h = base::At(base::At(WindowList, static_cast<int>(window)),
                         kWindowHeight);  // Window dimensions.

  Draw_Box(view, x, y, w, h, style, true);
  const int border = base::At(_border.at(style), 1);  // Width of border.

  /*
  **	Draw the second border if requested.
  */
  if (border) {
    Draw_Box(view, x + border, y + border, w - (border * 2), h - (border * 2),
             style, false);
  }
}

namespace {

// The shadows the 3-point font has no room for.
constexpr TextPrintType kShadowsTooBigFor3Point =
    TPF_DROPSHADOW | TPF_FULLSHADOW | TPF_NOSHADOW;

// Returns flags without the shadows `font` cannot draw.
TextPrintType SupportedTextFlags(const FontType font,
                                 const TextPrintType flags) {
  return font == FontType::k3Point ? flags & ~kShadowsTooBigFor3Point : flags;
}

}  // namespace

FontStyle TextFontStyle(const FontType font, TextPrintType flags) {
  flags = SupportedTextFlags(font, flags);
  int xspace = 1;
  int yspace = 0;

  switch (font) {
    case FontType::kGreen12:
    case FontType::kGreen12Gradient:
    case FontType::kVcr:
    case FontType::kScore:
    default:
      // The font's own spacing.
      break;

    case FontType::kMap:
    case FontType::k6PointGradient:
    case FontType::k6Point:
      xspace -= 1;
      break;

    case FontType::k3Point:
      xspace += 1;
      break;

    case FontType::k8Point:
      xspace -= 2;
      yspace -= 4;
      break;

    case FontType::kLed:
      xspace -= 4;
      break;
  }

  // Any one shadow flag takes a pixel off the letter spacing, and
  // TPF_NOSHADOW two off the line spacing too.
  const TextPrintType shadow = flags & (TPF_NOSHADOW | TPF_DROPSHADOW |
                                        TPF_FULLSHADOW | TPF_LIGHTSHADOW);
  if (shadow == TPF_NOSHADOW) {
    xspace -= 1;
    yspace -= 2;
  } else if (shadow == TPF_DROPSHADOW || shadow == TPF_LIGHTSHADOW ||
             shadow == TPF_FULLSHADOW) {
    xspace -= 1;
  }

  return {.font = FontView(TheAssets().font(font)),
          .x_spacing = xspace,
          .y_spacing = yspace};
}

TextStyle TextStyleFor(const FontType font, TextPrintType flags,
                       engine::gfx::PaletteIndex fore,
                       const engine::gfx::PaletteIndex back) {
  flags = SupportedTextFlags(font, flags);
  ////////////////#if (0)
  static unsigned char _textfontpal[16][16] = {
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 26, 0, 0, 0, 0, 0, 0, 0, 0, 0, 27, 27, 26, 25, 24},
      {0, 135, 0, 0, 0, 0, 0, 0, 0, 0, 0, 136, 136, 135, 119, 2},
      {0, 159, 0, 0, 0, 0, 0, 0, 0, 0, 0, 142, 143, 159, 41, 167},

      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 157, 0, 0, 0, 0, 0, 0, 0, 0, 0, 180, 180, 157, 158, 5},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 179, 0, 0, 0, 0, 0, 0, 0, 0, 0, 180, 180, 179, 178, 176},

      {0, 123, 0, 0, 0, 0, 0, 0, 0, 0, 0, 122, 122, 123, 125, 127},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 203, 0, 0, 0, 0, 0, 0, 0, 0, 0, 204, 204, 203, 202, 201},
      {0, 1, 4, 166, 41, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

      {0, 203, 0, 0, 0, 0, 0, 0, 0, 0, 0, 204, 204, 203, 202, 201},
      {0, 203, 0, 0, 0, 0, 0, 0, 0, 0, 0, 204, 204, 203, 202, 201},

      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  };
  static const unsigned char _textpalmedium[16] = {
      0, 25, 119, 41, 0, 158, 0, 178, 125, 0, 202, 0, 0, 0, 0, 0};

  static const unsigned char _textpalbright[16] = {
      0, 24, 2, 4, 0, 5, 0, 176, 127, 0, 201, 0, 0, 0, 0, 0};
  ///////////////////////#endif	//(0)

  unsigned char fontpalette[16];  // Working font palette array.
  base::FillBytes(std::as_writable_bytes(base::Suffix(fontpalette, 0)),
                  back.byte(), 16);

  if (font == FontType::kVcr) {
    base::At(fontpalette, 3) = 12;
    base::At(fontpalette, 9) = 15;
    base::At(fontpalette, 10) = 200;
    base::At(fontpalette, 11) = 201;
    base::At(fontpalette, 12) = 202;
    base::At(fontpalette, 13) = 203;
    base::At(fontpalette, 14) = 204;
    base::At(fontpalette, 15) = 205;
  }

  /*
  **	A gradient font always requires special fixups for the palette.
  */
  if (font == FontType::k6PointGradient || font == FontType::kGreen12Gradient ||
      font == FontType::kGreen12) {
    /*
    **	If a gradient palette was requested, then fill in the font palette array
    **	according to the color index specified.
    */
    if (base::Any(flags & TPF_USE_GRAD_PAL)) {
      base::CopyBytes(
          std::as_writable_bytes(base::Suffix(fontpalette, 0)),
          base::ObjectBytes(base::At(_textfontpal, fore.value() % 16)), 16);
    } else {
      /*
      **	Special adjustment for fonts that have gradient artwork. When
      *there is *	no special gradient effect desired, then set the font
      *color based on the *	forground color specified.
      */
      base::FillBytes(std::as_writable_bytes(base::Suffix(fontpalette, 4)),
                      fore.byte(), 12);
    }

    if (base::Any(flags & TPF_MEDIUM_COLOR)) {
      fore = engine::gfx::PaletteIndex{
          base::At(_textpalmedium, fore.value() % 16)};
      base::FillBytes(std::as_writable_bytes(base::Suffix(fontpalette, 4)),
                      fore.byte(), 12);
    } else {
      if (base::Any(flags & TPF_BRIGHT_COLOR)) {
        fore = engine::gfx::PaletteIndex{
            base::At(_textpalbright, fore.value() % 16)};
        base::FillBytes(std::as_writable_bytes(base::Suffix(fontpalette, 4)),
                        fore.byte(), 12);
      } else {
        fore = engine::gfx::PaletteIndex{base::At(fontpalette, 1)};
      }
    }
  }

  // Set the glyph palette's shadow colours from the shadow flags.
  const TextPrintType shadow =
      flags & (TPF_NOSHADOW | TPF_DROPSHADOW | TPF_FULLSHADOW |
               TPF_LIGHTSHADOW);  // Requested shadow value.
  switch (shadow) {
    /*
    **	The text is rendered plain.
    */
    case TPF_NOSHADOW:
      base::At(fontpalette, 2) = back.byte();
      base::At(fontpalette, 3) = back.byte();
      break;

    /*
    **	The text is rendered with a simple
    **	drop shadow.
    */
    case TPF_DROPSHADOW:
      base::At(fontpalette, 2) = game::kBlack.byte();
      base::At(fontpalette, 3) = back.byte();
      break;

    /*
    **	Special engraved text look for the options
    **	dialog system.
    */
    case TPF_LIGHTSHADOW:
      base::At(fontpalette, 2) = (14 * 16) + 7 + 1;
      base::At(fontpalette, 3) = back.byte();
      break;

    /*
    **	Each letter is surrounded by black. This is used
    **	when the text will be over a non-plain background.
    */
    case TPF_FULLSHADOW:
      base::At(fontpalette, 2) = game::kBlack.byte();
      base::At(fontpalette, 3) = game::kBlack.byte();
      break;

    case TextPrintType::TPF_NONE:
    case TextPrintType::TPF_CENTER:
    case TextPrintType::TPF_RIGHT:
    case TextPrintType::TPF_MEDIUM_COLOR:
    case TextPrintType::TPF_BRIGHT_COLOR:
    case TextPrintType::TPF_USE_GRAD_PAL:
    default:
      break;
  }
  base::At(fontpalette, 0) = back.byte();
  base::At(fontpalette, 1) = fore.byte();

  FontStyle font_style = TextFontStyle(font, flags);
  std::ranges::copy(fontpalette, font_style.palette.begin());
  return {.flag = flags, .forecolor = fore, .font_style = font_style};
}

TextPrinter::TextPrinter(PixelView& view, const PrintStyle& style)
    : view_(&view), style_(style) {}

TextPrinter TextPrinter::With(const TextPrintType flags) const {
  TextPrinter printer = *this;
  printer.style_.flags = style_.flags | flags;
  return printer;
}

void TextPrinter::Print(const std::string& text, const base::Point at) const {
  Print(text.c_str(), at);
}

void TextPrinter::Print(const int text, const base::Point at) const {
  if (text != TXT_NONE) {
    Print(Text_String(text), at);
  }
}

void TextPrinter::Print(const char* text, const base::Point at) const {
  if (text == nullptr || *text == '\0') {
    return;
  }
  // Bytes of 0xff are not for display.
  std::string filtered;
  for (const char character : std::string_view(text)) {
    if (static_cast<unsigned char>(character) != 0xff) {
      filtered.push_back(character);
    }
  }
  const TextStyle style =
      TextStyleFor(style_.font, style_.flags, style_.fore, style_.back);
  int x = at.x;
  switch (style.flag & (TPF_CENTER | TPF_RIGHT)) {
    case TPF_CENTER:
      x -= StringPixelWidth(style.font_style, filtered.c_str()) / 2;
      break;

    case TPF_RIGHT:
      x -= StringPixelWidth(style.font_style, filtered.c_str());
      break;

    case TextPrintType::TPF_NONE:
    case TextPrintType::TPF_NOSHADOW:
    case TextPrintType::TPF_DROPSHADOW:
    case TextPrintType::TPF_FULLSHADOW:
    case TextPrintType::TPF_LIGHTSHADOW:
    case TextPrintType::TPF_MEDIUM_COLOR:
    case TextPrintType::TPF_BRIGHT_COLOR:
    case TextPrintType::TPF_USE_GRAD_PAL:
    default:
      break;
  }

  if (x < TheScreen().visible_view().width() &&
      at.y < TheScreen().visible_view().height()) {
    view_->Print(style.font_style, filtered.c_str(), x, at.y, style_.fore,
                 style_.back);
  }
}

void TextPrinter::PrintClipped(const char* text, const base::Point at,
                               const int width,
                               std::span<const int> tabs) const {
  if (text == nullptr) {
    return;
  }
  char buffer[512];
  base::SafeCopy(buffer, text);
  const FontStyle font = TextFontStyle(style_.font, style_.flags);
  std::span<char> source(buffer);
  int offset = 0;
  while (offset < width && !source.empty() && source.front() != '\0') {
    const auto text_view = std::string_view(source.data());
    const auto tab = text_view.find('\t');
    const auto count = tab == std::string_view::npos ? text_view.size() : tab;
    int line_width = 0;
    size_t visible = 0;
    while (visible < count) {
      const int next = CharPixelWidth(font, base::At(source, visible));
      if (offset + line_width + next >= width) {
        break;
      }
      line_width += next;
      ++visible;
    }
    base::At(source, visible) = '\0';
    Print(source.data(), {at.x + offset, at.y});
    offset += line_width;
    if (visible < count || tab == std::string_view::npos) {
      break;
    }
    if (!tabs.empty()) {
      while (tabs.size() > 1 && offset > tabs.front()) {
        tabs = tabs.subspan(1);
      }
      offset = tabs.front();
    } else {
      offset = (offset + (1 / 50) + 1) * 50;
    }
    source = source.subspan(tab + 1);
  }
}

}  // namespace td
