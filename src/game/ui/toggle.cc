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

/* $Header: /CounterStrike/TOGGLE.CPP 1     3/03/97 10:26a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : TOGGLE.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 01/15/95 *
 *                                                                                             *
 *                  Last Update : February 2, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * ToggleClass::ToggleClass -- Normal constructor for toggle button
 *gadgets.                 * ToggleClass::Turn_Off -- Turns the toggle button to
 *the "OFF" state.                      * ToggleClass::Turn_On -- Turns the
 *toggle button to the "ON" state.                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "game/ui/toggle.h"

#include "base/flags.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/ui/control.h"
#include "game/ui/gadget.h"
#include "game/ui/input.h"

namespace game {

/***********************************************************************************************
 * ToggleClass::ToggleClass -- Normal constructor for toggle button gadgets. *
 *                                                                                             *
 *    This is the normal constructor for toggle buttons. A toggle button is one
 *that most      * closely resembles the Windows style. It has an up and down
 *state as well as an on        * and off state. *
 *                                                                                             *
 * INPUT:   id    -- ID number for this button. *
 *                                                                                             *
 *          x,y   -- Pixel coordinate of upper left corner of this button. *
 *                                                                                             *
 *          w,h   -- Width and height of the button. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/15/1995 JLB : Created. *
 *=============================================================================================*/
ToggleClass::ToggleClass(unsigned id, int x, int y, int w, int h) noexcept
    : ControlClass(id, x, y, w, h,
                   InputFlags::kLeftPress | InputFlags::kLeftRelease, true) {}

/***********************************************************************************************
 * ToggleClass::Turn_On -- Turns the toggle button to the "ON" state. *
 *                                                                                             *
 *    This routine will turn the button on. The button will also be flagged to
 *be redrawn      * at the next opportunity. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/15/1995 JLB : Created. *
 *=============================================================================================*/
void ToggleClass::Turn_On() {
  IsOn = true;
  FlagToRedraw();
}

/***********************************************************************************************
 * ToggleClass::Turn_Off -- Turns the toggle button to the "OFF" state. *
 *                                                                                             *
 *    This routine will turn the toggle button "off". It will also be flagged to
 *be redrawn    * at the next opportunity. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/15/1995 JLB : Created. *
 *=============================================================================================*/
void ToggleClass::Turn_Off() {
  IsOn = false;
  FlagToRedraw();
}

/***********************************************************************************************
 * ToggleClass::Action -- Handles mouse clicks on a text button. *
 *                                                                                             *
 *    This routine will process any mouse or keyboard event that is associated
 *with this       * button object. It detects and flags the text button so that
 *it will properly be drawn    * in a pressed or raised state. It also handles
 *any toggle state for the button.           *
 *                                                                                             *
 * INPUT:   flags -- The event flags that triggered this button. *
 *                                                                                             *
 *          key   -- The key that triggered this event: KN_NONE for a mouse
 *                   click, or a regular key if this text button has a hotkey. *
 *                                                                                             *
 * OUTPUT:  Returns whatever the lower level processing for buttons decides.
 *This is usually   * true. *
 *                                                                                             *
 * WARNINGS:   The button is flagged to be redrawn by this routine. *
 *                                                                                             *
 * HISTORY: * 01/14/1995 JLB : Created. * 02/02/1995 JLB : Left press doesn't
 *get passed to other buttons now                       *
 *=============================================================================================*/
bool ToggleClass::Action(InputFlags flags, engine::window::KeyNumber& key,
                         int x, int y) {
  /*
  **	If there are no action flag bits set, then this must be a forced call. A
  *forced call *	must never actually function like a real call, but
  *rather only performs any necessary *	graphic updating.
  */
  const bool overbutton = static_cast<unsigned>(TheMouse().x() - this->x()) <
                              static_cast<unsigned>(width()) &&
                          static_cast<unsigned>(TheMouse().y() - this->y()) <
                              static_cast<unsigned>(height());
  if (!base::Any(flags)) {
    if (overbutton) {
      if (!IsPressed) {
        IsPressed = true;
        FlagToRedraw();
      }
    } else {
      if (IsPressed) {
        IsPressed = false;
        FlagToRedraw();
      }
    }
  }

  /*
  **	Handle the sticky state for this gadget. It must be processed here
  **	because the event flags might be cleared before the action function
  **	is called.
  */
  UpdateMouseCapture(flags);

  /*
  **	Flag the button to show the pressed down imagery if this mouse button
  **	was pressed over this gadget.
  */
  if (base::Any(flags & InputFlags::kLeftPress)) {
    IsPressed = true;
    FlagToRedraw();
    flags &= ~InputFlags::kLeftPress;
    ControlClass::Action(flags, key, x, y);
    key = engine::window::KN_NONE;  // erase the event
    return true;                    // stop processing other buttons now
  }

  if (base::Any(flags & InputFlags::kLeftRelease)) {
    if (IsPressed) {
      if (IsToggleType && overbutton) {
        IsOn = !static_cast<bool>(IsOn);
      }
      IsPressed = false;
      FlagToRedraw();
    } else {
      flags &= ~InputFlags::kLeftRelease;
    }
  }

  /*
  **	Do normal button processing. This ends up causing the button's ID number
  *to *	be returned from the controlling Input() function.
  */
  return ControlClass::Action(flags, key, x, y);
}

}  // namespace game
