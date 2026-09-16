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

/* $Header:   F:\projects\c&c\vcs\code\mapeddlg.cpv   2.18   16 Oct 1995
 * 16:49:00   JOE_BOSTIC  $ */
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
 *                  Last Update : December 12, 1994   [BR]                 *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Map Editor dialogs & main menu options                                  *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   MapEditClass::New_Scenario -- creates a new scenario                  *
 *   MapEditClass::Load_Scenario -- loads a scenario INI file              *
 *   MapEditClass::Save_Scenario -- saves current scenario to an INI file  *
 *   MapEditClass::Pick_Scenario -- dialog for choosing scenario           *
 *   MapEditClass::Size_Map -- lets user set size & location of map        *
 *   MapEditClass::Scenario_Dialog -- scenario global parameters dialog    *
 *   MapEditClass::Handle_Triggers -- processes the trigger dialogs        *
 *   MapEditClass::Select_Trigger -- lets user select a trigger            *
 *   MapEditClass::Edit_Trigger -- lets user edit a [new] trigger          *
 *   MapEditClass::Import_Triggers -- lets user import triggers            *
 *   MapEditClass::Import_Teams -- lets user import teams                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "absl/strings/str_format.h"
#include "base/numeric.h"
#include "port/safe_string.h"
#include "port/tokenizer.h"
#include "sdllib/drawbuff.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "td/base.h"
#include "td/cell.h"
#include "td/cheklist.h"
#include "td/conquer.h"
#include "td/const.h"
#include "td/control.h"
#include "td/defines.h"
#include "td/dialog.h"
#include "td/display_constants.h"
#include "td/edit.h"
#include "td/externs.h"
#include "td/globals.h"
#include "td/goptions.h"
#include "td/heap.h"
#include "td/house.h"
#include "td/ini.h"
#include "td/inline.h"
#include "td/jshell.h"
#include "td/list.h"
#include "td/mapedit.h"
#include "td/mplayer.h"
#include "td/msgbox.h"
#include "td/object.h"
#include "td/palette.h"
#include "td/profile.h"
#include "td/scenario.h"
#include "td/teamtype.h"
#include "td/terrain.h"
#include "td/textbtn.h"
#include "td/trigger.h"
#include "td/type.h"
#include "td/vector.h"
#include "tech/game_file.h"
#include "tech/number_parse.h"

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
  int scen_num = Scenario;
  ScenarioPlayerType player = ScenPlayer;
  ScenarioDirType dir = ScenDir;
  ScenarioVarType var = ScenVar;

  /*
  ------------------------ Prompt for scenario info ------------------------
  */
  const int rc =
      Pick_Scenario("New Scenario", &scen_num, &player, &dir, &var, 1);
  if (rc != 0) {
    return (-1);
  }

  /*
  -------------------------- Blow away everything --------------------------
  */
  Clear_Scenario();

  /*
  ----------------------------- Set parameters -----------------------------
  */
  Scenario = scen_num;
  ScenPlayer = player;
  ScenDir = dir;
  ScenVar = var;
  Set_Scenario_Name(ScenarioName, scen_num, player, dir, var);

  /*
  ----------------------------- Create houses ------------------------------
  */
  for (HousesType house = HOUSE_FIRST; house < HOUSE_COUNT; house++) {
    new HouseClass(house);
  }

  if (ScenPlayer == SCEN_PLAYER_MPLAYER) {
    PlayerPtr = HouseClass::As_Pointer(HOUSE_MULTI1);
    PlayerPtr->IsHuman = true;
    LastHouse = HOUSE_MULTI1;
  } else {
    if (player == SCEN_PLAYER_GDI) {
      PlayerPtr = HouseClass::As_Pointer(HOUSE_GOOD);
      PlayerPtr->IsHuman = true;
      Base.House = HOUSE_BAD;
    } else {
      if (player == SCEN_PLAYER_NOD) {
        PlayerPtr = HouseClass::As_Pointer(HOUSE_BAD);
        PlayerPtr->IsHuman = true;
        Base.House = HOUSE_GOOD;
      } else {
        PlayerPtr = HouseClass::As_Pointer(HOUSE_MULTI4);
        PlayerPtr->IsHuman = true;
        Base.House = HOUSE_MULTI4;
      }
    }
    LastHouse = HOUSE_GOOD;
  }

  /*
  -------------------------- Init the entire map ---------------------------
  */
  Init_Clear();
  Fill_In_Data();

  /*
  -------------------------- Prompt for map size ---------------------------
  */
  Size_Map(-1, -1, 20, 20);

  /*
  ------ Set the Home & Reinforcement Cells to the center of the map -------
  */
  Waypoint[kWayptReinf] =
      XY_Cell(MapCellX + (MapCellWidth / 2), MapCellY + (MapCellHeight / 2));
  Waypoint[kWayptHome] =
      XY_Cell(MapCellX + (MapCellWidth / 2), MapCellY + (MapCellHeight / 2));
  (*this)[Coord_Cell(TacticalCoord)].IsWaypoint = true;
  Flag_Cell(Coord_Cell(TacticalCoord));

  ScenarioInit++;
  Set_Tactical_Position(Cell_Coord(Waypoint[kWayptHome]));
  ScenarioInit--;

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
  int scen_num = Scenario;
  ScenarioPlayerType player = ScenPlayer;
  ScenarioDirType dir = ScenDir;
  ScenarioVarType var = ScenVar;

  /*
  ------------------------ Prompt for scenario info ------------------------
  */
  const int rc =
      Pick_Scenario("Load Scenario", &scen_num, &player, &dir, &var, 1);
  if (rc != 0) {
    return (-1);
  }

  /*
  ----------------------------- Set parameters -----------------------------
  */
  Scenario = scen_num;
  ScenPlayer = player;
  ScenDir = dir;
  ScenVar = var;
  Set_Scenario_Name(ScenarioName, scen_num, player, dir, var);

  /*------------------------------------------------------------------------
  Read_Scenario_Ini() must be able to set PlayerPtr to the right house:
  - Reading the INI will create the house objects
  - PlayerPtr must be set before any Techno objects are created
  - For GDI or NOD scenarios, PlayerPtr is set by reading the INI;
    but for multiplayer, it's set via the MPlayerLocalID; so, here we have
    to set various multiplayer variables to fool the Assign_Houses() routine
    into working properly.
  ------------------------------------------------------------------------*/
  if (ScenPlayer == SCEN_PLAYER_MPLAYER) {
    MPlayerLocalID = static_cast<unsigned char>(Build_MPlayerID(2, HOUSE_GOOD));
    MPlayerCount = 1;
    LastHouse = HOUSE_MULTI1;
  } else if (ScenPlayer == SCEN_PLAYER_JP) {
    PlayerPtr = HouseClass::As_Pointer(HOUSE_MULTI4);
    PlayerPtr->IsHuman = true;
    Base.House = HOUSE_MULTI4;
  } else {
    LastHouse = HOUSE_GOOD;
  }

  /*
  -------------------------- Blow away everything --------------------------
  */
  Clear_Scenario();

  /*
  ------------------------------ Read the INI ------------------------------
  */
  if (!Read_Scenario_Ini(ScenarioName)) {
    CCMessageBox().Process("Unable to read scenario!");
    HiddenPage.Clear();
    Flag_To_Redraw(true);
    Render();
  } else {
    Fill_In_Data();
    Set_Palette(GamePalette);
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
  int scen_num = Scenario;
  ScenarioPlayerType player = ScenPlayer;
  ScenarioDirType dir = ScenDir;
  ScenarioVarType var = ScenVar;
  char fname[13];

  /*
  ------------------------ Prompt for scenario info ------------------------
  */
  int rc = Pick_Scenario("Save Scenario", &scen_num, &player, &dir, &var, 0);
  if (rc != 0) {
    return (-1);
  }

  /*
  ------------------- Warning if scenario already exists -------------------
  */
  Set_Scenario_Name(fname, scen_num, player, dir, var);
  FILE* fp = fopen(fname, "rb");
  if (fp) {
    fclose(fp);
    rc = CCMessageBox().Process("File exists. Replace?", TXT_YES, TXT_NO);
    HiddenPage.Clear();
    Flag_To_Redraw(true);
    Render();
    if (rc == 1) {
      return (-1);
    }
  }

  /*
  ----------------------------- Set parameters -----------------------------
  */
  Scenario = scen_num;
  ScenPlayer = player;
  ScenDir = dir;
  ScenVar = var;
  Set_Scenario_Name(ScenarioName, scen_num, player, dir, var);

  /*------------------------------------------------------------------------
  Player may have changed from GDI to NOD, so change playerptr accordingly
  ------------------------------------------------------------------------*/
  if (ScenPlayer == SCEN_PLAYER_GDI || ScenPlayer == SCEN_PLAYER_NOD) {
    if (ScenPlayer == SCEN_PLAYER_GDI) {
      PlayerPtr = HouseClass::As_Pointer(HOUSE_GOOD);
      PlayerPtr->IsHuman = true;
      Base.House = HOUSE_BAD;
    } else {
      if (ScenPlayer == SCEN_PLAYER_NOD) {
        PlayerPtr = HouseClass::As_Pointer(HOUSE_BAD);
        PlayerPtr->IsHuman = true;
        Base.House = HOUSE_GOOD;
      } else {
        PlayerPtr = HouseClass::As_Pointer(HOUSE_MULTI4);
        PlayerPtr->IsHuman = true;
        Base.House = HOUSE_MULTI4;
      }
    }
    LastHouse = HOUSE_GOOD;
  }

  /*
  ----------------------------- Write the INI ------------------------------
  */
  Write_Scenario_Ini(ScenarioName);

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
 *=========================================================================*/
int MapEditClass::Pick_Scenario(const char* caption, int* scen_nump,
                                ScenarioPlayerType* playerp,
                                ScenarioDirType* dirp, ScenarioVarType* varp,
                                int multi) {
  /*........................................................................
  Dialog & button dimensions
  ........................................................................*/
  constexpr int kDialogW = 400;                         // dialog width
  constexpr int kDialogH = 328;                         // dialog height
  constexpr int kDialogX = ((640 - kDialogW) / 2);      // centered x-coord
  constexpr int kDialogY = ((400 - kDialogH) / 2);      // centered y-coord
  constexpr int kDialogCx = kDialogX + (kDialogW / 2);  // coord of x-center
  constexpr int kTxt8H = 22;                            // ht of 8-pt text
  constexpr int kMargin = 14;                           // margin width/height
  constexpr int kScenW = 90;                            // Scenario # width
  constexpr int kScenH = 18;                            // Scenario # height
  constexpr int kScenX = kDialogCx + 5;                 // Scenario # x
  constexpr int kScenY = kDialogY + kMargin + kTxt8H + kMargin;  // Scenario # y
  constexpr int kVaraW = 26;                              // Version A width
  constexpr int kVaraH = 18;                              // Version A height
  constexpr int kVaraX = kDialogCx - ((kVaraW * 5) / 2);  // Version A x
  constexpr int kVaraY = kScenY + kScenH + kMargin;       // Version A y
  constexpr int kVarbW = 26;                              // Version B width
  constexpr int kVarbH = 18;                              // Version B height
  constexpr int kVarbX = kVaraX + kVaraW;                 // Version B x
  constexpr int kVarbY = kScenY + kScenH + kMargin;       // Version B y
  constexpr int kVarcW = 26;                              // Version C width
  constexpr int kVarcH = 18;                              // Version C height
  constexpr int kVarcX = kVarbX + kVarbW;                 // Version C x
  constexpr int kVarcY = kScenY + kScenH + kMargin;       // Version C y
  constexpr int kVardW = 26;                              // Version D width
  constexpr int kVardH = 18;                              // Version D height
  constexpr int kVardX = kVarcX + kVarcW;                 // Version D x
  constexpr int kVardY = kScenY + kScenH + kMargin;       // Version D y
  constexpr int kVarloseW = 26;                           // Version Lose width
  constexpr int kVarloseH = 18;                           // Version Lose height
  constexpr int kVarloseX = kVardX + kVardW;              // Version Lose x
  constexpr int kVarloseY = kScenY + kScenH + kMargin;    // Version Lose y
  constexpr int kEastW = 100;                             // EAST width
  constexpr int kEastH = 18;                              // EAST height
  constexpr int kEastX = kDialogCx - kEastW - 5;          // EAST x
  constexpr int kEastY = kVarloseY + kVarloseH + kMargin;  // EAST y
  constexpr int kWestW = 100;                              // WEST width
  constexpr int kWestH = 18;                               // WEST height
  constexpr int kWestX = kDialogCx + 5;                    // WEST x
  constexpr int kWestY = kVarloseY + kVarloseH + kMargin;  // EAST y
  constexpr int kGdiW = 140;                               // GDI width
  constexpr int kGdiH = 18;                                // GDI height
  constexpr int kGdiX = kDialogCx - (kGdiW / 2);           // GDI x
  constexpr int kGdiY = kEastY + kEastH + kMargin;         // GDI y
  constexpr int kNodW = 140;                               // NOD width
  constexpr int kNodH = 18;                                // NOD height
  constexpr int kNodX = kDialogCx - (kNodW / 2);           // NOD x
  constexpr int kNodY = kGdiY + kGdiH;                     // NOD y
  constexpr int kNeuH = 18;                                // Neutral height
  constexpr int kNeuY = kNodY + kNodH;                     // Neutral y
  constexpr int kMplayerW = 140;                           // Multi-Player width
  constexpr int kMplayerH = 18;                           // Multi-Player height
  constexpr int kMplayerX = kDialogCx - (kMplayerW / 2);  // Multi-Player x
  constexpr int kMplayerY = kNeuY + kNeuH;                // Multi-Player y
  constexpr int kOkW = 90;                                // OK width
  constexpr int kOkH = 18;                                // OK height
  constexpr int kOkX = kDialogCx - kOkW - 5;              // OK x
  constexpr int kOkY = kDialogY + kDialogH - kOkH - kMargin;  // OK y
  constexpr int kCancelW = 90;                                // Cancel width
  constexpr int kCancelH = 18;                                // Cancel height
  constexpr int kCancelX = kDialogCx + 5;                     // Cancel x
  constexpr int kCancelY =
      kDialogY + kDialogH - kCancelH - kMargin;  // Cancel y
  /*........................................................................
  Button enumerations
  ........................................................................*/
  constexpr int kButtonGdi = 100;
  constexpr int kButtonNod = 101;
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
  constexpr int kButtonVarL = 113;
  /*........................................................................
  Redraw values: in order from "top" to "bottom" layer of the dialog
  ........................................................................*/
  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_BUTTONS = 1,
    REDRAW_BACKGROUND = 2,
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;
  /*........................................................................
  Dialog variables
  ........................................................................*/
  bool cancel = false;  // true = user cancels
  /*........................................................................
  Other Variables
  ........................................................................*/
  char scen_buf[10] = {0};  // buffer for editing scenario #
  /*........................................................................
  Buttons
  ........................................................................*/
  ControlClass* commands = nullptr;  // the button list

  EditClass editbtn(kButtonScenario, scen_buf, 5,
                    TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kScenX,
                    kScenY, kScenW, kScenH, EditClass::NUMERIC);

  TextButtonClass varabtn(
      kButtonVarA, "A",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kVaraX,
      kVaraY, kVaraW, kVaraH);

  TextButtonClass varbbtn(
      kButtonVarB, "B",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kVarbX,
      kVarbY, kVarbW, kVarbH);

  TextButtonClass varcbtn(
      kButtonVarC, "C",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kVarcX,
      kVarcY, kVarcW, kVarcH);

  TextButtonClass vardbtn(
      kButtonVarD, "D",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kVardX,
      kVardY, kVardW, kVardH);

  TextButtonClass varlbtn(
      kButtonVarL, "L",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kVarloseX,
      kVarloseY, kVarloseW, kVarloseH);

  TextButtonClass gdibtn(
      kButtonGdi, "GDI",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kGdiX, kGdiY,
      kGdiW, kGdiH);

  TextButtonClass nodbtn(
      kButtonNod, "NOD",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kNodX, kNodY,
      kNodW, kNodH);

  TextButtonClass playermbtn(
      kButtonMplayer, "Multi Player",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kMplayerX,
      kMplayerY, kMplayerW, kMplayerH);

  TextButtonClass eastbtn(
      kButtonEast, "East",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kEastX,
      kEastY, kEastW, kEastH);

  TextButtonClass westbtn(
      kButtonWest, "West",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kWestX,
      kWestY, kWestW, kWestH);

  TextButtonClass okbtn(
      kButtonOk, TXT_OK,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kOkX, kOkY,
      kOkW, kOkH);

  TextButtonClass cancelbtn(
      kButtonCancel, TXT_CANCEL,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kCancelX,
      kCancelY, kCancelW, kCancelH);

  /*
  ------------------------------- Initialize -------------------------------
  */
  Set_Logic_Page(SeenBuff);

  absl::SNPrintF(scen_buf, sizeof(scen_buf), "%d",
                 (*scen_nump));  // init edit buffer
  editbtn.Set_Text(scen_buf, 5);

  varabtn.Turn_Off();
  varbbtn.Turn_Off();
  varcbtn.Turn_Off();
  vardbtn.Turn_Off();
  varlbtn.Turn_Off();
  switch (*varp) {
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

    case SCEN_VAR_LOSE:
      varlbtn.Turn_On();
      break;
    default:
      break;
  }

  /*
  ......................... Create the button list .........................
  */
  commands = &editbtn;
  varabtn.Add_Tail(*commands);
  varbbtn.Add_Tail(*commands);
  varcbtn.Add_Tail(*commands);
  vardbtn.Add_Tail(*commands);
  varlbtn.Add_Tail(*commands);
  if (multi) {
    gdibtn.Add_Tail(*commands);
    nodbtn.Add_Tail(*commands);
    playermbtn.Add_Tail(*commands);
  } else {
    if ((*playerp) == SCEN_PLAYER_MPLAYER) {
      playermbtn.Add_Tail(*commands);
    } else {
      gdibtn.Add_Tail(*commands);
      nodbtn.Add_Tail(*commands);
    }
  }
  eastbtn.Add_Tail(*commands);
  westbtn.Add_Tail(*commands);
  okbtn.Add_Tail(*commands);
  cancelbtn.Add_Tail(*commands);
  /*
  ......................... Init the button states .........................
  */
  if ((*playerp) == SCEN_PLAYER_GDI) {
    gdibtn.Turn_On();
    nodbtn.Turn_Off();
    playermbtn.Turn_Off();
  } else {
    if ((*playerp) == SCEN_PLAYER_NOD) {
      gdibtn.Turn_Off();
      nodbtn.Turn_On();
      playermbtn.Turn_Off();
    } else {
      gdibtn.Turn_Off();
      nodbtn.Turn_Off();
      playermbtn.Turn_On();
    }
  }

  if ((*dirp) == SCEN_DIR_EAST) {
    eastbtn.Turn_On();
    westbtn.Turn_Off();
  } else {
    eastbtn.Turn_Off();
    westbtn.Turn_On();
  }

  /*
  -------------------------- Main Processing Loop --------------------------
  */
  RedrawType display = REDRAW_ALL;  // requested redraw level
  bool process = true;              // loop while true
  while (process) {
    /*
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      display = REDRAW_ALL;
    }

    /*
    ........................ Invoke game callback .........................
    */
    Call_Back();

    /*
    ...................... Refresh display if needed ......................
    */
    if (display != REDRAW_NONE) {
      /*
      ...................... Display the dialog box ......................
      */
      Hide_Mouse();
      if (display >= REDRAW_BACKGROUND) {
        Dialog_Box(kDialogX, kDialogY, kDialogW, kDialogH);
        Draw_Caption(TXT_NONE, kDialogX, kDialogY, kDialogW);

        /*
        ....................... Draw the captions .......................
        */
        Fancy_Text_Print(
            caption, kDialogCx, kDialogY + kMargin, kCcGreen, kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        Fancy_Text_Print(
            "Scenario", kDialogCx - 5, kScenY, kCcGreen, kTBlack,
            TPF_RIGHT | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
      }

      /*
      ........................ Redraw the buttons ........................
      */
      if (display >= REDRAW_BUTTONS) {
        commands->Draw_All();
      }
      Show_Mouse();
      display = REDRAW_NONE;
    }

    /*
    ........................... Get user input ............................
    */
    const KeyNumType input = commands->Input();

    /*
    ............................ Process input ............................
    */
    switch (static_cast<int>(input)) {
      case ButtonKey(kButtonVarA):
        (*varp) = SCEN_VAR_A;
        varabtn.Turn_On();
        varbbtn.Turn_Off();
        varcbtn.Turn_Off();
        vardbtn.Turn_Off();
        varlbtn.Turn_Off();
        break;

      case ButtonKey(kButtonVarB):
        (*varp) = SCEN_VAR_B;
        varabtn.Turn_Off();
        varbbtn.Turn_On();
        varcbtn.Turn_Off();
        vardbtn.Turn_Off();
        varlbtn.Turn_Off();
        break;

      case ButtonKey(kButtonVarC):
        (*varp) = SCEN_VAR_C;
        varabtn.Turn_Off();
        varbbtn.Turn_Off();
        varcbtn.Turn_On();
        vardbtn.Turn_Off();
        varlbtn.Turn_Off();
        break;

      case ButtonKey(kButtonVarD):
        (*varp) = SCEN_VAR_D;
        varabtn.Turn_Off();
        varbbtn.Turn_Off();
        varcbtn.Turn_Off();
        vardbtn.Turn_On();
        varlbtn.Turn_Off();
        break;

      case ButtonKey(kButtonVarL):
        (*varp) = SCEN_VAR_LOSE;
        varabtn.Turn_Off();
        varbbtn.Turn_Off();
        varcbtn.Turn_Off();
        vardbtn.Turn_Off();
        varlbtn.Turn_On();
        break;

      case ButtonKey(kButtonEast):
        (*dirp) = SCEN_DIR_EAST;
        eastbtn.Turn_On();
        westbtn.Turn_Off();
        break;

      case ButtonKey(kButtonWest):
        (*dirp) = SCEN_DIR_WEST;
        eastbtn.Turn_Off();
        westbtn.Turn_On();
        break;

      case ButtonKey(kButtonGdi):
        (*playerp) = SCEN_PLAYER_GDI;
        gdibtn.Turn_On();
        nodbtn.Turn_Off();
        playermbtn.Turn_Off();
        break;

      case ButtonKey(kButtonNod):
        (*playerp) = SCEN_PLAYER_NOD;
        gdibtn.Turn_Off();
        nodbtn.Turn_On();
        playermbtn.Turn_Off();
        break;

      case ButtonKey(kButtonMplayer):
        (*playerp) = SCEN_PLAYER_MPLAYER;
        gdibtn.Turn_Off();
        nodbtn.Turn_Off();
        playermbtn.Turn_On();
        break;

      case KN_RETURN:
      case ButtonKey(kButtonOk):
        cancel = false;
        process = false;
        break;

      case KN_ESC:
      case ButtonKey(kButtonCancel):
        cancel = true;
        process = false;
        break;

      case ButtonKey(kButtonScenario):
      default:
        break;
    }
  }

  /*
  --------------------------- Redraw the display ---------------------------
  */
  HiddenPage.Clear();
  Flag_To_Redraw(true);
  Render();

  /*
  ------------------------- If cancel, just return -------------------------
  */
  if (cancel) {
    return (-1);
  }

  /*
  ------------------------ Save selections & return ------------------------
  */
  (*scen_nump) = tech::ParseInteger<int>(scen_buf).value_or(0);

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
  /*........................................................................
  Dialog & button dimensions
  ........................................................................*/
  constexpr int kDialogW = 480;                         // dialog width
  constexpr int kDialogH = 280;                         // dialog height
  constexpr int kDialogX = ((640 - kDialogW) / 2);      // centered x-coord
  constexpr int kDialogY = ((400 - kDialogH) / 2);      // centered y-coord
  constexpr int kDialogCx = kDialogX + (kDialogW / 2);  // coord of x-center
  constexpr int kMargin = 14;                           // margin width/height
  constexpr int kBordX1 = kDialogX + (((kDialogW / 2) - MAP_CELL_W) / 2);
  constexpr int kBordY1 = kDialogY + 10;
  constexpr int kBordX2 = kBordX1 + MAP_CELL_W + 1;
  constexpr int kBordY2 = kBordY1 + MAP_CELL_H + 1;
  constexpr int kOkW = 90;                                    // OK width
  constexpr int kOkH = 18;                                    // OK height
  constexpr int kOkX = kDialogCx - kOkW - 5;                  // OK x
  constexpr int kOkY = kDialogY + kDialogH - kOkH - kMargin;  // OK y
  constexpr int kCancelW = 90;                                // Cancel width
  constexpr int kCancelH = 18;                                // Cancel height
  constexpr int kCancelX = kDialogCx + 5;                     // Cancel x
  constexpr int kCancelY =
      kDialogY + kDialogH - kCancelH - kMargin;  // Cancel y
  /*........................................................................
  Button enumerations:
  ........................................................................*/
  constexpr int kButtonOk = 100;
  constexpr int kButtonCancel = 101;
  /*........................................................................
  Redraw values: in order from "top" to "bottom" layer of the dialog
  ........................................................................*/
  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_MAP = 1,         // includes map interior & coord values
    REDRAW_BACKGROUND = 2,  // includes box, map bord, key, coord labels, btns
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;
  /*........................................................................
  Dialog variables:
  ........................................................................*/
  bool cancel = false;  // true = user cancels
  int grabbed = 0;      // 1=TLeft,2=TRight,3=BRight,4=BLeft
  int map_x1 = 0;       // map coords x1, pixel coords
  int map_y1 = 0;       // map coords y1, pixel coords
  int delta1 = 0;
  int delta2 = 0;  // mouse-click proximity
  int mx = 0;
  int my = 0;  // last-saved mouse coords
  char txt[40];
  int txt_x = 0;
  int txt_y = 0;                    // for displaying text
  int color = 0;                    // for drawing map symbology
  ObjectClass* occupier = nullptr;  // cell's occupier
  /*........................................................................
  Buttons
  ........................................................................*/
  ControlClass* commands = nullptr;

  TextButtonClass okbtn(
      kButtonOk, TXT_OK,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kOkX, kOkY,
      kOkW, kOkH);

  TextButtonClass cancelbtn(
      kButtonCancel, TXT_CANCEL,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kCancelX,
      kCancelY, kCancelW, kCancelH);

  /*
  ------------------------------- Initialize -------------------------------
  */
  Set_Logic_Page(SeenBuff);

  /*........................................................................
  Set up the actual map area relative to the map's border coords
  ........................................................................*/
  if (x == -1) {
    map_x1 = kBordX1 + ((MAP_CELL_W - w) / 2) + 1;
  } else {
    map_x1 = kBordX1 + x + 1;
  }

  if (y == -1) {
    map_y1 = kBordY1 + ((MAP_CELL_H - h) / 2) + 1;
  } else {
    map_y1 = kBordY1 + y + 1;
  }

  int map_x2 = map_x1 + w - 1;  // map coords x2, pixel coords
  int map_y2 = map_y1 + h - 1;  // map coords y2, pixel coords

  /*
  ------------------------- Build the button list --------------------------
  */
  commands = &okbtn;
  cancelbtn.Add_Tail(*commands);

  /*------------------------------------------------------------------------
  Main processing loop
  ------------------------------------------------------------------------*/
  RedrawType display = REDRAW_ALL;  // requested redraw level
  bool process = true;              // Loop while true
  while (process) {
    /*
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      display = REDRAW_ALL;
    }

    /*
    ------------------------ Invoke game callback -------------------------
    */
    Call_Back();

    /*
    ---------------------- Refresh display if needed ----------------------
    */
    if (display != REDRAW_NONE) {
      Hide_Mouse();
      /*------------------------------------------------------------------
      Redraw the background, map border, key, and coord labels
      ------------------------------------------------------------------*/
      if (display >= REDRAW_BACKGROUND) {
        /*
        .......................... Background ...........................
        */
        Dialog_Box(kDialogX, kDialogY, kDialogW, kDialogH);
        Draw_Caption(TXT_NONE, kDialogX, kDialogY, kDialogW);

        /*
        ..................... Draw the map border .......................
        */
        LogicPage->Lock();
        LogicPage->Draw_Rect(kBordX1, kBordY1, kBordX2, kBordY2,
                             kCcGreenShadow);
        for (int index = kBordX1; index < kBordX2;
             index += (320 / ICON_PIXEL_W)) {
          LogicPage->Put_Pixel(index, kBordY1 - 1, kCcGreenShadow);
          LogicPage->Put_Pixel(index, kBordY2 + 1, kCcGreenShadow);
        }
        for (int index = kBordY1; index < kBordY2 - 8;
             index += (200 / ICON_PIXEL_H)) {
          LogicPage->Put_Pixel(kBordX1 - 1, index, kCcGreenShadow);
          LogicPage->Put_Pixel(kBordX2 + 1, index, kCcGreenShadow);
        }

        /*...............................................................
        Draw the map "key"
        ...............................................................*/
        txt_x = kDialogCx;
        txt_y = kDialogY + 8;
        Fancy_Text_Print("Clear Terrain", txt_x, txt_y, kLtGrey, kTBlack,
                         TPF_DROPSHADOW | TPF_6POINT);
        txt_y += 16;
        Fancy_Text_Print("Water", txt_x, txt_y, kBlue, kTBlack,
                         TPF_DROPSHADOW | TPF_6POINT);
        txt_y += 16;
        Fancy_Text_Print("Tiberium", txt_x, txt_y, kGrey, kTBlack,
                         TPF_DROPSHADOW | TPF_6POINT);
        txt_y += 16;
        Fancy_Text_Print("Rock/Wall/Road", txt_x, txt_y, kBrown, kTBlack,
                         TPF_DROPSHADOW | TPF_6POINT);
        txt_y += 16;
        Fancy_Text_Print("GDI Unit", txt_x, txt_y, kYellow, kTBlack,
                         TPF_DROPSHADOW | TPF_6POINT);
        txt_y += 16;
        Fancy_Text_Print("Nod Unit", txt_x, txt_y, kRed, kTBlack,
                         TPF_DROPSHADOW | TPF_6POINT);
        txt_y += 16;
        Fancy_Text_Print("Neutral Unit", txt_x, txt_y, kPurple, kTBlack,
                         TPF_DROPSHADOW | TPF_6POINT);
        txt_y += 16;
        Fancy_Text_Print("Terrain Object", txt_x, txt_y, kGreen, kTBlack,
                         TPF_DROPSHADOW | TPF_6POINT);
        txt_y += 16;
        Fancy_Text_Print("Starting Cell", txt_x, txt_y, kWhite, kTBlack,
                         TPF_DROPSHADOW | TPF_6POINT);
        /*
        .................. Draw the coordinate labels ...................
        */
        txt_x = kDialogX + (kDialogW / 8);
        txt_y = kDialogY + kDialogH - kOkH - 10 - 33;
        Fancy_Text_Print(
            "X", txt_x, txt_y, kCcGreen, kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        txt_x += (kDialogW - 20) / 4;
        Fancy_Text_Print(
            "Y", txt_x, txt_y, kCcGreen, kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        txt_x += (kDialogW - 20) / 4;
        Fancy_Text_Print(
            "Width", txt_x, txt_y, kCcGreen, kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        txt_x += (kDialogW - 20) / 4;
        Fancy_Text_Print(
            "Height", txt_x, txt_y, kCcGreen, kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        LogicPage->Unlock();

        /*
        ...................... Redraw the buttons .......................
        */
        commands->Flag_List_To_Redraw();
      }

      /*------------------------------------------------------------------
      Redraw the map symbology & location
      ------------------------------------------------------------------*/
      if (display >= REDRAW_MAP) {
        LogicPage->Lock();

        /*
        .................... Erase the map interior .....................
        */
        LogicPage->Fill_Rect(kBordX1 + 1, kBordY1 + 1, kBordX2 - 1, kBordY2 - 1,
                             kBlack);

        /*...............................................................
        Draw Land map symbols (use color according to Ground[] array).
        ...............................................................*/
        for (CELL cell = 0; cell < MAP_CELL_TOTAL; cell++) {
          occupier = (*this)[cell].Cell_Occupier();
          if (occupier == nullptr) {
            color = Ground[(*this)[cell].Land_Type()].Color;
            LogicPage->Put_Pixel(kBordX1 + Cell_X(cell) + 1,
                                 kBordY1 + Cell_Y(cell) + 1,
                                 static_cast<unsigned char>(color));
          }
        }

        LogicPage->Unlock();

        /*
        ................. Draw the actual map location ..................
        */
        LogicPage->Draw_Rect(map_x1, map_y1, map_x2, map_y2, kWhite);
        switch (grabbed) {
          case 1:
            LogicPage->Draw_Line(map_x1, map_y1, map_x1 + 5, map_y1, kBlue);
            LogicPage->Draw_Line(map_x1, map_y1, map_x1, map_y1 + 5, kBlue);
            break;

          case 2:
            LogicPage->Draw_Line(map_x2, map_y1, map_x2 - 5, map_y1, kBlue);
            LogicPage->Draw_Line(map_x2, map_y1, map_x2, map_y1 + 5, kBlue);
            break;

          case 3:
            LogicPage->Draw_Line(map_x2, map_y2, map_x2 - 5, map_y2, kBlue);
            LogicPage->Draw_Line(map_x2, map_y2, map_x2, map_y2 - 5, kBlue);
            break;

          case 4:
            LogicPage->Draw_Line(map_x1, map_y2, map_x1 + 5, map_y2, kBlue);
            LogicPage->Draw_Line(map_x1, map_y2, map_x1, map_y2 - 5, kBlue);
            break;

          case 5:
            LogicPage->Draw_Rect(map_x1, map_y1, map_x2, map_y2, kBlue);
            break;

          default:
            break;
        }

        /*...............................................................
        Draw Unit map symbols (Use the radar map color according to
        that specified in the house type class object.
        DKGREEN = terrain object
        ...............................................................*/
        for (CELL cell = 0; cell < MAP_CELL_TOTAL; cell++) {
          occupier = (*this)[cell].Cell_Occupier();
          if (occupier) {
            color = kGreen;
            if (occupier && occupier->Owner() != HOUSE_NONE) {
              color = HouseClass::As_Pointer(occupier->Owner())->Color;
            }
            LogicPage->Put_Pixel(kBordX1 + Cell_X(cell) + 1,
                                 kBordY1 + Cell_Y(cell) + 1,
                                 static_cast<unsigned char>(color));
          }
        }

        /*
        ...................... Draw Home location .......................
        */
        LogicPage->Put_Pixel(kBordX1 + Cell_X(Waypoint[kWayptHome]) + 1,
                             kBordY1 + Cell_Y(Waypoint[kWayptHome]) + 1,
                             kWhite);

        /*
        ..................... Erase old coordinates .....................
        */
        LogicPage->Fill_Rect(kDialogX + 7, kDialogY + kDialogH - kOkH - 10 - 22,
                             kDialogX + kDialogW - 7,
                             kDialogY + kDialogH - kOkH - 10 - 22 + 10, kBlack);

        /*
        ..................... Draw the coordinates ......................
        */
        txt_x = kDialogX + (kDialogW / 8);
        txt_y = kDialogY + kDialogH - kOkH - 10 - 22;
        absl::SNPrintF(txt, sizeof(txt), "%d", map_x1 - kBordX1 - 1);
        Fancy_Text_Print(
            txt, txt_x, txt_y, kCcGreen, kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        txt_x += (kDialogW - 20) / 4;
        absl::SNPrintF(txt, sizeof(txt), "%d", map_y1 - kBordY1 - 1);
        Fancy_Text_Print(
            txt, txt_x, txt_y, kCcGreen, kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        txt_x += (kDialogW - 20) / 4;
        absl::SNPrintF(txt, sizeof(txt), "%d", map_x2 - map_x1 + 1);
        Fancy_Text_Print(
            txt, txt_x, txt_y, kCcGreen, kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        txt_x += (kDialogW - 20) / 4;
        absl::SNPrintF(txt, sizeof(txt), "%d", map_y2 - map_y1 + 1);
        Fancy_Text_Print(
            txt, txt_x, txt_y, kCcGreen, kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
      }

      Show_Mouse();
      display = REDRAW_NONE;
    }

    /*
    ------------------------- Process user input --------------------------
    */
    const KeyNumType input = commands->Input();  // user input
    /*.....................................................................
    Normal button processing: This is done when the mouse button is NOT
    being held down ('grabbed' is 0).
    .....................................................................*/
    if (grabbed == 0) {
      switch (static_cast<int>(input)) {
        case KN_RETURN:
        case ButtonKey(kButtonOk):
          cancel = false;
          process = false;
          break;

        case KN_ESC:
        case ButtonKey(kButtonCancel):
          cancel = true;
          process = false;
          break;

        case KN_LMOUSE:
          /*
          ....................... Grab top left ........................
          */
          delta1 = abs(ActiveKeyboard->MouseQX - map_x1);
          delta2 = abs(ActiveKeyboard->MouseQY - map_y1);
          if (delta1 < 3 && delta2 < 3) {
            grabbed = 1;
            mx = ActiveKeyboard->MouseQX;
            my = ActiveKeyboard->MouseQY;
            display = REDRAW_MAP;
            break;
          }
          /*
          ...................... Grab top right ........................
          */
          delta1 = abs(ActiveKeyboard->MouseQX - map_x2);
          delta2 = abs(ActiveKeyboard->MouseQY - map_y1);
          if (delta1 < 3 && delta2 < 3) {
            grabbed = 2;
            mx = ActiveKeyboard->MouseQX;
            my = ActiveKeyboard->MouseQY;
            display = REDRAW_MAP;
            break;
          }
          /*
          ..................... Grab bottom right ......................
          */
          delta1 = abs(ActiveKeyboard->MouseQX - map_x2);
          delta2 = abs(ActiveKeyboard->MouseQY - map_y2);
          if (delta1 < 3 && delta2 < 3) {
            grabbed = 3;
            mx = ActiveKeyboard->MouseQX;
            my = ActiveKeyboard->MouseQY;
            display = REDRAW_MAP;
            break;
          }
          /*
          ..................... Grab bottom left .......................
          */
          delta1 = abs(ActiveKeyboard->MouseQX - map_x1);
          delta2 = abs(ActiveKeyboard->MouseQY - map_y2);
          if (delta1 < 3 && delta2 < 3) {
            grabbed = 4;
            mx = ActiveKeyboard->MouseQX;
            my = ActiveKeyboard->MouseQY;
            display = REDRAW_MAP;
            break;
          }
          /*
          ..................... Grab the whole map .....................
          */
          delta1 = abs(ActiveKeyboard->MouseQX - ((map_x1 + map_x2) / 2));
          delta2 = abs(ActiveKeyboard->MouseQY - ((map_y1 + map_y2) / 2));
          if (delta1 < (map_x2 - map_x1) / 4 &&
              delta2 < (map_y2 - map_y1) / 4) {
            grabbed = 5;
            mx = ActiveKeyboard->MouseQX;
            my = ActiveKeyboard->MouseQY;
            display = REDRAW_MAP;
          }
          break;

        default:
          break;
      }
    } else {
      /*.....................................................................
      Mouse motion processing: This is done while the left mouse button IS
      being held down.
      - First, check for the button release; if detected, un-grab
      - Then, handle mouse motion. WWLIB doesn't pass through a KN_MOUSE_MOVE
        value while the button is being held down, so this case must be
        trapped as a default.
      .....................................................................*/
      if (static_cast<int>(input) == (KN_LMOUSE | KN_RLSE_BIT)) {
        grabbed = 0;
        display = REDRAW_MAP;
      } else {
        delta1 = Get_Mouse_X() - mx;
        delta2 = Get_Mouse_Y() - my;
        if (delta1 == 0 && delta2 == 0) {
          break;
        }

        /*
        ....................... Move top left ........................
        */
        if (grabbed == 1) {
          map_x1 += delta1;
          map_x1 = std::clamp(map_x1, kBordX1 + 2, map_x2 - 2);
          map_y1 += delta2;
          map_y1 = std::clamp(map_y1, kBordY1 + 2, map_y2 - 2);
          display = REDRAW_MAP;
          mx = Get_Mouse_X();
          my = Get_Mouse_Y();
        }

        /*
        ....................... Move top right .......................
        */
        if (grabbed == 2) {
          map_x2 += delta1;
          map_x2 = std::clamp(map_x2, map_x1 + 2, kBordX2 - 2);
          map_y1 += delta2;
          map_y1 = std::clamp(map_y1, kBordY1 + 2, map_y2 - 2);
          display = REDRAW_MAP;
          mx = Get_Mouse_X();
          my = Get_Mouse_Y();
        }

        /*
        ..................... Move bottom right ......................
        */
        if (grabbed == 3) {
          map_x2 += delta1;
          map_x2 = std::clamp(map_x2, map_x1 + 2, kBordX2 - 2);
          map_y2 += delta2;
          map_y2 = std::clamp(map_y2, map_y1 + 2, kBordY2 - 2);
          display = REDRAW_MAP;
          mx = Get_Mouse_X();
          my = Get_Mouse_Y();
        }

        /*
        ...................... Move bottom left ......................
        */
        if (grabbed == 4) {
          map_x1 += delta1;
          map_x1 = std::clamp(map_x1, kBordX1 + 2, map_x2 - 2);
          map_y2 += delta2;
          map_y2 = std::clamp(map_y2, map_y1 + 2, kBordY2 - 2);
          display = REDRAW_MAP;
          mx = Get_Mouse_X();
          my = Get_Mouse_Y();
        }

        /*
        ....................... Move whole map .......................
        */
        if (grabbed == 5) {
          if (map_x1 + delta1 > kBordX1 + 1 && map_x2 + delta1 < kBordX2 - 1) {
            map_x1 += delta1;
            map_x2 += delta1;
          }

          if (map_y1 + delta2 > kBordY1 + 1 && map_y2 + delta2 < kBordY2 - 1) {
            map_y1 += delta2;
            map_y2 += delta2;
          }
          display = REDRAW_MAP;
          mx = Get_Mouse_X();
          my = Get_Mouse_Y();
        }
      }
    }
  }

  /*
  --------------------------- Redraw the display ---------------------------
  */
  HiddenPage.Clear();
  Flag_To_Redraw(true);
  Render();

  /*
  ------------------------- If cancel, just return -------------------------
  */
  if (cancel) {
    return (-1);
  }

  /*
  ---------------------------- Save selections -----------------------------
  */
  MapCellX = map_x1 - kBordX1 - 1;
  MapCellY = map_y1 - kBordY1 - 1;
  MapCellWidth = map_x2 - map_x1 + 1;
  MapCellHeight = map_y2 - map_y1 + 1;

  /*
  --------------------- Clip Home Cell to new map size ---------------------
  */
  if (Cell_X(Waypoint[kWayptHome]) < MapCellX) {
    Waypoint[kWayptHome] = XY_Cell(MapCellX, Cell_Y(Waypoint[kWayptHome]));
  }

  if (Cell_X(Waypoint[kWayptHome]) > MapCellX + MapCellWidth - 1) {
    Waypoint[kWayptHome] =
        XY_Cell(MapCellX + MapCellWidth - 1, Cell_Y(Waypoint[kWayptHome]));
  }

  if (Cell_Y(Waypoint[kWayptHome]) < MapCellY) {
    Waypoint[kWayptHome] = XY_Cell(Cell_X(Waypoint[kWayptHome]), MapCellY);
  }

  if (Cell_Y(Waypoint[kWayptHome]) > MapCellY + MapCellHeight - 1) {
    Waypoint[kWayptHome] =
        XY_Cell(Cell_X(Waypoint[kWayptHome]), MapCellY + MapCellHeight - 1);
  }

  return 0;
}

/***************************************************************************
 * MapEditClass::Scenario_Dialog -- scenario global parameters dialog      *
 *                                                                         *
 * Lets the user edit the Theater, starting credits for houses, and the    *
 * Edge for HOUSE_GOOD & HOUSE_BAD.                                        *
 *                                                                         *
 * Ŀ *
 *        Theater                                Credits / 1000          *
 *   Ŀ                                                   *
 *    Temperate                               GDI: _____               *
 *    Desert                                  NOD: _____               *
 *    Jungle                              Neutral: _____               *
 *                                                                     *
 *                                                      *
 *    Build Level:___                                                    *
 *                                                                       *
 *                          Reinforcements                               *
 *                                                                       *
 *                     GDI                NOD                            *
 *                                                                       *
 *                                                                     *
 *                   <-   ->            <-   ->                          *
 *                                                                     *
 *                                                                       *
 *                                                                       *
 *                        [OK]     [Cancel]                              *
 *                                                                       *
 *  *
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
 *=========================================================================*/
int MapEditClass::Scenario_Dialog() {
  /*........................................................................
  Dialog & button dimensions
  ........................................................................*/
  constexpr int kDialogW = 544;
  constexpr int kDialogH = 320;
  constexpr int kDialogX = ((640 - kDialogW) / 2);
  constexpr int kDialogY = ((400 - kDialogH) / 2);
  constexpr int kDialogCx = kDialogX + (kDialogW / 2);
  constexpr int kTxt8H = 22;
  constexpr int kMargin = 14;
  constexpr int kTheaterW = 200;
  constexpr int kTheaterH = 68;
  constexpr int kTheaterX = kDialogX + kMargin;
  constexpr int kTheaterY = kDialogY + kMargin + kTxt8H;
  constexpr int kLevelW = 80;
  constexpr int kLevelH = 18;
  constexpr int kLevelX = kTheaterX + kTheaterW - kLevelW;
  constexpr int kLevelY = kTheaterY + kTheaterH + kMargin;
  constexpr int kGdicredW = 120;
  constexpr int kGdicredH = 18;
  constexpr int kGdicredX = kDialogX + kDialogW - kMargin - kGdicredW;
  constexpr int kGdicredY = kDialogY + kMargin + kTxt8H;
  constexpr int kNodcredW = 120;
  constexpr int kNodcredH = 18;
  constexpr int kNodcredX = kGdicredX;
  constexpr int kNodcredY = kGdicredY + kGdicredH;
  constexpr int kNeutcredW = 120;
  constexpr int kNeutcredH = 18;
  constexpr int kNeutcredX = kGdicredX;
  constexpr int kNeutcredY = kNodcredY + kNodcredH;
  constexpr int kGdinW = 26;
  constexpr int kGdinH = 18;
  constexpr int kGdinX = kDialogCx - 5 - (kGdinW * 2);
  constexpr int kGdinY =
      kLevelY + kLevelH + kMargin + kTxt8H + kMargin + kTxt8H;
  constexpr int kGdisW = 26;
  constexpr int kGdisH = 18;
  constexpr int kGdisX = kGdinX;
  constexpr int kGdisY = kGdinY + (kGdinH * 2);
  constexpr int kGdiwW = 26;
  constexpr int kGdiwH = 18;
  constexpr int kGdiwX = kDialogCx - 5 - (kGdinW * 3);
  constexpr int kGdiwY = kGdinY + kGdinH;
  constexpr int kGdieW = 26;
  constexpr int kGdieH = 18;
  constexpr int kGdieX = kDialogCx - 5 - kGdinW;
  constexpr int kGdieY = kGdinY + kGdinH;
  constexpr int kNodnW = 26;
  constexpr int kNodnH = 18;
  constexpr int kNodnX = kDialogCx + 5 + kNodnW;
  constexpr int kNodnY =
      kLevelY + kLevelH + kMargin + kTxt8H + kMargin + kTxt8H;
  constexpr int kNodsW = 26;
  constexpr int kNodsH = 18;
  constexpr int kNodsX = kNodnX;
  constexpr int kNodsY = kNodnY + (kNodnH * 2);
  constexpr int kNodwW = 26;
  constexpr int kNodwH = 18;
  constexpr int kNodwX = kDialogCx + 5;
  constexpr int kNodwY = kNodnY + kNodnH;
  constexpr int kNodeW = 26;
  constexpr int kNodeH = 18;
  constexpr int kNodeX = kDialogCx + 5 + (kNodnW * 2);
  constexpr int kNodeY = kNodnY + kNodnH;
  constexpr int kOkW = 90;
  constexpr int kOkH = 18;
  constexpr int kOkX = kDialogCx - kOkW - 5;
  constexpr int kOkY = kDialogY + kDialogH - kOkH - kMargin;
  constexpr int kCancelW = 90;
  constexpr int kCancelH = 18;
  constexpr int kCancelX = kDialogCx + 5;
  constexpr int kCancelY = kDialogY + kDialogH - kCancelH - kMargin;
  /*........................................................................
  Button enumerations:
  ........................................................................*/
  constexpr int kListTheater = 100;
  constexpr int kTeditGdicred = 102;
  constexpr int kTeditNodcred = 103;
  constexpr int kTeditNeutcred = 104;
  constexpr int kButtonGdiN = 105;
  constexpr int kButtonGdiE = 106;
  constexpr int kButtonGdiS = 107;
  constexpr int kButtonGdiW = 108;
  constexpr int kButtonNodN = 109;
  constexpr int kButtonNodE = 110;
  constexpr int kButtonNodS = 111;
  constexpr int kButtonNodW = 112;
  constexpr int kButtonOk = 113;
  constexpr int kButtonCancel = 114;
  /*........................................................................
  Redraw values: in order from "top" to "bottom" layer of the dialog
  ........................................................................*/
  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_BUTTONS = 1,     // includes map interior & coord values
    REDRAW_BACKGROUND = 2,  // includes box, map bord, key, coord labels, btns
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;
  /*........................................................................
  Dialog variables:
  ........................................................................*/
  bool cancel = false;  // true = user cancels
  /*
  .......................... Scenario parameters ...........................
  */
  int64_t gdi_credits = 0;   // HouseClass::As_Pointer(HouseType)->Credits
  int64_t nod_credits = 0;   // HouseClass::As_Pointer(HouseType)->Credits
  int64_t neut_credits = 0;  // HouseClass::As_Pointer(HouseType)->Credits
  SourceType gdi_edge = SOURCE_NONE;  // HouseClass::As_Pointer(HouseType)->Edge
  SourceType nod_edge = SOURCE_NONE;  // HouseClass::As_Pointer(HouseType)->Edge
  char level_buf[10] = {0};
  char gdicred_buf[10] = {0};
  char nodcred_buf[10] = {0};
  char neutcred_buf[10] = {0};
  /*
  ....................... Theater-changing variables .......................
  */
  uint32_t theater_mask = 0;  // template/terrain mask
  /*........................................................................
  Buttons
  ........................................................................*/
  ControlClass* commands = nullptr;  // the button list
  ListClass theaterbtn(kListTheater, kTheaterX, kTheaterY, kTheaterW, kTheaterH,
                       TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                       Hires_Retrieve("BTN-UP.SHP"),
                       Hires_Retrieve("BTN-DN.SHP"));

  EditClass leveledt(kTeditGdicred, level_buf, 4,
                     TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kLevelX,
                     kLevelY, kLevelW, kLevelH, EditClass::NUMERIC);

  EditClass gdicred(kTeditGdicred, gdicred_buf, 8,
                    TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kGdicredX,
                    kGdicredY, kGdicredW, kGdicredH, EditClass::NUMERIC);

  EditClass nodcred(kTeditNodcred, nodcred_buf, 8,
                    TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kNodcredX,
                    kNodcredY, kNodcredW, kNodcredH, EditClass::NUMERIC);

  EditClass neutcred(kTeditNeutcred, neutcred_buf, 8,
                     TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kNeutcredX,
                     kNeutcredY, kNeutcredW, kNeutcredH, EditClass::NUMERIC);

  TextButtonClass gdinbtn(
      kButtonGdiN, TXT_UP,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kGdinX,
      kGdinY, kGdinW, kGdinH);

  TextButtonClass gdiebtn(
      kButtonGdiE, TXT_RIGHT,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kGdieX,
      kGdieY, kGdieW, kGdieH);

  TextButtonClass gdisbtn(
      kButtonGdiS, TXT_DOWN,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kGdisX,
      kGdisY, kGdisW, kGdisH);

  TextButtonClass gdiwbtn(
      kButtonGdiW, TXT_LEFT,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kGdiwX,
      kGdiwY, kGdiwW, kGdiwH);

  TextButtonClass nodnbtn(
      kButtonNodN, TXT_UP,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kNodnX,
      kNodnY, kNodnW, kNodnH);

  TextButtonClass nodebtn(
      kButtonNodE, TXT_RIGHT,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kNodeX,
      kNodeY, kNodeW, kNodeH);

  TextButtonClass nodsbtn(
      kButtonNodS, TXT_DOWN,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kNodsX,
      kNodsY, kNodsW, kNodsH);

  TextButtonClass nodwbtn(
      kButtonNodW, TXT_LEFT,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kNodwX,
      kNodwY, kNodwW, kNodwH);

  TextButtonClass okbtn(
      kButtonOk, TXT_OK,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kOkX, kOkY,
      kOkW, kOkH);

  TextButtonClass cancelbtn(
      kButtonCancel, TXT_CANCEL,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kCancelX,
      kCancelY, kCancelW, kCancelH);

  /*
  ------------------------------- Initialize -------------------------------
  */
  Set_Logic_Page(SeenBuff);

  /*
  .......................... Fill in theater items .........................
  */
  theaterbtn.Add_Item("Desert");
  theaterbtn.Add_Item("Jungle");
  theaterbtn.Add_Item("Temperate");
  theaterbtn.Add_Item("Winter");

  /*
  ............................ Init parameters .............................
  */
  const TheaterType orig_theater = Theater;  // original theater
  if (ScenPlayer != SCEN_PLAYER_MPLAYER) {
    gdi_credits = HouseClass::As_Pointer(HOUSE_GOOD)->Credits / 1000L;
    nod_credits = HouseClass::As_Pointer(HOUSE_BAD)->Credits / 1000L;
    neut_credits = HouseClass::As_Pointer(HOUSE_NEUTRAL)->Credits / 1000L;
    gdi_edge = HouseClass::As_Pointer(HOUSE_GOOD)->Edge;
    nod_edge = HouseClass::As_Pointer(HOUSE_BAD)->Edge;
  } else {
    gdi_credits = 0;
    nod_credits = 0;
    neut_credits = 0;
    gdi_edge = SOURCE_NONE;
    nod_edge = SOURCE_NONE;
  }

  /*
  ............................ Create the list .............................
  */
  commands = &theaterbtn;
  leveledt.Add_Tail(*commands);
  gdicred.Add_Tail(*commands);
  nodcred.Add_Tail(*commands);
  neutcred.Add_Tail(*commands);
  gdinbtn.Add_Tail(*commands);
  gdiebtn.Add_Tail(*commands);
  gdisbtn.Add_Tail(*commands);
  gdiwbtn.Add_Tail(*commands);
  nodnbtn.Add_Tail(*commands);
  nodebtn.Add_Tail(*commands);
  nodsbtn.Add_Tail(*commands);
  nodwbtn.Add_Tail(*commands);
  okbtn.Add_Tail(*commands);
  cancelbtn.Add_Tail(*commands);

  /*
  ...................... Init GDI Edge button states .......................
  */
  if (gdi_edge == SOURCE_NORTH) {
    gdinbtn.Turn_On();
  }
  if (gdi_edge == SOURCE_EAST) {
    gdiebtn.Turn_On();
  }
  if (gdi_edge == SOURCE_SOUTH) {
    gdisbtn.Turn_On();
  }
  if (gdi_edge == SOURCE_WEST) {
    gdiwbtn.Turn_On();
  }

  /*
  ...................... Init NOD Edge button states .......................
  */
  if (nod_edge == SOURCE_NORTH) {
    nodnbtn.Turn_On();
  }
  if (nod_edge == SOURCE_EAST) {
    nodebtn.Turn_On();
  }
  if (nod_edge == SOURCE_SOUTH) {
    nodsbtn.Turn_On();
  }
  if (nod_edge == SOURCE_WEST) {
    nodwbtn.Turn_On();
  }

  /*
  .......................... Init credits buffers ..........................
  */
  absl::SNPrintF(level_buf, sizeof(level_buf), "%d", BuildLevel);
  leveledt.Set_Text(level_buf, 4);

  absl::SNPrintF(gdicred_buf, sizeof(gdicred_buf), "%ld", gdi_credits);
  gdicred.Set_Text(gdicred_buf, 8);

  absl::SNPrintF(nodcred_buf, sizeof(nodcred_buf), "%ld", nod_credits);
  nodcred.Set_Text(nodcred_buf, 8);

  absl::SNPrintF(neutcred_buf, sizeof(neutcred_buf), "%ld", neut_credits);
  neutcred.Set_Text(neutcred_buf, 8);

  theaterbtn.Set_Selected_Index(static_cast<int>(orig_theater) -
                                static_cast<int>(THEATER_NONE) - 1);

  /*
  -------------------------- Main Processing Loop --------------------------
  */
  RedrawType display = REDRAW_ALL;  // true = re-draw everything
  bool process = true;              // loop while true
  while (process) {
    /*
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      display = REDRAW_ALL;
    }

    /*
    ........................ Invoke game callback .........................
    */
    Call_Back();

    /*
    ...................... Refresh display if needed ......................
    */
    if (display != REDRAW_NONE) {
      Hide_Mouse();
      if (display >= REDRAW_BACKGROUND) {
        /*
        ..................... Draw the background .......................
        */
        Dialog_Box(kDialogX, kDialogY, kDialogW, kDialogH);
        Draw_Caption(TXT_NONE, kDialogX, kDialogY, kDialogW);

        /*
        ....................... Draw the labels .........................
        */
        Fancy_Text_Print(
            "Theater", kTheaterX + (kTheaterW / 2), kTheaterY - kTxt8H,
            kCcGreen, kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        Fancy_Text_Print(
            "Build Level", kLevelX, kLevelY, kCcGreen, kTBlack,
            TPF_RIGHT | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        Fancy_Text_Print(
            "Credits/1000", kGdicredX + (kGdicredW / 2), kGdicredY - kTxt8H,
            kCcGreen, kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        Fancy_Text_Print(
            "GDI", kGdicredX - 5, kGdicredY, kCcGreen, kTBlack,
            TPF_RIGHT | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        Fancy_Text_Print(
            "NOD", kNodcredX - 5, kNodcredY, kCcGreen, kTBlack,
            TPF_RIGHT | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        Fancy_Text_Print(
            "Neutral", kNeutcredX - 5, kNeutcredY, kCcGreen, kTBlack,
            TPF_RIGHT | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        Fancy_Text_Print(
            "Reinforcements", kDialogCx, kLevelY + kLevelH + kMargin, kCcGreen,
            kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        Fancy_Text_Print(
            "GDI", kGdinX + (kGdinW / 2), kGdinY - kTxt8H, kCcGreen, kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        Fancy_Text_Print(
            "NOD", kNodnX + (kNodnW / 2), kNodnY - kTxt8H, kCcGreen, kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
      }
      Show_Mouse();
      display = REDRAW_NONE;
    }

    /*
    ........................... Get user input ............................
    */
    const KeyNumType input = commands->Input();  // input from user

    /*
    ............................ Process input ............................
    */
    switch (static_cast<int>(input)) {
      /*..................................................................
      Credit edit boxes: no need for any action
      ..................................................................*/
      case ButtonKey(kTeditGdicred):
      case ButtonKey(kTeditNodcred):
      case ButtonKey(kTeditNeutcred):
        break;

      /*..................................................................
      GDI Edge buttons: turn this one on, others off, save the edge value
      ..................................................................*/
      case ButtonKey(kButtonGdiN):
        gdi_edge = SOURCE_NORTH;
        gdinbtn.Turn_On();
        gdiebtn.Turn_Off();
        gdisbtn.Turn_Off();
        gdiwbtn.Turn_Off();
        break;

      case ButtonKey(kButtonGdiE):
        gdi_edge = SOURCE_EAST;
        gdinbtn.Turn_Off();
        gdiebtn.Turn_On();
        gdisbtn.Turn_Off();
        gdiwbtn.Turn_Off();
        break;

      case ButtonKey(kButtonGdiS):
        gdi_edge = SOURCE_SOUTH;
        gdinbtn.Turn_Off();
        gdiebtn.Turn_Off();
        gdisbtn.Turn_On();
        gdiwbtn.Turn_Off();
        break;

      case ButtonKey(kButtonGdiW):
        gdi_edge = SOURCE_WEST;
        gdinbtn.Turn_Off();
        gdiebtn.Turn_Off();
        gdisbtn.Turn_Off();
        gdiwbtn.Turn_On();
        break;

      /*..................................................................
      NOD Edge buttons: turn this one on, others off, save the edge value
      ..................................................................*/
      case ButtonKey(kButtonNodN):
        nod_edge = SOURCE_NORTH;
        nodnbtn.Turn_On();
        nodebtn.Turn_Off();
        nodsbtn.Turn_Off();
        nodwbtn.Turn_Off();
        break;

      case ButtonKey(kButtonNodE):
        nod_edge = SOURCE_EAST;
        nodnbtn.Turn_Off();
        nodebtn.Turn_On();
        nodsbtn.Turn_Off();
        nodwbtn.Turn_Off();
        break;

      case ButtonKey(kButtonNodS):
        nod_edge = SOURCE_SOUTH;
        nodnbtn.Turn_Off();
        nodebtn.Turn_Off();
        nodsbtn.Turn_On();
        nodwbtn.Turn_Off();
        break;

      case ButtonKey(kButtonNodW):
        nod_edge = SOURCE_WEST;
        nodnbtn.Turn_Off();
        nodebtn.Turn_Off();
        nodsbtn.Turn_Off();
        nodwbtn.Turn_On();
        break;

      case KN_RETURN:
      case ButtonKey(kButtonOk):
        cancel = false;
        process = false;
        break;

      case KN_ESC:
      case ButtonKey(kButtonCancel):
        cancel = true;
        process = false;
        break;

      default:
        break;
    }
  }

  /*
  ----------------------------- Redraw the map -----------------------------
  */
  HiddenPage.Clear();
  Flag_To_Redraw(true);
  Render();

  /*
  ------------------------- If cancel, just return -------------------------
  */
  if (cancel) {
    return (-1);
  }

  /*
  ------------------------ Save selections & return ------------------------
  */
  if (ScenPlayer != SCEN_PLAYER_MPLAYER) {
    /*
    .............................. Credits ................................
    */
    gdi_credits =
        tech::ParseInteger<decltype(gdi_credits)>(gdicred_buf).value_or(0);
    nod_credits =
        tech::ParseInteger<decltype(nod_credits)>(nodcred_buf).value_or(0);
    neut_credits =
        tech::ParseInteger<decltype(neut_credits)>(neutcred_buf).value_or(0);
    HouseClass::As_Pointer(HOUSE_GOOD)->Credits = gdi_credits * 1000L;
    HouseClass::As_Pointer(HOUSE_BAD)->Credits = nod_credits * 1000L;
    HouseClass::As_Pointer(HOUSE_NEUTRAL)->Credits = neut_credits * 1000L;
    /*
    ............................... Edges .................................
    */
    HouseClass::As_Pointer(HOUSE_GOOD)->Edge = gdi_edge;
    HouseClass::As_Pointer(HOUSE_BAD)->Edge = nod_edge;
  }

  /*
  ........................... Sidebar build level ..........................
  */
  BuildLevel = tech::ParseInteger<int>(level_buf).value_or(0);

  /*........................................................................
  Change the theater:
  - 1st set the Theater global
  - scan all cells to check their TType for compatibility with the new
    theater; if not compatible, set TType to TEMPLATE_NONE & TIcon to 0
  - Then, re-initialize the TypeClasses for the new Theater
  ........................................................................*/
  const auto theater = static_cast<TheaterType>(
      static_cast<int>(THEATER_NONE) + 1 +
      theaterbtn.Current_Index());  // DisplayClass::Theater
  if (theater != orig_theater) {
    /*
    ....................... Loop through all cells ........................
    */
    for (CELL i = 0; i < MAP_CELL_TOTAL; i++) {
      /*..................................................................
      If this cell has a template icon & that template isn't compatible
      with this theater, set the icon to NONE
      ..................................................................*/
      if ((*this)[i].TType != TEMPLATE_NONE) {
        theater_mask =
            TemplateTypeClass::As_Reference((*this)[i].TType).Theater;
        if ((theater_mask & base::Bit<uint32_t>(theater)) == 0) {
          (*this)[i].TType = TEMPLATE_NONE;
          (*this)[i].TIcon = 0;
        }
      }
      /*..................................................................
      If this cell has terrain in it, and that terrain isn't compatible
      with this theater, delete the terrain object.
      ..................................................................*/
      TerrainClass* terrain =
          (*this)[i].Cell_Terrain();  // cell's terrain pointer
      if (terrain) {
        theater_mask = terrain->Class->Theater;
        if ((theater_mask & base::Bit<uint32_t>(theater)) == 0) {
          delete terrain;
        }
      }
    }

    /*.....................................................................
    Re-init the object Type Classes for this theater
    .....................................................................*/
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
    SmudgeTypeClass::Init(theater);
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

  /*------------------------------------------------------------------------
  Trigger dialog processing loop:
  - Invoke the trigger selection dialog. If a trigger's selected, break
    & return
  - If user wants to edit the current trigger, do so
  - If user wants to create new trigger, new a TriggerClass & edit it
  - If user wants to delete trigger, delete the current trigger
  - Keep looping until 'OK'
  ------------------------------------------------------------------------*/
  while (true) {
    /*
    ........................... Select trigger ............................
    */
    const int rc = Select_Trigger();

    /*
    ............................. 'OK'; break .............................
    */
    if (rc == 0) {
      break;
    }

    /*
    ............................... 'Edit' ................................
    */
    if ((rc == 1 && CurTrigger) && (Edit_Trigger() == 0)) {
      Changed = true;
    }

    /*
    ................................ 'New' ................................
    */
    if (rc == 2) {
      /*
      ..................... Create a new trigger ......................
      */
      CurTrigger = new TriggerClass();
      if (CurTrigger) {
        /*
        ................... delete it if user cancels ...................
        */
        if (Edit_Trigger() == -1) {
          delete CurTrigger;
          CurTrigger = nullptr;
        } else {
          Changed = true;
        }

      } else {
        /*
        ................. Unable to create; issue warning ..................
        */
        CCMessageBox().Process("No more triggers available.");
        HiddenPage.Clear();
        Flag_To_Redraw(true);
        Render();
      }
    }

    /*
    .............................. 'Delete' ...............................
    */
    if ((rc == 3) && CurTrigger) {
      CurTrigger->Remove();
      CurTrigger = nullptr;
      Changed = true;
    }
  }

  /*------------------------------------------------------------------------
  Don't allow trigger placement if the trigger is house-specific; such
  triggers cannot be "placed".
  ------------------------------------------------------------------------*/
  if (CurTrigger && (!TriggerClass::Event_Need_Object(CurTrigger->Event))) {
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
 *=========================================================================*/
int MapEditClass::Select_Trigger() {
  /*........................................................................
  Dialog & button dimensions
  ........................................................................*/
  constexpr int kDialogW = 640;
  constexpr int kDialogH = 290;
  constexpr int kDialogX = ((640 - kDialogW) / 2);
  constexpr int kDialogY = ((400 - kDialogH) / 2);
  constexpr int kDialogCx = kDialogX + (kDialogW / 2);
  constexpr int kTxt8H = 22;
  constexpr int kMargin = 14;
  constexpr int kListW = 612;
  constexpr int kListH = 208;
  constexpr int kListX = kDialogX + kMargin;
  constexpr int kListY = kDialogY + kMargin + kTxt8H;
  constexpr int kEditW = 90;
  constexpr int kEditH = 18;
  constexpr int kEditX = kDialogX + (kDialogW / 8) - (kEditW / 2);
  constexpr int kEditY = kDialogY + kDialogH - kMargin - kEditH;
  constexpr int kNewW = 90;
  constexpr int kNewH = 18;
  constexpr int kNewX = kDialogX + ((kDialogW / 8) * 3) - (kNewW / 2);
  constexpr int kNewY = kDialogY + kDialogH - kMargin - kNewH;
  constexpr int kDeleteW = 90;
  constexpr int kDeleteH = 18;
  constexpr int kDeleteX = kDialogX + ((kDialogW / 8) * 5) - (kDeleteW / 2);
  constexpr int kDeleteY = kDialogY + kDialogH - kMargin - kDeleteH;
  constexpr int kOkW = 90;
  constexpr int kOkH = 18;
  constexpr int kOkX = kDialogX + ((kDialogW / 8) * 7) - (kOkW / 2);
  constexpr int kOkY = kDialogY + kDialogH - kMargin - kOkH;

  /*........................................................................
  Button enumerations:
  ........................................................................*/
  constexpr int kTriggerList = 100;
  constexpr int kButtonEdit = 101;
  constexpr int kButtonNew = 102;
  constexpr int kButtonDelete = 103;
  constexpr int kButtonOk = 104;

  /*........................................................................
  Redraw values: in order from "top" to "bottom" layer of the dialog
  ........................................................................*/
  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_BUTTONS = 1,
    REDRAW_BACKGROUND = 2,
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;

  /*........................................................................
  Dialog variables:
  ........................................................................*/
  char* trigtext[kTriggerMax + 1] = {};     // text for defined triggers
  bool edit_trig = false;                   // true = user wants to edit
  bool new_trig = false;                    // true = user wants to new
  bool del_trig = false;                    // true = user wants to new
  static int tabs[] = {70, 240, 390, 440};  // list box tab stops

  /*........................................................................
  Buttons
  ........................................................................*/
  ControlClass* commands = nullptr;  // the button list

  ListClass triggerlist(kTriggerList, kListX, kListY, kListW, kListH,
                        TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                        Hires_Retrieve("BTN-UP.SHP"),
                        Hires_Retrieve("BTN-DN.SHP"));

  TextButtonClass editbtn(
      kButtonEdit, "Edit",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kEditX,
      kEditY, kEditW, kEditH);

  TextButtonClass newbtn(
      kButtonNew, "New",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kNewX, kNewY,
      kNewW, kNewH);

  TextButtonClass deletebtn(
      kButtonDelete, "Delete",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kDeleteX,
      kDeleteY, kDeleteW, kDeleteH);

  TextButtonClass okbtn(
      kButtonOk, TXT_OK,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kOkX, kOkY,
      kOkW, kOkH);

  /*
  ------------------------------- Initialize -------------------------------
  */
  Set_Logic_Page(SeenBuff);

  /*
  ......................... Fill in trigger names ..........................
  */
  int def_idx = 0;  // default list index
  for (int i = 0; i < Triggers.Count(); i++) {
    /*.....................................................................
    Generate string for this trigger
    - Name can be up to 4 characters
    - Event can be up to 15 characters
    - Action can be up to 15 characters
    - House is 3 characters
    - Team name is up to 11 characters
    .....................................................................*/
    // trigtext[i] = (char *)HidPage.Get_Graphic_Buffer()->Get_Buffer() + 60 *
    // i;
    constexpr int kTrigTextSize = 255;
    trigtext[i] = new char[kTrigTextSize];
    absl::SNPrintF(trigtext[i], kTrigTextSize, "%s\t%s\t%s\t",
                   Triggers.Ptr(i)->Get_Name(),
                   TriggerClass::Name_From_Event(Triggers.Ptr(i)->Event),
                   TriggerClass::Name_From_Action(Triggers.Ptr(i)->Action));

    /*
    ......................... Add on the house ID .........................
    */
    if (TriggerClass::Event_Need_House(Triggers.Ptr(i)->Event)) {
      if (Triggers.Ptr(i)->House != HOUSE_NONE) {
        port::SafeAppend(
            trigtext[i],
            HouseTypeClass::As_Reference(Triggers.Ptr(i)->House).Suffix,
            kTrigTextSize);
      } else {
        port::SafeAppend(trigtext[i], "!!!", kTrigTextSize);
      }
    } else {
      port::SafeAppend(trigtext[i], "   ", kTrigTextSize);
    }

    /*
    .......................... Add the team name ..........................
    */
    port::SafeAppend(trigtext[i], "\t", kTrigTextSize);
    if (TriggerClass::Action_Need_Team(Triggers.Ptr(i)->Action)) {
      if (Triggers.Ptr(i)->Team) {
        port::SafeAppend(trigtext[i], Triggers.Ptr(i)->Team->IniName,
                         kTrigTextSize);
      } else {
        port::SafeAppend(trigtext[i], "!!!", kTrigTextSize);
      }
    }

    /*
    ................. Set def_idx if this is CurTrigger ...................
    */
    if (Triggers.Ptr(i) == CurTrigger) {
      def_idx = i;
    }
  }

  /*
  .......................... Fill in the list box ..........................
  */
  for (int i = 0; i < Triggers.Count(); i++) {
    triggerlist.Add_Item(trigtext[i]);
  }
  triggerlist.Set_Selected_Index(def_idx);

  /*
  ....................... Set CurTrigger if it isn't .......................
  */
  if (Triggers.Count() == 0) {
    CurTrigger = nullptr;
  } else {
    if (!CurTrigger) {
      CurTrigger = Triggers.Ptr(def_idx);
    }
  }

  /*
  ............................ Create the list .............................
  */
  commands = &triggerlist;
  editbtn.Add_Tail(*commands);
  newbtn.Add_Tail(*commands);
  deletebtn.Add_Tail(*commands);
  okbtn.Add_Tail(*commands);

  /*
  ------------------------ Init tab stops for list -------------------------
  */
  triggerlist.Set_Tabs(tabs);

  /*
  -------------------------- Main Processing Loop --------------------------
  */
  RedrawType display = REDRAW_ALL;  // requested redraw level
  bool process = true;              // loop while true
  while (process) {
    /*
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      display = REDRAW_ALL;
    }

    /*
    ........................ Invoke game callback .........................
    */
    Call_Back();

    /*
    ...................... Refresh display if needed ......................
    */
    if (display != REDRAW_NONE) {
      /*
      ...................... Display the dialog box ......................
      */
      Hide_Mouse();
      if (display >= REDRAW_BACKGROUND) {
        Dialog_Box(kDialogX, kDialogY, kDialogW, kDialogH);
        Draw_Caption(TXT_NONE, kDialogX, kDialogY, kDialogW);

        /*
        ....................... Draw the captions .......................
        */
        Fancy_Text_Print(
            "Triggers", kDialogCx, kDialogY + kMargin, kCcGreen, kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
      }

      /*
      ........................ Redraw the buttons ........................
      */
      if (display >= REDRAW_BUTTONS) {
        commands->Draw_All();
      }
      Show_Mouse();
      display = REDRAW_NONE;
    }

    /*
    ........................... Get user input ............................
    */
    const KeyNumType input = commands->Input();  // user input

    /*
    ............................ Process input ............................
    */
    switch (static_cast<int>(input)) {
      case ButtonKey(kTriggerList):
        def_idx = triggerlist.Current_Index();
        if (def_idx < Triggers.Count()) {
          CurTrigger = Triggers.Ptr(def_idx);
        }
        break;

      case ButtonKey(kButtonEdit):
        if (CurTrigger) {  // only allow if there's one selected
          process = false;
          edit_trig = true;
        }
        break;

      case ButtonKey(kButtonNew):
        process = false;
        new_trig = true;
        break;

      case ButtonKey(kButtonDelete):
        process = false;
        del_trig = true;
        break;

      case KN_RETURN:
      case ButtonKey(kButtonOk):
        process = false;
        break;
      default:
        break;
    }
  }

  /*
  --------------------------- Redraw the display ---------------------------
  */
  HiddenPage.Clear();
  Flag_To_Redraw(true);
  Render();

  for (int i = 0; i < Triggers.Count(); i++) {
    delete[] trigtext[i];
  }

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

/***************************************************************************
 * MapEditClass::Edit_Trigger -- lets user edit a [new] trigger            *
 *                                                                         *
 * Ŀ *
 *                            Trigger Editor                             *
 *                                                                       *
 *              Events                              Actions              *
 *   Ŀ        Ŀ   *
 *                                                               *
 *                           Ĵ                                 Ĵ   *
 *                                                                 *
 *                                                                 *
 *                           Ĵ                                 Ĵ   *
 *                                                               *
 *              *
 *                                                                       *
 *            Name: _______                   [  Volatile    ]           *
 *                                [GDI]       [  Persistent  ]           *
 *  Time / Credits: _______       [NOD]       [SemiPersistent]           *
 *                                                                       *
 *           [Team] Team_Name                                            *
 *                                                                       *
 *                          [OK]        [Cancel]                         *
 *  *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      0 = OK, -1 = cancel                                                *
 *                                                                         *
 * WARNINGS:                                                               *
 *      CurTrigger must NOT be NULL when this function is called.          *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/29/1994 BR : Created.                                              *
 *=========================================================================*/
int MapEditClass::Edit_Trigger() {
  /*........................................................................
  Dialog & button dimensions
  ........................................................................*/
  constexpr int kDialogW = 528;
  constexpr int kDialogH = 376;
  constexpr int kDialogX = ((640 - kDialogW) / 2);
  constexpr int kDialogY = ((400 - kDialogH) / 2);
  constexpr int kDialogCx = kDialogX + (kDialogW / 2);
  constexpr int kTxt8H = 22;
  constexpr int kMargin = 14;
  constexpr int kEventW = 240;
  constexpr int kEventH = 88;
  constexpr int kEventX = kDialogX + kMargin;
  constexpr int kEventY = kDialogY + kMargin + kTxt8H + kMargin + kTxt8H;
  constexpr int kActionW = 240;
  constexpr int kActionH = 88;
  constexpr int kActionX = kDialogX + kDialogW - kMargin - kActionW;
  constexpr int kActionY = kDialogY + kMargin + kTxt8H + kMargin + kTxt8H;
  constexpr int kNameW = 80;
  constexpr int kNameH = 18;
  constexpr int kNameX = kEventX + (kEventW / 2) - 10;
  constexpr int kNameY = kEventY + kEventH + kMargin;
  constexpr int kDataW = 80;
  constexpr int kDataH = 18;
  constexpr int kDataX = kNameX;
  constexpr int kDataY = kNameY + kNameH + kMargin;
  constexpr int kTeamW = 80;
  constexpr int kTeamH = 18;
  constexpr int kTeamX = kNameX - kTeamW - 5;
  constexpr int kTeamY = kDataY + kDataH + kMargin;
  constexpr int kGdiW = 90;
  constexpr int kGdiH = 18;
  constexpr int kGdiX = kDialogCx - (kGdiW / 2);
  constexpr int kGdiY = kNameY;
  constexpr int kNodW = 90;
  constexpr int kNodH = 18;
  constexpr int kNodX = kGdiX;
  constexpr int kNodY = kGdiY + kGdiH;
  constexpr int kNeuW = 90;
  constexpr int kNeuH = 18;
  constexpr int kNeuX = kNodX;
  constexpr int kNeuY = kNodY + kNodH;
  constexpr int kMulti1W = 44;
  constexpr int kMulti1H = 18;
  constexpr int kMulti1X = kGdiX;
  constexpr int kMulti1Y = kGdiY;
  constexpr int kMulti2W = 44;
  constexpr int kMulti2H = 18;
  constexpr int kMulti2X = kGdiX + kMulti1W;
  constexpr int kMulti2Y = kGdiY;
  constexpr int kMulti3W = 44;
  constexpr int kMulti3H = 18;
  constexpr int kMulti3X = kNodX;
  constexpr int kMulti3Y = kNodY;
  constexpr int kMulti4W = 44;
  constexpr int kMulti4H = 18;
  constexpr int kMulti4X = kNodX + kMulti1W;
  constexpr int kMulti4Y = kNodY;
  constexpr int kVolatileW = 100;
  constexpr int kVolatileH = 18;
  constexpr int kVolatileX = kActionX + (kActionW / 2) - (kVolatileW / 2) + 10;
  constexpr int kVolatileY = kNameY;
  constexpr int kPersistW = 100;
  constexpr int kPersistH = 18;
  constexpr int kPersistX = kActionX + (kActionW / 2) - (kPersistW / 2) + 10;
  constexpr int kPersistY = kVolatileY + kVolatileH;
  constexpr int kSemipersistW = 100;
  constexpr int kSemipersistH = 18;
  constexpr int kSemipersistX =
      kActionX + (kActionW / 2) - (kSemipersistW / 2) + 10;
  constexpr int kSemipersistY = kPersistY + kPersistH;
  constexpr int kOkW = 90;
  constexpr int kOkH = 18;
  constexpr int kOkX = kDialogCx - 5 - kOkW;
  constexpr int kOkY = kDialogY + kDialogH - kMargin - kOkH;
  constexpr int kCancelW = 90;
  constexpr int kCancelH = 18;
  constexpr int kCancelX = kDialogCx + 5;
  constexpr int kCancelY = kDialogY + kDialogH - kMargin - kCancelH;

  /*........................................................................
  Button enumerations:
  ........................................................................*/
  constexpr int kEventList = 100;
  constexpr int kActionList = 101;
  constexpr int kNameEdit = 102;
  constexpr int kDataEdit = 103;
  constexpr int kButtonTeam = 104;
  constexpr int kButtonGdi = 105;
  constexpr int kButtonNod = 106;
  constexpr int kButtonNeutral = 107;
  constexpr int kButtonMulti1 = 109;
  constexpr int kButtonMulti2 = 110;
  constexpr int kButtonMulti3 = 111;
  constexpr int kButtonMulti4 = 112;
  constexpr int kButtonMulti5 = 113;
  constexpr int kButtonMulti6 = 114;
  constexpr int kButtonVolatile = 115;
  constexpr int kButtonPersist = 116;
  constexpr int kButtonSemipersist = 117;
  constexpr int kButtonOk = 118;
  constexpr int kButtonCancel = 119;

  /*........................................................................
  Redraw values: in order from "top" to "bottom" layer of the dialog
  ........................................................................*/
  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_BUTTONS = 1,
    REDRAW_BACKGROUND = 2,
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;

  /*........................................................................
  Dialog variables:
  ........................................................................*/
  bool cancel = false;                      // true = user cancels
  char namebuf[5];                          // name of this trigger
  char databuf[10];                         // for credit/time-based triggers
  const char* eventnames[static_cast<int>(EVENT_COUNT) + 1];  // names of events
  const char* actionnames[static_cast<int>(TriggerClass::ACTION_COUNT) +
                          1];  // names of actions

  /*........................................................................
  These flags enable various controls for each EventType.
  ........................................................................*/
  //	static char data_enabled[EVENT_COUNT] = {0,0,0,0,0,0,0,0,0,1,1,1,1,0,0};
  //	static char house_enabled[EVENT_COUNT] =
  //{1,0,0,0,0,1,1,1,1,1,1,1,1,1,1}; 	static char
  // team_enabled[TriggerClass::ACTION_COUNT] = {0,0,0,1,1,0,1,0,0,0,0,0,0,0};

  /*........................................................................
  Buttons
  ........................................................................*/
  ControlClass* commands = nullptr;  // the button list

  ListClass eventlist(kEventList, kEventX, kEventY, kEventW, kEventH,
                      TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                      Hires_Retrieve("BTN-UP.SHP"),
                      Hires_Retrieve("BTN-DN.SHP"));

  ListClass actionlist(kActionList, kActionX, kActionY, kActionW, kActionH,
                       TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                       Hires_Retrieve("BTN-UP.SHP"),
                       Hires_Retrieve("BTN-DN.SHP"));

  EditClass name_edt(kNameEdit, namebuf, 5,
                     TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kNameX,
                     kNameY, kNameW, kNameH, EditClass::ALPHANUMERIC);

  EditClass data_edt(kDataEdit, databuf, 8,
                     TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kDataX,
                     kDataY, kDataW, kDataH, EditClass::ALPHANUMERIC);

  TextButtonClass teambtn(
      kButtonTeam, "Team",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kTeamX,
      kTeamY, kTeamW, kTeamH);

  TextButtonClass gdibtn(
      kButtonGdi, "GDI",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kGdiX, kGdiY,
      kGdiW, kGdiH);

  TextButtonClass nodbtn(
      kButtonNod, "NOD",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kNodX, kNodY,
      kNodW, kNodH);

  TextButtonClass neutralbtn(
      kButtonNeutral, "Neutral",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kNeuX, kNeuY,
      kNeuW, kNeuH);

  const TextButtonClass multi1btn(
      kButtonMulti1, "M1",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kMulti1X,
      kMulti1Y, kMulti1W, kMulti1H);

  const TextButtonClass multi2btn(
      kButtonMulti2, "M2",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kMulti2X,
      kMulti2Y, kMulti2W, kMulti2H);

  const TextButtonClass multi3btn(
      kButtonMulti3, "M3",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kMulti3X,
      kMulti3Y, kMulti3W, kMulti3H);

  const TextButtonClass multi4btn(
      kButtonMulti4, "M4",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kMulti4X,
      kMulti4Y, kMulti4W, kMulti4H);

  TextButtonClass volatilebtn(
      kButtonVolatile, "Volatile",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kVolatileX,
      kVolatileY, kVolatileW, kVolatileH);

  TextButtonClass persistbtn(
      kButtonPersist, "Persistant",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kPersistX,
      kPersistY, kPersistW, kPersistH);

  TextButtonClass semipersistbtn(
      kButtonSemipersist, "SemiPersistant",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
      kSemipersistX, kSemipersistY, kSemipersistW, kSemipersistH);

  TextButtonClass okbtn(
      kButtonOk, TXT_OK,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kOkX, kOkY,
      kOkW, kOkH);

  TextButtonClass cancelbtn(
      kButtonCancel, TXT_CANCEL,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kCancelX,
      kCancelY, kCancelW, kCancelH);

  /*
  ------------------------------- Initialize -------------------------------
  */
  Set_Logic_Page(SeenBuff);

  /*
  ....................... Set default button states ........................
  */
  EventType event_idx =
      CurTrigger->Event;  // index for event list  // event list
  if (event_idx == EVENT_NONE) {
    event_idx = EVENT_PLAYER_ENTERED;
  }

  TriggerClass::ActionType action_idx =
      CurTrigger->Action;  // index for action list  // action list
  if (action_idx == TriggerClass::ACTION_NONE) {
    action_idx = TriggerClass::ACTION_WIN;
  }

  port::SafeCopy(namebuf, CurTrigger->Get_Name());  // Name
  name_edt.Set_Text(namebuf, 5);

  if (TriggerClass::Event_Need_Data(event_idx)) {
    absl::SNPrintF(databuf, sizeof(databuf), "%ld",
                   CurTrigger->Data);  // Credits/Time
    data_edt.Set_Text(databuf, 8);
  }

  HousesType house = CurTrigger->House;  // house for this trigger  // House

  TriggerClass::PersistantType persistant =
      CurTrigger->IsPersistant;  // trigger's persistence level

  volatilebtn.Turn_Off();
  persistbtn.Turn_Off();
  semipersistbtn.Turn_Off();
  switch (CurTrigger->IsPersistant) {
    case TriggerClass::VOLATILE:
      volatilebtn.Turn_On();
      break;

    case TriggerClass::SEMIPERSISTANT:
      semipersistbtn.Turn_On();
      break;

    case TriggerClass::PERSISTANT:
      persistbtn.Turn_On();
      break;
    default:
      break;
  }

  /*
  ......................... Fill in the list boxes .........................
  */
  for (int i = 0; i < static_cast<int>(EVENT_COUNT); i++) {
    eventnames[i] = TriggerClass::Name_From_Event(static_cast<EventType>(i));
    eventlist.Add_Item(eventnames[i]);
  }
  eventlist.Set_Selected_Index(static_cast<int>(event_idx));

  for (int i = 0; i < static_cast<int>(TriggerClass::ACTION_COUNT); i++) {
    actionnames[i] = TriggerClass::Name_From_Action(
        static_cast<TriggerClass::ActionType>(i));
    actionlist.Add_Item(actionnames[i]);
  }
  actionlist.Set_Selected_Index(static_cast<int>(action_idx));

  /*
  -------------------------- Main Processing Loop --------------------------
  */
  RedrawType display = REDRAW_ALL;  // requested redraw level
  bool process = true;              // loop while true
  while (process) {
    /*
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      display = REDRAW_ALL;
    }

    /*
    ........................ Invoke game callback .........................
    */
    Call_Back();

    /*
    ...................... Refresh display if needed ......................
    */
    if (display != REDRAW_NONE) {
      /*
      ...................... Display the dialog box ......................
      */
      Hide_Mouse();
      if (display >= REDRAW_BACKGROUND) {
        Dialog_Box(kDialogX, kDialogY, kDialogW, kDialogH);
        Draw_Caption(TXT_NONE, kDialogX, kDialogY, kDialogW);

        /*
        ....................... Draw the captions .......................
        */
        Fancy_Text_Print(
            "Trigger Editor", kDialogCx, kDialogY + kMargin, kCcGreen, kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        Fancy_Text_Print(
            "Events", kEventX + (kEventW / 2), kEventY - kTxt8H, kCcGreen,
            kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        Fancy_Text_Print(
            "Actions", kActionX + (kActionW / 2), kActionY - kTxt8H, kCcGreen,
            kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        Fancy_Text_Print(
            "Name", kNameX - 5, kNameY, kCcGreen, kTBlack,
            TPF_RIGHT | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        if (event_idx == EVENT_CREDITS) {  // use 'Data' for Credits
          Fancy_Text_Print(
              "Credits", kDataX - 5, kDataY, kCcGreen, kTBlack,
              TPF_RIGHT | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        } else {
          if (event_idx == EVENT_TIME) {  // use 'Data' for Time
            Fancy_Text_Print(
                "1/10 Min", kDataX - 5, kDataY, kCcGreen, kTBlack,
                TPF_RIGHT | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
          }
        }

        if (TriggerClass::Action_Need_Team(action_idx)) {
          if (CurTrigger->Team) {
            Fancy_Text_Print(CurTrigger->Team->IniName, kTeamX + kTeamW + 5,
                             kTeamY, kCcGreen, kTBlack,
                             TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
          } else {
            Fancy_Text_Print("!!!", kTeamX + kTeamW + 5, kTeamY, kCcGreen,
                             kTBlack,
                             TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
          }
        }
      }

      /*
      ..................... Rebuild the button list ......................
      */
      eventlist.Zap();
      actionlist.Zap();
      name_edt.Zap();
      data_edt.Zap();
      teambtn.Zap();
      gdibtn.Zap();
      nodbtn.Zap();
      neutralbtn.Zap();
      volatilebtn.Zap();
      persistbtn.Zap();
      semipersistbtn.Zap();
      okbtn.Zap();
      cancelbtn.Zap();

      commands = &okbtn;
      cancelbtn.Add_Tail(*commands);
      eventlist.Add_Tail(*commands);
      actionlist.Add_Tail(*commands);
      name_edt.Add_Tail(*commands);
      volatilebtn.Add_Tail(*commands);
      persistbtn.Add_Tail(*commands);
      semipersistbtn.Add_Tail(*commands);
      if (TriggerClass::Event_Need_Data(event_idx)) {
        data_edt.Add_Tail(*commands);
        absl::SNPrintF(databuf, sizeof(databuf), "%ld", CurTrigger->Data);
        data_edt.Set_Text(databuf, 8);
      }
      if (TriggerClass::Event_Need_House(event_idx)) {
        gdibtn.Add_Tail(*commands);
        nodbtn.Add_Tail(*commands);
        neutralbtn.Add_Tail(*commands);
        Set_House_Buttons(house, commands, kButtonGdi);
      }
      if (TriggerClass::Action_Need_Team(action_idx)) {
        teambtn.Add_Tail(*commands);
      }

      /*
      ........................ Redraw the buttons ........................
      */
      if (display >= REDRAW_BUTTONS) {
        commands->Flag_List_To_Redraw();
      }
      Show_Mouse();
      display = REDRAW_NONE;
    }

    /*
    ........................... Get user input ............................
    */
    const KeyNumType input = commands->Input();  // user input

    /*
    ............................ Process input ............................
    */
    switch (static_cast<int>(input)) {
      case ButtonKey(kEventList):
        if (eventlist.Current_Index() != static_cast<int>(event_idx)) {
          event_idx = EventType(eventlist.Current_Index());
          databuf[0] = 0;
          CurTrigger->Data = 0;
          if (!TriggerClass::Event_Need_House(event_idx)) {
            CurTrigger->House = HOUSE_NONE;
          }
          display = REDRAW_ALL;
        }
        break;

      case ButtonKey(kActionList):
        if (actionlist.Current_Index() != static_cast<int>(action_idx)) {
          action_idx = TriggerClass::ActionType(actionlist.Current_Index());
          display = REDRAW_ALL;
        }
        break;

      case ButtonKey(kNameEdit):
      case ButtonKey(kDataEdit):
        break;

      case ButtonKey(kButtonGdi):
      case ButtonKey(kButtonNod):
      case ButtonKey(kButtonNeutral):
      case ButtonKey(kButtonMulti1):
      case ButtonKey(kButtonMulti2):
      case ButtonKey(kButtonMulti3):
      case ButtonKey(kButtonMulti4):
      case ButtonKey(kButtonMulti5):
      case ButtonKey(kButtonMulti6):
        house = static_cast<HousesType>(static_cast<int>(input & ~KN_BUTTON) -
                                        kButtonGdi);
        Set_House_Buttons(house, commands, kButtonGdi);
        break;

      case ButtonKey(kButtonTeam):
        Handle_Teams("Select a Team");
        if (CurTeam) {
          CurTrigger->Team = CurTeam;
        }
        HiddenPage.Clear();
        Flag_To_Redraw(true);
        Render();
        display = REDRAW_ALL;
        break;

      case ButtonKey(kButtonVolatile):
        persistant = TriggerClass::VOLATILE;
        volatilebtn.Turn_On();
        persistbtn.Turn_Off();
        semipersistbtn.Turn_Off();
        break;

      case ButtonKey(kButtonPersist):
        persistant = TriggerClass::PERSISTANT;
        volatilebtn.Turn_Off();
        persistbtn.Turn_On();
        semipersistbtn.Turn_Off();
        break;

      case ButtonKey(kButtonSemipersist):
        persistant = TriggerClass::SEMIPERSISTANT;
        volatilebtn.Turn_Off();
        persistbtn.Turn_Off();
        semipersistbtn.Turn_On();
        break;

      case KN_RETURN:
      case ButtonKey(kButtonOk):
        process = false;
        break;

      case KN_ESC:
      case ButtonKey(kButtonCancel):
        cancel = true;
        process = false;
        break;

      default:
        break;
    }
  }

  /*
  ------------------------------ Save values -------------------------------
  */
  if (!cancel) {
    /*
    .......................... Get list indices ...........................
    */
    event_idx = EventType(eventlist.Current_Index());
    action_idx = TriggerClass::ActionType(actionlist.Current_Index());

    /*
    ......................... Set Event & Action ..........................
    */
    CurTrigger->Event = EventType(event_idx);
    CurTrigger->Action = TriggerClass::ActionType(action_idx);

    /*
    .............................. Set name ...............................
    */
    if (strlen(namebuf) == 0) {
      CurTrigger->Set_Name("____");
    } else {
      CurTrigger->Set_Name(namebuf);
    }

    /*
    .............................. Set Data ...............................
    */
    if (TriggerClass::Event_Need_Data(event_idx)) {
      CurTrigger->Data = tech::ParseInteger<int64_t>(databuf).value_or(0);
    }

    /*
    .............................. Set House ..............................
    */
    if (TriggerClass::Event_Need_House(event_idx)) {
      CurTrigger->House = house;
    } else {
      CurTrigger->House = HOUSE_NONE;
    }

    /*
    ........................... Set Persistence  ..........................
    */
    CurTrigger->IsPersistant = persistant;
  }

  /*
  --------------------------- Redraw the display ---------------------------
  */
  HiddenPage.Clear();
  Flag_To_Redraw(true);
  Render();

  if (cancel) {
    return (-1);
  }
  return 0;
}

/***************************************************************************
 * MapEditClass::Import_Triggers -- lets user import triggers              *
 *                                                                         *
 *    Ŀ                  *
 *                        Triggers                                       *
 *        Ŀ                     *
 *        x Name     Event     Action    House                       *
 *          Name     Event     Action    House  Ĵ                     *
 *        x Name     Event     Action    House                        *
 *          Name     Event     Action    House                        *
 *                                                                    *
 *                                                                    *
 *                                              Ĵ                     *
 *                                                                   *
 *                             *
 *                                                                       *
 *                    [OK]     [Cancel]                                  *
 *                                                                       *
 *                      *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      0 = OK, -1 = user cancelled                                        *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   03/29/1995 BRR : Created.                                             *
 *=========================================================================*/
int MapEditClass::Import_Triggers() {
  /*........................................................................
  Dialog & button dimensions
  ........................................................................*/
  constexpr int kDialogW = 480;
  constexpr int kDialogH = 290;
  constexpr int kDialogX = ((640 - kDialogW) / 2);
  constexpr int kDialogY = ((400 - kDialogH) / 2);
  constexpr int kDialogCx = kDialogX + (kDialogW / 2);
  constexpr int kTxt8H = 22;
  constexpr int kMargin = 14;
  constexpr int kListW = 452;
  constexpr int kListH = 208;
  constexpr int kListX = kDialogX + kMargin;
  constexpr int kListY = kDialogY + kMargin + kTxt8H;
  constexpr int kOkW = 90;
  constexpr int kOkH = 18;
  constexpr int kOkX = kDialogCx - kOkW - 5;
  constexpr int kOkY = kDialogY + kDialogH - kMargin - kOkH;
  constexpr int kCancelW = 90;
  constexpr int kCancelH = 18;
  constexpr int kCancelX = kDialogCx + 5;
  constexpr int kCancelY = kDialogY + kDialogH - kMargin - kOkH;
  /*........................................................................
  Button enumerations:
  ........................................................................*/
  constexpr int kTriggerList = 100;
  constexpr int kButtonOk = 101;
  constexpr int kButtonCancel = 102;
  /*........................................................................
  Redraw values: in order from "top" to "bottom" layer of the dialog
  ........................................................................*/
  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_BUTTONS = 1,
    REDRAW_BACKGROUND = 2,
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;
  /*........................................................................
  Dialog variables:
  ........................................................................*/
  bool cancel = false;
  static int tabs[] = {70, 220, 370, 420};  // list box tab stops
  DynamicVectorClass<char*> trignames;      // list of INI trigger names
  GameFile file;                            // file for reading the INI file
  char buf[128];                            // for reading an INI entry
  constexpr int kItemSize = 60;
  char item[kItemSize];  // for adding to list box
  /*........................................................................
  Buttons
  ........................................................................*/
  ControlClass* commands = nullptr;  // the button list

  CheckListClass triggerlist(kTriggerList, kListX, kListY, kListW, kListH,
                             TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                             Hires_Retrieve("BTN-UP.SHP"),
                             Hires_Retrieve("BTN-DN.SHP"));

  TextButtonClass okbtn(
      kButtonOk, TXT_OK,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kOkX, kOkY,
      kOkW, kOkH);

  TextButtonClass cancelbtn(
      kButtonCancel, TXT_CANCEL,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kCancelX,
      kCancelY, kCancelW, kCancelH);

  Set_Logic_Page(SeenBuff);

  /*------------------------------------------------------------------------
  Read the MASTER.INI file
  ------------------------------------------------------------------------*/
  /*........................................................................
  Read the file into the staging buffer
  ........................................................................*/
  char* inibuf = new char[30000];  // working INI buffer
  memset(inibuf, '\0', 30000);
  file.SetName("MASTER.INI");
  if (!file.IsAvailable()) {
    file.Close();
    delete[] inibuf;
    return (-1);
  }
  file.Read(inibuf, 30000 - 1);
  file.Close();

  /*........................................................................
  Read all entry names in the Triggers section into a temp buffer
  ........................................................................*/
  const int len =
      static_cast<int>(strlen(inibuf)) + 2;  // Length of data in buffer.
  char* tbuffer = inibuf + len;  // Accumulation buffer of trigger IDs.
  WWGetPrivateProfileString(TriggerClass::INI_Name(), nullptr, nullptr, tbuffer,
                            30000 - len, inibuf);

  /*........................................................................
  For each entry in the INI section:
  - Get the entry
  - Generate a string describing the trigger
  - Add that string to the list box
  - Add a ptr to the INI entry name to our 'trignames' list
  ........................................................................*/
  while (*tbuffer != '\0') {
    WWGetPrivateProfileString(TriggerClass::INI_Name(), tbuffer, nullptr, buf,
                              sizeof(buf) - 1, inibuf);

    /*
    ** Parse the INI entry
    */
    port::Tokenizer tokens(buf, ",");
    char* eventptr = tokens.Next();
    char* actionptr = tokens.Next();
    tokens.Next();  // data, unused
    char* houseptr = tokens.Next();

    /*
    ** Generate the descriptive string
    */
    absl::SNPrintF(item, sizeof(item), " %s\t%s\t%s\t", tbuffer, eventptr,
                   actionptr);

    /*
    ** Add house name if needed
    */
    if (TriggerClass::Event_Need_House(
            TriggerClass::Event_From_Name(eventptr))) {
      const HousesType house = HouseTypeClass::From_Name(houseptr);
      if (house != HOUSE_NONE) {
        port::SafeAppend(item, HouseTypeClass::As_Reference(house).Suffix,
                         kItemSize);
      } else {
        port::SafeAppend(item, "!!!", kItemSize);
      }
    } else {
      port::SafeAppend(item, "   ", kItemSize);
    }

    /*
    ** Add the item to the list box
    */
    triggerlist.Add_Item(item);

    /*
    ** Add the name to our internal name list
    */
    trignames.Add(tbuffer);

    tbuffer += strlen(tbuffer) + 1;
  }

  /*
  ............................ Create the list .............................
  */
  commands = &triggerlist;
  okbtn.Add_Tail(*commands);
  cancelbtn.Add_Tail(*commands);

  /*
  ------------------------ Init tab stops for list -------------------------
  */
  triggerlist.Set_Tabs(tabs);

  /*
  -------------------------- Main Processing Loop --------------------------
  */
  RedrawType display = REDRAW_ALL;  // requested redraw level
  bool process = true;              // loop while true
  while (process) {
    /*
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      display = REDRAW_ALL;
    }

    /*
    ........................ Invoke game callback .........................
    */
    Call_Back();
    /*
    ...................... Refresh display if needed ......................
    */
    if (display != REDRAW_NONE) {
      /*
      ...................... Display the dialog box ......................
      */
      Hide_Mouse();
      if (display >= REDRAW_BACKGROUND) {
        Dialog_Box(kDialogX, kDialogY, kDialogW, kDialogH);
        Draw_Caption(TXT_NONE, kDialogX, kDialogY, kDialogW);
        /*
        ....................... Draw the captions .......................
        */
        Fancy_Text_Print(
            "Import Triggers", kDialogCx, kDialogY + kMargin, kCcGreen, kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
      }
      /*
      ........................ Redraw the buttons ........................
      */
      if (display >= REDRAW_BUTTONS) {
        commands->Flag_List_To_Redraw();
      }
      Show_Mouse();
      display = REDRAW_NONE;
    }

    /*
    ........................... Get user input ............................
    */
    const KeyNumType input = commands->Input();  // user input

    /*
    ............................ Process input ............................
    */
    switch (static_cast<int>(input)) {
      case ButtonKey(kTriggerList):
        break;

      case KN_RETURN:
      case ButtonKey(kButtonOk):
        process = false;
        break;

      case KN_ESC:
      case ButtonKey(kButtonCancel):
        cancel = true;
        process = false;
        break;
      default:
        break;
    }
  }

  /*
  --------------------------- Redraw the display ---------------------------
  */
  HiddenPage.Clear();
  Flag_To_Redraw(true);
  Render();

  /*........................................................................
  Re-parse the INI section; if any item is checked in the list box, create
  that trigger for this scenario.
  ........................................................................*/
  if (!cancel) {
    tbuffer = inibuf + len;
    int i = 0;
    while (*tbuffer != '\0') {
      /*
      ** If this item is checked on the list, create a new trigger
      ** and fill it in.
      */
      if (triggerlist.Is_Checked(i)) {
        WWGetPrivateProfileString(TriggerClass::INI_Name(), tbuffer, nullptr,
                                  buf, sizeof(buf) - 1, inibuf);

        auto* trigger = new TriggerClass();  // Working trigger pointer.
        trigger->Fill_In(tbuffer, buf);

        if (trigger->House != HOUSE_NONE) {
          HouseTriggers[trigger->House].Add(trigger);
        }
      }

      tbuffer += strlen(tbuffer) + 1;
      i++;
    }
  }

  /*........................................................................
  Clean up memory
  ........................................................................*/
  trignames.Clear();
  triggerlist.Clear();
  delete[] inibuf;

  if (cancel) {
    return (-1);
  }
  return 0;
}

/***************************************************************************
 * MapEditClass::Import_Teams -- lets the user import teams                *
 *                                                                         *
 *    Ŀ           *
 *                             Teams                                     *
 *        Ŀ               *
 *         Name     House    Class:Count,Class:Count                 *
 *         Name     House    Class:Count,Class:Count  Ĵ               *
 *         Name     House    Class:Count,Class:Count                  *
 *         Name     House    Class:Count,Class:Count                  *
 *                                                                    *
 *                                                                    *
 *                                                    Ĵ               *
 *                                                                   *
 *                       *
 *                                                                       *
 *                        [OK]    [Cancel]                               *
 *                                                                       *
 *               *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      0 = OK, -1 = user cancelled                                        *
 *                                                                         *
 * WARNINGS:                                                               *
 *      Uses HIDBUFF.                                                      *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/08/1994 BR : Created.                                              *
 *=========================================================================*/
int MapEditClass::Import_Teams() {
  /*........................................................................
  Dialog & button dimensions
  ........................................................................*/
  constexpr int kDialogW = 528;
  constexpr int kDialogH = 290;
  constexpr int kDialogX = ((640 - kDialogW) / 2);
  constexpr int kDialogY = ((400 - kDialogH) / 2);
  constexpr int kDialogCx = kDialogX + (kDialogW / 2);
  constexpr int kTxt8H = 22;
  constexpr int kMargin = 14;
  constexpr int kListW = 500;
  constexpr int kListH = 208;
  constexpr int kListX = kDialogX + kMargin;
  constexpr int kListY = kDialogY + kMargin + kTxt8H;
  constexpr int kOkW = 90;
  constexpr int kOkH = 18;
  constexpr int kOkX = kDialogCx - kOkW - 5;
  constexpr int kOkY = kDialogY + kDialogH - kMargin - kOkH;
  constexpr int kCancelW = 90;
  constexpr int kCancelH = 18;
  constexpr int kCancelX = kDialogCx + 5;
  constexpr int kCancelY = kDialogY + kDialogH - kMargin - kOkH;
  /*........................................................................
  Button enumerations:
  ........................................................................*/
  constexpr int kTeamList = 100;
  constexpr int kButtonOk = 101;
  constexpr int kButtonCancel = 102;
  /*........................................................................
  Redraw values: in order from "top" to "bottom" layer of the dialog
  ........................................................................*/
  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_BUTTONS = 1,
    REDRAW_BACKGROUND = 2,
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;
  /*........................................................................
  Dialog variables:
  ........................................................................*/
  bool cancel = false;
  static int tabs[] = {120, 180};       // list box tab stops
  DynamicVectorClass<char*> teamnames;  // list of INI team names
  GameFile file;                        // file for reading the INI file
  char buf[128];                        // for reading an INI entry
  constexpr int kItemSize = 60;
  char item[kItemSize];  // for adding to list box
  int i = 0;
  /*........................................................................
  Buttons
  ........................................................................*/
  ControlClass* commands = nullptr;  // the button list

  CheckListClass teamlist(kTeamList, kListX, kListY, kListW, kListH,
                          TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                          Hires_Retrieve("BTN-UP.SHP"),
                          Hires_Retrieve("BTN-DN.SHP"));

  TextButtonClass okbtn(
      kButtonOk, TXT_OK,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kOkX, kOkY,
      kOkW, kOkH);

  TextButtonClass cancelbtn(
      kButtonCancel, TXT_CANCEL,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kCancelX,
      kCancelY, kCancelW, kCancelH);

  Set_Logic_Page(SeenBuff);

  /*------------------------------------------------------------------------
  Read the MASTER.INI file
  ------------------------------------------------------------------------*/
  /*........................................................................
  Read the file into the staging buffer
  ........................................................................*/
  char* inibuf = new char[30000];  // working INI buffer
  memset(inibuf, '\0', 30000);
  file.SetName("MASTER.INI");
  if (!file.IsAvailable()) {
    file.Close();
    delete[] inibuf;
    return (-1);
  }
  file.Read(inibuf, 30000 - 1);

  file.Close();
  /*........................................................................
  Read all entry names in the TeamTypes section into a temp buffer
  ........................................................................*/
  const int len =
      static_cast<int>(strlen(inibuf)) + 2;  // Length of data in buffer.
  char* tbuffer = inibuf + len;              // Accumulation buffer of team IDs.
  WWGetPrivateProfileString(TeamTypeClass::INI_Name(), nullptr, nullptr,
                            tbuffer, 30000 - len, inibuf);

  /*........................................................................
  For each entry in the INI section:
  - Get the entry
  - Generate a string describing the team
  - Add that string to the list box
  - Add a ptr to the INI entry name to our 'teamnames' list
  ........................................................................*/
  while (*tbuffer != '\0') {
    WWGetPrivateProfileString(TeamTypeClass::INI_Name(), tbuffer, nullptr, buf,
                              sizeof(buf) - 1, inibuf);

    /*
    ** Parse the INI entry
    */
    port::Tokenizer tokens(buf, ",");
    char* houseptr = tokens.Next();
    for (i = 0; i < 9; i++) {
      tokens.Next();  // flags and counts, unused
    }
    const int numclasses = tech::ParseInteger<int>(tokens.Next()).value_or(0);

    /*
    ** Generate the descriptive string
    */
    absl::SNPrintF(item, sizeof(item), " %s\t", tbuffer);
    const HousesType house = HouseTypeClass::From_Name(houseptr);
    if (house != HOUSE_NONE) {
      port::SafeAppend(item, HouseTypeClass::As_Reference(house).Suffix,
                       kItemSize);
    } else {
      port::SafeAppend(item, "!!!", kItemSize);
    }
    port::SafeAppend(item, "\t", kItemSize);

    char* classptr = tokens.Next();
    for (i = 0; i < numclasses; i++) {
      if (strlen(item) + strlen(classptr) < kItemSize) {
        port::SafeAppend(item, classptr, kItemSize);
        classptr = tokens.Next();
      } else {
        break;
      }
    }

    /*
    ** Add the item to the list box
    */
    teamlist.Add_Item(item);
    /*
    ** Add the name to our internal name list
    */
    teamnames.Add(tbuffer);

    tbuffer += strlen(tbuffer) + 1;
  }

  /*
  ............................ Create the list .............................
  */
  commands = &teamlist;
  okbtn.Add_Tail(*commands);
  cancelbtn.Add_Tail(*commands);

  /*
  ------------------------ Init tab stops for list -------------------------
  */
  teamlist.Set_Tabs(tabs);

  /*
  -------------------------- Main Processing Loop --------------------------
  */
  RedrawType display = REDRAW_ALL;  // requested redraw level
  bool process = true;              // loop while true
  while (process) {
    /*
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      display = REDRAW_ALL;
    }

    /*
    ........................ Invoke game callback .........................
    */
    Call_Back();
    /*
    ...................... Refresh display if needed ......................
    */
    if (display != REDRAW_NONE) {
      /*
      ...................... Display the dialog box ......................
      */
      Hide_Mouse();
      if (display >= REDRAW_BACKGROUND) {
        Dialog_Box(kDialogX, kDialogY, kDialogW, kDialogH);
        Draw_Caption(TXT_NONE, kDialogX, kDialogY, kDialogW);
        /*
        ....................... Draw the captions .......................
        */
        Fancy_Text_Print(
            "Import Teams", kDialogCx, kDialogY + kMargin, kCcGreen, kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
      }
      /*
      ........................ Redraw the buttons ........................
      */
      if (display >= REDRAW_BUTTONS) {
        commands->Flag_List_To_Redraw();
      }
      Show_Mouse();
      display = REDRAW_NONE;
    }

    /*
    ........................... Get user input ............................
    */
    const KeyNumType input = commands->Input();  // user input

    /*
    ............................ Process input ............................
    */
    switch (static_cast<int>(input)) {
      case ButtonKey(kTeamList):
        break;

      case KN_RETURN:
      case ButtonKey(kButtonOk):
        process = false;
        break;

      case KN_ESC:
      case ButtonKey(kButtonCancel):
        cancel = true;
        process = false;
        break;
      default:
        break;
    }
  }

  /*
  --------------------------- Redraw the display ---------------------------
  */
  HiddenPage.Clear();
  Flag_To_Redraw(true);
  Render();

  /*........................................................................
  Re-parse the INI section; if any item is checked in the list box, create
  that team for this scenario.
  ........................................................................*/
  if (!cancel) {
    tbuffer = inibuf + len;
    i = 0;
    while (*tbuffer != '\0') {
      /*
      ** If this item is checked on the list, create a new team
      ** and fill it in.
      */
      if (teamlist.Is_Checked(i)) {
        WWGetPrivateProfileString(TeamTypeClass::INI_Name(), tbuffer, nullptr,
                                  buf, sizeof(buf) - 1, inibuf);

        auto* team = new TeamTypeClass();  // Working team pointer.
        team->Fill_In(tbuffer, buf);
      }

      tbuffer += strlen(tbuffer) + 1;
      i++;
    }
  }

  /*........................................................................
  Clean up memory
  ........................................................................*/
  teamnames.Clear();
  teamlist.Clear();
  delete[] inibuf;

  if (cancel) {
    return (-1);
  }
  return 0;
}
