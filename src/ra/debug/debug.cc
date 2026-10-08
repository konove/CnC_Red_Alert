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

#include "ra/debug/debug.h"

#include <cstdint>
#include <iterator>

#include "base/array.h"
#include "base/pooled.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/core/object_heap.h"
#include "game/sim/direction.h"
#include "game/ui/input.h"
#include "game/ui/standard_colors.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/assets/screen.h"
#include "ra/core/coord.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/core/random_pick.h"
#include "ra/debug/debug_state.h"
#include "ra/editor/mapedit.h"
#include "ra/map/display.h"
#include "ra/map/vortex.h"
#include "ra/map/world.h"
#include "ra/objects/aircraft.h"
#include "ra/objects/anim.h"
#include "ra/objects/building.h"
#include "ra/objects/cell.h"
#include "ra/objects/combat.h"
#include "ra/objects/house.h"
#include "ra/objects/object.h"
#include "ra/objects/super.h"
#include "ra/objects/type.h"
#include "ra/objects/weapon.h"

namespace ra {

using enum engine::window::KeyNumber;

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
          const int xxxx = game::TheMouse().x() + TheMap().TacPixelX;
          const int yyyy = game::TheMouse().y() + TheMap().TacPixelY;
          const Cell cell = TheMap().DisplayClass::Click_Cell_Calc(xxxx, yyyy);
          TheWorld().chronal_vortex().Appear(cell.Center());
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
        auto* air = base::NewPooled<AircraftClass>(AIRCRAFT_HIND,
                                                   ThePlayer()->Class->House);
        if (air) {
          air->Height = 0;
          air->Unlimbo(TheMap().Pixel_To_Coord(game::TheMouse().x(),
                                               game::TheMouse().y()),
                       game::Direction::kNorth);
        }
      } break;

      case KN_B: {
        auto* air = base::NewPooled<AircraftClass>(AIRCRAFT_LONGBOW,
                                                   ThePlayer()->Class->House);
        if (air) {
          air->Height = 0;
          air->Unlimbo(TheMap().Pixel_To_Coord(game::TheMouse().x(),
                                               game::TheMouse().y()),
                       game::Direction::kNorth);
        }
      } break;

      case KN_GRAVE: {
        const WarheadType warhead = RandomPick(WARHEAD_HE, WARHEAD_FIRE);
        const Coordinate coord =
            TheMap().Pixel_To_Coord(game::TheMouse().x(), game::TheMouse().y());
        const int damage = 1000;
        base::NewPooled<AnimClass>(
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
            TheObjectHeaps().building().ActiveAt(index)->Update_Buildables();
          }
        }
        break;

      case Alt(KN_Z):
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

      case Alt(KN_W):
        ThePlayer()->Flag_To_Win();
        break;

      case Alt(KN_L):
        ThePlayer()->Flag_To_Lose();
        break;

      case KN_DELETE:
        if (std::ssize(TheWorld().current_object())) {
          TheMap().Recalc();
          // CurrentObject[0]->Detach_All();
          if (base::At(TheWorld().current_object(), 0)->What_Am_I() ==
              RTTI_BUILDING) {
            dynamic_cast<BuildingClass*>(
                base::At(TheWorld().current_object(), 0))
                ->Sell_Back(1);
          } else {
            ObjectClass* object = base::At(TheWorld().current_object(), 0);
            object->Unselect();
            object->Limbo();
            base::DeletePooled(object);
          }
        }
        break;

      case Shift(KN_DELETE):
        if (std::ssize(TheWorld().current_object())) {
          TheMap().Recalc();
          int damage = 50;
          base::At(TheWorld().current_object(), 0)
              ->Take_Damage(damage, 0, WARHEAD_SA);
        }
        break;

      case KN_INSERT:
        if (std::ssize(TheWorld().current_object())) {
          TheMap().PendingObject =
              &base::At(TheWorld().current_object(), 0)->Class_Of();
          if (TheMap().PendingObject) {
            TheMap().PendingHouse =
                base::At(TheWorld().current_object(), 0)->Owner();
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
        if (std::ssize(TheWorld().current_object()) &&
            base::At(TheWorld().current_object(), 0)->Is_Techno()) {
          const auto& ttype = dynamic_cast<const TechnoTypeClass&>(
              base::At(TheWorld().current_object(), 0)->Class_Of());
          const int sight = ttype.SightRange * 256;
          int weapon = 0;
          if (ttype.PrimaryWeapon != nullptr) {
            weapon = ttype.PrimaryWeapon->Range;
          }
          PixelView& view = TheScreen().visible_view();
          const Coordinate center =
              base::At(TheWorld().current_object(), 0)->Center_Coord();
          const Coordinate center2 =
              base::At(TheWorld().current_object(), 0)->Fire_Coord(0);

          for (int r = 0; r < 255; r += 10) {
            int x = 0;
            int y = 0;
            int x1 = 0;
            int y1 = 0;
            const game::Direction r1 = game::Direction(r);
            const game::Direction r2 = game::Direction(r + 10);

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
      case Ctrl(KN_F4):
        TheDebugState().set_unshroud(!TheDebugState().unshroud());
        TheMap().Flag_To_Redraw(true);
        break;

      default:
        break;
    }
  }
}

}  // namespace ra
