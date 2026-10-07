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
constexpr FacingType operator+(const FacingType lhs, const FacingType rhs) {
  return AsFacing(static_cast<int>(lhs) + static_cast<int>(rhs));
}
constexpr FacingType operator+(const FacingType facing, const int steps) {
  return AsFacing(static_cast<int>(facing) + steps);
}
constexpr FacingType operator-(const FacingType lhs, const FacingType rhs) {
  return AsFacing(static_cast<int>(lhs) - static_cast<int>(rhs));
}
constexpr FacingType operator-(const FacingType facing, const int steps) {
  return AsFacing(static_cast<int>(facing) - steps);
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

// Builds a direction from any angle, wrapping it to the 256-step circle. Use
// it for every computed direction: it is where the analyzer's named-enumerator
// model of the enum is set aside, so a direct cast of a computed value is
// reported as out of range.
constexpr DirType AsDirection(const int angle) {
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  return static_cast<DirType>(static_cast<uint32_t>(angle) & 0xFFU);
}

// Direction arithmetic wraps around the circle.
constexpr DirType operator+(const DirType lhs, const DirType rhs) {
  return AsDirection(static_cast<int>(lhs) + static_cast<int>(rhs));
}
constexpr DirType operator+(const DirType dir, const int steps) {
  return AsDirection(static_cast<int>(dir) + steps);
}
constexpr DirType operator-(const DirType lhs, const DirType rhs) {
  return AsDirection(static_cast<int>(lhs) - static_cast<int>(rhs));
}
constexpr DirType operator-(const DirType dir, const int steps) {
  return AsDirection(static_cast<int>(dir) - steps);
}

// Returns the direction from (from_x, from_y) to (to_x, to_y) as one of the
// eight compass points; (0, 0) is the north-west corner, so y grows southward.
// The diagonals take more than their share of the circle: a slope steeper than
// 1 in 2 off an axis already counts as diagonal (26.6 degrees, where an even
// split would be 22.5). Equal points give DirType::kNorthEast.
DirType Direction8Toward(int from_x, int from_y, int to_x, int to_y);

// Returns the direction from (from_x, from_y) to (to_x, to_y) to full 256-step
// accuracy, with the same axes as Direction8Toward(). The angle is interpolated
// linearly along the shorter offset rather than taken from an arctangent, so
// it is exact only on the axes and diagonals and up to four steps off between
// them. An angle between two steps rounds toward the axis its quadrant starts
// on, going clockwise. Equal points give 255, just shy of north.
DirType Direction256Toward(int from_x, int from_y, int to_x, int to_y);

// Returns the facing nearest to `dir`; the last sixteenth of the circle
// rounds up to FacingType::kNorth.
constexpr FacingType NearestFacing(const DirType dir) {
  return static_cast<FacingType>(static_cast<int>(dir + 0x10) / 32);
}

// Returns the direction that points exactly at `facing`.
constexpr DirType DirectionOf(const FacingType facing) {
  return AsDirection(static_cast<int>(facing) * 32);
}

// Returns `dir` as the nearest of 16 evenly spaced body frames, 0 north and
// rising clockwise.
constexpr int BodyFrame16(const DirType dir) {
  return static_cast<int>(dir + 8) / 16;
}

// Returns `dir` as one of 32 body frames, 0 north and rising clockwise. The
// steps are uneven to undo the distortion 3D Studio rendered into the
// vehicle shapes.
int BodyFrame32(DirType dir);

// Returns `to` minus `from` with both read as signed bytes (DirType::kSouth
// and above are negative), so the result spans -255..255. This is not the
// shortest turn between the two: across south it is nearly a full circle (127
// to 128 gives -255, not 1).
constexpr int SignedDirectionDifference(const DirType from, const DirType to) {
  return static_cast<int8_t>(to) - static_cast<int8_t>(from);
}

}  // namespace game

// The path finder and the facing scans walk the facings in order.
template <>
inline constexpr bool base::kIsSteppableEnum<game::FacingType> = true;

#endif  // CNC_RED_ALERT_GAME_SIM_DIRECTION_H_
