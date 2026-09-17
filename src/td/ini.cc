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

/* $Header:   F:\projects\c&c\vcs\code\ini.cpv   2.18   16 Oct 1995 16:48:50
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : INI.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : September 10, 1993 *
 *                                                                                             *
 *                  Last Update : July 30, 1995 [BRR] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Assign_Houses -- Assigns multiplayer houses to various players *
 *   Clear_Flag_Spots -- Clears flag overlays off the map * Clip_Move -- moves
 *in given direction from given cell; clips to map                       *
 *   Clip_Scatter -- randomly scatters from given cell; won't fall off map *
 *   Create_Units -- Creates infantry & units, for non-base multiplayer *
 *   Furthest_Cell -- Finds cell furthest from a group of cells * Place_Flags --
 *Places flags for multiplayer games                                         *
 *   Read_Scenario_Ini -- Read specified scenario INI file. * Remove_AI_Players
 *-- Removes the computer AI houses & their units                         *
 *   Scan_Place_Object -- places an object >near< the given cell *
 *   Set_Scenario_Name -- Creates the INI scenario name string. * Sort_Cells --
 *sorts an array of cells by distance                                         *
 *   Write_Scenario_Ini -- Write the scenario INI file. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/ini.h"

#include <absl/log/check.h>

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>
#include <string_view>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "port/bytes_of.h"
#include "port/ex_string.h"
#include "port/safe_string.h"
#include "sdllib/misc.h"
#include "sdllib/shape.h"
#include "td/base.h"
#include "td/building.h"
#include "td/cell.h"
#include "td/config.h"
#include "td/conquer.h"
#include "td/defines.h"
#include "td/externs.h"
#include "td/ftimer.h"
#include "td/globals.h"
#include "td/house.h"
#include "td/infantry.h"
#include "td/inline.h"
#include "td/jshell.h"
#include "td/mapedit.h"
#include "td/mplayer.h"
#include "td/object.h"
#include "td/overlay.h"
#include "td/profile.h"
#include "td/queue.h"
#include "td/rand.h"
#include "td/randomstate.h"
#include "td/scenario.h"
#include "td/smudge.h"
#include "td/special.h"
#include "td/teamtype.h"
#include "td/techno.h"
#include "td/template.h"
#include "td/terrain.h"
#include "td/theme.h"
#include "td/trigger.h"
#include "td/type.h"
#include "td/unit.h"
#include "td/vector.h"
#include "tech/game_file.h"

/************************************* Prototypes
 * *********************************************/
static void Assign_Houses();
static void Remove_AI_Players();
static void Create_Units();
static void Sort_Cells(std::span<CELL> cells, int numcells,
                       std::span<CELL> outcells);
static int Furthest_Cell(std::span<const CELL> ref_cells, int num_ref_cells,
                         std::span<const CELL> test_cells, int num_test_cells);
static CELL Clip_Scatter(CELL cell, int maxdist);
static CELL Clip_Move(CELL cell, FacingType facing, int dist);

/***********************************************************************************************
 * Set_Scenario_Name -- Creates the INI scenario name string. *
 *                                                                                             *
 *    This routine is used by the scenario loading and saving code. It generates
 *the scenario  * INI root file name for the specified scenario parameters. *
 *                                                                                             *
 * INPUT: * buf         buffer to store filename in; must be long enough for
 *root.ext           * scenario      scenario number * player      player type
 *for this game (GDI, NOD, multi-player, ...)                   * dir
 *directional parameter for this game (East/West)                           *
 *       var         variation of this game (Lose, A/B/C/D, etc) *
 *                                                                                             *
 * OUTPUT:  none. *
 *                                                                                             *
 * WARNINGS:   none. *
 *                                                                                             *
 * HISTORY: * 05/28/1994 JLB : Created. * 05/01/1995 BRR : 2-player scenarios
 *use same names as multiplayer                         *
 *=============================================================================================*/
void Set_Scenario_Name(char* buf, int scenario, ScenarioPlayerType player,
                       ScenarioDirType dir, ScenarioVarType var) {
  // "SC" + player + two-digit number + direction + variation, plus the NUL.
  constexpr size_t kScenarioNameSize = sizeof("SCG01EA");
  char c_player = 0;  // character representing player type
  char c_dir = 0;     // character representing direction type
  char c_var = 0;     // character representing variation type
  ScenarioVarType i = SCEN_VAR_NONE;
  char fname[kMaxFname + kMaxExt];

  /*
  ** Set the player-type value.
  */
  switch (player) {
    case SCEN_PLAYER_GDI:
      c_player = HouseTypeClass::As_Reference(HOUSE_GOOD).Prefix;
      //			c_player = 'G';
      break;

    case SCEN_PLAYER_NOD:
      c_player = HouseTypeClass::As_Reference(HOUSE_BAD).Prefix;
      //			c_player = 'B';
      break;

    case SCEN_PLAYER_JP:
      c_player = HouseTypeClass::As_Reference(HOUSE_JP).Prefix;
      //			c_player = 'J';
      break;

    /*
    **	Multi player scenario.
    */
    case ScenarioPlayerType::SCEN_PLAYER_NONE:
    case ScenarioPlayerType::SCEN_PLAYER_2PLAYER:
    case ScenarioPlayerType::SCEN_PLAYER_MPLAYER:
    case ScenarioPlayerType::SCEN_PLAYER_COUNT:
    default:
      c_player = HouseTypeClass::As_Reference(HOUSE_MULTI1).Prefix;
      //			c_player = 'M';
      break;
  }

  /*
  ** Set the directional character value.
  ** If SCEN_DIR_NONE is specified, randomly pick a direction; otherwise, use
  *'E' or 'W'
  */
  switch (dir) {
    case SCEN_DIR_EAST:
      c_dir = 'E';
      break;

    case SCEN_DIR_WEST:
      c_dir = 'W';
      break;

    case ScenarioDirType::SCEN_DIR_COUNT:
    default:
    case SCEN_DIR_NONE:
      c_dir = Random_Pick(0, 1) == 0 ? 'W' : 'E';
      break;
  }

  /*
  ** Set the variation value.
  */
  if (var == SCEN_VAR_NONE) {
    /*
    ** Find which variations are available for this scenario
    */
    for (i = SCEN_VAR_A; i < SCEN_VAR_COUNT; i++) {
      absl::SNPrintF(fname, sizeof(fname), "SC%c%02d%c%c.INI", c_player,
                     scenario, c_dir, 'A' + static_cast<int>(i));
      if (!GameFile(fname).IsAvailable()) {
        break;
      }
    }

    if (i == SCEN_VAR_A) {
      c_var = 'X';  // indicates an error
    } else {
      c_var = static_cast<char>('A' + Random_Pick(0, static_cast<int>(i) - 1));
    }
  } else {
    switch (var) {
      case SCEN_VAR_A:
        c_var = 'A';
        break;

      case SCEN_VAR_B:
        c_var = 'B';
        break;

      case SCEN_VAR_C:
        c_var = 'C';
        break;

      case SCEN_VAR_D:
        c_var = 'D';
        break;

      case ScenarioVarType::SCEN_VAR_NONE:
      case ScenarioVarType::SCEN_VAR_COUNT:
      case ScenarioVarType::SCEN_VAR_LOSE:
      default:
        c_var = 'L';
        break;
    }
  }

  /*
  ** generate the filename
  */
  absl::SNPrintF(buf, kScenarioNameSize, "SC%c%02d%c%c", c_player, scenario,
                 c_dir, c_var);
}

