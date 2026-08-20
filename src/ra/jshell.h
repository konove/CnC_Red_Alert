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

#include <cassert>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "absl/base/attributes.h"

#include "port/ex_string.h"
#include "ra/compat.h"
#include "ra/globals.h"
#include "ra/palette.h"
#include "sdllib/buffer.h"
#include "sdllib/iff.h"
#include "sdllib/keyboard.h"
#include "sdllib/timer.h"
#include "sdllib/ww_mouse.h"
#include "tech/noinit.h"
#include "tech/wwfile.h"

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
  unsigned IsLibrary;

  KeyboardClass() : IsLibrary(true) {}
  KeyNumType Get() { return (KeyNumType)WWKeyboardClass::Get(); }
  KeyNumType Check() { return (KeyNumType)WWKeyboardClass::Check(); }
  KeyASCIIType To_ASCII(KeyNumType key) {
    return (KeyASCIIType)WWKeyboardClass::To_ASCII(key);
  }
  void Clear() { WWKeyboardClass::Clear(); }
  int Down(KeyNumType key) { return WWKeyboardClass::Down(key); }

  int Mouse_X() { return Get_Mouse_X(); }
  int Mouse_Y() { return Get_Mouse_Y(); }
};

/*
**	These templates allow enumeration types to have simple bitwise
**	arithmatic performed. The operators must be instatiated for the
**	enumerated types desired.
*/
template <class T>
T operator++(T& a) {
  a = (T)((int)a + 1);
  return a;
}
template <class T>
T operator++(T& a, int) {
  T aa = a;
  a = (T)((int)a + 1);
  return aa;
}
template <class T>
T operator--(T& a) {
  a = (T)((int)a - 1);
  return a;
}
template <class T>
T operator--(T& a, int) {
  T aa = a;
  a = (T)((int)a - 1);
  return aa;
}
template <class T>
constexpr T operator|(T t1, T t2) {
  return (T)((int)t1 | (int)t2);
}
template <class T>
T operator&(T t1, T t2) {
  return (T)((int)t1 & (int)t2);
}
template <class T>
T operator~(T t1) {
  return (T) ~(int)t1;
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

inline void Set_Bit(void* array, int bit, int value) {
  if (value) {
    ((uint32_t*)array)[(unsigned)bit >> 5] |= 1 << (bit & 0x1F);
  } else {
    ((uint32_t*)array)[(unsigned)bit >> 5] &= ~(1 << (bit & 0x1F));
  }
}

inline int Get_Bit(const void* array, int bit) {
  return !!(((const uint32_t*)array)[(unsigned)bit >> 5] & 1 << (bit & 0x1F));
}

inline int First_True_Bit(const void* array) {
  const uint32_t* array32 = (const uint32_t*)array;
  int off = 0;
  while (true) {
    uint32_t v = *array32++;
#ifdef _MSC_VER
    DWORD pos;
    if (_BitScanForward(&pos, v)) {
      return off + pos;
    }
#else
    int pos = __builtin_ffs(v);
    if (pos) {
      return off + pos - 1;
    }
#endif
    off += 32;
  }
}
inline int First_False_Bit(const void* array) {
  const uint32_t* array32 = (const uint32_t*)array;
  int off = 0;
  while (true) {
    uint32_t v = *array32++;
#ifdef _MSC_VER
    DWORD pos;
    if (_BitScanForward(&pos, ~v)) {
      return off + pos;
    }
#else
    int pos = __builtin_ffs(~v);
    if (pos) {
      return off + pos - 1;
    }
#endif
    off += 32;
  }
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
void Bubble_Sort(T* array, int count) {
  if (array != nullptr && count > 1) {
    bool swapflag;

    do {
      swapflag = false;
      for (int index = 0; index < count - 1; index++) {
        if (array[index] > array[index + 1]) {
          T temp = array[index];
          array[index] = array[index + 1];
          array[index + 1] = temp;
          swapflag = true;
        }
      }
    } while (swapflag);
  }
}

template <class T>
void PBubble_Sort(T* array, int count) {
  if (array != nullptr && count > 1) {
    bool swapflag;

    do {
      swapflag = false;
      for (int index = 0; index < count - 1; index++) {
        if (*array[index] > *array[index + 1]) {
          T temp = array[index];
          array[index] = array[index + 1];
          array[index + 1] = temp;
          swapflag = true;
        }
      }
    } while (swapflag);
  }
}

template <class T>
void PNBubble_Sort(T* array, int count) {
  if (array != nullptr && count > 1) {
    bool swapflag;

    do {
      swapflag = false;
      for (int index = 0; index < count - 1; index++) {
        if (stricmp(array[index]->Name(), array[index + 1]->Name()) > 0) {
          T temp = array[index];
          array[index] = array[index + 1];
          array[index + 1] = temp;
          swapflag = true;
        }
      }
    } while (swapflag);
  }
}

typedef struct {
  unsigned char SourceColor;
  unsigned char DestColor;
  unsigned char Fading;
  unsigned char reserved;
} TLucentType;

int Load_Picture(const char* filename, BufferClass& scratchbuf,
                 BufferClass& destbuf, unsigned char* palette,
                 PicturePlaneType format);
void* Conquer_Build_Fading_Table(const PaletteClass& palette, void* dest,
                                 int color, int frac);
void* Small_Icon(const void* iconptr, int iconnum);
void Set_Window(int window, int x, int y, int w, int h);
void* Load_Alloc_Data(FileClass& file);
std::vector<std::byte> LoadAllocData(FileClass& file);
long Load_Uncompress(FileClass& file, BuffType& uncomp_buff,
                     BuffType& dest_buff, void* reserved_data);
long Translucent_Table_Size(int count);
void* Build_Translucent_Table(const PaletteClass& palette,
                              const TLucentType* control, int count,
                              void* buffer);
void* Conquer_Build_Translucent_Table(const PaletteClass& palette,
                                      const TLucentType* control, int count,
                                      void* buffer);
void* Make_Fading_Table(const PaletteClass& palette, void* dest, int color,
                        int frac);

// Prints a printf-style message to stderr and exits with a failure code. The
// format attribute both type-checks every call site and tells the compiler the
// forwarded format string inside Fatal() is intentionally non-literal.
void Fatal(const char* message, ...) ABSL_PRINTF_ATTRIBUTE(1, 2);

// Formats "format" and its arguments into "buffer", which holds "size" bytes.
// The result is always null terminated and is truncated rather than allowed to
// overflow.
//
// Use this whenever the format string is only known at runtime -- the
// localized string table, or a format handed in by a caller. The compiler
// cannot check such a format against its arguments, and the suppression of
// that diagnostic is centralized here instead of being repeated at every call
// site.
void Format_Runtime_Text(char* buffer, size_t size, const char* format, ...);
void Format_Runtime_Text(char* buffer, size_t size, const char* format,
                         va_list args);

#endif  // CNC_RED_ALERT_RA_JSHELL_H_
