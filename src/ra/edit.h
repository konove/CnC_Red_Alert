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

#ifndef CNC_RED_ALERT_RA_EDIT_H_
#define CNC_RED_ALERT_RA_EDIT_H_
#include "ra/control.h"
#include "ra/defines.h"
#include "sdllib/keyboard.h"

// A text editing gadget that accepts keyboard input and displays the result.
// Supports filtering by character type (alpha, numeric, misc) and optional
// uppercase forcing. The gadget does not own its text buffer; the caller
// provides a char array that EditClass modifies in place.
class EditClass : public ControlClass {
 public:
  // Character filter flags for the text input field.
  struct EditStyle {
    bool alpha;      // Accept alphabetic characters.
    bool numeric;    // Accept numbers.
    bool misc;       // Accept misc graphic characters.
    bool uppercase;  // Force to upper case.
  };

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
  EditClass(int id, char* text, int max_len, TextPrintType flags, int x, int y,
            int w = -1, int h = -1, EditStyle style = kAlphanumeric);
  ~EditClass() override;

  void Set_Focus() override;
  int Draw_Me(int forced) override;

  // Changes the text buffer and maximum length. Does not copy; |text| must
  // outlive this gadget.
  virtual void Set_Text(char* text, int max_len);
  virtual char* Get_Text() { return String; }
  void Set_Color(RemapControlType* color) { Color = color; }

  void Set_Read_Only(int rdonly) { IsReadOnly = rdonly; }
  int Get_Max_Length() const { return MaxLength; }

 protected:
  TextPrintType TextFlags;  // Text rendering style (font, alignment).
  EditStyle EditFlags;      // Allowed character types for input filtering.

  char* String;   // Caller-owned text buffer modified in place.
  int MaxLength;  // Max string length (excludes null terminator).
  int Length;     // Current string length, always <= MaxLength.

  RemapControlType* Color;  // Color scheme for rendering.

  // Processes mouse and keyboard events. Sets focus on left-click; inserts
  // characters on keypress. Returns the gadget ID on RETURN, clears focus
  // on ESC.
  int Action(unsigned flags, KeyNumType& key) override;

  // Draws the gadget background. Called with the mouse hidden.
  virtual void Draw_Background();

  // Draws the text content and cursor. Called after Draw_Background with
  // the mouse hidden.
  virtual void Draw_Text(const char* text);

  // Processes a single keyboard character. Returns false if the RETURN key
  // was pressed (allowing the gadget ID to propagate), true otherwise.
  virtual bool Handle_Key(KeyASCIIType ascii);

 private:
  int IsReadOnly;
};

#endif  // CNC_RED_ALERT_RA_EDIT_H_
