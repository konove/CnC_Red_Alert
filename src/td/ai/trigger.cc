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

/* $Header:   F:\projects\c&c\vcs\code\trigger.cpv   2.17   16 Oct 1995 16:51:20
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : TRIGGER.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 11/12/94 *
 *                                                                                             *
 *                  Last Update : August 27, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Do_All_To_Hunt -- Forces all computer controlled units into hunt
 *mode.                    * TriggerClass::Action_From_Name -- retrieves
 *ActionType for given name                     * TriggerClass::Action_Need_Team
 *-- Determines if this action event requires a team.        *
 *   TriggerClass::As_Pointer -- returns pointer for the given trigger name *
 *   TriggerClass::As_Target -- Converts trigger to a target value *
 *   TriggerClass::Event_From_Name -- retrieves EventType for given name *
 *   TriggerClass::Event_Need_Data -- Determines if this event requires a data
 *value.          * TriggerClass::Event_Need_House -- Determines if this event
 *requires a house identifier.   * TriggerClass::Event_Need_Object -- Determines
 *if the specified event requires an object.  * TriggerClass::Init -- clears
 *triggers for new scenario                                    *
 *   TriggerClass::Name_From_Action -- retrieves name for ActionType *
 *   TriggerClass::Name_From_Event -- retrieves name for EventType *
 *   TriggerClass::Read_INI -- reads triggers from the INI file *
 *   TriggerClass::Remove -- removes this trigger from the game *
 *   TriggerClass::Spring -- Trigger processing routine for cell-based triggers
 ** TriggerClass::Spring -- Trigger processing routine for house-based triggers
 ** TriggerClass::Spring -- Trigger processing routine for object-based triggers
 ** TriggerClass::TriggerClass -- constructor * TriggerClass::Validate --
 *validates trigger pointer
 ** TriggerClass::Write_INI -- writes triggers to the INI file *
 *   TriggerClass::operator delete -- 'delete' operator * TriggerClass::operator
 *new -- 'new' operator                                              *
 *   TriggerClass::~TriggerClass -- Destructor for trigger objects. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/ai/trigger.h"

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <vector>

#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "base/algorithm.h"
#include "base/array.h"
#include "base/pooled.h"
#include "base/strings/number_parse.h"
#include "base/strings/tokenizer.h"
#include "engine/file/ini_file.h"
#include "engine/platform/timer.h"
#include "game/core/object_heap.h"
#include "td/ai/reinforcements.h"
#include "td/ai/team.h"
#include "td/ai/teamtype.h"
#include "td/core/config.h"
#include "td/core/defines.h"
#include "td/core/object_heaps.h"
#include "td/editor/mapedit.h"
#include "td/map/world.h"
#include "td/objects/anim.h"
#include "td/objects/building.h"
#include "td/objects/cell.h"
#include "td/objects/house.h"
#include "td/objects/infantry.h"
#include "td/objects/object.h"
#include "td/objects/super.h"
#include "td/objects/techno.h"
#include "td/objects/terrain.h"
#include "td/objects/type.h"
#include "td/objects/unit.h"
#include "td/startup/conquer.h"
#include "td/startup/game_state.h"

namespace td {

static void Do_All_To_Hunt();

// #define FIXUP 0

/*
********************************** Globals **********************************
*/
static const char* EventText[static_cast<int>(EVENT_COUNT) + 1] = {
    "None",          "Player Enters", "Discovered",    "Attacked",
    "Destroyed",     "Any",           "House Discov.", "Units Destr.",
    "Bldgs Destr.",  "All Destr.",    "Credits",       "Time",
    "# Bldgs Dstr.", "# Units Dstr.", "No Factories",  "Civ. Evac.",
    "Built It"};

static const char* ActionText[static_cast<int>(TriggerClass::ACTION_COUNT) +
                              1] = {"None",
                                    "Win",
                                    "Lose",
                                    "Production",
                                    "Create Team",
                                    "Dstry Teams",
                                    "All to Hunt",
                                    "Reinforce.",
                                    "DZ at 'Z'",
                                    "Airstrike",
                                    "Nuclear Missile",
                                    "Ion Cannon",
                                    "Dstry Trig 'XXXX'",
                                    "Dstry Trig 'YYYY'",
                                    "Dstry Trig 'ZZZZ'",
                                    "Autocreate",
                                    "Cap=Win/Des=Lose",
                                    "Allow Win"};

/***********************************************************************************************
 * TriggerClass::Validate -- validates trigger pointer
 **
 *                                                                                             *
 * INPUT: * none.
 **
 *                                                                                             *
 * OUTPUT: * 1 = ok, 0 = error
 **
 *                                                                                             *
 * WARNINGS: * none.
 **
 *                                                                                             *
 * HISTORY: * 08/09/1995 BRR : Created. *
 *=============================================================================================*/
int TriggerClass::Validate() const {
  if constexpr (config::kCheatKeysEnabled) {
    const int num = TheObjectHeaps().trigger().SlotIndexOf(this);
    if (num < 0 || num >= kTriggerMax) {
      Validate_Error("TRIGGER");
    }
    return 1;
  } else {
    return 1;
  }
}

/***********************************************************************************************
 * TriggerClass::Event_Need_Object -- Determines if the specified event requires
 *an object.    *
 *                                                                                             *
 *    This routine determines if the specified event must be attached to an
 *object. Such       * events can only exist in a parasitic fashion attached to
 *object(s) in the game.          *
 *                                                                                             *
 * INPUT:   event -- The event type to examine. *
 *                                                                                             *
 * OUTPUT:  Does the specified event require attachement to an object? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/27/1995 JLB : Created. *
 *=============================================================================================*/
