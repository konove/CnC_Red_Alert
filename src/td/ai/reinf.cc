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

bool Do_Reinforcements(const TeamTypeClass* teamtype) {
  // A missing type, or one with no members, has nothing to bring on.
  if (!teamtype || !teamtype->ClassCount) {
    return false;
  }

  // The team that will control the members. A type without missions gets no
  // team, since the team would have nothing to tell them: its members are
  // given orders of their own when they are placed.
  TeamClass* team = nullptr;
  if (teamtype->MissionCount) {
    team = base::NewPooled<TeamClass>(teamtype,
                                      HouseClass::As_Pointer(teamtype->House));
    if (!team) {
      return false;
    }
    team->Force_Active();
  }

  // Find out what the type is made of. Every aircraft counts as a transport
  // here, whether or not it carries anything, because a group with an
  // aircraft in it arrives by air.
  bool okvoice = true;          // Announce the arrival to the player?
  bool airtransport = false;    // Does the type have an aircraft?
  bool watertransport = false;  // Is its last ground transport a hovercraft?
  bool onlytransport = true;    // Is there nothing but transports and aircraft?
  for (int index = 0; std::cmp_less(index, teamtype->ClassCount); index++) {
    if (base::At(teamtype->Class, index)->IsTransporter ||
        base::At(teamtype->Class, index)->What_Am_I() == RTTI_AIRCRAFTTYPE) {
      if (base::At(teamtype->Class, index)->What_Am_I() == RTTI_AIRCRAFTTYPE) {
        airtransport = true;
      } else {
        watertransport =
            dynamic_cast<const UnitTypeClass*>(base::At(teamtype->Class, index))
                ->Type == UNIT_HOVER;
      }
    } else {
      onlytransport = false;
    }
  }

  // Decide where the group comes from: by air with an aircraft, onto the beach
  // with a hovercraft, otherwise over the house's map edge.
  SourceType source = SOURCE_NONE;
  if (airtransport) {
    source = SOURCE_AIR;
  } else {
    if (watertransport) {
      source = SOURCE_BEACH;
    } else {
      // A gunboat sails the shipping lane instead. Only the first member is
      // looked at.
      if (base::At(teamtype->Class, 0)->What_Am_I() == RTTI_UNITTYPE &&
          dynamic_cast<const UnitTypeClass*>(base::At(teamtype->Class, 0))
                  ->Type == UNIT_GUNBOAT) {
        source = SOURCE_SHIPPING;
      } else {
        source = HouseClass::As_Pointer(teamtype->House)->Edge;
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
  TechnoClass* transport = nullptr;
  TechnoClass* object = nullptr;
  for (int index = 0; std::cmp_less(index, teamtype->ClassCount); index++) {
    const TechnoTypeClass* tclass = base::At(teamtype->Class, index);

    for (int sub = 0; std::cmp_less(sub, base::At(teamtype->DesiredNum, index));
         sub++) {
      TheWorld().scenario_init()++;
      auto* temp = dynamic_cast<FootClass*>(
          tclass->Create_One_Of(HouseClass::As_Pointer(teamtype->House)));
      TheWorld().scenario_init()--;

      if (temp) {
        // Add the member to the team, transports included; scenario_init makes
        // the team accept a member that is still in limbo. The hovercraft is
        // the exception: it never joins, since it leaves once it has unloaded.
        const auto* unit = dynamic_cast<UnitClass*>(temp);
        if (team && (unit == nullptr || *unit != UNIT_HOVER)) {
          TheWorld().scenario_init()++;
          team->Add(temp);
          TheWorld().scenario_init()--;
        }

        // A transporter goes on the transport list. In a group that flies in,
        // only the aircraft do: a ground transport rides along as cargo.
        if (tclass->IsTransporter &&
            (!airtransport || tclass->What_Am_I() == RTTI_AIRCRAFTTYPE)) {
          // An aircraft that carries the others is a loaner: it only delivers
          // them and then leaves the map. A transport that arrives empty is
          // itself the reinforcement, and a ground transport is always kept.
          if (!onlytransport && temp->What_Am_I() != RTTI_UNIT) {
            temp->IsALoaner = true;
          }

          // Push it on the front of the transport list.
          temp->Next = transport;
          transport = temp;
        } else {
          // A-10s are always loaners, since the player must never get to
          // control them.
          const auto* air = dynamic_cast<AircraftClass*>(temp);
          if (air != nullptr && *air == AIRCRAFT_A10) {
            temp->IsALoaner = true;
          }

          // Push it on the front of the list of other members.
          temp->Next = object;
          object = temp;
        }
      }
    }
  }

  // Nothing could be created, most likely because the object limit was
  // reached.
  if (!object && !transport) {
    base::DeletePooled(team);
    return false;
  }

  // Load everything else into the first transport. From here on `object` is
  // the list that goes on the map: the transports when there are any,
  // otherwise the members themselves.
  if (transport) {
    if (object) {
      // A cargo plane with a load is not announced when it is created; the
      // announcement comes when it unloads at the airstrip.
      if (transport->What_Am_I() == RTTI_AIRCRAFT &&
          *dynamic_cast<AircraftClass*>(transport) == AIRCRAFT_CARGO) {
        okvoice = false;
      }

      transport->Attach(dynamic_cast<FootClass*>(object));
    }
    object = transport;
  }

  // Put the list on the map, in the way its source calls for.
  bool placed = false;
  Cell cell{};
  switch (source) {
    // The gunboat starts at the east end of the shipping lane and sails west
    // to the map's left edge, along its own row. Calculated_Cell() returns
    // cell 0 when the map has no lane.
    //
    // TODO: only the head of the list is placed; a type with more than one
    // gunboat, or with other members, leaves the rest in limbo for good.
    case SOURCE_SHIPPING:
      cell = TheMap().Calculated_Cell(source, teamtype->House);
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
      const auto eface =
          static_cast<game::FacingType>(static_cast<int>(source) * 2);

      // `airtransport` is never set here: a group with an aircraft has
      // SOURCE_AIR as its source.

      if (airtransport) {
        TheWorld().scenario_init()++;
      }
      cell = TheMap().Calculated_Cell(source, teamtype->House);
      if (airtransport) {
        TheWorld().scenario_init()--;
      }
      Cell newcell = cell;

      // Place the members one at a time: `object` is the one being placed and
      // `o` the rest of the list. A cell index of 0 or less means there is no
      // cell. The aircraft branches below never run, for the reason above, so
      // `ok` stays true.
      auto* o = dynamic_cast<FootClass*>(object->Next);
      object->Next = nullptr;
      bool ok = true;
      while (newcell.index() > 0 && object) {
        game::DirType desiredfacing = game::Facing_Dir(eface);
        if (object->What_Am_I() == RTTI_AIRCRAFT) {
          desiredfacing = Random_Pick(game::DIR_N, kMaxRandomDirection);
        }

        if (ok && object->Unlimbo(newcell.Center(), desiredfacing)) {
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
              object->Assign_Destination(
                  Target::FromCellIndex(Adjacent_Cell(newcell, eface)));
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
    // TODO: a member that is not an aircraft ends the walk, and it and
    // everything after it stay in limbo for good. That takes a type with an
    // aircraft that is not a transport next to ground members, or a house
    // whose edge is SOURCE_AIR.
    case SOURCE_AIR: {
      auto* thisone = dynamic_cast<AircraftClass*>(object);
      while (thisone) {
        auto* next = dynamic_cast<AircraftClass*>(thisone->Next);

        // Pick the cell on the house's edge to enter at; scenario_init makes
        // Calculated_Cell() accept a cell that ground units could not use. A
        // cargo plane instead enters at the east edge, in the row of the
        // airstrip it will unload at, so that it flies straight in. The 2 is
        // added to the packed docking coordinate, which nudges it two leptons
        // in X and so leaves the row alone.
        TheWorld().scenario_init()++;
        Cell newcell = TheMap().Calculated_Cell(
            HouseClass::As_Pointer(teamtype->House)->Edge, teamtype->House);
        TheWorld().scenario_init()--;
        if (*thisone == AIRCRAFT_CARGO) {
          const BuildingClass* building =
              thisone->Find_Docking_Bay(STRUCT_AIRSTRIP, false);
          if (building) {
            newcell = Cell::FromXY(
                TheMap().MapCellX + TheMap().MapCellWidth,
                Coordinate::FromPacked(building->Docking_Coord().packed() + 2)
                    .cell_y());
          }
        }
        thisone->Next = nullptr;

        // scenario_init places the aircraft on the edge cell without asking
        // whether it can enter it.
        TheWorld().scenario_init()++;
        placed = thisone->Unlimbo(newcell.Center(), game::DIR_W);
        TheWorld().scenario_init()--;
        if (placed) {
          // The team handler gives a team member its orders. Without a team,
          // a fixed-wing aircraft hunts, and a helicopter flies to the
          // reinforcement waypoint, where a loaded transport unloads.
          if (!team) {
            if (thisone->Class->IsFixedWing) {
              thisone->Assign_Mission(MISSION_HUNT);
            } else {
              if (thisone->Class->IsTransporter &&
                  thisone->Is_Something_Attached()) {
                thisone->Assign_Mission(MISSION_UNLOAD);
              } else {
                thisone->Assign_Mission(MISSION_MOVE);
              }
              thisone->Assign_Destination(Target::FromCellIndex(
                  TheMap().Calculated_Cell(source, teamtype->House)));
            }
            thisone->Commence();
          }
        } else {
          base::DeletePooled(thisone);
        }

        thisone = next;
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
    // of the beach cell it will land on.
    //
    // TODO: only the head of the list is placed, and when no beach cell is
    // free nothing is: what is left stays in limbo for good.
    case SOURCE_OCEAN:
    case SOURCE_BEACH:
      cell = TheMap().Calculated_Cell(SOURCE_BEACH, teamtype->House);
      if (cell.index() != 0) {
        const Cell edge =
            Cell::FromXY(cell.x(), TheMap().MapCellY + TheMap().MapCellHeight);

        placed = object->Unlimbo(edge.Center(), game::DIR_N);
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
      }
      break;

    // TODO: a house whose edge is one of these has its reinforcements left
    // in limbo for good, and reported as delivered.
    case SourceType::SOURCE_NONE:
    case SourceType::SOURCE_VISIBLE:
    case SourceType::SOURCE_ENEMYBASE:
    case SourceType::SOURCE_HOMEBASE:
    case SourceType::SOURCE_COUNT:
    default:
      break;
  }

  // Announce the reinforcements to the player they belong to. This does not
  // depend on any member having reached the map.
  if (okvoice && teamtype->House == ThePlayer()->Class->House) {
    Speak(VOX_REINFORCEMENTS);
  }

  return true;
}

bool Create_Special_Reinforcement(const HouseClass* house,
                                  const TechnoTypeClass* type,
                                  const TechnoTypeClass* another,
                                  TeamMissionType mission, int argument) {
  if (house && type) {
    auto* team = base::NewPooled<TeamTypeClass>();

    if (team) {
      // A vehicle or soldier with no transport given drives on from the
      // house's map edge, if it can.
      if (!another && (type->What_Am_I() == RTTI_UNITTYPE ||
                       type->What_Am_I() == RTTI_INFANTRYTYPE)) {
        // Only the south edge is checked for a way in, because the hovercraft
        // can only arrive from the south. Yes, this is an arbitrary limitation,
        // but that's the way it is (for now). The way in is a column whose cell
        // below the map and whose bottom map cell are both clear; the last
        // column is not looked at.
        if (house->Edge == SOURCE_SOUTH) {
          bool found = false;
          for (int index = TheMap().MapCellX;
               index < TheMap().MapCellX + TheMap().MapCellWidth - 1; index++) {
            const Cell cell =
                Cell::FromXY(index, TheMap().MapCellY + TheMap().MapCellHeight);
            if (TheMap().at(cell).Is_Generally_Clear() &&
                TheMap().at(cell.Offset(-MAP_CELL_W)).Is_Generally_Clear()) {
              found = true;
              break;
            }
          }

          // No land route was found for the reinforcement to drive itself onto
          // the map. Assign it a hovercraft. This presumes that if the south
          // edge is non drivable, it will have water there instead. Risky, but
          // is not a problem with the current C&C maps. The mission the caller
          // asked for is dropped: the hovercraft delivers and that is all.
          if (!found) {
            mission = TMISSION_NONE;
            another = &UnitTypeClass::As_Reference(UNIT_HOVER);
          }
        }

        // Otherwise it drives to the reinforcement waypoint, whatever mission
        // the caller asked for.
        if (!another) {
          mission = TMISSION_MOVECELL;
          argument =
              TheMap().Calculated_Cell(SOURCE_AIR, house->Class->House).index();
        }
      }

      // Anything else that comes without a transport and without a mission is
      // sent to the reinforcement waypoint too. If not, the reinforcement will
      // just sit at the edge of the map.
      if (!another && mission == TMISSION_NONE) {
        mission = TMISSION_MOVECELL;
        argument =
            TheMap().Calculated_Cell(SOURCE_AIR, house->Class->House).index();
      }

      // Fill in the team type: transient, so that it is deleted along with its
      // last team. It always has one mission slot, so Do_Reinforcements()
      // always makes a team for it. The slot stays TMISSION_NONE for a
      // hovercraft delivery; any other transport without a mission unloads at
      // the reinforcement waypoint.
      base::SafeCopy(team->IniName, "TEMP");
      team->IsReinforcable = false;
      team->IsTransient = true;
      team->ClassCount = 1;
      base::At(team->Class, 0) = type;
      base::At(team->DesiredNum, 0) = 1;
      team->MissionCount = 1;
      if (mission == TMISSION_NONE) {
        if (another &&
            (another->What_Am_I() != RTTI_UNITTYPE ||
             dynamic_cast<const UnitTypeClass*>(another)->Type != UNIT_HOVER)) {
          base::At(team->MissionList, 0).Mission = TMISSION_UNLOAD;
          base::At(team->MissionList, 0).Argument = kWayptReinf;
        }
      } else {
        base::At(team->MissionList, 0).Mission = mission;
        base::At(team->MissionList, 0).Argument = argument;
      }
      team->House = house->Class->House;
      if (another) {
        team->ClassCount++;
        base::At(team->Class, 1) = another;
        base::At(team->DesiredNum, 1) = 1;
      }

      // TODO: when Do_Reinforcements() fails after making its team, deleting
      // that team has already deleted the transient type, and this deletes it
      // a second time. Multiplayer games skip the delete, so there a type
      // whose team could not be made is never freed.
      const bool ok = Do_Reinforcements(team);
      if (!ok && TheSession().type() == GAME_NORMAL) {
        base::DeletePooled(team);
      }
      return ok;
    }
  }
  return false;
}

int Create_Air_Reinforcement(HouseClass* house, AircraftType air, int number,
                             MissionType mission, Target tarcom,
                             Target navcom) {
  const TechnoTypeClass* type = &AircraftTypeClass::As_Reference(air);

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

    // Enter from the house's map edge, or the north one when its edge is not
    // one of the four.
    SourceType source = house->Edge;
    switch (source) {
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
        source = SOURCE_NORTH;
        break;
    }
    const Cell newcell = TheMap().Calculated_Cell(source, house->Class->House);

    // scenario_init places the aircraft on the edge cell without asking
    // whether it can enter it.
    TheWorld().scenario_init()++;
    const bool placed = obj->Unlimbo(newcell.Center(), game::DIR_N);
    TheWorld().scenario_init()--;
    if (placed) {
      // The aircraft is on the map: give it its orders. A default Target
      // means that none was given.
      obj->Assign_Mission(mission);

      if (navcom != Target{}) {
        obj->Assign_Destination(navcom);
      }

      if (tarcom != Target{}) {
        obj->Assign_Target(tarcom);
      }

      obj->Commence();
    } else {
      base::DeletePooled(obj);
      return sub;
    }
  }
  return sub;
}

}  // namespace td
