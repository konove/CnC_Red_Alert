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

/* $Header: /CounterStrike/DEBUG.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : DEBUG.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : September 10, 1993 *
 *                                                                                             *
 *                  Last Update : July 18, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Debug_Key -- Debug mode keyboard processing. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/debug.h"

#include <cstdint>

#include "base/array.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/aircraft.h"
#include "ra/anim.h"
#include "ra/building.h"
#include "ra/ccptr.h"
#include "ra/cell.h"
#include "ra/combat.h"
#include "ra/coord.h"
#include "ra/debug_state.h"
#include "ra/defines.h"
#include "ra/display.h"
#include "ra/face.h"
#include "ra/heap.h"
#include "ra/house.h"
#include "ra/inline.h"
#include "ra/mapedit.h"
#include "ra/object.h"
#include "ra/object_heaps.h"
#include "ra/screen.h"
#include "ra/super.h"
#include "ra/type.h"
#include "ra/vector_dynamic.h"
#include "ra/vortex.h"
#include "ra/weapon.h"
#include "ra/world.h"
#include "sdllib/keyboard.h"
#include "sdllib/pixel_buffer.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"

/***********************************************************************************************
 * Debug_Key -- Debug mode keyboard processing. *
 *                                                                                             *
 *    If debugging is enabled, then this routine will be called for every
 *keystroke that the   * game doesn't recognize. These extra keys usually
 *perform some debugging function.        *
 *                                                                                             *
 * INPUT:   input -- The key code that was pressed. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/07/1992 JLB : Created. *
 *=============================================================================================*/
