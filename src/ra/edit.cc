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

#include <algorithm>
#include <cctype>
#include <cstring>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "base/numeric.h"
#include "ra/conquer.h"
#include "ra/control.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/gadget.h"
#include "ra/globals.h"
#include "ra/jshell.h"
#include "sdllib/drawbuff.h"
#include "sdllib/font.h"
#include "sdllib/keyboard.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"

namespace {
void PrepareEditBuffer(std::span<char>& buffer, int capacity) {
  if (buffer.empty() || capacity <= 0) {
    throw std::invalid_argument("empty edit buffer");
  }
  buffer = buffer.first(std::min(buffer.size(), static_cast<size_t>(capacity)));
  buffer.back() = '\0';
}
}  // namespace

EditClass::EditClass(const int id, std::span<char> text, const int max_len,
                     const TextPrintType flags, const int x, const int y,
                     const int w, const int h, const EditStyle style)
    : ControlClass(static_cast<unsigned>(id), x, y, w, h, kLeftPress),
      TextFlags(flags & ~TPF_CENTER),
      EditFlags(style),
      String(text),
      MaxLength(std::min(max_len, static_cast<int>(text.size())) - 1),
      Color(Get_Color_Scheme()) {
  PrepareEditBuffer(String, max_len);
  Length = static_cast<int>(std::string_view(String.data()).size());
  GadgetClass::Flag_To_Redraw();

  if (w == -1 || h == -1) {
    Fancy_Text_Print(TXT_NONE, 0, 0, nullptr, kTBlack, TextFlags);

    if (h == -1) {
      Height = FontHeight + 1;
    }
    if (w == -1) {
      if (!std::string_view(String.data()).empty()) {
        Width = String_Pixel_Width(String.data()) + 6;
      } else {
        Width = ((Char_Pixel_Width('X') + FontXSpacing) * (MaxLength + 1)) + 2;
      }
    }
  }

  IsReadOnly = false;
}

EditClass::~EditClass() {
  if (GadgetClass::Has_Focus()) {
    GadgetClass::Clear_Focus();
  }
}

void EditClass::Set_Text(std::span<char> text, const int max_len) {
  String = text;
  PrepareEditBuffer(String, max_len);
  MaxLength = static_cast<int>(String.size()) - 1;
  Length = static_cast<int>(std::string_view(String.data()).size());
  Flag_To_Redraw();
}

bool EditClass::Draw_Me(const bool forced) {
  if (ControlClass::Draw_Me(forced)) {
    if (LogicPage == &SeenBuff) {
      Conditional_Hide_Mouse(X, Y, X + Width, Y + Height);
    }

    Draw_Background();
    Draw_Text(String.data());

    if (LogicPage == &SeenBuff) {
      Conditional_Show_Mouse();
    }

    return true;
  }
  return false;
}

