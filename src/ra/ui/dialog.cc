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

/* $Header: /CounterStrike/DIALOG.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
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
 *                  Last Update : July 31, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Clip_Text_Print -- Prints text with clipping and <TAB> support.
 ** Dialog_Box -- draws a dialog background box * Display_Place_Building --
 *Displays the "place building" dialog box.                       *
 *   Display_Select_Target -- Displays the "choose target" prompt. *
 *   Display_Status -- Display the player scenario status box. * Draw_Box --
 *Displays a highlighted box. * Draw_Caption -- Draws a caption on a dialog box.
 ** Fancy_Text_Print -- Prints text with a drop shadow. * Plain_Text_Print --
 *Prints text without using a color scheme                              *
 *   Redraw_Needed -- Determine if sidebar needs to be redrawn. *
 *   Render_Bar_Graph -- Renders a specified bargraph. * Simple_Text_Print --
 *Prints text with a drop shadow.                                      *
 *   Window_Box -- Draws a fancy box over the specified window. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/ui/dialog.h"

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
#include "base/flags.h"
#include "base/numeric.h"
#include "base/point.h"
#include "base/rect.h"
#include "base/strings/safe_string.h"
#include "engine/file/mix_archive.h"
#include "engine/gfx/font.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/shape.h"
#include "engine/gfx/text_window.h"
#include "game/ui/standard_colors.h"
#include "ra/assets/assets.h"
#include "ra/assets/palettes.h"
#include "ra/assets/screen.h"
#include "ra/assets/shape_draw.h"
#include "ra/core/config.h"
#include "ra/core/defines.h"
#include "ra/data/text.h"
#include "ra/data/text_ids.h"
#include "ra/debug/debug_state.h"
#include "ra/ui/color_scheme.h"

namespace ra {

using engine::gfx::ShapeFlags;

/***********************************************************************************************
 * Dialog_Box -- draws a dialog background box *
 *                                                                                             *
 * INPUT: * x,y,w,h      the usual *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 01/26/1995 BR : Created. * 07/31/1996 JLB : Uses shapes to draw
 *the box.                                             *
 *=============================================================================================*/
void Dialog_Box(int x, int y, int w, int h) {
  base::At(WindowList[static_cast<int>(WINDOW_PARTIAL)], kWindowX) = x;
  base::At(WindowList[static_cast<int>(WINDOW_PARTIAL)], kWindowY) = y;
  base::At(WindowList[static_cast<int>(WINDOW_PARTIAL)], kWindowWidth) = w;
  base::At(WindowList[static_cast<int>(WINDOW_PARTIAL)], kWindowHeight) = h;

  /*
  **	Always draw to the hidpage and then blit forward.
  */
  PixelView& view = TheScreen().hidden_view();

  /*
  **	Draw the background block.
  */
  const int cx = w / 2;
  const int cy = h / 2;
  auto shapedata = MixArchive::RetrieveData("DD-BKGND.SHP");
  CC_Draw_Shape(view, shapedata, 0, cx - 312, cy - 192, WINDOW_PARTIAL,
                ShapeFlags::kNone);
  CC_Draw_Shape(view, shapedata, 1, cx, cy - 192, WINDOW_PARTIAL,
                ShapeFlags::kNone);
  CC_Draw_Shape(view, shapedata, 2, cx - 312, cy, WINDOW_PARTIAL,
                ShapeFlags::kNone);
  CC_Draw_Shape(view, shapedata, 3, cx, cy, WINDOW_PARTIAL, ShapeFlags::kNone);
  /*
  **	Draw the side strips.
  */
  shapedata = MixArchive::RetrieveData("DD-EDGE.SHP");
  for (int yy = 0; yy < h; yy += 6) {
    CC_Draw_Shape(view, shapedata, 0, 14, yy, WINDOW_PARTIAL,
                  ShapeFlags::kNone);
    CC_Draw_Shape(view, shapedata, 1, w - ((7 + 8) * 2), yy, WINDOW_PARTIAL,
                  ShapeFlags::kNone);
  }

  /*
  **	Draw the border bars.
  */
  shapedata = MixArchive::RetrieveData("DD-LEFT.SHP");
  CC_Draw_Shape(view, shapedata, 0, 0, cy - 200, WINDOW_PARTIAL,
                ShapeFlags::kNone);
  CC_Draw_Shape(view, shapedata, 0, 0, cy, WINDOW_PARTIAL, ShapeFlags::kNone);

  shapedata = MixArchive::RetrieveData("DD-RIGHT.SHP");
  const int rightx = w - 14;
  CC_Draw_Shape(view, shapedata, 0, rightx, cy - 200, WINDOW_PARTIAL,
                ShapeFlags::kNone);
  CC_Draw_Shape(view, shapedata, 0, rightx, cy, WINDOW_PARTIAL,
                ShapeFlags::kNone);

  shapedata = MixArchive::RetrieveData("DD-BOTM.SHP");
  CC_Draw_Shape(view, shapedata, 0, cx - 320, h - 16, WINDOW_PARTIAL,
                ShapeFlags::kNone);
  CC_Draw_Shape(view, shapedata, 0, cx, h - 16, WINDOW_PARTIAL,
                ShapeFlags::kNone);

  shapedata = MixArchive::RetrieveData("DD-TOP.SHP");
  CC_Draw_Shape(view, shapedata, 0, cx - 320, 0, WINDOW_PARTIAL,
                ShapeFlags::kNone);
  CC_Draw_Shape(view, shapedata, 0, cx, 0, WINDOW_PARTIAL, ShapeFlags::kNone);

  /*
  **	Draw the corner caps.
  */
  shapedata = MixArchive::RetrieveData("DD-CRNR.SHP");
  CC_Draw_Shape(view, shapedata, 0, 0, 0, WINDOW_PARTIAL, ShapeFlags::kNone);
  CC_Draw_Shape(view, shapedata, 1, w - 23, 0, WINDOW_PARTIAL,
                ShapeFlags::kNone);
  CC_Draw_Shape(view, shapedata, 2, 0, h - 24, WINDOW_PARTIAL,
                ShapeFlags::kNone);
  CC_Draw_Shape(view, shapedata, 3, w - 23, h - 24, WINDOW_PARTIAL,
                ShapeFlags::kNone);

  TheScreen().hidden_view().BlitTo(TheScreen().visible_view(),
                                   base::Rect{x, y, w, h}, x, y);
}

