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

#include "ra/ai/reinf.h"

#include "absl/log/check.h"
#include "base/array.h"
#include "base/pooled.h"
#include "game/core/object_heap.h"
#include "game/sim/direction.h"
#include "game/sim/enums.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/scenario.h"
#include "ra/ai/team.h"
#include "ra/ai/teamtype.h"
#include "ra/assets/audio.h"
#include "ra/core/defines.h"
#include "ra/core/inline.h"
#include "ra/core/object_heaps.h"
#include "ra/editor/mapedit.h"
#include "ra/map/world.h"
#include "ra/objects/aircraft.h"
#include "ra/objects/building.h"
#include "ra/objects/cell.h"
#include "ra/objects/foot.h"
#include "ra/objects/object.h"
#include "ra/objects/techno.h"
#include "ra/objects/type.h"
#include "ra/objects/unit.h"

namespace ra {

// Makes every member of the linked list `group` come out of `object`: walking
// out of a building, or loaded into a transport that is then told to unload.
// Members that cannot come out are deleted. Returns whether any did.
static bool Pop_Group_Out_Of_Object(FootClass* group, TechnoClass* object) {
  DCHECK(group != nullptr && object != nullptr);
  int quantity = 0;

  // Take the members off the list one at a time: a transport chains its cargo
  // through Next as well.
  while (group != nullptr) {
    TechnoClass* todo = group;
    group = dynamic_cast<FootClass*>(group->Next);
    todo->Next = nullptr;

    switch (object->What_Am_I()) {
      // Infantry walks out of a building. Any result but 2, a completed exit,
      // leaves the soldier in limbo, so it is deleted.
      case RTTI_BUILDING:
        if (object->Exit_Object(todo) != 2) {
          base::DeletePooled(todo);
        } else {
          ++quantity;
        }
        break;

      // Infantry is loaded into a transport, which then unloads it. Nothing
      // checks that the transport has room.
      case RTTI_UNIT:
      case RTTI_VESSEL:
      case RTTI_AIRCRAFT:
        object->Attach(dynamic_cast<FootClass*>(todo));
        object->Assign_Mission(MISSION_UNLOAD);
        ++quantity;
        break;

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
      case RTTIType::RTTI_INFANTRY:
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
        base::DeletePooled(todo);
        break;
    }
  }

  return quantity != 0;
}

// Returns whether the house keeps the reinforcement aircraft `air` rather than
// borrowing it for one sortie. Only a Yak or a MiG can be kept, and only while
// the house has an airstrip that no other one is using.
static bool Need_To_Take(const AircraftClass* air) {
  if (*air == AIRCRAFT_YAK || *air == AIRCRAFT_MIG) {
    int deficit = air->House->Get_Quantity(STRUCT_AIRSTRIP);

    // Each Yak or MiG that is already kept uses up one airstrip. Only the human
    // player's aircraft are counted, whichever house `air` belongs to.
    for (int index = 0; index < TheObjectHeaps().aircraft().Count(); index++) {
      const AircraftClass* airptr = TheObjectHeaps().aircraft().ActiveAt(index);
      if ((*airptr == AIRCRAFT_YAK || *airptr == AIRCRAFT_MIG) &&
          airptr->IsOwnedByPlayer && !airptr->IsALoaner && airptr != air) {
        deficit -= 1;
        if (deficit == 0) {
          break;
        }
      }
    }

    if (deficit > 0) {
      return true;
    }
  }
  return false;
}

// Creates the team for `teamtype` and all its members, in limbo. Returns the
// head of a list linked through `Next`: the transports, with every other
// member loaded into the first one, or the members themselves when the type
// has no transport. Returns nullptr when no member could be created.
static FootClass* Create_Group(TeamTypeClass* teamtype) {
  DCHECK(teamtype != nullptr);

  auto* team = base::NewPooled<TeamClass>(teamtype);
  if (team != nullptr) {
    team->Force_Active();
  }

  bool hasunload = false;
  for (int tm = 0; tm < teamtype->MissionCount; tm++) {
    if (base::At(teamtype->MissionList, tm).Mission == TMISSION_UNLOAD) {
      hasunload = true;
      break;
    }
  }

  // Create the members, sorting them into transports and everything else.
  FootClass* transport = nullptr;
  FootClass* object = nullptr;
  for (int index = 0; index < teamtype->ClassCount; index++) {
    const TechnoTypeClass* tclass = base::At(teamtype->Members, index).Class;

    for (int sub = 0; sub < base::At(teamtype->Members, index).Quantity;
         sub++) {
      TheWorld().scenario_init()++;
      auto* temp = dynamic_cast<FootClass*>(
          tclass->Create_One_Of(HouseClass::As_Pointer(teamtype->House)));
      TheWorld().scenario_init()--;

      if (temp != nullptr) {
        // Add the member to the team. scenario_init makes the team accept a
        // member that is still in limbo.
        if (team != nullptr) {
          TheWorld().scenario_init()++;
          team->Add(temp);
          TheWorld().scenario_init()--;
          temp->IsInitiated = true;
        }

        const auto* air = dynamic_cast<const AircraftClass*>(temp);
        if (air != nullptr && !Need_To_Take(air)) {
          temp->IsALoaner = true;
        }

        // A type that can carry passengers is a transport.
        if (tclass->Max_Passengers() > 0) {
          temp->Next = transport;
          transport = temp;

        } else {
          temp->Next = object;
          object = temp;
        }
      }
    }
  }

  // Everything else rides in the first transport.
  if (transport != nullptr && object != nullptr) {
    transport->Attach(object);

    // HACK ALERT! A flying or sailing transport that the team's missions unload
    // is a loaner: it leaves the map once it has unloaded.
    if (hasunload && (transport->What_Am_I() == RTTI_AIRCRAFT ||
                      transport->What_Am_I() == RTTI_VESSEL)) {
      transport->IsALoaner = true;
    }
  }

  // A transport helicopter that arrives empty is itself the reinforcement, so
  // the house keeps it.
  const auto* air_transport = dynamic_cast<AircraftClass*>(transport);
  if (air_transport != nullptr && object == nullptr &&
      *air_transport == AIRCRAFT_TRANSPORT) {
    transport->IsALoaner = false;
  }

  if (transport == nullptr && object == nullptr) {
    base::DeletePooled(team);
    return nullptr;
  }

  // Without a transport the group is the members themselves.
  if (transport == nullptr) {
    return object;
  }

  return transport;
}

// Returns whether every member of the list `first` is infantry. Only such a
// group can come out of a building or a transport instead of crossing the map
// edge.
static bool Consists_Only_Of_Infantry(const FootClass* first) {
  while (first != nullptr) {
    if (first->What_Am_I() != RTTI_INFANTRY) {
      return false;
    }
    first = dynamic_cast<const FootClass*>(first->Next);
  }
  return true;
}

// Returns something at `origin` or in one of the eight cells around it that
// infantry can come out of: a transport vehicle if there is one, otherwise a
// building; nullptr when there is neither.
static TechnoClass* Who_Can_Pop_Out_Of(Cell origin) {
  CellClass* cellptr = &TheMap().at(origin);
  TechnoClass* candidate = nullptr;

  for (int f = -1; f < 8; f++) {
    CellClass* ptr = cellptr;
    if (f != -1) {
      ptr = &ptr->Adjacent_Cell(static_cast<game::FacingType>(f));
    }

    BuildingClass* building = ptr->Cell_Building();
    if (building && building->Strength > 0) {
      candidate = building;
    }

    UnitClass* unit = ptr->Cell_Unit();
    if (unit && unit->Strength && unit->Class->Max_Passengers() > 0) {
      return unit;
    }
  }
  return candidate;
}

bool Do_Reinforcements(TeamTypeClass* teamtype) {
  DCHECK(teamtype != nullptr);

  // A type with no members has nothing to bring on.
  if (!teamtype || !teamtype->ClassCount) {
    return false;
  }

  // HACK ALERT! A team without missions would sit where it arrived, so the type
  // is given one: attack its origin waypoint. The team type keeps it.
  if (teamtype->MissionCount == 0) {
    teamtype->MissionCount = 1;
    teamtype->MissionList[0].Mission = TMISSION_ATT_WAYPT;
    teamtype->MissionList[0].Data.Value = teamtype->Origin;
  }

  FootClass* object = Create_Group(teamtype);

  // Nothing could be created, most likely because the object limit was reached.
  if (!object) {
    return false;
  }

  // Infantry with an origin waypoint comes out of a building or a transport
  // there, if there is one. This relies on infantry never being a transport, so
  // the head of the list is a member and not a carrier.
  if (object != nullptr && teamtype->Origin != -1 &&
      Consists_Only_Of_Infantry(object)) {
    TechnoClass* candidate =
        Who_Can_Pop_Out_Of(base::At(TheScenario().Waypoint, teamtype->Origin));

    if (candidate != nullptr) {
      return Pop_Group_Out_Of_Object(object, candidate);
    }
  }

  // Otherwise the group moves onto the map under its own power, from the
  // house's map edge, or north when it has none.
  SourceType source = HouseClass::As_Pointer(teamtype->House)->Control.Edge;
  if (source == SOURCE_NONE) {
    source = SOURCE_NORTH;
  }

  // SourceType counts the four edges clockwise from north and FacingType the
  // eight compass points, hence the factor of two. The facing is the edge's own
  // compass point, so the group starts out facing off the map.
  const auto eface =
      static_cast<game::FacingType>(static_cast<int>(source) * 2);

  Cell cell = TheMap().Calculated_Cell(source, teamtype->Origin, Cell::None(),
                                       object->Techno_Type_Class()->Speed);
  // Ants come straight out of the ant hill at their origin waypoint instead.
  const auto* unit = dynamic_cast<UnitClass*>(object);
  if (teamtype->Origin != -1 && unit != nullptr &&
      (*unit == UNIT_ANT1 || *unit == UNIT_ANT2 || *unit == UNIT_ANT3)) {
    const Cell newcell = base::At(TheScenario().Waypoint, teamtype->Origin);
    if ((newcell != Cell::None()) &&
        (TheMap().at(newcell).TType == TEMPLATE_HILL01)) {
      cell = newcell;
    }
  }

  Cell newcell = cell;

  auto* o = dynamic_cast<FootClass*>(object->Next);
  object->Next = nullptr;
  bool okvoice = false;
  // Place the members one at a time: `object` is the one being placed and `o`
  // the rest of the list. A cell index of 0 or less means there is no cell.
  while (newcell.index() > 0 && object != nullptr) {
    game::DirType desiredfacing = game::Facing_Dir(eface);
    if (object->What_Am_I() == RTTI_AIRCRAFT) {
      desiredfacing = Random_Pick(game::DIR_N, kMaxRandomDirection);
    }

    // scenario_init places the object on the edge cell without asking whether
    // it can enter it.
    TheWorld().scenario_init()++;
    if (object->Unlimbo(newcell.Center(), desiredfacing)) {
      okvoice = true;

      // The team handler assigns the mission the member follows; until then it
      // guards. Aircraft start on their own.
      if (object->What_Am_I() != RTTI_AIRCRAFT) {
        object->Assign_Mission(MISSION_GUARD);
        object->Commence();
      }

    } else {
      // The object could not be placed here, so place it again from a cell next
      // to this one that is off the visible map and that it can enter; the rest
      // of the group follows from that cell. Without such a cell, give up on
      // the rest of the group.
      bool found = false;
      for (const game::FacingType adj :
           magic_enum::enum_values<game::FacingType>()) {
        const Cell trycell = Adjacent_Cell(newcell, adj);
        if (!TheMap().In_Radar(trycell) &&
            object->Can_Enter_Cell(trycell, adj) == game::MOVE_OK) {
          newcell = trycell;
          found = true;
          break;
        }
      }
      TheWorld().scenario_init()--;
      if (found) {
        continue;
      }
      break;
    }
    TheWorld().scenario_init()--;

    object = o;
    if (object != nullptr) {
      o = dynamic_cast<FootClass*>(object->Next);
      object->Next = nullptr;
    }
  }

  // Delete the members that could not be placed: the one the loop stopped at
  // and the rest of the list.
  base::DeletePooled(object);
  if (o != nullptr) {
    while (o != nullptr) {
      FootClass* old = o;
      o = dynamic_cast<FootClass*>(o->Next);
      old->Next = nullptr;

      base::DeletePooled(old);
    }
  }

  // Announce the reinforcements to the player they belong to.
  if (okvoice && teamtype->House == ThePlayer()->Class->House) {
    Speak(VOX_REINFORCEMENTS);
  }

  return true;
}

int Create_Air_Reinforcement(HouseClass* house, AircraftType air, int number,
                             MissionType mission, Target tarcom,
                             Target navcom) {
  DCHECK(house != nullptr);
  DCHECK(static_cast<unsigned>(air) < magic_enum::enum_count<AircraftType>());
  DCHECK(number != 0);
  DCHECK(static_cast<unsigned>(mission) <
         magic_enum::enum_count<MissionType>());
  const TechnoTypeClass* type =
      static_cast<TechnoTypeClass*>(&AircraftTypeClass::As_Reference(air));

  // Create and place the aircraft one at a time.
  int sub = 0;
  for (sub = 0; sub < number; sub++) {
    // Creation fails when the aircraft pool is full.
    TheWorld().scenario_init()++;
    auto* obj = dynamic_cast<TechnoClass*>(type->Create_One_Of(house));
    TheWorld().scenario_init()--;
    if (!obj) {
      return sub;
    }

    // Strike aircraft are always loaners: they leave the map when done.
    obj->IsALoaner = true;

    // Enter from the house's map edge, or from the north when it has none.
    SourceType source = house->Control.Edge;
    switch (source) {
      case SOURCE_NORTH:
      case SOURCE_EAST:
      case SOURCE_SOUTH:
      case SOURCE_WEST:
        break;

      case SourceType::SOURCE_NONE:
      case SourceType::SOURCE_AIR:
      default:
        source = SOURCE_NORTH;
        break;
    }
    const Cell newcell =
        TheMap().Calculated_Cell(source, -1, Cell::None(), SPEED_WINGED);

    // scenario_init places the aircraft on the edge cell without asking whether
    // it can enter it.
    TheWorld().scenario_init()++;
    const bool placed = obj->Unlimbo(newcell.Center(), game::DIR_N);
    TheWorld().scenario_init()--;
    if (placed) {
      // The aircraft is on the map: give it its orders. A default Target means
      // that none was given.
      obj->Assign_Mission(mission);

      if (navcom != Target{}) {
        obj->Assign_Destination(navcom);
      }

      if (tarcom != Target{}) {
        obj->Assign_Target(tarcom);
      }

      // Start the object into action.
      obj->Commence();
    } else {
      base::DeletePooled(obj);
      return sub;
    }
  }
  return sub;
}

}  // namespace ra
