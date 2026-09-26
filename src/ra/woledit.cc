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

/***************************************************************************
 * WOLEditClass -- Derived from EditClass, includes changes I wanted for
 *                 wolapi integration stuff.
 *					Note: An editbox of this class cannot be
 *made read-only. See comment below. HISTORY:    07/17/1998 ajw : Created.
 *=========================================================================*/

#include "ra/woledit.h"

#include "engine/gfx/font.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/wwstd.h"
#include "engine/window/keyboard.h"
#include "ra/control.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/gadget.h"
#include "ra/game_state.h"
#include "ra/jshell.h"

using enum engine::window::KeyNumber;

//***********************************************************************************************
void WOLEditClass::Draw_Text(PixelView& view, const char* text) {
  //	Only difference between this and EditClass: cursor shows up when
  //	string is at MaxLength.

  const TextPrintType flags =
      Has_Focus() ? TPF_BRIGHT_COLOR : static_cast<TextPrintType>(0);

  Conquer_Clip_Text_Print(view, text, X + 1, Y + 1, Color, kTBlack,
                          TextFlags | flags, Width - 2);

  const FontStyle font = TextFontStyle(TextFlags | flags);
  const int text_width = StringPixelWidth(font, text);
  if (Has_Focus() &&  //	strlen(text) < MaxLength &&
      text_width + StringPixelWidth(font, "_") < Width - 2) {
    Conquer_Clip_Text_Print(view, "_", X + 1 + text_width, Y + 1, Color,
                            kTBlack, TextFlags | flags);
  }
}

//***********************************************************************************************
//	Override of EditClass::Action, because the base class does not behave
// correctly in certain circumstances. 	(Escape key is being processed as enter
// key.) 	Again, I'm not about to change the base class directly, as I'm
// trying to have as minimal an affect as possible on 	the current game code.
// -ajw
bool WOLEditClass::Action(unsigned flags, engine::window::KeyNumber& key, int x,
                          int y) {
  //	(Mostly duplicated from base class ::Action)
  /*	For some painful reason, IsReadOnly is private in the base class, so I
     can't do the following. For this reason, don't make a WOLEditClass edit box
     read-only.

          //
          // If this is a read-only edit box, it's a display-only device
          //
          if (IsReadOnly) {
                  return(false);
          }
  */

  // debugprint( "WOLEditClass::Action this=%i, flags=0x%x, key=0x%x\n", this,
  // flags, key );
  //
  //	If the left mouse button is pressed over this gadget, then set the focus
  // to 	this gadget. The event flag is cleared so that no button ID
  // number is returned.
  //
  if ((flags & kLeftPress)) {
    flags &= ~kLeftPress;
    Set_Focus();
    Flag_To_Redraw();  // force to draw cursor
  }

  //
  //	Handle keyboard events here. Normally, the key is added to the string,
  // but if the 	RETURN key is pressed, then the button ID number is
  // returned from the Input() 	function.
  //
  if ((flags & kKeyboard) && Has_Focus()) {
    //
    //	Process the keyboard character. If indicated, consume this keyboard
    // event 	so that the edit gadget ID number is not returned.
    //
    if (key == KN_ESC) {
      Clear_Focus();
      flags = 0;

    } else {
      const auto ascii = engine::window::KeyBuffer::ToAscii(key);

      //
      // Filter out all special keys except return and backspace
      //
      if (ascii >= ' ' || key == KN_RETURN || key == KN_BACKSPACE) {
        if (((!(flags & kLeftRelease)) && (!(flags & kRightRelease))) &&
            Handle_Key(ascii)) {
          flags &= ~kKeyboard;
          key = KN_NONE;
        }

      } else {
        if (key == KN_TAB) {
          TheGameState().tab_key_pressed() = true;
        }
        flags &= ~kKeyboard;
        key = KN_NONE;
      }
    }

  } else {
    //	ajw added
    //		if( key == ( KN_ESC | kKeyReleaseBit ) && ( key & kKeyAltBit )
    //)
    //		{
    // Clear_Focus();
    flags = 0;
    key = KN_NONE;
    //		}
  }

  // This reimplements EditClass::Action with WOL-specific key handling rather
  // than extending it, and finishes the same way EditClass does.
  // NOLINTNEXTLINE(bugprone-parent-virtual-call)
  return ControlClass::Action(flags, key, x, y);
}
