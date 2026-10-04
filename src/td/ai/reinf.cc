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

#include <utility>

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
#include "td/core/rand.h"
#include "td/editor/mapedit.h"
#include "td/map/world.h"
#include "td/net/session.h"
#include "td/objects/aircraft.h"
#include "td/objects/building.h"
#include "td/objects/cell.h"
#include "td/objects/foot.h"
#include "td/objects/object.h"
#include "td/objects/techno.h"
#include "td/objects/unit.h"

namespace td {

// Deletes every member of the list `group`, which is linked through Next. A
// transport takes its cargo with it.
static void DeleteGroup(ObjectClass* group) {
  while (group != nullptr) {
    ObjectClass* next = group->Next;
    group->Next = nullptr;
    base::DeletePooled(group);
    group = next;
  }
}

bool DeliverReinforcements(const TeamTypeClass* team_type) {
  // A missing type, or one with no members, has nothing to bring on.
  if (!team_type || !team_type->ClassCount) {
    return false;
  }

  // The team that will control the members. A type without missions gets no
  // team, since the team would have nothing to tell them: its members are
  // given orders of their own when they are placed.
  TeamClass* team = nullptr;
  if (team_type->MissionCount) {
    team = base::NewPooled<TeamClass>(team_type,
                                      HouseClass::As_Pointer(team_type->House));
    if (!team) {
      return false;
    }
    team->Force_Active();
  }

  // Find out what the type is made of. Every aircraft counts as a transport
  // here, whether or not it carries anything, because a group with an
  // aircraft in it arrives by air.
  bool announce = true;         // Tell the player?
  bool arrives_by_air = false;  // Does the type have an aircraft?
  bool arrives_by_hovercraft =
      false;                    // Is its last ground transport a hovercraft?
  bool only_transports = true;  // Is there nothing but transports and aircraft?
  for (int member_index = 0; std::cmp_less(member_index, team_type->ClassCount);
       member_index++) {
    if (base::At(team_type->Class, member_index)->IsTransporter ||
        base::At(team_type->Class, member_index)->What_Am_I() ==
            RTTI_AIRCRAFTTYPE) {
      if (base::At(team_type->Class, member_index)->What_Am_I() ==
          RTTI_AIRCRAFTTYPE) {
        arrives_by_air = true;
      } else {
        arrives_by_hovercraft = dynamic_cast<const UnitTypeClass*>(
                                    base::At(team_type->Class, member_index))
                                    ->Type == UNIT_HOVER;
      }
    } else {
      only_transports = false;
    }
  }

  // Decide where the group comes from: by air with an aircraft, onto the beach
  // with a hovercraft, otherwise over the house's map edge.
  SourceType source = SOURCE_NONE;
  if (arrives_by_air) {
    source = SOURCE_AIR;
  } else {
    if (arrives_by_hovercraft) {
      source = SOURCE_BEACH;
    } else {
      // A gunboat sails the shipping lane instead. Only the first member is
      // looked at.
      if (base::At(team_type->Class, 0)->What_Am_I() == RTTI_UNITTYPE &&
          dynamic_cast<const UnitTypeClass*>(base::At(team_type->Class, 0))
                  ->Type == UNIT_GUNBOAT) {
        source = SOURCE_SHIPPING;
      } else {
        source = HouseClass::As_Pointer(team_type->House)->Edge;
      }
    }
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
  TechnoClass* transports = nullptr;
  TechnoClass* object = nullptr;
  for (int member_index = 0; std::cmp_less(member_index, team_type->ClassCount);
       member_index++) {
    const TechnoTypeClass* member_type =
        base::At(team_type->Class, member_index);

    for (int copy = 0;
         std::cmp_less(copy, base::At(team_type->DesiredNum, member_index));
         copy++) {
      TheWorld().scenario_init()++;
      auto* member = dynamic_cast<FootClass*>(
          member_type->Create_One_Of(HouseClass::As_Pointer(team_type->House)));
      TheWorld().scenario_init()--;

      if (member) {
        // Add the member to the team, transports included; scenario_init makes
        // the team accept a member that is still in limbo. The hovercraft is
        // the exception: it never joins, since it leaves once it has unloaded.
        const auto* unit = dynamic_cast<UnitClass*>(member);
        if (team && (unit == nullptr || *unit != UNIT_HOVER)) {
          TheWorld().scenario_init()++;
          team->Add(member);
          TheWorld().scenario_init()--;
        }

        // A transporter goes on the transport list. In a group that flies in,
        // only the aircraft do: a ground transport rides along as cargo.
        if (member_type->IsTransporter &&
            (!arrives_by_air ||
             member_type->What_Am_I() == RTTI_AIRCRAFTTYPE)) {
          // An aircraft that carries the others is a loaner: it only delivers
          // them and then leaves the map. A transport that arrives empty is
          // itself the reinforcement, and a ground transport is always kept.
          if (!only_transports && member->What_Am_I() != RTTI_UNIT) {
            member->IsALoaner = true;
          }

          // Push it on the front of the transport list.
          member->Next = transports;
          transports = member;
        } else {
          // A-10s are always loaners, since the player must never get to
          // control them.
          const auto* a10 = dynamic_cast<AircraftClass*>(member);
          if (a10 != nullptr && *a10 == AIRCRAFT_A10) {
            member->IsALoaner = true;
          }

          // Push it on the front of the list of other members.
          member->Next = object;
          object = member;
        }
      }
    }
  }

  // Nothing could be created, most likely because the object limit was
  // reached.
  if (!object && !transports) {
    base::DeletePooled(team);
    return false;
  }

  // Load everything else into the first transport. From here on `object` is
  // the list that goes on the map: the transports when there are any,
  // otherwise the members themselves.
  if (transports) {
    if (object) {
      // A cargo plane with a load is not announced when it is created; the
      // announcement comes when it unloads at the airstrip.
      if (transports->What_Am_I() == RTTI_AIRCRAFT &&
          *dynamic_cast<AircraftClass*>(transports) == AIRCRAFT_CARGO) {
        announce = false;
      }

      transports->Attach(dynamic_cast<FootClass*>(object));
    }
    object = transports;
  }

  // Put the list on the map, in the way its source calls for.
  bool placed = false;
  Cell cell{};
  switch (source) {
    // The gunboat starts at the east end of the shipping lane and sails west
    // to the map's left edge, along its own row. Calculated_Cell() returns
    // cell 0 when the map has no lane. Only the head of the list sails;
    // whatever else the type holds is dropped.
    case SOURCE_SHIPPING:
      DeleteGroup(std::exchange(object->Next, nullptr));
      cell = TheMap().Calculated_Cell(source, team_type->House);
      object->IsALoaner = true;
      if (object->Unlimbo(cell.Center(), game::DIR_W)) {
        object->Assign_Mission(MISSION_GUARD);
        object->Assign_Destination(Target::FromCellIndex(Cell::FromXY(
            TheMap().MapCellX - 1, Cell::Containing(object->Coord).y())));
      } else {
        base::DeletePooled(team);
        base::DeletePooled(object);
        return false;
      }
      break;

    // The members drive or walk on from the map edge, one at a time.
    case SOURCE_NORTH:
    case SOURCE_SOUTH:
    case SOURCE_EAST:
    case SOURCE_WEST: {
      // SourceType counts the four edges clockwise from north and FacingType
      // the eight compass points, hence the factor of two. The facing is the
      // edge's own compass point, so the members start out facing off the map.
      const auto entry_facing =
          static_cast<game::FacingType>(static_cast<int>(source) * 2);

      // `arrives_by_air` is never set here: a group with an aircraft has
      // SOURCE_AIR as its source.

      if (arrives_by_air) {
        TheWorld().scenario_init()++;
      }
      cell = TheMap().Calculated_Cell(source, team_type->House);
      if (arrives_by_air) {
        TheWorld().scenario_init()--;
      }
      Cell place_cell = cell;

      // Place the members one at a time: `object` is the one being placed and
      // `o` the rest of the list. A cell index of 0 or less means there is no
      // cell. The aircraft branches below never run, for the reason above, so
      // `ok` stays true.
      auto* o = dynamic_cast<FootClass*>(object->Next);
      object->Next = nullptr;
      bool ok = true;
      while (place_cell.index() > 0 && object) {
        game::DirType facing = game::Facing_Dir(entry_facing);
        if (object->What_Am_I() == RTTI_AIRCRAFT) {
          facing = Random_Pick(game::DIR_N, kMaxRandomDirection);
        }

        if (ok && object->Unlimbo(place_cell.Center(), facing)) {
          // The team handler assigns the mission a team member follows; until
          // then it guards. Without a team the member moves one cell further
          // in, which takes it onto the map.
          if (object->What_Am_I() == RTTI_AIRCRAFT) {
            ok = false;
          } else {
            if (team) {
              object->Assign_Mission(MISSION_GUARD);
            } else {
              object->Assign_Mission(MISSION_MOVE);
              object->Assign_Destination(Target::FromCellIndex(
                  Adjacent_Cell(place_cell, entry_facing)));
            }
            object->Commence();
          }

        } else {
          ok = true;

          // The member could not be placed here, usually because the member
          // before it stands on the cell. Place it again from the first cell
          // next to this one that is off the visible map and that it can enter;
          // the rest of the group follows from that cell. Without such a cell,
          // give up on the rest of the group.
          // TODO: nothing bounds the retries. A member whose Unlimbo() fails
          // for a reason a new cell does not cure is handed between two edge
          // cells for ever; Red Alert's DeliverReinforcements() gives up after
          // eight.
          bool found = false;
          for (const game::FacingType direction :
               magic_enum::enum_values<game::FacingType>()) {
            const Cell neighbor = Adjacent_Cell(place_cell, direction);
            if (!TheMap().In_Radar(neighbor) &&
                object->Can_Enter_Cell(neighbor, direction) == game::MOVE_OK) {
              place_cell = neighbor;
              found = true;
              break;
            }
          }
          if (found) {
            continue;
          }
          break;
        }

        object = o;
        if (object) {
          o = dynamic_cast<FootClass*>(object->Next);
          object->Next = nullptr;
        }
      }

      // Delete the members that could not be placed: the one the loop stopped
      // at and the rest of the list.
      base::DeletePooled(object);
      if (o) {
        while (o) {
          FootClass* old = o;
          o = dynamic_cast<FootClass*>(o->Next);
          old->Next = nullptr;

          base::DeletePooled(old);
        }
      }
    } break;

    // Each aircraft flies in on its own, from the house's map edge.
    case SOURCE_AIR: {
      ObjectClass* rest = object;
      while (rest) {
        ObjectClass* next_member = rest;
        rest = next_member->Next;
        next_member->Next = nullptr;

        // Only aircraft can fly in. A ground member gets here when the type
        // has an aircraft but no transport to carry it, and is dropped.
        auto* aircraft = dynamic_cast<AircraftClass*>(next_member);
        if (!aircraft) {
          base::DeletePooled(next_member);
          continue;
        }

        // Pick the cell on the house's edge to enter at; scenario_init makes
        // Calculated_Cell() accept a cell that ground units could not use. A
        // cargo plane instead enters at the east edge, in the row of the
        // airstrip it will unload at, so that it flies straight in. The 2 is
        // added to the packed docking coordinate, which nudges it two leptons
        // in X and so leaves the row alone.
        TheWorld().scenario_init()++;
        Cell entry_cell = TheMap().Calculated_Cell(
            HouseClass::As_Pointer(team_type->House)->Edge, team_type->House);
        TheWorld().scenario_init()--;
        if (*aircraft == AIRCRAFT_CARGO) {
          const BuildingClass* airstrip =
              aircraft->Find_Docking_Bay(STRUCT_AIRSTRIP, false);
          if (airstrip) {
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
        if (placed) {
          // The team handler gives a team member its orders. Without a team,
          // a fixed-wing aircraft hunts, and a helicopter flies to the
          // reinforcement waypoint, where a loaded transport unloads.
          if (!team) {
            if (aircraft->Class->IsFixedWing) {
              aircraft->Assign_Mission(MISSION_HUNT);
            } else {
              if (aircraft->Class->IsTransporter &&
                  aircraft->Is_Something_Attached()) {
                aircraft->Assign_Mission(MISSION_UNLOAD);
              } else {
                aircraft->Assign_Mission(MISSION_MOVE);
              }
              aircraft->Assign_Destination(Target::FromCellIndex(
                  TheMap().Calculated_Cell(source, team_type->House)));
            }
            aircraft->Commence();
          }
        } else {
          base::DeletePooled(aircraft);
        }
      }
      // `placed` is the result for the last aircraft only, so the team goes
      // when that one failed, even if earlier ones are on the map.
      if (!placed && team) {
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
    // beach cell is free.
    case SOURCE_OCEAN:
    case SOURCE_BEACH:
      DeleteGroup(std::exchange(object->Next, nullptr));
      cell = TheMap().Calculated_Cell(SOURCE_BEACH, team_type->House);
      if (cell.index() != 0) {
        const Cell start_cell =
            Cell::FromXY(cell.x(), TheMap().MapCellY + TheMap().MapCellHeight);

        placed = object->Unlimbo(start_cell.Center(), game::DIR_N);
        if (placed) {
          // Without a team to order it, the hovercraft is sent to the beach
          // to unload.
          if (!team) {
            object->IsLocked = false;
            object->Assign_Mission(MISSION_UNLOAD);
            object->Commence();
            object->Assign_Destination(Target::FromCellIndex(cell));
          }
        } else {
          base::DeletePooled(team);
          base::DeletePooled(object);
          return false;
        }
      } else {
        base::DeletePooled(object);
      }
      break;

    // A house whose edge is one of these has nowhere to enter the map, so the
    // group is dropped. It is still reported as delivered.
    case SourceType::SOURCE_NONE:
    case SourceType::SOURCE_VISIBLE:
    case SourceType::SOURCE_ENEMYBASE:
    case SourceType::SOURCE_HOMEBASE:
    case SourceType::SOURCE_COUNT:
    default:
      DeleteGroup(object);
      break;
  }

  // Announce the reinforcements to the player they belong to. This does not
  // depend on any member having reached the map.
  if (announce && team_type->House == ThePlayer()->Class->House) {
    Speak(VOX_REINFORCEMENTS);
  }

  return true;
}

bool DeliverSpecialReinforcement(const HouseClass* house,
                                 const TechnoTypeClass* type,
                                 const TechnoTypeClass* transport_type,
                                 TeamMissionType mission,
                                 int mission_argument) {
  if (house && type) {
    auto* team_type = base::NewPooled<TeamTypeClass>();

    if (team_type) {
      // A vehicle or soldier with no transport given drives on from the
      // house's map edge, if it can.
      if (!transport_type && (type->What_Am_I() == RTTI_UNITTYPE ||
                              type->What_Am_I() == RTTI_INFANTRYTYPE)) {
        // Only the south edge is checked for a way in, because the hovercraft
        // can only arrive from the south. Yes, this is an arbitrary limitation,
        // but that's the way it is (for now). The way in is a column whose cell
        // below the map and whose bottom map cell are both clear; the last
        // column is not looked at.
        if (house->Edge == SOURCE_SOUTH) {
          bool has_land_route = false;
          for (int column = TheMap().MapCellX;
               column < TheMap().MapCellX + TheMap().MapCellWidth - 1;
               column++) {
            const Cell cell = Cell::FromXY(
                column, TheMap().MapCellY + TheMap().MapCellHeight);
            if (TheMap().at(cell).Is_Generally_Clear() &&
                TheMap().at(cell.Offset(-MAP_CELL_W)).Is_Generally_Clear()) {
              has_land_route = true;
              break;
            }
          }

          // No land route was found for the reinforcement to drive itself onto
          // the map. Assign it a hovercraft. This presumes that if the south
          // edge is non drivable, it will have water there instead. Risky, but
          // is not a problem with the current C&C maps. The mission the caller
          // asked for is dropped: the hovercraft delivers and that is all.
          if (!has_land_route) {
            mission = TMISSION_NONE;
            transport_type = &UnitTypeClass::As_Reference(UNIT_HOVER);
          }
        }

        // Otherwise it drives to the reinforcement waypoint, whatever mission
        // the caller asked for.
        if (!transport_type) {
          mission = TMISSION_MOVECELL;
          mission_argument =
              TheMap().Calculated_Cell(SOURCE_AIR, house->Class->House).index();
        }
      }

      // Anything else that comes without a transport and without a mission is
      // sent to the reinforcement waypoint too. If not, the reinforcement will
      // just sit at the edge of the map.
      if (!transport_type && mission == TMISSION_NONE) {
        mission = TMISSION_MOVECELL;
        mission_argument =
            TheMap().Calculated_Cell(SOURCE_AIR, house->Class->House).index();
      }

      // Fill in the team type: transient, so that it is deleted along with its
      // last team. It always has one mission slot, so DeliverReinforcements()
      // always makes a team for it. The slot stays TMISSION_NONE for a
      // hovercraft delivery; any other transport without a mission unloads at
      // the reinforcement waypoint.
      base::SafeCopy(team_type->IniName, "TEMP");
      team_type->IsReinforcable = false;
      team_type->IsTransient = true;
      team_type->ClassCount = 1;
      base::At(team_type->Class, 0) = type;
      base::At(team_type->DesiredNum, 0) = 1;
      team_type->MissionCount = 1;
      if (mission == TMISSION_NONE) {
        if (transport_type &&
            (transport_type->What_Am_I() != RTTI_UNITTYPE ||
             dynamic_cast<const UnitTypeClass*>(transport_type)->Type !=
                 UNIT_HOVER)) {
          base::At(team_type->MissionList, 0).Mission = TMISSION_UNLOAD;
          base::At(team_type->MissionList, 0).Argument = kWayptReinf;
        }
      } else {
        base::At(team_type->MissionList, 0).Mission = mission;
        base::At(team_type->MissionList, 0).Argument = mission_argument;
      }
      team_type->House = house->Class->House;
      if (transport_type) {
        team_type->ClassCount++;
        base::At(team_type->Class, 1) = transport_type;
        base::At(team_type->DesiredNum, 1) = 1;
      }

      // A delivery that fails after making its team deletes the team, and the
      // team takes its transient type with it; the type is left over only
      // when the team could not be made. Multiplayer games skip the delete,
      // so there such a type is never freed.
      const int slot = TheObjectHeaps().team_type().SlotIndexOf(team_type);
      const bool delivered = DeliverReinforcements(team_type);
      if (!delivered && TheSession().type() == GAME_NORMAL &&
          TheObjectHeaps().team_type().IsAllocated(slot)) {
        base::DeletePooled(team_type);
      }
      return delivered;
    }
  }
  return false;
}

int LaunchAirStrike(HouseClass* house, AircraftType aircraft_type, int count,
                    MissionType mission, Target target, Target destination) {
  const TechnoTypeClass* type_class =
      &AircraftTypeClass::As_Reference(aircraft_type);

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

    // Enter from the house's map edge, or the north one when its edge is not
    // one of the four.
    SourceType edge = house->Edge;
    switch (edge) {
      case SOURCE_NORTH:
      case SOURCE_EAST:
      case SOURCE_SOUTH:
      case SOURCE_WEST:
        break;

      case SourceType::SOURCE_NONE:
      case SourceType::SOURCE_SHIPPING:
      case SourceType::SOURCE_BEACH:
      case SourceType::SOURCE_AIR:
      case SourceType::SOURCE_VISIBLE:
      case SourceType::SOURCE_ENEMYBASE:
      case SourceType::SOURCE_HOMEBASE:
      case SourceType::SOURCE_OCEAN:
      case SourceType::SOURCE_COUNT:
      default:
        edge = SOURCE_NORTH;
        break;
    }
    const Cell entry_cell = TheMap().Calculated_Cell(edge, house->Class->House);

    // scenario_init places the aircraft on the edge cell without asking
    // whether it can enter it.
    TheWorld().scenario_init()++;
    const bool placed = aircraft->Unlimbo(entry_cell.Center(), game::DIR_N);
    TheWorld().scenario_init()--;
    if (placed) {
      // The aircraft is on the map: give it its orders. A default Target
      // means that none was given.
      aircraft->Assign_Mission(mission);

      if (destination != Target{}) {
        aircraft->Assign_Destination(destination);
      }

      if (target != Target{}) {
        aircraft->Assign_Target(target);
      }

      aircraft->Commence();
    } else {
      base::DeletePooled(aircraft);
      return placed_count;
    }
  }
  return placed_count;
}

}  // namespace td
