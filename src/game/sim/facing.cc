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

Heading::Heading() : current_(Direction::kNorth), desired_(Direction::kNorth) {}

bool Heading::set_desired(Direction direction) {
  if (desired_ != direction) {
    desired_ = direction;
    return true;
  }
  return false;
}

bool Heading::set_current(Direction direction) {
  if (current_ != direction) {
    current_ = direction;
    return true;
  }
  return false;
}

bool Heading::Turn(int max_steps) {
  if (IsRotating()) {
    // A turn is at most half the circle, so a larger step adds nothing.
    max_steps = std::min(max_steps, 127);

    const Direction previous = current_;
    const int turn = RemainingTurn();

    // Snap to the desired direction when the step covers the gap, so the turn
    // never overshoots.
    if (std::abs(turn) < max_steps) {
      current_ = desired_;
    } else {
      // Otherwise step the short way round: anticlockwise for a negative
      // difference.
      if (turn < 0) {
        current_ = current_ - Direction(max_steps);
      } else {
        current_ = current_ + Direction(max_steps);
      }
    }

    // Only a change of body frame is visible, and so worth a redraw.
    return BodyFrame32(current_) != BodyFrame32(previous);
  }
  return false;
}

}  // namespace game
