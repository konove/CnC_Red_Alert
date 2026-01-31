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

/* $Header: /CounterStrike/COMPAT.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : COMPAT.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 03/02/95 *
 *                                                                                             *
 *                  Last Update : March 2, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef COMPAT_H
#define COMPAT_H

#include <cstddef>

#define BuffType BufferClass
// #define movmem(a,b,c) memmove(b,a,c)
#define ShapeBufferSize _ShapeBufferSize

/*=========================================================================*/
/* Define some equates for the different graphic routines we will install
 */
/*		later.
 */
/*=========================================================================*/
#define HIDBUFF ((void*)(0xA0000))
#define Size_Of_Region(a, b) ((a) * (b))

/*=========================================================================*/
/* Define some Graphic Routines which will only be fixed by these defines
 */
/*=========================================================================*/
#define Set_Font_Palette(a) Set_Font_Palette_Range(a, 0, 15)

/*
**	These are the Open_File, Read_File, and Seek_File constants.
*/
#define READ 1   // Read access.
#define WRITE 2  // Write access.
#include "sdllib/include/tile.h"

#ifndef SEEK_SET
#define SEEK_SET 0  // Seek from start of file.
#define SEEK_CUR 1  // Seek relative from current location.
#define SEEK_END 2  // Seek from end of file.
#endif

#define ERROR_WINDOW 1
#define ErrorWindow 1

// extern unsigned char *Palette;
extern unsigned char MDisabled;  // Is mouse disabled?
#ifndef WIN32
extern WORD Hard_Error_Occured;
#endif

/*
**	This is the menu control structures.
*/
typedef enum MenuIndexType {
  MENUX,
  MENUY,
  ITEMWIDTH,
  ITEMSHIGH,
  MSELECTED,
  NORMCOL,
  HILITE,
  MENUPADDING = 0x1000
} MenuIndexType;

/* These defines handle the various names given to the same color. */
#define DKGREEN GREEN
#define DKBLUE BLUE
#define GRAY GREY
#define DKGREY GREY
#define DKGRAY GREY
#define LTGRAY LTGREY

inline short Get_IconSet_MapWidth(const void* data) {
  if (data) {
    return ((IControl_Type*)data)->MapWidth;
  }
  return 0;
}

inline short Get_IconSet_MapHeight(const void* data) {
  if (data) {
    return ((IControl_Type*)data)->MapHeight;
  }
  return 0;
}

inline const unsigned char* Get_IconSet_ControlMap(const void* data) {
  if (data) {
    return (const unsigned char*)((char*)data +
                                  ((IControl_Type*)data)->ColorMap);
  }
  return nullptr;
}

class IconsetClass : protected IControl_Type {
 public:
  /*
  **	Query functions.
  */
  int Map_Width() const { return MapWidth; }
  int Map_Height() const { return MapHeight; }
  unsigned char* Control_Map() { return (unsigned char*)this + ColorMap; }
  const unsigned char* Control_Map() const {
    return (const unsigned char*)this + ColorMap;
  }
  int Icon_Count() const { return Count; }
  int Pixel_Width() const { return Width; }
  int Pixel_Height() const { return Height; }
  int Total_Size() const { return Size; }
  const unsigned char* Palette_Data() const {
    return (const unsigned char*)this + Palettes;
  }
  unsigned char* Palette_Data() { return (unsigned char*)this + Palettes; }
  const unsigned char* Icon_Data() const {
    return (const unsigned char*)this + Icons;
  }
  unsigned char* Icon_Data() { return (unsigned char*)this + Icons; }
  const unsigned char* Map_Data() const {
    return (const unsigned char*)this + Map;
  }
  unsigned char* Map_Data() { return (unsigned char*)this + Map; }
  const unsigned char* Remap_Data() const {
    return (const unsigned char*)this + Remaps;
  }
  unsigned char* Remap_Data() { return (unsigned char*)this + Remaps; }
  const unsigned char* Trans_Data() const {
    return (const unsigned char*)this + TransFlag;
  }
  unsigned char* Trans_Data() { return (unsigned char*)this + TransFlag; }

  /*
  **	Disallow these operations with an IconsetClass object.
  */
 private:
  IconsetClass& operator=(const IconsetClass&);
  IconsetClass();
  void* operator new(size_t);
};

#endif
