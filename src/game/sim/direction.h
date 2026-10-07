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

// The two direction types both games share. FacingType is one of the eight
// neighbouring cells; DirType is a 256-step compass where 0 is north and
// values rise clockwise, so one facing is 32 direction steps. Conversions
// between them and the direction from one point to another live here too.
//
// Originally FACE.H by Joe L. Bostic, started March 8, 1996.

#ifndef CNC_RED_ALERT_GAME_SIM_DIRECTION_H_
#define CNC_RED_ALERT_GAME_SIM_DIRECTION_H_

#include <cstdint>

#include "base/enum_step.h"  // IWYU pragma: export

namespace game {

// The eight cells around a cell, clockwise from north. Used when moving
// between cells and by the path finder.
enum class FacingType : int8_t {
  kNone = -1,  // No facing: a failed search, or the end of a path.
  kNorth,
  kNorthEast,
  kEast,
  kSouthEast,
  kSouth,
  kSouthWest,
  kWest,
  kNorthWest
};

// Wraps a facing sum or difference onto the eight compass facings. Negative
// values wrap the way the old low-three-bits mask did (-1 is
// FacingType::kNorthWest), so the result is never FacingType::kNone.
constexpr FacingType AsFacing(const int facing) {
  return static_cast<FacingType>(static_cast<uint32_t>(facing) & 0x07U);
}

// Facing arithmetic wraps around the eight facings. FacingType::kNone is not a
// number to add to: as -1 it would step one facing anticlockwise.
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

// A direction on the 256-step compass: 0 is north, 64 east, 128 south and 192
// west. Every byte value is a valid direction; only the compass points and
// the two harvester poses are named.
enum class DirType : uint8_t {
  kNorth = 0,
  kNorthEast = 1 << 5,
  kEast = 2 << 5,
  kSouthEast = 3 << 5,
  kSouth = 4 << 5,
  kSouthWest = 5 << 5,
  // Two directions a little south of south-west that a harvester turns
  // through as it backs onto the refinery pad; it unloads at kSouthWestX2.
  kSouthWestX1 = (5 << 5) - 8,
  kSouthWestX2 = (5 << 5) - 16,
  kWest = 6 << 5,
  kNorthWest = 7 << 5
};

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

// Returns the direction from (x1, y1) to (x2, y2) as one of the eight compass
// points; (0, 0) is the north-west corner, so y grows southward. The diagonals
// take more than their share of the circle: a slope steeper than 1 in 2 off an
// axis already counts as diagonal (26.6 degrees, where an even split would be
// 22.5). Equal points give DirType::kNorthEast.
DirType Desired_Facing8(int x1, int y1, int x2, int y2);

// Returns the direction from (srcx, srcy) to (dstx, dsty) to full 256-step
// accuracy, with the same axes as Desired_Facing8(). The angle is interpolated
// linearly along the shorter offset rather than taken from an arctangent, so
// it is exact only on the axes and diagonals and up to four steps off between
// them. Due east, due south, exact south-east and exact north-west come out
// one step short (DirType::kEast - 1, ...), as they did in the original. Equal
// points give 255, just shy of north.
DirType Desired_Facing256(int srcx, int srcy, int dstx, int dsty);

// Returns the facing nearest to `dir`; the last sixteenth of the circle
// rounds up to FacingType::kNorth.
constexpr FacingType Dir_Facing(const DirType dir) {
  return static_cast<FacingType>(static_cast<int>(dir + 0x10) / 32);
}

// Returns the direction that points exactly at `facing`.
constexpr DirType Facing_Dir(const FacingType facing) {
  return AsDirection(static_cast<int>(facing) * 32);
}

// Returns the facing whose 32-step sector starting at it holds `dir`: unlike
// Dir_Facing(), this truncates, so DirType::kNorthEast - 1 gives
// FacingType::kNorth.
// TODO: the `| 0x10` changes nothing, since it sets a bit the division by 32
// drops. The original probably meant the `+ 0x10` of Dir_Facing().
constexpr FacingType Dir_To_8(const DirType dir) {
  return static_cast<FacingType>((static_cast<uint32_t>(dir) | 0x10U) / 32);
}

// Returns `dir` as one of 16 body frames, 0 north and rising clockwise.
int Dir_To_16(DirType dir);

// Returns `dir` as one of 32 body frames, 0 north and rising clockwise. The
// steps are uneven to undo the distortion 3D Studio rendered into the
// vehicle shapes.
int Dir_To_32(DirType dir);

// Returns `dir2` minus `dir1` with both read as signed bytes (DirType::kSouth
// and above are negative), so the result spans -255..255. This is not the
// shortest turn between the two: across south it is nearly a full circle (127
// to 128 gives -255, not 1).
constexpr int Dir_Diff(const DirType dir1, const DirType dir2) {
  return static_cast<int8_t>(dir2) - static_cast<int8_t>(dir1);
}

}  // namespace game

// The path finder and the facing scans walk the facings in order.
template <>
inline constexpr bool base::kIsSteppableEnum<game::FacingType> = true;

#endif  // CNC_RED_ALERT_GAME_SIM_DIRECTION_H_
