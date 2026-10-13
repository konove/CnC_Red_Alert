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

// The two direction types both games share. Facing is one of the eight
// neighbouring cells; Direction is a 256-step compass where 0 is north and
// values rise clockwise, so one facing is 32 direction steps. Conversions
// between them and the direction from one point to another live here too.
//
// Originally FACE.H by Joe L. Bostic, started March 8, 1996.

#ifndef CNC_RED_ALERT_GAME_SIM_DIRECTION_H_
#define CNC_RED_ALERT_GAME_SIM_DIRECTION_H_

#include <array>
#include <cstddef>
#include <cstdint>

#include "base/enum_step.h"  // IWYU pragma: export
#include "base/point.h"      // IWYU pragma: export

namespace game {

// The eight cells around a cell, clockwise from north. Used when moving
// between cells and by the path finder.
enum class Facing : int8_t {
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
// Facing::kNorthWest), so the result is never Facing::kNone.
constexpr Facing AsFacing(const int facing) {
  return static_cast<Facing>(static_cast<uint32_t>(facing) & 0x07U);
}

// Facing arithmetic wraps around the eight facings. Facing::kNone is not a
// number to add to: as -1 it would step one facing anticlockwise.
constexpr Facing operator+(const Facing lhs, const Facing rhs) {
  return AsFacing(static_cast<int>(lhs) + static_cast<int>(rhs));
}
constexpr Facing operator+(const Facing facing, const int steps) {
  return AsFacing(static_cast<int>(facing) + steps);
}
constexpr Facing operator-(const Facing lhs, const Facing rhs) {
  return AsFacing(static_cast<int>(lhs) - static_cast<int>(rhs));
}
constexpr Facing operator-(const Facing facing, const int steps) {
  return AsFacing(static_cast<int>(facing) - steps);
}

// A direction on the 256-step compass: 0 is north, 64 east, 128 south and 192
// west, one byte. Every byte value is a valid direction; only the compass
// points are named. Arithmetic wraps around the circle.
//
// Example:
//   const Direction behind = facing + Direction::kSouth;
//   const Direction a_little_right = facing + 8;
class Direction {
 public:
  static const Direction kNorth;
  static const Direction kNorthEast;
  static const Direction kEast;
  static const Direction kSouthEast;
  static const Direction kSouth;
  static const Direction kSouthWest;
  static const Direction kWest;
  static const Direction kNorthWest;

  // North.
  constexpr Direction() = default;

  // Builds a direction from any angle, wrapping it to the 256-step circle.
  constexpr explicit Direction(const int angle) noexcept
      : value_(static_cast<uint8_t>(static_cast<uint32_t>(angle) & 0xFFU)) {}

  // The angle, 0..255.
  [[nodiscard]] constexpr int value() const noexcept { return value_; }

  friend constexpr bool operator==(Direction, Direction) noexcept = default;
  friend constexpr auto operator<=>(Direction, Direction) noexcept = default;

  friend constexpr Direction operator+(const Direction lhs,
                                       const Direction rhs) noexcept {
    return Direction(lhs.value() + rhs.value());
  }
  friend constexpr Direction operator+(const Direction dir,
                                       const int steps) noexcept {
    return Direction(dir.value() + steps);
  }
  friend constexpr Direction operator-(const Direction lhs,
                                       const Direction rhs) noexcept {
    return Direction(lhs.value() - rhs.value());
  }
  friend constexpr Direction operator-(const Direction dir,
                                       const int steps) noexcept {
    return Direction(dir.value() - steps);
  }

  // Saved-game support. A direction is saved in the four bytes it took when
  // it was an enum.
  template <class Archive>
  void Serialize(Archive& ar) {
    int32_t raw = value_;
    ar(raw);
    value_ = static_cast<uint8_t>(raw);
  }

 private:
  uint8_t value_ = 0;
};

inline constexpr Direction Direction::kNorth{0};
inline constexpr Direction Direction::kNorthEast{1 << 5};
inline constexpr Direction Direction::kEast{2 << 5};
inline constexpr Direction Direction::kSouthEast{3 << 5};
inline constexpr Direction Direction::kSouth{4 << 5};
inline constexpr Direction Direction::kSouthWest{5 << 5};
inline constexpr Direction Direction::kWest{6 << 5};
inline constexpr Direction Direction::kNorthWest{7 << 5};

// Objects, tracks and packets hold a direction in one byte.
static_assert(sizeof(Direction) == 1);

// Returns the direction from `from` to `to` as one of the eight compass points.
// The diagonals take more than their share of the circle: a slope steeper than
// 1 in 2 off an axis already counts as diagonal (26.6 degrees, where an even
// split would be 22.5). Equal points give Direction::kNorthEast.
Direction Direction8Toward(base::Point from, base::Point to);

// Returns the direction from `from` to `to` to full 256-step accuracy. The
// angle is interpolated linearly along the shorter offset rather than taken
// from an arctangent, so it is exact only on the axes and diagonals and up to
// four steps off between them. An angle between two steps rounds toward the
// axis its quadrant starts on, going clockwise. Equal points give 255, just
// shy of north.
Direction Direction256Toward(base::Point from, base::Point to);

// Returns the facing nearest to `dir`; the last sixteenth of the circle
// rounds up to Facing::kNorth.
constexpr Facing NearestFacing(const Direction dir) {
  return static_cast<Facing>((dir + 0x10).value() / 32);
}

// Returns the direction that points exactly at `facing`.
constexpr Direction DirectionOf(const Facing facing) {
  return Direction(static_cast<int>(facing) * 32);
}

// Returns `dir` as the nearest of 16 evenly spaced body frames, 0 north and
// rising clockwise.
constexpr int BodyFrame16(const Direction dir) {
  return (dir + 8).value() / 16;
}

// The body frame for each of the 256 directions. A simple shift won't do:
// a direction converts to the CLOSEST frame, not the one below it, so each
// frame's entries straddle its exact direction and the last few wrap to 0.
// The frames are uneven, between six and eleven directions each, to match the
// angles the vehicle shapes were actually rendered at.
inline constexpr std::array<uint8_t, 256> kBodyFrame32 = {
    0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,
    2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,
    4,  5,  5,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  6,  6,  7,  7,  7,  7,
    7,  7,  7,  8,  8,  8,  8,  8,  8,  8,  9,  9,  9,  9,  9,  9,  9,  10, 10,
    10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12,
    12, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    17, 17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21,
    21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 24, 24,
    24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26,
    27, 27, 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29,
    29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 30, 30, 31, 31, 31, 31, 31, 31,
    31, 31, 31, 0,  0,  0,  0,  0,  0};

// Returns `dir` as one of 32 body frames, 0 north and rising clockwise. The
// steps are uneven to undo the distortion 3D Studio rendered into the
// vehicle shapes.
constexpr int BodyFrame32(const Direction dir) {
  return kBodyFrame32.at(static_cast<std::size_t>(dir.value()));
}

// Returns the shortest turn from `from` to `to`, -128..127: positive is
// clockwise, and the two opposite directions are -128 apart.
constexpr int ShortestTurn(const Direction from, const Direction to) {
  return static_cast<int8_t>((to - from).value());
}

}  // namespace game

// The path finder and the facing scans walk the facings in order.
template <>
inline constexpr bool base::kIsSteppableEnum<game::Facing> = true;

#endif  // CNC_RED_ALERT_GAME_SIM_DIRECTION_H_
