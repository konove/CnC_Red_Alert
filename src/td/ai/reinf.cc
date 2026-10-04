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

#include "td/ai/reinf.h"

#include "base/array.h"
#include "base/pooled.h"
#include "base/strings/safe_string.h"
#include "game/sim/direction.h"
#include "game/sim/enums.h"
#include "magic_enum/magic_enum.hpp"
#include "td/ai/team.h"
#include "td/ai/teamtype.h"
#include "td/assets/audio.h"
#include "td/core/defines.h"
#include "td/core/inline.h"
#include "td/core/object_heaps.h"
#include "td/editor/mapedit.h"
#include "td/map/world.h"
#include "td/net/session.h"
#include "td/objects/aircraft.h"
#include "td/objects/building.h"
#include "td/objects/cell.h"
#include "td/objects/foot.h"
#include "td/objects/object.h"
#include "td/objects/techno.h"
#include "td/objects/type.h"
#include "td/objects/unit.h"

namespace td {

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

// Deletes every member of the list `group`. A transport takes its cargo with
// it.
static void DeleteGroup(FootClass* group) {
  while (FootClass* member = PopHead(group)) {
    base::DeletePooled(member);
  }
}

// Returns whether `type` is the vehicle type `unit`.
static bool IsUnitType(const TechnoTypeClass& type, const UnitType unit) {
  const auto* unit_type = dynamic_cast<const UnitTypeClass*>(&type);
  return unit_type != nullptr && unit_type->Type == unit;
}

// Returns the first cell around `cell` that is off the visible map and that
// `member` can enter, or Cell::None() when there is none.
static Cell FindEdgeCellNextTo(const Cell cell, const FootClass& member) {
  for (const game::FacingType direction :
       magic_enum::enum_values<game::FacingType>()) {
    const Cell neighbor = Adjacent_Cell(cell, direction);
    if (!TheMap().In_Radar(neighbor) &&
        member.Can_Enter_Cell(neighbor, direction) == game::MOVE_OK) {
      return neighbor;
    }
  }
  return Cell::None();
}

// Puts the ground members of the list `group` on the map at the map edge
// `edge` of `house`, one at a time. `has_team` says whether a team will give
// them their orders. Members that find no room are deleted.
static void EnterFromEdge(FootClass* group, const SourceType edge,
                          const HousesType house, const bool has_team) {
  // SourceType counts the four edges clockwise from north and FacingType the
  // eight compass points, hence the factor of two. The facing is the edge's
  // own compass point, so the members start out facing off the map.
  const auto entry_facing =
      static_cast<game::FacingType>(static_cast<int>(edge) * 2);
  const game::DirType facing = game::Facing_Dir(entry_facing);

  // `member` is the one being placed and `group` the rest of the list. A cell
  // index of 0 or less means there is no cell.
  Cell place_cell = TheMap().Calculated_Cell(edge, house);
  FootClass* member = PopHead(group);
  while (place_cell.index() > 0 && member != nullptr) {
    if (member->Unlimbo(place_cell.Center(), facing)) {
      // The team handler assigns the mission a team member follows; until
      // then it guards. Without a team the member moves one cell further in,
      // which takes it onto the map.
      if (has_team) {
        member->Assign_Mission(MISSION_GUARD);
      } else {
        member->Assign_Mission(MISSION_MOVE);
        member->Assign_Destination(
            Target::FromCellIndex(Adjacent_Cell(place_cell, entry_facing)));
      }
      member->Commence();
      member = PopHead(group);
    } else {
      // The member could not be placed here, usually because the member before
      // it stands on the cell. Place it again from a cell next to this one;
      // the rest of the group follows from that cell. Without such a cell,
      // give up on the rest of the group.
      //
      // TODO: nothing bounds the retries. A member whose Unlimbo() fails for
      // a reason a new cell does not cure is handed from edge cell to edge
      // cell until the walk leaves the map; Red Alert's
      // DeliverReinforcements() gives up after eight.
      place_cell = FindEdgeCellNextTo(place_cell, *member);
    }
  }

  // Delete the members that could not be placed: the one the loop stopped at
  // and the rest of the list.
  base::DeletePooled(member);
  DeleteGroup(group);
}

// Flies the aircraft of the list `group` onto the map, each on its own, from
// the map edge `edge` of `house`. `has_team` says whether a team will give
// them their orders. Aircraft that cannot be placed are deleted, and so are
// members that are not aircraft. Returns whether the last aircraft was placed.
static bool FlyIn(FootClass* group, const SourceType edge,
                  const HousesType house, const bool has_team) {
  bool placed = false;
  while (FootClass* member = PopHead(group)) {
    // Only aircraft can fly in. A ground member gets here when the type has an
    // aircraft but no transport to carry it, and is dropped.
    auto* aircraft = dynamic_cast<AircraftClass*>(member);
    if (aircraft == nullptr) {
      base::DeletePooled(member);
      continue;
    }

    // Pick the cell on the house's edge to enter at; scenario_init makes
    // Calculated_Cell() accept a cell that ground units could not use. A cargo
    // plane instead enters at the east edge, in the row of the airstrip it
    // will unload at, so that it flies straight in. The 2 is added to the
    // packed docking coordinate, which nudges it two leptons in X and so
    // leaves the row alone.
    TheWorld().scenario_init()++;
    Cell entry_cell = TheMap().Calculated_Cell(edge, house);
    TheWorld().scenario_init()--;
    if (*aircraft == AIRCRAFT_CARGO) {
      const BuildingClass* airstrip =
          aircraft->Find_Docking_Bay(STRUCT_AIRSTRIP, false);
      if (airstrip != nullptr) {
        entry_cell = Cell::FromXY(
            TheMap().MapCellX + TheMap().MapCellWidth,
            Coordinate::FromPacked(airstrip->Docking_Coord().packed() + 2)
                .cell_y());
      }
    }

    // scenario_init places the aircraft on the edge cell without asking
    // whether it can enter it.
    TheWorld().scenario_init()++;
    placed = aircraft->Unlimbo(entry_cell.Center(), game::DIR_W);
    TheWorld().scenario_init()--;
    if (!placed) {
      base::DeletePooled(aircraft);
      continue;
    }

    // The team handler gives a team member its orders. Without a team, a
    // fixed-wing aircraft hunts, and a helicopter flies to the reinforcement
    // waypoint, where a loaded transport unloads.
    if (has_team) {
      continue;
    }
    if (aircraft->Class->IsFixedWing) {
      aircraft->Assign_Mission(MISSION_HUNT);
    } else {
      const bool is_loaded =
          aircraft->Class->IsTransporter && aircraft->Is_Something_Attached();
      aircraft->Assign_Mission(is_loaded ? MISSION_UNLOAD : MISSION_MOVE);
      aircraft->Assign_Destination(
          Target::FromCellIndex(TheMap().Calculated_Cell(SOURCE_AIR, house)));
    }
    aircraft->Commence();
  }
  return placed;
}

bool DeliverReinforcements(const TeamTypeClass* team_type) {
  // A missing type, or one with no members, has nothing to bring on.
  if (team_type == nullptr || team_type->ClassCount == 0) {
    return false;
  }
  HouseClass* const house = HouseClass::As_Pointer(team_type->House);
  const int member_type_count = team_type->ClassCount;

  // The team that will control the members. A type without missions gets no
  // team, since the team would have nothing to tell them: its members are
  // given orders of their own when they are placed.
  TeamClass* team = nullptr;
  if (team_type->MissionCount != 0) {
    team = base::NewPooled<TeamClass>(team_type, house);
    if (team == nullptr) {
      return false;
    }
    team->Force_Active();
  }

  // Find out what the type is made of. Every aircraft counts as a transport
  // here, whether or not it carries anything, because a group with an
  // aircraft in it arrives by air.
  bool arrives_by_air = false;  // Does the type have an aircraft?
  // Is its last ground transport a hovercraft?
  bool arrives_by_hovercraft = false;
  bool only_transports = true;  // Is there nothing but transports and aircraft?
  for (int member_index = 0; member_index < member_type_count; member_index++) {
    const TechnoTypeClass* member_type =
        base::At(team_type->Class, member_index);
    if (member_type->What_Am_I() == RTTI_AIRCRAFTTYPE) {
      arrives_by_air = true;
    } else if (member_type->IsTransporter) {
      arrives_by_hovercraft = IsUnitType(*member_type, UNIT_HOVER);
    } else {
      only_transports = false;
    }
  }

  // Decide where the group comes from: by air with an aircraft, onto the beach
  // with a hovercraft, along the shipping lane when it is a gunboat (only the
  // first member is looked at), otherwise over the house's map edge.
  SourceType source = house->Edge;
  if (arrives_by_air) {
    source = SOURCE_AIR;
  } else if (arrives_by_hovercraft) {
    source = SOURCE_BEACH;
  } else if (IsUnitType(*base::At(team_type->Class, 0), UNIT_GUNBOAT)) {
    source = SOURCE_SHIPPING;
  }

  // Without a source there is nowhere to put the group. The house's edge is
  // never SOURCE_NONE once its INI entry is read, so this guards a house that
  // was set up by hand.
  if (source == SOURCE_NONE) {
    base::DeletePooled(team);
    return false;
  }

  // Create the members, in limbo, sorting them into two lists linked through
  // Next: the transports, and everything else. They are created with
  // scenario_init raised, like the objects a scenario starts with.
  FootClass* transports = nullptr;
  FootClass* riders = nullptr;
  for (int member_index = 0; member_index < member_type_count; member_index++) {
    const TechnoTypeClass* member_type =
        base::At(team_type->Class, member_index);

    const int copies = base::At(team_type->DesiredNum, member_index);
    for (int copy = 0; copy < copies; copy++) {
      TheWorld().scenario_init()++;
      auto* member =
          dynamic_cast<FootClass*>(member_type->Create_One_Of(house));
      TheWorld().scenario_init()--;

      if (member == nullptr) {
        continue;
      }

      // Add the member to the team, transports included; scenario_init makes
      // the team accept a member that is still in limbo. The hovercraft is the
      // exception: it never joins, since it leaves once it has unloaded.
      const auto* unit = dynamic_cast<UnitClass*>(member);
      if (team != nullptr && (unit == nullptr || *unit != UNIT_HOVER)) {
        TheWorld().scenario_init()++;
        team->Add(member);
        TheWorld().scenario_init()--;
      }

      // A transporter goes on the transport list. In a group that flies in,
      // only the aircraft do: a ground transport rides along as cargo.
      if (member_type->IsTransporter &&
          (!arrives_by_air || member_type->What_Am_I() == RTTI_AIRCRAFTTYPE)) {
        // An aircraft that carries the others is a loaner: it only delivers
        // them and then leaves the map. A transport that arrives empty is
        // itself the reinforcement, and a ground transport is always kept.
        if (!only_transports && member->What_Am_I() != RTTI_UNIT) {
          member->IsALoaner = true;
        }
        member->Next = transports;
        transports = member;
      } else {
        // A-10s are always loaners, since the player must never get to control
        // them.
        const auto* a10 = dynamic_cast<AircraftClass*>(member);
        if (a10 != nullptr && *a10 == AIRCRAFT_A10) {
          member->IsALoaner = true;
        }
        member->Next = riders;
        riders = member;
      }
    }
  }

  // Nothing could be created, most likely because the object limit was
  // reached.
  if (riders == nullptr && transports == nullptr) {
    base::DeletePooled(team);
    return false;
  }

  // The list that goes on the map is the transports, with everything else
  // loaded into the first one, or the members themselves when the type has no
  // transport.
  bool announce = true;
  FootClass* group = riders;
  if (transports != nullptr) {
    if (riders != nullptr) {
      // A cargo plane with a load is not announced when it is created; the
      // announcement comes when it unloads at the airstrip.
      const auto* plane = dynamic_cast<AircraftClass*>(transports);
      if (plane != nullptr && *plane == AIRCRAFT_CARGO) {
        announce = false;
      }
      transports->Attach(riders);
    }
    group = transports;
  }

  // Put the list on the map, in the way its source calls for.
  switch (source) {
    // The gunboat starts at the east end of the shipping lane and sails west
    // to the map's left edge, along its own row. Calculated_Cell() returns
    // cell 0 when the map has no lane. Only the head of the list sails;
    // whatever else the type holds is dropped.
    case SOURCE_SHIPPING: {
      FootClass* gunboat = PopHead(group);
      DeleteGroup(group);
      const Cell lane_cell = TheMap().Calculated_Cell(source, team_type->House);
      gunboat->IsALoaner = true;
      if (!gunboat->Unlimbo(lane_cell.Center(), game::DIR_W)) {
        base::DeletePooled(team);
        base::DeletePooled(gunboat);
        return false;
      }
      gunboat->Assign_Mission(MISSION_GUARD);
      gunboat->Assign_Destination(Target::FromCellIndex(Cell::FromXY(
          TheMap().MapCellX - 1, Cell::Containing(gunboat->Coord).y())));
    } break;

    // The members drive or walk on from the map edge. An aircraft never comes
    // this way: a group with one has SOURCE_AIR as its source.
    case SOURCE_NORTH:
    case SOURCE_SOUTH:
    case SOURCE_EAST:
    case SOURCE_WEST:
      EnterFromEdge(group, source, team_type->House, team != nullptr);
      break;

    case SOURCE_AIR: {
      const bool placed =
          FlyIn(group, house->Edge, team_type->House, team != nullptr);

      // `placed` is the result for the last aircraft only, so the team goes
      // when that one failed, even if earlier ones are on the map.
      if (!placed) {
        base::DeletePooled(team);
      }

      // Report the failure, so that the caller can delete the team type it
      // made for this: that prevents "phantom" teams and team types from
      // being left around. Only the campaign does; a multiplayer game goes on
      // to announce the reinforcement and reports success.
      if (TheSession().type() == GAME_NORMAL && !placed) {
        return false;
      }
    } break;

    // The hovercraft starts below the bottom edge of the map, in the column
    // of the beach cell it will land on. Only the head of the list lands;
    // any further transport is dropped, and so is the whole group when no
    // beach cell is free, which Calculated_Cell() reports as cell 0.
    case SOURCE_OCEAN:
    case SOURCE_BEACH: {
      FootClass* lander = PopHead(group);
      DeleteGroup(group);
      const Cell beach_cell =
          TheMap().Calculated_Cell(SOURCE_BEACH, team_type->House);
      if (beach_cell.index() == 0) {
        base::DeletePooled(lander);
        break;
      }
      const Cell start_cell = Cell::FromXY(
          beach_cell.x(), TheMap().MapCellY + TheMap().MapCellHeight);
      if (!lander->Unlimbo(start_cell.Center(), game::DIR_N)) {
        base::DeletePooled(team);
        base::DeletePooled(lander);
        return false;
      }

      // Without a team to order it, the hovercraft is sent to the beach to
      // unload.
      if (team == nullptr) {
        lander->IsLocked = false;
        lander->Assign_Mission(MISSION_UNLOAD);
        lander->Commence();
        lander->Assign_Destination(Target::FromCellIndex(beach_cell));
      }
    } break;

    // A house whose edge is one of these has nowhere to enter the map, so the
    // group is dropped. It is still reported as delivered.
    case SourceType::SOURCE_NONE:
    case SourceType::SOURCE_VISIBLE:
    case SourceType::SOURCE_ENEMYBASE:
    case SourceType::SOURCE_HOMEBASE:
    case SourceType::SOURCE_COUNT:
    default:
      DeleteGroup(group);
      break;
  }

  // Announce the reinforcements to the player they belong to. This does not
  // depend on any member having reached the map.
  if (announce && team_type->House == ThePlayer()->Class->House) {
    Speak(VOX_REINFORCEMENTS);
  }

  return true;
}

// Returns whether a ground unit can drive onto the map over its south edge:
// whether some column's cell below the map and its bottom map cell are both
// clear. The last column is not looked at.
static bool HasLandRouteFromSouth() {
  const int edge_row = TheMap().MapCellY + TheMap().MapCellHeight;
  for (int column = TheMap().MapCellX;
       column < TheMap().MapCellX + TheMap().MapCellWidth - 1; column++) {
    const Cell cell = Cell::FromXY(column, edge_row);
    if (TheMap().at(cell).Is_Generally_Clear() &&
        TheMap().at(cell.Offset(-MAP_CELL_W)).Is_Generally_Clear()) {
      return true;
    }
  }
  return false;
}

bool DeliverSpecialReinforcement(const HouseClass* house,
                                 const TechnoTypeClass* type,
                                 const TechnoTypeClass* transport_type,
                                 TeamMissionType mission,
                                 int mission_argument) {
  if (house == nullptr || type == nullptr) {
    return false;
  }
  auto* team_type = base::NewPooled<TeamTypeClass>();
  if (team_type == nullptr) {
    return false;
  }

  // A vehicle or soldier with no transport given drives on from the house's
  // map edge, if it can.
  const bool drives_on =
      transport_type == nullptr && (type->What_Am_I() == RTTI_UNITTYPE ||
                                    type->What_Am_I() == RTTI_INFANTRYTYPE);

  // No land route was found for the reinforcement to drive itself onto the
  // map. Assign it a hovercraft. This presumes that if the south edge is non
  // drivable, it will have water there instead. Risky, but is not a problem
  // with the current C&C maps. The mission the caller asked for is dropped:
  // the hovercraft delivers and that is all.
  //
  // Only the south edge is checked for a way in, because the hovercraft can
  // only arrive from the south. Yes, this is an arbitrary limitation, but
  // that's the way it is (for now).
  if (drives_on && house->Edge == SOURCE_SOUTH && !HasLandRouteFromSouth()) {
    mission = TMISSION_NONE;
    transport_type = &UnitTypeClass::As_Reference(UNIT_HOVER);
  }

  // Without a transport, a vehicle or soldier drives to the reinforcement
  // waypoint, whatever mission the caller asked for, and anything else does
  // when it has no mission. If not, the reinforcement will just sit at the
  // edge of the map.
  if (transport_type == nullptr && (drives_on || mission == TMISSION_NONE)) {
    mission = TMISSION_MOVECELL;
    mission_argument =
        TheMap().Calculated_Cell(SOURCE_AIR, house->Class->House).index();
  }

  // Fill in the team type: transient, so that it is deleted along with its
  // last team. It always has one mission slot, so DeliverReinforcements()
  // always makes a team for it. The slot stays TMISSION_NONE for a hovercraft
  // delivery; any other transport without a mission unloads at the
  // reinforcement waypoint.
  base::SafeCopy(team_type->IniName, "TEMP");
  team_type->IsReinforcable = false;
  team_type->IsTransient = true;
  team_type->House = house->Class->House;
  team_type->ClassCount = 1;
  base::At(team_type->Class, 0) = type;
  base::At(team_type->DesiredNum, 0) = 1;
  if (transport_type != nullptr) {
    team_type->ClassCount = 2;
    base::At(team_type->Class, 1) = transport_type;
    base::At(team_type->DesiredNum, 1) = 1;
  }
  team_type->MissionCount = 1;
  if (mission != TMISSION_NONE) {
    base::At(team_type->MissionList, 0).Mission = mission;
    base::At(team_type->MissionList, 0).Argument = mission_argument;
  } else if (transport_type != nullptr &&
             !IsUnitType(*transport_type, UNIT_HOVER)) {
    base::At(team_type->MissionList, 0).Mission = TMISSION_UNLOAD;
    base::At(team_type->MissionList, 0).Argument = kWayptReinf;
  }

  // A delivery that fails after making its team deletes the team, and the
  // team takes its transient type with it; the type is left over only when
  // the team could not be made. Multiplayer games skip the delete, so there
  // such a type is never freed.
  const int slot = TheObjectHeaps().team_type().SlotIndexOf(team_type);
  const bool delivered = DeliverReinforcements(team_type);
  if (!delivered && TheSession().type() == GAME_NORMAL &&
      TheObjectHeaps().team_type().IsAllocated(slot)) {
    base::DeletePooled(team_type);
  }
  return delivered;
}

int LaunchAirStrike(HouseClass* house, const AircraftType aircraft_type,
                    const int count, const MissionType mission,
                    const Target target, const Target destination) {
  const AircraftTypeClass& type_class =
      AircraftTypeClass::As_Reference(aircraft_type);

  // Enter from the house's map edge, or the north one when its edge is not
  // one of the four.
  SourceType edge = house->Edge;
  if (edge != SOURCE_NORTH && edge != SOURCE_EAST && edge != SOURCE_SOUTH &&
      edge != SOURCE_WEST) {
    edge = SOURCE_NORTH;
  }

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

    const Cell entry_cell = TheMap().Calculated_Cell(edge, house->Class->House);

    // scenario_init places the aircraft on the edge cell without asking
    // whether it can enter it.
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

}  // namespace td
