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

/* $Header: /CounterStrike/LOGIC.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : LOGIC.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : September 27, 1993 *
 *                                                                                             *
 *                  Last Update : July 30, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * LogicClass::AI -- Handles AI logic processing for game objects.
 *   * LogicClass::Detach -- Detatch the specified target from the logic system.
 *   *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/logic.h"

#include <cstdint>

#include "base/fixed.h"
#include "engine/platform/ftimer.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/anim.h"
#include "ra/audio.h"
#include "ra/coord.h"
#include "ra/defines.h"
#include "ra/factory.h"
#include "ra/gscreen.h"
#include "ra/heap.h"
#include "ra/house.h"
#include "ra/mapedit.h"
#include "ra/object.h"
#include "ra/object_heaps.h"
#include "ra/rules.h"
#include "ra/scenario.h"
#include "ra/session.h"
#include "ra/special.h"
#include "ra/target.h"
#include "ra/team.h"
#include "ra/tevent.h"
#include "ra/trigger.h"
#include "ra/type.h"
#include "ra/vector_dynamic.h"
#include "ra/vortex.h"
#include "ra/world.h"

/***********************************************************************************************
 * LogicClass::AI -- Handles AI logic processing for game objects. *
 *                                                                                             *
 *    This routine is used to perform the AI processing for all game objects.
 *This includes    * all houses, factories, objects, and teams. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/29/1994 JLB : Created. * 12/17/1994 JLB : Must perform one
 *complete pass rather than bailing early.                * 12/23/1994 JLB :
 *Ensures that no object gets skipped if it was deleted.                   *
 *=============================================================================================*/
