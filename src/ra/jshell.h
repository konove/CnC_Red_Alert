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

/* $Header: /CounterStrike/JSHELL.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : JSHELL.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 03/13/95 *
 *                                                                                             *
 *                  Last Update : March 13, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_JSHELL_H_
#define CNC_RED_ALERT_RA_JSHELL_H_

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "absl/base/attributes.h"
#include "absl/log/check.h"
#include "absl/strings/str_format.h"
#include "absl/types/span.h"
#include "base/array.h"
#include "base/numeric.h"
#include "base/types.h"
#include "port/ex_string.h"
#include "port/format.h"
#include "ra/compat.h"
#include "ra/globals.h"
#include "ra/palette.h"
#include "sdllib/buffer.h"
#include "sdllib/iff.h"
#include "sdllib/keyboard.h"
#include "sdllib/timer.h"
#include "sdllib/ww_mouse.h"
#include "tech/file.h"

/*
**	Interface class to the keyboard. This insulates the game from library
*vagaries. Most *	notable being the return values are declared as "int" in
*the library whereas C&C *	expects it to be of KeyNumType.
*/
struct KeyboardClass : public WWKeyboardClass {
  /*
  **	This flag is used to indicate whether the WW library has taken over
  **	the keyboard or not. If not, then the normal console input
  **	takes precedence.
  */
  bool IsLibrary{true};

  KeyboardClass() = default;
  // These deliberately hide the library's int-returning versions; narrowing to
  // the game's key enums is the only reason this interface class exists.
  // NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method)
  KeyNumType Get() { return static_cast<KeyNumType>(WWKeyboardClass::Get()); }
  // NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method)
  KeyNumType Check() {
    return static_cast<KeyNumType>(WWKeyboardClass::Check());
  }
  static KeyASCIIType To_ASCII(KeyNumType key) {
    return static_cast<KeyASCIIType>(WWKeyboardClass::To_ASCII(key));
  }
  static bool Down(KeyNumType key) { return WWKeyboardClass::Down(key); }

  static int Mouse_X() { return Get_Mouse_X(); }
  static int Mouse_Y() { return Get_Mouse_Y(); }
};

/*
**	These templates allow enumeration types to have simple bitwise
**	arithmatic performed. The operators must be instatiated for the
**	enumerated types desired.
*/
template <class T>
T operator++(T& a) {
  a = static_cast<T>(static_cast<int>(a) + 1);
  return a;
}
template <class T>
T operator++(T& a, int) {
  T aa = a;
  a = static_cast<T>(static_cast<int>(a) + 1);
  return aa;
}
template <class T>
T operator--(T& a) {
  a = static_cast<T>(static_cast<int>(a) - 1);
  return a;
}
template <class T>
T operator--(T& a, int) {
  T aa = a;
  a = static_cast<T>(static_cast<int>(a) - 1);
  return aa;
}
template <class T>
constexpr T operator|(T t1, T t2) noexcept {
  return static_cast<T>(static_cast<uint32_t>(t1) | static_cast<uint32_t>(t2));
}
template <class T>
T operator&(T t1, T t2) {
  return static_cast<T>(static_cast<uint32_t>(t1) & static_cast<uint32_t>(t2));
}
template <class T>
T operator~(T t1) {
  return static_cast<T>(~static_cast<uint32_t>(t1));
}

// TODO(konove): Replace with std::clamp
template <class T>
T Bound(T original, T minval, T maxval) {
  if (original < minval) {
    return minval;
  }
  if (original > maxval) {
    return maxval;
  }
  return original;
}

// Bit indices must refer to the supplied word span.
inline void Set_Bit(std::span<uint32_t> array, int bit, int value) {
  CHECK_GE(bit, 0);
  CHECK_LT(base::ToSize(bit / 32), array.size());
  if (value) {
    base::At(array, base::ToSize(bit / 32)) |= base::Bit<uint32_t>(bit % 32);
  } else {
    base::At(array, base::ToSize(bit / 32)) &= ~base::Bit<uint32_t>(bit % 32);
  }
}

inline bool Get_Bit(std::span<const uint32_t> array, int bit) {
  CHECK_GE(bit, 0);
  CHECK_LT(base::ToSize(bit / 32), array.size());
  return (base::At(array, base::ToSize(bit / 32)) &
          base::Bit<uint32_t>(bit % 32)) != 0;
}

// Returns -1 if no matching bit exists within the supplied words.
inline int First_True_Bit(std::span<const uint32_t> array) {
  int off = 0;
  for (const uint32_t word : array) {
    const int pos = std::countr_zero(word);
    if (pos < 32) {
      return off + pos;
    }
    off += 32;
  }
  return -1;
}

