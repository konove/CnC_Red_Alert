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

// File: UnitTrackerClass, a player's per-type counts of the units and
// buildings it built, lost, destroyed or captured, for the statistics an
// internet game reports when it ends.
//
// Originally UTRACKER.H by Steve Tall, June 3-7, 1996.

#ifndef CNC_RED_ALERT_GAME_SIM_UTRACKER_H_
#define CNC_RED_ALERT_GAME_SIM_UTRACKER_H_

#include <cstdint>
#include <span>
#include <vector>

#include "absl/base/attributes.h"

namespace game {

// One counter per type of one kind of object (infantry, vehicles, aircraft,
// buildings, vessels, crates), indexed by the type's enum value. Each house of
// an internet game owns a set of them (HouseClass::Init_Trackers()), the game
// counts into them as things are built and destroyed, and the Westwood Online
// statistics packet sends each set as a raw array of 32-bit big-endian
// counts. To_Network_Format() and To_PC_Format() switch the stored counts
// between the two byte orders in place, around the packing of that array.
//
// Example:
//   UnitTrackerClass destroyed(unit_type_count);
//   destroyed.Increment_Unit_Total(static_cast<int>(UNIT_HTANK));
//   destroyed.To_Network_Format();
//   packet.Add_Field("UNK1", std::as_bytes(destroyed.Get_All_Totals()));
class UnitTrackerClass {
 public:
  // Creates `unit_count` counters, all zero, in host byte order.
  explicit UnitTrackerClass(int unit_count);
  ~UnitTrackerClass();
  UnitTrackerClass(const UnitTrackerClass&) = delete;
  UnitTrackerClass& operator=(const UnitTrackerClass&) = delete;
  UnitTrackerClass(UnitTrackerClass&&) = delete;
  UnitTrackerClass& operator=(UnitTrackerClass&&) = delete;

  // Adds or takes one from the count of `unit_type`, which must be below the
  // unit count (std::out_of_range otherwise). Only meaningful while the
  // counts are in host byte order.
  void Increment_Unit_Total(int unit_type);
  void Decrement_Unit_Total(int unit_type);
  // Sets every count to zero. The byte order is left as it was.
  void Clear_Unit_Total();

  // Returns the count of `unit_type`, as stored: byte-swapped while the counts
  // are in network format.
  [[nodiscard]] int Get_Unit_Total(int unit_type) const;
  // Returns every count, one per type and in whichever byte order they are in
  // now; the statistics packet sends these bytes as they are.
  std::span<int32_t> Get_All_Totals() ABSL_ATTRIBUTE_LIFETIME_BOUND;
  // Returns the number of counters: the number of types this tracks.
  [[nodiscard]] int Get_Unit_Count() const { return UnitCount; }

  // Converts the counts to network (big-endian) byte order for sending, and
  // back to host order. Each does nothing when the counts are already in its
  // order, so a pair of calls may be repeated.
  void To_Network_Format();
  void To_PC_Format();

 private:
  // One count per type, indexed by the type's enum value.
  std::vector<int32_t> UnitTotals;
  // The number of counts in UnitTotals.
  int UnitCount;
  // Nonzero while UnitTotals holds network (big-endian) byte order; zero
  // while it holds host order.
  int InNetworkFormat{0};
};

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_SIM_UTRACKER_H_
