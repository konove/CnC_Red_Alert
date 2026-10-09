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

/* $Header: /CounterStrike/MAPEDDLG.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : MAPEDDLG.CPP                             *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : November 18, 1994                        *
 *                                                                         *
 *                  Last Update : September 4, 1996 [JLB]                  *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Map Editor dialogs & main menu options                                  *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   MapEditClass::Handle_Triggers -- processes the trigger dialogs        *
 *   MapEditClass::Load_Scenario -- loads a scenario INI file              *
 *   MapEditClass::New_Scenario -- creates a new scenario                  *
 *   MapEditClass::Pick_Scenario -- dialog for choosing scenario           *
 *   MapEditClass::Save_Scenario -- saves current scenario to an INI file  *
 *   MapEditClass::Scenario_Dialog -- scenario global parameters dialog    *
 *   MapEditClass::Select_Trigger -- lets user select a trigger            *
 *   MapEditClass::Size_Map -- lets user set size & location of map        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string_view>
#include <utility>

#include "absl/strings/ascii.h"
#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/enum_array.h"
#include "base/numeric.h"
#include "base/pooled.h"
#include "base/strings/number_parse.h"
#include "base/strings/safe_string.h"
#include "engine/file/mix_archive.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/platform/platform.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/core/object_heap.h"
#include "game/sim/enums.h"
#include "game/ui/gadget.h"
#include "game/ui/gadget_list.h"
#include "game/ui/input.h"
#include "game/ui/standard_colors.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/prebuilt_base.h"
#include "ra/ai/scenario.h"
#include "ra/ai/tevent.h"
#include "ra/ai/trigger_type.h"
#include "ra/assets/compat.h"
#include "ra/assets/palette.h"
#include "ra/assets/palettes.h"
#include "ra/assets/screen.h"
#include "ra/assets/theme.h"
#include "ra/core/const.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/data/rules.h"
#include "ra/data/text_ids.h"
#include "ra/debug/debug.h"
#include "ra/editor/mapedit.h"
#include "ra/map/world.h"
#include "ra/net/session.h"
#include "ra/objects/cell.h"
#include "ra/objects/house.h"
#include "ra/objects/object.h"
#include "ra/objects/terrain.h"
#include "ra/objects/tracker.h"
#include "ra/objects/type.h"
#include "ra/startup/conquer.h"
#include "ra/ui/checkbox.h"
#include "ra/ui/cheklist.h"
#include "ra/ui/color_scheme.h"
#include "ra/ui/dialog.h"
#include "ra/ui/drop.h"
#include "ra/ui/edit.h"
#include "ra/ui/list.h"
#include "ra/ui/msgbox.h"
#include "ra/ui/slider.h"
#include "ra/ui/statbtn.h"
#include "ra/ui/textbtn.h"
#include "ra/ui/tlist.h"

namespace ra {

using enum engine::window::KeyNumber;

/***************************************************************************
 * MapEditClass::New_Scenario -- creates a new scenario                    *
 *                                                                         *
 * - Prompts user for scenario data (house, scenario #); sets globals      *
 *     PlayerPtr (for house) & Scenario (for scenario #)                   *
 *   - Prompts user for map size                                           *
 * - Initializes the scenario by calling Clear_Scenario(), which calls     *
 *     everybody's Init() routine                                          *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      0 = new scenario created, -1 = not                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/21/1994 BR : Created.                                              *
 *=========================================================================*/
int MapEditClass::New_Scenario() {
  int scen_num = 0;
  ScenarioPlayerType player = SCEN_PLAYER_NONE;
  game::ScenarioDirType dir = game::SCEN_DIR_NONE;
  ScenarioVarType var = SCEN_VAR_NONE;
  Disect_Scenario_Name(TheScenario().ScenarioName, scen_num, player, dir, var);

  /*
  **	Force the house save value to match the player house.
  */
  if (ThePlayer()) {
    switch (ThePlayer()->Class->House) {
      case HOUSE_SPAIN:
        player = SCEN_PLAYER_SPAIN;
        break;

      case HOUSE_GREECE:
        player = SCEN_PLAYER_GREECE;
        break;

      case HousesType::HOUSE_NONE:
      case HousesType::HOUSE_ENGLAND:
      case HousesType::HOUSE_UKRAINE:
      case HousesType::HOUSE_GERMANY:
      case HousesType::HOUSE_FRANCE:
      case HousesType::HOUSE_TURKEY:
      case HousesType::HOUSE_GOOD:
      case HousesType::HOUSE_BAD:
      case HousesType::HOUSE_NEUTRAL:
      case HousesType::HOUSE_JP:
      case HousesType::HOUSE_MULTI1:
      case HousesType::HOUSE_MULTI2:
      case HousesType::HOUSE_MULTI3:
      case HousesType::HOUSE_MULTI4:
      case HousesType::HOUSE_MULTI5:
      case HousesType::HOUSE_MULTI6:
      case HousesType::HOUSE_MULTI7:
      case HousesType::HOUSE_MULTI8:
      default:
      case HOUSE_USSR:
        player = SCEN_PLAYER_USSR;
        break;
    }
  }

  /*
  **	Prompt for scenario info
  */
  const int rc = Pick_Scenario("New Scenario", scen_num, player, dir, var);
  if (rc != 0) {
    return (-1);
  }

  TheWorld().scenario_init()++;

  /*
  **	Blow away everything
  */
  Clear_Scenario();

  /*
  **	Set parameters
  */
  //	Scen.Scenario = scen_num;
  //	Scen.ScenPlayer = player;
  //	Scen.ScenDir = dir;
  //	Scen.ScenVar = var;
  TheScenario().Set_Scenario_Name(scen_num, player, dir, var);

  /*
  **	Create houses
  */
  for (const HousesType house : magic_enum::enum_values<HousesType>()) {
    base::NewPooled<HouseClass>(house);
  }

  switch (player) {
    case SCEN_PLAYER_MPLAYER:
      ThePlayer() = HouseClass::As_Pointer(HOUSE_MULTI1);
      ThePlayer()->IsHuman = true;
      LastHouse = HOUSE_MULTI1;
      break;

    case SCEN_PLAYER_USSR:
      ThePlayer() = HouseClass::As_Pointer(HOUSE_USSR);
      ThePlayer()->IsHuman = true;
      TheWorld().base().set_house(HOUSE_SPAIN);
      LastHouse = HOUSE_GOOD;
      break;

    case SCEN_PLAYER_SPAIN:
      ThePlayer() = HouseClass::As_Pointer(HOUSE_SPAIN);
      ThePlayer()->IsHuman = true;
      TheWorld().base().set_house(HOUSE_USSR);
      LastHouse = HOUSE_GOOD;
      break;

    case SCEN_PLAYER_GREECE:
      ThePlayer() = HouseClass::As_Pointer(HOUSE_GREECE);
      ThePlayer()->IsHuman = true;
      TheWorld().base().set_house(HOUSE_USSR);
      LastHouse = HOUSE_GOOD;
      break;
    case ScenarioPlayerType::SCEN_PLAYER_NONE:
    case ScenarioPlayerType::SCEN_PLAYER_JP:
    case ScenarioPlayerType::SCEN_PLAYER_2PLAYER:
    default:
      break;
  }

  /*
  **	Init the entire map
  */
  //	Init_Clear();
  Fill_In_Data();

  /*
  **	Prompt for map size
  */
  Size_Map(-1, -1, 30, 30);

  /*
  **	Set the Home & Reinforcement Cells to the center of the map
  */
  base::At(TheScenario().Waypoint, ScenarioClass::kReinforcementWaypoint) =
      Cell::FromXY(MapCellX + (MapCellWidth / 2),
                   MapCellY + (MapCellHeight / 2));
  base::At(TheScenario().Waypoint, ScenarioClass::kHomeWaypoint) = Cell::FromXY(
      MapCellX + (MapCellWidth / 2), MapCellY + (MapCellHeight / 2));
  (*this).at(TacticalCoord).IsWaypoint = true;
  Flag_Cell(Cell::Containing(TacticalCoord));

  Set_Tactical_Position(
      base::At(TheScenario().Waypoint, ScenarioClass::kHomeWaypoint)
          .Offset(-(MAP_CELL_W * 8) - 10)
          .Center());
  TheWorld().scenario_init()--;

  return 0;
}

/***************************************************************************
 * MapEditClass::Load_Scenario -- loads a scenario INI file                *
 *                                                                         *
 * - Prompts user for scenario data (house, scenario #); sets globals      *
 *     PlayerPtr (for house) & Scenario (for scenario #)                   *
 * - Loads the INI file for that scenario                                  *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      0.                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/21/1994 BR : Created.                                              *
 *=========================================================================*/
