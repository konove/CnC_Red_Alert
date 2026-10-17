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

/* $Header: /CounterStrike/CHECKBOX.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : CHECKBOX.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 05/26/95 *
 *                                                                                             *
 *                  Last Update : May 26, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_GAME_UI_CHECKBOX_H_
#define CNC_RED_ALERT_GAME_UI_CHECKBOX_H_

#include "engine/gfx/pixel_buffer.h"
#include "engine/window/keyboard.h"
#include "game/ui/input.h"
#include "game/ui/toggle.h"
#include "game/ui/widget_skin.h"

namespace game {

// A 7x7 check box that turns itself on and off when clicked; read IsOn when
// its ID comes back.
// NOLINTBEGIN(portability-template-virtual-member-function): see WidgetSkin.
template <WidgetSkin Skin>
class CheckBoxClass : public ToggleClass {
 public:
  CheckBoxClass(unsigned id, int x, int y) : ToggleClass(id, x, y, 7, 7) {}

  bool Draw(PixelView& view, bool forced = false) override {
    if (!ToggleClass::Draw(view, forced)) {
      return false;
    }
    TheMouse().Hide();
    Skin::Draw_Check_Box(view, x(), y(), width(), height(), IsOn);
    TheMouse().Show();
    return true;
  }

 protected:
  bool Action(unsigned flags, engine::window::KeyNumber& key, int x,
              int y) override {
    // Only a release that ends a press on the box toggles it: that is the
    // release ToggleClass reports as the box's ID, so whoever reads IsOn when
    // the ID comes back sees the state drawn.
    if (flags & kLeftRelease && IsPressed) {
      if (IsOn) {
        Turn_Off();
      } else {
        Turn_On();
      }
    }
    return ToggleClass::Action(flags, key, x, y);
  }
};
// NOLINTEND(portability-template-virtual-member-function)

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_UI_CHECKBOX_H_
