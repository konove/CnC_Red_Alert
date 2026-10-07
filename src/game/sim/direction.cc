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
constexpr std::array<uint8_t, 256> kFacing16 = {
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
constexpr std::array<uint8_t, 256> kFacing32 = {
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

DirType Desired_Facing8(int x1, int y1, int x2, int y2) {
  // The direction being built. It starts as the first direction of the
  // quadrant: 0x00 north-east, 0x40 south-east, 0x80 south-west, 0xC0
  // north-west.
  uint32_t index = 0;

  // Take the absolute X difference. A westward target starts in the
  // north-west quadrant.
  int xdiff = x2 - x1;
  if (xdiff < 0) {
    index |= 0x00C0;
    xdiff = -xdiff;
  }

  // Take the absolute Y difference, north positive. A southward target flips
  // the 0x40 bit, which moves north-east to south-east and north-west to
  // south-west and so settles the quadrant.
  int ydiff = y1 - y2;
  if (ydiff < 0) {
    index ^= 0x0040;
    ydiff = -ydiff;
  }

  // Find which of the two offsets is bigger. The bigger one (X or Y) tells
  // which axis the direction is closer to.
  int bigger = 0;
  int smaller = 0;
  if (xdiff < ydiff) {
    smaller = xdiff;
    bigger = ydiff;
  } else {
    smaller = ydiff;
    bigger = xdiff;
  }

  // If the smaller offset is at least half the bigger one, call it the
  // diagonal, which lies 0x20 into every quadrant, and bail. Equal points
  // land here too, in the north-east quadrant.
  if ((bigger + 1) / 2 <= smaller) {
    index += 0x0020;
    return static_cast<DirType>(index);
  }

  // Otherwise the direction is the axis at the start of the quadrant or the
  // one at its end, 0x40 further on. The north-east and south-west quadrants
  // start on the Y axis and the other two (0x40 set) on the X axis, so step
  // on when the closer axis is not the one the quadrant starts on. From the
  // north-west quadrant that step wraps to north.
  uint32_t adder = index & 0x0040;
  if (xdiff == bigger) {
    adder ^= 0x0040;
  }
  index += adder;

  return AsDirection(static_cast<int>(index));
}

DirType Desired_Facing256(int srcx, int srcy, int dstx, int dsty) {
  // The direction being built, starting as the first direction of the
  // quadrant exactly as in Desired_Facing8().
  uint32_t composite = 0;

  // Take the absolute X difference. A westward target starts in the
  // north-west quadrant.
  int xdiff = dstx - srcx;
  if (xdiff < 0) {
    composite |= 0x00C0;
    xdiff = -xdiff;
  }

  // Take the absolute Y difference, north positive. A southward target flips
  // the 0x40 bit and so settles the quadrant.
  int ydiff = srcy - dsty;
  if (ydiff < 0) {
    composite ^= 0x0040;
    ydiff = -ydiff;
  }

  // Bail early if the coordinates are the same. This also keeps the division
  // below away from zero.
  if (xdiff == 0 && ydiff == 0) {
    return AsDirection(0xFF);
  }

  // Find which of the two offsets is bigger. The bigger one (X or Y) tells
  // which axis the direction is closer to.
  int bigger = 0;
  int smaller = 0;
  if (xdiff < ydiff) {
    smaller = xdiff;
    bigger = ydiff;
  } else {
    smaller = ydiff;
    bigger = xdiff;
  }

  // Now that the quadrant is known, work out how far from the closer axis
  // the direction lies, as a ratio from 0 (on the axis) to 32 (on the
  // diagonal). This is the linear stand-in for the arctangent.
  //
  // CHECK(a != b), not CHECK_NE: the shallow analyzer does not see through the
  // CHECK_NE helper and would still report the division below.
  CHECK(bigger != 0);
  int frac = smaller * 32 / bigger;

  // `frac` counts from the closer axis. When that is the axis the quadrant
  // ends on (the same test as in Desired_Facing8(), where `adder` is 0x40),
  // count back from it instead. The extra -1 makes that half round toward the
  // start of the quadrant like the other half, and keeps the result inside
  // the quadrant, but it also puts a direction exactly on the end axis, or on
  // the diagonal of a quadrant that starts on the X axis, one step short.
  uint32_t adder = composite & 0x0040;
  if (xdiff > ydiff) {
    adder ^= 0x0040;
  }
  if (adder) {
    frac = static_cast<int>(adder) - frac - 1;
  }

  composite += static_cast<uint32_t>(frac);
  return AsDirection(static_cast<int>(composite));
}

int Dir_To_16(const DirType dir) {
  return kFacing16.at(static_cast<uint8_t>(dir));
}

int Dir_To_32(const DirType dir) {
  return kFacing32.at(static_cast<uint8_t>(dir));
}

}  // namespace game
