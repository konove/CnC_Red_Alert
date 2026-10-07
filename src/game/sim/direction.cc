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

// The direction between two points and the tables that turn a direction into
// a body frame.
//
// Originally FACE.CPP by Joe L. Bostic, started March 8, 1996.

#include "game/sim/direction.h"

#include <array>
#include <cstdint>

#include "absl/log/check.h"

namespace game {
namespace {

// The body frame for each of the 256 directions. A simple shift won't do:
// a direction converts to the CLOSEST frame, not the one below it, so each
// frame's entries straddle its exact direction and the last few wrap to 0.
// The 16-frame table is even, sixteen directions a frame.
constexpr std::array<uint8_t, 256> kBodyFrame16 = {
    0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
    2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  4,
    4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  5,  5,  5,  5,
    5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  6,  6,
    6,  6,  6,  6,  6,  6,  6,  6,  6,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    7,  7,  7,  7,  7,  7,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
    8,  8,  8,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
    14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 0,  0,  0,  0,  0,  0,  0,  0};

// The 32-frame table is uneven, between six and eleven directions a frame,
// to match the angles the vehicle shapes were actually rendered at.
constexpr std::array<uint8_t, 256> kBodyFrame32 = {
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

}  // namespace

DirType Direction8Toward(const int from_x, const int from_y, const int to_x,
                         const int to_y) {
  // The direction being built. It starts as the first direction of the
  // quadrant: 0x00 north-east, 0x40 south-east, 0x80 south-west, 0xC0
  // north-west.
  uint32_t direction = 0;

  // Take the absolute X difference. A westward target starts in the
  // north-west quadrant.
  int x_distance = to_x - from_x;
  if (x_distance < 0) {
    direction |= 0x00C0;
    x_distance = -x_distance;
  }

  // Take the absolute Y difference, north positive. A southward target flips
  // the 0x40 bit, which moves north-east to south-east and north-west to
  // south-west and so settles the quadrant.
  int y_distance = from_y - to_y;
  if (y_distance < 0) {
    direction ^= 0x0040;
    y_distance = -y_distance;
  }

  // Find which of the two offsets is bigger. The bigger one (X or Y) tells
  // which axis the direction is closer to.
  int bigger = 0;
  int smaller = 0;
  if (x_distance < y_distance) {
    smaller = x_distance;
    bigger = y_distance;
  } else {
    smaller = y_distance;
    bigger = x_distance;
  }

  // If the smaller offset is at least half the bigger one, call it the
  // diagonal, which lies 0x20 into every quadrant, and bail. Equal points
  // land here too, in the north-east quadrant.
  if ((bigger + 1) / 2 <= smaller) {
    direction += 0x0020;
    return static_cast<DirType>(direction);
  }

  // Otherwise the direction is the axis at the start of the quadrant or the
  // one at its end, 0x40 further on. The north-east and south-west quadrants
  // start on the Y axis and the other two (0x40 set) on the X axis, so step
  // on when the closer axis is not the one the quadrant starts on. From the
  // north-west quadrant that step wraps to north.
  uint32_t axis_step = direction & 0x0040;
  if (x_distance == bigger) {
    axis_step ^= 0x0040;
  }
  direction += axis_step;

  return AsDirection(static_cast<int>(direction));
}

DirType Direction256Toward(const int from_x, const int from_y, const int to_x,
                           const int to_y) {
  // The direction being built, starting as the first direction of the
  // quadrant exactly as in Direction8Toward().
  uint32_t direction = 0;

  // Take the absolute X difference. A westward target starts in the
  // north-west quadrant.
  int x_distance = to_x - from_x;
  if (x_distance < 0) {
    direction |= 0x00C0;
    x_distance = -x_distance;
  }

  // Take the absolute Y difference, north positive. A southward target flips
  // the 0x40 bit and so settles the quadrant.
  int y_distance = from_y - to_y;
  if (y_distance < 0) {
    direction ^= 0x0040;
    y_distance = -y_distance;
  }

  // Bail early if the coordinates are the same. This also keeps the division
  // below away from zero.
  if (x_distance == 0 && y_distance == 0) {
    return AsDirection(0xFF);
  }

  // Find which of the two offsets is bigger. The bigger one (X or Y) tells
  // which axis the direction is closer to.
  int bigger = 0;
  int smaller = 0;
  if (x_distance < y_distance) {
    smaller = x_distance;
    bigger = y_distance;
  } else {
    smaller = y_distance;
    bigger = x_distance;
  }

  // Now that the quadrant is known, work out how far from the closer axis
  // the direction lies, as a ratio from 0 (on the axis) to 32 (on the
  // diagonal). This is the linear stand-in for the arctangent.
  //
  // CHECK(a != b), not CHECK_NE: the shallow analyzer does not see through the
  // CHECK_NE helper and would still report the division below.
  CHECK(bigger != 0);
  int fraction = smaller * 32 / bigger;

  // `fraction` counts from the closer axis. When that is the axis the quadrant
  // ends on (the same test as in Direction8Toward(), where `axis_step` is
  // 0x40), count back from it instead. The extra -1 makes that half round
  // toward the start of the quadrant like the other half, and keeps the result
  // inside the quadrant, but it also puts a direction exactly on the end axis,
  // or on the diagonal of a quadrant that starts on the X axis, one step short.
  uint32_t axis_step = direction & 0x0040;
  if (x_distance > y_distance) {
    axis_step ^= 0x0040;
  }
  if (axis_step) {
    fraction = static_cast<int>(axis_step) - fraction - 1;
  }

  direction += static_cast<uint32_t>(fraction);
  return AsDirection(static_cast<int>(direction));
}

int BodyFrame16(const DirType dir) {
  return kBodyFrame16.at(static_cast<uint8_t>(dir));
}

int BodyFrame32(const DirType dir) {
  return kBodyFrame32.at(static_cast<uint8_t>(dir));
}

}  // namespace game