// Draws the beveled edges shared by the button box styles: "Shadow" along the
// bottom and right, "Highlight" along the top and left, and "Corner" on the two
// pixels where they meet. Swapping shadow and highlight is what turns a raised
// button into a depressed one. All coordinates are inclusive.
static void Draw_Beveled_Box(PixelView& view, const int left, const int top,
                             const int right, const int bottom,
                             const bool filled, const BoxStyleType& colors) {
  if (filled) {
    view.FillRect(base::Rect::FromCorners(left, top, right, bottom),
                  colors.Filler);
  }

  view.DrawLine(left, bottom, right, bottom, colors.Shadow);
  view.DrawLine(right, top, right, bottom, colors.Shadow);

  view.DrawLine(left, top, right, top, colors.Highlight);
  view.DrawLine(left, top, left, bottom, colors.Highlight);

  view.PutPixel(left, bottom, colors.Corner);
  view.PutPixel(right, top, colors.Corner);
}

// Draw_Box -- Displays a highlighted box.
//
// HISTORY: 05/28/1991 JLB : Created.
//          05/30/1992 JLB : Embedded color codes.
//          07/31/1992 JLB : Depressed option added.
void Draw_Box(PixelView& view, const int x, const int y, const int w,
              const int h, const BoxStyleEnum up, const bool filled) {
  const RemapControlType* scheme = Get_Color_Scheme();

  // The draw calls below take inclusive corner coordinates, so a box "w" pixels
  // wide ends at x + w - 1.
  const int right = x + w - 1;
  const int bottom = y + h - 1;

  switch (up) {
    // Flat outline drawn on the box edge itself.
    case BOXSTYLE_BOX:
      if (filled) {
        view.FillRect(base::Rect{x, y, w, h}, game::kBlack);
      }
      view.DrawRect(base::Rect{x, y, w, h}, scheme->Box);
      break;

    // Same outline, inset one pixel, which leaves a filled margin around the
    // frame of a dialog.
    case BOXSTYLE_BORDER:
      if (filled) {
        view.FillRect(base::Rect{x, y, w, h}, game::kBlack);
      }
      view.DrawRect(base::Rect{x + 1, y + 1, w - 2, h - 2}, scheme->Box);
      break;

    case BOXSTYLE_DOWN:
      Draw_Beveled_Box(view, x, y, right, bottom, filled,
                       {
                           .Filler = scheme->Background,
                           .Shadow = scheme->Highlight,
                           .Highlight = scheme->Shadow,
                           .Corner = scheme->Corners,
                       });
      break;

    // The disabled styles use fixed greys rather than the color scheme.
    case BOXSTYLE_DIS_DOWN:
      Draw_Beveled_Box(view, x, y, right, bottom, filled,
                       {
                           .Filler = game::kGrey,
                           .Shadow = game::kWhite,
                           .Highlight = game::kBlack,
                           .Corner = game::kGrey,
                       });
      break;

    case BOXSTYLE_DIS_RAISED:
      Draw_Beveled_Box(view, x, y, right, bottom, filled,
                       {
                           .Filler = game::kGrey,
                           .Shadow = game::kBlack,
                           .Highlight = game::kLightGrey,
                           .Corner = game::kGrey,
                       });
      break;

    // A raised button is also the fallback for an unrecognized style.
    case BOXSTYLE_RAISED:
    default:
      Draw_Beveled_Box(view, x, y, right, bottom, filled,
                       {
                           .Filler = scheme->Background,
                           .Shadow = scheme->Shadow,
                           .Highlight = scheme->Highlight,
                           .Corner = scheme->Corners,
                       });
      break;
  }
}

