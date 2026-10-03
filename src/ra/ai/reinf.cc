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

// Makes every member of the linked list `group` come out of `host`: walking
// out of a building, or loaded into a transport that is then told to unload.
// Members that cannot come out are deleted. Returns whether any did.
static bool SendGroupOutOf(FootClass* group, TechnoClass* host) {
  DCHECK(group != nullptr && host != nullptr);
  int emerged_count = 0;

  // Take the members off the list one at a time: a transport chains its cargo
  // through Next as well.
  while (group != nullptr) {
    TechnoClass* member = group;
    group = dynamic_cast<FootClass*>(group->Next);
    member->Next = nullptr;

    switch (host->What_Am_I()) {
      // Infantry walks out of a building. Any result but 2, a completed exit,
      // leaves the soldier in limbo, so it is deleted.
      case RTTI_BUILDING:
        if (host->Exit_Object(member) != 2) {
          base::DeletePooled(member);
        } else {
          ++emerged_count;
        }
        break;

      // Infantry is loaded into a transport, which then unloads it. Nothing
      // checks that the transport has room.
      case RTTI_UNIT:
      case RTTI_VESSEL:
      case RTTI_AIRCRAFT:
        host->Attach(dynamic_cast<FootClass*>(member));
        host->Assign_Mission(MISSION_UNLOAD);
        ++emerged_count;
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
        base::DeletePooled(member);
        break;
    }
  }

  return emerged_count != 0;
}

// Returns whether the house keeps the reinforcement `aircraft` rather than
// borrowing it for one sortie. Only a Yak or a MiG can be kept, and only while
// the house has an airstrip that no other one is using.
static bool HouseKeepsAircraft(const AircraftClass* aircraft) {
  if (*aircraft == AIRCRAFT_YAK || *aircraft == AIRCRAFT_MIG) {
    int free_airstrips = aircraft->House->Get_Quantity(STRUCT_AIRSTRIP);

    // Each Yak or MiG that is already kept uses up one airstrip. Only the human
    // player's aircraft are counted, whichever house `aircraft` belongs to.
    for (int index = 0; index < TheObjectHeaps().aircraft().Count(); index++) {
      const AircraftClass* other = TheObjectHeaps().aircraft().ActiveAt(index);
      if ((*other == AIRCRAFT_YAK || *other == AIRCRAFT_MIG) &&
          other->IsOwnedByPlayer && !other->IsALoaner && other != aircraft) {
        free_airstrips -= 1;
        if (free_airstrips == 0) {
          break;
        }
      }
    }

    if (free_airstrips > 0) {
      return true;
    }
  }
  return false;
}

// Creates the team for `team_type` and all its members, in limbo. Returns the
// head of a list linked through `Next`: the transports, with every other
// member loaded into the first one, or the members themselves when the type
// has no transport. Returns nullptr when no member could be created.
static FootClass* CreateGroup(TeamTypeClass* team_type) {
  DCHECK(team_type != nullptr);

  auto* team = base::NewPooled<TeamClass>(team_type);
  if (team != nullptr) {
    team->Force_Active();
  }

  bool has_unload_mission = false;
  for (int mission_index = 0; mission_index < team_type->MissionCount;
       mission_index++) {
    if (base::At(team_type->MissionList, mission_index).Mission ==
        TMISSION_UNLOAD) {
      has_unload_mission = true;
      break;
    }
  }

  // Create the members, sorting them into transports and everything else.
  FootClass* transports = nullptr;
  FootClass* riders = nullptr;
  for (int member_index = 0; member_index < team_type->ClassCount;
       member_index++) {
    const TechnoTypeClass* member_type =
        base::At(team_type->Members, member_index).Class;

    for (int copy = 0;
         copy < base::At(team_type->Members, member_index).Quantity; copy++) {
      TheWorld().scenario_init()++;
      auto* member = dynamic_cast<FootClass*>(
          member_type->Create_One_Of(HouseClass::As_Pointer(team_type->House)));
      TheWorld().scenario_init()--;

      if (member != nullptr) {
        // Add the member to the team. scenario_init makes the team accept a
        // member that is still in limbo.
        if (team != nullptr) {
          TheWorld().scenario_init()++;
          team->Add(member);
          TheWorld().scenario_init()--;
          member->IsInitiated = true;
        }

        const auto* aircraft = dynamic_cast<const AircraftClass*>(member);
        if (aircraft != nullptr && !HouseKeepsAircraft(aircraft)) {
          member->IsALoaner = true;
        }

        // A type that can carry passengers is a transport.
        if (member_type->Max_Passengers() > 0) {
          member->Next = transports;
          transports = member;

        } else {
          member->Next = riders;
          riders = member;
        }
      }
    }
  }

  // Everything else rides in the first transport.
  if (transports != nullptr && riders != nullptr) {
    transports->Attach(riders);

    // HACK ALERT! A flying or sailing transport that the team's missions unload
    // is a loaner: it leaves the map once it has unloaded.
    if (has_unload_mission && (transports->What_Am_I() == RTTI_AIRCRAFT ||
                               transports->What_Am_I() == RTTI_VESSEL)) {
      transports->IsALoaner = true;
    }
  }

  // A transport helicopter that arrives empty is itself the reinforcement, so
  // the house keeps it.
  const auto* helicopter = dynamic_cast<AircraftClass*>(transports);
  if (helicopter != nullptr && riders == nullptr &&
      *helicopter == AIRCRAFT_TRANSPORT) {
    transports->IsALoaner = false;
  }

  if (transports == nullptr && riders == nullptr) {
    base::DeletePooled(team);
    return nullptr;
  }

  // Without a transport the group is the members themselves.
  if (transports == nullptr) {
    return riders;
  }

  return transports;
}

