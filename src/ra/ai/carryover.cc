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

#include "ra/ai/carryover.h"

#include <cstdint>
#include <span>

#include "base/array.h"
#include "base/pooled.h"
#include "ra/core/cell.h"
#include "ra/core/defines.h"
#include "ra/map/world.h"
#include "ra/objects/cell.h"
#include "ra/objects/house.h"
#include "ra/objects/object.h"
#include "ra/objects/techno.h"
#include "ra/objects/type.h"

namespace ra {
namespace {

// Whether every cell an object of `type` standing in `cell` occupies is on the
// playable map and holds no object. The terrain is not checked: a scenario may
// put its objects on ground the game would not let them be built on or driven
// to, and those must come back.
bool IsFootprintFree(const TechnoTypeClass& type, const Cell cell) {
  std::span<const int16_t> offsets = type.Occupy_List(true);
  while (!offsets.empty() && offsets.front() != kRefreshEol) {
    const Cell occupied = cell.Offset(base::ConsumeFront(offsets));
    if (!TheMap().In_Radar(occupied) ||
        TheMap().at(occupied).Cell_Occupier() != nullptr) {
      return false;
    }
  }
  return true;
}

}  // namespace

Carryover::Carryover(const TechnoClass& techno)
    : type_(techno.Techno_Type_Class()),
      cell_(Cell::Containing(techno.Coord)),
      strength_(techno.Strength),
      house_(techno.Owner()) {}

void Carryover::Recreate() const {
  // The type's factories read the house, and the new scenario need not have
  // the one the object belonged to.
  HouseClass* const house = HouseClass::As_Pointer(house_);
  if (type_ == nullptr || house == nullptr) {
    return;
  }
  // While a scenario is being read (scenario_init set), Unlimbo() puts an
  // object down without asking whether its cell can take it, and the old cell
  // is not known to be good on this map. Infantry share a cell, so they are
  // placed under the in-game rules instead, which find a free spot in the cell
  // and refuse one off the map; every other object needs its whole footprint
  // free.
  const bool is_infantry = type_->What_Am_I() == RTTI_INFANTRYTYPE;
  if (!is_infantry && !IsFootprintFree(*type_, cell_)) {
    return;
  }
  ObjectClass* const object = type_->Create_One_Of(house);
  if (object == nullptr) {
    return;
  }

  const int saved_scenario_init = TheWorld().scenario_init();
  object->Strength = strength_;
  if (is_infantry) {
    TheWorld().scenario_init() = 0;
  }
  const bool placed = object->Unlimbo(cell_.Center());
  if (is_infantry) {
    TheWorld().scenario_init() = saved_scenario_init;
  }
  // An object left in limbo would hold its pool slot for the rest of the
  // scenario.
  if (!placed) {
    base::DeletePooled(object);
  }
}

}  // namespace ra
