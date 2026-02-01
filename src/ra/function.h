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

/* $Header: /CounterStrike/FUNCTION.H 2     3/13/97 2:05p Steve_tall $*/
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : FUNCTION.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : May 27, 1994 *
 *                                                                                             *
 *                  Last Update : May 27, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_FUNCTION_H_
#define CNC_RED_ALERT_RA_FUNCTION_H_

/*
Map (screen) class heirarchy.

 MapeditClass (most derived class) -- scenario editor
        │
   MouseClass -- handles mouse animation and display control
        │
  ScrollClass -- map scroll handler
        │
    HelpClass -- pop-up help text handler
        │
     TabClass -- file folder tab screen mode control dispatcher
        │
 SidebarClass -- displays and controls construction list sidebar
        │
   PowerClass -- display power production/consumption bargraph
        │
   RadarClass -- displays and controls radar map
        │
 DisplayClass -- general tactical map display handler
        │
     MapClass -- general tactical map data handler
        │
 GScreenClass (pure virtual base class) -- generic screen control

                          AbstractClass
                                  │
                                  │
                                  │
                                  │
                            ObjectClass
                                  │
       ┌──────┬──────────┬────────┼────────┬────────────────┬───────────┐
   AnimClass  │  TemplateClass    │        ├─ FuseClass     │    TerrainClass
              │                   │        ├─ FlyClass      │
              │                   │  BulletClass            │
       OverlayClass        MissionClass               SmudgeClass
                                  │
                             RadioClass
                                  │
                                  ├─ CrewClass
                                  ├─ FlasherClass
                                  ├─ StageClass
                                  ├─ CargoClass
                            TechnoClass
                                  │
                       ┌──────────┴────────────────────────────┐
                   FootClass                           BuildingClass
                       │
         ┌─────────────┴┬─────────────┐
    DriveClass  InfantryClass         ├─ FlyClass
         │                      AircraftClass
       ┌─┴─────────┐
       │           │
       │     VesselClass
       │
    UnitClass


                            AbstractTypeClass
                                    │
                              ObjectTypeClass
                                    │
             ┌──────────────────────┼────────────┬─────────────────┐
             │                      │            │                 │
       TechnoTypeClass              │            │                 │
             │                BulletTypeClass    │                 │
             │                           TemplateTypeClass         │
    ┌────────┴─────┬───────────┬──────────────┬────────┐    TerrainTypeClass
    │              │           │              │        │
UnitTypeClass      │   BuildingTypeClass      │  VesselTypeClass
                   │                          │
           AircraftTypeClass          InfantryTypeClass
*/

// #define int386x(a, b, c, d) 0
// #define int386(a, b, c) 0

#if (TEN)
#include "ra/tenmgr.h"
#endif

#if (MPATH)
#include "ra/mpmgrw.h"
#endif

#endif  // CNC_RED_ALERT_RA_FUNCTION_H_