void DrawColorBox(PixelView& view, const base::Rect& box,
                  const engine::gfx::PaletteIndex color, const bool selected) {
  view.FillRect(
      base::Rect{box.x() + 1, box.y() + 1, box.width() - 2, box.height() - 2},
      color);
  Draw_Box(view, box.x(), box.y(), box.width(), box.height(),
           selected ? BOXSTYLE_DOWN : BOXSTYLE_RAISED, false);
}

// Returns true for the characters that terminate a line: the carriage return
// this routine writes as its hard break, the author supplied break marker
// ('@') and the string terminator.
static constexpr bool Is_Line_Break(const char c) {
  return c == '\r' || c == '@' || c == '\0';
}

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
           !Is_Line_Break(base::At(string, cursor))) {
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
  const int x =
      base::At(base::At(WindowList, static_cast<int>(window)), kWindowX);
  const int y =
      base::At(base::At(WindowList, static_cast<int>(window)), kWindowY);
  const int w =
      base::At(base::At(WindowList, static_cast<int>(window)), kWindowWidth);
  const int h =
      base::At(base::At(WindowList, static_cast<int>(window)), kWindowHeight);

  Draw_Box(view, x, y, w, h, style, true);
}

// The C&C gradient font colors:
//   0      transparent (background)
//   1      foreground color for mono-color fonts only
//   2      shadow under characters ("drop shadow")
//   3      shadow all around characters ("full shadow")
//   4-10   unused
//   11-15  the rows of a glyph, top to bottom
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
    case FontType::kScore:
    case FontType::kVcr:
    case FontType::kMetal12:
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

    case FontType::kEditor:
      yspace += 1;
      xspace -= 1;
      xspace -= 1;
      break;

    case FontType::k8Point:
      // Also what a print without a font got: the original kept whichever
      // font printed last, and 8-point is the font Assets starts with.
      xspace -= 2;
      yspace -= 4;
      break;

    case FontType::kLed:
      xspace -= 4;
      break;

    case FontType::kType:
      xspace -= 1;

      if constexpr (config::kWolapiEnabled) {
        xspace -= 2;
        yspace += 2;
      } else if constexpr (config::kIsGerman) {
        //	ajw: "I am implicitly assuming that TPF_TYPE was no longer being
        //	used, before I came along, despite the following."
        yspace += 4;  // VG 10/17/96
      }

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
                       const RemapControlType* fore,
                       const engine::gfx::PaletteIndex back) {
  flags = SupportedTextFlags(font, flags);
  unsigned char fontpalette[16];  // Working font palette array.

  if (fore == nullptr) {
    fore = &ThePalettes().color_remaps().at(PCOLOR_RED);
  }

  /*
  ** Init the font palette to the given background color
  */
  base::FillBytes(std::as_writable_bytes(base::Suffix(fontpalette, 0)),
                  back.byte(), 16);

  engine::gfx::PaletteIndex forecolor = fore->Color;

  /*
  **	A gradient font always requires special fixups for the palette
  */
  if (font == FontType::kVcr || font == FontType::k6PointGradient ||
      font == FontType::kMetal12 || font == FontType::kEditor ||
      font == FontType::kType) {
    /*
    ** If a gradient palette is specified, copy the remap table directly,
    *otherwise *	use the foreground color as the entire font remap color.
    */
    if (base::Any(flags & TPF_USE_GRAD_PAL)) {
      base::CopyBytes(base::ObjectBytes(fontpalette),
                      base::ObjectBytes(fore->FontRemap), 16);
      forecolor = fore->Color;
      if (font == FontType::kType) {
        forecolor = engine::gfx::PaletteIndex{base::At(fontpalette, 1)};
      }
    } else {
      base::FillBytes(std::as_writable_bytes(base::Suffix(fontpalette, 4)),
                      fore->Color.byte(), 12);
      forecolor = fore->Color;
    }

    /*
    ** Medium color: set all font colors to a medium value.  This flag
    ** overrides any gradient effects.
    */
    if (base::Any(flags & TPF_MEDIUM_COLOR)) {
      forecolor = fore->Color;
      base::FillBytes(std::as_writable_bytes(base::Suffix(fontpalette, 4)),
                      fore->Color.byte(), 12);
    }

    /*
    ** Bright color: set all font colors to a bright value.  This flag
    ** overrides any gradient effects.
    */
    if (base::Any(flags & TPF_BRIGHT_COLOR)) {
      forecolor = fore->Bright;
      base::FillBytes(std::as_writable_bytes(base::Suffix(fontpalette, 4)),
                      fore->BrightColor.byte(), 12);
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
  if (font != FontType::kType) {
    base::At(fontpalette, 0) = back.byte();
    base::At(fontpalette, 1) = fore->Color.byte();
  }

  FontStyle font_style = TextFontStyle(font, flags);
  std::ranges::copy(fontpalette, font_style.palette.begin());
  return {.flag = flags, .forecolor = forecolor, .font_style = font_style};
}

namespace {

// The color scheme TextPrinter::Plain() paints with: every slot the same
// palette index, so the text comes out in one flat color.
RemapControlType PlainScheme(const engine::gfx::PaletteIndex fore) {
  RemapControlType scheme{};

  base::FillBytes(std::as_writable_bytes(base::Suffix(scheme.FontRemap, 4)),
                  fore.byte(), 12);

  scheme.BrightColor = fore;
  scheme.Color = fore;
  scheme.Shadow = fore;
  scheme.Background = fore;
  scheme.Corners = fore;
  scheme.Highlight = fore;
  scheme.Box = fore;
  scheme.Bright = fore;
  scheme.Underline = fore;
  scheme.Bar = fore;
  return scheme;
}

}  // namespace

TextPrinter::TextPrinter(PixelView& view, const PrintStyle& style)
    : view_(&view), style_(style) {}

TextPrinter TextPrinter::Plain(PixelView& view,
                               const engine::gfx::PaletteIndex fore,
                               const engine::gfx::PaletteIndex back,
                               const FontType font, const TextPrintType flags) {
  TextPrinter printer(view, {.back = back, .font = font, .flags = flags});
  printer.plain_scheme_ = PlainScheme(fore);
  return printer;
}

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
    Print(TextString(text), at);
  }
}

