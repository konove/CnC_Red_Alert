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

/* $Header: /CounterStrike/MONOC.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : MONO.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : July 2, 1994 *
 *                                                                                             *
 *                  Last Update : July 2, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_MONOC_H_
#define CNC_RED_ALERT_RA_MONOC_H_

#include <cstddef>
#include <string_view>

#include "absl/strings/str_format.h"
#include "absl/types/span.h"
#include "base/array.h"
#include "port/format.h"

class MonoClass {
 public:
  static constexpr int kColumns = 80;       // Number of columns.
  static constexpr int kLines = 25;         // Number of lines.
  static constexpr int kMaxMonoPages = 16;  // Maximum RAM pages on mono card.

  enum class MonoAttribute {
    INVISIBLE = 0x00,  // Black on black.
    UNDERLINE = 0x01,  // Underline.
    BLINKING = 0x90,   // Blinking white on black.
    NORMAL = 0x02,     // White on black.
    INVERSE = 0x70,    // Black on white.
  };
  using enum MonoAttribute;

  /*
  **	These are the various box styles that may be used.
  */
  enum class BoxStyleType {
    SINGLE,       // Single thickness.
    DOUBLE_HORZ,  // Double thick on the horizontal axis.
    DOUBLE_VERT,  // Double thick on the vertical axis.
    DOUBLE,       // Double thickness.

    COUNT
  };
  using enum BoxStyleType;

  MonoClass();
  ~MonoClass();
  MonoClass(MonoClass&&) = delete;
  MonoClass& operator=(MonoClass&&) = delete;

  static void Enable() { Enabled = true; }
  static void Disable() { Enabled = false; }
  static bool Is_Enabled() { return Enabled; }
  static MonoClass* Get_Current() { return PageUsage[0]; }

  void Sub_Window(int x = 0, int y = 0, int w = -1, int h = -1);
  void Fill_Attrib(int x, int y, int w, int h, MonoAttribute attrib);
  void Draw_Box(int x, int y, int w, int h, MonoAttribute attrib = NORMAL,
                BoxStyleType thick = SINGLE);
  void Set_Default_Attribute(MonoAttribute attrib) { Attrib = attrib; }
  void Clear();
  void Set_Cursor(int x, int y);
  void Print(const char* ptr);
  void Print(int text);
  // Prints `format` with `args`, checked at compile time, at the cursor.
  template <typename... Args>
  void Printf(const absl::FormatSpec<Args...>& format, const Args&... args) {
    if (Enabled) {
      Print(absl::StrFormat(format, args...).c_str());
    }
  }
  // Prints string-table entry `text`, formatted with `args` as printf would;
  // a text that is not a format for `args` prints verbatim (see
  // port::FormatRuntime).
  void Printf(int text, absl::Span<const absl::FormatArg> args = {});
  template <typename... Args>
    requires(sizeof...(Args) > 0)
  void Printf(const int text, const Args&... args) {
    const auto packed = port::MakeFormatArgs(args...);
    Printf(text, absl::MakeConstSpan(packed));
  }
  void Text_Print(const char* text, int x, int y,
                  MonoAttribute attrib = NORMAL);
  void Text_Print(int text, int x, int y, MonoAttribute attrib = NORMAL);
  void View();
  void Scroll(int lines = 1);
  void Pan(int cols = 1);
  [[nodiscard]] int Get_X() const { return X; }
  [[nodiscard]] int Get_Y() const { return Y; }
  [[nodiscard]] int Get_Width() const { return SubW; }
  [[nodiscard]] int Get_Height() const { return SubH; }

  /*
  **	Handles deep copies for the mono class objects. This performs what is
  *essentially *	a screen copy.
  */
  MonoClass& operator=(const MonoClass& /*src*/);

  /*
  **	This merely makes a duplicate of the mono object into a newly created
  *mono *	object.
  */
  MonoClass(const MonoClass&);

 private:
  /*
  **	Cursor coordinate (relative to sub-window).
  */
  int X{0};
  int Y{0};

  /*
  **	Default attribute to use when printing text.
  */
  MonoAttribute Attrib{NORMAL};

  /*
  **	The current physical page that this mono class object refers to.
  */
  int Page{0};

  /*
  **	Sub window coordinates.
  */
  int SubX{0};
  int SubY{0};
  int SubW{kColumns};
  int SubH{kLines};

  /*
  **	Pointer to the monochrome RAM.
  */
  //		static MonoPageType * MonoRAM;

  /*
  ** This the the arrays of characters used for drawing boxes.
  */
  /*
  **	This is a private structure that is used to control which characters
  **	are used when a box is drawn. Line drawing on the monochrome screen is
  **	really made up of characters. This specifies which characters to use.
  */
  struct BoxDataType {
    unsigned char UpperLeft;
    unsigned char TopEdge;
    unsigned char UpperRight;
    unsigned char RightEdge;
    unsigned char BottomRight;
    unsigned char BottomEdge;
    unsigned char BottomLeft;
    unsigned char LeftEdge;
  };
  static const BoxDataType CharData[4];

  /*
  **	Each cell is constructed of the actual character that is displayed and
  *the *	attribute to use. This character pair is located at every
  *position on the *	display (80 x 25). Since this cell pair can be
  *represented by a "short" *	integer, certain speed optimizations are taken
  *in the monochrome drawing *	code.
  */
  struct CellType {
    unsigned char Character;  // Character to display.
    unsigned char Attribute;  // Attribute.
  };

  struct MonoPageType {
    CellType Data[kLines][kColumns];
  };

  /*
  **	These private constants are used in the various monochrome operations.
  */
  static constexpr int kControlPort = 0x03B4;  // CRTC control register.
  static constexpr int kDataPort = 0x03B5;     // CRTC data register.
  static constexpr int kSizeOfPage =
      kLines * kColumns *
      static_cast<int>(sizeof(CellType));  // Entire page size.

  /*
  **	This array contains pointers to the monochrome objects that are assigned
  **	to each of the monochrome pages. As the monochrome pages are made
  *visible, *	they can be shuffled around between the actual locations. The
  *first entry *	in this table is the one that is visible.
  */
  static MonoClass* PageUsage[kMaxMonoPages];
  inline static MonoPageType MonoRAM[kMaxMonoPages]{};

  /*
  **	Fetches pointers to the appropriate mono RAM. The DOS build addressed
  * the *	card's memory at 0xB0000; the port has no card, so the pages
  * live in *	ordinary memory and enabling mono output no longer writes to a
  * fixed *	address.
  */
  static MonoPageType* Raw_Ptr(int page) {
    return base::Suffix(MonoRAM, page).data();
  }
  [[nodiscard]] MonoPageType* Page_Ptr() const { return Raw_Ptr(Page); }

  /*
  **	If this is true, then monochrome output is allowed. It defaults to false
  **	so that monochrome output must be explicitly enabled.
  */
  static bool Enabled;
};

extern void Mono_Set_Cursor(int x, int y);
// Prints `format` with `args`, checked at compile time, on the current mono
// page, opening one if none is shown yet. Nothing happens while mono output
// is disabled.
void Mono_Print_Text(std::string_view text);
template <typename... Args>
void Mono_Printf(const absl::FormatSpec<Args...>& format, const Args&... args) {
  if (MonoClass::Is_Enabled()) {
    Mono_Print_Text(absl::StrFormat(format, args...));
  }
}
extern void Mono_Clear_Screen();
extern void Mono_Text_Print(const void* text, int x, int y, int attrib);
extern void Mono_Draw_Rect(int x, int y, int w, int h, int attrib, int thick);
extern void Mono_Print(const void* text);
extern int Mono_X();
extern int Mono_Y();

#endif  // CNC_RED_ALERT_RA_MONOC_H_
