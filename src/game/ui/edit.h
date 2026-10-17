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

// Single-line text input UI gadget for the game's dialog system.

#ifndef CNC_RED_ALERT_GAME_UI_EDIT_H_
#define CNC_RED_ALERT_GAME_UI_EDIT_H_

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <string_view>

#include "absl/base/attributes.h"
#include "base/array.h"
#include "base/numeric.h"
#include "engine/gfx/font.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/keyboard.h"
#include "game/ui/control.h"
#include "game/ui/input.h"
#include "game/ui/widget_skin.h"

namespace game {

// Character filter flags for EditClass's text input field.
struct EditStyle {
  bool alpha;      // Accept alphabetic characters.
  bool numeric;    // Accept numbers.
  bool misc;       // Accept misc graphic characters.
  bool uppercase;  // Force to upper case.
};

// A text editing gadget that accepts keyboard input and displays the result.
// Supports filtering by character type (alpha, numeric, misc) and optional
// uppercase forcing. The gadget does not own its text buffer; the caller
// provides a char array that EditClass modifies in place.
// NOLINTBEGIN(portability-template-virtual-member-function): see WidgetSkin.
template <WidgetSkin Skin>
class EditClass : public ControlClass {
 public:
  using EditStyle = game::EditStyle;

  static constexpr EditStyle kAlphanumeric{
      .alpha = true,
      .numeric = true,
      .misc = true,
      .uppercase = false,
  };
  static constexpr EditStyle kNumeric{
      .alpha = false,
      .numeric = true,
      .misc = false,
      .uppercase = false,
  };
  static constexpr EditStyle kAlpha{
      .alpha = true,
      .numeric = false,
      .misc = false,
      .uppercase = false,
  };

  // Constructs an edit gadget. |text| is a caller-owned buffer that will be
  // modified in place. |max_len| is the buffer size including the null
  // terminator. |w| and |h| default to -1, meaning auto-sized from the text.
  // Throws std::invalid_argument if the buffer is empty.
  EditClass(int id, std::span<char> text ABSL_ATTRIBUTE_LIFETIME_BOUND,
            int max_len, Skin::Font font, Skin::PrintFlags flags, int x, int y,
            int w = -1, int h = -1, EditStyle style = kAlphanumeric)
      : ControlClass(static_cast<unsigned>(id), x, y, w, h, kLeftPress),
        TextFont(font),
        TextFlags(Skin::Edit_Flags(flags)),
        EditFlags(style),
        String(text),
        MaxLength(std::min(max_len, static_cast<int>(text.size())) - 1),
        Color(Skin::Edit_Default_Color()) {
    PrepareEditBuffer(String, max_len);
    Length = static_cast<int>(std::string_view(String.data()).size());
    Gadget::FlagToRedraw();

    if (w == -1 || h == -1) {
      const FontStyle font_style = Skin::FontStyleFor(TextFont, TextFlags);
      if (h == -1) {
        set_height(FontMaxHeight(font_style) + Skin::kEditHeightPad);
      }
      if (w == -1) {
        if (!std::string_view(String.data()).empty()) {
          set_width(StringPixelWidth(font_style, String.data()) + 6);
        } else {
          // CharPixelWidth() already includes the spacing, so it is counted
          // twice here, as it always was.
          set_width(((CharPixelWidth(font_style, 'X') + font_style.x_spacing) *
                     (MaxLength + 1)) +
                    2);
        }
      }
    }
  }
  ~EditClass() override {
    if (Gadget::HasFocus()) {
      Gadget::ClearFocus();
    }
  }
  EditClass(const EditClass&) = delete;
  EditClass& operator=(const EditClass&) = delete;
  EditClass(EditClass&&) = delete;
  EditClass& operator=(EditClass&&) = delete;

  void SetFocus() override {
    Length = 0;
    if (!String.empty()) {
      Length = static_cast<int>(std::string_view(String.data()).size());
    }
    ControlClass::SetFocus();
  }

  bool Draw(PixelView& view, bool forced) override {
    if (ControlClass::Draw(view, forced)) {
      Draw_Background(view);
      Draw_Text(view, String.data());
      return true;
    }
    return false;
  }