/***********************************************************************************************
 * Read_Scenario_Ini -- Read specified scenario INI file. *
 *                                                                                             *
 *    Read in the scenario INI file. This routine only sets the game * globals
 *with that data that is explicitly defined in the INI file. * The remaining
 *necessary interpolated data is generated elsewhere.                        *
 *                                                                                             *
 * INPUT: * root      root filename for scenario file to read *
 *                                                                                             *
 *          fresh      true = should the current scenario be cleared? *
 *                                                                                             *
 * OUTPUT:  bool; Was the scenario read successful? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/07/1992 JLB : Created. *
 *=============================================================================================*/
bool Read_Scenario_Ini(const char* root, bool fresh) {
  char fname[kMaxFname + kMaxExt];    // full INI filename
  char buf[128];                      // Working string staging buffer.
  int rndmax = 0;
  int rndmin = 0;
  unsigned char val = 0;

  ScenarioInit++;

  /*
  **	Fetch working pointer to the INI staging buffer. Make sure that the
  *buffer *	is cleared out before proceeding.  (Don't use the HidPage for
  *this, since *	the HidPage may be needed for various uncompressions
  *during the INI *	parsing.)
  */
  char* buffer = ShapeBuffer;  // Scenario.ini staging buffer pointer.
  std::ranges::fill(ShapeBufferBytes, 0);

  if (fresh) {
    Clear_Scenario();
  }

  /*
  ** If we are not dealing with scenario 1, or a multi player scenario
  ** then make sure the correct disk is in the drive.
  */
  if (RequiredCD != -2) {
    if (Scenario >= 20 && Scenario < 60 && GameToPlay == GAME_NORMAL) {
      RequiredCD = 2;
    } else {
      if (Scenario != 1) {
        if (Scenario >= 60) {
          RequiredCD = -1;
        } else {
          switch (ScenPlayer) {
            case SCEN_PLAYER_GDI:
              RequiredCD = 0;
              break;
            case SCEN_PLAYER_NOD:
              RequiredCD = 1;
              break;
            case ScenarioPlayerType::SCEN_PLAYER_NONE:
            case ScenarioPlayerType::SCEN_PLAYER_JP:
            case ScenarioPlayerType::SCEN_PLAYER_2PLAYER:
            case ScenarioPlayerType::SCEN_PLAYER_MPLAYER:
            case ScenarioPlayerType::SCEN_PLAYER_COUNT:
            default:
              RequiredCD = -1;
              break;
          }
        }
      } else {
        RequiredCD = -1;
      }
    }
  }
  if (!Force_CD_Available(RequiredCD)) {
    Prog_End();
    exit(EXIT_FAILURE);
  }

  /*
  **	Create scenario filename and read the file.
  */

  absl::SNPrintF(fname, sizeof(fname), "%s.INI", root);
  GameFile file(fname);
  if (!file.IsAvailable()) {
    return false;
  }
  file.Read(std::as_writable_bytes(ShapeBufferBytes)
                .first(ShapeBufferBytes.size() - 1));

  /*
  ** Init the Scenario CRC value
  */
  ScenarioCRC = 0;
  const int len = static_cast<int>(std::string_view(buffer).size());
  for (int i = 0; i < len; i++) {
    val = static_cast<unsigned char>(
        std::string_view(buffer).at(base::ToSize(i)));
#ifndef DEMO
    Add_CRC(&ScenarioCRC, val);
#endif
  }

  /*
  **	Fetch the appropriate movie names from the INI file.
  */
  WWGetPrivateProfileString("Basic", "Intro", "x", IntroMovie, buffer);
  WWGetPrivateProfileString("Basic", "Brief", "x", BriefMovie, buffer);
  WWGetPrivateProfileString("Basic", "Win", "x", WinMovie, buffer);
  WWGetPrivateProfileString("Basic", "Lose", "x", LoseMovie, buffer);
  WWGetPrivateProfileString("Basic", "Action", "x", ActionMovie, buffer);

  /*
  **	For single-player scenarios, 'BuildLevel' is the scenario number.
  **	This must be set before any buildings are created (if a factory is
  *created, *	it needs to know the BuildLevel for the sidebar.)
  */
  if (GameToPlay == GAME_NORMAL) {
#ifdef NEWMENU
    if (Scenario <= 15) {
      BuildLevel = Scenario;
    } else {
      BuildLevel =
          WWGetPrivateProfileInt("Basic", "BuildLevel", Scenario, buffer);
    }
#else
    BuildLevel = Scenario;
#endif
  }

  /*
  **	Jurassic scenarios are allowed to build the full multiplayer set
  **	of objects.
  */
  if (Special.IsJurassic && AreThingiesEnabled) {
    BuildLevel = 98;
  }

  /*
  **	Fetch the transition theme for this scenario.
  */
  TransitTheme = THEME_NONE;
  WWGetPrivateProfileString("Basic", "Theme", "No Theme", buf, buffer);
  TransitTheme = ThemeClass::From_Name(buf);

  /*
  **	Read in the team-type data. The team types must be created before any
  **	triggers can be created.
  */
  TeamTypeClass::Read_INI(buffer);
  Call_Back();

  /*
  **	Read in the specific information for each of the house types.  This
  *creates *	the houses of different types.
  */
  HouseClass::Read_INI(buffer);
  Call_Back();

  /*
  **	Assign PlayerPtr by reading the player's house from the INI;
  **	Must be done before any TechnoClass objects are created.
  */
  //	if (GameToPlay == GAME_NORMAL && (ScenPlayer == SCEN_PLAYER_GDI ||
  // ScenPlayer == SCEN_PLAYER_NOD)) {
  if (GameToPlay == GAME_NORMAL) {
    WWGetPrivateProfileString(
        "Basic", "Player", "GoodGuy",
        std::span(buf).first(static_cast<std::size_t>(127)), buffer);
    CarryOverPercent =
        WWGetPrivateProfileInt("Basic", "CarryOverMoney", 100, buffer);
    CarryOverPercent = Cardinal_To_Fixed(100, CarryOverPercent);
    CarryOverCap = WWGetPrivateProfileInt("Basic", "CarryOverCap", -1, buffer);

    PlayerPtr = HouseClass::As_Pointer(HouseTypeClass::From_Name(buf));
    PlayerPtr->IsHuman = true;
    int carryover = 0;
    // Any negative cap, not just the -1 default, means uncapped; the original
    // compared the cap as unsigned.
    if (CarryOverCap >= 0) {
      carryover = std::min(Fixed_To_Cardinal(CarryOverMoney, CarryOverPercent),
                           CarryOverCap);
    } else {
      carryover = Fixed_To_Cardinal(CarryOverMoney, CarryOverPercent);
    }
    PlayerPtr->Credits += carryover;
    PlayerPtr->InitialCredits += carryover;

    if (Special.IsJurassic) {
      PlayerPtr->ActLike = Whom;
    }
  } else {
#ifdef OBSOLETE
    if (GameToPlay == GAME_NORMAL && ScenPlayer == SCEN_PLAYER_JP) {
      PlayerPtr = HouseClass::As_Pointer(HOUSE_MULTI4);
      PlayerPtr->IsHuman = true;
      PlayerPtr->Credits += CarryOverMoney;
      PlayerPtr->InitialCredits += CarryOverMoney;
      PlayerPtr->ActLike = Whom;
    } else {
      Assign_Houses();
    }
#endif
    Assign_Houses();
  }

  /*
  **	Read in the trigger data. The triggers must be created before any other
  **	objects can be initialized.
  */
  TriggerClass::Read_INI(buffer);
  Call_Back();

  /*
  **	Read in the map control values. This includes dimensions
  **	as well as theater information.
  */
  Map.Read_INI(buffer);
  Call_Back();

  /*
  **	Attempt to read the map's binary image file; if fails, read the
  **	template data from the INI, for backward compatibility
  */
  if (fresh && (!MapEditClass::Read_Binary(root, &ScenarioCRC))) {
    TemplateClass::Read_INI(buffer);
  }

  Call_Back();

  /*
  **	Read in and place the 3D terrain objects.
  */
  TerrainClass::Read_INI(buffer);
  Call_Back();

  /*
  **	Read in and place the units (all sides).
  */
  UnitClass::Read_INI(buffer);
  Call_Back();

  /*
  **	Read in and place the infantry units (all sides).
  */
  InfantryClass::Read_INI(buffer);
  Call_Back();

  /*
  **	Read in and place all the buildings on the map.
  */
  BuildingClass::Read_INI(buffer);
  Call_Back();

  /*
  **	Read in the AI's base information.
  */
  Base.Read_INI(buffer);
  Call_Back();

  /*
  **	Read in any normal overlay objects.
  */
  OverlayClass::Read_INI(buffer);
  Call_Back();

  /*
  **	Read in any smudge overlays.
  */
  SmudgeClass::Read_INI(buffer);
  Call_Back();

  /*
  **	Read in any briefing text.
  */
  std::span<char> stage(BriefingText);
  stage.front() = '\0';
  int index = 1;

  /*
  **	Build the full text of the mission objective.
  */
  for (;;) {
    char buff[16];

    absl::SNPrintF(buff, sizeof(buff), "%d", index++);
    stage.front() = '\0';
    WWGetPrivateProfileString("Briefing", buff, "",
                              stage.first(stage.size() - 1), buffer);
    if (std::string_view(stage.data()).empty()) {
      break;
    }
    // Really old and ugly code - refactor.
    // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.strcpy)
    port::SafeAppend(stage, " ");
    stage = stage.subspan(std::string_view(stage.data()).size());
  }

  /*
  **	If the briefing text could not be found in the INI file, then search
  **	the mission.ini file.
  */
  if (base::At(BriefingText, 0) == '\0') {
    std::ranges::fill(ShapeBufferBytes, 0);
    GameFile("MISSION.INI")
        .Read(std::as_writable_bytes(ShapeBufferBytes)
                  .first(ShapeBufferBytes.size() - 1));

    std::span<char> work(BriefingText);
    int player_index = 1;

    /*
    **	Build the full text of the mission objective.
    */
    for (;;) {
      char buff[16];

      absl::SNPrintF(buff, sizeof(buff), "%d", player_index++);
      work.front() = '\0';
      WWGetPrivateProfileString(root, buff, "", work.first(work.size() - 1),
                                ShapeBuffer);
      if (std::string_view(work.data()).empty()) {
        break;
      }
      // Really old and ugly code - refactor.
      // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.strcpy)
      port::SafeAppend(work, " ");
      work = work.subspan(std::string_view(work.data()).size());
    }
  }

  /*
  **	Perform a final overpass of the map. This handles smoothing of certain
  **	types of terrain (tiberium).
  */
  Map.Overpass();
  Call_Back();

  /*
  **	Multi-player last-minute fixups:
  **	- If computer players are disabled, remove all computer-owned houses
  ** - Otherwise, set MPlayerBlitz to 0 or 1, randomly
  **	- If bases are disabled, create the scenario dynamically
  **	- Remove any flag spot overlays lying around
  **	- If capture-the-flag is enabled, assign flags to cells.
  */
  if (GameToPlay != GAME_NORMAL || ScenPlayer == SCEN_PLAYER_2PLAYER ||
      ScenPlayer == SCEN_PLAYER_MPLAYER) {
    /*
    **	If Ghosts are disabled and we're not editing, remove computer players
    **	(Must be done after all objects are read in from the INI)
    */
    if (!MPlayerGhosts && !Debug_Map) {
      Remove_AI_Players();
    } else {
      /*
      ** If Ghosts are on, set up their houses for blitzing the humans
      */
      MPlayerBlitz = GameRandomRange(0, 1);  // 1 = computer will blitz
      if (MPlayerBlitz) {
        if (MPlayerBases) {
          rndmax = 14000;
          rndmin = 10000;
        } else {
          rndmax = 8000;
          rndmin = 4000;
        }

        for (int i = 0; i < MPlayerMax; i++) {
          const auto house =
              static_cast<HousesType>(i + static_cast<int>(HOUSE_MULTI1));
          HouseClass* housep = HouseClass::As_Pointer(house);
          housep->BlitzTime = GameRandomRange(rndmin, rndmax);
        }
      }
    }

    /*
    **	Units must be created for each house.  If bases are ON, this routine
    **	will create an MCV along with the units; otherwise, it will just create
    **	a whole bunch of units.  MPlayerUnitCount is the total # of units
    **	to create.
    */
    if (!Debug_Map) {
      const int save_init = ScenarioInit;  // turn ScenarioInit off
      ScenarioInit = 0;
      Create_Units();
      ScenarioInit = save_init;  // turn ScenarioInit back on
    }

    /*
    **	Place crates if MPlayerGoodies is on.
    */
    if (MPlayerGoodies) {
      for (int player_index = 0; player_index < MPlayerCount; player_index++) {
        Map.Place_Random_Crate();
      }
    }

    /*
    **	Compute my starting location as the average Coord of all my stuff.
    */
    Map.Compute_Start_Pos();
  }

  Call_Back();

  /*
  **	Return with flag saying that the scenario file was read.
  */
  ScenarioInit--;
  return true;
}

