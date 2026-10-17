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

// File: TypeTally, a player's per-type counts of the units and
// buildings it built, lost, destroyed or captured, for the statistics an
// internet game reports when it ends.
//
// Originally UTRACKER.H by Steve Tall, June 3-7, 1996.

#ifndef CNC_RED_ALERT_GAME_SIM_TYPE_TALLY_H_
#define CNC_RED_ALERT_GAME_SIM_TYPE_TALLY_H_

#include <cstdint>
#include <span>
#include <vector>

#include "absl/base/attributes.h"
#include "base/types.h"

namespace game {

// One counter per type of one kind of object (infantry, vehicles, aircraft,
// buildings, vessels, crates), indexed by the type's enum value. Each house of
// an internet game owns a set of them (HouseClass::Init_Trackers()), the game
// counts into them as things are built and destroyed, and the Westwood Online
// statistics packet sends each set's counts().
//
// Example:
//   TypeTally destroyed(unit_type_count);
//   destroyed.Increment(static_cast<int>(UNIT_HTANK));
//   packet.Add_Field("UNK1", destroyed.counts());
class TypeTally {
 public:
  // Creates `type_count` counters, all zero.
  explicit TypeTally(base::ssize type_count);

  // Adds one to the count of `type`, which must be below the type count
  // (std::out_of_range otherwise).
  void Increment(int type);

  // Returns the count of `type`.
  [[nodiscard]] int Count(int type) const;
  // Returns every count, in type order.
  [[nodiscard]] std::span<const int32_t> counts() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return counts_;
  }

 private:
  // One count per type, indexed by the type's enum value.
  std::vector<int32_t> counts_;
};

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_SIM_TYPE_TALLY_H_