// Returns whether every member of the list `group` is infantry. Only such a
// group can come out of a building or a transport instead of crossing the map
// edge.
static bool IsAllInfantry(const FootClass* group) {
  while (group != nullptr) {
    if (group->What_Am_I() != RTTI_INFANTRY) {
      return false;
    }
    group = dynamic_cast<const FootClass*>(group->Next);
  }
  return true;
}

// Returns something at `origin` or in one of the eight cells around it that
// infantry can come out of: a transport vehicle if there is one, otherwise a
// building; nullptr when there is neither.
static TechnoClass* FindHostNear(Cell origin) {
  CellClass* origin_cell = &TheMap().at(origin);
  TechnoClass* building_host = nullptr;

  for (int facing = -1; facing < 8; facing++) {
    CellClass* cell = origin_cell;
    if (facing != -1) {
      cell = &cell->Adjacent_Cell(static_cast<game::FacingType>(facing));
    }

    BuildingClass* building = cell->Cell_Building();
    if (building && building->Strength > 0) {
      building_host = building;
    }

    UnitClass* unit = cell->Cell_Unit();
    if (unit && unit->Strength && unit->Class->Max_Passengers() > 0) {
      return unit;
    }
  }
  return building_host;
}

bool DeliverReinforcements(TeamTypeClass* team_type) {
  DCHECK(team_type != nullptr);

  // A type with no members has nothing to bring on.
  if (!team_type || !team_type->ClassCount) {
    return false;
  }

  // HACK ALERT! A team without missions would sit where it arrived, so the type
  // is given one: attack its origin waypoint. The team type keeps it.
  if (team_type->MissionCount == 0) {
    team_type->MissionCount = 1;
    team_type->MissionList[0].Mission = TMISSION_ATT_WAYPT;
    team_type->MissionList[0].Data.Value = team_type->Origin;
  }

  FootClass* member = CreateGroup(team_type);

  // Nothing could be created, most likely because the object limit was reached.
  if (!member) {
    return false;
  }

  // Infantry with an origin waypoint comes out of a building or a transport
  // there, if there is one. This relies on infantry never being a transport, so
  // the head of the list is a member and not a carrier.
  if (member != nullptr && team_type->Origin != -1 && IsAllInfantry(member)) {
    TechnoClass* host =
        FindHostNear(base::At(TheScenario().Waypoint, team_type->Origin));

    if (host != nullptr) {
      return SendGroupOutOf(member, host);
    }
  }

  // Otherwise the group moves onto the map under its own power, from the
  // house's map edge, or north when it has none.
  SourceType edge = HouseClass::As_Pointer(team_type->House)->Control.Edge;
  if (edge == SOURCE_NONE) {
    edge = SOURCE_NORTH;
  }

  // SourceType counts the four edges clockwise from north and FacingType the
  // eight compass points, hence the factor of two. The facing is the edge's own
  // compass point, so the group starts out facing off the map.
  const auto entry_facing =
      static_cast<game::FacingType>(static_cast<int>(edge) * 2);

  Cell edge_cell =
      TheMap().Calculated_Cell(edge, team_type->Origin, Cell::None(),
                               member->Techno_Type_Class()->Speed);
  // Ants come straight out of the ant hill at their origin waypoint instead.
  const auto* lead_unit = dynamic_cast<UnitClass*>(member);
  if (team_type->Origin != -1 && lead_unit != nullptr &&
      (*lead_unit == UNIT_ANT1 || *lead_unit == UNIT_ANT2 ||
       *lead_unit == UNIT_ANT3)) {
    const Cell hill_cell = base::At(TheScenario().Waypoint, team_type->Origin);
    if ((hill_cell != Cell::None()) &&
        (TheMap().at(hill_cell).TType == TEMPLATE_HILL01)) {
      edge_cell = hill_cell;
    }
  }

  Cell place_cell = edge_cell;

  auto* remaining = dynamic_cast<FootClass*>(member->Next);
  member->Next = nullptr;
  bool any_placed = false;
  // Place the members one at a time: `member` is the one being placed and
  // `remaining` the rest of the list. A cell index of 0 or less means there is
  // no cell.
  while (place_cell.index() > 0 && member != nullptr) {
    game::DirType facing = game::Facing_Dir(entry_facing);
    if (member->What_Am_I() == RTTI_AIRCRAFT) {
      facing = Random_Pick(game::DIR_N, kMaxRandomDirection);
    }

    // scenario_init places the member on the edge cell without asking whether
    // it can enter it.
    TheWorld().scenario_init()++;
    if (member->Unlimbo(place_cell.Center(), facing)) {
      any_placed = true;

      // The team handler assigns the mission the member follows; until then it
      // guards. Aircraft start on their own.
      if (member->What_Am_I() != RTTI_AIRCRAFT) {
        member->Assign_Mission(MISSION_GUARD);
        member->Commence();
      }

    } else {
      // The member could not be placed here, so place it again from a cell next
      // to this one that is off the visible map and that it can enter; the rest
      // of the group follows from that cell. Without such a cell, give up on
      // the rest of the group.
      bool found = false;
      for (const game::FacingType direction :
           magic_enum::enum_values<game::FacingType>()) {
        const Cell neighbor = Adjacent_Cell(place_cell, direction);
        if (!TheMap().In_Radar(neighbor) &&
            member->Can_Enter_Cell(neighbor, direction) == game::MOVE_OK) {
          place_cell = neighbor;
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

    member = remaining;
    if (member != nullptr) {
      remaining = dynamic_cast<FootClass*>(member->Next);
      member->Next = nullptr;
    }
  }

  // Delete the members that could not be placed: the one the loop stopped at
  // and the rest of the list.
  base::DeletePooled(member);
  if (remaining != nullptr) {
    while (remaining != nullptr) {
      FootClass* unplaced = remaining;
      remaining = dynamic_cast<FootClass*>(remaining->Next);
      unplaced->Next = nullptr;

      base::DeletePooled(unplaced);
    }
  }

  // Announce the reinforcements to the player they belong to.
  if (any_placed && team_type->House == ThePlayer()->Class->House) {
    Speak(VOX_REINFORCEMENTS);
  }

  return true;
}

int LaunchAirStrike(HouseClass* house, AircraftType aircraft_type, int count,
                    MissionType mission, Target target, Target destination) {
  DCHECK(house != nullptr);
  DCHECK(static_cast<unsigned>(aircraft_type) <
         magic_enum::enum_count<AircraftType>());
  DCHECK(count != 0);
  DCHECK(static_cast<unsigned>(mission) <
         magic_enum::enum_count<MissionType>());
  const TechnoTypeClass* type_class = static_cast<TechnoTypeClass*>(
      &AircraftTypeClass::As_Reference(aircraft_type));

  // Create and place the aircraft one at a time.
  int placed_count = 0;
  for (placed_count = 0; placed_count < count; placed_count++) {
    // Creation fails when the aircraft pool is full.
    TheWorld().scenario_init()++;
    auto* aircraft =
        dynamic_cast<TechnoClass*>(type_class->Create_One_Of(house));
    TheWorld().scenario_init()--;
    if (!aircraft) {
      return placed_count;
    }

    // Strike aircraft are always loaners: they leave the map when done.
    aircraft->IsALoaner = true;

    // Enter from the house's map edge, or from the north when it has none.
    SourceType edge = house->Control.Edge;
    switch (edge) {
      case SOURCE_NORTH:
      case SOURCE_EAST:
      case SOURCE_SOUTH:
      case SOURCE_WEST:
        break;

      case SourceType::SOURCE_NONE:
      case SourceType::SOURCE_AIR:
      default:
        edge = SOURCE_NORTH;
        break;
    }
    const Cell entry_cell =
        TheMap().Calculated_Cell(edge, -1, Cell::None(), SPEED_WINGED);

    // scenario_init places the aircraft on the edge cell without asking whether
    // it can enter it.
    TheWorld().scenario_init()++;
    const bool placed = aircraft->Unlimbo(entry_cell.Center(), game::DIR_N);
    TheWorld().scenario_init()--;
    if (placed) {
      // The aircraft is on the map: give it its orders. A default Target means
      // that none was given.
      aircraft->Assign_Mission(mission);

      if (destination != Target{}) {
        aircraft->Assign_Destination(destination);
      }

      if (target != Target{}) {
        aircraft->Assign_Target(target);
      }

      // Start the aircraft into action.
      aircraft->Commence();
    } else {
      base::DeletePooled(aircraft);
      return placed_count;
    }
  }
  return placed_count;
}

}  // namespace ra