bool TriggerClass::Event_Need_Object(EventType event) {
  switch (event) {
    case EVENT_PLAYER_ENTERED:
    case EVENT_DISCOVERED:
    case EVENT_ATTACKED:
    case EVENT_DESTROYED:
    case EVENT_ANY:
      return true;
    case EventType::EVENT_NONE:
    case EventType::EVENT_HOUSE_DISCOVERED:
    case EventType::EVENT_UNITS_DESTROYED:
    case EventType::EVENT_BUILDINGS_DESTROYED:
    case EventType::EVENT_ALL_DESTROYED:
    case EventType::EVENT_CREDITS:
    case EventType::EVENT_TIME:
    case EventType::EVENT_NBUILDINGS_DESTROYED:
    case EventType::EVENT_NUNITS_DESTROYED:
    case EventType::EVENT_NOFACTORIES:
    case EventType::EVENT_EVAC_CIVILIAN:
    case EventType::EVENT_BUILD:
    case EventType::EVENT_COUNT:
    default:
      break;
  }
  return false;
}

/***********************************************************************************************
 * TriggerClass::Event_Need_House -- Determines if this event requires a house
 *identifier.     *
 *                                                                                             *
 *    This routine is used to determine if the specified event requires a house
 *identifier.    * All trigger events that affect a house will require a house
 *identifier.                  *
 *                                                                                             *
 * INPUT:   event -- The event type to examine. *
 *                                                                                             *
 * OUTPUT:  Does the specified event type require a house identifier? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/27/1995 JLB : Created. *
 *=============================================================================================*/
bool TriggerClass::Event_Need_House(EventType event) {
  switch (event) {
    case EVENT_PLAYER_ENTERED:
    case EVENT_HOUSE_DISCOVERED:
    case EVENT_UNITS_DESTROYED:
    case EVENT_BUILDINGS_DESTROYED:
    case EVENT_ALL_DESTROYED:
    case EVENT_CREDITS:
    case EVENT_TIME:
    case EVENT_NBUILDINGS_DESTROYED:
    case EVENT_NUNITS_DESTROYED:
    case EVENT_NOFACTORIES:
    case EVENT_EVAC_CIVILIAN:
    case EVENT_BUILD:
      return true;
    case EventType::EVENT_NONE:
    case EventType::EVENT_DISCOVERED:
    case EventType::EVENT_ATTACKED:
    case EventType::EVENT_DESTROYED:
    case EventType::EVENT_ANY:
    case EventType::EVENT_COUNT:
    default:
      break;
  }
  return false;
}

/***********************************************************************************************
 * TriggerClass::Event_Need_Data -- Determines if this event requires a data
 *value.            *
 *                                                                                             *
 *    This routine will determine if the specified event requires a data number
 *parameter.     * This is commonly needed for trigger events. *
 *                                                                                             *
 * INPUT:   event -- The event to examine. *
 *                                                                                             *
 * OUTPUT:  Does the specified event require a data number parameter? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/27/1995 JLB : Created. *
 *=============================================================================================*/
bool TriggerClass::Event_Need_Data(EventType event) {
  switch (event) {
    case EVENT_CREDITS:
    case EVENT_TIME:
    case EVENT_NBUILDINGS_DESTROYED:
    case EVENT_NUNITS_DESTROYED:
    case EVENT_BUILD:
      return true;
    case EventType::EVENT_NONE:
    case EventType::EVENT_PLAYER_ENTERED:
    case EventType::EVENT_DISCOVERED:
    case EventType::EVENT_ATTACKED:
    case EventType::EVENT_DESTROYED:
    case EventType::EVENT_ANY:
    case EventType::EVENT_HOUSE_DISCOVERED:
    case EventType::EVENT_UNITS_DESTROYED:
    case EventType::EVENT_BUILDINGS_DESTROYED:
    case EventType::EVENT_ALL_DESTROYED:
    case EventType::EVENT_NOFACTORIES:
    case EventType::EVENT_EVAC_CIVILIAN:
    case EventType::EVENT_COUNT:
    default:
      break;
  }
  return false;
}

/***********************************************************************************************
 * TriggerClass::Action_Need_Team -- Determines if this action event requires a
 *team.          *
 *                                                                                             *
 *    This routine will determine if the specified action requires a team name
 *parameter.      * Typically, this is needed for reinforcements or other
 *trigger events that affect         * a particular team type. *
 *                                                                                             *
 * INPUT:   action   -- The action that is to be examined. *
 *                                                                                             *
 * OUTPUT:  Does the specified action require a team type name? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/27/1995 JLB : Created. *
 *=============================================================================================*/
bool TriggerClass::Action_Need_Team(ActionType action) {
  switch (action) {
    case ACTION_CREATE_TEAM:
    case ACTION_DESTROY_TEAM:
    case ACTION_REINFORCEMENTS:
      return true;
    case ActionType::ACTION_NONE:
    case ActionType::ACTION_WIN:
    case ActionType::ACTION_LOSE:
    case ActionType::ACTION_BEGIN_PRODUCTION:
    case ActionType::ACTION_ALL_HUNT:
    case ActionType::ACTION_DZ:
    case ActionType::ACTION_AIRSTRIKE:
    case ActionType::ACTION_NUKE:
    case ActionType::ACTION_ION:
    case ActionType::ACTION_DESTROY_XXXX:
    case ActionType::ACTION_DESTROY_YYYY:
    case ActionType::ACTION_DESTROY_ZZZZ:
    case ActionType::ACTION_AUTOCREATE:
    case ActionType::ACTION_WINLOSE:
    case ActionType::ACTION_ALLOWWIN:
    case ActionType::ACTION_COUNT:
    default:
      break;
  }
  return false;
}