int MapEditClass::Load_Scenario() {
  int scen_num = 0;
  ScenarioPlayerType player = SCEN_PLAYER_NONE;
  game::ScenarioDirType dir = game::SCEN_DIR_NONE;
  ScenarioVarType var = SCEN_VAR_NONE;
  Disect_Scenario_Name(TheScenario().ScenarioName, scen_num, player, dir, var);

  /*
  **	Prompt for scenario info
  */
  const int rc = Pick_Scenario("Load Scenario", scen_num, player, dir, var);
  if (rc != 0) {
    return (-1);
  }

  /*
  **	Set parameters
  */
  //	Scen.Scenario = scen_num;
  //	Scen.ScenPlayer = player;
  //	Scen.ScenDir = dir;
  //	Scen.ScenVar = var;
  TheScenario().Set_Scenario_Name(scen_num, player, dir, var);

  /*
  **	Read_Scenario_Ini() must be able to set PlayerPtr to the right house:
  **	- Reading the INI will create the house objects
  **	- PlayerPtr must be set before any Techno objects are created
  **	- For GDI or NOD scenarios, PlayerPtr is set by reading the INI;
  **	  but for multiplayer, it's set via the Players vector; so, here we have
  **	  to set various multiplayer variables to fool the Assign_Houses()
  *routine *	  into working properly.
  */
  if (player == SCEN_PLAYER_MPLAYER) {
    TheSession().Players.Clear();

    auto who = std::make_unique<NodeNameType>();  // node to add to Players
    base::SafeCopy(who->Name, TheSession().Handle);
    who->Player.House = TheSession().House;
    who->Player.Color = TheSession().ColorIdx;
    TheSession().Players.Add(std::move(who));

    TheSession().NumPlayers = 1;
    LastHouse = HOUSE_MULTI1;
  } else {
      LastHouse = HOUSE_GOOD;
    }

    /*
    **	Blow away everything
    */
    Clear_Scenario();

    /*
    **	Read the INI
    */
    if (!Read_Scenario_INI(TheScenario().ScenarioName)) {
      if (TheScenario().Scenario < 20 &&
          base::At(TheScenario().ScenarioName, 2) == 'G') {
        WWMessageBox().Process("Please insert Red Alert CD1");
      } else if (TheScenario().Scenario < 20 &&
                 base::At(TheScenario().ScenarioName, 2) == 'U') {
        WWMessageBox().Process("Please insert Red Alert CD2");
      } else {
        WWMessageBox().Process("Unable to read scenario!");
      }
      TheScreen().hidden_view().Clear();
      Flag_To_Redraw(true);
      Render();
    } else {
      Fill_In_Data();
      ThePalettes().game_palette().Set();
      //		Set_Palette(GamePalette);
    }

    return 0;
  }

  /***************************************************************************
   * MapEditClass::Save_Scenario -- saves current scenario to an INI file    *
   *                                                                         *
   * - Prompts user for scenario data (house, scenario #); sets globals      *
   *     PlayerPtr (for house) & Scenario (for scenario #)                   *
   * - Saves the INI file for this scenario                                  *
   *                                                                         *
   * INPUT:                                                                  *
   *      none.                                                              *
   *                                                                         *
   * OUTPUT:                                                                 *
   *      0 = OK, -1 = error/cancel                                          *
   *                                                                         *
   * WARNINGS:                                                               *
   *      none.                                                              *
   *                                                                         *
   * HISTORY:                                                                *
   *   10/21/1994 BR : Created.                                              *
   *=========================================================================*/
  int MapEditClass::Save_Scenario() {
    int scen_num = 0;
    ScenarioPlayerType player = SCEN_PLAYER_NONE;
    game::ScenarioDirType dir = game::SCEN_DIR_NONE;
    ScenarioVarType var = SCEN_VAR_NONE;

    Disect_Scenario_Name(TheScenario().ScenarioName, scen_num, player, dir,
                         var);

    //	FILE * fp;
    //	char fname[13];

    /*
    **	Prompt for scenario info
    */
    const int rc = Pick_Scenario("Save Scenario", scen_num, player, dir, var);
    if (rc != 0) {
      return (-1);
    }

    /*
    **	Warning if scenario already exists
    */
    //	Scen.Set_Scenario_Name(scen_num, player, dir, var);
    //	fp = fopen(fname, "rb");
    //	if (fp) {
    //		fclose(fp);
    //		rc = WWMessageBox().Process("File exists. Replace?", TXT_YES,
    // TXT_NO); 		hidden_view.Clear();
    // Flag_To_Redraw(true); 		Render(); 		if (rc==1) {
    //			return(-1);
    //		}
    //	}

    /*
    **	Set parameters
    */
    //	Scen.Scenario = scen_num;
    //	Scen.ScenPlayer = player;
    //	Scen.ScenDir = dir;
    //	Scen.ScenVar = var;
    TheScenario().Set_Scenario_Name(scen_num, player, dir, var);

    /*
    **	Player may have changed from GDI to NOD, so change playerptr accordingly
    */
    switch (player) {
      case SCEN_PLAYER_USSR:
        ThePlayer() = HouseClass::As_Pointer(HOUSE_USSR);
        ThePlayer()->IsHuman = true;
        //			Base.House = HOUSE_SPAIN;
        LastHouse = HOUSE_GOOD;
        break;

      case SCEN_PLAYER_SPAIN:
        ThePlayer() = HouseClass::As_Pointer(HOUSE_SPAIN);
        ThePlayer()->IsHuman = true;
        //			Base.House = HOUSE_USSR;
        LastHouse = HOUSE_GOOD;
        break;

      case SCEN_PLAYER_GREECE:
        ThePlayer() = HouseClass::As_Pointer(HOUSE_GREECE);
        ThePlayer()->IsHuman = true;
        //			Base.House = HOUSE_USSR;
        LastHouse = HOUSE_GOOD;
        break;
      case ScenarioPlayerType::SCEN_PLAYER_NONE:
      case ScenarioPlayerType::SCEN_PLAYER_JP:
      case ScenarioPlayerType::SCEN_PLAYER_2PLAYER:
      case ScenarioPlayerType::SCEN_PLAYER_MPLAYER:
      default:
        break;
    }

    /*
    **	Write the INI
    */
    Write_Scenario_INI(TheScenario().ScenarioName);

    return 0;
  }

  /***************************************************************************
   * MapEditClass::Pick_Scenario -- dialog for choosing scenario             *
   *                                                                         *
   * Prompts user for:                                                       *
   *   - House (GDI, NOD)                                                    *
   *   - Scenario #                                                          *
   *                                                                         *
   *           Ŀ                          *
   *                       Caption                                         *
   *                                                                       *
   *                    Scenario ___                                       *
   *                     Version ___                                       *
   *                                                                       *
   *                    [East]  [West]                                     *
   *                                                                       *
   *                    [    GDI     ]                                     *
   *                    [    NOD     ]                                     *
   *                    [Multi-Player]                                     *
   *                                                                       *
   *                    [OK]  [Cancel]                                     *
   *                                                                       *
   *                                     *
   *                                                                         *
   * INPUT:                                                                  *
   *      caption      string to use as a title                              *
   *      scen_nump   output: ptr to scenario #                              *
   *      playerp      output: ptr to player type                            *
   *      dirp         output: ptr to direction                              *
   *      varp         output: ptr to variation                              *
   *      multi         1 = allow to change single/multiplayer; 0 = not      *
   *                                                                         *
   * OUTPUT:                                                                 *
   *      0 = OK, -1 = cancel                                                *
   *                                                                         *
   * WARNINGS:                                                               *
   *      none.                                                              *
   *                                                                         *
   * HISTORY:                                                                *
   *   10/21/1994 BR : Created.                                              *
   *   09/04/1996 JLB : Simplified                                           *
   *=========================================================================*/
  int MapEditClass::Pick_Scenario(const char* caption, int& scen_nump,
                                  ScenarioPlayerType& playerp,
                                  game::ScenarioDirType& dirp,
                                  ScenarioVarType& varp) {
    /*
    **	Dialog & button dimensions
    */
    constexpr int kDDialogW = 200;                      // dialog width
    constexpr int kDDialogH = 164;                      // dialog height
    constexpr int kDDialogX = ((320 - kDDialogW) / 2);  // centered x-coord
    constexpr int kDDialogY = ((200 - kDDialogH) / 2);  // centered y-coord
    constexpr int kDDialogCx =
        kDDialogX + (kDDialogW / 2);         // coord of x-center
    constexpr int kDTxt8H = 11;              // ht of 8-pt text
    constexpr int kDMargin = 7;              // margin width/height
    constexpr int kDScenW = 45;              // Scenario # width
    constexpr int kDScenH = 9;               // Scenario # height
    constexpr int kDScenX = kDDialogCx + 5;  // Scenario # x
    constexpr int kDScenY =
        kDDialogY + kDMargin + kDTxt8H + kDMargin;  // Scenario # y
    constexpr int kDVaraW = 13;                     // Version A width
    constexpr int kDVaraH = 9;                      // Version A height
    constexpr int kDVaraX = kDDialogCx - ((kDVaraW * 5) / 2);  // Version A x
    constexpr int kDVaraY = kDScenY + kDScenH + kDMargin;      // Version A y
    constexpr int kDVarbW = 13;                            // Version B width
    constexpr int kDVarbH = 9;                             // Version B height
    constexpr int kDVarbX = kDVaraX + kDVaraW;             // Version B x
    constexpr int kDVarbY = kDScenY + kDScenH + kDMargin;  // Version B y
    constexpr int kDVarcW = 13;                            // Version C width
    constexpr int kDVarcH = 9;                             // Version C height
    constexpr int kDVarcX = kDVarbX + kDVarbW;             // Version C x
    constexpr int kDVarcY = kDScenY + kDScenH + kDMargin;  // Version C y
    constexpr int kDVardW = 13;                            // Version D width
    constexpr int kDVardH = 9;                             // Version D height
    constexpr int kDVardX = kDVarcX + kDVarcW;             // Version D x
    constexpr int kDVardY = kDScenY + kDScenH + kDMargin;  // Version D y
    constexpr int kDVarloseH = 9;  // Version Lose height
    constexpr int kDVarloseY = kDScenY + kDScenH + kDMargin;  // Version Lose y
    constexpr int kDEastW = 50;                               // EAST width
    constexpr int kDEastH = 9;                                // EAST height
    constexpr int kDEastX = kDDialogCx - kDEastW - 5;         // EAST x
    constexpr int kDEastY = kDVarloseY + kDVarloseH + kDMargin;  // EAST y
    constexpr int kDWestW = 50;                                  // WEST width
    constexpr int kDWestH = 9;                                   // WEST height
    constexpr int kDWestX = kDDialogCx + 5;                      // WEST x
    constexpr int kDWestY = kDVarloseY + kDVarloseH + kDMargin;  // EAST y
    constexpr int kDGdiW = 90;                                   // GDI width
    constexpr int kDGdiH = 9;                                    // GDI height
    constexpr int kDGdiX = kDDialogCx - (kDGdiW / 2);            // GDI x
    constexpr int kDGdiY = kDEastY + kDEastH + kDMargin;         // GDI y
    constexpr int kDNodW = 90;                                   // NOD width
    constexpr int kDNodH = 9;                                    // NOD height
    constexpr int kDNodX = kDDialogCx - (kDNodW / 2);            // NOD x
    constexpr int kDNodY = kDGdiY + kDGdiH;                      // NOD y
    constexpr int kDNeuW = 90;                         // Neutral width
    constexpr int kDNeuH = 9;                          // Neutral height
    constexpr int kDNeuX = kDDialogCx - (kDNodW / 2);  // Neutral x
    constexpr int kDNeuY = kDNodY + kDNodH;            // Neutral y
    constexpr int kDMplayerW = 90;                     // Multi-Player width
    constexpr int kDMplayerH = 9;                      // Multi-Player height
    constexpr int kDMplayerX = kDDialogCx - (kDMplayerW / 2);  // Multi-Player x
    constexpr int kDMplayerY = kDNeuY + kDNeuH;                // Multi-Player y
    constexpr int kDOkW = 45;                                  // OK width
    constexpr int kDOkH = 9;                                   // OK height
    constexpr int kDOkX = kDDialogCx - kDOkW - 5;              // OK x
    constexpr int kDOkY =
        kDDialogY + kDDialogH - kDOkH - (kDMargin + 15);  // OK y
    constexpr int kDCancelW = 45;                         // Cancel width
    constexpr int kDCancelH = 9;                          // Cancel height
    constexpr int kDCancelX = kDDialogCx + 5;             // Cancel x
    constexpr int kDCancelY =
        kDDialogY + kDDialogH - kDCancelH - (kDMargin + 15);  // Cancel y

    /*
    **	Button enumerations
    */
    constexpr int kButtonGdi = 100;
    constexpr int kButtonNod = 101;
    constexpr int kButtonNeutral = 102;
    constexpr int kButtonMplayer = 103;
    constexpr int kButtonEast = 104;
    constexpr int kButtonWest = 105;
    constexpr int kButtonOk = 106;
    constexpr int kButtonCancel = 107;
    constexpr int kButtonScenario = 108;
    constexpr int kButtonVarA = 109;
    constexpr int kButtonVarB = 110;
    constexpr int kButtonVarC = 111;
    constexpr int kButtonVarD = 112;

    /*
    **	Dialog variables
    */
    bool cancel = false;  // true = user cancels

    /*
    **	Other Variables
    */
    char scen_buf[10] = {0};  // buffer for editing scenario #

    /*
    **	Buttons
    */
    game::GadgetList commands;  // the button list
    EditClass editbtn(kButtonScenario, scen_buf, 5, FontType::kEditor,
                      TPF_NOSHADOW, kDScenX, kDScenY, kDScenW, kDScenH,
                      EditClass::kAlphanumeric);

    TextButtonClass varabtn(kButtonVarA, "A", kEButtonFont, kEButtonFlags,
                            kDVaraX, kDVaraY, kDVaraW, kDVaraH);
    TextButtonClass varbbtn(kButtonVarB, "B", kEButtonFont, kEButtonFlags,
                            kDVarbX, kDVarbY, kDVarbW, kDVarbH);
    TextButtonClass varcbtn(kButtonVarC, "C", kEButtonFont, kEButtonFlags,
                            kDVarcX, kDVarcY, kDVarcW, kDVarcH);
    TextButtonClass vardbtn(kButtonVarD, "D", kEButtonFont, kEButtonFlags,
                            kDVardX, kDVardY, kDVardW, kDVardH);
    TextButtonClass gdibtn(kButtonGdi, "North (Spain)", kEButtonFont,
                           kEButtonFlags, kDGdiX, kDGdiY, kDGdiW, kDGdiH);
    TextButtonClass nodbtn(kButtonNod, "South (Greece)", kEButtonFont,
                           kEButtonFlags, kDNodX, kDNodY, kDNodW, kDNodH);
    TextButtonClass neubtn(
        kButtonNeutral, HouseTypeClass::As_Reference(HOUSE_USSR).IniName,
        kEButtonFont, kEButtonFlags, kDNeuX, kDNeuY, kDNeuW, kDNeuH);
    TextButtonClass playermbtn(kButtonMplayer, "Multiplayer", kEButtonFont,
                               kEButtonFlags, kDMplayerX, kDMplayerY,
                               kDMplayerW, kDMplayerH);
    TextButtonClass eastbtn(kButtonEast, "East", kEButtonFont, kEButtonFlags,
                            kDEastX, kDEastY, kDEastW, kDEastH);
    TextButtonClass westbtn(kButtonWest, "West", kEButtonFont, kEButtonFlags,
                            kDWestX, kDWestY, kDWestW, kDWestH);
    TextButtonClass okbtn(kButtonOk, TXT_OK, kEButtonFont, kEButtonFlags, kDOkX,
                          kDOkY, kDOkW, kDOkH);
    TextButtonClass cancelbtn(kButtonCancel, TXT_CANCEL, kEButtonFont,
                              kEButtonFlags, kDCancelX, kDCancelY, kDCancelW,
                              kDCancelH);

    /*
    **	Initialize
    */
    PixelView& view = TheScreen().visible_view();

    if (scen_nump < 100) {
      absl::SNPrintF(scen_buf, sizeof(scen_buf), "%d",
                     scen_nump);  // init edit buffer
    } else {
      const char first = static_cast<char>(scen_nump / 36);
      const char second = static_cast<char>(scen_nump % 36);
      base::At(scen_buf, 0) = static_cast<char>(first + 'A');
      if (second < 10) {
        base::At(scen_buf, 1) = static_cast<char>(second + '0');
      } else {
        base::At(scen_buf, 1) = static_cast<char>((second - 10) + 'A');
      }
      base::At(scen_buf, 2) = 0;
    }
    editbtn.Set_Text(scen_buf, 5);

    varabtn.Turn_Off();
    varbbtn.Turn_Off();
    varcbtn.Turn_Off();
    vardbtn.Turn_Off();
    switch (varp) {
      case SCEN_VAR_A:
        varabtn.Turn_On();
        break;

      case SCEN_VAR_B:
        varbbtn.Turn_On();
        break;

      case SCEN_VAR_C:
        varcbtn.Turn_On();
        break;

      case SCEN_VAR_D:
        vardbtn.Turn_On();
        break;
      case ScenarioVarType::SCEN_VAR_NONE:
      case ScenarioVarType::SCEN_VAR_LOSE:
      default:
        break;
    }

    /*
    **	Create the button list
    */
    commands.Add(editbtn);
    commands.Add(varabtn);
    commands.Add(varbbtn);
    commands.Add(varcbtn);
    commands.Add(vardbtn);
    commands.Add(gdibtn);
    commands.Add(nodbtn);
    commands.Add(neubtn);
    commands.Add(playermbtn);
    commands.Add(eastbtn);
    commands.Add(westbtn);
    commands.Add(okbtn);
    commands.Add(cancelbtn);

    /*
    **	Init the button states
    */
    gdibtn.Turn_Off();
    nodbtn.Turn_Off();
    neubtn.Turn_Off();
    playermbtn.Turn_Off();
    if (playerp == SCEN_PLAYER_MPLAYER) {
      playermbtn.Turn_On();
    } else {
      if (ThePlayer()) {
        switch (ThePlayer()->Class->House) {
          case HOUSE_SPAIN:
            gdibtn.Turn_On();
            break;

          case HOUSE_GREECE:
            nodbtn.Turn_On();
            break;

          case HOUSE_USSR:
            neubtn.Turn_On();
            break;
          case HousesType::HOUSE_NONE:
          case HousesType::HOUSE_ENGLAND:
          case HousesType::HOUSE_UKRAINE:
          case HousesType::HOUSE_GERMANY:
          case HousesType::HOUSE_FRANCE:
          case HousesType::HOUSE_TURKEY:
          case HousesType::HOUSE_GOOD:
          case HousesType::HOUSE_BAD:
          case HousesType::HOUSE_NEUTRAL:
          case HousesType::HOUSE_JP:
          case HousesType::HOUSE_MULTI1:
          case HousesType::HOUSE_MULTI2:
          case HousesType::HOUSE_MULTI3:
          case HousesType::HOUSE_MULTI4:
          case HousesType::HOUSE_MULTI5:
          case HousesType::HOUSE_MULTI6:
          case HousesType::HOUSE_MULTI7:
          case HousesType::HOUSE_MULTI8:
          default:
            break;
        }
      } else {
        switch (base::At(TheScenario().ScenarioName, 2)) {
          case 'G':
            gdibtn.Turn_On();
            break;

          case 'U':
            nodbtn.Turn_On();
            break;

          case 'M':
            playermbtn.Turn_On();
            break;
          default:
            break;
        }
      }
    }

    eastbtn.Turn_Off();
    westbtn.Turn_Off();
    if (dirp == game::SCEN_DIR_EAST) {
      eastbtn.Turn_On();
    } else {
      westbtn.Turn_On();
    }

    /*
    **	Main Processing Loop
    */
    bool display = true;
    bool process = true;
    while (process) {
      /*
      **	Invoke game callback
      */
      ServiceRealTime();

      /*
      **	Refresh display if needed
      */
      if (display) {
        game::TheMouse().Hide();
        Dialog_Box(kDDialogX, kDDialogY, kDDialogW, kDDialogH);
        Draw_Caption(view, caption, kDDialogX, kDDialogY, kDDialogW);
        Fancy_Text_Print(view, "Scenario", kDDialogCx - 5, kDScenY,
                         Get_Color_Scheme(), game::kTransparentBlack,
                         FontType::kEditor, TPF_RIGHT | TPF_NOSHADOW);
        commands.DrawAll(view);
        game::TheMouse().Show();

        display = false;
      }

      /*
      **	Get user input
      */
      const engine::window::KeyNumber input = commands.Input(view).key;

      /*
      **	Process input
      */
      switch (static_cast<int>(input)) {
        /*
        **	Handle a click on one of the scenario variation group buttons.
        */
        case engine::window::ButtonKey(kButtonVarA):
        case engine::window::ButtonKey(kButtonVarB):
        case engine::window::ButtonKey(kButtonVarC):
        case engine::window::ButtonKey(kButtonVarD):
          varabtn.Turn_Off();
          varbbtn.Turn_Off();
          varcbtn.Turn_Off();
          vardbtn.Turn_Off();
          switch (static_cast<int>(input)) {
            case engine::window::ButtonKey(kButtonVarA):
              varp = SCEN_VAR_A;
              varabtn.Turn_On();
              break;

            case engine::window::ButtonKey(kButtonVarB):
              varp = SCEN_VAR_B;
              varbbtn.Turn_On();
              break;

            case engine::window::ButtonKey(kButtonVarC):
              varp = SCEN_VAR_C;
              varcbtn.Turn_On();
              break;

            case engine::window::ButtonKey(kButtonVarD):
              varp = SCEN_VAR_D;
              vardbtn.Turn_On();
              break;
            default:
              break;
          }
          break;

        /*
        **	Handle a click on the east/west variation group.
        */
        case engine::window::ButtonKey(kButtonEast):
        case engine::window::ButtonKey(kButtonWest):
          westbtn.Turn_Off();
          eastbtn.Turn_Off();
          switch (static_cast<int>(input)) {
            case engine::window::ButtonKey(kButtonEast):
              dirp = game::SCEN_DIR_EAST;
              eastbtn.Turn_On();
              break;

            case engine::window::ButtonKey(kButtonWest):
              dirp = game::SCEN_DIR_WEST;
              westbtn.Turn_On();
              break;
            default:
              break;
          }
          break;

        /*
        **	Handle a click on one of the player category
        **	group buttons.
        */
        case engine::window::ButtonKey(kButtonGdi):
        case engine::window::ButtonKey(kButtonNod):
        case engine::window::ButtonKey(kButtonNeutral):
        case engine::window::ButtonKey(kButtonMplayer):
          gdibtn.Turn_Off();
          nodbtn.Turn_Off();
          neubtn.Turn_Off();
          playermbtn.Turn_Off();
          switch (static_cast<int>(input)) {
            case engine::window::ButtonKey(kButtonGdi):
              playerp = SCEN_PLAYER_SPAIN;
              gdibtn.Turn_On();
              break;

            case engine::window::ButtonKey(kButtonNod):
              playerp = SCEN_PLAYER_GREECE;
              nodbtn.Turn_On();
              break;

            case engine::window::ButtonKey(kButtonNeutral):
              playerp = SCEN_PLAYER_USSR;
              neubtn.Turn_On();
              break;

            case engine::window::ButtonKey(kButtonMplayer):
              playerp = SCEN_PLAYER_MPLAYER;
              playermbtn.Turn_On();
              break;
            default:
              break;
          }
          break;

        case KN_RETURN:
        case engine::window::ButtonKey(kButtonOk):
          cancel = false;
          process = false;
          break;

        case KN_ESC:
        case engine::window::ButtonKey(kButtonCancel):
          cancel = true;
          process = false;
          break;

        case engine::window::ButtonKey(kButtonScenario):
        default:
          break;
      }
    }

    /*
    **	Redraw the display
    */
    TheScreen().hidden_view().Clear();
    Flag_To_Redraw(true);
    Render();

    /*
    **	If cancel, just return
    */
    if (cancel) {
      return (-1);
    }

    /*
    **	Save selections & return
    */
    if (base::At(scen_buf, 0) <= '9' && base::At(scen_buf, 1) <= '9') {
      scen_nump = base::ParseIntegerOr<int>(scen_buf, 0);
    } else {
      char first = base::At(scen_buf, 0);
      char second = base::At(scen_buf, 1);
      if (first <= '9') {
        first -= '0';
      } else {
        if (first >= 'a' && first <= 'z') {
          first -= 'a';
        } else {
          first -= 'A';
        }
      }
      if (second <= '9') {
        second -= '0';
      } else {
        if (second >= 'a' && second <= 'z') {
          second = static_cast<char>((second - 'a') + 10);
        } else {
          second = static_cast<char>((second - 'A') + 10);
        }
      }
      scen_nump = (first * 36) + second;
    }

    return 0;
  }

  /***************************************************************************
   * MapEditClass::Size_Map -- lets user set size & location of map          *
   *                                                                         *
   * Lets the user select a side of the map and expand/shrink it to the      *
   * desired size, or move the whole map around the available map area.      *
   *                                                                         *
   * The entire available map area is displayed, but the map is limited such *
   * that there's always one blank cell around the map; this lets objects    *
   * properly exit the screen, since they have a blank undisplayed cell to   *
   * exit onto.                                                              *
   *                                                                         *
   *   Ŀ               *
   *                                                                       *
   *     Ŀ   Clear Terrain                           *
   *                             Water                                   *
   *                             Tiberium                                *
   *                             Rock/Wall/Road                          *
   *          (Map Area)         GDI Unit                                *
   *                             NOD Unit                                *
   *                             Neutral Unit                            *
   *                             Terrain Object                          *
   *                             Starting Cell                           *
   *                                                *
   *                                                                       *
   *         X            Y            Width      Height                   *
   *         ##           ##            ##          ##                     *
   *                                                                       *
   *                                                                       *
   *                  [OK]            [Cancel]                             *
   *                                                                       *
   *                  *
   *                                                                         *
   * INPUT:                                                                  *
   *      x,y,w,h:      initial size parameters (-1 = center the thing)      *
   *                                                                         *
   * OUTPUT:                                                                 *
   *      0 = OK, -1 = cancel                                                *
   *                                                                         *
   * WARNINGS:                                                               *
   *      none.                                                              *
   *                                                                         *
   * HISTORY:                                                                *
   *   10/21/1994 BR : Created.                                              *
   *=========================================================================*/
  int MapEditClass::Size_Map(int x, int y, int w, int h) {
    /*
    **	Dialog & button dimensions
    */
    constexpr int kDDialogW = 350;  // dialog width
    constexpr int kDDialogH = 225;  // dialog height
    constexpr int kDDialogX = 0;    // centered x-coord
    constexpr int kDDialogY = 0;    // centered y-coord
    //		D_DIALOG_CX = D_DIALOG_X + (D_DIALOG_W / 2),
    //// coord of x-center
    constexpr int kDMargin = 7;  // margin width/height
    constexpr int kDBordX1 = kDDialogX + 45;
    //		D_BORD_X1 = D_DIALOG_X + (D_DIALOG_W / 2 - MAP_CELL_W) /
    // 2,
    constexpr int kDBordY1 = kDDialogY + 25;
    constexpr int kDBordX2 = kDBordX1 + MAP_CELL_W + 1;
    constexpr int kDBordY2 = kDBordY1 + MAP_CELL_H + 1;
    constexpr int kDOkW = 45;              // OK width
    constexpr int kDOkH = 9;               // OK height
    constexpr int kDOkX = kDDialogX + 45;  // OK x
    constexpr int kDOkY =
        kDDialogY + kDDialogH - kDOkH - (kDMargin + 10);  // OK y
    constexpr int kDCancelW = 45;                         // Cancel width
    constexpr int kDCancelH = 9;                          // Cancel height
    constexpr int kDCancelX =
        kDDialogX + kDDialogW - (35 + kDCancelW);  // Cancel x
    constexpr int kDCancelY =
        kDDialogY + kDDialogH - kDCancelH - (kDMargin + 10);  // Cancel y

    /*
    **	Button enumerations:
    */
    constexpr int kButtonOk = 100;
    constexpr int kButtonCancel = 101;

    /*
    **	Redraw values: in order from "top" to "bottom" layer of the dialog
    */
    enum class RedrawType {
      REDRAW_NONE = 0,
      REDRAW_MAP = 1,  // includes map interior & coord values
      REDRAW_BACKGROUND =
          2,  // includes box, map board, key, coord labels, btns
      REDRAW_ALL = REDRAW_BACKGROUND
    };
    using enum RedrawType;

    /*
    **	Dialog variables:
    */
    bool cancel = false;    // true = user cancels
    int grabbed = 0;        // 1=TLeft,2=TRight,3=BRight,4=BLeft
    int map_x1 = 0;         // map coords x1, pixel coords
    int map_y1 = 0;         // map coords y1, pixel coords
    int delta1 = 0;
    int delta2 = 0;  // mouse-click proximity
    int mx = 0;
    int my = 0;  // last-saved mouse coords
                 //	char txt[40];
    int txt_x = 0;
    int txt_y = 0;                    // for displaying text
                                      //	unsigned index;
                                      //// for drawing map symbology
    int color = 0;                    // for drawing map symbology
    ObjectClass* occupier = nullptr;  // cell's occupier
    const RemapControlType* scheme = Get_Color_Scheme();

    /*
    **	Buttons
    */
    game::GadgetList commands;

    TextButtonClass okbtn(kButtonOk, TXT_OK, kEButtonFont, kEButtonFlags, kDOkX,
                          kDOkY, kDOkW, kDOkH);

    TextButtonClass cancelbtn(kButtonCancel, TXT_CANCEL, kEButtonFont,
                              kEButtonFlags, kDCancelX, kDCancelY, kDCancelW,
                              kDCancelH);

    PixelView& view = TheScreen().visible_view();

    /*
    **	Set up the actual map area relative to the map's border coords
    */
    if (x == -1) {
      map_x1 = kDBordX1 + ((MAP_CELL_W - w) / 2) + 1;
    } else {
      map_x1 = kDBordX1 + x + 1;
    }

    if (y == -1) {
      map_y1 = kDBordY1 + ((MAP_CELL_H - h) / 2) + 1;
    } else {
      map_y1 = kDBordY1 + y + 1;
    }

    int map_x2 = map_x1 + w - 1;  // map coords x2, pixel coords
    int map_y2 = map_y1 + h - 1;  // map coords y2, pixel coords

    /*
    **	Build the button list
    */
    commands.Add(okbtn);
    commands.Add(cancelbtn);

    /*
    **	Main processing loop
    */
    RedrawType display = REDRAW_ALL;  // requested redraw level
    bool process = true;
    while (process) {
      /*
      **	Invoke game callback
      */
      ServiceRealTime();

      /*
      **	Refresh display if needed
      */
      if (display != REDRAW_NONE) {
        game::TheMouse().Hide();

        /*
        **	Redraw the background, map border, key, and coord labels
        */
        if (display >= REDRAW_BACKGROUND) {
          /*
          **	Background
          */
          Dialog_Box(kDDialogX, kDDialogY, kDDialogW, kDDialogH);
          Draw_Caption(view, TXT_SIZE_MAP, kDDialogX, kDDialogY, kDDialogW);

          /*
          **	Draw the map border
          */
          if (view.Lock()) {
            view.DrawRect(kDBordX1, kDBordY1, kDBordX2, kDBordY2,
                          scheme->Shadow);

            /*
            **	Draw the map "key"
            */
            txt_x = kDBordX2 + 15;
            txt_y = kDBordY1;
            Plain_Text_Print(
                view, "Clear Terrain", txt_x, txt_y, GroundColor.at(LAND_CLEAR),
                game::kTransparentBlack, FontType::kEditor, TPF_DROPSHADOW);
            txt_y += 8;
            Plain_Text_Print(
                view, "Water", txt_x, txt_y, GroundColor.at(LAND_WATER),
                game::kTransparentBlack, FontType::kEditor, TPF_DROPSHADOW);
            txt_y += 8;
            Plain_Text_Print(
                view, "Tiberium", txt_x, txt_y, GroundColor.at(LAND_TIBERIUM),
                game::kTransparentBlack, FontType::kEditor, TPF_DROPSHADOW);
            txt_y += 8;
            Plain_Text_Print(view, "Rock", txt_x, txt_y,
                             GroundColor.at(LAND_ROCK), game::kTransparentBlack,
                             FontType::kEditor, TPF_DROPSHADOW);
            txt_y += 8;
            Plain_Text_Print(view, "Wall", txt_x, txt_y,
                             GroundColor.at(LAND_WALL), game::kTransparentBlack,
                             FontType::kEditor, TPF_DROPSHADOW);
            txt_y += 8;
            Plain_Text_Print(
                view, "Beach", txt_x, txt_y, GroundColor.at(LAND_BEACH),
                game::kTransparentBlack, FontType::kEditor, TPF_DROPSHADOW);
            txt_y += 8;
            Plain_Text_Print(
                view, "Rough", txt_x, txt_y, GroundColor.at(LAND_ROUGH),
                game::kTransparentBlack, FontType::kEditor, TPF_DROPSHADOW);
            txt_y += 8;
            Plain_Text_Print(
                view, "River", txt_x, txt_y, GroundColor.at(LAND_RIVER),
                game::kTransparentBlack, FontType::kEditor, TPF_DROPSHADOW);
            //					txt_y += 8;
            //					Plain_Text_Print("GDI Unit",
            // txt_x, txt_y, YELLOW, TBLACK, TPF_DROPSHADOW | TPF_EFNT);
            // txt_y += 8;
            // Plain_Text_Print("Nod Unit", txt_x, txt_y, RED, TBLACK,
            // TPF_DROPSHADOW | TPF_EFNT);
            // txt_y += 8;
            // Plain_Text_Print("Neutral Unit", txt_x, txt_y, PURPLE, TBLACK,
            // TPF_DROPSHADOW | TPF_EFNT);
            txt_y += 8;
            Plain_Text_Print(view, "Terrain Object", txt_x, txt_y, DKGREEN,
                             game::kTransparentBlack, FontType::kEditor,
                             TPF_DROPSHADOW);
            txt_y += 8;
            Plain_Text_Print(view, "Starting Cell", txt_x, txt_y, game::kWhite,
                             game::kTransparentBlack, FontType::kEditor,
                             TPF_DROPSHADOW);

            /*
            **	Draw the coordinate labels
            */
            txt_x = kDDialogX + (kDDialogW / 8);
            txt_y = kDDialogY + kDDialogH - kDOkH - 43;
            Fancy_Text_Print(view, "  X", txt_x, txt_y, Get_Color_Scheme(),
                             game::kTransparentBlack, FontType::kEditor,
                             TPF_CENTER | TPF_NOSHADOW);

            txt_x += (kDDialogW - 20) / 4;
            Fancy_Text_Print(view, "  Y", txt_x, txt_y, Get_Color_Scheme(),
                             game::kTransparentBlack, FontType::kEditor,
                             TPF_CENTER | TPF_NOSHADOW);

            txt_x += (kDDialogW - 20) / 4;
            Fancy_Text_Print(view, " Width", txt_x, txt_y, Get_Color_Scheme(),
                             game::kTransparentBlack, FontType::kEditor,
                             TPF_CENTER | TPF_NOSHADOW);

            txt_x += (kDDialogW - 20) / 4;
            Fancy_Text_Print(view, " Height", txt_x, txt_y, Get_Color_Scheme(),
                             game::kTransparentBlack, FontType::kEditor,
                             TPF_CENTER | TPF_NOSHADOW);

            view.Unlock();
          }

          /*
          **	Redraw the buttons
          */
          commands.FlagToRedraw();
        }

        /*
        **	Redraw the map symbology & location
        */
        if ((display >= REDRAW_MAP) && view.Lock()) {
          /*
          **	Erase the map interior
          */
          view.FillRect(kDBordX1 + 1, kDBordY1 + 1, kDBordX2 - 1, kDBordY2 - 1,
                        game::kBlack);

          /*
          **	Draw Land map symbols (use color according to Ground[] array).
          */
          for (int cell_index = 0; cell_index < MAP_CELL_TOTAL; ++cell_index) {
            const Cell cell = Cell::FromIndex(cell_index);
            occupier = (*this).at(cell).Cell_Occupier();
            if (occupier == nullptr) {
              color = GroundColor.at((*this).at(cell).Land_Type());
              view.PutPixel(kDBordX1 + cell.x() + 1, kDBordY1 + cell.y() + 1,
                            static_cast<unsigned char>(color));
            }
          }

          /*
          **	Draw the actual map location
          */
          view.DrawRect(map_x1, map_y1, map_x2, map_y2, game::kWhite);
          switch (grabbed) {
            case 1:
              view.DrawLine(map_x1, map_y1, map_x1 + 5, map_y1, game::kBlue);
              view.DrawLine(map_x1, map_y1, map_x1, map_y1 + 5, game::kBlue);
              break;

            case 2:
              view.DrawLine(map_x2, map_y1, map_x2 - 5, map_y1, game::kBlue);
              view.DrawLine(map_x2, map_y1, map_x2, map_y1 + 5, game::kBlue);
              break;

            case 3:
              view.DrawLine(map_x2, map_y2, map_x2 - 5, map_y2, game::kBlue);
              view.DrawLine(map_x2, map_y2, map_x2, map_y2 - 5, game::kBlue);
              break;

            case 4:
              view.DrawLine(map_x1, map_y2, map_x1 + 5, map_y2, game::kBlue);
              view.DrawLine(map_x1, map_y2, map_x1, map_y2 - 5, game::kBlue);
              break;

            case 5:
              view.DrawRect(map_x1, map_y1, map_x2, map_y2, game::kBlue);
              break;

            default:
              break;
          }

          /*
          **	Draw Unit map symbols (Use the radar map color according to
          **	that specified in the house type class object.
          **	DKGREEN = terrain object
          */
          for (int cell_index = 0; cell_index < MAP_CELL_TOTAL; ++cell_index) {
            const Cell cell = Cell::FromIndex(cell_index);
            occupier = (*this).at(cell).Cell_Occupier();
            if (occupier) {
              color = DKGREEN;
              if (occupier && occupier->Owner() != HOUSE_NONE) {
                color = ThePalettes()
                            .color_remaps()
                            .at(HouseClass::As_Pointer(occupier->Owner())
                                    ->RemapColor)
                            .Color;
              }
              view.PutPixel(kDBordX1 + cell.x() + 1, kDBordY1 + cell.y() + 1,
                            static_cast<unsigned char>(color));
            }
          }

          /*
          **	Draw Home location
          */
          view.PutPixel(
              kDBordX1 +
                  base::At(TheScenario().Waypoint, ScenarioClass::kHomeWaypoint)
                      .x() +
                  1,
              kDBordY1 +
                  base::At(TheScenario().Waypoint, ScenarioClass::kHomeWaypoint)
                      .y() +
                  1,
              game::kWhite);

          /*
          **	Draw the coordinates
          */
          txt_x = kDDialogX + (kDDialogW / 8);
          txt_y = kDDialogY + kDDialogH - kDOkH - 32;
          Fancy_Text_Print(view, "%5d", txt_x, txt_y, Get_Color_Scheme(),
                           game::kBlack, FontType::kEditor,
                           TPF_CENTER | TPF_NOSHADOW, map_x1 - kDBordX1 - 1);

          txt_x += (kDDialogW - 20) / 4;
          Fancy_Text_Print(view, "%5d", txt_x, txt_y, Get_Color_Scheme(),
                           game::kBlack, FontType::kEditor,
                           TPF_CENTER | TPF_NOSHADOW, map_y1 - kDBordY1 - 1);

          txt_x += (kDDialogW - 20) / 4;
          Fancy_Text_Print(view, "%5d", txt_x, txt_y, Get_Color_Scheme(),
                           game::kBlack, FontType::kEditor,
                           TPF_CENTER | TPF_NOSHADOW, map_x2 - map_x1 + 1);

          txt_x += (kDDialogW - 20) / 4;
          Fancy_Text_Print(view, "%5d", txt_x, txt_y, Get_Color_Scheme(),
                           game::kBlack, FontType::kEditor,
                           TPF_CENTER | TPF_NOSHADOW, map_y2 - map_y1 + 1);

          view.Unlock();
        }

        game::TheMouse().Show();
        display = REDRAW_NONE;
      }

      /*
      **	Process user input
      */
      const engine::window::InputEvent input_event = commands.Input(view);
      const engine::window::KeyNumber input = input_event.key;

      /*
      **	Normal button processing: This is done when the mouse button is
      *NOT *	being held down ('grabbed' is 0).
      */
      if (grabbed == 0) {
        // A click near a corner grabs that corner; one near the middle grabs
        // the whole map.
        if (input_event.IsPress(engine::window::MouseButton::kLeft)) {
          const int click_x = input_event.x;
          const int click_y = input_event.y;
          if (abs(click_x - map_x1) < 3 && abs(click_y - map_y1) < 3) {
            grabbed = 1;  // top left
          } else if (abs(click_x - map_x2) < 3 && abs(click_y - map_y1) < 3) {
            grabbed = 2;  // top right
          } else if (abs(click_x - map_x2) < 3 && abs(click_y - map_y2) < 3) {
            grabbed = 3;  // bottom right
          } else if (abs(click_x - map_x1) < 3 && abs(click_y - map_y2) < 3) {
            grabbed = 4;  // bottom left
          } else if (abs(click_x - ((map_x1 + map_x2) / 2)) <
                         (map_x2 - map_x1) / 4 &&
                     abs(click_y - ((map_y1 + map_y2) / 2)) <
                         (map_y2 - map_y1) / 4) {
            grabbed = 5;  // the whole map
          }
          if (grabbed != 0) {
            mx = click_x;
            my = click_y;
            display = REDRAW_MAP;
          }
        }

        switch (static_cast<int>(input)) {
          case KN_RETURN:
          case engine::window::ButtonKey(kButtonOk):
            cancel = false;
            process = false;
            break;

          case KN_ESC:
          case engine::window::ButtonKey(kButtonCancel):
            cancel = true;
            process = false;
            break;

          default:
            break;
        }
      } else {
        /*
        **	Mouse motion processing: This is done while the left mouse
        *button IS *	being held down. *	- First, check for the button
        *release; if detected, un-grab *	- Then, handle mouse motion.
        *WWLIB doesn't pass through a KN_MOUSE_MOVE *	  value while the button
        *is being held down, so this case must be *	  trapped as a default.
        */
        if (input_event.IsRelease(engine::window::MouseButton::kLeft)) {
          grabbed = 0;
          display = REDRAW_MAP;
        } else {
          delta1 = game::TheMouse().x() - mx;
          delta2 = game::TheMouse().y() - my;
          if (delta1 == 0 && delta2 == 0) {
            break;
          }

          /*
          **	Move top left
          */
          if (grabbed == 1) {
            map_x1 += delta1;
            if (map_x1 > map_x2 - 2) {
              map_x1 = map_x2 - 2;
            } else {
              map_x1 = std::max(map_x1, kDBordX1 + 2);
            }

            map_y1 += delta2;
            if (map_y1 > map_y2 - 2) {
              map_y1 = map_y2 - 2;
            } else {
              map_y1 = std::max(map_y1, kDBordY1 + 2);
            }
            display = REDRAW_MAP;
            mx = game::TheMouse().x();
            my = game::TheMouse().y();
          }

          /*
          **	Move top right
          */
          if (grabbed == 2) {
            map_x2 += delta1;
            if (map_x2 < map_x1 + 2) {
              map_x2 = map_x1 + 2;
            } else {
              map_x2 = std::min(map_x2, kDBordX2 - 2);
            }

            map_y1 += delta2;
            if (map_y1 > map_y2 - 2) {
              map_y1 = map_y2 - 2;
            } else {
              map_y1 = std::max(map_y1, kDBordY1 + 2);
            }
            display = REDRAW_MAP;
            mx = game::TheMouse().x();
            my = game::TheMouse().y();
          }

          /*
          **	Move bottom right
          */
          if (grabbed == 3) {
            map_x2 += delta1;
            if (map_x2 < map_x1 + 2) {
              map_x2 = map_x1 + 2;
            } else {
              map_x2 = std::min(map_x2, kDBordX2 - 2);
            }

            map_y2 += delta2;
            if (map_y2 < map_y1 + 2) {
              map_y2 = map_y1 + 2;
            } else {
              map_y2 = std::min(map_y2, kDBordY2 - 2);
            }
            display = REDRAW_MAP;
            mx = game::TheMouse().x();
            my = game::TheMouse().y();
          }

          /*
          **	Move bottom left
          */
          if (grabbed == 4) {
            map_x1 += delta1;
            if (map_x1 > map_x2 - 2) {
              map_x1 = map_x2 - 2;
            } else {
              map_x1 = std::max(map_x1, kDBordX1 + 2);
            }

            map_y2 += delta2;
            if (map_y2 < map_y1 + 2) {
              map_y2 = map_y1 + 2;
            } else {
              map_y2 = std::min(map_y2, kDBordY2 - 2);
            }
            display = REDRAW_MAP;
            mx = game::TheMouse().x();
            my = game::TheMouse().y();
          }

          /*
          **	Move whole map
          */
          if (grabbed == 5) {
            if (map_x1 + delta1 > kDBordX1 + 1 &&
                map_x2 + delta1 < kDBordX2 - 1) {
              map_x1 += delta1;
              map_x2 += delta1;
            }

            if (map_y1 + delta2 > kDBordY1 + 1 &&
                map_y2 + delta2 < kDBordY2 - 1) {
              map_y1 += delta2;
              map_y2 += delta2;
            }
            display = REDRAW_MAP;
            mx = game::TheMouse().x();
            my = game::TheMouse().y();
          }
        }
      }
    }

    /*
    **	Redraw the display
    */
    TheScreen().hidden_view().Clear();
    Flag_To_Redraw(true);
    Render();

    /*
    **	If cancel, just return
    */
    if (cancel) {
      return (-1);
    }

    /*
    **	Save selections
    */
    MapCellX = map_x1 - kDBordX1 - 1;
    MapCellY = map_y1 - kDBordY1 - 1;
    MapCellWidth = map_x2 - map_x1 + 1;
    MapCellHeight = map_y2 - map_y1 + 1;

    /*
    **	Clip Home Cell to new map size
    */
    if (base::At(TheScenario().Waypoint, ScenarioClass::kHomeWaypoint).x() <
        MapCellX) {
      base::At(TheScenario().Waypoint, ScenarioClass::kHomeWaypoint) =
          Cell::FromXY(MapCellX, base::At(TheScenario().Waypoint,
                                          ScenarioClass::kHomeWaypoint)
                                     .y());
    }

    if (base::At(TheScenario().Waypoint, ScenarioClass::kHomeWaypoint).x() >
        MapCellX + MapCellWidth - 1) {
      base::At(TheScenario().Waypoint, ScenarioClass::kHomeWaypoint) =
          Cell::FromXY(
              MapCellX + MapCellWidth - 1,
              base::At(TheScenario().Waypoint, ScenarioClass::kHomeWaypoint)
                  .y());
    }

    if (base::At(TheScenario().Waypoint, ScenarioClass::kHomeWaypoint).y() <
        MapCellY) {
      base::At(TheScenario().Waypoint, ScenarioClass::kHomeWaypoint) =
          Cell::FromXY(
              base::At(TheScenario().Waypoint, ScenarioClass::kHomeWaypoint)
                  .x(),
              MapCellY);
    }

    if (base::At(TheScenario().Waypoint, ScenarioClass::kHomeWaypoint).y() >
        MapCellY + MapCellHeight - 1) {
      base::At(TheScenario().Waypoint, ScenarioClass::kHomeWaypoint) =
          Cell::FromXY(
              base::At(TheScenario().Waypoint, ScenarioClass::kHomeWaypoint)
                  .x(),
              MapCellY + MapCellHeight - 1);
    }

    return 0;
  }

  /***************************************************************************
   * MapEditClass::Scenario_Dialog -- scenario global parameters dialog      *
   *                                                                         *
   *    Edits the house specific and general scenario options.               *
   *                                                                         *
   *                                                                         *
   * INPUT:                                                                  *
   *      none.                                                              *
   *                                                                         *
   * OUTPUT:                                                                 *
   *      0 = OK, -1 = cancel                                                *
   *                                                                         *
   * WARNINGS:                                                               *
   *      Uses HIDBUFF.                                                      *
   *                                                                         *
   * HISTORY:                                                                *
   *   11/14/1994 BR : Created.                                              *
   *   02/13/1996 JLB : Revamped to new system.                              *
   *=========================================================================*/
  int MapEditClass::Scenario_Dialog() {
    const TheaterType orig_theater = TheScenario().Theater;  // original theater
    HousesType house = ThePlayer()->Class->House;
    HousesType newhouse = house;
    base::EnumArray<HousesType, HouseStaticClass> hdata;

    /*
    **	Fill in the house data for each house that exists.
    */
    for (const HousesType h : magic_enum::enum_values<HousesType>()) {
      const HouseClass* hptr = HouseClass::As_Pointer(h);
      if (hptr) {
        hdata.at(h) = hptr->Control;
      }
    }

    /*
    **	Dialog & button dimensions
    */
    constexpr int kDDialogW = 640;
    constexpr int kDDialogH = 400;
    constexpr int kDDialogX = ((640 - kDDialogW) / 2);
    constexpr int kDDialogY = ((400 - kDDialogH) / 2);
    constexpr int kDOkW = 45;
    constexpr int kDOkH = 9;
    constexpr int kDOkX = kDDialogX + 30;
    constexpr int kDOkY = kDDialogY + kDDialogH - 30;
    constexpr int kDCancelW = 45;
    constexpr int kDCancelH = 9;
    constexpr int kDCancelX = kDDialogX + kDDialogW - (kDCancelW + 30);
    constexpr int kDCancelY = kDDialogY + kDDialogH - 30;

    /*
    **	Button enumerations:
    */
    constexpr int kListTheater = 100;
    constexpr int kButtonDescription = 101;
    constexpr int kButtonAllies = 102;
    constexpr int kButtonControl = 103;
    constexpr int kButtonSmarties = 104;
    constexpr int kButtonBase = 105;
    constexpr int kButtonNospyplane = 106;
    constexpr int kButtonInherit = 107;
    constexpr int kButtonTimer = 108;
    constexpr int kButtonTheme = 109;
    constexpr int kButtonRecord = 110;
    constexpr int kButtonEvac = 111;
    constexpr int kButtonMoneytib = 112;
    constexpr int kButtonTech = 113;
    constexpr int kButtonTruckcrate = 114;
    constexpr int kButtonEndofgame = 115;
    constexpr int kButtonSkipscore = 116;
    constexpr int kButtonOnetime = 117;
    constexpr int kButtonNomapsel = 118;
    constexpr int kButtonHouse = 119;
    constexpr int kButtonCredits = 120;
    constexpr int kButtonSource = 121;
    constexpr int kButtonMaxunit = 122;
    constexpr int kButtonIntro = 123;
    constexpr int kButtonBriefing = 124;
    constexpr int kButtonAction = 125;
    constexpr int kButtonWin = 126;
    constexpr int kButtonLose = 127;
    constexpr int kButtonOk = 128;
    constexpr int kButtonCancel = 129;

    PixelView& view = TheScreen().visible_view();

    game::GadgetList commands;  // the button list

    /*
    **	Theater choice drop down list.
    */
    char theatertext[45] = "";
    DropListClass theaterbtn(kListTheater, theatertext, sizeof(theatertext) - 1,
                             FontType::kEditor, TPF_NOSHADOW, kDDialogX + 30,
                             kDDialogY + 30, 65, 8 * 5,
                             MixArchive::RetrieveData("EBTN-UP.SHP"),
                             MixArchive::RetrieveData("EBTN-DN.SHP"));
    for (const TheaterType t : magic_enum::enum_values<TheaterType>()) {
      theaterbtn.Add_Item(Theaters.at(t).Name);
    }
    theaterbtn.Set_Selected_Index(static_cast<int>(orig_theater));

    char description[kDescripMax] = "";
    base::SafeCopy(description, TheScenario().Description);
    EditClass desc(kButtonDescription, description, sizeof(description),
                   FontType::kEditor, TPF_NOSHADOW,
                   theaterbtn.X + theaterbtn.Width + 15, theaterbtn.Y, 160);

    /*
    **	Button that tells if this scenario should inherit buildings from the
    *previous.
    */
    CheckBoxClass inherit(kButtonInherit,
                          theaterbtn.X + theaterbtn.Width + 15 + 250,
                          theaterbtn.Y);
    if (TheScenario().IsToInherit) {
      inherit.Turn_On();
    } else {
      inherit.Turn_Off();
    }

    /*
    **	Records scenario disposition into holding slot.
    */
    CheckBoxClass record(kButtonRecord, inherit.X, inherit.Y + 8);
    if (TheScenario().IsToCarryOver) {
      record.Turn_On();
    } else {
      record.Turn_Off();
    }

    /*
    **	Should Tanya/civilian be automatically evacuated?
    */
    CheckBoxClass tanya(kButtonEvac, record.X, record.Y + 8);
    if (TheScenario().IsTanyaEvac) {
      tanya.Turn_On();
    } else {
      tanya.Turn_Off();
    }

    /*
    **	End of game with with scenario?
    */
    CheckBoxClass endofgame(kButtonEndofgame, tanya.X, tanya.Y + 8);
    if (TheScenario().IsEndOfGame) {
      endofgame.Turn_On();
    } else {
      endofgame.Turn_Off();
    }

    /*
    **	Timer inherit logic.
    */
    CheckBoxClass timercarry(kButtonTimer, endofgame.X, endofgame.Y + 8);
    if (TheScenario().IsInheritTimer) {
      timercarry.Turn_On();
    } else {
      timercarry.Turn_Off();
    }

    /*
    **	Disable spy plane option?
    */
    CheckBoxClass nospyplane(kButtonNospyplane, timercarry.X, timercarry.Y + 8);
    if (TheScenario().IsNoSpyPlane) {
      nospyplane.Turn_On();
    } else {
      nospyplane.Turn_Off();
    }

    /*
    **	Skip the score screen?
    */
    CheckBoxClass skipscore(kButtonSkipscore, nospyplane.X, nospyplane.Y + 8);
    if (TheScenario().IsSkipScore) {
      skipscore.Turn_On();
    } else {
      skipscore.Turn_Off();
    }

    /*
    **	Skip the map selection screen for next mission. Presume goes to
    **	variation "B"?
    */
    CheckBoxClass nomapsel(kButtonNomapsel, skipscore.X, skipscore.Y + 8);
    if (TheScenario().IsNoMapSel) {
      nomapsel.Turn_On();
    } else {
      nomapsel.Turn_Off();
    }

    /*
    **	Return to main menu after mission completes?
    */
    CheckBoxClass onetime(kButtonOnetime, nomapsel.X, nomapsel.Y + 8);
    if (TheScenario().IsOneTimeOnly) {
      onetime.Turn_On();
    } else {
      onetime.Turn_Off();
    }

    /*
    **	Trucks carry a wood crate?
    */
    CheckBoxClass truckcrate(kButtonTruckcrate, onetime.X, onetime.Y + 8);
    if (TheScenario().IsTruckCrate) {
      truckcrate.Turn_On();
    } else {
      truckcrate.Turn_Off();
    }

    /*
    **	Transfer credits into tiberium storage at scenario start?
    */
    CheckBoxClass moneytib(kButtonMoneytib, truckcrate.X, truckcrate.Y + 8);
    if (TheScenario().IsMoneyTiberium) {
      moneytib.Turn_On();
    } else {
      moneytib.Turn_Off();
    }

    /*
    **	Intro movie name.
    */
    char introtext[engine::platform::kMaxFname + engine::platform::kMaxExt];
    DropListClass intro(kButtonIntro, introtext, sizeof(introtext),
                        FontType::kEditor, TPF_NOSHADOW, theaterbtn.X,
                        theaterbtn.Y + theaterbtn.Height + 24, 50, 7 * 10,
                        MixArchive::RetrieveData("EBTN-UP.SHP"),
                        MixArchive::RetrieveData("EBTN-DN.SHP"));
    intro.Add_Item("<none>");
    for (const VQType v : magic_enum::enum_values<VQType>()) {
      intro.Add_Item(VQName.at(v));
    }
    intro.Set_Selected_Index(static_cast<int>(TheScenario().IntroMovie) + 1);

    /*
    **	Briefing movie name.
    */
    char brieftext[engine::platform::kMaxFname + engine::platform::kMaxExt];
    DropListClass briefing(kButtonBriefing, brieftext, sizeof(brieftext),
                           FontType::kEditor, TPF_NOSHADOW,
                           intro.X + intro.Width + 10, intro.Y, 50, 7 * 10,
                           MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));
    briefing.Add_Item("<none>");
    for (const VQType v : magic_enum::enum_values<VQType>()) {
      briefing.Add_Item(VQName.at(v));
    }
    briefing.Set_Selected_Index(static_cast<int>(TheScenario().BriefMovie) + 1);

    char actiontext[engine::platform::kMaxFname + engine::platform::kMaxExt];
    DropListClass action(kButtonAction, actiontext, sizeof(actiontext),
                         FontType::kEditor, TPF_NOSHADOW,
                         briefing.X + briefing.Width + 10, briefing.Y, 50,
                         7 * 10, MixArchive::RetrieveData("EBTN-UP.SHP"),
                         MixArchive::RetrieveData("EBTN-DN.SHP"));
    action.Add_Item("<none>");
    for (const VQType v : magic_enum::enum_values<VQType>()) {
      action.Add_Item(VQName.at(v));
    }
    action.Set_Selected_Index(static_cast<int>(TheScenario().ActionMovie) + 1);

    char wintext[engine::platform::kMaxFname + engine::platform::kMaxExt];
    DropListClass win(kButtonWin, wintext, sizeof(wintext), FontType::kEditor,
                      TPF_NOSHADOW, action.X + action.Width + 10, action.Y, 50,
                      7 * 10, MixArchive::RetrieveData("EBTN-UP.SHP"),
                      MixArchive::RetrieveData("EBTN-DN.SHP"));
    win.Add_Item("<none>");
    for (const VQType v : magic_enum::enum_values<VQType>()) {
      win.Add_Item(VQName.at(v));
    }
    win.Set_Selected_Index(static_cast<int>(TheScenario().WinMovie) + 1);

    char losetext[engine::platform::kMaxFname + engine::platform::kMaxExt];
    DropListClass lose(kButtonLose, losetext, sizeof(losetext),
                       FontType::kEditor, TPF_NOSHADOW, win.X + win.Width + 10,
                       win.Y, 50, 7 * 10,
                       MixArchive::RetrieveData("EBTN-UP.SHP"),
                       MixArchive::RetrieveData("EBTN-DN.SHP"));
    lose.Add_Item("<none>");
    for (const VQType v : magic_enum::enum_values<VQType>()) {
      lose.Add_Item(VQName.at(v));
    }
    lose.Set_Selected_Index(static_cast<int>(TheScenario().LoseMovie) + 1);

    /*
    **	House choice list.
    */
    ListClass housebtn(kButtonHouse, kDDialogX + 30, kDDialogY + 105, 55,
                       7 * 10, FontType::kEditor, TPF_NOSHADOW,
                       MixArchive::RetrieveData("EBTN-UP.SHP"),
                       MixArchive::RetrieveData("EBTN-DN.SHP"));
    for (const HousesType h : magic_enum::enum_values<HousesType>()) {
      housebtn.Add_Item(HouseTypeClass::As_Reference(h).IniName);
    }
    housebtn.Set_Selected_Index(static_cast<int>(ThePlayer()->Class->House));

    /*
    **	Base house choice drop down list.
    */
    char basetext[35];
    DropListClass basebtn(kButtonBase, basetext, sizeof(basetext),
                          FontType::kEditor, TPF_NOSHADOW, kDDialogX + 30,
                          kDDialogY + 80, 65, 7 * 10,
                          MixArchive::RetrieveData("EBTN-UP.SHP"),
                          MixArchive::RetrieveData("EBTN-DN.SHP"));
    for (const HousesType h : magic_enum::enum_values<HousesType>()) {
      basebtn.Add_Item(HouseTypeClass::As_Reference(h).IniName);
    }
    if (TheWorld().base().house() != HOUSE_NONE) {
      basebtn.Set_Selected_Index(static_cast<int>(TheWorld().base().house()));
    }

    /*
    **	Opening scenario theme.
    */
    char themetext[65];
    DropListClass themebtn(kButtonTheme, themetext, sizeof(themetext),
                           FontType::kEditor, TPF_NOSHADOW,
                           basebtn.X + basebtn.Width + 30, basebtn.Y, 85,
                           7 * 10, MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));
    themebtn.Add_Item("<none>");
    for (const ThemeType th : magic_enum::enum_values<ThemeType>()) {
      themebtn.Add_Item(ThemeClass::Full_Name(th));
    }
    if (TheScenario().TransitTheme != THEME_NONE) {
      themebtn.Set_Selected_Index(static_cast<int>(TheScenario().TransitTheme) +
                                  1);
    } else {
      themebtn.Set_Selected_Index(0);
    }

    /*
    **	Build level (technology).
    */
    SliderClass techlevel(kButtonTech, housebtn.X + housebtn.Width + 15,
                          housebtn.Y, 100, 8, game::SliderStyle::kGauge);
    techlevel.Set_Maximum(16);

    char statictechbuff[15];
    StaticButtonClass techstatic(0, "999", FontType::kEditor, TPF_NOSHADOW,
                                 techlevel.X + techlevel.Width - 20,
                                 techlevel.Y - 7);

    /*
    **	Starting credits.
    */
    SliderClass creditbtn(kButtonCredits, housebtn.X + housebtn.Width + 15,
                          techlevel.Y + 20, 100, 8, game::SliderStyle::kGauge);
    creditbtn.Set_Maximum(201);

    char staticcreditbuff[15];
    StaticButtonClass creditstatic(
        0, "999999999", FontType::kEditor, TPF_NOSHADOW,
        creditbtn.X + creditbtn.Width - 50, creditbtn.Y - 7);

    /*
    **	Maximum unit/infantry slider.
    */
    SliderClass maxunit(kButtonMaxunit, housebtn.X + housebtn.Width + 15,
                        creditbtn.Y + 20, 100, 8, game::SliderStyle::kGauge);
    maxunit.Set_Maximum(501);

    char staticmaxunitbuff[15];
    StaticButtonClass maxunitstatic(
        0, "999999", FontType::kEditor, TPF_NOSHADOW,
        maxunit.X + maxunit.Width - 30, maxunit.Y - 7);

    /*
    **	Source of ground delivery reinforcements.
    */
    ListClass sourcebtn(kButtonSource, housebtn.X + housebtn.Width + 15,
                        maxunit.Y + 20, 100, 7 * 4, FontType::kEditor,
                        TPF_NOSHADOW, MixArchive::RetrieveData("EBTN-UP.SHP"),
                        MixArchive::RetrieveData("EBTN-DN.SHP"));
    for (const SourceType source : magic_enum::enum_values<SourceType>()) {
      if (source > SOURCE_WEST) {
        break;
      }
      sourcebtn.Add_Item(SourceName.at(source));
    }

    /*
    **	Smartness lider.
    */
    SliderClass smarties(kButtonSmarties, sourcebtn.X,
                         sourcebtn.Y + sourcebtn.Height + 15, 35, 8,
                         game::SliderStyle::kGauge);
    smarties.Set_Maximum(TheRules().MaxIQ + 1);

    char staticsmartiesbuff[15];
    StaticButtonClass smartiesstatic(0, "9999", FontType::kEditor, TPF_NOSHADOW,
                                     smarties.X + smarties.Width - 20,
                                     smarties.Y - 7);

    /*
    **	List box of who is allied with whom.
    */
    CheckListClass allies(kButtonAllies, techlevel.X + techlevel.Width + 5,
                          housebtn.Y, 65, 7 * 10, FontType::kEditor,
                          TPF_NOSHADOW, MixArchive::RetrieveData("EBTN-UP.SHP"),
                          MixArchive::RetrieveData("EBTN-DN.SHP"));
    for (const HousesType h : magic_enum::enum_values<HousesType>()) {
      allies.Add_Item(HouseTypeClass::As_Reference(h).IniName);
      if (hdata.at(house).Allies & base::Bit<uint32_t>(h)) {
        allies.Check_Item(static_cast<int>(h), true);
      }
    }
    allies.Set_Selected_Index(0);

    /*
    **	List box of who the player can control.
    */
    CheckListClass control(kButtonControl, allies.X + allies.Width + 10,
                           housebtn.Y, 65, 7 * 10, FontType::kEditor,
                           TPF_NOSHADOW,
                           MixArchive::RetrieveData("EBTN-UP.SHP"),
                           MixArchive::RetrieveData("EBTN-DN.SHP"));
    for (const HousesType h : magic_enum::enum_values<HousesType>()) {
      control.Add_Item(HouseTypeClass::As_Reference(h).IniName);
      if (HouseClass::As_Pointer(h)->IsPlayerControl) {
        control.Check_Item(static_cast<int>(h), true);
      }
    }
    control.Set_Selected_Index(0);

    /*
    **	Create the ubiquitous "ok" and "cancel" buttons.
    */
    TextButtonClass okbtn(kButtonOk, TXT_OK, kEButtonFont, kEButtonFlags, kDOkX,
                          kDOkY, kDOkW, kDOkH);
    TextButtonClass cancelbtn(kButtonCancel, TXT_CANCEL, kEButtonFont,
                              kEButtonFlags, kDCancelX, kDCancelY, kDCancelW,
                              kDCancelH);

    /*
    **	Create the list
    */
    commands.Add(okbtn);
    commands.Add(cancelbtn);
    commands.Add(theaterbtn);
    commands.Add(themebtn);
    commands.Add(housebtn);
    commands.Add(techlevel);
    commands.Add(techstatic);
    commands.Add(sourcebtn);
    commands.Add(creditbtn);
    commands.Add(creditstatic);
    commands.Add(maxunitstatic);
    commands.Add(moneytib);
    commands.Add(smartiesstatic);
    commands.Add(allies);
    commands.Add(control);
    commands.Add(maxunit);
    commands.Add(nospyplane);
    commands.Add(skipscore);
    commands.Add(nomapsel);
    commands.Add(onetime);
    commands.Add(inherit);
    commands.Add(timercarry);
    commands.Add(tanya);
    commands.Add(record);
    commands.Add(truckcrate);
    commands.Add(endofgame);
    commands.Add(briefing);
    commands.Add(intro);
    commands.Add(action);
    commands.Add(win);
    commands.Add(lose);
    commands.Add(basebtn);
    commands.Add(smarties);
    commands.Add(desc);

    /*
    **	Main Processing Loop
    */
    bool housechange = true;
    bool display = true;
    bool process = true;
    bool cancel = false;  // true = user cancels
    bool dotext = true;   // display the text.
    bool fetch = false;   // Fetch data from dialog into tracking structure.
    while (process) {
      /*
      **	Invoke game callback
      */
      ServiceRealTime();

      /*
      **	If the house changes, then all the gadgets that reflect the
      *settings of the *	house should change as well.
      */
      if (housechange) {
        const HouseStaticClass* hstatic = &hdata.at(newhouse);
        creditbtn.Set_Value(
            static_cast<int>(hstatic->InitialCredits / 100));
        techlevel.Set_Value(hstatic->TechLevel);
        sourcebtn.Set_Selected_Index(static_cast<int>(hstatic->Edge));
        maxunit.Set_Value(hstatic->MaxUnit + hstatic->MaxInfantry);
        for (const HousesType h : magic_enum::enum_values<HousesType>()) {
          allies.Check_Item(static_cast<int>(h),
                            (hstatic->Allies & base::Bit<uint32_t>(h)) != 0);
        }
        smarties.Set_Value(hstatic->IQ);

        house = newhouse;
        housechange = false;
        display = true;
      }

      /*
      **	Refresh display if needed
      */
      if (display) {
        game::TheMouse().Hide();

        /*
        **	Draw the background
        */
        Dialog_Box(kDDialogX, kDDialogY, kDDialogW, kDDialogH);
        Draw_Caption(view, TXT_SCENARIO_OPTIONS, kDDialogX, kDDialogY,
                     kDDialogW);

        /*
        **	Display the text that doesn't need drawing except when the
        *entire dialog *	needs to be redrawn.
        */
        Fancy_Text_Print(view, "Tech Level =", techlevel.X, techlevel.Y - 7,
                         Get_Color_Scheme(), game::kTransparentBlack,
                         FontType::kEditor, TPF_NOSHADOW);
        Fancy_Text_Print(view, "Credits =", creditbtn.X, creditbtn.Y - 7,
                         Get_Color_Scheme(), game::kTransparentBlack,
                         FontType::kEditor, TPF_NOSHADOW);
        Fancy_Text_Print(view, "Unit Max =", maxunit.X, maxunit.Y - 7,
                         Get_Color_Scheme(), game::kTransparentBlack,
                         FontType::kEditor, TPF_NOSHADOW);
        Fancy_Text_Print(view, "IQ =", smarties.X, smarties.Y - 7,
                         Get_Color_Scheme(), game::kTransparentBlack,
                         FontType::kEditor, TPF_NOSHADOW);
        Fancy_Text_Print(view, "Prebuild Base:", basebtn.X, basebtn.Y - 7,
                         Get_Color_Scheme(), game::kTransparentBlack,
                         FontType::kEditor, TPF_NOSHADOW);
        Fancy_Text_Print(view, "Theater:", theaterbtn.X, theaterbtn.Y - 7,
                         Get_Color_Scheme(), game::kTransparentBlack,
                         FontType::kEditor, TPF_NOSHADOW);
        Fancy_Text_Print(view, "Scenario Name:", desc.X, desc.Y - 7,
                         Get_Color_Scheme(), game::kTransparentBlack,
                         FontType::kEditor, TPF_NOSHADOW);
        Fancy_Text_Print(view, "Country:", housebtn.X, housebtn.Y - 7,
                         Get_Color_Scheme(), game::kTransparentBlack,
                         FontType::kEditor, TPF_NOSHADOW);
        Fancy_Text_Print(view, "Home Edge:", sourcebtn.X, sourcebtn.Y - 7,
                         Get_Color_Scheme(), game::kTransparentBlack,
                         FontType::kEditor, TPF_NOSHADOW);
        Fancy_Text_Print(view, "Allies:", allies.X, allies.Y - 7,
                         Get_Color_Scheme(), game::kTransparentBlack,
                         FontType::kEditor, TPF_NOSHADOW);
        Fancy_Text_Print(view, "Plyr Control:", control.X, control.Y - 7,
                         Get_Color_Scheme(), game::kTransparentBlack,
                         FontType::kEditor, TPF_NOSHADOW);
        Fancy_Text_Print(view, "Theme:", themebtn.X, themebtn.Y - 7,
                         Get_Color_Scheme(), game::kTransparentBlack,
                         FontType::kEditor, TPF_NOSHADOW);
        Fancy_Text_Print(view, "Intro:", intro.X, intro.Y - 7,
                         Get_Color_Scheme(), game::kTransparentBlack,
                         FontType::kEditor, TPF_NOSHADOW);
        Fancy_Text_Print(view, "Briefing:", briefing.X, briefing.Y - 7,
                         Get_Color_Scheme(), game::kTransparentBlack,
                         FontType::kEditor, TPF_NOSHADOW);
        Fancy_Text_Print(view, "Action:", action.X, action.Y - 7,
                         Get_Color_Scheme(), game::kTransparentBlack,
                         FontType::kEditor, TPF_NOSHADOW);
        Fancy_Text_Print(view, "Win:", win.X, win.Y - 7, Get_Color_Scheme(),
                         game::kTransparentBlack, FontType::kEditor,
                         TPF_NOSHADOW);
        Fancy_Text_Print(view, "Lose:", lose.X, lose.Y - 7, Get_Color_Scheme(),
                         game::kTransparentBlack, FontType::kEditor,
                         TPF_NOSHADOW);
        Fancy_Text_Print(view, "Store scenario?", record.X + 10, record.Y,
                         Get_Color_Scheme(), game::kTransparentBlack,
                         FontType::kEditor, TPF_NOSHADOW);
        Fancy_Text_Print(view, "Inherit stored scenario?", inherit.X + 10,
                         inherit.Y, Get_Color_Scheme(), game::kTransparentBlack,
                         FontType::kEditor, TPF_NOSHADOW);
        Fancy_Text_Print(view, "Auto evac. Tanya (civilian)?", tanya.X + 10,
                         tanya.Y, Get_Color_Scheme(), game::kTransparentBlack,
                         FontType::kEditor, TPF_NOSHADOW);
        Fancy_Text_Print(view, "Last mission of game?", endofgame.X + 10,
                         endofgame.Y, Get_Color_Scheme(),
                         game::kTransparentBlack, FontType::kEditor,
                         TPF_NOSHADOW);
        Fancy_Text_Print(view, "Inherit mission timer from last scenario?",
                         timercarry.X + 10, timercarry.Y, Get_Color_Scheme(),
                         game::kTransparentBlack, FontType::kEditor,
                         TPF_NOSHADOW);
        Fancy_Text_Print(view, "Disable spy plane?", nospyplane.X + 10,
                         nospyplane.Y, Get_Color_Scheme(),
                         game::kTransparentBlack, FontType::kEditor,
                         TPF_NOSHADOW);
        Fancy_Text_Print(view, "Skip the score screen?", skipscore.X + 10,
                         skipscore.Y, Get_Color_Scheme(),
                         game::kTransparentBlack, FontType::kEditor,
                         TPF_NOSHADOW);
        Fancy_Text_Print(view, "No map selection (force var 'B')?",
                         nomapsel.X + 10, nomapsel.Y, Get_Color_Scheme(),
                         game::kTransparentBlack, FontType::kEditor,
                         TPF_NOSHADOW);
        Fancy_Text_Print(view, "Return to main menu after scenario finishes?",
                         onetime.X + 10, onetime.Y, Get_Color_Scheme(),
                         game::kTransparentBlack, FontType::kEditor,
                         TPF_NOSHADOW);
        Fancy_Text_Print(view, "Truck carries wood crate?", truckcrate.X + 10,
                         truckcrate.Y, Get_Color_Scheme(),
                         game::kTransparentBlack, FontType::kEditor,
                         TPF_NOSHADOW);
        Fancy_Text_Print(view, "Initial money is transferred to silos?",
                         moneytib.X + 10, moneytib.Y, Get_Color_Scheme(),
                         game::kTransparentBlack, FontType::kEditor,
                         TPF_NOSHADOW);

        theaterbtn.Collapse();
        themebtn.Collapse();
        intro.Collapse();
        briefing.Collapse();
        action.Collapse();
        win.Collapse();
        lose.Collapse();
        basebtn.Collapse();
        commands.DrawAll(view, true);
        game::TheMouse().Show();
        display = false;
        dotext = true;
      }

      /*
      **	Display the text of the buttons that could change their text as
      *a *	result of slider interaction.
      */
      if (dotext) {
        dotext = false;
        game::TheMouse().Hide();

        absl::SNPrintF(statictechbuff, sizeof(statictechbuff), "%2d",
                       techlevel.Get_Value());
        techstatic.Set_Text(statictechbuff);
        techstatic.Draw_Me(view);

        absl::SNPrintF(staticcreditbuff, sizeof(staticcreditbuff), "$%-7d",
                       creditbtn.Get_Value() * 100);
        creditstatic.Set_Text(staticcreditbuff);
        creditstatic.Draw_Me(view);

        absl::SNPrintF(staticmaxunitbuff, sizeof(staticmaxunitbuff), "%4d",
                       maxunit.Get_Value());
        maxunitstatic.Set_Text(staticmaxunitbuff);
        maxunitstatic.Draw_Me(view);

        absl::SNPrintF(staticsmartiesbuff, sizeof(staticsmartiesbuff), "%2d",
                       smarties.Get_Value());
        smartiesstatic.Set_Text(staticsmartiesbuff);
        smartiesstatic.Draw_Me(view);

        game::TheMouse().Show();
      }

      /*
      **	Get user input
      */
      const engine::window::KeyNumber input = commands.Input(view).key;

      /*
      **	Process input
      */
      switch (static_cast<int>(input)) {
        case engine::window::ButtonKey(kButtonAllies):
          allies.Check_Item(static_cast<int>(house), true);
          break;

        case engine::window::ButtonKey(kButtonControl):
          control.Check_Item(static_cast<int>(house), true);
          break;

        case engine::window::ButtonKey(kButtonTheme):
        case engine::window::ButtonKey(kButtonIntro):
        case engine::window::ButtonKey(kButtonBriefing):
        case engine::window::ButtonKey(kButtonAction):
        case engine::window::ButtonKey(kButtonWin):
        case engine::window::ButtonKey(kButtonLose):
        case engine::window::ButtonKey(kButtonBase):
        case engine::window::ButtonKey(kListTheater):
          briefing.Collapse();
          action.Collapse();
          win.Collapse();
          themebtn.Collapse();
          intro.Collapse();
          lose.Collapse();
          basebtn.Collapse();
          theaterbtn.Collapse();
          display = true;
          break;

        case engine::window::ButtonKey(kButtonSmarties):
        case engine::window::ButtonKey(kButtonMaxunit):
        case engine::window::ButtonKey(kButtonCredits):
        case engine::window::ButtonKey(kButtonTech):
          briefing.Collapse();
          action.Collapse();
          win.Collapse();
          lose.Collapse();
          basebtn.Collapse();
          themebtn.Collapse();
          theaterbtn.Collapse();
          dotext = true;
          break;

        case engine::window::ButtonKey(kButtonHouse):
          newhouse = HousesType(housebtn.Current_Index());
          housechange = true;
          briefing.Collapse();
          action.Collapse();
          themebtn.Collapse();
          win.Collapse();
          intro.Collapse();
          lose.Collapse();
          basebtn.Collapse();
          theaterbtn.Collapse();
          fetch = true;
          break;

        case KN_RETURN:
        case engine::window::ButtonKey(kButtonOk):
          cancel = false;
          process = false;
          fetch = true;
          break;

        case KN_ESC:
        case engine::window::ButtonKey(kButtonCancel):
          cancel = true;
          process = false;
          break;

        default:
          break;
      }

      /*
      **	If the house changes, then all the gadgets that reflect the
      *settings of the *	house should change as well.
      */
      if (fetch) {
        fetch = false;
        HouseStaticClass* hstatic = &hdata.at(house);

        TheWorld().base().set_house(HousesType(basebtn.Current_Index()));
        hstatic->InitialCredits =
            static_cast<int64_t>(creditbtn.Get_Value()) * 100;
        hstatic->Edge = SourceType(sourcebtn.Current_Index());
        hstatic->TechLevel = techlevel.Get_Value();
        hstatic->MaxUnit = maxunit.Get_Value() / 2;
        hstatic->MaxInfantry = maxunit.Get_Value() / 2;
        hstatic->IQ = smarties.Get_Value();
        for (const HousesType h : magic_enum::enum_values<HousesType>()) {
          if (allies.Is_Checked(static_cast<int>(h))) {
            hstatic->Allies |= base::Bit<uint32_t>(h);
          } else {
            hstatic->Allies &= ~base::Bit<uint32_t>(h);
          }
        }
      }
    }

    /*
    **	Redraw the map
    */
    TheScreen().hidden_view().Clear();
    Flag_To_Redraw(true);
    Render();

    /*
    **	If cancel, just return
    */
    if (cancel) {
      return (-1);
    }

    /*
    **	Copy the dialog data back into the appropriate game data locations.
    */
    for (const HousesType h : magic_enum::enum_values<HousesType>()) {
      HouseClass* hptr = HouseClass::As_Pointer(h);
      if (hptr != nullptr) {
        hptr->Control = hdata.at(h);
        hptr->Allies = static_cast<unsigned>(hdata.at(h).Allies);
        hptr->IsPlayerControl = control.Is_Checked(static_cast<int>(h));
      }
    }
    ThePlayer()->IsPlayerControl = true;
    base::SafeCopy(TheScenario().Description, desc.Get_Text());
    base::At(TheScenario().Description, sizeof(TheScenario().Description) - 1) =
        '\0';
    TheScenario().IntroMovie = VQType(intro.Current_Index() - 1);
    TheScenario().BriefMovie = VQType(briefing.Current_Index() - 1);
    TheScenario().ActionMovie = VQType(action.Current_Index() - 1);
    TheScenario().WinMovie = VQType(win.Current_Index() - 1);
    TheScenario().LoseMovie = VQType(lose.Current_Index() - 1);
    TheScenario().IsToInherit = inherit.IsOn;
    TheScenario().IsToCarryOver = record.IsOn;
    TheScenario().IsTanyaEvac = tanya.IsOn;
    TheScenario().IsEndOfGame = endofgame.IsOn;
    TheScenario().IsInheritTimer = timercarry.IsOn;
    TheScenario().IsNoSpyPlane = nospyplane.IsOn;
    TheScenario().IsSkipScore = skipscore.IsOn;
    TheScenario().IsNoMapSel = nomapsel.IsOn;
    TheScenario().IsOneTimeOnly = onetime.IsOn;
    TheScenario().IsTruckCrate = truckcrate.IsOn;
    TheScenario().IsMoneyTiberium = moneytib.IsOn;
    TheScenario().TransitTheme = ThemeType(themebtn.Current_Index() - 1);

    /*
    **	Change the theater:
    **	- 1st set the Theater global
    **	- scan all cells to check their TType for compatibility with the new
    **	  theater; if not compatible, set TType to TEMPLATE_NONE & TIcon to 0
    **	- Then, re-initialize the TypeClasses for the new Theater
    */
    const auto theater = TheaterType(theaterbtn.Current_Index());
    if (theater != orig_theater) {
      uint32_t theater_mask = 0;  // template/terrain mask

      /*
      **	Loop through all cells
      */
      for (int cell_index = 0; cell_index < MAP_CELL_TOTAL; ++cell_index) {
        const Cell cell = Cell::FromIndex(cell_index);
        /*
        **	If this cell has a template icon & that template isn't
        *compatible *	with this theater, set the icon to NONE
        */
        if ((*this).at(cell).TType != TEMPLATE_NONE) {
          theater_mask =
              TemplateTypeClass::As_Reference((*this).at(cell).TType).Theater;
          if ((theater_mask & base::Bit<uint32_t>(theater)) == 0) {
            (*this).at(cell).TType = TEMPLATE_NONE;
            (*this).at(cell).TIcon = 0;
          }
        }

        /*
        **	If this cell has terrain in it, and that terrain isn't
        *compatible *	with this theater, delete the terrain object.
        */
        TerrainClass* terrain =
            (*this).at(cell).Cell_Terrain();  // cell's terrain pointer
        if (terrain != nullptr) {
          theater_mask = terrain->Class->Theater;
          if ((theater_mask & base::Bit<uint32_t>(theater)) == 0) {
            base::DeletePooled(terrain);
          }
        }
      }

      /*
      ** Force shapes to reload
      */
      TheWorld().last_theater() = THEATER_NONE;

      /*
      **	Re-init the object Type Classes for this theater
      */
      Init_Theater(theater);
      TerrainTypeClass::Init(theater);
      TemplateTypeClass::Init(theater);
      OverlayTypeClass::Init(theater);
      UnitTypeClass::Init(theater);
      InfantryTypeClass::Init(theater);
      BuildingTypeClass::Init(theater);
      BulletTypeClass::Init(theater);
      AnimTypeClass::Init(theater);
      AircraftTypeClass::Init(theater);
      VesselTypeClass::Init(theater);
      SmudgeTypeClass::Init(theater);

      //		LastTheater = theater;
    }

    return 0;
  }

  /***************************************************************************
   * Handle_Triggers -- processes the trigger dialogs                        *
   *                                                                         *
   * INPUT:                                                                  *
   *      none.                                                              *
   *                                                                         *
   * OUTPUT:                                                                 *
   *      none.                                                              *
   *                                                                         *
   * WARNINGS:                                                               *
   *      none.                                                              *
   *                                                                         *
   * HISTORY:                                                                *
   *   11/29/1994 BR : Created.                                              *
   *=========================================================================*/
  void MapEditClass::Handle_Triggers() {

    /*
    **	Trigger dialog processing loop:
    **	- Invoke the trigger selection dialog. If a trigger's selected, break
    **	  & return
    **	- If user wants to edit the current trigger, do so
    **	- If user wants to create new trigger, new a TriggerClass & edit it
    **	- If user wants to delete trigger, delete the current trigger
    **	- Keep looping until 'OK'
    */
    while (true) {
      /*
      **	Select trigger
      */
      const int rc = Select_Trigger();

      /*
      **	'OK'; break
      */
      if (rc == 0) {
        break;
      }

      /*
      **	'Edit'
      */
      if (rc == 1 && CurTrigger) {
        if (CurTrigger->Edit()) {
          Changed = true;
        }
        TheScreen().hidden_view().Clear();
        Flag_To_Redraw(true);
        Render();
      }

      /*
      **	'New'
      */
      if (rc == 2) {
        /*
        **	Create a new trigger
        */
        CurTrigger = base::NewPooled<TriggerType>();
        if (CurTrigger) {
          /*
          **	delete it if user cancels
          */
          if (!CurTrigger->Edit()) {
            base::DeletePooled(CurTrigger);
            CurTrigger = nullptr;
          } else {
            Changed = true;
          }
          TheScreen().hidden_view().Clear();
          Flag_To_Redraw(true);
          Render();

        } else {
          /*
          **	Unable to create; issue warning
          */
          WWMessageBox().Process("No more triggers available.");
          TheScreen().hidden_view().Clear();
          Flag_To_Redraw(true);
          Render();
        }
      }

      /*
      **	'Delete'
      */
      if ((rc == 3) && CurTrigger) {
        Detach_This_From_All(CurTrigger->As_Target(), true);
        base::DeletePooled(CurTrigger);
        // CurTrigger->Remove();
        CurTrigger = nullptr;
        Changed = true;
      }
    }

    /*
    **	Let the CurTrigger global exist if the trigger can be placed on the
    **	ground or on a game object.
    */
    if (CurTrigger &&
        !base::Any(CurTrigger->AttachesTo() & (ATTACH_OBJECT | ATTACH_CELL))) {
      CurTrigger = nullptr;
    }
  }

  /***************************************************************************
   * MapEditClass::Select_Trigger -- lets user select a trigger              *
   *                                                                         *
   * CurTrigger can be NULL when this function is called.                    *
   *                                                                         *
   *    Ŀ           *
   *                           Triggers                                    *
   *        Ŀ               *
   *         Name     Event     Action    House   Team                 *
   *         Name     Event     Action    House   Team  Ĵ               *
   *         Name     Event     Action    House   Team                  *
   *         Name     Event     Action    House   Team                  *
   *                                                                    *
   *                                                                    *
   *                                                    Ĵ               *
   *                                                                   *
   *                       *
   *                                                                       *
   *          [Edit]        [New]        [Delete]      [OK]                *
   *                                                                       *
   *               *
   *                                                                         *
   * INPUT:                                                                  *
   *      none.                                                              *
   *                                                                         *
   * OUTPUT:                                                                 *
   *      0 = OK, 1 = Edit, 2 = New, 3 = Delete                              *
   *                                                                         *
   * WARNINGS:                                                               *
   *      Uses HIDBUFF.                                                      *
   *                                                                         *
   * HISTORY:                                                                *
   *   11/29/1994 BR : Created.                                              *
   *   05/07/1996 JLB : Streamlined and sort trigger list.                   *
   *=========================================================================*/
  int MapEditClass::Select_Trigger() {
    /*
    **	Dialog & button dimensions
    */
    constexpr int kDDialogW = 400;
    constexpr int kDDialogH = 250;
    constexpr int kDDialogX = 0;
    constexpr int kDDialogY = 0;
    constexpr int kDMargin = 35;
    constexpr int kDListW = (kDDialogW - (kDMargin * 2)) - 10;
    constexpr int kDListH = kDDialogH - 70;
    constexpr int kDListX = kDDialogX + ((kDDialogW - kDListW) / 2);
    constexpr int kDListY = kDDialogY + 25;
    constexpr int kButtonW = 45;
    constexpr int kButtonH = 9;
    constexpr int kDEditW = kButtonW;
    constexpr int kDEditH = kButtonH;
    constexpr int kDEditX = kDDialogX + kDDialogW - (((kDEditW + 10) * 4) + 25);
    constexpr int kDEditY = kDDialogY + kDDialogH - 20 - kDEditH;
    constexpr int kDNewW = kButtonW;
    constexpr int kDNewH = kButtonH;
    constexpr int kDNewX = kDEditX + kDEditW + 10;
    constexpr int kDNewY = kDDialogY + kDDialogH - 20 - kDNewH;
    constexpr int kDDeleteW = kButtonW;
    constexpr int kDDeleteH = kButtonH;
    constexpr int kDDeleteX = kDNewX + kDNewW + 10;
    constexpr int kDDeleteY = kDDialogY + kDDialogH - 20 - kDDeleteH;
    constexpr int kDOkW = kButtonW;
    constexpr int kDOkH = kButtonH;
    constexpr int kDOkX = kDDeleteX + kDDeleteW + 10;
    constexpr int kDOkY = kDDialogY + kDDialogH - 20 - kDOkH;

    /*
    **	Button enumerations:
    */
    constexpr int kTriggerList = 100;
    constexpr int kButtonEdit = 101;
    constexpr int kButtonNew = 102;
    constexpr int kButtonDelete = 103;
    constexpr int kButtonOk = 104;

    /*
    **	Dialog variables:
    */
    bool edit_trig = false;  // true = user wants to edit
    bool new_trig = false;   // true = user wants to new
    bool del_trig = false;   // true = user wants to new

    /*
    **	Buttons
    */
    game::GadgetList commands;  // the button list

    TListClass<TriggerType*> triggerlist(
        kTriggerList, kDListX, kDListY, kDListW, kDListH, FontType::kEditor,
        TPF_NOSHADOW, MixArchive::RetrieveData("EBTN-UP.SHP"),
        MixArchive::RetrieveData("EBTN-DN.SHP"));

    TextButtonClass editbtn(kButtonEdit, "Edit", kEButtonFont, kEButtonFlags,
                            kDEditX, kDEditY, kDEditW, kDEditH);
    TextButtonClass newbtn(kButtonNew, "New", kEButtonFont, kEButtonFlags,
                           kDNewX, kDNewY, kDNewW, kDNewH);
    TextButtonClass deletebtn(kButtonDelete, "Delete", kEButtonFont,
                              kEButtonFlags, kDDeleteX, kDDeleteY, kDDeleteW,
                              kDDeleteH);
    TextButtonClass okbtn(kButtonOk, TXT_OK, kEButtonFont, kEButtonFlags, kDOkX,
                          kDOkY, kDOkW, kDOkH);

    PixelView& view = TheScreen().visible_view();

    /*
    **	Fill in the list box
    */
    for (int i = 0; i < TheObjectHeaps().trigger_type().Count(); i++) {
      triggerlist.Add_Item(TheObjectHeaps().trigger_type().ActiveAt(i));
    }

    triggerlist.Sort(
        [](const TriggerType* left, const TriggerType* right) {
          return std::ranges::lexicographical_compare(
              std::string_view(left->Name()), std::string_view(right->Name()),
              {}, absl::ascii_tolower, absl::ascii_tolower);
        });

    if (CurTrigger) {
      triggerlist.Set_Selected_Index(CurTrigger);
    } else {
      triggerlist.Set_Selected_Index(0);
    }

    /*
    **	Set CurTrigger if it isn't
    */
    if (TheObjectHeaps().trigger_type().Count() == 0) {
      CurTrigger = nullptr;
    } else {
      CurTrigger = triggerlist.Current_Item();
    }

    /*
    **	Create the list
    */
    commands.Add(triggerlist);
    commands.Add(editbtn);
    commands.Add(newbtn);
    commands.Add(deletebtn);
    commands.Add(okbtn);

    /*
    **	Main Processing Loop
    */
    bool display = true;
    bool process = true;
    while (process) {
      /*
      **	Invoke game callback
      */
      ServiceRealTime();

      /*
      **	Refresh display if requested.
      */
      if (display) {
        game::TheMouse().Hide();
        Dialog_Box(kDDialogX, kDDialogY, kDDialogW, kDDialogH);
        Draw_Caption(view, TXT_TRIGGER_EDITOR, kDDialogX, kDDialogY, kDDialogW);
        commands.FlagToRedraw();
        commands.DrawAll(view);
        game::TheMouse().Show();
        display = false;
      }

      /*
      **	Get user input
      */
      const engine::window::KeyNumber input = commands.Input(view).key;

      /*
      **	Process input
      */
      switch (static_cast<int>(input)) {
        case engine::window::ButtonKey(kTriggerList):
          CurTrigger = triggerlist.Current_Item();
          break;

        case engine::window::ButtonKey(kButtonEdit):
          if (CurTrigger) {  // only allow if there's one selected
            process = false;
            edit_trig = true;
          }
          break;

        case engine::window::ButtonKey(kButtonNew):
          process = false;
          new_trig = true;
          break;

        case engine::window::ButtonKey(kButtonDelete):
          process = false;
          del_trig = true;
          break;

        case KN_RETURN:
        case engine::window::ButtonKey(kButtonOk):
          process = false;
          break;
        default:
          break;
      }
    }

    /*
    **	Redraw the display
    */
    TheScreen().hidden_view().Clear();
    Flag_To_Redraw(true);
    Render();

    if (edit_trig) {
      return 1;
    }
    if (new_trig) {
      return 2;
    }
    if (del_trig) {
      return 3;
    }
    return 0;
  }

  }  // namespace ra