/***********************************************************************************************
 * Write_Scenario_Ini -- Write the scenario INI file. *
 *                                                                                             *
 * INPUT: * root      root filename for the scenario *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 10/07/1992 JLB : Created. * 05/11/1995 JLB : Updates movie data. *
 *=============================================================================================*/
void Write_Scenario_Ini(const char* root) {
  if constexpr (config::kCheatKeysEnabled) {
    char fname[kMaxFname + kMaxExt];    // full scenario name
    HousesType house = HOUSE_NONE;
    GameFile file;

    /*
    **	Get a working pointer to the INI staging buffer. Make sure that the
    *buffer *	starts cleared out of any data.
    */
    char* buffer = ShapeBuffer;  // Scenario.ini staging buffer pointer.
    std::ranges::fill(ShapeBufferBytes, 0);

    switch (ScenPlayer) {
      case SCEN_PLAYER_GDI:
        house = HOUSE_GOOD;
        break;

      case SCEN_PLAYER_NOD:
        house = HOUSE_BAD;
        break;

      case SCEN_PLAYER_JP:
        house = HOUSE_JP;
        break;

      case ScenarioPlayerType::SCEN_PLAYER_NONE:
      case ScenarioPlayerType::SCEN_PLAYER_2PLAYER:
      case ScenarioPlayerType::SCEN_PLAYER_MPLAYER:
      case ScenarioPlayerType::SCEN_PLAYER_COUNT:
      default:
        house = HOUSE_MULTI1;
        break;
    }

    /*
    **	Create scenario filename and clear the buffer to empty.
    */
    absl::SNPrintF(fname, sizeof(fname), "%s.INI", root);
    file.SetName(fname);
    if (file.IsAvailable()) {
      //		file.Open(READ);
      file.Read(std::as_writable_bytes(ShapeBufferBytes)
                    .first(ShapeBufferBytes.size() - 1));
      //		file.Close();
    } else {
      absl::SNPrintF(buffer, base::ToSize(ShapeBufferSize),
                     "; Scenario %d control for house %s.\r\n", Scenario,
                     HouseTypeClass::As_Reference(house).IniName);
    }

    WWWritePrivateProfileString("Basic", "Intro", IntroMovie,
                                port::CharBytes(ShapeBufferBytes));
    WWWritePrivateProfileString("Basic", "Brief", BriefMovie,
                                port::CharBytes(ShapeBufferBytes));
    WWWritePrivateProfileString("Basic", "Win", WinMovie,
                                port::CharBytes(ShapeBufferBytes));
    WWWritePrivateProfileString("Basic", "Lose", LoseMovie,
                                port::CharBytes(ShapeBufferBytes));
    WWWritePrivateProfileString("Basic", "Action", ActionMovie,
                                port::CharBytes(ShapeBufferBytes));
    WWWritePrivateProfileString("Basic", "Player", PlayerPtr->Class->IniName,
                                port::CharBytes(ShapeBufferBytes));
    WWWritePrivateProfileString("Basic", "Theme",
                                ThemeClass::Base_Name(TransitTheme),
                                port::CharBytes(ShapeBufferBytes));
    WWWritePrivateProfileInt("Basic", "BuildLevel", BuildLevel,
                             port::CharBytes(ShapeBufferBytes));
    WWWritePrivateProfileInt("Basic", "CarryOverMoney",
                             Fixed_To_Cardinal(100, CarryOverPercent),
                             port::CharBytes(ShapeBufferBytes));
    WWWritePrivateProfileInt("Basic", "CarryOverCap", CarryOverCap,
                             port::CharBytes(ShapeBufferBytes));

    TeamTypeClass::Write_INI(port::CharBytes(ShapeBufferBytes), true);
    TriggerClass::Write_INI(port::CharBytes(ShapeBufferBytes), true);
    Map.Write_INI(port::CharBytes(ShapeBufferBytes));
    MapEditClass::Write_Binary(root);
    HouseClass::Write_INI(port::CharBytes(ShapeBufferBytes));
    UnitClass::Write_INI(port::CharBytes(ShapeBufferBytes));
    InfantryClass::Write_INI(port::CharBytes(ShapeBufferBytes));
    BuildingClass::Write_INI(port::CharBytes(ShapeBufferBytes));
    TerrainClass::Write_INI(port::CharBytes(ShapeBufferBytes));
    OverlayClass::Write_INI(port::CharBytes(ShapeBufferBytes));
    SmudgeClass::Write_INI(port::CharBytes(ShapeBufferBytes));

    Base.Write_INI(port::CharBytes(ShapeBufferBytes));

    /*
    **	Write the scenario data out to a file.
    */
    //	file.Open(WRITE);
    file.Write(
        std::as_bytes(ShapeBufferBytes).first(std::string_view(buffer).size()));
    //	file.Close();

    /*
    **	Now update the Master INI file, containing the master list of triggers &
    *teams
    */
    std::ranges::fill(ShapeBufferBytes, 0);

    file.SetName("MASTER.INI");
    if (file.IsAvailable()) {
      //		file.Open(READ);
      file.Read(std::as_writable_bytes(ShapeBufferBytes)
                    .first(ShapeBufferBytes.size() - 1));
      //		file.Close();
    } else {
      absl::SNPrintF(buffer, base::ToSize(ShapeBufferSize),
                     "; Master Trigger & Team List.\r\n");
    }

    TeamTypeClass::Write_INI(port::CharBytes(ShapeBufferBytes), false);
    TriggerClass::Write_INI(port::CharBytes(ShapeBufferBytes), false);

    //	file.Open(WRITE);
    file.Write(
        std::as_bytes(ShapeBufferBytes).first(std::string_view(buffer).size()));
    //	file.Close();
  }
}

