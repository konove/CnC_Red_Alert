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

/* $Header: /CounterStrike/GADGET.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : GADGET.CPP *
 *                                                                                             *
 *                   Programmer : Maria del Mar McCready Legg * Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 01/03/95 *
 *                                                                                             *
 *                  Last Update : August 1, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "game/ui/gadget.h"

#include "engine/gfx/pixel_buffer.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/ui/gadget_list.h"
#include "game/ui/input.h"

namespace game {

using enum engine::window::KeyNumber;

/*
**	This records the current gadget the the gadget system is "stuck on".
*Such a *	gadget will be processed to the exclusion of all others until
*the mouse button *	is no longer pressed.
*/
GadgetClass* GadgetClass::StuckOn = nullptr;

/*
**	This points to the gadget that is intercepting all keyboard events.
*/
GadgetClass* GadgetClass::Focused = nullptr;

/*
** This points to the current color scheme for drawing all gadgets.
*/
void (*GadgetClass::KeyHook)(engine::window::KeyNumber key) = nullptr;

/***********************************************************************************************
 * GadgetClass::GadgetClass -- Constructor for gadget object. *
 *                                                                                             *
 *    This is the normal constructor for gadget objects. A gadget object is only
 *concerned     * with the region on the screen to considered "its own" as well
 *as the flags that tell     * what mouse action should be recognized when the
 *mouse is over this screen area.          *
 *                                                                                             *
 * INPUT:   x,y   -- Coordinates (in pixels) of the upper left hand corner of
 *the region that  * will be "owned" by this gadget. *
 *                                                                                             *
 *          w,h   -- Width and height (in pixels) of this gadget's region. *
 *                                                                                             *
 *          flags -- The flags (mouse conditions) that will cause this gadget's
 *action         * function to be called. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/03/1995 MML : Created. *
 *=============================================================================================*/
GadgetClass::GadgetClass(int x, int y, int w, int h, unsigned flags,
                         bool sticky) noexcept
    : X(x),
      Y(y),
      Width(w),
      Height(h),

      IsSticky(sticky),

      Flags(flags) {
  if (IsSticky) {
    Flags |= kLeftPress | kLeftRelease;
  }
}

/***********************************************************************************************
 * GadgetClass::~GadgetClass -- Destructor for gadget object. *
 *                                                                                             *
 *    This is the destructor for the gadget object. It will clear the focus from
 *this gadget   * if this gadget currently has the focus. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/08/1995 JLB : Created. *
 *=============================================================================================*/
GadgetClass::~GadgetClass() {
  // Drop the keyboard focus directly. Clear_Focus is virtual -- DropListClass
  // collapses its drop-down in its override -- and a destructor cannot reach
  // that override; a derived class that needs more does it in its own
  // destructor, while the object is still of that type.
  if (Focused == this) {
    Flags &= ~kKeyboard;
    Focused = nullptr;
  }

  if (this == StuckOn) {
    StuckOn = nullptr;
  }

  // Not through Remove(): its hook is a virtual call this destructor cannot
  // make any more.
  if (list_ != nullptr) {
    list_->Erase(*this);
  }

  if (this == Focused) {
    Focused = nullptr;
  }
}

/***************************************************************************
 * GADGETCLASS::CLICKEDON -- If a mouse click is detected within gadget's  *
 *      area and the appropriate flag is set, then call Action().          *
 *                                                                         *
 * INPUT:      int key, int mousex, int mousey                             *
 *                                                                         *
 * OUTPUT:     true or false                                               *
 *                                                                         *
 * WARNINGS:   none.                                                       *
 *                                                                         *
 * HISTORY:    01/03/1995 MML : Created.                                   *
 *=========================================================================*/
bool GadgetClass::Clicked_On(engine::window::KeyNumber& key, unsigned flags,
                             int mousex, int mousey) {
  /*
  **	Set flags to match only those events that occur AND are being looked
  *for. If *	the result is NULL, then we know that this button should be
  *ignored.
  */
  flags &= Flags;

  /*
  **	If keyboard input should be processed by this "gadget" and keyboard
  *input is *	detected, then always call the action function. It is up to the
  *action function *	in this case to either ignore the keyboard input or not.
  **
  **	For mouse actions, check to see if the mouse is in the region of the
  *button *	before calling the associated action function. This is the
  *typical action for *	buttons.
  */
  if (this == StuckOn || flags & kKeyboard ||
      (flags &&
       static_cast<unsigned>(mousex - X) < static_cast<unsigned>(Width) &&
       static_cast<unsigned>(mousey - Y) < static_cast<unsigned>(Height))) {
    return Action(flags, key, mousex, mousey);
  }
  return false;
}

/***********************************************************************************************
 * GadgetClass::Enable -- Enables the gadget. *
 *                                                                                             *
 *    This function enables the gadget. An enabled gadget will be processed for
 *input          * purposes. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/15/1995 JLB : Created. *
 *=============================================================================================*/
void GadgetClass::Enable() {
  IsDisabled = false;
  IsToRepaint = true;
  Clear_Focus();
}