void TextPrinter::Print(const char* text, const base::Point at) const {
  if (text == nullptr || *text == '\0') {
    return;
  }
  const TextStyle style =
      TextStyleFor(style_.font, style_.flags, scheme(), style_.back);
  int x = at.x();
  switch (style.flag & (TPF_CENTER | TPF_RIGHT)) {
    case TPF_CENTER:
      x -= StringPixelWidth(style.font_style, text) / 2;
      break;

    case TPF_RIGHT:
      x -= StringPixelWidth(style.font_style, text);
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

  if (x < view_->width() && at.y() < view_->height()) {
    view_->Print(style.font_style, text, x, at.y(), style.forecolor,
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
    Print(source.data(), {at.x() + offset, at.y()});
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

// The scheme lives in the printer itself when it is a plain one, so that a
// copy of the printer points at its own.
const RemapControlType* TextPrinter::scheme() const {
  return plain_scheme_ ? &*plain_scheme_ : style_.scheme;
}

/***********************************************************************************************
 * Draw_Caption -- Draws a caption on a dialog box. *
 *                                                                                             *
 *    This routine draws the caption text and any fancy filigree that the dialog
 *may require.  *
 *                                                                                             *
 * INPUT:   text  -- The text of the caption. This is the text number. *
 *                                                                                             *
 *          x,y   -- The dialog box X and Y pixel coordinate of the upper left
 *corner.         *
 *                                                                                             *
 *          w     -- The width of the dialog box (in pixels). *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/23/1995 JLB : Created. *
 *=============================================================================================*/
void Draw_Caption(PixelView& view, int text, int x, int y, int w) {
  Draw_Caption(view, TextString(text), x, y, w);
}

void Draw_Caption(PixelView& view, const char* text, int x, int y, int w) {
  /*
  **	Draw the caption.
  */
  if (text != nullptr && *text != '\0') {
    if (TheDebugState().map_editor_active()) {
      TextPrinter(
          view, {Get_Color_Scheme(), game::kTransparentBlack, FontType::kEditor,
                 TPF_CENTER | TPF_USE_GRAD_PAL | TPF_NOSHADOW})
          .Print(text, {(w / 2) + x, 4 + y});
    } else {
      TextPrinter(view, {Get_Color_Scheme(), game::kTransparentBlack, kTextFont,
                         TPF_CENTER | kTextFlags})
          .Print(text, {(w / 2) + x, 16 + y});
      const FontStyle font = TextFontStyle(kTextFont, TPF_CENTER | kTextFlags);
      const int length = StringPixelWidth(font, text);
      view.DrawLine(x + (w / 2) - (length / 2), y + FontLineHeight(font) + 16,
                    x + (w / 2) + (length / 2), y + FontLineHeight(font) + 16,
                    Get_Color_Scheme()->Box);
    }
  }
}

}  // namespace ra