  // Changes the text buffer and maximum length. Does not copy; |text| must
  // outlive this gadget.
  virtual void Set_Text(std::span<char> text, int max_len) {
    String = text;
    PrepareEditBuffer(String, max_len);
    MaxLength = static_cast<int>(String.size()) - 1;
    Length = static_cast<int>(std::string_view(String.data()).size());
    FlagToRedraw();
  }
  std::span<char> Get_Text_Buffer() { return String; }
  virtual char* Get_Text() { return String.data(); }
  void Set_Color(Skin::TextColor color) { Color = color; }

  void Set_Read_Only(bool rdonly) { IsReadOnly = rdonly; }
  [[nodiscard]] int Get_Max_Length() const { return MaxLength; }

 protected:
  Skin::Font TextFont;         // The font the text renders in.
  Skin::PrintFlags TextFlags;  // Text rendering style (alignment, colour).
  EditStyle EditFlags{};       // Allowed character types for input filtering.

  std::span<char> String;  // Caller-owned text buffer modified in place.
  int MaxLength;           // Max string length (excludes null terminator).
  int Length{};            // Current string length, always <= MaxLength.

  Skin::TextColor Color;  // Colour the text is drawn in.

  // Processes mouse and keyboard events. Sets focus on left-click; inserts
  // characters on keypress. Returns the gadget ID on RETURN, clears focus
  // on ESC.
  bool Action(unsigned flags, engine::window::KeyNumber& key, int x,
              int y) override {
    using engine::window::KN_BACKSPACE;
    using engine::window::KN_ESC;
    using engine::window::KN_NONE;
    using engine::window::KN_RETURN;
    if (IsReadOnly) {
      return false;
    }

    // Claim focus on left-click. Clear the press flag so no button ID is
    // returned.
    if (flags & kLeftPress) {
      flags &= ~kLeftPress;
      SetFocus();
      FlagToRedraw();
    }

    if (flags & kKeyboard && HasFocus()) {
      // ESC clears focus without returning the gadget ID.
      if (key == KN_ESC) {
        ClearFocus();
        flags = 0;
      } else {
        const auto ascii = TheKeyboard().TypedCharacter(key);

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

    return ControlClass::Action(flags, key, x, y);
  }

  // Draws the gadget background. Called with the mouse hidden.
  virtual void Draw_Background(PixelView& view) {
    Skin::Draw_Edit_Box(view, x(), y(), width(), height());
  }

  // Draws the text content and cursor. Called after Draw_Background with
  // the mouse hidden.
  virtual void Draw_Text(PixelView& view, const char* text) {
    Skin::Draw_Edit_Text(view, text, x(), y(), width(), Color, TextFont,
                         TextFlags, HasFocus(), MaxLength);
  }

  // Processes a single keyboard character. Returns false if the RETURN key
  // was pressed (allowing the gadget ID to propagate), true otherwise.
  virtual bool Handle_Key(char ascii) {
    switch (ascii) {
      // A zero key code can arrive if a subclass consumed the event.
      case 0:
        break;

      // Return false so the gadget ID propagates to the caller.
      case '\r':
        ClearFocus();
        return false;

      case '\b':
        if (Length) {
          Length--;
          base::At(String, base::ToSize(Length)) = '\0';
          FlagToRedraw();
        }
        break;

      default: {
        const FontStyle font = Skin::FontStyleFor(TextFont, TextFlags);
        if (StringPixelWidth(font, String.data()) +
                CharPixelWidth(font, ascii) >=
            width() - 2) {
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
        FlagToRedraw();
        break;
      }
    }
    return true;
  }

 private:
  // Narrows `buffer` to `capacity` bytes and terminates it.
  static void PrepareEditBuffer(std::span<char>& buffer, int capacity) {
    if (buffer.empty() || capacity <= 0) {
      throw std::invalid_argument("empty edit buffer");
    }
    buffer =
        buffer.first(std::min(buffer.size(), static_cast<size_t>(capacity)));
    buffer.back() = '\0';
  }

  bool IsReadOnly = false;
};
// NOLINTEND(portability-template-virtual-member-function)

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_UI_EDIT_H_
