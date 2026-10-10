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

/* $Header: /CounterStrike/MISSION.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
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
 *                  Last Update : September 14, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * MissionClass::AI -- Processes order script. *
 *   MissionClass::Assign_Mission -- Give an order to a unit. *
 *   MissionClass::Commence -- Start script with new order. *
 *   MissionClass::Get_Mission -- Fetches the mission that this object is acting
 *   under. * MissionClass::MissionClass -- Default constructor for the mission
 *   object type. * MissionClass::Mission_??? -- Stub mission functions that do
 *   nothing. * MissionClass::Mission_From_Name -- Fetch order pointer from its
 *   name. * MissionClass::Mission_Name -- Converts a mission number into an
 *   ASCII string. * MissionClass::Override_Mission -- temporarily overrides the
 *   units mission * MissionClass::Restore_Mission -- Restores overridden
 * mission
 *   * MissionClass::Set_Mission -- Sets the mission to the specified value. *
 *   MissionClass::Is_Recruitable_Mission -- Determines if this mission is
 *   recruitable for a te*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/objects/mission.h"

#include <string_view>

#include "absl/strings/match.h"
#include "base/fixed.h"
#include "engine/file/ini_file.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/core/const.h"
#include "ra/core/defines.h"
#include "ra/data/rules.h"

namespace ra {

MissionHandler::~MissionHandler() = default;

void MissionClass::Assign(MissionType order) {
  /*
  **	Ensure that a MISSION_QMOVE is translated into a MISSION_MOVE.
  */
  if (order == MISSION_QMOVE) {
    order = MISSION_MOVE;
  }

  if (order != MISSION_NONE && current_ != order) {
    queued_ = order;
  }
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
  if (!timer_.IsFinished()) {
    return;
  }

  switch (current_) {
    case MISSION_HARMLESS:
    case MISSION_SLEEP:
    case MissionType::MISSION_NONE:
    default:
      timer_.Set(handler.Mission_Sleep());
      break;

    case MISSION_STICKY:
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

    case MISSION_QMOVE:
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
MissionType MissionClass::Mission_From_Name(const std::string_view name) {
  if (!name.empty()) {
    for (const MissionType order : magic_enum::enum_values<MissionType>()) {
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
  if (mission != MISSION_NONE) {
    return Missions.at(mission);
  }
  return "None";
}

/***********************************************************************************************
 * MissionClass::Is_Recruitable_Mission -- Determines if this mission is
 *recruitable for a tea *
 *                                                                                             *
 *    Some missions preclude recruitment into a team. This routine will examine
 *the mission    * specified and if not allowed for a team, it will return
 *false.                           *
 *                                                                                             *
 * INPUT:   mission  -- The mission type to examine. *
 *                                                                                             *
 * OUTPUT:  bool; Is an object following this mission allowed to be recruited
 *into a team?     *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/14/1996 JLB : Created. *
 *=============================================================================================*/
bool MissionClass::Is_Recruitable_Mission(MissionType mission) {
  if (mission == MISSION_NONE) {
    return true;
  }
  return TheRules().mission_control().at(mission).IsRecruitable;
}

MissionControlClass::MissionControlClass()
    : Rate(Fixed::FromString(".016")), AARate(Fixed::FromString(".016")) {}

const char* MissionControlClass::Name() const {
  if (Mission == MISSION_NONE) {
    return "<none>";
  }
  return Missions.at(Mission);
}

bool MissionControlClass::Read_INI(engine::file::IniFile& ini) {
  if (ini.HasSection(Name())) {
    IsNoThreat = ini.GetBool(Name(), "NoThreat", IsNoThreat);
    IsZombie = ini.GetBool(Name(), "Zombie", IsZombie);
    IsRecruitable = ini.GetBool(Name(), "Recruitable", IsRecruitable);
    IsParalyzed = ini.GetBool(Name(), "Paralyzed", IsParalyzed);
    IsRetaliate = ini.GetBool(Name(), "Retaliate", IsRetaliate);
    IsScatter = ini.GetBool(Name(), "Scatter", IsScatter);
    Rate = ini.GetFixed(Name(), "Rate", Rate);
    AARate = ini.GetFixed(Name(), "AARate", Fixed(0));
    if (AARate == 0) {
      AARate = Rate;
    }
    return true;
  }
  return false;
}

}  // namespace ra