/***********************************************************************************************
 * Assign_Houses -- Assigns multiplayer houses to various players *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 06/09/1995 BRR : Created. * 07/14/1995 JLB : Records name of
 *player in house structure.                               *
 *=============================================================================================*/
static void Assign_Houses() {
  HousesType house = HOUSE_NONE;
  HousesType pref_house = HOUSE_NONE;
  HouseClass* housep = nullptr;
  bool house_used[MAX_PLAYERS];  // true = this house is in use
  bool color_used[6];            // true = this color is in use
  PlayerColorType color = REMAP_NONE;

  char wibble[256];
  absl::SNPrintF(wibble, sizeof(wibble),
                 "C&C95 - In 'Assign_Houses'. Number of players:%d\n",
                 MPlayerCount);
  CCDebugString(wibble);

  /*
  **	Init the 'used' flag for all houses & colors to 0
  */
  for (bool& i : house_used) {
    i = false;
  }
  for (bool& i : color_used) {
    i = false;
  }

  /*
  **	For each player, randomly pick a house
  */
  for (int i = 0; i < MPlayerCount; i++) {
    const int j = Random_Pick(0, MPlayerMax - 1);

    /*
    **	If this house was already selected, decrement 'i' & keep looping.
    */
    if (base::At(house_used, j)) {
      i--;
      continue;
    }

    /*
    **	Set the house, preferred house (GDI/NOD), color, and actual house;
    **	get a pointer to the house instance
    */
    house = static_cast<HousesType>(j + static_cast<int>(HOUSE_MULTI1));
    pref_house = MPlayerID_To_HousesType(base::At(MPlayerID, i));
    color = MPlayerID_To_ColorIndex(base::At(MPlayerID, i));
    housep = HouseClass::As_Pointer(house);
    base::At(MPlayerHouses, i) = house;

    /*
    **	Mark this house & color as used
    */
    base::At(house_used, j) = true;
    base::At(color_used, static_cast<int>(color)) = true;

    /*
    **	Set the house's IsHuman, Credits, ActLike, & RemapTable
    */
    base::FillBytes(base::ObjectBytes(housep->Name), 0, MPLAYER_NAME_MAX);
    port::SafeCopy(housep->Name, base::At(MPlayerNames, i));
    housep->IsHuman = true;
    housep->Init_Data(color, pref_house, MPlayerCredits);

    /*
    **	If this ID is for myself, set up PlayerPtr
    */
    if (base::At(MPlayerID, i) == MPlayerLocalID) {
      PlayerPtr = housep;
    }
  }

  /*
  **	For all houses not assigned to a player, set them up for computer use
  */
  for (int i = 0; i < MPlayerMax; i++) {
    if (!base::At(house_used, i)) {
      /*
      **	Set the house, preferred house (GDI/NOD), and color; get a
      *pointer *	to the house instance
      */
      house = static_cast<HousesType>(i + static_cast<int>(HOUSE_MULTI1));
      pref_house = static_cast<HousesType>(GameRandomRange(0, 1) +
                                           static_cast<int>(HOUSE_GOOD));
      for (;;) {
        color = Random_Pick(REMAP_FIRST, REMAP_LAST);
        if (!base::At(color_used, static_cast<int>(color))) {
          break;
        }
      }
      housep = HouseClass::As_Pointer(house);

      /*
      **	Mark this house & color as used
      */
      base::At(house_used, i) = true;
      base::At(color_used, static_cast<int>(color)) = true;

      /*
      **	Set the house's IsHuman, Credits, ActLike, & RemapTable
      */
      housep->IsHuman = false;
      housep->Init_Data(color, pref_house, MPlayerCredits);
    }
  }

  /*
  **	Now make all computer-owned houses allies of each other.
  */
  const auto last_house =
      static_cast<HousesType>(static_cast<int>(HOUSE_MULTI1) + MPlayerMax);
  for (house = HOUSE_MULTI1; house < last_house; house++) {
    housep = HouseClass::As_Pointer(house);
    if (housep->IsHuman) {
      continue;
    }

    for (HousesType house2 = HOUSE_MULTI1; house2 < last_house; house2++) {
      HouseClass* housep2 = HouseClass::As_Pointer(house2);
      if (housep2->IsHuman) {
        continue;
      }
      housep->Make_Ally(house2);
    }
  }
}

