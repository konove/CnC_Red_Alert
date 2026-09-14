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

#ifndef CNC_RED_ALERT_RA_COMPAT_H_
#define CNC_RED_ALERT_RA_COMPAT_H_

#include <cstddef>
#include <cstdint>

#define BuffType BufferClass
// #define movmem(a,b,c) memmove(b,a,c)

/*=========================================================================*/
/* Define some equates for the different graphic routines we will install
 */
/*		later.
 */
/*=========================================================================*/
#define HIDBUFF ((void*)(0xA0000))
#define Size_Of_Region(a, b) ((a) * (b))

#include "absl/base/attributes.h"
#include "sdllib/tile.h"

#ifndef SEEK_SET
#define SEEK_SET 0  // Seek from start of file.
#define SEEK_CUR 1  // Seek relative from current location.
#define SEEK_END 2  // Seek from end of file.
#endif

#define ERROR_WINDOW 1
#define ErrorWindow 1

// extern unsigned char *Palette;

/*
**	This is the menu control structures.
*/
typedef enum MenuIndexType {
  MENUX = 0,
  MENUY = 1,
  ITEMWIDTH = 2,
  ITEMSHIGH = 3,
  MSELECTED = 4,
  NORMCOL = 5,
  HILITE = 6,
  MENUPADDING = 0x1000
} MenuIndexType;

/* These defines handle the various names given to the same color. */
#define DKGREEN GREEN
#define DKBLUE BLUE
#define GRAY GREY
#define DKGREY GREY
#define DKGRAY GREY
#define LTGRAY LTGREY

inline int16_t Get_IconSet_MapWidth(const void* data) {
  if (data) {
    return static_cast<const IControl_Type*>(data)->MapWidth;
  }
  return 0;
}

inline int16_t Get_IconSet_MapHeight(const void* data) {
  if (data) {
    return static_cast<const IControl_Type*>(data)->MapHeight;
  }
  return 0;
}

inline const unsigned char* Get_IconSet_ControlMap(
    const void* data ABSL_ATTRIBUTE_LIFETIME_BOUND) {
  if (data) {
    return static_cast<const unsigned char*>(data) +
           static_cast<const IControl_Type*>(data)->ColorMap;
  }
  return nullptr;
}

class IconsetClass : protected IControl_Type {
 public:
  /*
  **	Query functions.
  */
  [[nodiscard]] int Map_Width() const { return MapWidth; }
  [[nodiscard]] int Map_Height() const { return MapHeight; }
  unsigned char* Control_Map() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return (unsigned char*)this + ColorMap;
  }
  [[nodiscard]] const unsigned char* Control_Map() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return (const unsigned char*)this + ColorMap;
  }
  [[nodiscard]] int Icon_Count() const { return Count; }
  [[nodiscard]] int Pixel_Width() const { return Width; }
  [[nodiscard]] int Pixel_Height() const { return Height; }
  [[nodiscard]] int Total_Size() const { return Size; }
  [[nodiscard]] const unsigned char* Palette_Data() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return (const unsigned char*)this + Palettes;
  }
  unsigned char* Palette_Data() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return (unsigned char*)this + Palettes;
  }
  [[nodiscard]] const unsigned char* Icon_Data() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return (const unsigned char*)this + Icons;
  }
  unsigned char* Icon_Data() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return (unsigned char*)this + Icons;
  }
  [[nodiscard]] const unsigned char* Map_Data() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return (const unsigned char*)this + Map;
  }
  unsigned char* Map_Data() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return (unsigned char*)this + Map;
  }
  [[nodiscard]] const unsigned char* Remap_Data() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return (const unsigned char*)this + Remaps;
  }
  unsigned char* Remap_Data() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return (unsigned char*)this + Remaps;
  }
  [[nodiscard]] const unsigned char* Trans_Data() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return (const unsigned char*)this + TransFlag;
  }
  unsigned char* Trans_Data() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return (unsigned char*)this + TransFlag;
  }

  /*
  **	Disallow these operations with an IconsetClass object.
  */
 private:
 public:
  IconsetClass() = delete;
  ~IconsetClass() = delete;
  IconsetClass(const IconsetClass&) = delete;
  IconsetClass& operator=(const IconsetClass&) = delete;
  IconsetClass(IconsetClass&&) = delete;
  IconsetClass& operator=(IconsetClass&&) = delete;

 private:
  void* operator new(size_t);
};

#endif  // CNC_RED_ALERT_RA_COMPAT_H_
