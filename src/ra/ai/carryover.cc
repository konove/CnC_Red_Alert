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

#include "base/pooled.h"
#include "ra/core/defines.h"
#include "ra/map/world.h"
#include "ra/objects/house.h"
#include "ra/objects/object.h"
#include "ra/objects/techno.h"
#include "ra/objects/type.h"

namespace ra {

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
  ObjectClass* const object = type_->Create_One_Of(house);
  if (object == nullptr) {
    return;
  }

  // Infantry are placed under the in-game rules rather than the lenient ones
  // that apply while a scenario is being read: with scenario_init set, the
  // check against standing outside the playable map is skipped, and the old
  // cell is not known to be good on this map.
  const bool is_infantry = object->What_Am_I() == RTTI_INFANTRY;
  const int saved_scenario_init = TheWorld().scenario_init();
  object->Strength = strength_;
  if (is_infantry) {
    TheWorld().scenario_init() = 0;
  }
  const bool placed = object->Unlimbo(cell_.Center());
  if (is_infantry) {
    TheWorld().scenario_init() = saved_scenario_init;
  }
  // The old cell may be off this map or occupied. An object left in limbo
  // would hold its pool slot for the rest of the scenario.
  if (!placed) {
    base::DeletePooled(object);
  }
}

}  // namespace ra
