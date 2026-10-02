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

/* $Header: /CounterStrike/TEXTBTN.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : TEXTBTN.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 01/15/95 *
 *                                                                                             *
 *                  Last Update : January 15, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_GAME_UI_TEXTBTN_H_
#define CNC_RED_ALERT_GAME_UI_TEXTBTN_H_

#include "absl/base/attributes.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/wwstd.h"
#include "game/ui/control.h"
#include "game/ui/toggle.h"
#include "game/ui/widget_skin.h"

namespace game {

// A raised box with a centered caption that presses down when clicked. A
// width or height of -1 sizes the button to its text.
// NOLINTBEGIN(portability-template-virtual-member-function): see WidgetSkin.
template <WidgetSkin Skin>
class TextButtonClass final : public ToggleClass {
 public:
  TextButtonClass()
      : ToggleClass(0, 0, 0, 0, 0),
        IsBlackBorder(false),
        String(nullptr),
        TextFont(Skin::kButtonDefaultFont),
        PrintFlags(Skin::kButtonDefaultFlags) {}
  TextButtonClass(unsigned id, const char* text ABSL_ATTRIBUTE_LIFETIME_BOUND,
                  Skin::Font font, Skin::PrintFlags style, int x, int y,
                  int w = -1, int h = -1, bool blackborder = false)
      : ToggleClass(id, x, y, w, h),
        IsBlackBorder(blackborder),
        String(text),
        TextFont(font),
        PrintFlags(style) {
    Size_To_Text(w, h);
  }
  TextButtonClass(unsigned id, int text, Skin::Font font,
                  Skin::PrintFlags style, int x, int y, int w = -1, int h = -1,
                  bool blackborder = false)
      : ToggleClass(id, x, y, w, h),
        IsBlackBorder(blackborder),
        String(nullptr),
        TextFont(font),
        PrintFlags(style) {
    Set_Text(text);
    Size_To_Text(w, h);
  }

  bool Draw_Me(PixelView& view, bool forced = false) override {
    if (ControlClass::Draw_Me(view, forced)) {
      Draw_Background(view);
      Draw_Text(view, String);
      return true;
    }
    return false;
  }

  // Shows `text`, resizing the button to it when `resize`.
  void Set_Text(const char* text, bool resize = false) {
    String = text;
    Flag_To_Redraw();
    if (resize && String) {
      Width = Skin::Text_Width(TextFont, PrintFlags, String) + 8;
      Height = Skin::Text_Height(TextFont, PrintFlags) + 2;
    }
  }
  // Shows text number `text`; the game's "no text" number changes nothing.
  void Set_Text(int text, bool resize = false) {
    if (text != Skin::kTextNone) {
      Set_Text(Skin::Text_String(text), resize);
    }
  }
  void Set_Style(Skin::Font font, Skin::PrintFlags style) {
    TextFont = font;
    PrintFlags = style;
  }

 protected:
  void Draw_Background(PixelView& view) {
    if (IsBlackBorder) {
      view.DrawRect(X - 1, Y - 1, X + Width + 2, Y + Height + 2, kBlack);
    }
    Skin::Draw_Button_Box(view, X, Y, Width, Height, TextFont, PrintFlags,
                          IsDisabled, IsPressed);
  }
  void Draw_Text(PixelView& view, const char* text) {
    if (String) {
      Skin::Print_Button_Text(view, text, X + (Width / 2) - 1, Y + 1, TextFont,
                              PrintFlags, IsDisabled, IsPressed, IsOn);
    }
  }

  bool IsBlackBorder : 1;

  /*
  **	This points to a constant string that is used for the button's text.
  */
  const char* String;

  // The font and the print flags the button's text renders with.
  Skin::Font TextFont;
  Skin::PrintFlags PrintFlags;

 private:
  // Applies the -1 ("fit the text") width and height of a constructor.
  void Size_To_Text(int w, int h) {
    if (w == -1) {
      Width = Skin::Text_Width(TextFont, PrintFlags, String) + 8;
    }
    if (h == -1) {
      Height = Skin::Text_Height(TextFont, PrintFlags) + 2;
    }
  }
};
// NOLINTEND(portability-template-virtual-member-function)

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_UI_TEXTBTN_H_
