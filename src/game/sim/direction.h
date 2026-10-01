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

// File: The two direction types both games share. FacingType is one of the
// eight neighbouring cells; DirType is a 256-step compass where 0 is north and
// values rise clockwise. Conversions between them and the facing between two
// points live here too.

#ifndef CNC_RED_ALERT_GAME_SIM_DIRECTION_H_
#define CNC_RED_ALERT_GAME_SIM_DIRECTION_H_

#include <cstdint>

namespace game {

// The eight cells around a cell, clockwise from north. Used when moving
// between cells and by the path finder.
enum class FacingType : int8_t {
  FACING_NONE = -1,
  FACING_N,   // North
  FACING_NE,  // North-East
  FACING_E,   // East
  FACING_SE,  // South-East
  FACING_S,   // South
  FACING_SW,  // South-West
  FACING_W,   // West
  FACING_NW   // North-West
};
using enum FacingType;

// Wraps a facing sum or difference onto the eight compass facings. Negative
// values wrap the way the old low-three-bits mask did (-1 is FACING_NW).
constexpr FacingType AsFacing(const int facing) {
  return static_cast<FacingType>(static_cast<uint32_t>(facing) & 0x07U);
}

constexpr FacingType operator+(const FacingType f1, const FacingType f2) {
  return AsFacing(static_cast<int>(f1) + static_cast<int>(f2));
}
constexpr FacingType operator+(const FacingType f1, const int f2) {
  return AsFacing(static_cast<int>(f1) + f2);
}
constexpr FacingType operator-(const FacingType f1, const FacingType f2) {
  return AsFacing(static_cast<int>(f1) - static_cast<int>(f2));
}
constexpr FacingType operator-(const FacingType f1, const int f2) {
  return AsFacing(static_cast<int>(f1) - f2);
}
constexpr FacingType operator+=(FacingType& f1, const FacingType f2) {
  f1 = f1 + f2;
  return f1;
}
constexpr FacingType operator+=(FacingType& f1, const int f2) {
  f1 = f1 + f2;
  return f1;
}

// The facing values returned from Desired_Facing8() and Desired_Facing256().
// Every byte value is a valid direction; only the compass points are named.
enum class DirType : uint8_t {
  DIR_N = 0,
  DIR_NE = 1 << 5,
  DIR_E = 2 << 5,
  DIR_SE = 3 << 5,
  DIR_S = 4 << 5,
  DIR_SW = 5 << 5,
  DIR_SW_X1 = (5 << 5) - 8,   // Direction of harvester while unloading.
  DIR_SW_X2 = (5 << 5) - 16,  // Direction of harvester while unloading.
  DIR_W = 6 << 5,
  DIR_NW = 7 << 5
};
using enum DirType;

// Builds a direction from any angle, wrapping it to the 256-step circle.
// This is the one place that turns a computed integer into the type, and the
// one place the analyzer's named-enumerator model of the enum is set aside.
constexpr DirType AsDirection(const int angle) {
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  return static_cast<DirType>(static_cast<uint32_t>(angle) & 0xFFU);
}

// Direction arithmetic wraps around the circle.
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

// Returns the direction from (x1, y1) to (x2, y2), accurate to the eight
// compass points; (0, 0) is the north-west corner. Equal points give DIR_NE.
DirType Desired_Facing8(int x1, int y1, int x2, int y2);

// Returns the direction from (srcx, srcy) to (dstx, dsty) to full 256-step
// accuracy. Equal points give 255, just shy of north.
DirType Desired_Facing256(int srcx, int srcy, int dstx, int dsty);

// Returns the facing nearest to `dir`; the last sixteenth of the circle
// rounds up to FACING_N.
constexpr FacingType Dir_Facing(const DirType dir) {
  return static_cast<FacingType>(static_cast<int>(dir + 0x10) / 32);
}

// Returns the direction that points exactly at `facing`.
constexpr DirType Facing_Dir(const FacingType facing) {
  return AsDirection(static_cast<int>(facing) * 32);
}

// Returns the facing whose 32-step sector starting at it holds `dir`: unlike
// Dir_Facing(), this truncates, so DIR_NE - 1 gives FACING_N.
constexpr FacingType Dir_To_8(const DirType dir) {
  return static_cast<FacingType>((static_cast<uint32_t>(dir) | 0x10U) / 32);
}

// Returns `dir` as one of 16 body frames, 0 north and rising clockwise.
int Dir_To_16(DirType dir);

// Returns `dir` as one of 32 body frames, 0 north and rising clockwise. The
// steps are uneven to undo the distortion 3D Studio rendered into the
// vehicle shapes.
int Dir_To_32(DirType dir);

// Returns `dir2` minus `dir1` with both read as signed bytes (DIR_S and above
// are negative), so the result spans -255..255.
constexpr int Dir_Diff(const DirType dir1, const DirType dir2) {
  return static_cast<int8_t>(dir2) - static_cast<int8_t>(dir1);
}

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_SIM_DIRECTION_H_
