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

/* $Header:   F:\projects\c&c\vcs\code\checkbox.cpv   1.6   16 Oct 1995 16:49:36
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : CHECKBOX.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 05/26/95 *
 *                                                                                             *
 *                  Last Update : July 1, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * CheckBoxClass::Draw_Me -- Draws the checkbox imagery. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/ui/checkbox.h"

#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/wwstd.h"
#include "engine/window/hardware_cursor.h"
#include "game/ui/input.h"
#include "td/core/defines.h"
#include "td/ui/dialog.h"
#include "td/ui/toggle.h"

namespace td {

bool CheckBoxClass::Draw_Me(PixelView& view, bool forced) {
  if (ToggleClass::Draw_Me(view, forced)) {
    game::TheMouse().Hide();
    Draw_Box(view, X, Y, Width, Height, BOXSTYLE_GREEN_DOWN, false);
    view.FillRect(X + 1, Y + 1, X + Width - 2, Y + Height - 2, kGrey);
    if (IsOn) {
      view.DrawLine(X + 1, Y + 1, X + Width - 2, Y + Height - 2, kBlack);
      view.DrawLine(X + Width - 2, Y + 1, X + 1, Y + Height - 2, kBlack);
    }
    game::TheMouse().Show();
    return true;
  }
  return false;
}

}  // namespace td