/***********************************************************************************************
 * TriggerClass::TriggerClass -- constructor *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 11/28/1994 BR : Created. *
 *=============================================================================================*/

/***********************************************************************************************
 * TriggerClass::~TriggerClass -- Destructor for trigger objects. *
 *                                                                                             *
 *    This destructor will update the house blockage value if necessary. No
 *other action need  * be performed on trigger destruction. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/29/1995 JLB : Created. *
 *=============================================================================================*/
TriggerClass::~TriggerClass() {
  if (TheGameState().active() && House != HOUSE_NONE &&
      Action == ACTION_ALLOWWIN) {
    if (HouseClass::As_Pointer(House)->Blockage) {
      HouseClass::As_Pointer(House)->Blockage--;
    }
    HouseClass::As_Pointer(House)->BorrowedTime.Set(int64_t{kTicksPerSecond} *
                                                    4);
  }
}

/***********************************************************************************************
 * TriggerClass::Init -- clears triggers for new scenario *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 11/29/1994 BR : Created. *
 *=============================================================================================*/
void TriggerClass::Init() { TheObjectHeaps().trigger().FreeAll(); }

/***********************************************************************************************
 * TriggerClass::Spring -- Trigger processing routine *
 *                                                                                             *
 * Checks whether this trigger should "spring" for the given event & object; *
 * If it should, then some really cool undocumented stuff magically happens. *
 *                                                                                             *
 * INPUT: * event      EventType: What happened? * object   Ptr to object
 *containing this trigger: What did it happen to?                 *
 *                                                                                             *
 * OUTPUT: * 0 = nothing happened; 1 = the trigger was sprung *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 12/06/1994 BR : Created. * 06/25/1995 JLB : Added more trigger
 *events.                                               *
 *=============================================================================================*/
bool TriggerClass::Spring(EventType event, ObjectClass* obj) {
  Validate();
  /*
  **	If this is not the event for this trigger, just return.
  */
  if (event != Event && Event != EVENT_ANY) {
    return false;
  }

  /*
  **	If time-based, decrement the minute counter; return if it's not time yet
  */
  if (Event == EVENT_TIME) {
    Data--;
    if (Data > 0) {
      return false;
    }
    Data = DataCopy;
  }

  /*
  **	Semi-persistant trigger: first detach it from the calling object, then
  **	see if this is the last object we're attached to; if so, the trigger
  **	will spring.
  */
  if (IsPersistant == SEMIPERSISTANT) {
    /*
    ** Detach ourselves from the object
    */
    obj->Trigger = nullptr;

    /*
    ** Decrement our attachment counter
    */
    AttachCount--;

    /*
    ** If we're attached to more objects, don't spring; otherwise, spring.
    ** And, mark ourselves as volatile so we'll completely remove ourselves
    ** from the game after we go off.
    */
    if (AttachCount > 0) {
      return false;
    }
    IsPersistant = VOLATILE;
  }

  /*
  **	Otherwise, take an appropriate action.
  */
  bool success = true;
  TriggerClass* trig = nullptr;
  switch (Action) {
    case ACTION_NUKE:
      HouseClass::As_Pointer(HOUSE_BAD)->NukeStrike.Enable(true, false);
      HouseClass::As_Pointer(HOUSE_BAD)->NukeStrike.Forced_Charge(
          ThePlayer()->Class->House == HOUSE_BAD);
      break;

    case ACTION_ION:
      HouseClass::As_Pointer(HOUSE_GOOD)->IonCannon.Enable(true, false);
      HouseClass::As_Pointer(HOUSE_GOOD)
          ->IonCannon.Forced_Charge(ThePlayer()->Class->House == HOUSE_GOOD);
      break;

    case ACTION_WINLOSE:
      switch (event) {
        case EVENT_DESTROYED:
          if (!ThePlayer()->IsToWin || ThePlayer()->Blockage > 0) {
            ThePlayer()->Flag_To_Lose();
          }
          success = true;
          break;

        case EVENT_PLAYER_ENTERED:
          if (!ThePlayer()->IsToLose) {
            ThePlayer()->Flag_To_Win();
          }
          success = true;
          break;

        case EventType::EVENT_NONE:
        case EventType::EVENT_DISCOVERED:
        case EventType::EVENT_ATTACKED:
        case EventType::EVENT_ANY:
        case EventType::EVENT_HOUSE_DISCOVERED:
        case EventType::EVENT_UNITS_DESTROYED:
        case EventType::EVENT_BUILDINGS_DESTROYED:
        case EventType::EVENT_ALL_DESTROYED:
        case EventType::EVENT_CREDITS:
        case EventType::EVENT_TIME:
        case EventType::EVENT_NBUILDINGS_DESTROYED:
        case EventType::EVENT_NUNITS_DESTROYED:
        case EventType::EVENT_NOFACTORIES:
        case EventType::EVENT_EVAC_CIVILIAN:
        case EventType::EVENT_BUILD:
        case EventType::EVENT_COUNT:
        default:
          success = false;
          break;
      }
      break;

    case ACTION_DESTROY_XXXX:
      trig = As_Pointer("XXXX");
      if (trig) {
        trig->Remove();
      }
      base::DeletePooled(trig);
      break;

    case ACTION_DESTROY_YYYY:
      trig = As_Pointer("YYYY");
      if (trig) {
        trig->Remove();
      }
      base::DeletePooled(trig);
      break;

    case ACTION_DESTROY_ZZZZ:
      trig = As_Pointer("ZZZZ");
      if (trig) {
        trig->Remove();
      }
      base::DeletePooled(trig);
      break;

    case ACTION_AIRSTRIKE:
      ThePlayer()->IsAirstrikePending = true;
      //			PlayerPtr->Make_Air_Strike_Available(true);
      break;

    case ACTION_DZ:
      base::NewPooled<AnimClass>(ANIM_LZ_SMOKE,
                                 base::At(TheWorld().waypoint(), 25).Center());
      break;

    case ACTION_NONE:
      break;

    case ACTION_WIN:
      ThePlayer()->Flag_To_Win();
      break;

    case ACTION_LOSE:
      ThePlayer()->Flag_To_Lose();
      break;

    case ACTION_BEGIN_PRODUCTION:
      HouseClass::As_Pointer(House)->Begin_Production();
      break;

    case ACTION_AUTOCREATE:
      if (obj && obj->Is_Techno()) {
        dynamic_cast<TechnoClass*>(obj)->House->IsAlerted = true;
      }
      break;

    case ACTION_CREATE_TEAM:
      if (Team) {
        TheWorld().scenario_init()++;
        Team->CreateTeam();
        TheWorld().scenario_init()--;
      }
      break;

    case ACTION_DESTROY_TEAM:
      if (Team) {
        Team->DestroyTeams();
      }
      break;

    case ACTION_REINFORCEMENTS:
      if (Team) {
        success = DeliverReinforcements(Team);
      }
      break;

    case ACTION_ALL_HUNT:
      Do_All_To_Hunt();
      break;

    case ActionType::ACTION_ALLOWWIN:
    case ActionType::ACTION_COUNT:
    default:
      break;
  }

  if (!success && Event == EVENT_TIME) {
    Data = 1;
  }

  /*
  **	Remove trigger from the game.
  */
  if (success && IsPersistant == VOLATILE) {
    Remove();
  }

  return true;
}

