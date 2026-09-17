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

#include "ra/dialog.h"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <span>
#include <string>

#include "absl/strings/str_format.h"
#include "absl/types/span.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "port/format.h"
#include "port/safe_string.h"
#include "ra/config.h"
#include "ra/conquer.h"
#include "ra/defines.h"
#include "ra/externs.h"
#include "ra/gadget.h"
#include "ra/globals.h"
#include "ra/inline.h"
#include "ra/jshell.h"
#include "sdllib/drawbuff.h"
#include "sdllib/font.h"
#include "sdllib/gbuffer.h"
#include "sdllib/shape.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"
#include "sdllib/wwstd.h"
#include "tech/mix_archive.h"

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
  GraphicViewPortClass* oldpage = Set_Logic_Page(HidPage);

  /*
  **	Draw the background block.
  */
  const int cx = w / 2;
  const int cy = h / 2;
  auto shapedata = MixArchive::RetrieveData("DD-BKGND.SHP");
  CC_Draw_Shape(shapedata, 0, cx - 312, cy - 192, WINDOW_PARTIAL,
                SHAPE_WIN_REL);
  CC_Draw_Shape(shapedata, 1, cx, cy - 192, WINDOW_PARTIAL, SHAPE_WIN_REL);
  CC_Draw_Shape(shapedata, 2, cx - 312, cy, WINDOW_PARTIAL, SHAPE_WIN_REL);
  CC_Draw_Shape(shapedata, 3, cx, cy, WINDOW_PARTIAL, SHAPE_WIN_REL);
  /*
  **	Draw the side strips.
  */
  shapedata = MixArchive::RetrieveData("DD-EDGE.SHP");
  for (int yy = 0; yy < h; yy += 6) {
    CC_Draw_Shape(shapedata, 0, 14, yy, WINDOW_PARTIAL, SHAPE_WIN_REL);
    CC_Draw_Shape(shapedata, 1, w - ((7 + 8) * 2), yy, WINDOW_PARTIAL,
                  SHAPE_WIN_REL);
  }

  /*
  **	Draw the border bars.
  */
  shapedata = MixArchive::RetrieveData("DD-LEFT.SHP");
  CC_Draw_Shape(shapedata, 0, 0, cy - 200, WINDOW_PARTIAL, SHAPE_WIN_REL);
  CC_Draw_Shape(shapedata, 0, 0, cy, WINDOW_PARTIAL, SHAPE_WIN_REL);

  shapedata = MixArchive::RetrieveData("DD-RIGHT.SHP");
  const int rightx = w - 14;
  CC_Draw_Shape(shapedata, 0, rightx, cy - 200, WINDOW_PARTIAL, SHAPE_WIN_REL);
  CC_Draw_Shape(shapedata, 0, rightx, cy, WINDOW_PARTIAL, SHAPE_WIN_REL);

  shapedata = MixArchive::RetrieveData("DD-BOTM.SHP");
  CC_Draw_Shape(shapedata, 0, cx - 320, h - 16, WINDOW_PARTIAL, SHAPE_WIN_REL);
  CC_Draw_Shape(shapedata, 0, cx, h - 16, WINDOW_PARTIAL, SHAPE_WIN_REL);

  shapedata = MixArchive::RetrieveData("DD-TOP.SHP");
  CC_Draw_Shape(shapedata, 0, cx - 320, 0, WINDOW_PARTIAL, SHAPE_WIN_REL);
  CC_Draw_Shape(shapedata, 0, cx, 0, WINDOW_PARTIAL, SHAPE_WIN_REL);

  /*
  **	Draw the corner caps.
  */
  shapedata = MixArchive::RetrieveData("DD-CRNR.SHP");
  CC_Draw_Shape(shapedata, 0, 0, 0, WINDOW_PARTIAL, SHAPE_WIN_REL);
  CC_Draw_Shape(shapedata, 1, w - 23, 0, WINDOW_PARTIAL, SHAPE_WIN_REL);
  CC_Draw_Shape(shapedata, 2, 0, h - 24, WINDOW_PARTIAL, SHAPE_WIN_REL);
  CC_Draw_Shape(shapedata, 3, w - 23, h - 24, WINDOW_PARTIAL, SHAPE_WIN_REL);

  WWMouse->Draw_Mouse(&HidPage);
  HidPage.Blit(SeenBuff, x, y, x, y, w, h, false);
  WWMouse->Erase_Mouse(&HidPage, false);
  Set_Logic_Page(oldpage);
}

