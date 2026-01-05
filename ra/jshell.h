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

#ifndef JSHELL_H
#define JSHELL_H

#include <cassert>
#include <cstdint>

#include "ra/compat.h"
#include "ra/noinit.h"
#include "ra/palette.h"
#include "ra/wwfile.h"
#include "sdllib/include/buffer.h"
#include "sdllib/include/iff.h"
#include "sdllib/include/keyboard.h"
#include "sdllib/include/timer.h"
#include "sdllib/include/ww_mouse.h"

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

#ifndef WIN32
  int &MouseQX;
  int &MouseQY;

  KeyboardClass() : IsLibrary(true), MouseQX(::MouseQX), MouseQY(::MouseQY) {}
  KeyNumType Get(void) {
    return (IsLibrary ? (KeyNumType)Get_Key_Num() : (KeyNumType)getch());
  };
  KeyNumType Check(void) {
    return (IsLibrary ? (KeyNumType)Check_Key_Num() : (KeyNumType)kbhit());
  };
  KeyASCIIType To_ASCII(KeyNumType key) {
    return ((KeyASCIIType)KN_To_KA(key));
  };
  void Clear(void) {
    if (IsLibrary) Clear_KeyBuffer();
  };
  int Down(KeyNumType key) { return (Key_Down(key)); };
#else
  KeyboardClass() : IsLibrary(true) {}
  KeyNumType Get(void) { return ((KeyNumType)WWKeyboardClass::Get()); };
  KeyNumType Check(void) { return ((KeyNumType)WWKeyboardClass::Check()); };
  KeyASCIIType To_ASCII(KeyNumType key) {
    return ((KeyASCIIType)WWKeyboardClass::To_ASCII(key));
  };
  void Clear(void) { WWKeyboardClass::Clear(); };
  int Down(KeyNumType key) { return (WWKeyboardClass::Down(key)); };
#endif

  int Mouse_X(void) { return (Get_Mouse_X()); };
  int Mouse_Y(void) { return (Get_Mouse_Y()); };
};

/*
**	These templates allow enumeration types to have simple bitwise
**	arithmatic performed. The operators must be instatiated for the
**	enumerated types desired.
*/
template <class T>
inline T operator++(T &a) {
  a = (T)((int)a + (int)1);
  return (a);
}
template <class T>
inline T operator++(T &a, int) {
  T aa = a;
  a = (T)((int)a + (int)1);
  return (aa);
}
template <class T>
inline T operator--(T &a) {
  a = (T)((int)a - (int)1);
  return (a);
}
template <class T>
inline T operator--(T &a, int) {
  T aa = a;
  a = (T)((int)a - (int)1);
  return (aa);
}
template <class T>
inline constexpr T operator|(T t1, T t2) {
  return ((T)((int)t1 | (int)t2));
}
template <class T>
inline T operator&(T t1, T t2) {
  return ((T)((int)t1 & (int)t2));
}
template <class T>
inline T operator~(T t1) {
  return ((T)(~(int)t1));
}

#ifndef WIN32
template <class T>
inline T min(T value1, T value2) {
  if (value1 < value2) {
    return (value1);
  }
  return (value2);
}
int min(int, int);
long min(long, long);

template <class T>
inline T max(T value1, T value2) {
  if (value1 > value2) {
    return (value1);
  }
  return (value2);
}
int max(int, int);
long max(long, long);
#endif

template <class T>
inline void swap(T &value1, T &value2) {
  T temp = value1;
  value1 = value2;
  value2 = temp;
}
int swap(int, int);
long swap(long, long);

// TODO(konove): Replace with std::clamp
template <class T>
inline T Bound(T original, T minval, T maxval) {
  if (original < minval) return (minval);
  if (original > maxval) return (maxval);
  return (original);
};

template <class T>
T _rotl(T X, int n) {
  return ((T)((((X) << n) | ((X) >> ((sizeof(T) * 8) - n)))));
}

/*
**	This macro serves as a general way to determine the number of elements
**	within an array.
*/
#define ARRAY_LENGTH(x) int(sizeof(x) / sizeof(x[0]))
#define ARRAY_SIZE(x) int(sizeof(x) / sizeof(x[0]))

inline void Set_Bit(void *array, int bit, int value) {
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
  if (value)
    ((uint32_t *)array)[(unsigned)bit >> 5] |= (1 << (bit & 0x1F));
  else
    ((uint32_t *)array)[(unsigned)bit >> 5] &= ~(1 << (bit & 0x1F));
}

