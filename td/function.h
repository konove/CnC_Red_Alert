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

/* $Header:   F:\projects\c&c\vcs\code\function.h_v   2.21   16 Oct 1995
 * 16:46:44   JOE_BOSTIC  $*/
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

#ifndef FUNCTION_H
#define FUNCTION_H
#include "sdllib/include/dipthong.h"
#include "sdllib/include/iff.h"
#include "sdllib/include/shape.h"
#include "sdllib/include/misc.h"

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
         ┌────────────────────────┴────────────────────────────┐
     FootClass                                         BuildingClass
         │
         ├──────────────┬─────────────┐
    DriveClass  InfantryClass         ├─ FlyClass
         │                      AircraftClass
   TurretClass
         │
   TarComClass
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
    ┌────────┴─────┬───────────┬──────────────┐             TerrainTypeClass
    │              │           │              │
UnitTypeClass      │   BuildingTypeClass      │
                   │                  InfantryTypeClass
           AircraftTypeClass
*/

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef PORTABLE
#include "sdllib/include/keyboard.h"
#endif

#define WWMEM_H
#include "td/compat.h"
#include "sdllib/include/font.h"
#include "sdllib/include/wsa.h"
#include "sdllib/include/ww_audio.h"
#include "sdllib/include/memflag.h"
#include "sdllib/include/file.h"
#include "td/jshell.h"

// Don't complain if these headers aren't referenced.
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstddef>
#include "port/ex_string.h"
#include <cstdarg>
#include <cctype>
#include <cassert>
#include <new>

#include "winvq/vqa32/vqaplay.h"
#include "winvq/vqa32/vqafile.h"

#include "td/vector.h"
#include "td/heap.h"
#include "td/ccfile.h"
#include "td/monoc.h"
#include "td/conquer.h"
#include "td/special.h"
#include "td/defines.h"
#include "td/inline.h"

#include "td/utracker.h"
#include "td/palette.h"
#include "td/facing.h"
#include "td/ftimer.h"
#include "td/theme.h"
#include "td/link.h"
#include "td/gadget.h"
#include "td/control.h"
#include "td/toggle.h"
#include "td/checkbox.h"
#include "td/shapebtn.h"
#include "td/textbtn.h"
#include "td/slider.h"
#include "td/list.h"
#include "td/cheklist.h"
#include "td/colrlist.h"
#include "td/edit.h"
#include "td/gauge.h"
#include "td/msgbox.h"
#include "td/dial8.h"
#include "td/txtlabel.h"
#include "td/super.h"
#include "td/house.h"
#include "td/gscreen.h"
#include "td/map.h"
#include "td/display.h"
#include "td/radar.h"
#include "td/power.h"
#include "td/sidebar.h"
#include "td/tab.h"
#include "td/help.h"
#include "td/mouse.h"
// #include	"mapedit.h"
#include "td/help.h"
#include "td/target.h"
#include "td/theme.h"
#include "td/team.h"      // Team objects.
#include "td/teamtype.h"  // Team type objects.
#include "td/trigger.h"   // Trigger event objects.
#include "td/mapedit.h"   // ???
#include "td/abstract.h"
#include "td/object.h"
#include "td/mission.h"
#include "td/door.h"
#include "td/bullet.h"    // Bullet objects.
#include "td/terrain.h"   // Terrain objects.
#include "td/anim.h"      // Animation objects.
#include "td/template.h"  // Icon template objects.
#include "td/overlay.h"   // Overlay objects.
#include "td/smudge.h"    // Stains on the terrain objects.
#include "td/aircraft.h"  // Aircraft objects.
#include "td/unit.h"      // Ground unit objects.
#include "td/infantry.h"  // Infantry objects.
#include "td/credits.h"   // Credit counter class.
#include "td/score.h"     // Scoring system class.
#include "td/factory.h"   // Production manager class.
#include "td/intro.h"
#include "td/ending.h"
#include "td/logic.h"
#include "td/queue.h"
#include "td/event.h"
#include "td/base.h"  // defines the AI's pre-built base
#include "td/ipxmgr.h"
#include "td/combuf.h"
#include "td/connect.h"
#include "td/connmgr.h"
#include "td/noseqcon.h"
#include "td/msglist.h"
#include "td/nullconn.h"
#include "td/nullmgr.h"
#include "td/phone.h"
#include "td/loaddlg.h"
#include "td/ipxaddr.h"
#include "td/profile.h"
#include "td/nulldlg.h"
#include "td/netdlg.h"
#include "td/audio.h"
#include "td/combat.h"
#include "td/coord.h"
#include "td/dialog.h"
#include "td/ending.h"
#include "td/nodename.h"
#include "td/externs.h"

#include "td/expand.h"
#include "td/findpath.h"
#include "td/ini.h"
#include "td/init.h"
#include "td/keyframe.h"
#include "td/menus.h"
#include "td/mplayer.h"
#include "td/support.h"
#include "td/text.h"

#endif
