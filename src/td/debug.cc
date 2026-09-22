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

/* $Header:   F:\projects\c&c\vcs\code\debug.cpv   2.17   16 Oct 1995 16:49:18
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
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
 *                  Last Update : July 5, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:
 *   Debug_Key -- Debug mode keyboard processing.
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/debug.h"

#include <cstdint>
#include <cstdio>
#include <filesystem>

#include "absl/strings/str_format.h"
#include "sdllib/keyboard.h"
#include "sdllib/pixel_buffer.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "td/aircraft.h"
#include "td/anim.h"
#include "td/building.h"
#include "td/combat.h"
#include "td/const.h"
#include "td/coord.h"
#include "td/debug_state.h"
#include "td/defines.h"
#include "td/ending.h"
#include "td/gscreen.h"
#include "td/heap.h"
#include "td/house.h"
#include "td/jshell.h"
#include "td/mapedit.h"
#include "td/object.h"
#include "td/object_heaps.h"
#include "td/palette.h"
#include "td/screen.h"
#include "td/session.h"
#include "td/team.h"
#include "td/techno.h"
#include "td/type.h"
#include "td/vector.h"
#include "td/world.h"
#include "tech/audio_mixer.h"
#include "tech/pcx_file.h"

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
      /*
      ** Start saving off screens
      */
      case KN_K | KN_CTRL_BIT:
        ScreenRecording = true;
        break;

      case KN_K:
        /*
        ** time to create a screen shot using the PCX code (if it works)
        */
        {
          PixelBuffer temp_page(
              TheScreen().visible_view().width(),
              TheScreen().visible_view().height(), {},
              static_cast<int32_t>(TheScreen().visible_view().width()) *
                  TheScreen().visible_view().height());
          char filename[30];

          TheScreen().visible_view().Blit(temp_page.view());
          for (int lp = 0; lp < 99; lp++) {
            if (lp < 10) {
              absl::SNPrintF(filename, sizeof(filename), "scrsht0%d.pcx", lp);
            } else {
              absl::SNPrintF(filename, sizeof(filename), "scrsht%d.pcx", lp);
            }
            if (!std::filesystem::exists(filename)) {
              break;
            }
          }

          Write_PCX_File(filename, temp_page.view(), CurrentPalette);
          // Map.Place_Random_Crate();
        }
        break;

      case KN_P:
        Keyboard::Clear();
        while (!Keyboard::Check()) {
          TheAudio().PumpStreams();
        }
        Keyboard::Clear();
        break;

      case KN_O: {
        auto* air = new AircraftClass(AIRCRAFT_ORCA, ThePlayer()->Class->House);
        if (air) {
          air->Altitude = 0;
          air->Unlimbo(TheMap().Pixel_To_Coord(Get_Mouse_X(), Get_Mouse_Y()),
                       DIR_N);
        }
      } break;

      case KN_B | KN_ALT_BIT: {
        TheDebugState().set_instant_build(!TheDebugState().instant_build());
      } break;
      case KN_B: {
        auto* air =
            new AircraftClass(AIRCRAFT_HELICOPTER, ThePlayer()->Class->House);
        if (air) {
          air->Altitude = 0;
          air->Unlimbo(TheMap().Pixel_To_Coord(Get_Mouse_X(), Get_Mouse_Y()),
                       DIR_N);
        }
      } break;

      case KN_T: {
        auto* air =
            new AircraftClass(AIRCRAFT_TRANSPORT, ThePlayer()->Class->House);
        if (air) {
          air->Altitude = 0;
          air->Unlimbo(TheMap().Pixel_To_Coord(Get_Mouse_X(), Get_Mouse_Y()),
                       DIR_N);
        }
      } break;

      case KN_GRAVE:
        new AnimClass(ANIM_ART_EXP1,
                      TheMap().Pixel_To_Coord(Get_Mouse_X(), Get_Mouse_Y()));
        Explosion_Damage(TheMap().Pixel_To_Coord(Get_Mouse_X(), Get_Mouse_Y()),
                         250, nullptr, WARHEAD_HE);
        break;

      case KN_Z:
        //				new AnimClass(ANIM_LZ_SMOKE,
        // Map.Pixel_To_Coord(Get_Mouse_X(), Get_Mouse_Y()));
        GDI_Ending();
        break;

      case KN_C:
        TheDebugState().set_build_anything(!TheDebugState().build_anything());
        ThePlayer()->IsRecalcNeeded = true;
        ThePlayer()->Add_Nuke_Piece();
        ThePlayer()->Add_Nuke_Piece();
        ThePlayer()->Add_Nuke_Piece();

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
          TheMap().MapCellWidth = 62;
          TheMap().MapCellHeight = 62;
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