/***********************************************************************************************
 * TriggerClass::Spring -- Trigger processing routine *
 *                                                                                             *
 * This version of Spring is for cell-based triggers. *
 *                                                                                             *
 * INPUT: * data      elapsed time, or credits, depending on what 'Event' is. *
 *                                                                                             *
 * OUTPUT: * 0 = nothing happened; 1 = the trigger was sprung *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 12/06/1994 BR : Created. *
 *=============================================================================================*/
bool TriggerClass::Spring(EventType event, Cell cell) {
  Validate();
  /*
  **	If this is not the event for this trigger, just return.
  */
  if (event != Event) {
    return false;
  }

  /*
  **	If time-based, decrement the minute counter; return if it's not time yet
  */
  if (Event == EVENT_TIME) {
    Data--;
    if (Data > 0) {
      return false;
    }
    Data = DataCopy;
  }

  /*
  **	Semi-persistant trigger: first detach it from the calling cell, then
  **	see if this is the last cell we're attached to; if so, the trigger
  **	will spring.
  */
  if (IsPersistant == SEMIPERSISTANT) {
    /*
    ** Detach ourselves from the cell
    */
    TheMap().at(cell).IsTrigger = false;

    /*
    ** Decrement our attachment counter
    */
    AttachCount--;

    /*
    ** If we're attached to more cells, don't spring; otherwise, spring.
    ** And, mark ourselves as volatile so we'll completely remove ourselves
    ** from the game after we go off.
    */
    if (AttachCount > 0) {
      return false;
    }
    IsPersistant = VOLATILE;
  }

  /*
  **	Otherwise, take an appropriate action.
  */
  bool success = true;
  TriggerClass* trig = nullptr;
  switch (Action) {
    case ACTION_NUKE:
      HouseClass::As_Pointer(HOUSE_BAD)->NukeStrike.Enable(true, false);
      HouseClass::As_Pointer(HOUSE_BAD)->NukeStrike.Forced_Charge(
          ThePlayer()->Class->House == HOUSE_BAD);
      break;

    case ACTION_ION:
      HouseClass::As_Pointer(HOUSE_GOOD)->IonCannon.Enable(true, false);
      HouseClass::As_Pointer(HOUSE_GOOD)
          ->IonCannon.Forced_Charge(ThePlayer()->Class->House == HOUSE_GOOD);
      break;

    case ACTION_AUTOCREATE:
      for (int index = 0; index < TheObjectHeaps().house().Count(); index++) {
        TheObjectHeaps().house().ActiveAt(index)->IsAlerted = true;
      }
      break;

    case ACTION_DESTROY_XXXX:
      trig = As_Pointer("XXXX");
      if (trig) {
        trig->Remove();
      }
      base::DeletePooled(trig);
      break;

    case ACTION_DESTROY_YYYY:
      trig = As_Pointer("YYYY");
      if (trig) {
        trig->Remove();
      }
      base::DeletePooled(trig);
      break;

    case ACTION_DESTROY_ZZZZ:
      trig = As_Pointer("ZZZZ");
      if (trig) {
        trig->Remove();
      }
      base::DeletePooled(trig);
      break;

    case ACTION_AIRSTRIKE:
      HouseClass::As_Pointer(House)->AirStrike.Enable(false, true);
      if (House == ThePlayer()->Class->House) {
        ThePlayer()->AirStrike.Forced_Charge(true);
        TheMap().Add(RTTI_SPECIAL, static_cast<int>(SPC_AIR_STRIKE));
        base::At(TheMap().Column, 1).Flag_To_Redraw();
      }
      //			PlayerPtr->Make_Air_Strike_Available(true);
      break;

    case ACTION_DZ:
      base::NewPooled<AnimClass>(ANIM_LZ_SMOKE,
                                 base::At(TheWorld().waypoint(), 25).Center());
      break;

    case ACTION_NONE:
      break;

    case ACTION_WIN:
      ThePlayer()->Flag_To_Win();
      break;

    case ACTION_LOSE:
      ThePlayer()->Flag_To_Lose();
      break;

    case ACTION_BEGIN_PRODUCTION:
      if (ThePlayer()->Class->House == HOUSE_GOOD) {
        HouseClass::As_Pointer(HOUSE_BAD)->Begin_Production();
      } else {
        HouseClass::As_Pointer(HOUSE_GOOD)->Begin_Production();
      }
      break;

    case ACTION_CREATE_TEAM:
      if (Team) {
        TheWorld().scenario_init()++;
        Team->CreateTeam();
        TheWorld().scenario_init()--;
      }
      break;

    case ACTION_DESTROY_TEAM:
      if (Team) {
        Team->DestroyTeams();
      }
      break;

    case ACTION_REINFORCEMENTS:
      if (Team) {
        success = DeliverReinforcements(Team);
      }
      break;

    case ACTION_ALL_HUNT:
      Do_All_To_Hunt();
      break;

    case ActionType::ACTION_WINLOSE:
    case ActionType::ACTION_ALLOWWIN:
    case ActionType::ACTION_COUNT:
    default:
      break;
  }

  if (!success && Event == EVENT_TIME) {
    Data = 1;
  }

  /*
  **	Remove trigger from the game.
  */
  if (success && IsPersistant == VOLATILE) {
    Remove();
  }

  return true;
}