void Debug_Key(unsigned input) {
  static int map_x = -1;
  static int map_y = -1;
  static int map_width = -1;
  static int map_height = -1;

  if (!input || input & KN_BUTTON) {
    return;
  }

  /*
  **	Processing of normal keystrokes.
  */
  if (TheDebugState().developer_mode()) {
    switch (input) {
      case KN_BACKSPACE:

        if (TheWorld().chronal_vortex().Is_Active()) {
          TheWorld().chronal_vortex().Disappear();
        } else {
          const int xxxx = Get_Mouse_X() + TheMap().TacPixelX;
          const int yyyy = Get_Mouse_Y() + TheMap().TacPixelY;
          const CELL cell = TheMap().DisplayClass::Click_Cell_Calc(xxxx, yyyy);
          TheWorld().chronal_vortex().Appear(Cell_Coord(cell));
        }
        break;

      case KN_J:
        TheDebugState().set_motion_capture(true);
        break;

      case KN_P: {
        for (const SpecialWeaponType spc :
             magic_enum::enum_values<SpecialWeaponType>()) {
          ThePlayer()->SuperWeapon.at(spc).Enable(true, true);
          ThePlayer()->SuperWeapon.at(spc).Forced_Charge(true);
          TheMap().Add(RTTI_SPECIAL, static_cast<int>(spc));
          base::At(TheMap().Column, 1).Flag_To_Redraw();
        }
      } break;

      case KN_I: {
        TheMap().Flash_Power();
        TheMap().Flash_Money();
      } break;

      case KN_O: {
        auto* air = new AircraftClass(AIRCRAFT_HIND, ThePlayer()->Class->House);
        if (air) {
          air->Height = 0;
          air->Unlimbo(TheMap().Pixel_To_Coord(Get_Mouse_X(), Get_Mouse_Y()),
                       DIR_N);
        }
      } break;

      case KN_B: {
        auto* air =
            new AircraftClass(AIRCRAFT_LONGBOW, ThePlayer()->Class->House);
        if (air) {
          air->Height = 0;
          air->Unlimbo(TheMap().Pixel_To_Coord(Get_Mouse_X(), Get_Mouse_Y()),
                       DIR_N);
        }
      } break;

      case KN_GRAVE: {
        const WarheadType warhead = Random_Pick(WARHEAD_HE, WARHEAD_FIRE);
        const COORDINATE coord =
            TheMap().Pixel_To_Coord(Get_Mouse_X(), Get_Mouse_Y());
        const int damage = 1000;
        new AnimClass(
            Combat_Anim(damage, warhead, TheMap().at(coord).Land_Type()),
            coord);
        Explosion_Damage(coord, damage, nullptr, warhead);
      } break;

      case KN_C:
        TheDebugState().set_build_anything(!TheDebugState().build_anything());
        ThePlayer()->IsRecalcNeeded = true;

        /*
        **	This placement might affect any prerequisite requirements for
        *construction *	lists. Update the buildable options accordingly.
        */
        if (!TheWorld().scenario_init()) {
          TheMap().Recalc();
          for (int index = 0; index < TheObjectHeaps().building().Count();
               index++) {
            TheObjectHeaps().building().Ptr(index)->Update_Buildables();
          }
        }
        break;

      case KN_Z | KN_ALT_BIT:
        if (map_x == -1) {
          map_x = TheMap().MapCellX;
          map_y = TheMap().MapCellY;
          map_width = TheMap().MapCellWidth;
          map_height = TheMap().MapCellHeight;
          TheMap().MapCellX = 1;
          TheMap().MapCellY = 1;
          TheMap().MapCellWidth = MAP_CELL_W - 2;
          TheMap().MapCellHeight = MAP_CELL_H - 2;
        } else {
          TheMap().MapCellX = map_x;
          TheMap().MapCellY = map_y;
          TheMap().MapCellWidth = map_width;
          TheMap().MapCellHeight = map_height;
          map_x = -1;
          map_y = -1;
          map_width = -1;
          map_height = -1;
        }
        break;

      case KN_W | KN_ALT_BIT:
        ThePlayer()->Flag_To_Win();
        break;

      case KN_L | KN_ALT_BIT:
        ThePlayer()->Flag_To_Lose();
        break;

      case KN_DELETE:
        if (TheWorld().current_object().Count()) {
          TheMap().Recalc();
          // CurrentObject[0]->Detach_All();
          if (TheWorld().current_object().at(0)->What_Am_I() == RTTI_BUILDING) {
            dynamic_cast<BuildingClass*>(TheWorld().current_object().at(0))
                ->Sell_Back(1);
          } else {
            ObjectClass* object = TheWorld().current_object().at(0);
            object->Unselect();
            object->Limbo();
            delete object;
          }
        }
        break;

      case KN_DELETE | KN_SHIFT_BIT:
        if (TheWorld().current_object().Count()) {
          TheMap().Recalc();
          int damage = 50;
          TheWorld().current_object().at(0)->Take_Damage(damage, 0, WARHEAD_SA);
        }
        break;

      case KN_INSERT:
        if (TheWorld().current_object().Count()) {
          TheMap().PendingObject =
              &TheWorld().current_object().at(0)->Class_Of();
          if (TheMap().PendingObject) {
            TheMap().PendingHouse = TheWorld().current_object().at(0)->Owner();
            TheMap().PendingObjectPtr = TheMap().PendingObject->Create_One_Of(
                HouseClass::As_Pointer(TheMap().PendingHouse));
            if (TheMap().PendingObjectPtr) {
              TheMap().Set_Cursor_Pos();
              TheMap().Set_Cursor_Shape(TheMap().PendingObject->Occupy_List());
            }
          }
        }
        break;

      case KN_V:
      case KN_F3:
        TheDebugState().set_show_cell_info(!TheDebugState().show_cell_info());
        TheMap().Flag_To_Redraw(true);
        break;

      /*
      **	Shows sight and fire range in the form of circles emanating from
      *the currently *	selected unit. The white circle is for sight range, the
      *red circle is for *	fire range.
      */
      case KN_F7:
        if (TheWorld().current_object().Count() &&
            TheWorld().current_object().at(0)->Is_Techno()) {
          const auto& ttype = dynamic_cast<const TechnoTypeClass&>(
              TheWorld().current_object().at(0)->Class_Of());
          const int sight = ttype.SightRange * 256;
          int weapon = 0;
          if (ttype.PrimaryWeapon != nullptr) {
            weapon = ttype.PrimaryWeapon->Range;
          }
          PixelView& view = TheScreen().visible_view();
          const COORDINATE center =
              TheWorld().current_object().at(0)->Center_Coord();
          const COORDINATE center2 =
              TheWorld().current_object().at(0)->Fire_Coord(0);

          for (int r = 0; r < 255; r += 10) {
            int x = 0;
            int y = 0;
            int x1 = 0;
            int y1 = 0;
            const DirType r1 = AsDirection(r);
            const DirType r2 = AsDirection(r + 10);

            if (TheMap().Coord_To_Pixel(
                    Coord_Move(center, r1, static_cast<uint16_t>(sight)), x,
                    y)) {
              TheMap().Coord_To_Pixel(
                  Coord_Move(center, r2, static_cast<uint16_t>(sight)), x1, y1);
              view.DrawLine(x, y + 8, x1, y1 + 8, kWhite);
            }
            if (TheMap().Coord_To_Pixel(
                    Coord_Move(center2, r1, static_cast<uint16_t>(weapon)), x,
                    y)) {
              TheMap().Coord_To_Pixel(
                  Coord_Move(center2, r2, static_cast<uint16_t>(weapon)), x1,
                  y1);
              view.DrawLine(x, y + 8, x1, y1 + 8, kRed);
            }
          }
        }
        break;

      /*
      **	Reveal the entire map to the player.
      */
      case (KN_F4 | KN_CTRL_BIT):
        TheDebugState().set_unshroud(!TheDebugState().unshroud());
        TheMap().Flag_To_Redraw(true);
        break;

      default:
        break;
    }
  }
}
