/*
**	Command & Conquer(tm)
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

/* $Header:   F:\projects\c&c\vcs\code\target.cpv   2.17   16 Oct 1995 16:51:06
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : target.cpp *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : September 10, 1993 *
 *                                                                                             *
 *                  Last Update : August 27, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:
 *   Target::AsObject -- Converts a target into a live object pointer.
 *   Target::AsTechno -- Converts a target into a live techno pointer.
 *   Target::AsAircraft, AsBuilding, AsUnit -- Single-kind heap slot decoders.
 *   Target::AsCell -- Converts a target into a cell.
 *   Target::AsCoordinate -- Converts a target into a coordinate.
 *   Target::AsMovementCoordinate -- Where a unit moving to the target heads.
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/core/target.h"

#include "game/core/object_heap.h"
#include "td/core/cell.h"
#include "td/core/coordinate.h"
#include "td/core/object_heaps.h"
#include "td/objects/aircraft.h"
#include "td/objects/anim.h"  // IWYU pragma: keep
#include "td/objects/building.h"
#include "td/objects/bullet.h"    // IWYU pragma: keep
#include "td/objects/infantry.h"  // IWYU pragma: keep
#include "td/objects/object.h"
#include "td/objects/techno.h"
#include "td/objects/terrain.h"  // IWYU pragma: keep
#include "td/objects/unit.h"

namespace td {

ObjectClass* Target::AsObject() const {
  const int val = value();
  ObjectClass* object = nullptr;

  switch (kind()) {
    case KIND_INFANTRY:
      object = TheObjectHeaps().infantry().Slot(val);
      break;

    case KIND_UNIT:
      object = TheObjectHeaps().unit().Slot(val);
      break;

    case KIND_BUILDING:
      object = TheObjectHeaps().building().Slot(val);
      break;

    case KIND_AIRCRAFT:
      object = TheObjectHeaps().aircraft().Slot(val);
      break;

    case KIND_TERRAIN:
      object = TheObjectHeaps().terrain().Slot(val);
      break;

    case KIND_BULLET:
      object = TheObjectHeaps().bullet().Slot(val);
      break;

    case KIND_ANIMATION:
      object = TheObjectHeaps().anim().Slot(val);
      break;

    case KindType::KIND_NONE:
    case KindType::KIND_CELL:
    case KindType::KIND_TEMPLATE:
    case KindType::KIND_TRIGGER:
    case KindType::KIND_TEAM:
    case KindType::KIND_TEAMTYPE:
    default:
      break;
  }

  // A target can outlive its object: in a network game the object may be
  // destroyed between sending an event and receiving it. A dead slot is no
  // object.
  if (object != nullptr && !object->IsActive) {
    object = nullptr;
  }

  return object;
}

TechnoClass* Target::AsTechno() const {
  ObjectClass* obj = AsObject();
  if (obj && obj->Is_Techno()) {
    return dynamic_cast<TechnoClass*>(obj);
  }
  return nullptr;
}

AircraftClass* Target::AsAircraft() const {
  return kind() == KIND_AIRCRAFT ? TheObjectHeaps().aircraft().Slot(value())
                                 : nullptr;
}

BuildingClass* Target::AsBuilding() const {
  return kind() == KIND_BUILDING ? TheObjectHeaps().building().Slot(value())
                                 : nullptr;
}

UnitClass* Target::AsUnit() const {
  return kind() == KIND_UNIT ? TheObjectHeaps().unit().Slot(value()) : nullptr;
}

Cell Target::AsCell() const { return Cell::Containing(AsCoordinate()); }

Coordinate Target::AsCoordinate() const {
  if (*this) {
    // A cell target's value is the cell index.
    if (kind() == KIND_CELL) {
      return Cell::FromIndex(value()).Center();
    }
    const ObjectClass* obj = AsObject();
    if (obj) {
      return obj->Target_Coord();
    }
  }
  return {};
}

Coordinate Target::AsMovementCoordinate() const {
  if (*this) {
    // A cell target's value is the cell index.
    if (kind() == KIND_CELL) {
      return Cell::FromIndex(value()).Center();
    }
    const ObjectClass* obj = AsObject();
    if (obj) {
      return obj->Docking_Coord();
    }
  }
  return {};
}

}  // namespace td