/***********************************************************************************************
 * Remove_AI_Players -- Removes the computer AI houses & their units *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 06/09/1995 BRR : Created. *
 *=============================================================================================*/
static void Remove_AI_Players() {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    const auto house =
        static_cast<HousesType>(i + static_cast<int>(HOUSE_MULTI1));
    HouseClass* housep = HouseClass::As_Pointer(house);
    if (!static_cast<bool>(housep->IsHuman)) {
      housep->Clobber_All();
    }
  }
}

/***********************************************************************************************
 * Create_Units -- Creates infantry & units, for non-base multiplayer *
 *                                                                                             *
 * This routine uses data tables to determine which units to create for either *
 * a GDI or NOD house, and how many of each. *
 *                                                                                             *
 * It also sets each house's FlagHome & FlagLocation to the Waypoint selected *
 * as that house's "home" cell. *
 *                                                                                             *
 *   ------------------ Unit Summary: ------------------------------- *
 *   UNIT_MTANK               Medium tank (M1).            GDI      7 *
 *   UNIT_JEEP               4x4 jeep replacement.      GDI      5 * UNIT_MLRS
 *MLRS rocket launcher.      GDI      99                            * UNIT_APC
 *APC.                        GDI      10                         * UNIT_HTANK
 *Heavy tank (Mammoth).      GDI      13                           *
 *                                                                                             *
 *   UNIT_LTANK               Light tank ('Bradly').      NOD      5 *
 *   UNIT_BUGGY               Rat patrol dune buggy type NOD      5 * UNIT_ARTY
 *Artillery unit.            NOD      10                            * UNIT_FTANK
 *Flame thrower tank.         NOD      11                          * UNIT_STANK
 *Stealth tank (Romulan).      NOD      13                         * UNIT_BIKE
 *Nod recon motor-bike.      NOD      99                            *
 *                                                                                             *
 *   ~1/3 chance of getting: {UNIT_MHQ,               Mobile Head Quarters. *
 *                                                                                             *
 *   ------------------ Infantry Summary: ------------------------------- *
 *   INFANTRY_E1,            Mini-gun armed.            GDI/NOD * INFANTRY_E2,
 *Grenade thrower.            GDI                                   *
 *   INFANTRY_E3,            Rocket launcher.            NOD * INFANTRY_E6,
 *Rocket launcher             GDI                                   *
 *   INFANTRY_E4,            Flame thrower equipped.      NOD * INFANTRY_RAMBO,
 *Commando.                  GDI/NOD                                *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 06/09/1995 BRR : Created. *
 *=============================================================================================*/