/***********************************************************************************************
 * TriggerClass::Spring -- Trigger processing routine *
 *                                                                                             *
 * This version of Spring is for house-specific triggers. * For a time-based
 *trigger, 'data' will the the current tick count.                            *
 * For a credit-based trigger, 'data' will be the credits for the HouseClass *
 * containing this trigger. *
 *                                                                                             *
 * INPUT: * event      the event that happened * house      house that this
 *event relates to                                            * data elapsed
 *time, or credits, depending on what 'Event' is.                      *
 *                                                                                             *
 * OUTPUT: * 0 = nothing happened; 1 = the trigger was sprung *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 12/06/1994 BR : Created. * 06/25/1995 JLB : Added more trigger
 *events.                                               *
 *=============================================================================================*/
bool TriggerClass::Spring(EventType event, HousesType house, int64_t data) {
  Validate();
  /*
  **	If this is not the event for this trigger, just return.
  */
  if (event != Event || house != House) {
    return false;
  }

  /*
  **	If credits-based, check 'data'
  */
  if (Event == EVENT_CREDITS && data < Data) {
    return false;
  }

  /*
  **	Building event check to ensure that the building number matches.
  */
  if (Event == EVENT_BUILD && data != Data) {
    return false;
  }

  /*
  **	Number of objects destroyed checker. If the data supplied indicates that
  **	the correct number of objects have been destroyed, then this trigger
  **	will succeed.
  */
  if ((Event == EVENT_NBUILDINGS_DESTROYED ||
       Event == EVENT_NUNITS_DESTROYED) &&
      (data < Data)) {
    return false;
  }

  /*
  **	If time-based, decrement the minute counter; return if it's not time yet
  */
  if (Event == EVENT_TIME) {
    Data--;
    if (Data > 0) {
      return false;
    }
    Data = DataCopy;
  }

  /*
  **	The trigger has gone off; take appropriate action
  */
  bool success = true;
  TriggerClass* trig = nullptr;
  switch (Action) {
    case ACTION_NUKE:
      HouseClass::As_Pointer(HOUSE_BAD)->NukeStrike.Enable(true, false);
      HouseClass::As_Pointer(HOUSE_BAD)->NukeStrike.Forced_Charge(
          ThePlayer()->Class->House == HOUSE_BAD);
      break;

    case ACTION_ION:
      HouseClass::As_Pointer(HOUSE_GOOD)->IonCannon.Enable(true, false);
      HouseClass::As_Pointer(HOUSE_GOOD)
          ->IonCannon.Forced_Charge(ThePlayer()->Class->House == HOUSE_GOOD);
      break;

    /*
    **	This will remove a blockage to the win condition. No action need
    **	be performed here since the act of deleting the trigger will
    **	remove the blockage.
    */
    case ACTION_ALLOWWIN:
      break;

    case ACTION_AUTOCREATE:
      HouseClass::As_Pointer(House)->IsAlerted = true;
      break;

    case ACTION_DESTROY_XXXX:
      trig = As_Pointer("XXXX");
      if (trig) {
        trig->Remove();
      }
      base::DeletePooled(trig);
      break;

    case ACTION_DESTROY_YYYY:
      trig = As_Pointer("YYYY");
      if (trig) {
        trig->Remove();
      }
      base::DeletePooled(trig);
      break;

    case ACTION_DESTROY_ZZZZ:
      trig = As_Pointer("ZZZZ");
      if (trig) {
        trig->Remove();
      }
      base::DeletePooled(trig);
      break;

    case ACTION_AIRSTRIKE:
      ThePlayer()->AirStrike.Enable(false, true);
      if (House == ThePlayer()->Class->House) {
        ThePlayer()->AirStrike.Forced_Charge(true);
        TheMap().Add(RTTI_SPECIAL, static_cast<int>(SPC_AIR_STRIKE));
        base::At(TheMap().Column, 1).Flag_To_Redraw();
      }
      break;

    case ACTION_NONE:
      break;

    case ACTION_DZ:
      base::NewPooled<AnimClass>(ANIM_LZ_SMOKE,
                                 base::At(TheWorld().waypoint(), 25).Center());
      break;

    case ACTION_WIN:
      ThePlayer()->Flag_To_Win();
      break;

    case ACTION_LOSE:
      ThePlayer()->Flag_To_Lose();
      break;

    case ACTION_BEGIN_PRODUCTION:
      HouseClass::As_Pointer(House)->Begin_Production();
      break;

    case ACTION_CREATE_TEAM:
      if (Team) {
        TheWorld().scenario_init()++;
        Team->CreateTeam();
        TheWorld().scenario_init()--;
      }
      break;

    case ACTION_DESTROY_TEAM:
      if (Team) {
        Team->DestroyTeams();
      }
      break;

    case ACTION_REINFORCEMENTS:
      if (Team) {
        success = DeliverReinforcements(Team);
      }
      break;

    case ACTION_ALL_HUNT:
      Do_All_To_Hunt();
      break;

    case ActionType::ACTION_WINLOSE:
    case ActionType::ACTION_COUNT:
    default:
      break;
  }

  if (!success && Event == EVENT_TIME) {
    Data = 1;
  }

  /*
  **	Remove trigger from the game.
  */
  if (success && IsPersistant == VOLATILE) {
    Remove();
  }

  return true;
}

