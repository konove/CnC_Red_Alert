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
#include <span>
#include <utility>

#include "base/buffer.h"

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
#include "port/bytes_of.h"
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
inline constexpr int kMenux = 0;
inline constexpr int kMenuy = 1;
inline constexpr int kItemwidth = 2;
inline constexpr int kItemshigh = 3;
inline constexpr int kMselected = 4;
inline constexpr int kNormcol = 5;
inline constexpr int kHilite = 6;
inline constexpr int kMenupadding = 0x1000;

/* These defines handle the various names given to the same color. */
#define DKGREEN kGreen
#define DKBLUE kBlue
#define GRAY kGrey
#define DKGREY kGrey
#define DKGRAY kGrey
#define LTGRAY kLtGrey

// A view retains the archive extent rather than treating a file header as an
// object.
class IconsetClass {
 public:
  explicit IconsetClass(
      std::span<const std::byte> data ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : data_(data) {
    if (data.size() >= sizeof(header_)) {
      base::CopyBytes(base::ObjectBytes(header_), data, sizeof(header_));
    }
  }
  [[nodiscard]] int Map_Width() const { return header_.MapWidth; }
  [[nodiscard]] int Map_Height() const { return header_.MapHeight; }
  [[nodiscard]] int Icon_Count() const { return header_.Count; }
  [[nodiscard]] int Pixel_Width() const { return header_.Width; }
  [[nodiscard]] int Pixel_Height() const { return header_.Height; }
  [[nodiscard]] int Total_Size() const { return header_.Size; }
  [[nodiscard]] std::span<const unsigned char> Control_Map() const {
    return Section(header_.ColorMap);
  }
  [[nodiscard]] std::span<const unsigned char> Palette_Data() const {
    return Section(header_.Palettes);
  }
  [[nodiscard]] std::span<const unsigned char> Icon_Data() const {
    return Section(header_.Icons);
  }
  [[nodiscard]] std::span<const unsigned char> Map_Data() const {
    return Section(header_.Map);
  }
  [[nodiscard]] std::span<const unsigned char> Remap_Data() const {
    return Section(header_.Remaps);
  }
  [[nodiscard]] std::span<const unsigned char> Trans_Data() const {
    return Section(header_.TransFlag);
  }

 private:
  [[nodiscard]] std::span<const unsigned char> Section(int offset) const {
    if (std::cmp_less(offset, sizeof(header_)) ||
        static_cast<size_t>(offset) > data_.size()) {
      return {};
    }
    return base::UnsignedBytes(data_.subspan(static_cast<size_t>(offset)));
  }
  std::span<const std::byte> data_;
  IControl_Type header_{};
};

inline int16_t Get_IconSet_MapWidth(std::span<const std::byte> data) {
  return static_cast<int16_t>(IconsetClass(data).Map_Width());
}
inline int16_t Get_IconSet_MapHeight(std::span<const std::byte> data) {
  return static_cast<int16_t>(IconsetClass(data).Map_Height());
}

#endif  // CNC_RED_ALERT_RA_COMPAT_H_