static void Create_Units() {
  constexpr int kNumUnitCategories = 8;
  constexpr int kNumInfantryCategories = 5;

  static const struct {
    int MinLevel;
    int GDICount;
    UnitType GDIType;
    int NODCount;
    UnitType NODType;
  } utable[] = {
      {0, 1, UNIT_MTANK, 2, UNIT_LTANK}, {2, 1, UNIT_JEEP, 1, UNIT_BUGGY},
      {3, 1, UNIT_MLRS, 1, UNIT_ARTY},   {4, 1, UNIT_APC, 2, UNIT_BUGGY},
      {5, 1, UNIT_JEEP, 1, UNIT_BIKE},   {5, 2, UNIT_JEEP, 1, UNIT_FTANK},
      {6, 1, UNIT_MSAM, 1, UNIT_MSAM},   {7, 1, UNIT_HTANK, 2, UNIT_STANK},
  };
  static int num_units[kNumUnitCategories];  // # of each type of unit to create

  static const struct {
    int MinLevel;
    int GDICount;
    InfantryType GDIType;
    int NODCount;
    InfantryType NODType;
  } itable[] = {
      {0, 1, INFANTRY_E1, 1, INFANTRY_E1},
      {1, 1, INFANTRY_E2, 1, INFANTRY_E3},
      {3, 1, INFANTRY_E3, 1, INFANTRY_E3},
      {5, 1, INFANTRY_E3, 1, INFANTRY_E4},
      {7, 1, INFANTRY_RAMBO, 1, INFANTRY_RAMBO},
  };
  static int num_infantry[kNumInfantryCategories];  // # of each type of
                                                    // infantry to create

  CELL waypts[26];
  CELL sorted_waypts[26];

  CELL centroid = 0;  // centroid of this house's stuff
  CELL centerpt = 0;  // centroid for a category of objects, as a CELL

  int u_limit = 0;   // last allowable index of units for this BuildLevel
  int i_limit = 0;   // last allowable index of infantry for this BuildLevel
  TechnoClass* obj = nullptr;  // newly-created object
  int scaleval = 0;            // value to scale # units or infantry

  /*------------------------------------------------------------------------
  For the current BuildLevel, find the max allowable index into the tables
  ------------------------------------------------------------------------*/
  for (int i = 0; i < kNumUnitCategories; i++) {
    if (BuildLevel >= base::At(utable, i).MinLevel) {
      u_limit = i;
    }
  }
  for (int i = 0; i < kNumInfantryCategories; i++) {
    if (BuildLevel >= base::At(utable, i).MinLevel) {
      i_limit = i;
    }
  }

  /*------------------------------------------------------------------------
  Compute how many of each buildable category to create
  ------------------------------------------------------------------------*/
  /*........................................................................
  Compute allowed # units
  ........................................................................*/
  const int tot_units = MPlayerUnitCount * 2 / 3;  // total # units to create
  //	tot_units = std::max(tot_units, 1);

  /*........................................................................
  Init # of each category to 0
  ........................................................................*/
  for (int i = 0; i <= u_limit; i++) {
    base::At(num_units, i) = 0;
  }

  /*........................................................................
  Increment # of each category, until we've used up all units
  ........................................................................*/
  int j = 0;
  for (int i = 0; i < tot_units; i++) {
    base::At(num_units, j)++;
    j++;
    if (j > u_limit) {
      j = 0;
    }
  }

  /*........................................................................
  Compute allowed # infantry
  ........................................................................*/
  const int tot_infantry =
      MPlayerUnitCount - tot_units;  // total # infantry to create

  /*........................................................................
  Init # of each category to 0
  ........................................................................*/
  for (int i = 0; i <= i_limit; i++) {
    base::At(num_infantry, i) = 0;
  }

  /*........................................................................
  Increment # of each category, until we've used up all infantry
  ........................................................................*/
  j = 0;
  for (int i = 0; i < tot_infantry; i++) {
    base::At(num_infantry, j)++;
    j++;
    if (j > i_limit) {
      j = 0;
    }
  }

  /*------------------------------------------------------------------------
  Now sort all the Waypoints on the map by distance.
  ------------------------------------------------------------------------*/
  int num_waypts = 0;  // counts # waypoints

  /*........................................................................
  First, copy all valid waytpoints into my 'waypts' array
  ........................................................................*/
  for (int i = 0; i < 26; i++) {
    if (base::At(Waypoint, i) != -1) {
      base::At(waypts, num_waypts) = base::At(Waypoint, i);
      num_waypts++;
    }
  }

  /*........................................................................
  Now sort the 'waypts' array
  ........................................................................*/
  Sort_Cells(waypts, num_waypts, sorted_waypts);

  /*------------------------------------------------------------------------
  Loop through all houses.  Computer-controlled houses, with MPlayerBases
  ON, are treated as though bases are OFF (since we have no base-building
  AI logic.)
  ------------------------------------------------------------------------*/
  const auto last_house =
      static_cast<HousesType>(static_cast<int>(HOUSE_MULTI1) + MPlayerMax);
  for (HousesType h = HOUSE_MULTI1; h < last_house; h++) {
    /*.....................................................................
    Get a pointer to this house; if there is none, go to the next house
    .....................................................................*/
    HouseClass* hptr =
        HouseClass::As_Pointer(h);  // ptr to house being processed
    if (!hptr) {
      continue;
    }

    /*.....................................................................
    Pick a random waypoint; if the chosen waypoint isn't valid, try again.
    'centroid' will be the centroid of all this house's stuff.
    .....................................................................*/
    int try_count = 0;  // # times we've tried to select a centroid
    while (true) {
      j = GameRandomRange(0, MPlayerMax - 1);
      if (base::At(sorted_waypts, j) != -1) {
        centroid = base::At(sorted_waypts, j);
        base::At(sorted_waypts, j) = -1;
        break;
      }
      try_count++;

      /*..................................................................
      OK, we've tried enough; just pick any old cell at random, as long
      as it's mappable.
      ..................................................................*/
      if (try_count > 200) {
        while (true) {
          centroid = static_cast<CELL>(GameRandomRange(0, MAP_CELL_TOTAL - 1));
          if (Map.In_Radar(centroid)) {
            break;
          }
        }
        break;
      }
    }

    /*---------------------------------------------------------------------
    If Bases are ON, human & computer houses are treated differently
    ---------------------------------------------------------------------*/
    if (MPlayerBases) {
      /*..................................................................
      - For a human-controlled house:
        - Set 'scaleval' to 1
        - Create an MCV
        - Attach a flag to it for capture-the-flag mode
      ..................................................................*/
      if (hptr->IsHuman) {
        scaleval = 1;
        obj = new UnitClass(UNIT_MCV, h);
        if ((!obj->Unlimbo(Cell_Coord(centroid), DIR_N)) &&
            (!Scan_Place_Object(obj, centroid))) {
          delete obj;
          obj = nullptr;
        }

        if (obj) {
          hptr->FlagHome = 0;
          hptr->FlagLocation = 0;
          if (Special.IsCaptureTheFlag) {
            hptr->Flag_Attach(dynamic_cast<UnitClass*>(obj), true);
          }
        }
      } else {
        /*..................................................................
        - For computer-controlled house:
          - Set 'scaleval' to 3
          - Create a Mobile HQ for capture-the-flag mode
        ..................................................................*/
        scaleval = 3 / (MPlayerMax - MPlayerCount);
        if (scaleval == 0) {
          scaleval = 1;
        }

        if (Special.IsCaptureTheFlag) {
          obj = new UnitClass(UNIT_MHQ, h);
          if ((!obj->Unlimbo(Cell_Coord(centroid), DIR_N)) &&
              (!Scan_Place_Object(obj, centroid))) {
            delete obj;
            obj = nullptr;
          }

          hptr->FlagHome = 0;  // turn house's flag off
          hptr->FlagLocation = 0;
        }
      }
    } else {
      /*---------------------------------------------------------------------
      If bases are OFF, set 'scaleval' to 1 & create a Mobile HQ for
      capture-the-flag mode.
      ---------------------------------------------------------------------*/
      scaleval = 1;
      if (Special.IsCaptureTheFlag) {
        obj = new UnitClass(UNIT_MHQ, h);
        obj->Unlimbo(Cell_Coord(centroid), DIR_N);
        hptr->FlagHome = 0;  // turn house's flag off
        hptr->FlagLocation = 0;
      }
    }

    /*---------------------------------------------------------------------
    Set the house's max # units (this is used in the Mission_Timed_Hunt())
    ---------------------------------------------------------------------*/
    hptr->MaxUnit = MPlayerUnitCount * scaleval;

    /*---------------------------------------------------------------------
    Create units for this house
    ---------------------------------------------------------------------*/
    for (int i = 0; i <= u_limit; i++) {
      /*..................................................................
      Find the center point for this category.
      ..................................................................*/
      centerpt = Clip_Scatter(centroid, 4);

      /*..................................................................
      Place objects; loop through all unit in this category
      ..................................................................*/
      for (j = 0; j < base::At(num_units, i) * scaleval; j++) {
        /*...............................................................
        Create a GDI unit
        ...............................................................*/
        if (hptr->ActLike == HOUSE_GOOD) {
          for (int k = 0; k < base::At(utable, i).GDICount; k++) {
            obj = new UnitClass(base::At(utable, i).GDIType, h);
            if (!Scan_Place_Object(obj, centerpt)) {
              delete obj;
            } else {
              if (!hptr->IsHuman) {
                obj->Set_Mission(MISSION_TIMED_HUNT);
              }
            }
          }
        } else {
          /*...............................................................
          Create a NOD unit
          ...............................................................*/
          for (int k = 0; k < base::At(utable, i).NODCount; k++) {
            obj = new UnitClass(base::At(utable, i).NODType, h);
            if (!Scan_Place_Object(obj, centerpt)) {
              delete obj;
            } else {
              if (!hptr->IsHuman) {
                obj->Set_Mission(MISSION_TIMED_HUNT);
              }
            }
          }
        }
      }
    }

    /*---------------------------------------------------------------------
    Create infantry
    ---------------------------------------------------------------------*/
    for (int i = 0; i <= i_limit; i++) {
      /*..................................................................
      Find the center point for this category.
      ..................................................................*/
      centerpt = Clip_Scatter(centroid, 4);

      /*..................................................................
      Place objects; loop through all unit in this category
      ..................................................................*/
      for (j = 0; j < base::At(num_infantry, i) * scaleval; j++) {
        /*...............................................................
        Create GDI infantry (Note: Unlimbo calls Enter_Idle_Mode(), which
        assigns the infantry to HUNT; we must use Set_Mission() to override
        this state.)
        ...............................................................*/
        if (hptr->ActLike == HOUSE_GOOD) {
          for (int k = 0; k < base::At(itable, i).GDICount; k++) {
            obj = new InfantryClass(base::At(itable, i).GDIType, h);
            if (!Scan_Place_Object(obj, centerpt)) {
              delete obj;
            } else {
              if (!hptr->IsHuman) {
                obj->Set_Mission(MISSION_TIMED_HUNT);
              }
            }
          }
        } else {
          /*...............................................................
          Create NOD infantry
          ...............................................................*/
          for (int k = 0; k < base::At(itable, i).NODCount; k++) {
            obj = new InfantryClass(base::At(itable, i).NODType, h);
            if (!Scan_Place_Object(obj, centerpt)) {
              delete obj;
            } else {
              if (!hptr->IsHuman) {
                obj->Set_Mission(MISSION_TIMED_HUNT);
              }
            }
          }
        }
      }
    }
  }
}