/***********************************************************************************************
 * TriggerClass::Remove -- removes this trigger from the game *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * 1 = trigger was removed, 0 = it wasn't *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 12/06/1994 BR : Created. *
 *=============================================================================================*/
bool TriggerClass::Remove() {
  Validate();

  /*
  **	Loop through all cells; remove any reference to this trigger
  */
  for (int cell_index = 0; cell_index < MAP_CELL_TOTAL; ++cell_index) {
    const Cell cell = Cell::FromIndex(cell_index);
    if (TheMap().at(cell).IsTrigger &&
        (base::At(TheWorld().cell_triggers(), cell_index) == this)) {
      TheMap().at(cell).IsTrigger = false;
      base::At(TheWorld().cell_triggers(), cell_index) = nullptr;
    }
  }

  /*
  **	Loop through all objects, removing any reference to this trigger
  */
  for (int index = 0; index < TheObjectHeaps().infantry().Count(); index++) {
    if (TheObjectHeaps().infantry().ActiveAt(index)->Trigger == this) {
      TheObjectHeaps().infantry().ActiveAt(index)->Trigger = nullptr;
    }
  }
  for (int index = 0; index < TheObjectHeaps().building().Count(); index++) {
    if (TheObjectHeaps().building().ActiveAt(index)->Trigger == this) {
      TheObjectHeaps().building().ActiveAt(index)->Trigger = nullptr;
    }
  }
  for (int index = 0; index < TheObjectHeaps().unit().Count(); index++) {
    if (TheObjectHeaps().unit().ActiveAt(index)->Trigger == this) {
      TheObjectHeaps().unit().ActiveAt(index)->Trigger = nullptr;
    }
  }
  for (int index = 0; index < TheObjectHeaps().terrain().Count(); index++) {
    if (TheObjectHeaps().terrain().ActiveAt(index)->Trigger == this) {
      TheObjectHeaps().terrain().ActiveAt(index)->Trigger = nullptr;
    }
  }

  /*
  **	Remove this trigger from any house list it's in. Invoking '-=' with a
  **	pointer not in the list has no effect; loop through all houses just to
  **	be on the safe side.
  */
  for (HousesType h = HOUSE_FIRST; h < HOUSE_COUNT; h++) {
    base::EraseFirst(TheWorld().house_triggers().at(h), this);
  }

  delete this;

  return true;
}

/***********************************************************************************************
 * TriggerClass::Read_INI -- reads triggers from the INI file *
 *                                                                                             *
 *    INI entry format: * Triggername = Eventname, Actionname, Data, Housename,
 *TeamName, IsPersistant           *
 *                                                                                             *
 * This routine reads in the triggers & creates them. Then, other classes can *
 * get pointers to the triggers they're linked to. *
 *                                                                                             *
 * The routine relies on the team types already being loaded so it can
 *resolve            * references to teams in this function. *
 *                                                                                             *
 * Cell Trigger pointers & IsTrigger flags are set in DisplayClass::Read_INI(),
 ** and cleared in the Map::Init() routine (which clears all cell objects to
 *0's).              *
 *                                                                                             *
 * Object's pointers are set in: * InfantryClass::Read_INI() *
 *      BuildingClass::Read_INI() * UnitClass::Read_INI() *
 *      TerrainClass::Read_INI() * The object trigger pointers are cleared in
 *the ObjectClass constructor.                     *
 *                                                                                             *
 * The House's EMSListOf triggers is set in this routine, and cleared in the *
 * HouseClass::Init() routine. *
 *                                                                                             *
 * INPUT: * ini         parsed scenario INI file *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * This function must be called before any other class's Read_INI. *
 *                                                                                             *
 * HISTORY: * 11/28/1994 BR : Created. *
 *=============================================================================================*/
