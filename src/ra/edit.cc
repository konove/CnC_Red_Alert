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

// Implementation of EditClass, the single-line text input UI gadget.

#include "ra/edit.h"

#include <cctype>
#include <cstring>

#include "ra/conquer.h"
#include "ra/control.h"
#include "ra/dialog.h"
#include "ra/externs.h"
#include "ra/globals.h"
#include "ra/jshell.h"
#include "sdllib/drawbuff.h"
#include "sdllib/font.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"

EditClass::EditClass(const int id, char* text, const int max_len,
                     const TextPrintType flags, const int x, const int y,
                     const int w, const int h, const EditStyle style)
    : ControlClass(id, x, y, w, h, LEFTPRESS), String(text) {
  TextFlags = flags & ~TPF_CENTER;
  EditFlags = style;
  String = text;
  MaxLength = max_len - 1;
  Length = strlen(String);
  GadgetClass::Flag_To_Redraw();
  Color = Get_Color_Scheme();

  if (w == -1 || h == -1) {
    Fancy_Text_Print(TXT_NONE, 0, 0, nullptr, TBLACK, TextFlags);

    if (h == -1) {
      Height = FontHeight + 1;
    }
    if (w == -1) {
      if (strlen(String) > 0) {
        Width = String_Pixel_Width(String) + 6;
      } else {
        Width = (Char_Pixel_Width('X') + FontXSpacing) * (MaxLength + 1) + 2;
      }
    }
  }

  IsReadOnly = 0;
}

EditClass::~EditClass() {
  if (GadgetClass::Has_Focus()) {
    GadgetClass::Clear_Focus();
  }
}

void EditClass::Set_Text(char* text, const int max_len) {
  String = text;
  MaxLength = max_len - 1;
  Length = strlen(String);
  Flag_To_Redraw();
}

int EditClass::Draw_Me(const bool forced) {
  if (ControlClass::Draw_Me(forced)) {
    if (LogicPage == &SeenBuff) {
      Conditional_Hide_Mouse(X, Y, X + Width, Y + Height);
    }

    Draw_Background();
    Draw_Text(String);

    if (LogicPage == &SeenBuff) {
      Conditional_Show_Mouse();
    }

    return true;
  }
  return false;
}

int EditClass::Action(unsigned flags, KeyNumType& key) {
  if (IsReadOnly) {
    return false;
  }

  // Claim focus on left-click. Clear the press flag so no button ID is
  // returned.
  if (flags & LEFTPRESS) {
    flags &= ~LEFTPRESS;
    Set_Focus();
    Flag_To_Redraw();
  }

  if (flags & KEYBOARD && Has_Focus()) {
    // ESC clears focus without returning the gadget ID.
    if (key == KN_ESC) {
      Clear_Focus();
      flags = 0;

    } else {
      const auto ascii =
          static_cast<KeyASCIIType>(Keyboard->To_ASCII(key) & 0xff);

      // Allow numeric keypad presses to map to ascii numbers.
      if (key & WWKEY_VK_BIT && ascii >= '0' && ascii <= '9') {
        key = static_cast<KeyNumType>(key & ~WWKEY_VK_BIT);
        if (!(flags & LEFTRELEASE) && !(flags & RIGHTRELEASE)) {
          if (Handle_Key(ascii)) {
            flags &= ~KEYBOARD;
            key = KN_NONE;
          }
        }
      } else {
        // Filter out all special keys except return and backspace.
        if ((!(key & WWKEY_VK_BIT) && ascii >= ' ' && ascii <= 255) ||
            key == KN_RETURN || key == KN_BACKSPACE) {
          if (!(flags & LEFTRELEASE) && !(flags & RIGHTRELEASE)) {
            if (Handle_Key(Keyboard->To_ASCII(key))) {
              flags &= ~KEYBOARD;
              key = KN_NONE;
            }
          }
        } else {
          flags &= ~KEYBOARD;
          key = KN_NONE;
        }
      }
    }
  }

  return ControlClass::Action(flags, key);
}

void EditClass::Draw_Background() {
  Draw_Box(X, Y, Width, Height, BOXSTYLE_BOX, true);
}

void EditClass::Draw_Text(const char* text) {
  TextPrintType flags;

  if (Has_Focus()) {
    flags = TPF_BRIGHT_COLOR;
  } else {
    flags = static_cast<TextPrintType>(0);
  }

  Conquer_Clip_Text_Print(text, X + 1, Y + 1, Color, TBLACK, TextFlags | flags,
                          Width - 2);

  if (Has_Focus() && static_cast<int>(strlen(text)) < MaxLength &&
      static_cast<int>(String_Pixel_Width(text) + String_Pixel_Width("_")) <
          Width - 2) {
    Conquer_Clip_Text_Print("_", X + 1 + String_Pixel_Width(text), Y + 1, Color,
                            TBLACK, TextFlags | flags);
  }
}

bool EditClass::Handle_Key(KeyASCIIType ascii) {
  switch (ascii) {
    // A zero key code can arrive if a subclass consumed the event.
    case 0:
      break;

    // Return false so the gadget ID propagates to the caller.
    case KA_RETURN:
      Clear_Focus();
      return false;

    case KA_BACKSPACE:
      if (Length) {
        Length--;
        String[Length] = '\0';
        Flag_To_Redraw();
      }
      break;

    default:
      if (static_cast<int>(String_Pixel_Width(String) +
                       Char_Pixel_Width(ascii)) >= Width - 2) {
        break;
      }
      if (Length >= MaxLength) {
        break;
      }

      // Reject non-printable characters and leading spaces.
      if (!isgraph(ascii) && ascii != ' ') {
        break;
      }
      if (ascii == ' ' && Length == 0) {
        break;
      }

      if (EditFlags.uppercase && isalpha(ascii)) {
        ascii = static_cast<KeyASCIIType>(toupper(ascii));
      }

      // Reject characters not matching any enabled EditStyle category.
      const bool accepted = (EditFlags.numeric && isdigit(ascii)) ||
                            (EditFlags.alpha && isalpha(ascii)) ||
                            (EditFlags.misc && !isalnum(ascii)) || ascii == ' ';
      if (!accepted) {
        break;
      }

      // Manual redraw needed because the event flag was cleared to prevent
      // the gadget ID from being returned on every keystroke.
      String[Length++] = ascii;
      String[Length] = '\0';
      Flag_To_Redraw();
      break;
  }
  return true;
}

void EditClass::Set_Focus() {
  Length = 0;
  if (String) {
    Length = strlen(String);
  }
  ControlClass::Set_Focus();
}
