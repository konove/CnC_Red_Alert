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

#include "ra/ai/carry.h"

#include <cstdint>

#include "base/pooled.h"
#include "ra/core/defines.h"
#include "ra/map/world.h"
#include "ra/objects/building.h"
#include "ra/objects/infantry.h"
#include "ra/objects/techno.h"
#include "ra/objects/type.h"
#include "ra/objects/unit.h"
#include "ra/objects/vessel.h"

namespace ra {

Carryover::Carryover(TechnoClass* techno) {
  if (techno) {
    rtti_ = techno->What_Am_I();

    switch (rtti_) {
      case RTTI_UNIT:
        type_.unit = dynamic_cast<UnitClass*>(techno)->Class->Type;
        break;

      case RTTI_BUILDING:
        type_.building = dynamic_cast<BuildingClass*>(techno)->Class->Type;
        break;

      case RTTI_INFANTRY:
        type_.infantry = dynamic_cast<InfantryClass*>(techno)->Class->Type;
        break;

      case RTTI_VESSEL:
        type_.vessel = dynamic_cast<VesselClass*>(techno)->Class->Type;
        break;

      case RTTIType::RTTI_NONE:
      case RTTIType::RTTI_AIRCRAFT:
      case RTTIType::RTTI_AIRCRAFTTYPE:
      case RTTIType::RTTI_ANIM:
      case RTTIType::RTTI_ANIMTYPE:
      case RTTIType::RTTI_BUILDINGTYPE:
      case RTTIType::RTTI_BULLET:
      case RTTIType::RTTI_BULLETTYPE:
      case RTTIType::RTTI_CELL:
      case RTTIType::RTTI_FACTORY:
      case RTTIType::RTTI_HOUSE:
      case RTTIType::RTTI_HOUSETYPE:
      case RTTIType::RTTI_INFANTRYTYPE:
      case RTTIType::RTTI_OVERLAY:
      case RTTIType::RTTI_OVERLAYTYPE:
      case RTTIType::RTTI_SMUDGE:
      case RTTIType::RTTI_SMUDGETYPE:
      case RTTIType::RTTI_SPECIAL:
      case RTTIType::RTTI_TEAM:
      case RTTIType::RTTI_TEAMTYPE:
      case RTTIType::RTTI_TEMPLATE:
      case RTTIType::RTTI_TEMPLATETYPE:
      case RTTIType::RTTI_TERRAIN:
      case RTTIType::RTTI_TERRAINTYPE:
      case RTTIType::RTTI_TRIGGER:
      case RTTIType::RTTI_TRIGGERTYPE:
      case RTTIType::RTTI_UNITTYPE:
      case RTTIType::RTTI_VESSELTYPE:
      default:
        break;
    }

    house_ = techno->Owner();
    strength_ = techno->Strength;
    cell_ = Cell::Containing(techno->Coord);
  }
}

bool Carryover::Recreate() const {
  TechnoClass* techno = nullptr;

  switch (rtti_) {
    case RTTI_UNIT:
      techno = base::NewPooled<UnitClass>(type_.unit, house_);
      break;

    case RTTI_INFANTRY:
      techno = base::NewPooled<InfantryClass>(type_.infantry, house_);
      break;

    case RTTI_BUILDING:
      techno = base::NewPooled<BuildingClass>(type_.building, house_);
      break;

    case RTTI_VESSEL:
      techno = base::NewPooled<VesselClass>(type_.vessel, house_);
      break;
    case RTTIType::RTTI_NONE:
    case RTTIType::RTTI_AIRCRAFT:
    case RTTIType::RTTI_AIRCRAFTTYPE:
    case RTTIType::RTTI_ANIM:
    case RTTIType::RTTI_ANIMTYPE:
    case RTTIType::RTTI_BUILDINGTYPE:
    case RTTIType::RTTI_BULLET:
    case RTTIType::RTTI_BULLETTYPE:
    case RTTIType::RTTI_CELL:
    case RTTIType::RTTI_FACTORY:
    case RTTIType::RTTI_HOUSE:
    case RTTIType::RTTI_HOUSETYPE:
    case RTTIType::RTTI_INFANTRYTYPE:
    case RTTIType::RTTI_OVERLAY:
    case RTTIType::RTTI_OVERLAYTYPE:
    case RTTIType::RTTI_SMUDGE:
    case RTTIType::RTTI_SMUDGETYPE:
    case RTTIType::RTTI_SPECIAL:
    case RTTIType::RTTI_TEAM:
    case RTTIType::RTTI_TEAMTYPE:
    case RTTIType::RTTI_TEMPLATE:
    case RTTIType::RTTI_TEMPLATETYPE:
    case RTTIType::RTTI_TERRAIN:
    case RTTIType::RTTI_TERRAINTYPE:
    case RTTIType::RTTI_TRIGGER:
    case RTTIType::RTTI_TRIGGERTYPE:
    case RTTIType::RTTI_UNITTYPE:
    case RTTIType::RTTI_VESSELTYPE:
    default:
      break;
  }

  if (techno) {
    // Infantry are placed under the in-game rules rather than the lenient ones
    // that apply while a scenario is being read: with scenario_init set, the
    // check against standing outside the playable map is skipped, and the old
    // cell is not known to be good on this map.
    const int saved_scenario_init = TheWorld().scenario_init();
    techno->Strength = static_cast<int16_t>(strength_);
    if (rtti_ == RTTI_INFANTRY) {
      TheWorld().scenario_init() = 0;
    }
    // TODO: an object that cannot be placed stays in limbo and keeps its pool
    // slot for the rest of the scenario.
    techno->Unlimbo(cell_.Center());
    if (rtti_ == RTTI_INFANTRY) {
      TheWorld().scenario_init() = saved_scenario_init;
    }
  }

  return false;
}

}  // namespace ra
