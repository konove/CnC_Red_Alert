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

/* $Header: /CounterStrike/TRACKER.CPP 1     3/03/97 10:26a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : TRACKER.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 06/14/96 *
 *                                                                                             *
 *                  Last Update : June 14, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Detach_This_From_All -- Detaches this object from all others. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/tracker.h"

#include "ra/aircraft.h"
#include "ra/anim.h"
#include "ra/building.h"
#include "ra/bullet.h"
#include "ra/ccptr.h"
#include "ra/defines.h"
#include "ra/heap.h"
#include "ra/house.h"
#include "ra/infantry.h"
#include "ra/logic.h"
#include "ra/mapedit.h"
#include "ra/object_heaps.h"
#include "ra/target.h"
#include "ra/team.h"
#include "ra/teamtype.h"
#include "ra/trigger.h"
#include "ra/trigtype.h"
#include "ra/unit.h"
#include "ra/vessel.h"
#include "ra/vortex.h"
#include "ra/world.h"

/***********************************************************************************************
 * Detach_This_From_All -- Detaches this object from all others. *
 *                                                                                             *
 *    This routine sweeps through all game objects and makes sure that it is no
 *longer         * referenced by them. Typically, this is called in preparation
 *for the object's death      * or limbo state. *
 *                                                                                             *
 * INPUT:   target   -- This object expressed as a target number. *
 *                                                                                             *
 *          all      -- Is this object really in truly being removed from the
 *game? The        * answer would be false if the target was actually a stealth
 ** tank that is cloaking. In such a case, the object should be removed    *
 *                      from all non-friendly tracking systems, but otherwise
 *left alone.      *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/08/1995 JLB : Created. *
 *=============================================================================================*/
void Detach_This_From_All(TARGET target, bool all) {
  if (Target_Legal(target)) {
    for (int index = 0; index < TheObjectHeaps().house().Count(); index++) {
      TheObjectHeaps().house().Ptr(index)->Detach(target, all);
    }
    for (int index = 0; index < TheObjectHeaps().team().Count(); index++) {
      TheObjectHeaps().team().Ptr(index)->Detach(target, all);
    }
    for (int index = 0; index < TheObjectHeaps().team_type().Count(); index++) {
      TheObjectHeaps().team_type().Ptr(index)->Detach(target, all);
    }
    for (int index = 0; index < TheObjectHeaps().unit().Count(); index++) {
      TheObjectHeaps().unit().Ptr(index)->Detach(target, all);
    }
    for (int index = 0; index < TheObjectHeaps().vessel().Count(); index++) {
      TheObjectHeaps().vessel().Ptr(index)->Detach(target, all);
    }
    for (int index = 0; index < TheObjectHeaps().aircraft().Count(); index++) {
      TheObjectHeaps().aircraft().Ptr(index)->Detach(target, all);
    }
    for (int index = 0; index < TheObjectHeaps().building().Count(); index++) {
      TheObjectHeaps().building().Ptr(index)->Detach(target, all);
    }
    for (int index = 0; index < TheObjectHeaps().bullet().Count(); index++) {
      TheObjectHeaps().bullet().Ptr(index)->Detach(target, all);
    }
    for (int index = 0; index < TheObjectHeaps().infantry().Count(); index++) {
      TheObjectHeaps().infantry().Ptr(index)->Detach(target, all);
    }
    for (int index = 0; index < TheObjectHeaps().anim().Count(); index++) {
      TheObjectHeaps().anim().Ptr(index)->Detach(target, all);
    }

    TheMap().Detach(target, all);

    LogicClass::Detach(target, all);

    TheWorld().chronal_vortex().Detach(target);

    /*
    **	Removing a trigger type must also remove all triggers that are dependant
    **	upon that type.
    */
    if (As_TriggerType(target) != nullptr) {
      for (int j = 0; j < TheObjectHeaps().trigger().Count(); j++) {
        const TriggerClass* tp = TheObjectHeaps().trigger().Ptr(j);

        if (tp->Class->As_Target() == target) {
          Detach_This_From_All(tp->As_Target());
          delete tp;
          j--;
        }
      }
    }

    for (int index = 0; index < TheObjectHeaps().trigger().Count(); index++) {
      TheObjectHeaps().trigger().Ptr(index)->Detach(target, all);
    }
    for (int index = 0; index < TheObjectHeaps().trigger_type().Count();
         index++) {
      TheObjectHeaps().trigger_type().Ptr(index)->Detach(target, all);
    }
  }
}
