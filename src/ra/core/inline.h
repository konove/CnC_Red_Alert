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

// Small helpers nearly every Red Alert file uses: coordinate and cell
// geometry, the two random number streams, and the text table lookup.

#ifndef CNC_RED_ALERT_RA_CORE_INLINE_H_
#define CNC_RED_ALERT_RA_CORE_INLINE_H_

#include <cstdint>
#include <cstdlib>
#include <iterator>

#include "base/array.h"
#include "base/random.h"
#include "engine/file/string_table.h"
#include "game/sim/direction.h"
#include "ra/ai/scenario.h"
#include "ra/assets/assets.h"
#include "ra/core/const.h"
#include "ra/core/coord.h"
#include "ra/core/defines.h"
#include "ra/core/packed.h"  // IWYU pragma: export
#include "ra/map/display.h"
#include "ra/map/world.h"

namespace ra {

// Source of random numbers for events that must NOT affect game logic, such
// as cosmetic animations and one-machine-only effects. Because it never
// influences the simulation, it does not need to stay in sync across
// networked machines, and it is not saved. For sync-critical randomness use
// TheScenario().sync_rng_.
inline RandomClass& LocalRandom() {
  static RandomClass rng;
  return rng;
}

// Returns the coordinate `x` and `y` pixels right of and below the map's upper
// left corner, each rounded to the nearest lepton. Mouse positions come
// through here; the caller adds the tactical view's own coordinate to get a
// position in the world.
inline Coordinate XYP_Coord(int x, int y) noexcept {
  return Coordinate::FromLeptons(Pixel_To_Lepton(x), Pixel_To_Lepton(y));
}

// Returns the direction from `coord1` to `coord2` at the full 256-step
// resolution. Same as Direction().
inline game::DirType Direction256(Coordinate coord1, Coordinate coord2) {
  return game::Desired_Facing256(coord1.x(), coord1.y(), coord2.x(),
                                 coord2.y());
}

// Returns the direction from `coord1` to `coord2` at the full 256-step
// resolution.
inline game::DirType Direction(Coordinate coord1, Coordinate coord2) {
  return game::Desired_Facing256(coord1.x(), coord1.y(), coord2.x(),
                                 coord2.y());
}

// Returns the direction from `coord1` to `coord2` as one of the eight compass
// points, which is cheaper to work out than Direction().
inline game::DirType Direction8(Coordinate coord1, Coordinate coord2) {
  return game::Desired_Facing8(coord1.x(), coord1.y(), coord2.x(), coord2.y());
}

// Returns the center of the cell next to the one `coord` lies in, in direction
// `dir`. A `dir` outside the eight facings, FACING_NONE included, wraps into
// them. No map edge check: the result may lie off the map.
inline Coordinate Adjacent_Cell(Coordinate coord, game::FacingType dir) {
  return (AdjacentCoord.at(game::AsFacing(static_cast<int>(dir))) + coord)
      .Snapped();
}

// As above, with `dir` rounded to the nearest of the eight facings.
inline Coordinate Adjacent_Cell(Coordinate coord, game::DirType dir) {
  return Adjacent_Cell(coord, game::Dir_Facing(dir));
}

// Returns the cell next to `cell` in direction `dir`. A `dir` outside the eight
// facings, FACING_NONE included, wraps into them. No map edge check: stepping
// off the east or west edge lands on the opposite edge of a neighbouring row.
inline Cell Adjacent_Cell(Cell cell, game::FacingType dir) {
  // Wrapped so that FACING_NONE (-1) does not index before the start of the
  // table.
  return cell.Offset(AdjacentCell.at(game::AsFacing(static_cast<int>(dir))));
}

// As above, with `dir` rounded to the nearest of the eight facings.
inline Cell Adjacent_Cell(Cell cell, game::DirType dir) {
  return cell.Offset(AdjacentCell.at(game::Dir_Facing(dir)));
}

// Returns the text for the text number `index`, or nullptr when the table has
// no such entry. Three sources share the number space:
//   below 0     the scenario's name overrides, slot -index - 1, as
//               AbstractTypeClass::Full_Name() hands them out
//   0..999      the game's text table (the TXT_* constants)
//   1000 and up the developer text table, entry index - 1000
// The pointer is into the loaded table or the override and stays valid until
// the next scenario starts.
inline const char* Text_String(int index) {
  if (index < 0 && -index <= std::ssize(TheWorld().name_override())) {
    return base::At(std::span(TheWorld().name_override()), -index - 1).c_str();
  }

  // The developer table's numbers start at 1000 so they cannot collide with
  // the TXT_* constants.
  if (index < 1000) {
    return Extract_String(TheAssets().system_strings(), index).data();
  }
  return Extract_String(TheAssets().debug_strings(), index - 1000).data();
}

// Returns a random value in [a, b] from the synchronized generator. Every
// machine in a multiplayer game must make the same calls in the same order, so
// use it for whatever affects the simulation, and only for that. `T` is an
// integer or an enum; the range must span less than 32768 values.
template <class T>
T Random_Pick(T a, T b) {
  return static_cast<T>(TheScenario().sync_rng_.InRange(static_cast<int>(a),
                                                        static_cast<int>(b)));
}

// Returns true `percent` times out of 100, drawing from the synchronized
// generator like Random_Pick().
inline bool Percent_Chance(int percent) {
  // The draw is 0..99, so 0 never succeeds and 100 always does.
  return TheScenario().sync_rng_.InRange(0, 99) < percent;
}

// Returns a random value in [a, b] from LocalRandom(), leaving the
// synchronized generator alone. For anything that differs between machines or
// is only cosmetic: animations, sounds, what one player's screen shows.
template <class T>
T Sim_Random_Pick(T a, T b) {
  return static_cast<T>(
      LocalRandom().InRange(static_cast<int>(a), static_cast<int>(b)));
}

// Returns true `percent` times out of 100, drawing from LocalRandom() like
// Sim_Random_Pick().
inline bool Sim_Percent_Chance(int percent) {
  return LocalRandom().InRange(0, 99) < percent;
}

// Returns the approximate distance between two points, in the units they are
// given in: the longer axis difference plus half the shorter one (the "Dragon
// Strike" method). Exact along an axis and about 6% long on a diagonal, with
// no multiply or square root.
inline int Distance(int x1, int y1, int x2, int y2) {
  int diff1 = y1 - y2;
  if (diff1 < 0) {
    diff1 = -diff1;
  }
  int diff2 = x1 - x2;
  if (diff2 < 0) {
    diff2 = -diff2;
  }
  if (diff1 > diff2) {
    return diff1 + (diff2 / 2);
  }
  return diff2 + (diff1 / 2);
}

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_CORE_INLINE_H_
