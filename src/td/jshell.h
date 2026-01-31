/*
**	Command & Conquer(tm)
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

/* $Header:   F:\projects\c&c\vcs\code\jshell.h_v   2.16   16 Oct 1995 16:45:06
 * JOE_BOSTIC  $ */
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

#ifndef JSHELL_H
#define JSHELL_H

#include <algorithm>
#include <cstdint>

#include "sdllib/include/buffer.h"
#include "sdllib/include/iff.h"
#include "sdllib/include/keyboard.h"
#include "sdllib/include/ww_mouse.h"
#include "td/compat.h"
#include "tech/wwfile.h"

/*
**	Interface class to the keyboard. This insulates the game from library
*vagaries. Most *	notable being the return values are declared as "int" in
*the library whereas C&C *	expects it to be of KeyNumType.
*/
class Keyboard {
 public:
  static KeyNumType Get() { return static_cast<KeyNumType>(Get_Key_Num()); }
  static KeyNumType Check() { return static_cast<KeyNumType>(Check_Key_Num()); }
  static KeyASCIIType To_ASCII(KeyNumType key) {
    return static_cast<KeyASCIIType>(KN_To_KA(key));
  }
  static void Clear() { Clear_KeyBuffer(); }
  static void Stuff(KeyNumType key) { Stuff_Key_Num(key); }
  static int Down(KeyNumType key) { return Key_Down(key); }
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
  a = static_cast<T>((int)a + 1);
  return a;
}
template <class T>
T operator++(T& a, int) {
  T aa = a;
  a = static_cast<T>((int)a + 1);
  return aa;
}
template <class T>
T operator--(T& a) {
  a = static_cast<T>((int)a - 1);
  return a;
}
template <class T>
T operator--(T& a, int) {
  T aa = a;
  a = static_cast<T>((int)a - 1);
  return aa;
}
template <class T>
constexpr T operator|(T t1, T t2) {
  return static_cast<T>((int)t1 | (int)t2);
}
template <class T>
T operator&(T t1, T t2) {
  return static_cast<T>((int)t1 & (int)t2);
}
template <class T>
T operator~(T t1) {
  return static_cast<T>(~(int)t1);
}

inline void Set_Bit(void* array, int bit, int value) {
  /*
  #pragma aux Set_Bit parm [esi] [ecx] [eax] \
          modify [esi ebx] = 			\
          "mov	ebx,ecx"					\
          "shr	ebx,5"					\
          "and	ecx,01Fh"				\
          "btr	[esi+ebx*4],ecx"		\
          "or	eax,eax"					\
          "jz	ok"						\
          "bts	[esi+ebx*4],ecx"		\
          "ok:"
  */
  if (value) {
    static_cast<uint32_t*>(array)[static_cast<unsigned>(bit) >> 5] |=
        1 << (bit & 0x1F);
  } else {
    static_cast<uint32_t*>(array)[static_cast<unsigned>(bit) >> 5] &=
        ~(1 << (bit & 0x1F));
  }
}

inline int Get_Bit(const void* array, int bit) {
  /*
          "mov	ebx,eax"					\
          "shr	ebx,5"					\
          "and	eax,01Fh"				\
          "bt	[esi+ebx*4],eax"		\
          "setc	al"
  */
  return !!(
      static_cast<const uint32_t*>(array)[static_cast<unsigned>(bit) >> 5] &
      1 << (bit & 0x1F));
}

inline int First_True_Bit(const void* array) {
  /*
  #pragma aux First_True_Bit parm [esi] \
          modify [esi ebx] \
          value [eax]		= 				\
          "mov	eax,-32"					\
          "again:"							\
          "add	eax,32"					\
          "mov	ebx,[esi]"				\
          "add	esi,4"					\
          "bsf	ebx,ebx"					\
          "jz	again"					\
          "add	eax,ebx"
  */
  const uint32_t* array32 = static_cast<const uint32_t*>(array);
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
  const uint32_t* array32 = static_cast<const uint32_t*>(array);
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

inline int Bound(int original, int minval, int maxval) {
  return std::clamp(original, minval, maxval);
}

unsigned Fixed_To_Cardinal(unsigned base, unsigned fixed);

unsigned Cardinal_To_Fixed(unsigned base, unsigned cardinal);

extern void Fatal(const char* message, ...);

typedef struct {
  unsigned char SourceColor;
  unsigned char DestColor;
  unsigned char Fading;
  unsigned char reserved;
} TLucentType;

int Load_Picture(const char* filename, BufferClass& scratchbuf,
                 BufferClass& destbuf, unsigned char* palette,
                 PicturePlaneType format);
void* Small_Icon(const void* iconptr, int iconnum);
void Set_Window(int window, int x, int y, int w, int h);
void* Load_Alloc_Data(FileClass& file);
long Load_Uncompress(FileClass& file, BuffType& uncomp_buff,
                     BuffType& dest_buff, void* reserved_data);
long Translucent_Table_Size(int count);
void* Build_Translucent_Table(const void* palette, const TLucentType* control,
                              int count, void* buffer);
void* Conquer_Build_Translucent_Table(const void* palette,
                                      const TLucentType* control, int count,
                                      void* buffer);

#endif
