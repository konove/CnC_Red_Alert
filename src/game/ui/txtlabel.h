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

/* $Header: /CounterStrike/TXTLABEL.H 1     3/03/97 10:26a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : TXTLABEL.H *
 *                                                                                             *
 *                   Programmer : Bill Randolph
 **
 *                                                                                             *
 *                   Start Date : 02/06/95 *
 *                                                                                             *
 *                  Last Update : February 6, 1995 [BR]
 **
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_GAME_UI_TXTLABEL_H_
#define CNC_RED_ALERT_GAME_UI_TXTLABEL_H_

#include "absl/base/attributes.h"
#include "engine/gfx/pixel_buffer.h"
#include "game/ui/gadget.h"
#include "game/ui/widget_skin.h"

namespace game {

// A line of text that is drawn but takes no input. label_data() holds the
// fields the game's message list keeps on each label.
// NOLINTBEGIN(portability-template-virtual-member-function): see WidgetSkin.
template <WidgetSkin Skin>
class TextLabelClass : public GadgetClass {
 public:
  // Red Alert's colour is a scheme the label points at, Tiberian Dawn's a
  // palette index, which cannot be marked lifetimebound.
  // NOLINTBEGIN(clang-diagnostic-lifetime-safety-intra-tu-constructor-suggestions)
  TextLabelClass(char* txt ABSL_ATTRIBUTE_LIFETIME_BOUND, int x, int y,
                 Skin::TextColor color, Skin::Font font, Skin::PrintFlags style)
      : GadgetClass(x, y, 1, 1, 0, false),
        TextFont(font),
        Style(style),
        Text(txt),
        Color(color) {}
  // NOLINTEND(clang-diagnostic-lifetime-safety-intra-tu-constructor-suggestions)

  bool Draw_Me(PixelView& view, bool forced = false) override {
    if (!GadgetClass::Draw_Me(view, forced)) {
      return false;
    }
    if (PixWidth == -1) {
      Skin::Print_Label(view, Text, X, Y, Color, TextFont, Style);
    } else {
      Skin::Print_Clipped(view, Text, X, Y, Color, TextFont, Style, PixWidth);
    }
    return true;
  }

  // Sets the displayed text of the label.
  virtual void Set_Text(char* txt) { Text = txt; }

  // The game's own bookkeeping for this label (its timeout, id or segments).
  Skin::LabelData& label_data() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return label_data_;
  }
  [[nodiscard]] const Skin::LabelData& label_data() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return label_data_;
  }

  Skin::Font TextFont;
  Skin::PrintFlags Style;
  char* Text;
  Skin::TextColor Color;
  int PixWidth{-1};  // -1: not clipped

 private:
  Skin::LabelData label_data_;
};
// NOLINTEND(portability-template-virtual-member-function)

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_UI_TXTLABEL_H_
