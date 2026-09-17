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

/* $Header:   F:\projects\c&c\vcs\code\logic.cpv   2.17   16 Oct 1995 16:50:52
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
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
 *                  Last Update : December 23, 1994 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:
 *   LogicClass::AI -- Handles AI logic processing for game objects.
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/logic.h"

#include "rand.h"
#include "td/aircraft.h"
#include "td/building.h"
#include "td/defines.h"
#include "td/externs.h"
#include "td/factory.h"
#include "td/ftimer.h"
#include "td/heap.h"
#include "td/house.h"
#include "td/infantry.h"
#include "td/jshell.h"
#include "td/mapedit.h"
#include "td/object.h"
#include "td/team.h"
#include "td/type.h"
#include "td/unit.h"
#include "td/vector.h"

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
 *Esures that no object gets skipped if it was deleted.                    *
 *=============================================================================================*/
void LogicClass::AI() {
  /*
  **	Crate regeneration is handled here.
  */
  if (GameToPlay != GAME_NORMAL && CrateMaker && CrateTimer.Expired()) {
    Map.Place_Random_Crate();
    CrateTimer = kTicksPerMinute * Random_Pick(7, 15);
  }

  /*
  **	Team AI is processed.
  */
  for (int index = 0; index < Teams.Count(); index++) {
    Teams.Ptr(index)->AI();
  }

  //	Heap_Dump_Check( "After Team AI" );

  /*
  **	AI for all sentient objects is processed.
  */
  for (int index = 0; index < Count(); index++) {
    ObjectClass* obj = (*this).at(index);

    obj->AI();

    /*
    **	If the object was destroyed in the process of performing its AI, then
    **	adjust the index so that no object gets skipped.
    */
    if (obj != (*this).at(index)) {
      //		if (!obj->IsActive) {
      index--;
    }
  }

  //	Heap_Dump_Check( "After Object AI" );

  /*
  **	A second pass through the sentient objects is required so that the
  *appropriate scan *	bits will be set for the owner house.
  */
  for (int index = 0; index < Units.Count(); index++) {
    const UnitClass* unit = Units.Ptr(index);
    if (unit->IsLocked && (GameToPlay != GAME_NORMAL || !unit->House->IsHuman ||
                           unit->IsDiscoveredByPlayer)) {
      unit->House->NewUScan |= ScanBit(static_cast<int>(unit->Class->Type));
      if (!unit->IsInLimbo) {
        unit->House->NewActiveUScan |=
            ScanBit(static_cast<int>(unit->Class->Type));
      }
    }
  }
  for (int index = 0; index < Infantry.Count(); index++) {
    const InfantryClass* infantry = Infantry.Ptr(index);
    if (infantry->IsLocked &&
        (GameToPlay != GAME_NORMAL || !infantry->House->IsHuman ||
         infantry->IsDiscoveredByPlayer)) {
      infantry->House->NewIScan |=
          ScanBit(static_cast<int>(infantry->Class->Type));
      if (!infantry->IsInLimbo) {
        infantry->House->NewActiveIScan |=
            ScanBit(static_cast<int>(infantry->Class->Type));
      }
    }
  }
  for (int index = 0; index < Aircraft.Count(); index++) {
    const AircraftClass* aircraft = Aircraft.Ptr(index);
    if (aircraft->IsLocked &&
        (GameToPlay != GAME_NORMAL || !aircraft->House->IsHuman ||
         aircraft->IsDiscoveredByPlayer)) {
      aircraft->House->NewAScan |=
          ScanBit(static_cast<int>(aircraft->Class->Type));
      if (!aircraft->IsInLimbo) {
        aircraft->House->NewActiveAScan |=
            ScanBit(static_cast<int>(aircraft->Class->Type));
      }
    }
  }
  for (int index = 0; index < Buildings.Count(); index++) {
    const BuildingClass* building = Buildings.Ptr(index);
    if (building->IsLocked &&
        (GameToPlay != GAME_NORMAL || !building->House->IsHuman ||
         building->IsDiscoveredByPlayer)) {
      building->House->NewBScan |=
          ScanBit(static_cast<int>(building->Class->Type));
      if (!building->IsInLimbo) {
        building->House->NewActiveBScan |=
            ScanBit(static_cast<int>(building->Class->Type));
      }
    }
  }

  //	Heap_Dump_Check( "After Object AI 2" );

  /*
  **	Map related logic is performed.
  */
  Map.Logic();

  //	Heap_Dump_Check( "After Map.Logic" );

  /*
  **	Factory processing is performed.
  */
  for (int index = 0; index < Factories.Count(); index++) {
    Factories.Ptr(index)->AI();
  }

  //	Heap_Dump_Check( "After Factory AI" );

  /*
  **	House processing is performed.
  */
  for (HousesType house = HOUSE_FIRST; house < HOUSE_COUNT; house++) {
    HouseClass* hptr = HouseClass::As_Pointer(house);
    if (hptr && hptr->IsActive) {
      hptr->AI();
    }
  }

  //	Heap_Dump_Check( "After House AI" );
}
