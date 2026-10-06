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

/* $Header: /CounterStrike/TARGET.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : TARGET.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : September 10, 1993 *
 *                                                                                             *
 *                  Last Update : July 16, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:
 *   Target::Of -- The target of an object, type or cell pointer.
 *   Target::AsObject -- Converts a target into a live object pointer.
 *   Target::AsTechno -- Converts a target into a live techno pointer.
 *   Target::AsObjectInSlot -- Converts a target into its object heap slot.
 *   Target::AsTechnoInSlot -- Converts a target into its techno heap slot.
 *   Target::AsAircraft, AsAnimation, ... -- Single-kind heap slot decoders.
 *   Target::AsTechnoType -- Converts a target into a techno type pointer.
 *   Target::AsCoordinate -- Converts a target into a coordinate.
 *   Target::AsCell -- Converts a target into a cell.
 *   Target::AsMovementCoordinate -- Where a unit moving to the target heads.
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/core/target.h"

#include <cstdint>

#include "absl/log/check.h"
#include "game/core/object_heap.h"
#include "ra/ai/team.h"
#include "ra/ai/teamtype.h"
#include "ra/ai/trigger.h"
#include "ra/ai/trigger_type.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/map/world.h"
#include "ra/objects/abstract.h"
#include "ra/objects/aircraft.h"
#include "ra/objects/anim.h"
#include "ra/objects/building.h"
#include "ra/objects/bullet.h"
#include "ra/objects/infantry.h"
#include "ra/objects/object.h"
#include "ra/objects/techno.h"
#include "ra/objects/terrain.h"  // IWYU pragma: keep
#include "ra/objects/type.h"
#include "ra/objects/unit.h"
#include "ra/objects/vessel.h"

namespace ra {

Target Target::Of(const AbstractClass* object) {
  return object != nullptr ? Make(object->RTTI, object->ID) : Target{};
}

ObjectClass* Target::AsObject() const {
  const int val = value();
  ObjectClass* object = nullptr;
  switch (kind()) {
    case RTTI_INFANTRY:
      object = TheObjectHeaps().infantry().Slot(val);
      break;

    case RTTI_UNIT:
      object = TheObjectHeaps().unit().Slot(val);
      break;

    case RTTI_VESSEL:
      object = TheObjectHeaps().vessel().Slot(val);
      break;

    case RTTI_BUILDING:
      object = TheObjectHeaps().building().Slot(val);
      break;

    case RTTI_AIRCRAFT:
      object = TheObjectHeaps().aircraft().Slot(val);
      break;

    case RTTI_TERRAIN:
      object = TheObjectHeaps().terrain().Slot(val);
      break;

    case RTTI_BULLET:
      object = TheObjectHeaps().bullet().Slot(val);
      break;

    case RTTI_ANIM:
      object = TheObjectHeaps().anim().Slot(val);
      break;

    case RTTIType::RTTI_NONE:
    case RTTIType::RTTI_AIRCRAFTTYPE:
    case RTTIType::RTTI_ANIMTYPE:
    case RTTIType::RTTI_BUILDINGTYPE:
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
    case RTTIType::RTTI_TERRAINTYPE:
    case RTTIType::RTTI_TRIGGER:
    case RTTIType::RTTI_TRIGGERTYPE:
    case RTTIType::RTTI_UNITTYPE:
    case RTTIType::RTTI_VESSELTYPE:
    default:
      break;
  }

  // A network event can name an object that died between the frame it was
  // sent and the frame it runs; the slot then holds an inactive object,
  // which must not come back as a pointer.
  if (object != nullptr && !object->IsActive) {
    object = nullptr;
  }

  return object;
}

TechnoClass* Target::AsTechno() const {
  ObjectClass* object = AsObject();
  if (object != nullptr && object->Is_Techno()) {
    return dynamic_cast<TechnoClass*>(object);
  }
  return nullptr;
}

ObjectClass* Target::AsObjectInSlot() const {
  const int val = value();
  switch (kind()) {
    case RTTI_TERRAIN:
      return TheObjectHeaps().terrain().Slot(val);

    case RTTI_SMUDGE:
      return TheObjectHeaps().smudge().Slot(val);

    case RTTI_OVERLAY:
      return TheObjectHeaps().overlay().Slot(val);

    case RTTI_BULLET:
      return TheObjectHeaps().bullet().Slot(val);

    case RTTI_ANIM:
      return TheObjectHeaps().anim().Slot(val);

    case RTTI_UNIT:
      return TheObjectHeaps().unit().Slot(val);

    case RTTI_VESSEL:
      return TheObjectHeaps().vessel().Slot(val);

    case RTTI_BUILDING:
      return TheObjectHeaps().building().Slot(val);

    case RTTI_INFANTRY:
      return TheObjectHeaps().infantry().Slot(val);

    case RTTI_AIRCRAFT:
      return TheObjectHeaps().aircraft().Slot(val);

    case RTTIType::RTTI_NONE:
    case RTTIType::RTTI_AIRCRAFTTYPE:
    case RTTIType::RTTI_ANIMTYPE:
    case RTTIType::RTTI_BUILDINGTYPE:
    case RTTIType::RTTI_BULLETTYPE:
    case RTTIType::RTTI_CELL:
    case RTTIType::RTTI_FACTORY:
    case RTTIType::RTTI_HOUSE:
    case RTTIType::RTTI_HOUSETYPE:
    case RTTIType::RTTI_INFANTRYTYPE:
    case RTTIType::RTTI_OVERLAYTYPE:
    case RTTIType::RTTI_SMUDGETYPE:
    case RTTIType::RTTI_SPECIAL:
    case RTTIType::RTTI_TEAM:
    case RTTIType::RTTI_TEAMTYPE:
    case RTTIType::RTTI_TEMPLATE:
    case RTTIType::RTTI_TEMPLATETYPE:
    case RTTIType::RTTI_TERRAINTYPE:
    case RTTIType::RTTI_TRIGGER:
    case RTTIType::RTTI_TRIGGERTYPE:
    case RTTIType::RTTI_UNITTYPE:
    case RTTIType::RTTI_VESSELTYPE:
    default:
      break;
  }
  return nullptr;
}

TechnoClass* Target::AsTechnoInSlot() const {
  const int val = value();
  switch (kind()) {
    case RTTI_UNIT:
      return TheObjectHeaps().unit().Slot(val);

    case RTTI_VESSEL:
      return TheObjectHeaps().vessel().Slot(val);

    case RTTI_BUILDING:
      return TheObjectHeaps().building().Slot(val);

    case RTTI_INFANTRY:
      return TheObjectHeaps().infantry().Slot(val);

    case RTTI_AIRCRAFT:
      return TheObjectHeaps().aircraft().Slot(val);

    case RTTIType::RTTI_NONE:
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
  return nullptr;
}

AircraftClass* Target::AsAircraft() const {
  return kind() == RTTI_AIRCRAFT ? TheObjectHeaps().aircraft().Slot(value())
                                 : nullptr;
}

AnimClass* Target::AsAnimation() const {
  return kind() == RTTI_ANIM ? TheObjectHeaps().anim().Slot(value()) : nullptr;
}

BuildingClass* Target::AsBuilding() const {
  return kind() == RTTI_BUILDING ? TheObjectHeaps().building().Slot(value())
                                 : nullptr;
}

BulletClass* Target::AsBullet() const {
  return kind() == RTTI_BULLET ? TheObjectHeaps().bullet().Slot(value())
                               : nullptr;
}

InfantryClass* Target::AsInfantry() const {
  return kind() == RTTI_INFANTRY ? TheObjectHeaps().infantry().Slot(value())
                                 : nullptr;
}

TeamClass* Target::AsTeam() const {
  return kind() == RTTI_TEAM ? TheObjectHeaps().team().Slot(value()) : nullptr;
}

TeamType* Target::AsTeamType() const {
  return kind() == RTTI_TEAMTYPE ? TheObjectHeaps().team_type().Slot(value())
                                 : nullptr;
}

TriggerClass* Target::AsTrigger() const {
  return kind() == RTTI_TRIGGER ? TheObjectHeaps().trigger().Slot(value())
                                : nullptr;
}

TriggerType* Target::AsTriggerType() const {
  return kind() == RTTI_TRIGGERTYPE
             ? TheObjectHeaps().trigger_type().Slot(value())
             : nullptr;
}

UnitClass* Target::AsUnit() const {
  return kind() == RTTI_UNIT ? TheObjectHeaps().unit().Slot(value()) : nullptr;
}

VesselClass* Target::AsVessel() const {
  return kind() == RTTI_VESSEL ? TheObjectHeaps().vessel().Slot(value())
                               : nullptr;
}

const TechnoTypeClass* Target::AsTechnoType() const {
  const int val = value();
  switch (kind()) {
    case RTTI_INFANTRYTYPE:
      return &InfantryTypeClass::As_Reference(static_cast<InfantryType>(val));

    case RTTI_UNITTYPE:
      return &UnitTypeClass::As_Reference(static_cast<UnitType>(val));

    case RTTI_VESSELTYPE:
      return &VesselTypeClass::As_Reference(static_cast<VesselType>(val));

    case RTTI_AIRCRAFTTYPE:
      return &AircraftTypeClass::As_Reference(static_cast<AircraftType>(val));

    case RTTI_BUILDINGTYPE:
      return &BuildingTypeClass::As_Reference(static_cast<StructType>(val));

    case RTTIType::RTTI_NONE:
    case RTTIType::RTTI_AIRCRAFT:
    case RTTIType::RTTI_ANIM:
    case RTTIType::RTTI_ANIMTYPE:
    case RTTIType::RTTI_BUILDING:
    case RTTIType::RTTI_BULLET:
    case RTTIType::RTTI_BULLETTYPE:
    case RTTIType::RTTI_CELL:
    case RTTIType::RTTI_FACTORY:
    case RTTIType::RTTI_HOUSE:
    case RTTIType::RTTI_HOUSETYPE:
    case RTTIType::RTTI_INFANTRY:
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
    case RTTIType::RTTI_UNIT:
    case RTTIType::RTTI_VESSEL:
    default:
      break;
  }
  return nullptr;
}

// The original comments call a cell target's value "the cell index number",
// but this reads the sub-cell encoding, as the code always has.
Coordinate Target::AsCoordinate() const {
  if (!*this) {
    return Coordinate{};
  }
  if (kind() == RTTI_CELL) {
    const auto v = static_cast<uint32_t>(value());
    const uint32_t x = (v & kSubCellAxisMask) * kLeptonsPerUnit;
    const uint32_t y =
        ((v >> kSubCellAxisBits) & kSubCellAxisMask) * kLeptonsPerUnit;
    return Coordinate::FromLeptons(static_cast<int>(x + (kLeptonsPerUnit / 2)),
                                   static_cast<int>(y + (kLeptonsPerUnit / 2)));
  }
  const ObjectClass* object = AsObject();
  if (object != nullptr) {
    DCHECK(object->IsActive);
    return object->Target_Coord();
  }
  return Coordinate{};
}

Cell Target::AsCell() const { return Cell::Containing(AsCoordinate()); }

// Reads a cell target as a cell index, where AsCoordinate() reads the
// sub-cell encoding; see the class comment.
Coordinate Target::AsMovementCoordinate() const {
  if (!*this) {
    return Coordinate{};
  }
  if (kind() == RTTI_CELL) {
    return cell_index().Center();
  }
  const ObjectClass* object = AsObject();
  if (object != nullptr) {
    return object->Docking_Coord();
  }
  return Coordinate{};
}

}  // namespace ra