/***********************************************************************************************
 * Scan_Place_Object -- places an object >near< the given cell *
 *                                                                                             *
 * INPUT: * obj      ptr to object to Unlimbo * cell      center of search area
 **
 *                                                                                             *
 * OUTPUT: * true = object was placed; false = it wasn't *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 06/09/1995 BRR : Created. *
 *=============================================================================================*/
bool Scan_Place_Object(ObjectClass* obj, CELL cell) {
  TechnoClass* techno = nullptr;

  /*------------------------------------------------------------------------
  First try to unlimbo the object in the given cell.
  ------------------------------------------------------------------------*/
  if (Map.In_Radar(cell)) {
    techno = Map.at(cell).Cell_Techno();
    if ((!techno || (techno->What_Am_I() == RTTI_INFANTRY &&
                     obj->What_Am_I() == RTTI_INFANTRY)) &&
        obj->Unlimbo(Cell_Coord(cell), DIR_N)) {
      return true;
    }
  }

  /*------------------------------------------------------------------------
  Loop through distances from the given center cell; skip the center cell.
  For each distance, try placing the object along each rotational direction;
  if none are available, try each direction with a random scatter value.
  If that fails, go to the next distance.
  This ensures that the closest coordinates are filled first.
  ------------------------------------------------------------------------*/
  for (int dist = 1; dist < 32; dist++) {
    /*.....................................................................
    Pick a random starting direction
    .....................................................................*/
    auto rot = Random_Pick(FACING_N, FACING_NW);  // for object placement
    /*.....................................................................
    Try all directions twice
    .....................................................................*/
    for (int tryval = 0; tryval < 2; tryval++) {
      /*..................................................................
      Loop through all directions, at this distance.
      ..................................................................*/
      for (FacingType fcounter = FACING_N; fcounter <= FACING_NW; fcounter++) {
        bool skipit = false;

        /*...............................................................
        Pick a coordinate along this directional axis
        ...............................................................*/
        CELL newcell = Clip_Move(cell, rot, dist);

        /*...............................................................
        If this is our second try at this distance, add a random scatter
        to the desired cell, so our units aren't all aligned along spokes.
        ...............................................................*/
        if (tryval > 0) {
          newcell = Clip_Scatter(newcell, 1);
        }

        /*...............................................................
        If, by randomly scattering, we've chosen the exact center, skip
        it & try another direction.
        ...............................................................*/
        if (newcell == cell) {
          skipit = true;
        }

        if (!skipit) {
          /*............................................................
          Only attempt to Unlimbo the object if:
          - there is no techno in the cell
          - the techno in the cell & the object are both infantry
          ............................................................*/
          techno = Map.at(newcell).Cell_Techno();
          if ((!techno || (techno->What_Am_I() == RTTI_INFANTRY &&
                           obj->What_Am_I() == RTTI_INFANTRY)) &&
              obj->Unlimbo(Cell_Coord(newcell), DIR_N)) {
            return true;
          }
        }

        rot++;
        if (rot > FACING_NW) {
          rot = FACING_N;
        }
      }
    }
  }

  return false;
}

/***********************************************************************************************
 * Sort_Cells -- sorts an array of cells by distance *
 *                                                                                             *
 * INPUT: * cells         array to sort * numcells      # entries in 'cells' *
 *      outcells      array to store sorted values in *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 07/19/1995 BRR : Created. *
 *=============================================================================================*/
