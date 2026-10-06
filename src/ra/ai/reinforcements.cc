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

#include "ra/ai/reinforcements.h"

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
#include "ra/core/coord.h"
#include "ra/core/defines.h"
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

// Takes the first member off the list `group`, which is linked through `Next`,
// and returns it with its own link cleared; nullptr when the list is empty.
static FootClass* PopHead(FootClass*& group) {
  FootClass* head = group;
  if (head != nullptr) {
    group = dynamic_cast<FootClass*>(head->Next);
    head->Next = nullptr;
  }
  return head;
}

// Makes every member of the linked list `group` come out of `host`, a building
// or a transport vehicle: walking out of the building, or loaded into the
// transport, which is then told to unload. Members that cannot come out are
// deleted. Returns whether any did.
static bool SendGroupOutOf(FootClass* group, TechnoClass* host) {
  DCHECK(group != nullptr && host != nullptr);
  const bool is_building = host->What_Am_I() == RTTI_BUILDING;
  DCHECK(is_building || host->What_Am_I() == RTTI_UNIT);
  bool any_emerged = false;

  // The members come off the list one at a time because a transport chains its
  // cargo through Next as well.
  while (FootClass* member = PopHead(group)) {
    if (!is_building) {
      // Nothing checks that the transport has room.
      host->Attach(member);
      host->Assign_Mission(MISSION_UNLOAD);
      any_emerged = true;
    } else if (host->Exit_Object(member) == 2) {
      any_emerged = true;
    } else {
      // Any result but 2, a completed exit, leaves the soldier in limbo.
      base::DeletePooled(member);
    }
  }

  return any_emerged;
}

