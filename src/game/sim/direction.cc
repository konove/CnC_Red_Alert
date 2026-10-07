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

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>

#include "base/numeric.h"

namespace game {
namespace {

// The body frame for each of the 256 directions. A simple shift won't do:
// a direction converts to the CLOSEST frame, not the one below it, so each
// frame's entries straddle its exact direction and the last few wrap to 0.
// The frames are uneven, between six and eleven directions each, to match the
// angles the vehicle shapes were actually rendered at.
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

// The part of a direction the two point functions work out the same way.
struct QuadrantOffset {
  // The first direction of the quadrant the target lies in: 0x00 north-east,
  // 0x40 south-east, 0x80 south-west, 0xC0 north-west.
  int quadrant = 0;
  // The absolute X and Y offsets to the target, sorted. The bigger one tells
  // which axis the direction is closer to.
  int smaller = 0;
  int bigger = 0;
  // Whether that closer axis is the one the quadrant ends on, 0x40 past its
  // start, rather than the one it starts on. The north-east and south-west
  // quadrants start on the Y axis and the other two on the X axis. A target
  // exactly on the diagonal counts as closer to the Y axis.
  bool closer_to_end_axis = false;
};

QuadrantOffset QuadrantOffsetToward(const int from_x, const int from_y,
                                    const int to_x, const int to_y) {
  const int x_offset = to_x - from_x;
  // (0, 0) is the north-west corner, so north is the negative Y direction.
  const int y_offset = to_y - from_y;
  const bool west = x_offset < 0;
  const bool south = y_offset > 0;
  const int x_distance = std::abs(x_offset);
  const int y_distance = std::abs(y_offset);
  const bool starts_on_x_axis = west != south;
  const auto [smaller, bigger] = std::minmax(x_distance, y_distance);
  // The quadrants run clockwise, so a southward target is one quadrant on from
  // the north-east one and one back from the north-west one.
  const int south_step = south ? 0x40 : 0;
  return {.quadrant = west ? 0xC0 - south_step : south_step,
          .smaller = smaller,
          .bigger = bigger,
          .closer_to_end_axis = starts_on_x_axis != (x_distance > y_distance)};
}

}  // namespace

Direction Direction8Toward(const int from_x, const int from_y, const int to_x,
                           const int to_y) {
  const auto [quadrant, smaller, bigger, closer_to_end_axis] =
      QuadrantOffsetToward(from_x, from_y, to_x, to_y);

  // If the smaller offset is at least half the bigger one, call it the
  // diagonal, which lies 0x20 into every quadrant, and bail. Equal points
  // land here too, in the north-east quadrant.
  if ((bigger + 1) / 2 <= smaller) {
    return Direction(quadrant + 0x20);
  }

  // Otherwise it is one of the quadrant's two axes. From the north-west
  // quadrant the end axis wraps to north.
  return Direction(quadrant + (closer_to_end_axis ? 0x40 : 0));
}

Direction Direction256Toward(const int from_x, const int from_y, const int to_x,
                             const int to_y) {
  const auto [quadrant, smaller, bigger, closer_to_end_axis] =
      QuadrantOffsetToward(from_x, from_y, to_x, to_y);

  // Bail early if the coordinates are the same. This also keeps the division
  // below away from zero.
  if (bigger == 0) {
    return Direction(0xFF);
  }

  // How far from the closer axis the direction lies, as a ratio from 0 (on
  // the axis) to 32 (on the diagonal). This is the linear stand-in for the
  // arctangent.
  const int scaled = smaller * 32;
  if (!closer_to_end_axis) {
    return Direction(quadrant + (scaled / bigger));
  }

  // The ratio counts from the closer axis, and here that is the axis the
  // quadrant ends on, so count back from it. Rounding the ratio up makes this
  // half round toward the start of the quadrant like the other half. The
  // original subtracted one from the rounded-down ratio instead, which is the
  // same thing except when the division is exact: then it landed one step
  // short, due east and due south included.
  const int rounded_up = (scaled + bigger - 1) / bigger;
  return Direction(quadrant + 0x40 - rounded_up);
}

int BodyFrame32(const Direction dir) {
  return kBodyFrame32.at(base::ToSize(dir.value()));
}

}  // namespace game