void LogicClass::AI() {
  /*
  ** Fading to B&W or color due to the chronosphere is handled here.
  */
  TheScenario().Do_Fade_AI();

  /*
  **	Handle any general timer trigger events.
  */
  for (TheWorld().logic_trigger_id() = 0;
       TheWorld().logic_trigger_id() < TheWorld().logic_triggers().Count();
       TheWorld().logic_trigger_id()++) {
    TriggerClass* trig =
        TheWorld().logic_triggers().at(TheWorld().logic_trigger_id());

    /*
    **	Global changed trigger event might be triggered.
    */
    if (TheScenario().IsGlobalChanged) {
      if (trig->Spring(TEVENT_GLOBAL_SET)) {
        continue;
      }
      if (trig->Spring(TEVENT_GLOBAL_CLEAR)) {
        continue;
      }
    }

    /*
    **	Bridge change event.
    */
    if (TheScenario().IsBridgeChanged &&
        trig->Spring(TEVENT_ALL_BRIDGES_DESTROYED)) {
      continue;
    }

    /*
    **	General time expire trigger events can be sprung without warning.
    */
    if (trig->Spring(TEVENT_TIME)) {
      continue;
    }

    /*
    **	The mission timer expiration trigger event might spring if the timer is
    *active *	but at a value of zero.
    */
    if ((TheScenario().MissionTimer.IsRunning() &&
         TheScenario().MissionTimer.IsFinished()) &&
        trig->Spring(TEVENT_MISSION_TIMER_EXPIRED)) {
      continue;
    }
  }

  /*
  **	Clean up any status values that were maintained only for logic trigger
  **	purposes.
  */
  if (TheScenario().MissionTimer.IsRunning() &&
      TheScenario().MissionTimer.IsFinished()) {
    TheScenario().MissionTimer.Stop();
    TheMap().Flag_To_Redraw(
        true);  // Used only to cause tabs to redraw in new state.
  }
  TheScenario().IsGlobalChanged = false;
  TheScenario().IsBridgeChanged = false;
  /*
  **	Shadow creeping back over time is handled here.
  */
  if (TheSpecial().IsShadowGrow && TheRules().ShroudRate != 0 &&
      TheScenario().ShroudTimer.IsFinished()) {
    TheScenario().ShroudTimer.Set(kTicksPerMinute * TheRules().ShroudRate);
    TheMap().Encroach_Shadow();
  }

  /*
  **	Team AI is processed.
  */
  for (int index = 0; index < TheObjectHeaps().team().Count(); ++index) {
    TheObjectHeaps().team().Ptr(index)->AI();
  }

  /*
  ** If there's a time quake, handle it here.
  */
  if (TheWorld().time_quake()) {
    PlaySoundEffect(VOC_KABOOM15);
    Shake_The_Screen(8);
  }

  TheWorld().chronal_vortex().AI();
  /*
  **	AI for all sentient objects is processed.
  */
  for (int index = 0; index < Count(); index++) {
    ObjectClass* obj = (*this).at(index);

    obj->AI();

    if (TheWorld().time_quake() && obj->IsActive && !obj->IsInLimbo &&
        obj->Strength) {
      int damage = obj->Class_Of().MaxStrength * TheRules().QuakeDamagePercent;
      if (TheWorld().time_quake_center()) {
        if (Distance(obj->As_Target(), TheWorld().time_quake_center()) / 256 <
            TheRules().MTankDistance) {
          switch (obj->What_Am_I()) {
            case RTTI_INFANTRY:
              damage = TheRules().QuakeInfantryDamage;
              break;
            case RTTI_BUILDING:
              damage =
                  TheRules().QuakeBuildingDamage * obj->Class_Of().MaxStrength;
              break;
            case RTTIType::RTTI_NONE:
            case RTTIType::RTTI_AIRCRAFT:
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
            case RTTIType::RTTI_UNIT:
            case RTTIType::RTTI_UNITTYPE:
            case RTTIType::RTTI_VESSEL:
            case RTTIType::RTTI_VESSELTYPE:
            default:
              damage = TheRules().QuakeUnitDamage * obj->Class_Of().MaxStrength;
              break;
          }
          if (damage) {
            obj->Clicked_As_Target();
            new AnimClass(ANIM_MINE_EXP1, obj->Center_Coord());
          }
          obj->Take_Damage(damage, 0, WARHEAD_AP, nullptr, true);
        }
      } else {
        obj->Take_Damage(damage, 0, WARHEAD_AP, nullptr, true);
      }
    }
    /*
    **	If the object was destroyed in the process of performing its AI, then
    **	adjust the index so that no object gets skipped.
    */
    if (obj != (*this).at(index)) {
      index--;
    }
  }
  HouseClass::Recalc_Attributes();

  /*
  **	Map related logic is performed.
  */
  TheMap().Logic();

  /*
  **	Factory processing is performed.
  */
  for (int index = 0; index < TheObjectHeaps().factory().Count(); index++) {
    TheObjectHeaps().factory().Ptr(index)->AI();
  }

  /*
  **	House processing is performed.
  */
  if (TheSession().Type == GAME_NORMAL) {
    for (const HousesType house : magic_enum::enum_values<HousesType>()) {
      HouseClass* hptr = HouseClass::As_Pointer(house);
      if (hptr != nullptr && hptr->IsActive) {
        hptr->AI();
      }
    }
  } else {
    for (const HousesType house : magic_enum::enum_values<HousesType>()) {
      if (house < HOUSE_MULTI1) {
        continue;
      }
      HouseClass* hptr = HouseClass::As_Pointer(house);
      if (hptr != nullptr && hptr->IsActive) {
        hptr->AI();
      }
    }
  }

  if (TheSession().Type != GAME_NORMAL &&
      TheScenario().AutoSonarTimer.IsFinished()) {
    if (TheWorld().auto_sonar_pulse()) {
      TheMap().Activate_Pulse();
      PlaySoundEffect(VOC_SONAR);
      TheWorld().auto_sonar_pulse() = false;
    }
    TheScenario().AutoSonarTimer.Set(int64_t{kTicksPerSecond} * 40);
  }
}

/***********************************************************************************************
 * LogicClass::Detach -- Detach the specified target from the logic system. *
 *                                                                                             *
 *    This routine is called when the specified target object is about to be
 *removed from the  * game system and all references to it must be severed. The
 *only thing that the logic      * system looks for in this case is to see if
 *the target refers to a trigger and if so,     * it scans through the trigger
 *list and removes all references to it.                      *
 *                                                                                             *
 * INPUT:   target   -- The target to remove from the sytem. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/30/1996 JLB : Created. *
 *=============================================================================================*/
void LogicClass::Detach(TARGET target, bool /*unused*/) {
  /*
  **	Remove any triggers from the logic trigger list.
  */
  if (Is_Target_Trigger(target)) {
    for (int index = 0; index < TheWorld().logic_triggers().Count(); index++) {
      if (As_Trigger(target) == TheWorld().logic_triggers().at(index)) {
        TheWorld().logic_triggers().Delete(index);
        index--;
      }
    }
  }
}