void TriggerClass::Read_INI(const engine::file::IniFile& ini) {
  /*
  **	Loop for all trigger entries.
  */
  for (const auto& [name, entry] : ini.Entries(INI_Name())) {
    /*
    **	Create a new trigger.
    */
    auto* trigger =
        base::NewPooled<TriggerClass>();  // Working trigger pointer.

    /*
    **	Set its name.
    */
    trigger->Set_Name(name);

    /*
    **	Fill in the trigger.
    */
    trigger->Fill_In(name, entry);

    /*
    **	Add 'trigger' to the House's list.
    */
    //		if (trigger->House != HOUSE_NONE && trigger->Event !=
    // EVENT_PLAYER_ENTERED) { 		if (Event_Need_House(trigger->Event) &&
    //! Event_Need_Object(trigger->Event)) {
    if (trigger->House != HOUSE_NONE) {
      if (trigger->Action == ACTION_ALLOWWIN) {
        HouseClass::As_Pointer(trigger->House)->Blockage++;
      }
      TheWorld().house_triggers().at(trigger->House).push_back(trigger);
      trigger->AttachCount++;
    }
  }
}

/***********************************************************************************************
 * TriggerClass::Fill_In -- fills in trigger from the given INI entry *
 *                                                                                             *
 * This routine fills in the given trigger with the given name, and values from
 ** the given INI entry. *
 *                                                                                             *
 * (This routine is used by the scenario editor, to import teams from the
 *MASTER.INI file.)    *
 *                                                                                             *
 *    INI entry format: * Triggername = Eventname, Actionname, Data, Housename,
 *TeamName, IsPersistant           *
 *                                                                                             *
 * INPUT: * name      mnemonic for the desired trigger * entry      INI entry to
 *parse                                                          *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 11/28/1994 BR : Created. *
 *=============================================================================================*/
void TriggerClass::Fill_In(std::string_view name, std::string_view entry) {
  Validate();

  /*
  **	Set its name.
  */
  Set_Name(name);

  base::Tokenizer tokens(entry);

  /*
  **	1st token: Event.
  */
  Event = Event_From_Name(tokens.Next());

  /*
  **	2nd token: Action.
  */
  Action = Action_From_Name(tokens.Next());

  /*
  **	3rd token: Data.
  */
  DataCopy = Data = base::ParseIntegerOr<int64_t>(tokens.Next(), 0);

  /*
  **	4th token: House.
  */
  House = HouseTypeClass::From_Name(tokens.Next());
  if (House == HOUSE_NONE && Event == EVENT_PLAYER_ENTERED) {
    House = ThePlayer()->Class->House;
  }

  /*
  **	5th token: Team.
  */
  Team = TeamType::FromName(tokens.Next());

  /*
  ** 6th token: IsPersistant.  This token was added later, so we must check
  ** for its existence.
  */
  const std::string_view p = tokens.Next();
  if (!p.empty()) {
    IsPersistant = static_cast<PersistantType>(base::ParseIntegerOr<int>(p, 0));
  } else {
    IsPersistant = VOLATILE;
  }
}

/***********************************************************************************************
 * TriggerClass::Write_INI -- writes triggers to the INI file *
 *                                                                                             *
 *    INI entry format: * Triggername = Eventname, Actionname, Data, Housename,
 *TeamName, IsPersistant           *
 *                                                                                             *
 * INPUT: * ini      The INI database to write into. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 11/28/1994 BR : Created. *
 *=============================================================================================*/
void TriggerClass::Write_INI(engine::file::IniFile& ini, bool refresh) {
  char buf[128];
  const char* hname = nullptr;
  const char* tname = nullptr;

  /*
  **	First, clear out all existing trigger data from the INI file.
  */
  if (refresh) {
    ini.Clear(INI_Name());
  }

  /*
  **	Now write all the trigger data out
  */
  for (int index = 0; index < TheObjectHeaps().trigger().Count(); index++) {
    /*
    **	Get ptr to next active trigger.
    */
    TriggerClass* trigger = TheObjectHeaps().trigger().ActiveAt(index);

    /*
    **	Generate INI entry.
    */
    if (trigger->House == HOUSE_NONE) {
      hname = "None";
    } else {
      hname = HouseClass::As_Pointer(trigger->House)->Class->IniName;
    }

    if (trigger->Team == nullptr) {
      tname = "None";
    } else {
      tname = trigger->Team->IniName;
    }

    absl::SNPrintF(buf, sizeof(buf), "%s,%s,%" PRId64 ",%s,%s,%d",
                   Name_From_Event(trigger->Event),
                   Name_From_Action(trigger->Action), trigger->Data, hname,
                   tname, trigger->IsPersistant);
    ini.PutString(INI_Name(), trigger->Get_Name(), buf);
  }
}

/***********************************************************************************************
 * TriggerClass::As_Pointer -- returns pointer for the given trigger name *
 *                                                                                             *
 * This function is designed for use at initialization time, ie when the *
 * trigger mnemonics are read from the INI and the Read_INI functions need * to
 *get a pointer to that trigger. *
 *                                                                                             *
 * INPUT: * name      mnemonic for the desired trigger *
 *                                                                                             *
 * OUTPUT: * near pointer to that trigger, NULL if not found *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 11/28/1994 BR : Created. *
 *=============================================================================================*/