#ifdef NEVER
      case KN_G:
        HouseClass::As_Pointer(HOUSE_GOOD)
            ->Flag_Attach(
                TheMap().Click_Cell_Calc(Get_Mouse_X(), Get_Mouse_Y()));
        break;

      case KN_N:
        HouseClass::As_Pointer(HOUSE_BAD)->Flag_Attach(
            TheMap().Click_Cell_Calc(Get_Mouse_X(), Get_Mouse_Y()));
        break;
#endif

      case KN_R:
        if (TheWorld().current_object().Count()) {
          dynamic_cast<TechnoClass*>(TheWorld().current_object().at(0))
              ->IsCloakable = true;
        }
        break;

      case KN_W | KN_ALT_BIT:
        ThePlayer()->Flag_To_Win();
        break;

      case KN_L | KN_ALT_BIT:
        ThePlayer()->Flag_To_Lose();
        break;

      case KN_F:
        TheDebugState().set_trace_path_search(
            !TheDebugState().trace_path_search());
        break;

      case KN_DELETE:
        if (TheWorld().current_object().Count()) {
          TheMap().Recalc();
          // CurrentObject[0]->Detach_All();
          delete TheWorld().current_object().at(0);
        }
        break;

      case KN_D:
        if (TheObjectHeaps().team().Ptr(0)) {
          delete TheObjectHeaps().team().Ptr(0);
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

#ifdef NEVER
      case (KN_F1 | KN_SHIFT_BIT):
        TheSpecial().IsBarOn = (TheSpecial().IsBarOn == false);
        TheMap().Flag_To_Redraw(true);
        break;

      case (KN_F1 | KN_SHIFT_BIT):  // quick load/save for debugging
        if (!Save_Game(0, "Command & Conquer Save Game File")) {
          CCMessageBox().Process("Error saving game!");
          Prog_End();
          exit(EXIT_SUCCESS);
        }
        break;

      case (KN_F2 | KN_SHIFT_BIT):  // quick load/save for debugging
        if (!Load_Game(0)) {
          CCMessageBox().Process("Error loading game!");
          Prog_End();
          exit(EXIT_SUCCESS);
        }
        break;

        // #ifdef SCENARIO_EDITOR
      case KN_F2:  // enable/disable the map editor
        Go_Editor(!TheDebugState().map_editor_active());
        break;
// #endif
#endif

#ifdef NEVER
      case (KN_F3 | KN_ALT_BIT):  // quick load/save for debugging
        TheDebugState().set_show_threat(!TheDebugState().show_threat());
        TheMap().Flag_To_Redraw(true);
        break;

#endif

      case KN_F3:
        TheDebugState().set_show_cell_info(!TheDebugState().show_cell_info());
        TheMap().Flag_To_Redraw(true);
        break;

      /*
      **	Reveal entire map to player.
      */
      case KN_F4:
        if (TheSession().type() == GAME_NORMAL) {
          TheDebugState().set_unshroud(!TheDebugState().unshroud());
          TheMap().Flag_To_Redraw(true);
        }
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
          if (ttype.Primary != WEAPON_NONE) {
            weapon = Weapons.at(ttype.Primary).Range;
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

      case (KN_F4 | KN_CTRL_BIT):
        TheDebugState().set_unshroud(!TheDebugState().unshroud());
        TheMap().Flag_To_Redraw(true);
        break;

#ifdef NEVER
      case KN_F5:
        TheSpecial().IsShowPath = (TheSpecial().IsShowPath == false);
        // PlayerPtr->Credits += 1000;
        break;

      case (KN_F9 | KN_CTRL_BIT):
        if (HouseClass::As_Pointer(HOUSE_GOOD)) {
          (HouseClass::As_Pointer(HOUSE_GOOD))->Blowup_All();
        }
        break;

      case (KN_F10 | KN_CTRL_BIT):
        if (HouseClass::As_Pointer(HOUSE_BAD)) {
          (HouseClass::As_Pointer(HOUSE_BAD))->Blowup_All();
        }
        break;
#endif
      default:
        break;
    }
  }
}