// Draws the beveled edges shared by the button box styles: "Shadow" along the
// bottom and right, "Highlight" along the top and left, and "Corner" on the two
// pixels where they meet. Swapping shadow and highlight is what turns a raised
// button into a depressed one. All coordinates are inclusive.
static void Draw_Beveled_Box(const int left, const int top, const int right,
                             const int bottom, const bool filled,
                             const BoxStyleType& colors) {
  if (filled) {
    LogicPage->Fill_Rect(left, top, right, bottom, colors.Filler);
  }

  LogicPage->Draw_Line(left, bottom, right, bottom, colors.Shadow);
  LogicPage->Draw_Line(right, top, right, bottom, colors.Shadow);

  LogicPage->Draw_Line(left, top, right, top, colors.Highlight);
  LogicPage->Draw_Line(left, top, left, bottom, colors.Highlight);

  LogicPage->Put_Pixel(left, bottom, colors.Corner);
  LogicPage->Put_Pixel(right, top, colors.Corner);
}

// Draw_Box -- Displays a highlighted box.
//
// HISTORY: 05/28/1991 JLB : Created.
//          05/30/1992 JLB : Embedded color codes.
//          07/31/1992 JLB : Depressed option added.
void Draw_Box(const int x, const int y, const int w, const int h,
              const BoxStyleEnum up, const bool filled) {
  const RemapControlType* scheme = GadgetClass::Get_Color_Scheme();

  // The draw calls below take inclusive corner coordinates, so a box "w" pixels
  // wide ends at x + w - 1.
  const int right = x + w - 1;
  const int bottom = y + h - 1;

  switch (up) {
    // Flat outline drawn on the box edge itself.
    case BOXSTYLE_BOX:
      if (filled) {
        LogicPage->Fill_Rect(x, y, right, bottom, kBlack);
      }
      LogicPage->Draw_Rect(x, y, right, bottom, scheme->Box);
      break;

    // Same outline, inset one pixel, which leaves a filled margin around the
    // frame of a dialog.
    case BOXSTYLE_BORDER:
      if (filled) {
        LogicPage->Fill_Rect(x, y, right, bottom, kBlack);
      }
      LogicPage->Draw_Rect(x + 1, y + 1, right - 1, bottom - 1, scheme->Box);
      break;

    case BOXSTYLE_DOWN:
      Draw_Beveled_Box(x, y, right, bottom, filled,
                       {
                           .Filler = scheme->Background,
                           .Shadow = scheme->Highlight,
                           .Highlight = scheme->Shadow,
                           .Corner = scheme->Corners,
                       });
      break;

    // The disabled styles use fixed greys rather than the color scheme.
    case BOXSTYLE_DIS_DOWN:
      Draw_Beveled_Box(x, y, right, bottom, filled,
                       {
                           .Filler = kGrey,
                           .Shadow = kWhite,
                           .Highlight = kBlack,
                           .Corner = kGrey,
                       });
      break;

    case BOXSTYLE_DIS_RAISED:
      Draw_Beveled_Box(x, y, right, bottom, filled,
                       {
                           .Filler = kGrey,
                           .Shadow = kBlack,
                           .Highlight = kLtGrey,
                           .Corner = kGrey,
                       });
      break;

    // A raised button is also the fallback for an unrecognized style.
    case BOXSTYLE_RAISED:
    default:
      Draw_Beveled_Box(x, y, right, bottom, filled,
                       {
                           .Filler = scheme->Background,
                           .Shadow = scheme->Shadow,
                           .Highlight = scheme->Highlight,
                           .Corner = scheme->Corners,
                       });
      break;
  }
}

// Returns true for the characters that terminate a line: the carriage return
// this routine writes as its hard break, the author supplied break marker
// ('@') and the string terminator.
static constexpr bool Is_Line_Break(const char c) {
  return c == '\r' || c == '@' || c == '\0';
}

