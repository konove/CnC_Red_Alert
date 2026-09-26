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

#include "base/array.h"
#include "base/numeric.h"
#include "engine/gfx/font.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/wwstd.h"
#include "engine/window/keyboard.h"
#include "engine/window/ww_mouse.h"
#include "ra/control.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/gadget.h"
#include "ra/jshell.h"
#include "ra/screen.h"

using enum engine::window::KeyNumber;

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
    const FontStyle font = TextFontStyle(TextFlags);

    if (h == -1) {
      Height = FontMaxHeight(font) + 1;
    }
    if (w == -1) {
      if (!std::string_view(String.data()).empty()) {
        Width = StringPixelWidth(font, String.data()) + 6;
      } else {
        // CharPixelWidth() already includes the spacing, so it is counted
        // twice here, as it always was.
        Width =
            ((CharPixelWidth(font, 'X') + font.x_spacing) * (MaxLength + 1)) +
            2;
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

bool EditClass::Draw_Me(PixelView& view, const bool forced) {
  if (ControlClass::Draw_Me(view, forced)) {
    if (TheScreen().IsVisible(&view)) {
      Conditional_Hide_Mouse(X, Y, X + Width, Y + Height);
    }

    Draw_Background(view);
    Draw_Text(view, String.data());

    if (TheScreen().IsVisible(&view)) {
      Conditional_Show_Mouse();
    }

    return true;
  }
  return false;
}

bool EditClass::Action(unsigned flags, engine::window::KeyNumber& key) {
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
      const auto ascii = engine::window::KeyBuffer::ToAscii(key);

      // Filter out all special keys except return and backspace.
      if (ascii >= ' ' || key == KN_RETURN || key == KN_BACKSPACE) {
        if ((!(flags & kLeftRelease) && !(flags & kRightRelease)) &&
            Handle_Key(ascii)) {
          flags &= ~kKeyboard;
          key = KN_NONE;
        }

      } else {
        flags &= ~kKeyboard;
        key = KN_NONE;
      }
    }
  }

  return ControlClass::Action(flags, key);
}

void EditClass::Draw_Background(PixelView& view) {
  Draw_Box(view, X, Y, Width, Height, BOXSTYLE_BOX, true);
}

void EditClass::Draw_Text(PixelView& view, const char* text) {
  const TextPrintType flags =
      Has_Focus() ? TPF_BRIGHT_COLOR : static_cast<TextPrintType>(0);

  Conquer_Clip_Text_Print(view, text, X + 1, Y + 1, Color, kTBlack,
                          TextFlags | flags, Width - 2);

  const FontStyle font = TextFontStyle(TextFlags | flags);
  if (Has_Focus() && std::cmp_less(std::string_view(text).size(), MaxLength) &&
      StringPixelWidth(font, text) + StringPixelWidth(font, "_") < Width - 2) {
    Conquer_Clip_Text_Print(view, "_", X + 1 + StringPixelWidth(font, text),
                            Y + 1, Color, kTBlack, TextFlags | flags);
  }
}

bool EditClass::Handle_Key(char ascii) {
  switch (ascii) {
    // A zero key code can arrive if a subclass consumed the event.
    case 0:
      break;

    // Return false so the gadget ID propagates to the caller.
    case '\r':
      Clear_Focus();
      return false;

    case '\b':
      if (Length) {
        Length--;
        base::At(String, base::ToSize(Length)) = '\0';
        Flag_To_Redraw();
      }
      break;

    default:
      if (const FontStyle font = TextFontStyle(TextFlags);
          StringPixelWidth(font, String.data()) + CharPixelWidth(font, ascii) >=
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
        ascii = static_cast<char>(toupper(ascii));
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
      base::At(String, base::ToSize(Length++)) = ascii;
      base::At(String, base::ToSize(Length)) = '\0';
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