inline int Get_Bit(void const *array, int bit) {
  /*
          "mov	ebx,eax"					\
          "shr	ebx,5"					\
          "and	eax,01Fh"				\
          "bt	[esi+ebx*4],eax"		\
          "setc	al"
  */
  return !!(((const uint32_t *)array)[(unsigned)bit >> 5] &
            (1 << (bit & 0x1F)));
}

inline int First_True_Bit(void const *array) {
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
  const uint32_t *array32 = (const uint32_t *)array;
  int off = 0;
  while (true) {
    uint32_t v = *array32++;
#ifdef _MSC_VER
    DWORD pos;
    if (_BitScanForward(&pos, v)) return off + pos;
#else
    int pos = __builtin_ffs(v);
    if (pos) return off + pos - 1;
#endif
    off += 32;
  }
}
inline int First_False_Bit(void const *array) {
  /*
  #pragma aux First_False_Bit parm [esi] \
          modify [esi ebx] \
          value [eax]		= 				\
          "mov	eax,-32"					\
          "again:"							\
          "add	eax,32"					\
          "mov	ebx,[esi]"				\
          "not	ebx"						\
          "add	esi,4"					\
          "bsf	ebx,ebx"					\
          "jz	again"					\
          "add	eax,ebx"
  */
  const uint32_t *array32 = (const uint32_t *)array;
  int off = 0;
  while (true) {
    uint32_t v = *array32++;
#ifdef _MSC_VER
    DWORD pos;
    if (_BitScanForward(&pos, ~v)) return off + pos;
#else
    int pos = __builtin_ffs(~v);
    if (pos) return off + pos - 1;
#endif
    off += 32;
  }
}

#ifdef OBSOLETE
extern int Bound(int original, int min, int max);
#pragma aux Bound parm[eax][ebx]                       \
    [ecx] modify[eax] value[eax] =                     \
                                "cmp	ebx,ecx"          \
                                "jl	okorder"           \
                                "xchg	ebx,ecx"         \
                                "okorder: cmp	eax,ebx" \
                                "jg	okmin"             \
                                "mov	eax,ebx"          \
                                "okmin: cmp	eax,ecx"   \
                                "jl	okmax"             \
                                "mov	eax,ecx"          \
                                "okmax:"

extern unsigned Bound(unsigned original, unsigned min, unsigned max);
#pragma aux Bound parm[eax][ebx]                       \
    [ecx] modify[eax] value[eax] =                     \
                                "cmp	ebx,ecx"          \
                                "jb	okorder"           \
                                "xchg	ebx,ecx"         \
                                "okorder: cmp	eax,ebx" \
                                "ja	okmin"             \
                                "mov	eax,ebx"          \
                                "okmin: cmp	eax,ecx"   \
                                "jb	okmax"             \
                                "mov	eax,ecx"          \
                                "okmax:"
#endif

unsigned Fixed_To_Cardinal(unsigned base, unsigned fixed);
#pragma aux Fixed_To_Cardinal parm[eax]               \
    [edx] modify[edx] value[eax] =                    \
                                "mul	edx"             \
                                "add	eax,080h"        \
                                "test	eax,0FF000000h" \
                                "jz	ok"               \
                                "mov	eax,000FFFFFFh"  \
                                "ok:"                 \
                                "shr	eax,8"

unsigned Cardinal_To_Fixed(unsigned base, unsigned cardinal);
#pragma aux Cardinal_To_Fixed parm[ebx]       \
    [eax] modify[edx] value[eax] =            \
                                "or	ebx,ebx"  \
                                "jz	fini"     \
                                "shl	eax,8"   \
                                "xor	edx,edx" \
                                "div	ebx"     \
                                "fini:"

#ifndef OUTPORTB
#define OUTPORTB
extern void outportb(int port, unsigned char data);
#pragma aux outportb parm[edx][al] = "out	dx,al"

extern void outport(int port, unsigned short data);
#pragma aux outport parm[edx][ax] =           \
                                  "out	dx,al" \
                                  "inc	dx"    \
                                  "mov	al,ah" \
                                  "out	dx,al"
#endif

/*
**	Timer objects that fetch the appropriate timer value according to
**	the type of timer they are.
*/
extern std::uint64_t Frame;
class FrameTimerClass {
 public:
  std::uint64_t operator()(void) const { return (Frame); };
  operator std::uint64_t(void) const { return (Frame); };
};