int Format_Window_String(std::span<char> string, int max_line_len, int& width,
                         int& height) {
  width = 0;
  height = 0;

  if (string.empty()) {
    return 0;
  }

  int lines = 0;
  size_t cursor = 0;
  while (cursor < string.size() && string[cursor] != '\0') {
    const auto line_start = cursor;
    height += FontHeight + FontYSpacing;
    ++lines;
    int line_len = 0;
    while (cursor < string.size() && line_len < max_line_len &&
           !Is_Line_Break(string[cursor])) {
      line_len += Char_Pixel_Width(string[cursor++]);
    }
    if (line_len >= max_line_len) {
      const auto overflow = cursor;
      while (cursor > line_start &&
             (cursor == string.size() || string[cursor] != ' ')) {
        line_len -= Char_Pixel_Width(string[--cursor]);
      }
      if (cursor == line_start) {
        cursor = overflow > line_start ? overflow - 1 : line_start;
        line_len = 0;
        for (auto c = line_start; c < cursor; ++c) {
          line_len += Char_Pixel_Width(string[c]);
        }
      }
    }
    width = std::max(line_len, width);
    if (cursor < string.size() && string[cursor] != '\0') {
      string[cursor++] = '\r';
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
 * WARNINGS:   The rendering is done to the LogicPage. *
 *                                                                                             *
 * HISTORY: * 03/03/1992 JLB : Created. * 07/31/1992 JLB : Cool raised border
 *effect.                                               * 06/08/1994 JLB : Takes
 *appropriate enumeration parameters.                                *
 *=============================================================================================*/
void Window_Box(WindowNumberType window, BoxStyleEnum style) {
  const int x =
      base::At(base::At(WindowList, static_cast<int>(window)), kWindowX);
  const int y =
      base::At(base::At(WindowList, static_cast<int>(window)), kWindowY);
  const int w =
      base::At(base::At(WindowList, static_cast<int>(window)), kWindowWidth);
  const int h =
      base::At(base::At(WindowList, static_cast<int>(window)), kWindowHeight);

  /*
  **	If it is to be rendered to the seenpage, then
  **	hide the mouse.
  */
  if (LogicPage == &SeenBuff) {
    Conditional_Hide_Mouse(x, y, x + w, y + h);
  }

  Draw_Box(x, y, w, h, style, true);

  /*
  **	Restore the mouse if it has been hidden and return.
  */
  if (LogicPage == &SeenBuff) {
    Conditional_Show_Mouse();
  }
}

/***********************************************************************************************
 * Simple_Text_Print -- Prints text with a drop shadow. *
 *                                                                                             *
 *    This routine functions like Text_Print, but will render a drop * shadow
 *(in black). *
 *                                                                                             *
 *    The C&C gradient font colors are as follows:
 ** 0		transparent (background)
 ** 1		foreground color for mono-color fonts only
 ** 2		shadow under characters ("drop shadow")
 ** 3		shadow all around characters ("full shadow")
 ** 4-10	unused
 ** 11		top row
 ** 12		next row
 ** 13		next row
 ** 14		next row
 ** 15		bottom row
 **
 *                                                                                             *
 * INPUT:   text  -- Pointer to text to render. *
 *                                                                                             *
 *          x,y   -- Pixel coordinate for to print text. *
 *                                                                                             *
 *          fore  -- Foreground color. *
 *                                                                                             *
 *          back  -- Background color. *
 *                                                                                             *
 *          flag  -- Text print control flags. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/24/1991 JLB : Created. * 10/26/94   JLB : Handles font X
 *spacing in a more friendly manner.                        *
 *=============================================================================================*/
void Simple_Text_Print(const char* text, int x, int y,
                       RemapControlType* fore, int back,
                       TextPrintType flag) {
  static int yspace = 0;          // Y spacing adjustment for font.
  static int xspace = 0;          // Spacing adjustment for font.
  std::span<const std::byte> font = {};  // Font to use.
  unsigned char fontpalette[16];  // Working font palette array.

  if (fore == nullptr) {
    fore = &ColorRemaps[PCOLOR_RED];
  }

  /*
  ** Init the font palette to the given background color
  */
  base::FillBytes(std::as_writable_bytes(base::Suffix(fontpalette, 0)), back,
                  16);

  int forecolor = fore->Color;

  /*
  **	A gradient font always requires special fixups for the palette
  */
  const TextPrintType point = flag & static_cast<TextPrintType>(0x000F);
  if (point == TPF_VCR || point == TPF_6PT_GRAD || point == TPF_METAL12 ||
      point == TPF_EFNT || point == TPF_TYPE) {
    /*
    ** If a gradient palette is specified, copy the remap table directly,
    *otherwise *	use the foreground color as the entire font remap color.
    */
    if (base::Any(flag & TPF_USE_GRAD_PAL)) {
      base::CopyBytes(base::ObjectBytes(fontpalette),
                      base::ObjectBytes(fore->FontRemap), 16);
      forecolor = fore->Color;
      if (point == TPF_TYPE) {
        forecolor = base::At(fontpalette, 1);
      }
    } else {
      base::FillBytes(std::as_writable_bytes(base::Suffix(fontpalette, 4)),
                      fore->Color, 12);
      forecolor = fore->Color;
    }

    /*
    ** Medium color: set all font colors to a medium value.  This flag
    ** overrides any gradient effects.
    */
    if (base::Any(flag & TPF_MEDIUM_COLOR)) {
      forecolor = fore->Color;
      base::FillBytes(std::as_writable_bytes(base::Suffix(fontpalette, 4)),
                      fore->Color, 12);
    }

    /*
    ** Bright color: set all font colors to a bright value.  This flag
    ** overrides any gradient effects.
    */
    if (base::Any(flag & TPF_BRIGHT_COLOR)) {
      forecolor = fore->Bright;
      base::FillBytes(std::as_writable_bytes(base::Suffix(fontpalette, 4)),
                      fore->BrightColor, 12);
    }
  }

  /*
  **	Change the current font if it differs from the font desired.
  */
  xspace = 1;
  yspace = 0;

  switch (point) {
    case TPF_SCORE:
      font = ScoreFontPtr;
      break;

    case TPF_METAL12:
      font = Metal12FontPtr;
      // xspace += 1;
      break;

    case TPF_MAP:
      font = MapFontPtr;
      xspace -= 1;
      break;

    case TPF_VCR:
      font = VCRFontPtr;
      break;

    case TPF_6PT_GRAD:
      font = GradFont6Ptr;
      xspace -= 1;
      break;

    case TPF_3POINT:
      xspace += 1;
      font = Font3Ptr;
      flag = flag & ~(TPF_DROPSHADOW | TPF_FULLSHADOW | TPF_NOSHADOW);
      break;

    case TPF_6POINT:
      font = Font6Ptr;
      xspace -= 1;
      break;

    case TPF_EFNT:
      font = EditorFont;
      yspace += 1;
      xspace -= 1;
      xspace -= 1;
      break;

    case TPF_8POINT:
      font = Font8Ptr;
      xspace -= 2;
      yspace -= 4;
      break;

    case TPF_LED:
      xspace -= 4;
      font = FontLEDPtr;
      break;

    case TPF_TYPE:
      font = TypeFontPtr;
      xspace -= 1;

      if constexpr (config::kWolapiEnabled) {
        xspace -= 2;
        yspace += 2;
      } else if constexpr (config::kBuildLanguage ==
                           config::BuildLanguage::German) {
        //	ajw: "I am implicitly assuming that TPF_TYPE was no longer being
        //	used, before I came along, despite the following."
        yspace += 4;  // VG 10/17/96
      }

      break;

    case TextPrintType::TPF_LASTPOINT:
    case TextPrintType::TPF_NOSHADOW:
    case TextPrintType::TPF_DROPSHADOW:
    case TextPrintType::TPF_FULLSHADOW:
    case TextPrintType::TPF_LIGHTSHADOW:
    case TextPrintType::TPF_CENTER:
    case TextPrintType::TPF_RIGHT:
    case TextPrintType::TPF_MEDIUM_COLOR:
    case TextPrintType::TPF_BRIGHT_COLOR:
    case TextPrintType::TPF_USE_GRAD_PAL:
    default:
      font = FontPtr;
      break;
  }

  /*
  **	Change the current font palette according to the dropshadow flags.
  */
  const TextPrintType shadow =
      flag & (TPF_NOSHADOW | TPF_DROPSHADOW | TPF_FULLSHADOW |
              TPF_LIGHTSHADOW);  // Requested shadow value.
  switch (shadow) {
    /*
    **	The text is rendered plain.
    */
    case TPF_NOSHADOW:
      base::At(fontpalette, 2) = static_cast<unsigned char>(back);
      base::At(fontpalette, 3) = static_cast<unsigned char>(back);
      xspace -= 1;
      yspace -= 2;
      break;

    /*
    **	The text is rendered with a simple
    **	drop shadow.
    */
    case TPF_DROPSHADOW:
      base::At(fontpalette, 2) = kBlack;
      base::At(fontpalette, 3) = static_cast<unsigned char>(back);
      xspace -= 1;
      break;

    /*
    **	Special engraved text look for the options
    **	dialog system.
    */
    case TPF_LIGHTSHADOW:
      base::At(fontpalette, 2) = (14 * 16) + 7 + 1;
      base::At(fontpalette, 3) = static_cast<unsigned char>(back);
      xspace -= 1;
      break;

    /*
    **	Each letter is surrounded by black. This is used
    **	when the text will be over a non-plain background.
    */
    case TPF_FULLSHADOW:
      base::At(fontpalette, 2) = kBlack;
      base::At(fontpalette, 3) = kBlack;
      xspace -= 1;
      break;

    case TextPrintType::TPF_LASTPOINT:
    case TextPrintType::TPF_6POINT:
    case TextPrintType::TPF_8POINT:
    case TextPrintType::TPF_3POINT:
    case TextPrintType::TPF_LED:
    case TextPrintType::TPF_VCR:
    case TextPrintType::TPF_6PT_GRAD:
    case TextPrintType::TPF_MAP:
    case TextPrintType::TPF_METAL12:
    case TextPrintType::TPF_EFNT:
    case TextPrintType::TPF_TYPE:
    case TextPrintType::TPF_SCORE:
    case TextPrintType::TPF_CENTER:
    case TextPrintType::TPF_RIGHT:
    case TextPrintType::TPF_MEDIUM_COLOR:
    case TextPrintType::TPF_BRIGHT_COLOR:
    case TextPrintType::TPF_USE_GRAD_PAL:
    default:
      break;
  }
  if (point != TPF_TYPE) {
    base::At(fontpalette, 0) = static_cast<unsigned char>(back);
    base::At(fontpalette, 1) = fore->Color;
  }

  /*
  **	Set the font and spacing according to the values they should be.
  */
  FontXSpacing = xspace;
  FontYSpacing = yspace;
  Set_Font(font);
  Set_Font_Palette(fontpalette);

  /*
  **	Display the (centered) message if there is one.
  */
  if (text && *text) {
    switch (flag & (TPF_CENTER | TPF_RIGHT)) {
      case TPF_CENTER:
        x -= String_Pixel_Width(text) / 2;
        break;

      case TPF_RIGHT:
        x -= String_Pixel_Width(text);
        break;

      case TextPrintType::TPF_LASTPOINT:
      case TextPrintType::TPF_6POINT:
      case TextPrintType::TPF_8POINT:
      case TextPrintType::TPF_3POINT:
      case TextPrintType::TPF_LED:
      case TextPrintType::TPF_VCR:
      case TextPrintType::TPF_6PT_GRAD:
      case TextPrintType::TPF_MAP:
      case TextPrintType::TPF_METAL12:
      case TextPrintType::TPF_EFNT:
      case TextPrintType::TPF_TYPE:
      case TextPrintType::TPF_SCORE:
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

    if (x < LogicPage->Get_Width() && y < LogicPage->Get_Height()) {
      LogicPage->Print(text, x, y, forecolor, back);
      //			LogicPage->Print(text, x, y, fore->Color, back);
    }
  }
}

/***********************************************************************************************
 * Fancy_Text_Print -- Prints text with a drop shadow. *
 *                                                                                             *
 *    This routine functions like Text_Print, but will render a drop * shadow
 *(in black). *
 *                                                                                             *
 * INPUT:   text  -- Text number to print. *
 *                                                                                             *
 *          x,y   -- Pixel coordinate for to print text. *
 *                                                                                             *
 *          fore  -- Foreground color. *
 *                                                                                             *
 *          back  -- Background color. *
 *                                                                                             *
 *          flag  -- Text print control flags. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   This routine is much slower than normal text print and * if
 *rendered to the SEENPAGE, the intermediate rendering                         *
 *             steps could be visible. *
 *                                                                                             *
 * HISTORY: * 11/29/1994 JLB : Created *
 *=============================================================================================*/
void Fancy_Text_Print(const int text, const int x, const int y,
                      RemapControlType* fore, const int back,
                      const TextPrintType flag,
                      const absl::Span<const absl::FormatArg> args) {
  if (text != TXT_NONE) {
    Fancy_Text_Print(Text_String(text), x, y, fore, back, flag, args);
  } else {
    /*
    **	Just the flags are to be changed, since the text number is TXT_NONE.
    */
    Simple_Text_Print(nullptr, x, y, fore, back, flag);
  }
}

/***********************************************************************************************
 * Fancy_Text_Print -- Prints text with a drop shadow. *
 *                                                                                             *
 *    This routine functions like Text_Print, but will render a drop * shadow
 *(in black). *
 *                                                                                             *
 * INPUT:   text  -- Pointer to text to render. *
 *                                                                                             *
 *          x,y   -- Pixel coordinate for to print text. *
 *                                                                                             *
 *          fore  -- Foreground color. *
 *                                                                                             *
 *          back  -- Background color. *
 *                                                                                             *
 *          flag  -- Text print control flags. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   This routine is much slower than normal text print and * if
 *rendered to the SEENPAGE, the intermediate rendering                         *
 *             steps could be visible. *
 *                                                                                             *
 * HISTORY: * 12/24/1991 JLB : Created. * 10/26/94   JLB : Handles font X
 *spacing in a more friendly manner.                        * 11/29/1994 JLB :
 *Separated actual draw action.                                            *
 *=============================================================================================*/
void Fancy_Text_Print(const char* text, const int x, const int y,
                      RemapControlType* fore, const int back,
                      const TextPrintType flag,
                      const absl::Span<const absl::FormatArg> args) {
  if (text) {
    const std::string formatted = port::FormatRuntime(text, args);
    Simple_Text_Print(formatted.c_str(), x, y, fore, back, flag);
  } else {
    /*
    **	Just the flags are desired to be changed, so call the simple print
    *routine with *	a nullptr text pointer.
    */
    Simple_Text_Print(nullptr, x, y, fore, back, flag);
  }
}

/***********************************************************************************************
 * Clip_Text_Print -- Prints text with clipping and <TAB> support. *
 *                                                                                             *
 *    Use this routine to print text that that should be clipped at an arbitrary
 *right margin  * as well as possibly recognizing <TAB> characters. Typical
 *users of this routine would    * be list boxes. *
 *                                                                                             *
 * INPUT:   text  -- Reference to the text to print. *
 *                                                                                             *
 *          x,y   -- Pixel coordinate of the upper left corner of the text
 *position.           *
 *                                                                                             *
 *          fore  -- The foreground color to use. *
 *                                                                                             *
 *          back  -- The background color to use. *
 *                                                                                             *
 *          flag  -- The text print flags to use. *
 *                                                                                             *
 *          width -- The maximum pixel width to draw the text. Extra characters
 *beyond this    * point will not be printed. *
 *                                                                                             *
 *          tabs  -- Optional pointer to a series of pixel tabstop positions. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/21/1995 JLB : Created. *
 *=============================================================================================*/
void Conquer_Clip_Text_Print(const char* text, int x, int y,
                             RemapControlType* fore, int back,
                             TextPrintType flag, int width,
                             std::span<const int> tabs) {
  if (text == nullptr) {
    return;
  }
  char buffer[512];
  port::SafeCopy(buffer, text);
  Simple_Text_Print(nullptr, 0, 0, nullptr, kTBlack, flag);
  std::span<char> source(buffer);
  int offset = 0;
  while (offset < width && !source.empty() && source.front() != '\0') {
    const auto text_view = std::string_view(source.data());
    const auto tab = text_view.find('\t');
    const auto count = tab == std::string_view::npos ? text_view.size() : tab;
    int line_width = 0;
    size_t visible = 0;
    while (visible < count) {
      const int next = Char_Pixel_Width(source[visible]);
      if (offset + line_width + next >= width) {
        break;
      }
      line_width += next;
      ++visible;
    }
    source[visible] = '\0';
    Simple_Text_Print(source.data(), x + offset, y, fore, back, flag);
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

/***************************************************************************
 * Plain_Text_Print -- Prints text without using a color scheme            *
 *                                                                         *
 * INPUT:                                                                  *
 *		text		text to print
 ** x,y		coords to print at
 ** fore		desired foreground color
 ** back		desired background color
 ** flag		text print control flags
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		Do not use the gradient control flag with this routine!  For
 ** a gradient appearance, use Fancy_Text_Print.
 ** Despite this routine's name, it is actually faster to call
 ** Fancy_Text_Print than this routine.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   01/05/1996 BRR : Created.                                             *
 *=========================================================================*/
void Plain_Text_Print(const int text, const int x, const int y, const int fore,
                      const int back, const TextPrintType flag,
                      const absl::Span<const absl::FormatArg> args) {
  RemapControlType scheme{};

  base::FillBytes(std::as_writable_bytes(base::Suffix(scheme.FontRemap, 4)),
                  fore, 12);

  scheme.BrightColor = static_cast<unsigned char>(fore);
  scheme.Color = static_cast<unsigned char>(fore);
  scheme.Shadow = static_cast<unsigned char>(fore);
  scheme.Background = static_cast<unsigned char>(fore);
  scheme.Corners = static_cast<unsigned char>(fore);
  scheme.Highlight = static_cast<unsigned char>(fore);
  scheme.Box = static_cast<unsigned char>(fore);
  scheme.Bright = static_cast<unsigned char>(fore);
  scheme.Underline = static_cast<unsigned char>(fore);
  scheme.Bar = static_cast<unsigned char>(fore);

  Fancy_Text_Print(text, x, y, &scheme, back, flag, args);
}

/***************************************************************************
 * Plain_Text_Print -- Prints text without using a color scheme            *
 *                                                                         *
 * INPUT:                                                                  *
 *		text		text to print
 ** x,y		coords to print at
 ** fore		desired foreground color
 ** back		desired background color
 ** flag		text print control flags
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		Do not use the gradient control flag with this routine!  For
 ** a gradient appearance, use Fancy_Text_Print.
 ** Despite this routine's name, it is actually faster to call
 ** Fancy_Text_Print than this routine.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   01/05/1996 BRR : Created.                                             *
 *=========================================================================*/
void Plain_Text_Print(const char* text, const int x, const int y,
                      const int fore, const int back, const TextPrintType flag,
                      const absl::Span<const absl::FormatArg> args) {
  RemapControlType scheme{};

  base::FillBytes(std::as_writable_bytes(base::Suffix(scheme.FontRemap, 4)),
                  fore, 12);

  scheme.BrightColor = static_cast<unsigned char>(fore);
  scheme.Color = static_cast<unsigned char>(fore);
  scheme.Shadow = static_cast<unsigned char>(fore);
  scheme.Background = static_cast<unsigned char>(fore);
  scheme.Corners = static_cast<unsigned char>(fore);
  scheme.Highlight = static_cast<unsigned char>(fore);
  scheme.Box = static_cast<unsigned char>(fore);
  scheme.Bright = static_cast<unsigned char>(fore);
  scheme.Underline = static_cast<unsigned char>(fore);
  scheme.Bar = static_cast<unsigned char>(fore);

  Fancy_Text_Print(text, x, y, &scheme, back, flag, args);
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
void Draw_Caption(int text, int x, int y, int w) {
  Draw_Caption(Text_String(text), x, y, w);
}

void Draw_Caption(const char* text, int x, int y, int w) {
  /*
  **	Draw the caption.
  */
  if (text != nullptr && *text != '\0') {
    if (MapEditorActive) {
      Fancy_Text_Print(text, (w / 2) + x, 4 + y,
                       GadgetClass::Get_Color_Scheme(), kTBlack,
                       TPF_CENTER | TPF_EFNT | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
    } else {
      Fancy_Text_Print(text, (w / 2) + x, 16 + y,
                       GadgetClass::Get_Color_Scheme(), kTBlack,
                       TPF_CENTER | kTpfText);
      const int length = String_Pixel_Width(text);
      LogicPage->Draw_Line(
          x + (w / 2) - (length / 2), y + FontHeight + FontYSpacing + 16,
          x + (w / 2) + (length / 2), y + FontHeight + FontYSpacing + 16,
          GadgetClass::Get_Color_Scheme()->Box);
    }
  }
}