inline int First_False_Bit(std::span<const uint32_t> array) {
  int off = 0;
  for (const uint32_t word : array) {
    const int pos = std::countr_zero(~word);
    if (pos < 32) {
      return off + pos;
    }
    off += 32;
  }
  return -1;
}

// Tick sources for Ticker<T>. Each provides a Tick() function that
// returns the current value of a specific time source.
struct FrameTickSource {
  static int64_t Tick() { return Frame; }
};

struct SystemTickSource {
  static int64_t Tick() {
    return g_tick_timer == nullptr ? 0 : g_tick_timer->TickCount();
  }
};

template <class T>
void Bubble_Sort(std::span<T> array, int count) {
  if (count > 1) {
    bool swapflag = false;

    do {
      swapflag = false;
      for (int index = 0; index < count - 1; index++) {
        if (array[index] > array[index + 1]) {
          const auto temp = array[index];
          array[index] = array[index + 1];
          array[index + 1] = temp;
          swapflag = true;
        }
      }
    } while (swapflag);
  }
}

template <class T>
void PBubble_Sort(T& array, int count) {
  if (count > 1) {
    bool swapflag = false;

    do {
      swapflag = false;
      for (int index = 0; index < count - 1; index++) {
        if (*array.at(index) > *array.at(index + 1)) {
          const auto temp = array.at(index);
          array.at(index) = array.at(index + 1);
          array.at(index + 1) = temp;
          swapflag = true;
        }
      }
    } while (swapflag);
  }
}

template <class T>
void PNBubble_Sort(T& array, int count) {
  if (count > 1) {
    bool swapflag = false;

    do {
      swapflag = false;
      for (int index = 0; index < count - 1; index++) {
        if (port::CompareIgnoreCase(array.at(index)->Name(),
                                    array.at(index + 1)->Name()) > 0) {
          const auto temp = array.at(index);
          array.at(index) = array.at(index + 1);
          array.at(index + 1) = temp;
          swapflag = true;
        }
      }
    } while (swapflag);
  }
}

struct TLucentType {
  unsigned char SourceColor;
  unsigned char DestColor;
  unsigned char Fading;
  unsigned char reserved;
};

int Load_Picture(const char* filename, BufferClass& scratchbuf,
                 BufferClass& destbuf, std::span<unsigned char> palette,
                 PicturePlaneType format);
std::span<unsigned char> Conquer_Build_Fading_Table(
    const PaletteClass& palette,
    std::span<unsigned char> dest ABSL_ATTRIBUTE_LIFETIME_BOUND, int color,
    int frac);
std::span<const unsigned char> Small_Icon(std::span<const std::byte> iconptr,
                                          int iconnum);
void Set_Window(int window, int x, int y, int w, int h);
int32_t Load_Uncompress(File& file, BuffType& uncomp_buff, BuffType& dest_buff,
                        std::span<unsigned char> reserved_data);
int32_t Translucent_Table_Size(int count);
std::span<unsigned char> Build_Translucent_Table(
    const PaletteClass& palette, std::span<const TLucentType> control,
    int count, std::span<unsigned char> buffer ABSL_ATTRIBUTE_LIFETIME_BOUND);
std::span<unsigned char> Conquer_Build_Translucent_Table(
    const PaletteClass& palette, std::span<const TLucentType> control,
    int count, std::span<unsigned char> buffer ABSL_ATTRIBUTE_LIFETIME_BOUND);
std::span<unsigned char> Make_Fading_Table(const PaletteClass& palette,
                                           std::span<unsigned char> dest
                                               ABSL_ATTRIBUTE_LIFETIME_BOUND,
                                           int color, int frac);

// Prints `format` with `args`, checked at compile time, to stderr and the
// mono page, then exits with a failure code.
[[noreturn]] void Fatal_Message(std::string_view message);
template <typename... Args>
[[noreturn]] void Fatal(const absl::FormatSpec<Args...>& format,
                        const Args&... args) {
  Fatal_Message(absl::StrFormat(format, args...));
}

// Formats `format`, known only at run time (the string table or a format
// handed in by a caller), with `args` into `buffer`, which holds `size` bytes.
// The result is always null terminated and truncated rather than allowed to
// overflow. Each conversion is checked against its argument; a format that
// does not match `args` is copied unformatted (see port::FormatRuntime).
void Format_Runtime_Text(std::span<char> buffer, size_t size,
                         const char* format,
                         absl::Span<const absl::FormatArg> args = {});
template <typename... Args>
  requires(sizeof...(Args) > 0)
void Format_Runtime_Text(std::span<char> buffer, const size_t size,
                         const char* format, const Args&... args) {
  const auto packed = port::MakeFormatArgs(args...);
  Format_Runtime_Text(buffer, size, format, absl::MakeConstSpan(packed));
}

#endif  // CNC_RED_ALERT_RA_JSHELL_H_
