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

#ifndef CNC_RED_ALERT_GAME_SIM_FACING_H_
#define CNC_RED_ALERT_GAME_SIM_FACING_H_

#include "game/sim/direction.h"

namespace game {

// The direction something points in, together with the direction it is
// turning toward. Objects keep one for the body and, when they have one, one
// for the turret; callers set the desired direction and call Rotation_Adjust()
// each game frame until Is_Rotating() is false. It reads as its current
// direction, so the turning stays out of the code that only draws or aims.
//
// Example:
//   Heading turret(Direction::kNorth);
//   turret.Set_Desired(Direction::kEast);
//   if (turret.Rotation_Adjust(rate)) {
//     // The turret shows another frame: redraw.
//   }
class Heading {
 public:
  // Points north and is not turning.
  Heading();

  // Points in `dir` and is not turning. Implicit: a heading reads and assigns
  // as its direction.
  // NOLINTNEXTLINE(*-explicit-constructor)
  Heading(Direction dir) : CurrentFacing(dir), DesiredFacing(dir) {}

  // The current direction. Implicit, so a heading compares with a direction
  // and passes where one is expected.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator Direction() const { return CurrentFacing; }

  // The direction pointed in now, and the one being turned toward. They are
  // equal when not turning.
  [[nodiscard]] Direction Current() const { return CurrentFacing; }
  [[nodiscard]] Direction Desired() const { return DesiredFacing; }

  // Saved-game support.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(CurrentFacing, DesiredFacing);
  }

  // Sets the direction to turn toward and leaves the current one alone; the
  // following Rotation_Adjust() calls close the gap. Returns whether the
  // desired direction changed.
  bool Set_Desired(Direction facing);

  // Sets the current direction and leaves the desired one alone, so it turns
  // back if they now differ. Returns whether the current direction changed.
  bool Set_Current(Direction facing);

  // Points in `facing` at once, with nothing left to turn.
  void Set(Direction facing) {
    Set_Current(facing);
    Set_Desired(facing);
  }

  // The current direction, like Current().
  [[nodiscard]] Direction Get() const { return CurrentFacing; }

  // Whether the current direction has yet to reach the desired one.
  [[nodiscard]] bool Is_Rotating() const {
    return DesiredFacing != CurrentFacing;
  }

  // The shortest turn from the current direction to the desired one, or to
  // `facing`: -128..127 direction steps, positive clockwise.
  [[nodiscard]] int Difference() const {
    return ShortestTurn(CurrentFacing, DesiredFacing);
  }
  [[nodiscard]] int Difference(Direction facing) const {
    return ShortestTurn(CurrentFacing, facing);
  }

  // Turns the current direction toward the desired one by `rate` direction
  // steps, the short way round, stopping at the desired direction. Rates above
  // 127 count as 127, which reaches any direction but the exact opposite in
  // one call. Returns whether the turn crossed into another of the 32 body
  // frames, in which case the owner most likely needs to be redrawn.
  bool Rotation_Adjust(int rate);

 private:
  Direction CurrentFacing;
  Direction DesiredFacing;
};

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_SIM_FACING_H_