/***********************************************************************************************
 * GadgetClass::Disable -- Disables the gadget from input processing. *
 *                                                                                             *
 *    This routine will disable the gadget. A disabled gadget might be rendered,
 *but is        * ignored for input processing. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/15/1995 JLB : Created. *
 *=============================================================================================*/
void GadgetClass::Disable() {
  IsDisabled = true;
  IsToRepaint = true;
  Clear_Focus();
}

/***********************************************************************************************
 * GadgetClass::Action -- Base action for gadget. *
 *                                                                                             *
 *    This handles the base level action that a gadget performs when a
 *qualifying input event  * is detected. This sets the redraw flag and returns
 *true (to stop further processing).    * If no qualifying input event was
 *detected, but this routine was called anyway, then      * don't perform any
 *action. The call to this routine, in that case, must have been forced  * for
 *some other reason. *
 *                                                                                             *
 * INPUT:   flag  -- The input event bits that qualify for this gadget. A NULL
 *indicates that  * no qualifying event occurred. *
 *                                                                                             *
 * OUTPUT:  bool; Should further gadget list processing be aborted? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/15/1995 JLB : Created. *
 *=============================================================================================*/
bool GadgetClass::Action(unsigned flags, engine::window::KeyNumber& /*unused*/,
                         int /*x*/, int /*y*/) {
  /*
  **	If any of the event flags are active, then this indicates that something
  *probably *	has changed the gadget. Flag the gadget to be redrawn. Also,
  *make sure that *	any sticky flags are cleared up.
  */
  if (flags) {
    IsToRepaint = true;
    Sticky_Process(flags);
    return true;
  }
  return false;
}

/***********************************************************************************************
 * GadgetClass::Draw_Me -- Gadget redraw action (flag control). *
 *                                                                                             *
 *    At this level, there is no actual rendering taking place with the call to
 *Draw_Me, but   * the IsToRepaint flag must be cleared. Derived objects will
 *call this routine and if it   * returns true, they will perform their custom
 *rendering.                                  *
 *                                                                                             *
 * INPUT:   forced   -- Is this redraw forced by outside circumstances? *
 *                                                                                             *
 * OUTPUT:  bool; Should the gadget imagery be redrawn? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/14/1995 JLB : Created. *
 *=============================================================================================*/
bool GadgetClass::Draw_Me(PixelView& /*view*/, bool forced) {
  if (forced || IsToRepaint) {
    IsToRepaint = false;
    return true;
  }
  return false;
}

GadgetClass::PendingInput GadgetClass::ReadInput() {
  PendingInput input;

  /*
  **	Fetch any pending keyboard input.
  */
  if (TheKeyboard().Peek()) {
    input.event = TheKeyboard().Read();
  }
  const engine::window::InputEvent& event = input.event;
  input.key = event.key;

  if (KeyHook != nullptr) {
    KeyHook(input.key);
  }

  /*
  **	For mouse button clicks, the position is the one the event recorded at
  **	the instant of the click rather than where the mouse is by the time we
  **	get around to this function.
  */
  if (event.IsClick()) {
    input.x = event.x;
    input.y = event.y;
  } else {
    input.x = TheMouse().x();
    input.y = TheMouse().y();
  }

  /*
  **	Set the mouse button state flags. These will be passed to the individual
  **	buttons so that they can determine what action to perform (if any).
  */
  using engine::window::MouseButton;
  if (event.IsPress(MouseButton::kLeft)) {
    input.flags |= kLeftPress;
  }
  if (event.IsPress(MouseButton::kRight)) {
    input.flags |= kRightPress;
  }
  if (event.IsRelease(MouseButton::kLeft)) {
    input.flags |= kLeftRelease;
  }
  if (event.IsRelease(MouseButton::kRight)) {
    input.flags |= kRightRelease;
  }

  /*
  **	If the mouse wasn't responsible for this key code, then it must be from
  **	the keyboard. Flag this fact.
  */
  if (input.key && !input.flags) {
    input.flags |= kKeyboard;
  }

  /*
  **	Mouse button up or down action is ignored if there is a keyboard event.
  **	This allows keyboard events to fall through normally even if the mouse
  * is *	over a gadget that is flagged for kLeftUp or kRightUp.
  */
  if (!event) {
    /*
    **	Check for the mouse being held down. We can't use the normal input
    **	system for this, so we must examine the actual current state of the
    **	mouse buttons. If a button isn't being held down, then it must be up.
    */
    if (engine::window::IsLeftButtonDown()) {
      input.flags |= kLeftHeld;
    } else {
      input.flags |= kLeftUp;
    }
    if (engine::window::IsRightButtonDown()) {
      input.flags |= kRightHeld;
    } else {
      input.flags |= kRightUp;
    }
  }
  return input;
}