bool EditClass::Action(unsigned flags, KeyNumType& key) {
  if (IsReadOnly) {
    return false;
  }

  // Claim focus on left-click. Clear the press flag so no button ID is
  // returned.
  if (flags & kLeftPress) {
    flags &= ~kLeftPress;
    Set_Focus();
    Flag_To_Redraw();
  }

  if (flags & kKeyboard && Has_Focus()) {
    // ESC clears focus without returning the gadget ID.
    if (key == KN_ESC) {
      Clear_Focus();
      flags = 0;

    } else {
      const auto ascii =
          static_cast<KeyASCIIType>(KeyboardClass::To_ASCII(key) & 0xff);

      // Allow numeric keypad presses to map to ascii numbers.
      if (key & WWKEY_VK_BIT && ascii >= '0' && ascii <= '9') {
        key = static_cast<KeyNumType>(key & ~WWKEY_VK_BIT);
        if ((!(flags & kLeftRelease) && !(flags & kRightRelease)) &&
            Handle_Key(ascii)) {
          flags &= ~kKeyboard;
          key = KN_NONE;
        }

      } else {
        // Filter out all special keys except return and backspace.
        if ((!(key & WWKEY_VK_BIT) && ascii >= ' ' && ascii <= 255) ||
            key == KN_RETURN || key == KN_BACKSPACE) {
          if ((!(flags & kLeftRelease) && !(flags & kRightRelease)) &&
              Handle_Key(KeyboardClass::To_ASCII(key))) {
            flags &= ~kKeyboard;
            key = KN_NONE;
          }

        } else {
          flags &= ~kKeyboard;
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
  const TextPrintType flags =
      Has_Focus() ? TPF_BRIGHT_COLOR : static_cast<TextPrintType>(0);

  Conquer_Clip_Text_Print(text, X + 1, Y + 1, Color, kTBlack, TextFlags | flags,
                          Width - 2);

  if (Has_Focus() && std::cmp_less(std::string_view(text).size(), MaxLength) &&
      String_Pixel_Width(text) + String_Pixel_Width("_") < Width - 2) {
    Conquer_Clip_Text_Print("_", X + 1 + String_Pixel_Width(text), Y + 1, Color,
                            kTBlack, TextFlags | flags);
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
        String[base::ToSize(Length)] = '\0';
        Flag_To_Redraw();
      }
      break;

    case KA_MORE:
    case KA_SETBKGDCOL:
    case KA_SETFORECOL:
    case KA_FORMFEED:
    case KA_SPCTAB:
    case KA_SETX:
    case KA_SETY:
    case KA_SPACE:
    case KA_EXCLAMATION:
    case KA_DQUOTE:
    case KA_POUND:
    case KA_DOLLAR:
    case KA_PERCENT:
    case KA_AMPER:
    case KA_SQUOTE:
    case KA_LPAREN:
    case KA_RPAREN:
    case KA_ASTERISK:
    case KA_PLUS:
    case KA_COMMA:
    case KA_MINUS:
    case KA_PERIOD:
    case KA_SLASH:
    case KA_0:
    case KA_1:
    case KA_2:
    case KA_3:
    case KA_4:
    case KA_5:
    case KA_6:
    case KA_7:
    case KA_8:
    case KA_9:
    case KA_COLON:
    case KA_SEMICOLON:
    case KA_LESS_THAN:
    case KA_EQUAL:
    case KA_GREATER_THAN:
    case KA_QUESTION:
    case KA_AT:
    case KA_A:
    case KA_B:
    case KA_C:
    case KA_D:
    case KA_E:
    case KA_F:
    case KA_G:
    case KA_H:
    case KA_I:
    case KA_J:
    case KA_K:
    case KA_L:
    case KA_M:
    case KA_N:
    case KA_O:
    case KA_P:
    case KA_Q:
    case KA_R:
    case KA_S:
    case KA_T:
    case KA_U:
    case KA_V:
    case KA_W:
    case KA_X:
    case KA_Y:
    case KA_Z:
    case KA_LBRACKET:
    case KA_BACKSLASH:
    case KA_RBRACKET:
    case KA_CARROT:
    case KA_UNDERLINE:
    case KA_GRAVE:
    case KA_a:
    case KA_b:
    case KA_c:
    case KA_d:
    case KA_e:
    case KA_f:
    case KA_g:
    case KA_h:
    case KA_i:
    case KA_j:
    case KA_k:
    case KA_l:
    case KA_m:
    case KA_n:
    case KA_o:
    case KA_p:
    case KA_q:
    case KA_r:
    case KA_s:
    case KA_t:
    case KA_u:
    case KA_v:
    case KA_w:
    case KA_x:
    case KA_y:
    case KA_z:
    case KA_LBRACE:
    case KA_BAR:
    case KA_RBRACE:
    case KA_TILDA:
    case KA_ESC:
    case KA_TAB:
    case KA_SHIFT_BIT:
    case KA_CTRL_BIT:
    case KA_ALT_BIT:
    case KA_RLSE_BIT:
    default:
      if (String_Pixel_Width(String.data()) +
              Char_Pixel_Width(static_cast<char>(ascii)) >=
          Width - 2) {
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

      if (EditFlags.uppercase && isalpha(ascii) != 0) {
        ascii = static_cast<KeyASCIIType>(toupper(ascii));
      }

      // Reject characters not matching any enabled EditStyle category.
      const bool accepted = (EditFlags.numeric && isdigit(ascii) != 0) ||
                            (EditFlags.alpha && isalpha(ascii) != 0) ||
                            (EditFlags.misc && isalnum(ascii) == 0) ||
                            ascii == ' ';
      if (!accepted) {
        break;
      }

      // Manual redraw needed because the event flag was cleared to prevent
      // the gadget ID from being returned on every keystroke.
      String[base::ToSize(Length++)] = static_cast<char>(ascii);
      String[base::ToSize(Length)] = '\0';
      Flag_To_Redraw();
      break;
  }
  return true;
}

void EditClass::Set_Focus() {
  Length = 0;
  if (!String.empty()) {
    Length = static_cast<int>(std::string_view(String.data()).size());
  }
  ControlClass::Set_Focus();
}