static void Sort_Cells(std::span<CELL> cells, int numcells,
                       std::span<CELL> outcells) {
  CHECK_GE(numcells, 0);
  CHECK_LE(base::ToSize(numcells), cells.size());
  CHECK_LE(base::ToSize(numcells), outcells.size());
  if (numcells == 0) {
    return;
  }
  int num_sorted = 0;
  int num_unsorted = numcells;

  /*------------------------------------------------------------------------
  Pick the first cell at random
  ------------------------------------------------------------------------*/
  int j = Random_Pick(0, numcells - 1);
  base::At(outcells, base::ToSize(0)) = base::At(cells, base::ToSize(j));
  num_sorted++;

  for (int k = j; k < num_unsorted - 1; k++) {
    base::At(cells, base::ToSize(k)) = base::At(cells, base::ToSize(k + 1));
  }
  num_unsorted--;

  /*------------------------------------------------------------------------
  After the first cell, assign the other cells based on who's furthest away
  from the chosen ones.
  ------------------------------------------------------------------------*/
  for (int i = 1; i < numcells; i++) {
    j = Furthest_Cell(outcells, num_sorted, cells, num_unsorted);
    base::At(outcells, base::ToSize(num_sorted)) =
        base::At(cells, base::ToSize(j));
    num_sorted++;

    for (int k = j; k < num_unsorted - 1; k++) {
      base::At(cells, base::ToSize(k)) = base::At(cells, base::ToSize(k + 1));
    }
    num_unsorted--;
  }
}

/***********************************************************************************************
 * Furthest_Cell -- Finds cell furthest from a group of cells *
 *                                                                                             *
 * INPUT: * cells            array of cells to find furthest-cell-away-from *
 *      numcells         # entries in 'cells' * tcells         array of cells to
 *test; one of these will be selected as being          * "furthest" from all
 *the cells in 'cells'                                * numtcells      # entries
 *in 'tcells'                                                   *
 *                                                                                             *
 * OUTPUT: * index of 'tcell' that's furthest away from 'cells' *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 07/19/1995 BRR : Created. *
 *=============================================================================================*/
static int Furthest_Cell(std::span<const CELL> ref_cells, int num_ref_cells,
                         std::span<const CELL> test_cells, int num_test_cells) {
  /*------------------------------------------------------------------------
  Initialize
  ------------------------------------------------------------------------*/
  int maxmindist = 0;  // the highest mindist value of all test_cells
  int maxmin_idx = 0;  // index of the test_cell with largest mindist

  /*------------------------------------------------------------------------
  Loop through all test cells, finding the furthest one from all entries in
  the ref_cells array
  ------------------------------------------------------------------------*/
  for (int i = 0; i < num_test_cells; i++) {
    /*.....................................................................
    Find the ref_cell closest to this test_cell
    .....................................................................*/
    int mindist = 0xffff;  // minimum distance a test_cell is from a ref_cell
    for (int j = 0; j < num_ref_cells; j++) {
      const int dist = Distance(
          base::At(test_cells, base::ToSize(i)),
          base::At(ref_cells, base::ToSize(j)));  // working distance measure
      mindist = std::min(dist, mindist);
    }

    /*.....................................................................
    If this test_cell is further away than the others, save its distance &
    index value
    .....................................................................*/
    if (mindist >= maxmindist) {
      maxmindist = mindist;
      maxmin_idx = i;
    }
  }

  return maxmin_idx;
}

/***********************************************************************************************
 * Clip_Scatter -- randomly scatters from given cell; won't fall off map *
 *                                                                                             *
 * INPUT: * cell      cell to scatter from * maxdist   max distance to scatter *
 *                                                                                             *
 * OUTPUT: * new cell number *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 07/30/1995 BRR : Created. *
 *=============================================================================================*/
static CELL Clip_Scatter(CELL cell, int maxdist) {

  /*------------------------------------------------------------------------
  Get X & Y coords of given starting cell
  ------------------------------------------------------------------------*/
  int x = Cell_X(cell);
  int y = Cell_Y(cell);

  /*------------------------------------------------------------------------
  Compute our x & y limits
  ------------------------------------------------------------------------*/
  const int xmin = Map.MapCellX;
  const int xmax = xmin + Map.MapCellWidth - 1;
  const int ymin = Map.MapCellY;
  const int ymax = ymin + Map.MapCellHeight - 1;

  /*------------------------------------------------------------------------
  Adjust the x-coordinate
  ------------------------------------------------------------------------*/
  const int xdist = GameRandomRange(0, maxdist);
  if (GameRandomRange(0, 1) == 0) {
    x += xdist;
    x = std::min(x, xmax);
  } else {
    x -= xdist;
    x = std::max(x, xmin);
  }

  /*------------------------------------------------------------------------
  Adjust the y-coordinate
  ------------------------------------------------------------------------*/
  const int ydist = GameRandomRange(0, maxdist);
  if (GameRandomRange(0, 1) == 0) {
    y += ydist;
    y = std::min(y, ymax);
  } else {
    y -= ydist;
    y = std::max(y, ymin);
  }

  return XY_Cell(x, y);
}

/***********************************************************************************************
 * Clip_Move -- moves in given direction from given cell; clips to map *
 *                                                                                             *
 * INPUT: * cell      cell to start from * facing   direction to move * dist
 *distance to move                                                             *
 *                                                                                             *
 * OUTPUT: * new cell number *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 07/30/1995 BRR : Created. *
 *=============================================================================================*/
static CELL Clip_Move(CELL cell, FacingType facing, int dist) {

  /*------------------------------------------------------------------------
  Get X & Y coords of given starting cell
  ------------------------------------------------------------------------*/
  int x = Cell_X(cell);
  int y = Cell_Y(cell);

  /*------------------------------------------------------------------------
  Compute our x & y limits
  ------------------------------------------------------------------------*/
  const int xmin = Map.MapCellX;
  const int xmax = xmin + Map.MapCellWidth - 1;
  const int ymin = Map.MapCellY;
  const int ymax = ymin + Map.MapCellHeight - 1;

  /*------------------------------------------------------------------------
  Adjust the x-coordinate
  ------------------------------------------------------------------------*/
  switch (facing) {
    case FACING_N:
      y -= dist;
      break;

    case FACING_NE:
      x += dist;
      y -= dist;
      break;

    case FACING_E:
      x += dist;
      break;

    case FACING_SE:
      x += dist;
      y += dist;
      break;

    case FACING_S:
      y += dist;
      break;

    case FACING_SW:
      x -= dist;
      y += dist;
      break;

    case FACING_W:
      x -= dist;
      break;

    case FACING_NW:
      x -= dist;
      y -= dist;
      break;
    case FacingType::FACING_NONE:
    case FacingType::FACING_COUNT:
    default:
      break;
  }

  /*------------------------------------------------------------------------
  Clip to the map
  ------------------------------------------------------------------------*/
  x = std::clamp(x, xmin, xmax);
  y = std::clamp(y, ymin, ymax);
  return XY_Cell(x, y);
}
