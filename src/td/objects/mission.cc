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

/* $Header:   F:\projects\c&c\vcs\code\mission.cpv   2.18   16 Oct 1995 16:49:12
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : MISSION.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 23, 1994 *
 *                                                                                             *
 *                  Last Update : June 25, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:
 *   MissionClass::AI -- Processes order script.
 *   MissionClass::Assign_Mission -- Give an order to a unit.
 *   MissionClass::Commence -- Start script with new order.
 *   MissionClass::Get_Mission -- Fetches the mission that this object is
 *     acting under.
 *   MissionClass::MissionClass -- Default constructor for the mission
 *     object type.
 *   MissionClass::Mission_From_Name -- Fetch order pointer from its name.
 *   MissionClass::Mission_Name -- Converts a mission number into an ASCII
 *     string.
 *   MissionClass::Overide_Mission -- temporarily overides the units mission
 *   MissionClass::Restore_Mission -- Restores overidden mission
 *   MissionClass::Set_Mission -- Sets the mission to the specified value.
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

/***********************************************************************************************
 * MissionClass::MissionClass -- Default constructor for the mission object
 *type.              *
 *                                                                                             *
 *    This is the default constructor for the mission class object. It sets the
 *mission        * handler into a default -- do nothing -- state. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/23/1995 JLB : Created. *
 *=============================================================================================*/
#include "td/objects/mission.h"

#include <string_view>

#include "absl/strings/match.h"
#include "base/enum_array.h"
#include "engine/platform/timer.h"
#include "td/core/defines.h"

namespace td {

MissionHandler::~MissionHandler() = default;

MissionClass::MissionClass() { timer_.Set(0); }

void MissionClass::Assign(MissionType order) {
  if (order == MISSION_NONE || current_ == order) {
    return;
  }
  queued_ = order;
}

bool MissionClass::Commence() {
  if (queued_ != MISSION_NONE) {
    current_ = queued_;
    queued_ = MISSION_NONE;

    /*
    **	Force immediate state machine processing at the first state machine
    **	state value.
    */
    timer_.Set(0);
    step_ = 0;
    return true;
  }
  return false;
}

// The timer is set after the mission function returns, so a function that
// shortens it, or commences another mission, still waits the delay it asks
// for.
void MissionClass::AI(MissionHandler& handler) {
  if (timer_.HasTimeLeft()) {
    return;
  }

  switch (current_) {
    case MissionType::MISSION_NONE:
    case MissionType::MISSION_COUNT:
    default:
    case MISSION_STICKY:
    case MISSION_SLEEP:
      timer_.Set(handler.Mission_Sleep());
      break;

    case MISSION_GUARD:
      timer_.Set(handler.Mission_Guard());
      break;

    case MISSION_ENTER:
      timer_.Set(handler.Mission_Enter());
      break;

    case MISSION_CONSTRUCTION:
      timer_.Set(handler.Mission_Construction());
      break;

    case MISSION_DECONSTRUCTION:
      timer_.Set(handler.Mission_Deconstruction());
      break;

    case MISSION_CAPTURE:
    case MISSION_SABOTAGE:
      timer_.Set(handler.Mission_Capture());
      break;

    case MISSION_MOVE:
      timer_.Set(handler.Mission_Move());
      break;

    case MISSION_ATTACK:
      timer_.Set(handler.Mission_Attack());
      break;

    case MISSION_RETREAT:
      timer_.Set(handler.Mission_Retreat());
      break;

    case MISSION_HARVEST:
      timer_.Set(handler.Mission_Harvest());
      break;

    case MISSION_GUARD_AREA:
      timer_.Set(handler.Mission_Guard_Area());
      break;

    case MISSION_RETURN:
      timer_.Set(handler.Mission_Return());
      break;

    case MISSION_STOP:
      timer_.Set(handler.Mission_Stop());
      break;

    case MISSION_AMBUSH:
      timer_.Set(handler.Mission_Ambush());
      break;

    case MISSION_HUNT:
    case MISSION_RESCUE:
      timer_.Set(handler.Mission_Hunt());
      break;

    case MISSION_TIMED_HUNT:
      timer_.Set(handler.Mission_Timed_Hunt());
      break;

    case MISSION_UNLOAD:
      timer_.Set(handler.Mission_Unload());
      break;

    case MISSION_REPAIR:
      timer_.Set(handler.Mission_Repair());
      break;

    case MISSION_MISSILE:
      timer_.Set(handler.Mission_Missile());
      break;
  }
}

/***********************************************************************************************
 * MissionClass::Mission_From_Name -- Fetch order pointer from its name. *
 *                                                                                             *
 *    This routine is used to convert an ASCII order name into the actual *
 *    order number it represents. Typically, this is used when processing * a
 *scenario INI file. *
 *                                                                                             *
 * INPUT:   name  -- The ASCII order name to process. *
 *                                                                                             *
 * OUTPUT:  Returns with the actual order number that the ASCII name *
 *          represents. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/07/1992 JLB : Created. * 04/22/1994 JLB : Converted to static
 *member function.                                     *
 *=============================================================================================*/
MissionType MissionClass::Mission_From_Name(std::string_view name) {
  if (!name.empty()) {
    for (MissionType order = MISSION_SLEEP; order < MISSION_COUNT; order++) {
      if (absl::EqualsIgnoreCase(Missions.at(order), name)) {
        return order;
      }
    }
  }
  return MISSION_NONE;
}

/***********************************************************************************************
 * MissionClass::Mission_Name -- Converts a mission number into an ASCII string.
 **
 *                                                                                             *
 *    Use this routine to convert a mission number into the ASCII string that
 *represents       * it. Typical use of this is when generating an INI file. *
 *                                                                                             *
 * INPUT:   mission  -- The mission number to convert. *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the ASCII string that represents the
 *mission type.       *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/23/1995 JLB : Created. *
 *=============================================================================================*/
const char* MissionClass::Mission_Name(MissionType mission) {
  return mission == MISSION_NONE ? "None" : Missions.at(mission);
}

/***********************************************************************************************
**	Unit order names. These names correspond to the player selectable orders
**	a unit can have. The system initiated orders have no use for the ASCII
*name *	associated, but they are listed here for completeness sake.
*/
base::EnumArray<MissionType, const char*, kMissionCount>
    MissionClass::Missions = {
        "Sleep",  "Attack",   "Move",         "Retreat", "Guard",
        "Sticky", "Enter",    "Capture",      "Harvest", "Area Guard",
        "Return", "Stop",     "Ambush",       "Hunt",    "Timed Hunt",
        "Unload", "Sabotage", "Construction", "Selling", "Repair",
        "Rescue", "Missile",
};

}  // namespace td
