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

#include "game/sim/facing.h"

#include <algorithm>
#include <cstdlib>

#include "game/sim/direction.h"

namespace game {

Heading::Heading()
    : CurrentFacing(Direction::kNorth), DesiredFacing(Direction::kNorth) {}

bool Heading::Set_Desired(Direction facing) {
  if (DesiredFacing != facing) {
    DesiredFacing = facing;
    return true;
  }
  return false;
}

bool Heading::Set_Current(Direction facing) {
  if (CurrentFacing != facing) {
    CurrentFacing = facing;
    return true;
  }
  return false;
}

bool Heading::Rotation_Adjust(int rate) {
  if (Is_Rotating()) {
    // A turn is at most half the circle, so a faster rate adds nothing.
    rate = std::min(rate, 127);

    const Direction oldfacing = CurrentFacing;
    const int diff = Difference();

    // Snap to the desired direction when the rate covers the gap, so the turn
    // never overshoots.
    if (std::abs(diff) < rate) {
      CurrentFacing = DesiredFacing;
    } else {
      // Otherwise step the short way round: anticlockwise for a negative
      // difference.
      if (diff < 0) {
        CurrentFacing = CurrentFacing - Direction(rate);
      } else {
        CurrentFacing = CurrentFacing + Direction(rate);
      }
    }

    // Only a change of body frame is visible, and so worth a redraw.
    return BodyFrame32(CurrentFacing) != BodyFrame32(oldfacing);
  }
  return false;
}

}  // namespace game
