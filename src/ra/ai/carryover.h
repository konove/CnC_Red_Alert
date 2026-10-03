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

// The record of one object that survives from a finished scenario into a later
// one: "ToCarryOver" in a scenario's [Basic] section saves its buildings,
// vehicles, infantry and vessels when it ends, and "ToInherit" in a later
// scenario puts them back on its map.

#ifndef CNC_RED_ALERT_RA_AI_CARRYOVER_H_
#define CNC_RED_ALERT_RA_AI_CARRYOVER_H_

#include <cstdint>

#include "ra/core/defines.h"

class ArchiveReader;
class ArchiveWriter;

namespace ra {

class TechnoClass;
class TechnoTypeClass;

// What it takes to recreate one object in a later scenario: its type, owner,
// cell and strength. Everything else (facing, mission, veterancy, cargo) starts
// afresh. The world keeps the list (World::carryover()) and saves it with the
// game, since the inheriting scenario may be several missions away.
//
// Example:
//   TheWorld().carryover().emplace_back(*building);  // as the scenario ends
//   ...
//   for (const auto& object : TheWorld().carryover()) object.Recreate();
class Carryover {
 public:
  // Field-wise saved-game state; read and write share this field list.
  template <class Archive>
  void Serialize(Archive& ar);

  // An empty record, for the saved-game loader to fill.
  Carryover() = default;

  // Records `techno`. A scenario carries over its buildings, vehicles, infantry
  // and vessels, but any kind of techno can be recorded.
  explicit Carryover(const TechnoClass& techno);

  // Builds the recorded object and places it on the current map at its old
  // cell. Nothing is placed when the record is empty, its house is not in this
  // scenario, the object pool is full, or the cell cannot take the object (the
  // new map may differ from the old one).
  void Recreate() const;

 private:
  // The object's type, which also knows how to build one. The type objects live
  // for the whole program, so the pointer outlasts the scenario. nullptr for an
  // empty record.
  const TechnoTypeClass* type_{nullptr};

  // The cell the object stood in on the old map. The inheriting scenario is
  // expected to use a map this still makes sense on.
  Cell cell_{};

  // The object's strength (hit points) when it was recorded, so damage carries
  // over too.
  int16_t strength_{0};

  // The house that owned the object. HOUSE_NONE for an empty record.
  HousesType house_{HOUSE_NONE};
};

extern template void Carryover::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void Carryover::Serialize<ArchiveReader>(ArchiveReader&);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_AI_CARRYOVER_H_