TriggerClass* TriggerClass::As_Pointer(std::string_view name) {
  if (name.empty()) {
    return nullptr;
  }

  for (int i = 0; i < TheObjectHeaps().trigger().Count(); i++) {
    TriggerClass* trigger = TheObjectHeaps().trigger().ActiveAt(i);

    if (absl::EqualsIgnoreCase(name, trigger->Name)) {
      return trigger;
    }
  }

  return nullptr;
}

/***********************************************************************************************
 * TriggerClass::operator new -- 'new' operator *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * pointer to new trigger *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 11/28/1994 BR : Created. *
 *=============================================================================================*/
void* TriggerClass::operator new(std::size_t /*unused*/) noexcept {
  void* ptr = TheObjectHeaps().trigger().Allocate();
  if (ptr) {
    static_cast<TriggerClass*>(ptr)->IsActive = true;
  }
  return ptr;
}

/***********************************************************************************************
 * TriggerClass::operator delete -- 'delete' operator *
 *                                                                                             *
 * INPUT: * ptr      pointer to delete *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 11/28/1994 BR : Created. *
 *=============================================================================================*/
void TriggerClass::operator delete(void* ptr) {
  if (ptr) {
    static_cast<TriggerClass*>(ptr)->IsActive = false;
  }
  TheObjectHeaps().trigger().Free(static_cast<TriggerClass*>(ptr));
}

/***********************************************************************************************
 * TriggerClass::Event_From_Name -- retrieves EventType for given name *
 *                                                                                             *
 * INPUT: * name      name to get event for *
 *                                                                                             *
 * OUTPUT: * EventType for given name *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 11/29/1994 BR : Created. *
 *=============================================================================================*/
EventType TriggerClass::Event_From_Name(std::string_view name) {
  if (name.empty()) {
    return EVENT_NONE;
  }

  for (int i = static_cast<int>(EVENT_NONE); i < static_cast<int>(EVENT_COUNT);
       i++) {
    if (absl::EqualsIgnoreCase(name, base::At(EventText, i + 1))) {
      return static_cast<EventType>(i);
    }
  }

  return EVENT_NONE;
}

/***********************************************************************************************
 * TriggerClass::Name_From_Event -- retrieves name for EventType *
 *                                                                                             *
 * INPUT: * event      EventType to get name for *
 *                                                                                             *
 * OUTPUT: * name for EventType *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 11/29/1994 BR : Created. *
 *=============================================================================================*/
const char* TriggerClass::Name_From_Event(EventType event) {
  return base::At(EventText, static_cast<int>(event) + 1);
}

/***********************************************************************************************
 * TriggerClass::Action_From_Name -- retrieves ActionType for given name *
 *                                                                                             *
 * INPUT: * name         name to get ActionType for *
 *                                                                                             *
 * OUTPUT: * ActionType for given name *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 11/29/1994 BR : Created. *
 *=============================================================================================*/
TriggerClass::ActionType TriggerClass::Action_From_Name(std::string_view name) {
  if (name.empty()) {
    return ACTION_NONE;
  }

  for (int i = static_cast<int>(ACTION_NONE);
       i < static_cast<int>(ACTION_COUNT); i++) {
    if (absl::EqualsIgnoreCase(name, base::At(ActionText, i + 1))) {
      return static_cast<ActionType>(i);
    }
  }

  return ACTION_NONE;
}

/***********************************************************************************************
 * TriggerClass::Name_From_Action -- retrieves name for ActionType *
 *                                                                                             *
 * INPUT: * action      ActionType to get name for *
 *                                                                                             *
 * OUTPUT: * name of ActionType *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 11/29/1994 BR : Created. *
 *=============================================================================================*/
const char* TriggerClass::Name_From_Action(ActionType action) {
  return base::At(ActionText, static_cast<int>(action) + 1);
}

/***********************************************************************************************
 * TriggerClass::As_Target -- Converts trigger to a target value *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Target value *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/19/1994 JLB : Created. *
 *=============================================================================================*/
Target TriggerClass::As_Target() const {
  Validate();
  return Target::Make(KIND_TRIGGER,
                      TheObjectHeaps().trigger().SlotIndexOf(this));
}

/***********************************************************************************************
 * Do_All_To_Hunt -- Forces all computer controlled units into hunt mode. *
 *                                                                                             *
 *    This trigger action will cause the computer units and infantry to go into
 *hunt mode.     * Use it to bring a scenario to a sudden conclusion. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 04/20/1995 JLB : Created. * 08/14/1995 JLB : Removes the member
 *from a team if necessary.                             *
 *=============================================================================================*/
static void Do_All_To_Hunt() {
  for (int index = 0; index < TheObjectHeaps().unit().Count(); index++) {
    UnitClass* unit = TheObjectHeaps().unit().ActiveAt(index);

    if (!unit->House->IsHuman && unit->IsDown && !unit->IsInLimbo) {
      if (unit->Team) {
        unit->Team->Remove(unit);
      }
      unit->Assign_Mission(MISSION_HUNT);
    }
  }

  for (int index = 0; index < TheObjectHeaps().infantry().Count(); index++) {
    InfantryClass* infantry = TheObjectHeaps().infantry().ActiveAt(index);

    if (!infantry->House->IsHuman && infantry->IsDown && !infantry->IsInLimbo) {
      if (infantry->Team) {
        infantry->Team->Remove(infantry);
      }
      infantry->Assign_Mission(MISSION_HUNT);
    }
  }
}

}  // namespace td