// Returns whether the house keeps the reinforcement `aircraft` rather than
// borrowing it for one sortie. Only a Yak or a MiG can be kept, and only while
// the house has an airstrip that no other one is using.
static bool HouseKeepsAircraft(const AircraftClass* aircraft) {
  if (*aircraft != AIRCRAFT_YAK && *aircraft != AIRCRAFT_MIG) {
    return false;
  }

  // Each Yak or MiG the house already keeps uses up one of its airstrips.
  int free_airstrips = aircraft->House->Get_Quantity(STRUCT_AIRSTRIP);
  for (int index = 0; index < TheObjectHeaps().aircraft().Count(); index++) {
    const AircraftClass* other = TheObjectHeaps().aircraft().ActiveAt(index);
    if ((*other == AIRCRAFT_YAK || *other == AIRCRAFT_MIG) &&
        other->House == aircraft->House && !other->IsALoaner &&
        other != aircraft) {
      free_airstrips -= 1;
      if (free_airstrips == 0) {
        break;
      }
    }
  }
  return free_airstrips > 0;
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
  HouseClass* const house = HouseClass::As_Pointer(team_type->House);
  for (int member_index = 0; member_index < team_type->ClassCount;
       member_index++) {
    const TeamMemberClass& entry = base::At(team_type->Members, member_index);
    const TechnoTypeClass* member_type = entry.Class;

    for (int copy = 0; copy < entry.Quantity; copy++) {
      TheWorld().scenario_init()++;
      auto* member =
          dynamic_cast<FootClass*>(member_type->Create_One_Of(house));
      TheWorld().scenario_init()--;

      if (member == nullptr) {
        continue;
      }

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

  // Without a transport the group is the members themselves.
  if (transports == nullptr) {
    if (riders == nullptr) {
      base::DeletePooled(team);
    }
    return riders;
  }

  if (riders != nullptr) {
    // Everything else rides in the first transport.
    transports->Attach(riders);

    // HACK ALERT! A flying or sailing transport that the team's missions unload
    // is a loaner: it leaves the map once it has unloaded.
    if (has_unload_mission && (transports->What_Am_I() == RTTI_AIRCRAFT ||
                               transports->What_Am_I() == RTTI_VESSEL)) {
      transports->IsALoaner = true;
    }
  } else {
    // A transport helicopter that arrives empty is itself the reinforcement, so
    // the house keeps it.
    const auto* helicopter = dynamic_cast<AircraftClass*>(transports);
    if (helicopter != nullptr && *helicopter == AIRCRAFT_TRANSPORT) {
      transports->IsALoaner = false;
    }
  }
  return transports;
}

// Returns whether every member of the list `group` is infantry. Only such a
// group can come out of a building or a transport instead of crossing the map
// edge.
static bool IsAllInfantry(const FootClass* group) {
  while (group != nullptr) {
    if (!group->Is_Infantry()) {
      return false;
    }
    group = dynamic_cast<const FootClass*>(group->Next);
  }
  return true;
}

// Returns something at `origin` or in one of the eight cells around it that
// infantry can come out of: a transport vehicle if there is one, otherwise a
// building; nullptr when there is neither.
static TechnoClass* FindHostNear(const Cell origin) {
  const CellClass& origin_cell = TheMap().at(origin);
  TechnoClass* building_host = nullptr;

  // Returns the transport in `cell`, and remembers its building otherwise.
  const auto transport_in =
      [&building_host](const CellClass& cell) -> UnitClass* {
    BuildingClass* building = cell.Cell_Building();
    if (building != nullptr && building->Strength > 0) {
      building_host = building;
    }
    UnitClass* unit = cell.Cell_Unit();
    if (unit != nullptr && unit->Strength != 0 &&
        unit->Class->Max_Passengers() > 0) {
      return unit;
    }
    return nullptr;
  };

  if (UnitClass* transport = transport_in(origin_cell)) {
    return transport;
  }
  for (const game::FacingType facing :
       magic_enum::enum_values<game::FacingType>()) {
    if (UnitClass* transport =
            transport_in(origin_cell.Adjacent_Cell(facing))) {
      return transport;
    }
  }
  return building_host;
}

// Returns the first cell around `cell` that is off the visible map and that
// `member` can enter, or Cell::None() when there is none.
static Cell FindEdgeCellNextTo(const Cell cell, const FootClass& member) {
  for (const game::FacingType direction :
       magic_enum::enum_values<game::FacingType>()) {
    const Cell neighbor = AdjacentCell(cell, direction);
    if (!TheMap().In_Radar(neighbor) &&
        member.Can_Enter_Cell(neighbor, direction) == game::MOVE_OK) {
      return neighbor;
    }
  }
  return Cell::None();
}

bool DeliverReinforcements(TeamTypeClass* team_type) {
  DCHECK(team_type != nullptr);

  // A type with no members has nothing to bring on.
  if (team_type->ClassCount == 0) {
    return false;
  }

  // HACK ALERT! A team without missions would sit where it arrived, so the type
  // is given one: attack its origin waypoint. The team type keeps it.
  if (team_type->MissionCount == 0) {
    team_type->MissionCount = 1;
    team_type->MissionList[0].Mission = TMISSION_ATT_WAYPT;
    team_type->MissionList[0].Data.Value = team_type->Origin;
  }

  FootClass* group = CreateGroup(team_type);

  // Nothing could be created, most likely because the object limit was reached.
  if (group == nullptr) {
    return false;
  }

  // Infantry with an origin waypoint comes out of a building or a transport
  // there, if there is one. This relies on infantry never being a transport, so
  // the head of the list is a member and not a carrier.
  if (team_type->Origin != -1 && IsAllInfantry(group)) {
    TechnoClass* host =
        FindHostNear(base::At(TheScenario().Waypoint, team_type->Origin));

    if (host != nullptr) {
      return SendGroupOutOf(group, host);
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
  const game::DirType entry_dir =
      game::Facing_Dir(game::AsFacing(static_cast<int>(edge) * 2));
  // Aircraft face the other way, onto the map: a fixed-wing one flies where its
  // nose points and turns slowly, so any other heading sends it on a detour
  // along or off the edge before it comes about.
  const game::DirType inward_dir = entry_dir + game::DIR_S;
  // With an origin waypoint they face that instead: Calculated_Cell() then
  // picks the edge nearest the waypoint, which need not be the house's.
  const Cell origin_cell =
      team_type->Origin != -1
          ? base::At(TheScenario().Waypoint, team_type->Origin)
          : Cell::None();

  Cell place_cell = TheMap().Calculated_Cell(
      edge, team_type->Origin, Cell::None(), group->Techno_Type_Class()->Speed);
  // Ants come straight out of the ant hill at their origin waypoint instead.
  const auto* lead_unit = dynamic_cast<UnitClass*>(group);
  if (team_type->Origin != -1 && lead_unit != nullptr &&
      (*lead_unit == UNIT_ANT1 || *lead_unit == UNIT_ANT2 ||
       *lead_unit == UNIT_ANT3)) {
    const Cell hill_cell = base::At(TheScenario().Waypoint, team_type->Origin);
    if (hill_cell != Cell::None() &&
        TheMap().at(hill_cell).TType == TEMPLATE_HILL01) {
      place_cell = hill_cell;
    }
  }

  // Place the members one at a time: `member` is the one being placed and
  // `group` the rest of the list. A cell index of 0 or less means there is no
  // cell.
  //
  // A member that fails is tried again from a neighbouring cell, but only once
  // per neighbour: with scenario_init raised the cell is not what makes
  // Unlimbo() fail, so two edge cells next to each other would otherwise hand
  // the member back and forth for ever.
  constexpr int kMaxRetries =
      static_cast<int>(magic_enum::enum_count<game::FacingType>());
  int retries_left = kMaxRetries;
  FootClass* member = PopHead(group);
  bool any_placed = false;
  while (place_cell.index() > 0 && member != nullptr) {
    const bool is_aircraft = member->What_Am_I() == RTTI_AIRCRAFT;
    game::DirType facing = entry_dir;
    if (is_aircraft) {
      facing = origin_cell != Cell::None() ? Direction(place_cell, origin_cell)
                                           : inward_dir;
    }

    // scenario_init places the member on the edge cell without asking whether
    // it can enter it.
    TheWorld().scenario_init()++;
    if (member->Unlimbo(place_cell.Center(), facing)) {
      any_placed = true;

      // The team handler assigns the mission the member follows; until then it
      // guards. Aircraft start on their own.
      if (!is_aircraft) {
        member->Assign_Mission(MISSION_GUARD);
        member->Commence();
      }
      member = PopHead(group);
      retries_left = kMaxRetries;
    } else if (retries_left-- > 0) {
      // The member could not be placed here, so place it again from a cell next
      // to this one; the rest of the group follows from that cell. Without such
      // a cell, give up on the rest of the group.
      place_cell = FindEdgeCellNextTo(place_cell, *member);
    } else {
      place_cell = Cell::None();
    }
    TheWorld().scenario_init()--;
  }

  // Delete the members that could not be placed: the one the loop stopped at
  // and the rest of the list.
  for (; member != nullptr; member = PopHead(group)) {
    base::DeletePooled(member);
  }

  // Announce the reinforcements to the player they belong to.
  if (any_placed && team_type->House == ThePlayer()->Class->House) {
    Speak(VOX_REINFORCEMENTS);
  }

  return true;
}

int LaunchAirStrike(HouseClass* house, const AircraftType aircraft_type,
                    const int count, const MissionType mission,
                    const Target target, const Target destination) {
  DCHECK(house != nullptr);
  DCHECK(static_cast<unsigned>(aircraft_type) <
         magic_enum::enum_count<AircraftType>());
  DCHECK(count != 0);
  DCHECK(static_cast<unsigned>(mission) <
         magic_enum::enum_count<MissionType>());
  const AircraftTypeClass& type_class =
      AircraftTypeClass::As_Reference(aircraft_type);

  // Create and place the aircraft one at a time.
  int placed_count = 0;
  for (; placed_count < count; placed_count++) {
    // Creation fails when the aircraft pool is full.
    TheWorld().scenario_init()++;
    auto* aircraft =
        dynamic_cast<TechnoClass*>(type_class.Create_One_Of(house));
    TheWorld().scenario_init()--;
    if (aircraft == nullptr) {
      return placed_count;
    }

    // Strike aircraft are always loaners: they leave the map when done.
    aircraft->IsALoaner = true;

    // Enter from the house's map edge; Calculated_Cell() uses the north one
    // when the house has none.
    const Cell entry_cell = TheMap().Calculated_Cell(
        house->Control.Edge, -1, Cell::None(), SPEED_WINGED);

    // scenario_init places the aircraft on the edge cell without asking whether
    // it can enter it.
    TheWorld().scenario_init()++;
    const bool placed = aircraft->Unlimbo(entry_cell.Center(), game::DIR_N);
    TheWorld().scenario_init()--;
    if (!placed) {
      base::DeletePooled(aircraft);
      return placed_count;
    }

    // The aircraft is on the map: give it its orders. A default Target means
    // that none was given.
    aircraft->Assign_Mission(mission);
    if (destination != Target{}) {
      aircraft->Assign_Destination(destination);
    }
    if (target != Target{}) {
      aircraft->Assign_Target(target);
    }
    aircraft->Commence();
  }
  return placed_count;
}

}  // namespace ra