#ifndef WIN32
extern bool TimerSystemOn;
extern "C" {
std::uint64_t Get_System_Tick_Count(void);
std::uint64_t Get_User_Tick_Count(void);
}
// bool Init_Timer_System(unsigned int freq, int partial=false);
bool Remove_Timer_System(void);
#else
extern WinTimerClass *WindowsTimer;
#endif

#ifndef SYSTEM_TIMER_CLASS
#define SYSTEM_TIMER_CLASS
class SystemTimerClass {
 public:
#ifdef WIN32
  std::uint64_t operator()(void) const {
    if (!WindowsTimer) return (0);
    return (WindowsTimer->Get_System_Tick_Count());
  };
  operator std::uint64_t(void) const {
    if (!WindowsTimer) return (0);
    return (WindowsTimer->Get_System_Tick_Count());
  };
#else
  std::uint64_t operator()(void) const { return (Get_System_Tick_Count()); };
  operator std::uint64_t(void) const { return (Get_System_Tick_Count()); };
#endif
};
#endif

class UserTimerClass {
 public:
#ifdef WIN32
  std::uint64_t operator()(void) const {
    if (!WindowsTimer) return (0);
    return (WindowsTimer->Get_User_Tick_Count());
  };
  operator std::uint64_t(void) const {
    if (!WindowsTimer) return (0);
    return (WindowsTimer->Get_User_Tick_Count());
  };
#else
  std::uint64_t operator()(void) const { return (Get_User_Tick_Count()); };
  operator std::uint64_t(void) const { return (Get_User_Tick_Count()); };
#endif
};

template <class T>
void Bubble_Sort(T *array, int count) {
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
void PBubble_Sort(T *array, int count) {
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
void PNBubble_Sort(T *array, int count) {
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

template <class T>
class SmartPtr {
 public:
  SmartPtr(NoInitClass const &) {}
  SmartPtr(T *realptr = nullptr) : Pointer(realptr) {}
  SmartPtr(SmartPtr const &rvalue) : Pointer(rvalue.Pointer) {}
  ~SmartPtr(void) { Pointer = nullptr; }

  operator T *(void) const { return (Pointer); }

  operator long(void) const { return ((long)Pointer); }

  SmartPtr<T> operator++(int) {
    assert(Pointer != nullptr);
    SmartPtr<T> temp = *this;
    ++Pointer;
    return (temp);
  }
  SmartPtr<T> &operator++(void) {
    assert(Pointer != nullptr);
    ++Pointer;
    return (*this);
  }
  SmartPtr<T> operator--(int) {
    assert(Pointer != nullptr);
    SmartPtr<T> temp = *this;
    --Pointer;
    return (temp);
  }
  SmartPtr<T> &operator--(void) {
    assert(Pointer != nullptr);
    --Pointer;
    return (*this);
  }

  SmartPtr &operator=(SmartPtr const &rvalue) {
    Pointer = rvalue.Pointer;
    return (*this);
  }
  T *operator->(void) const {
    assert(Pointer != nullptr);
    return (Pointer);
  }
  T &operator*(void) const {
    assert(Pointer != nullptr);
    return (*Pointer);
  }

 private:
  T *Pointer;
};

typedef struct {
  unsigned char SourceColor;
  unsigned char DestColor;
  unsigned char Fading;
  unsigned char reserved;
} TLucentType;

int Load_Picture(char const *filename, BufferClass &scratchbuf,
                 BufferClass &destbuf, unsigned char *palette,
                 PicturePlaneType format);
void *Conquer_Build_Fading_Table(PaletteClass const &palette, void *dest,
                                 int color, int frac);
void *Small_Icon(void const *iconptr, int iconnum);
void Set_Window(int window, int x, int y, int w, int h);
void *Load_Alloc_Data(FileClass &file);
long Load_Uncompress(FileClass &file, BuffType &uncomp_buff,
                     BuffType &dest_buff, void *reserved_data);
long Translucent_Table_Size(int count);
void *Build_Translucent_Table(PaletteClass const &palette,
                              TLucentType const *control, int count,
                              void *buffer);
void *Conquer_Build_Translucent_Table(PaletteClass const &palette,
                                      TLucentType const *control, int count,
                                      void *buffer);
void *Make_Fading_Table(PaletteClass const &palette, void *dest, int color,
                        int frac);

void Fatal(char const *message, ...);

#endif
