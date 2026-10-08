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

// A direction that turns: where an object or a turret points now, where it
// should end up, and the step that brings the first toward the second.
//
// Originally FACING.H by Joe L. Bostic, started March 21, 1995.

#ifndef CNC_RED_ALERT_GAME_SIM_HEADING_H_
#define CNC_RED_ALERT_GAME_SIM_HEADING_H_

#include "game/sim/direction.h"

namespace game {

// The direction something points in, together with the direction it is
// turning toward. Objects keep one for the body and, when they have one, one
// for the turret; callers set the desired direction and call Turn() each game
// frame until IsRotating() is false. It reads as its current direction, so the
// turning stays out of the code that only draws or aims.
//
// Example:
//   Heading turret(Direction::kNorth);
//   turret.set_desired(Direction::kEast);
//   if (turret.Turn(turn_rate)) {
//     // The turret shows another frame: redraw.
//   }
class Heading {
 public:
  // Points north and is not turning.
  constexpr Heading() = default;

  // Points in `direction` and is not turning. Explicit: to point an existing
  // heading somewhere at once, call SnapTo().
  constexpr explicit Heading(Direction direction)
      : current_(direction), desired_(direction) {}

  // The current direction. Implicit, so a heading compares with a direction
  // and passes where one is expected.
  // NOLINTNEXTLINE(*-explicit-constructor)
  constexpr operator Direction() const { return current_; }

  // The direction pointed in now, and the one being turned toward. They are
  // equal when not turning.
  [[nodiscard]] Direction current() const { return current_; }
  [[nodiscard]] Direction desired() const { return desired_; }

  // Saved-game support.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(current_, desired_);
  }

  // Sets the direction to turn toward and leaves the current one alone; the
  // following Turn() calls close the gap.
  void set_desired(const Direction direction) { desired_ = direction; }

  // Sets the current direction and leaves the desired one alone, so it turns
  // back if they now differ.
  void set_current(const Direction direction) { current_ = direction; }

  // Points in `direction` at once, with nothing left to turn.
  void SnapTo(const Direction direction) {
    current_ = direction;
    desired_ = direction;
  }

  // Whether the current direction has yet to reach the desired one.
  [[nodiscard]] bool IsRotating() const { return desired_ != current_; }

  // The shortest turn from the current direction to the desired one, or to
  // `direction`: -128..127 direction steps, positive clockwise.
  [[nodiscard]] int RemainingTurn() const {
    return ShortestTurn(current_, desired_);
  }
  [[nodiscard]] int ShortestTurnTo(Direction direction) const {
    return ShortestTurn(current_, direction);
  }

  // Turns the current direction toward the desired one by up to `max_steps`
  // direction steps, the short way round, stopping at the desired direction.
  // More than 127 steps count as 127, which reaches any direction but the
  // exact opposite in one call. Returns whether the turn crossed into another
  // of the 32 body frames, in which case the owner most likely needs to be
  // redrawn.
  bool Turn(int max_steps);

 private:
  Direction current_;
  Direction desired_;
};

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_SIM_HEADING_H_