bool GadgetClass::OfferToStuckOrFocused(PixelView& view, PendingInput& input) {
  /*
  **	If "sticky" processing is active, then only process the stuck gadget.
  */
  if (StuckOn) {
    StuckOn->Draw_Me(view, false);
    GadgetClass* oldstuck = StuckOn;
    StuckOn->Clicked_On(input.key, input.flags, input.x, input.y);
    if (StuckOn) {
      StuckOn->Draw_Me(view, false);
    } else {
      oldstuck->Draw_Me(view, false);
    }
    return true;
  }

  /*
  **	If there is a gadget that has the keyboard focus, then route all
  **	keyboard events to it.
  */
  if (Focused && input.flags & kKeyboard) {
    Focused->Draw_Me(view, false);
    Focused->Clicked_On(input.key, input.flags, input.x, input.y);
    if (Focused) {
      Focused->Draw_Me(view, false);
    }
    return true;
  }
  return false;
}

bool GadgetClass::Offer(PixelView& view, bool forced, PendingInput& input) {
  /*
  **	Maybe redraw the button if it needs to or is being forced to redraw.
  */
  Draw_Me(view, forced);

  /*
  **	Process this button. If the button was recognized and action was
  **	performed, then bail from further processing (speed reasons?).
  */
  if (!IsDisabled && Clicked_On(input.key, input.flags, input.x, input.y)) {
    /*
    **	Some buttons will require repainting when they perform some action.
    **	Do so at this time.
    */
    Draw_Me(view, false);
    return true;
  }
  return false;
}

engine::window::InputEvent GadgetClass::FinishInput(const PendingInput& input) {
  engine::window::InputEvent event = input.event;
  // A gadget that acted replaced the key, and used up the click with it.
  if (input.key != event.key) {
    event.key = input.key;
    event.button = engine::window::MouseButton::kNone;
  }
  return event;
}

/***********************************************************************************************
 * GadgetClass::Flag_To_Redraw -- Flags this gadget to be redrawn. *
 *                                                                                             *
 *    Use this routine to flag the gadget to be redrawn. A gadget so flagged
 *will have its     * Draw_Me function called at the next available opportunity.
 *Usually, this is the next     * time the Input() function is called. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/16/1995 JLB : Created. *
 *=============================================================================================*/
void GadgetClass::Flag_To_Redraw() { IsToRepaint = true; }

/***********************************************************************************************
 * GadgetClass::Sticky_Process -- Handles the sticky flag processing. *
 *                                                                                             *
 *    This function examines the event flags and handles any "sticky" processing
 *required.     * Sticky processing is when the button is flagged with the
 *"IsSticky" bit and it will      * be processed to the exclusion of all other
 *gadgets while the mouse button is held        * down. *
 *                                                                                             *
 * INPUT:   flags -- The event flags that triggered the call to this routine. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/16/1995 JLB : Created. *
 *=============================================================================================*/
void GadgetClass::Sticky_Process(unsigned flags) {
  if (IsSticky && flags & kLeftPress) {
    StuckOn = this;
  }
  if (StuckOn == this && flags & kLeftRelease) {
    StuckOn = nullptr;
  }
}

/***********************************************************************************************
 * GadgetClass::Set_Focus -- Sets focus to this gadget. *
 *                                                                                             *
 *    This will set the focus to this gadget regardless of any current focus
 *setting. If there * is another gadget that has focus, it will have its focus
 *cleared before this gadget will * get the focus. A focused gadget is one that
 *has all keyboard input routed to it.         *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/25/1995 JLB : Created. *
 *=============================================================================================*/
void GadgetClass::Set_Focus() {
  if (Focused) {
    Focused->Flag_To_Redraw();
    Focused->Clear_Focus();
  }
  Flags |= kKeyboard;
  Focused = this;
}

/***********************************************************************************************
 * GadgetClass::Clear_Focus -- Clears the focus if this gadget has it. *
 *                                                                                             *
 *    Use this function to clear the focus for the gadget. If the gadget doesn't
 *currently     * have focus, then this routine will do nothing. For added
 *functionality, overload this    * virtual function so that gadget specific
 *actions may be take when focus is lost.         *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/25/1995 JLB : Created. *
 *=============================================================================================*/
void GadgetClass::Clear_Focus() {
  if (Focused == this) {
    Flags &= ~kKeyboard;
    Focused = nullptr;
  }
}

/***********************************************************************************************
 * GadgetClass::Has_Focus -- Checks if this object currently has the keyboard
 *focus.           *
 *                                                                                             *
 *    If this object has the keyboard focus, then this routine will return true.
 *When the      * gadget has keyboard focus, all keyboard events get routed to
 *the gadget.                 *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Does this gadget have the keyboard focus? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/21/1995 JLB : Created. *
 *=============================================================================================*/
bool GadgetClass::Has_Focus() { return this == Focused; }

/***********************************************************************************************
 * GadgetClass::Set_Position -- Set the coordinate position of this gadget. *
 *                                                                                             *
 *    This routine helps with moving a gadget's location. It will set the
 *gadgets upper        * left corner to the pixel location specified. *
 *                                                                                             *
 * INPUT:   x,y   -- The X and Y position to put the gadget's upper left corner
 *to.            *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/01/1996 JLB : Created. *
 *=============================================================================================*/
void GadgetClass::Set_Position(int x, int y) {
  X = x;
  Y = y;
}

}  // namespace game
