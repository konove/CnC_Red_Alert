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

// The direction between two points.
//
// Originally FACE.CPP by Joe L. Bostic, started March 8, 1996.

#include "game/sim/direction.h"

#include <algorithm>
#include <cstdlib>

namespace game {
namespace {

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

QuadrantOffset QuadrantOffsetToward(const base::Point from,
                                    const base::Point to) {
  const int x_offset = to.x - from.x;
  // (0, 0) is the north-west corner, so north is the negative Y direction.
  const int y_offset = to.y - from.y;
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

Direction Direction8Toward(const base::Point from, const base::Point to) {
  const auto [quadrant, smaller, bigger, closer_to_end_axis] =
      QuadrantOffsetToward(from, to);

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

Direction Direction256Toward(const base::Point from, const base::Point to) {
  const auto [quadrant, smaller, bigger, closer_to_end_axis] =
      QuadrantOffsetToward(from, to);

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

}  // namespace game
