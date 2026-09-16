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

/* $Header: /CounterStrike/FACE.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : FACE.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 03/08/96 *
 *                                                                                             *
 *                  Last Update : March 8, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_FACE_H_
#define CNC_RED_ALERT_RA_FACE_H_

#include <bit>
#include <cstdint>

#include "ra/defines.h"

// Enumerations of the facing values returned from Desired_Facing().
enum class DirType : uint8_t {
  DIR_MIN = 0,
  DIR_N = 0,
  DIR_NE = 1 << 5,
  DIR_E = 2 << 5,
  DIR_SE = 3 << 5,
  DIR_S = 4 << 5,
  DIR_SW = 5 << 5,
  DIR_SW_X1 = (5 << 5) - 8,   // Direction of harvester while unloading.
  DIR_SW_X2 = (5 << 5) - 16,  // Direction of harvester while unloading.
  DIR_W = 6 << 5,
  DIR_NW = 7 << 5,
  DIR_MAX = 255
};
using enum DirType;

// Builds a direction from any angle, wrapping it to the 256-step circle.
// Every value is a valid DirType; only the compass points are named. This is
// the one place that turns a computed integer into the type, and the one place
// the analyzer's named-enumerator model of the enum is set aside.
constexpr DirType AsDirection(const int angle) {
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  return static_cast<DirType>(static_cast<uint32_t>(angle) & 0xFFU);
}

// Operators that allow simple math with DirType.
constexpr DirType operator+(const DirType f1, const DirType f2) {
  return AsDirection(static_cast<int>(f1) + static_cast<int>(f2));
}
constexpr DirType operator+(const DirType f1, const int f2) {
  return AsDirection(static_cast<int>(f1) + f2);
}
constexpr DirType operator-(const DirType f1, const DirType f2) {
  return AsDirection(static_cast<int>(f1) - static_cast<int>(f2));
}
constexpr DirType operator-(const DirType f1, const int f2) {
  return AsDirection(static_cast<int>(f1) - f2);
}

// Function prototypes.
DirType Desired_Facing8(int x1, int y1, int x2, int y2);
DirType Desired_Facing256(int srcx, int srcy, int dstx, int dsty);

// Calculates the DirType from one cell to another (8-direction accuracy).
inline DirType Direction(CELL cell1, CELL cell2) {
  const auto from = std::bit_cast<CELL_COMPOSITE>(cell1);
  const auto to = std::bit_cast<CELL_COMPOSITE>(cell2);
  return Desired_Facing8(from.Sub.X, from.Sub.Y, to.Sub.X, to.Sub.Y);
}

#endif  // CNC_RED_ALERT_RA_FACE_H_
