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

/* $Header: /CounterStrike/SCENARIO.CPP 15    3/13/97 2:06p Steve_tall $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : SCENARIO.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : September 10, 1993 *
 *                                                                                             *
 *                  Last Update : October 21, 1996 [JLB] *
 *                                                                                             *
 * This module handles the scenario reading and writing. Scenario related * code
 *that is executed between scenario play can also be here. *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Assign_Houses -- Assigns multiplayer houses to various players *
 *   Clear_Flag_Spots -- Clears flag overlays off the map * Clear_Scenario --
 *Clears all data in preparation for scenario load.                       *
 *   Clip_Move -- moves in given direction from given cell; clips to map *
 *   Clip_Scatter -- randomly scatters from given cell; won't fall off map *
 *   Create_Units -- Creates infantry & units, for non-base multiplayer *
 *   Do_Lose -- Display losing comments. * Do_Restart -- Handle the restart
 *mission process.                                         * Do_Win -- Display
 *winning congratulations.                                                *
 *   Fill_In_Data -- Recreate all data that is not loaded with scenario. *
 *   Post_Load_Game -- Fill in an inferred data from the game state. *
 *   Read_Scenario -- Reads a scenario from disk. * Read_Scenario_INI -- Read
 *specified scenario INI file.                                    *
 *   Remove_AI_Players -- Removes the computer AI houses & their units *
 *   Restate_Mission -- Handles restating the mission objective. *
 *   Scan_Place_Object -- places an object >near< the given cell *
 *   ScenarioClass::ScenarioClass -- Constructor for the scenario control
 *object.              * ScenarioClass::Set_Global_To -- Set scenario global to
 *value specified.                   * Set_Scenario_Name -- Creates the INI
 *scenario name string.                                * Start_Scenario --
 *Starts the scenario.                                                    *
 *   Write_Scenario_INI -- Write the scenario INI file. *
 *   ScenarioClass::Do_BW_Fade -- Cause the palette to temporarily shift to B/W.
 ** ScenarioClass::Do_Fade_AI -- Process the palette fading effect. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/scenario.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <span>
#include <string>
#include <string_view>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/enum_array.h"
#include "base/numeric.h"
#include "base/strings/number_parse.h"
#include "base/strings/safe_string.h"
#include "engine/audio/audio_mixer.h"
#include "engine/file/disk_file.h"
#include "engine/file/file_access.h"
#include "engine/file/game_file.h"
#include "engine/file/mix_archive.h"
#include "engine/file/search_paths.h"
#include "engine/gfx/font.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/wwstd.h"
#include "engine/platform/ftimer.h"
#include "engine/platform/platform.h"
#include "engine/window/keyboard.h"
#include "engine/window/ww_mouse.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/aircraft.h"
#include "ra/anim.h"
#include "ra/audio.h"
#include "ra/base.h"
#include "ra/building.h"
#include "ra/bullet.h"
#include "ra/carry.h"
#include "ra/ccini.h"
#include "ra/ccptr.h"
#include "ra/cell.h"
#include "ra/config.h"
#include "ra/conquer.h"
#include "ra/const.h"
#include "ra/coord.h"
#include "ra/debug_state.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/egos.h"
#include "ra/face.h"
#include "ra/factory.h"
#include "ra/gadget.h"
#include "ra/game_state.h"
#include "ra/goptions.h"
#include "ra/graphics_loader.h"
#include "ra/heap.h"
#include "ra/house.h"
#include "ra/infantry.h"
#include "ra/ini.h"
#include "ra/inline.h"
#include "ra/input.h"
#include "ra/installation.h"
#include "ra/jshell.h"
#include "ra/logic.h"
#include "ra/mapedit.h"
#include "ra/mapsel.h"
#include "ra/mission_id.h"
#include "ra/movie.h"
#include "ra/mplayer.h"
#include "ra/msgbox.h"
#include "ra/msglist.h"
#include "ra/object.h"
#include "ra/object_heaps.h"
#include "ra/overlay.h"
#include "ra/palette.h"
#include "ra/palettes.h"
#include "ra/rules.h"
#include "ra/score.h"
#include "ra/screen.h"
#include "ra/session.h"
#include "ra/smudge.h"
#include "ra/special.h"
#include "ra/startup.h"
#include "ra/taction.h"
#include "ra/team.h"
#include "ra/teamtype.h"
#include "ra/techno.h"
#include "ra/template.h"
#include "ra/terrain.h"
#include "ra/tevent.h"
#include "ra/text_ids.h"
#include "ra/textbtn.h"
#include "ra/theme.h"
#include "ra/trigger.h"
#include "ra/trigtype.h"
#include "ra/type.h"
#include "ra/unit.h"
#include "ra/vector_dynamic.h"
#include "ra/vessel.h"
#include "ra/weapon.h"
#include "ra/wolstrng.h"
#include "ra/world.h"

static void Remove_AI_Players();
static void Create_Units(bool official);
static CELL Clip_Scatter(CELL cell, int maxdist);
static CELL Clip_Move(CELL cell, FacingType facing, int dist);

// Paces the mission briefing: each line is left up for three seconds, or
// until the speech playing over it finishes. Only this file shows briefings.
static Timer<SystemTickSource> speech_timer;

static int build_tech[11] = {
    2, 2,  // Tech level 0 and 1 are the same (tech 0 is never used).
    4, 5, 7, 8, 9, 10, 11, 12, 13};

/***********************************************************************************************
 * ScenarioClass::ScenarioClass -- Constructor for the scenario control object.
 **
 *                                                                                             *
 *    This constructs the default scenario control object. Normally, all the
 *default values    * are meaningless since the act of starting a scenario will
 *fill in all of the values with * settings retrieved from the scenario control
 *file.                                       *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
ScenarioClass::ScenarioClass()
    :

      MissionTimer(0),
      // Not seeded from the rules: Scen is built before Game installs them,
      // and Clear_Scenario() zeroes this at the start of every scenario
      // anyway. LogicClass::AI() refills it from Rule.ShroudRate.
      ShroudTimer(0),

      CarryOverPercent(0),
#define AUTOSONAR_PERIOD (int64_t{kTicksPerSecond} * 40)
      FadeTimer(0),
      AutoSonarTimer(AUTOSONAR_PERIOD) {
  for (int index = 0; index < std::ssize(Waypoint); index++) {
    base::At(Waypoint, index) = -1;
  }
  base::SafeCopy(Description, "");
  base::SafeCopy(ScenarioName, "");
  base::SafeCopy(BriefingText, "");
  base::FillBytes(base::ObjectBytes(GlobalFlags), '\0', sizeof(GlobalFlags));
  base::FillBytes(base::ObjectBytes(Views), '\0', sizeof(Views));
}

/***********************************************************************************************
 * ScenarioClass::Do_BW_Fade -- Cause the palette to temporarily shift to B/W. *
 *                                                                                             *
 *    This routine will start the palette to fade to B/W for a brief moment. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/21/1996 JLB : Created. *
 *=============================================================================================*/
void ScenarioClass::Do_BW_Fade() {
  IsFadingBW = true;
  IsFadingColor = false;
  FadeTimer.Set(kGrayFadeTime);
}

/***********************************************************************************************
 * ScenarioClass::Do_Fade_AI -- Process the palette fading effect. *
 *                                                                                             *
 *    This routine will handle the maintenance of the palette fading effect. It
 *should be      * called once per game frame. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/21/1996 JLB : Created. *
 *=============================================================================================*/
void ScenarioClass::Do_Fade_AI() {
  if (IsFadingColor) {
    if (FadeTimer.IsFinished()) {
      IsFadingColor = false;
    }
    const fixed newsat =
        TheOptions().Get_Saturation() *
        fixed(static_cast<int>(kGrayFadeTime - FadeTimer.Value()),
              static_cast<int>(kGrayFadeTime));
    GameOptionsClass::Adjust_Palette(
        ThePalettes().original_palette(), ThePalettes().game_palette(),
        TheOptions().Get_Brightness(), newsat, TheOptions().Get_Tint(),
        TheOptions().Get_Contrast());
    ThePalettes().game_palette().Set();
  }
  if (IsFadingBW) {
    if (FadeTimer.IsFinished()) {
      IsFadingBW = false;
    }
    const fixed newsat =
        TheOptions().Get_Saturation() *
        fixed(static_cast<int>(FadeTimer.Value()), kGrayFadeTime);
    GameOptionsClass::Adjust_Palette(
        ThePalettes().original_palette(), ThePalettes().game_palette(),
        TheOptions().Get_Brightness(), newsat, TheOptions().Get_Tint(),
        TheOptions().Get_Contrast());
    ThePalettes().game_palette().Set();
    if (!IsFadingBW) {
      IsFadingColor = true;
      FadeTimer.Set(kGrayFadeTime);
    }
  }
}

/***********************************************************************************************
 * ScenarioClass::Set_Global_To -- Set scenario global to value specified. *
 *                                                                                             *
 *    This routine will set the global flag to the falue (true/false) specified.
 *It will       * also scan for and spring any triggers that are dependant upon
 *that global.               *
 *                                                                                             *
 * INPUT:   global   -- The global flag to change. *
 *                                                                                             *
 *          value    -- The value to change the global flag to. *
 *                                                                                             *
 * OUTPUT:  Returns with the previous value of the flag. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/26/1996 JLB : Created. *
 *=============================================================================================*/
bool ScenarioClass::Set_Global_To(int global, bool value) {
  if (static_cast<unsigned>(global) < std::ssize(TheScenario().GlobalFlags)) {
    const bool previous = base::At(GlobalFlags, global);
    if (previous != value) {
      base::At(GlobalFlags, global) = value;
      IsGlobalChanged = true;

      /*
      **	Special case to scan through all triggers and if any are found
      *that depend on this *	global being set/cleared, then if there is an
      *elapsed time event associated, it *	will be reset at this time.
      */
      for (int index = 0; index < TheObjectHeaps().trigger().Count(); index++) {
        TriggerClass* tp = TheObjectHeaps().trigger().Ptr(index);
        if ((tp->Class->Event1.Event == TEVENT_GLOBAL_SET ||
             tp->Class->Event1.Event == TEVENT_GLOBAL_CLEAR) &&
            tp->Class->Event1.Data.Value == global) {
          tp->Class->Event2.Reset(tp->Event1);
        }
        if ((tp->Class->Event2.Event == TEVENT_GLOBAL_SET ||
             tp->Class->Event2.Event == TEVENT_GLOBAL_CLEAR) &&
            tp->Class->Event2.Data.Value == global) {
          tp->Class->Event1.Reset(tp->Event1);
        }
      }
    }
    return previous;
  }
  return false;
}

/***********************************************************************************************
 * Start_Scenario -- Starts the scenario. *
 *                                                                                             *
 *    This routine will start the scenario. In addition to loading the scenario
 *data, it will  * play the briefing and action movies. *
 *                                                                                             *
 * INPUT:   root     -- Pointer to the filename root for this scenario (e.g.,
 *"SCG01EA").      *
 *                                                                                             *
 *          briefing -- Should the briefing be played? Normally this is true
 *except when the   * scenario is restarting. *
 *                                                                                             *
 * OUTPUT:  Was the scenario started without error? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/04/1995 JLB : Created. *
 *=============================================================================================*/
bool Start_Scenario(char* name, bool briefing) {
  TheTheme().Stop();
  TheWorld().is_tanya_dead() = TheWorld().save_tanya();
  if (!Read_Scenario(name)) {
    return false;
  }

  /*
  **	Play the winning movie and then start the next scenario.
  */
  TheGameState().required_cd() = -1;
  //	if (RequiredCD != -2 && Session.Type == GAME_NORMAL) {
  //		if (Scen.Scenario == 1)
  //			RequiredCD = -1;
  //		else {
  //			 if((Scen.Scenario >= 20 && Scen.ScenarioName[2] == 'G'
  //|| Scen.ScenarioName[2] == 'U') || Scen.ScenarioName[2] == 'A'
  //				|| (Scen.ScenarioName[2] == 'M' && Scen.Scenario
  //>= 25)) 		       	    RequiredCD = 2; 			 else
  // if(Scen.ScenarioName[2] == 'U') 			    RequiredCD = 1;
  // else if(Scen.ScenarioName[2] == 'G') 			    RequiredCD =
  // 0;
  //			}
  //

  //   	}
  TheTheme().Stop();

  if (briefing) {
    Hide_Mouse();
    TheScreen().visible_page().view().Clear();
    Show_Mouse();
    Play_Movie(TheScenario().IntroMovie);
    Play_Movie(TheScenario().BriefMovie);
  }

  /*
  ** If there's no briefing movie, restate the mission at the beginning.
  */
  char buffer[25];
  if (TheScenario().BriefMovie != VQ_NONE) {
    absl::SNPrintF(buffer, sizeof(buffer), "%s.VQA",
                   VQName.at(TheScenario().BriefMovie));
  }
  if (TheSession().Type == GAME_NORMAL &&
      (TheScenario().BriefMovie == VQ_NONE || !GameFileExists(buffer))) {
    /*
    ** Make sure the mouse is visible before showing the restatement.
    */
    while (Get_Mouse_State()) {
      Show_Mouse();
    }
    Restate_Mission();
  }

  if (briefing) {
    Hide_Mouse();
    TheScreen().visible_page().view().Clear();
    Show_Mouse();
    Play_Movie(TheScenario().ActionMovie, TheScenario().TransitTheme);
  }

  if (TheScenario().TransitTheme == THEME_NONE) {
    TheTheme().Queue_Song(magic_enum::enum_values<ThemeType>().front());
  }

  /*
  ** Set the options values, since the palette has been initialized by
  *Read_Scenario
  */
  TheOptions().Set();

  return true;
}

/***********************************************************************************************
 * Read_Scenario -- Reads a scenario from disk. *
 *                                                                                             *
 *    This will read a scenario from disk. Use this to begin a scenario. * It
 *doesn't perform any rendering, it merely sets up the system * with the proper
 *data. Setting of the right game state will start                         * the
 *scenario running. *
 *                                                                                             *
 * INPUT:   root     -- Scenario root filename *
 *                                                                                             *
 * OUTPUT:     none *
 *                                                                                             *
 * WARNINGS:   You must clear out the system variables before calling * this
 *function. Use the Clear_Scenario() function.                               *
 *               It is assumed that Scenario is set to the current scenario
 *number.            *
 *                                                                                             *
 * HISTORY: * 07/22/1991     : Created. * 02/03/1992 JLB : Uses house
 *identification.                                               *
 *=============================================================================================*/
bool Read_Scenario(char* name) {
  Clear_Scenario();
  TheWorld().scenario_init()++;
  if (Read_Scenario_INI(name)) {
    bool readini = false;
    switch (TheSession().Type) {
      case GAME_NORMAL:
        readini = false;
        break;
      case GAME_SKIRMISH:
      case GAME_INTERNET:
      case GameType::GAME_MODEM:
      case GameType::GAME_NULL_MODEM:
      case GameType::GAME_IPX:
      default:
        readini = TheSession().IsAftermath;
        break;
    }
    if (readini) {
      /*
      ** Find out if the CD in the current drive is the Aftermath disc.
      */
      const int cd_index =
          Get_CD_Index(SearchPaths::current_cd_drive(), 1 * 60);
      if ((!Using_DVD() || cd_index != 5) && cd_index != 3) {
        ThePalettes().game_palette().Set(kFadePaletteFast, ServiceRealTime);
        TheGameState().required_cd() = 3;
        if (!Force_CD_Available(
                TheGameState()
                    .required_cd())) {  // force Aftermath CD in drive.
          EmergencyExit(EXIT_FAILURE);
        }
      }
      CCINIClass ini;
      if (const auto fc = OpenGameFile("MPLAYER.INI");
          fc && ini.Load(*fc, false)) {
        TheRules().General(ini);
        TheRules().Recharge(ini);
        TheRules().AI(ini);
        TheRules().Powerups(ini);
        TheRules().Land_Types(ini);
        RulesClass::Themes(ini);
        TheRules().IQ(ini);
        TheRules().Objects(ini);
        TheRules().Difficulty(ini);
      }
    }
    Fill_In_Data();
  } else {
    ThePalettes().game_palette().Set(kFadePaletteFast, ServiceRealTime);
    //		Fade_Palette_To(GamePalette, kFadePaletteFast, ServiceRealTime);
    Show_Mouse();
    WWMessageBox().Process(TXT_UNABLE_READ_SCENARIO);
    Hide_Mouse();
    return false;
  }
  TheWorld().scenario_init()--;
  return true;
}

/***********************************************************************************************
 * Fill_In_Data -- Recreate all data that is not loaded with scenario. *
 *                                                                                             *
 *    This routine is called after the INI file for the scenario has been
 *processed. It will   * infer the game state from the scenario INI data. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/07/1992 JLB : Created. *
 *=============================================================================================*/
void Fill_In_Data() {
  /*
  **	The basic scenario data load does not contain the full set of
  **	game data. We now must fill in the missing pieces.
  */
  TheWorld().scenario_init()++;

  for (int index = 0; index < TheObjectHeaps().building().Count(); index++) {
    TheObjectHeaps().building().Ptr(index)->Update_Buildables();
  }

  TheMap().Flag_To_Redraw(true);

  /*
  **	Reset the movement zones according to the terrain passability.
  */
  TheMap().Zone_Reset(kZoneFlagAll);

  /*
  **	Since the sidebar starts up activated, adjust the home start position so
  *that *	the right edge of the map will still be visible.
  */
  if (!TheDebugState().map_editor_active()) {
    TheMap().Activate(1);
    //		if (Session.Type == GAME_NORMAL) {
    base::At(TheScenario().Views, 0) = base::At(TheScenario().Views, 1) =
        base::At(TheScenario().Views, 2) = base::At(TheScenario().Views, 3) =
            base::At(TheScenario().Waypoint, ScenarioClass::kHomeWaypoint);
    TheMap().Set_Tactical_Position(Cell_Coord(static_cast<CELL>(
        base::At(TheScenario().Waypoint, ScenarioClass::kHomeWaypoint) -
        (MAP_CELL_W * 8) - 10)));
    //		}
  }

  /*
  **	Handle any data resetting that can be safely inferred from the actual
  **	data that has been loaded.
  */
  /*
  **	Distribute the trigger pointers to the appropriate working lists.
  */
  for (int index = 0; index < TheObjectHeaps().trigger_type().Count();
       index++) {
    TriggerTypeClass* tp = TheObjectHeaps().trigger_type().Ptr(index);

    DCHECK(tp != nullptr);

    if (base::Any(tp->Attaches_To() & ATTACH_MAP)) {
      TheWorld().map_triggers().Add(Find_Or_Make(tp));
    }
    if (base::Any(tp->Attaches_To() & ATTACH_GENERAL)) {
      TheWorld().logic_triggers().Add(Find_Or_Make(tp));
    }
    if (base::Any(tp->Attaches_To() & ATTACH_HOUSE)) {
      TheWorld().house_triggers().at(tp->House).Add(Find_Or_Make(tp));
    }
  }

  TheWorld().scenario_init()--;

  /*
  ** Now go through and set all the cells ringing the map to be visible, so
  ** we won't get the wall of shadow at the edge of the map.
  */
  for (int x = TheMap().MapCellX - 1;
       x < TheMap().MapCellX + TheMap().MapCellWidth + 1; x++) {
    TheMap().at(XY_Cell(x, TheMap().MapCellY - 1)).IsVisible =
        TheMap().at(XY_Cell(x, TheMap().MapCellY - 1)).IsMapped = true;

    TheMap()
        .at(XY_Cell(x, TheMap().MapCellY + TheMap().MapCellHeight))
        .IsVisible =
        TheMap()
            .at(XY_Cell(x, TheMap().MapCellY + TheMap().MapCellHeight))
            .IsMapped = true;
  }
  for (int y = TheMap().MapCellY;
       y < TheMap().MapCellY + TheMap().MapCellHeight; y++) {
    TheMap().at(XY_Cell(TheMap().MapCellX - 1, y)).IsVisible =
        TheMap().at(XY_Cell(TheMap().MapCellX - 1, y)).IsMapped = true;
    TheMap()
        .at(XY_Cell(TheMap().MapCellX + TheMap().MapCellWidth, y))
        .IsVisible =
        TheMap()
            .at(XY_Cell(TheMap().MapCellX + TheMap().MapCellWidth, y))
            .IsMapped = true;
  }

  /*
  **	If inheriting from a previous scenario was indicated, then create the
  *carry over *	objects at this time.
  */
  if (TheScenario().IsToInherit) {
    for (const auto& object : TheWorld().carryover()) {
      object.Create();
    }
  }

  /*
  **	The "allow win" action is a special case that is handled here. The total
  *number *	of triggers that have this action must be recorded.
  */
  for (int index = 0; index < TheObjectHeaps().trigger_type().Count();
       index++) {
    const TriggerTypeClass* tp = TheObjectHeaps().trigger_type().Ptr(index);
    if (tp->Action1.Action == TACTION_ALLOWWIN ||
        (tp->ActionControl != MULTI_ONLY &&
         tp->Action2.Action == TACTION_ALLOWWIN)) {
      HouseClass::As_Pointer(tp->House)->Blockage++;
    }
  }

  /*
  **	Move available money to silos, if the scenario flag so indicates.
  */
  if (TheScenario().IsMoneyTiberium) {
    for (const HousesType house : magic_enum::enum_values<HousesType>()) {
      HouseClass* hptr = HouseClass::As_Pointer(house);
      if (hptr != nullptr) {
        const int tomove = static_cast<int>(hptr->Capacity - hptr->Tiberium);
        hptr->Credits -= tomove;
        hptr->Tiberium += tomove;
      }
    }
  }

  /*
  **	Count all non-destroyed bridges on the map.
  */
  TheScenario().BridgeCount = TheMap().Intact_Bridge_Count();

  MapEditClass::All_To_Look(true);
}

/***********************************************************************************************
 * Post_Load_Game -- Fill in an inferred data from the game state. *
 *                                                                                             *
 *    This routine is typically called after a game has been loaded. Some
 *working data lists   * can be rebuild from the game state. This working data
 *is rebuilt rather than being       * stored with the game data file. *
 *                                                                                             *
 * INPUT:   load_multi -- true if we're loading a multiplayer game *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   Although it is safe to call this routine whenever, it is only
 *needed after a    * game load. *
 *                                                                                             *
 * HISTORY: * 11/30/1995 JLB : Created. *
 *=============================================================================================*/
void Post_Load_Game(int load_multi) {
  //
  // Do NOT call Overpass if we're loading a multiplayer game; it calls the
  // random # generator, which throws the games out of sync if they were
  // saved on different frame #'s.
  //
  if (!load_multi) {
    TheMap().Overpass();
  }
  TheScenario().BridgeCount = TheMap().Intact_Bridge_Count();
  TheMap().Zone_Reset(kZoneFlagAll);
}

/***********************************************************************************************
 * Clear_Scenario -- Clears all data in preparation for scenario load. *
 *                                                                                             *
 *    This routine will clear out all data specific to a scenario in *
 *    preparation for a subsequent scenario data load. This will free * all
 *units, animations, and icon maps. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/22/1991     : Created. * 03/21/1992 JLB : Changed buffer
 *allocations, so changes memset code.                      * 07/13/1995 JLB :
 *End count down moved here.                                               *
 *=============================================================================================*/
void Clear_Scenario() {
  // TCTCTC -- possibly just use in-place new of scenario object?

  TheScenario().MissionTimer.Set(0);
  TheScenario().MissionTimer.Stop();
  TheScenario().ElapsedTime.Reset();
  TheScenario().ShroudTimer.Set(0);
  TheScenario().IntroMovie = VQ_NONE;
  TheScenario().BriefMovie = VQ_NONE;
  TheScenario().WinMovie = VQ_NONE;
  TheScenario().LoseMovie = VQ_NONE;
  TheScenario().ActionMovie = VQ_NONE;
  TheScenario().IsNoSpyPlane = false;
  TheScenario().IsTanyaEvac = false;
  TheScenario().IsEndOfGame = false;
  TheScenario().IsInheritTimer = false;
  TheScenario().IsToCarryOver = false;
  TheScenario().IsSkipScore = false;
  TheScenario().IsOneTimeOnly = false;
  TheScenario().IsTruckCrate = false;
  TheScenario().IsMoneyTiberium = false;
  TheScenario().IsNoMapSel = false;
  TheScenario().CarryOverCap = 0;
  TheScenario().CarryOverPercent = fixed(0);
  TheScenario().TransitTheme = THEME_NONE;
  TheScenario().Percent = 0;

  base::FillBytes(base::ObjectBytes(TheScenario().GlobalFlags), 0,
                  sizeof(TheScenario().GlobalFlags));

  TheWorld().map_triggers().Clear();
  TheWorld().logic_triggers().Clear();

  for (const HousesType house : magic_enum::enum_values<HousesType>()) {
    TheWorld().house_triggers().at(house).Clear();
  }

  /*
  ** Call everyone's Init routine, except the Map's; for the Map, only call
  ** MapClass::Init, which clears the Cell array.  The Display::Init requires
  ** a Theater argument, and the theater is not known at this point; also, it
  ** would reload MixFiles, which isn't desired.  Display::Read_INI calls its
  ** own Init, which will Init the entire Map hierarchy.
  */
  TheMap().Init_Clear();
  TheWorld().score().Init();
  TheWorld().logic().Init();

  HouseClass::Init();
  ObjectClass::Init();
  TeamTypeClass::Init();
  TeamClass::Init();
  TriggerClass::Init();
  TriggerTypeClass::Init();
  AircraftClass::Init();
  AnimClass::Init();
  BuildingClass::Init();
  BulletClass::Init();
  InfantryClass::Init();
  OverlayClass::Init();
  SmudgeClass::Init();
  TemplateClass::Init();
  TerrainClass::Init();
  UnitClass::Init();
  VesselClass::Init();

  FactoryClass::Init();

  TheWorld().base().Init();

  TheWorld().current_object().Clear();

  for (int16_t& index : TheScenario().Waypoint) {
    index = -1;
  }

  // For endgame auto-sonar pulse.
  TheWorld().auto_sonar_pulse() = false;

  // Stalemate games.
  TheScenario().bLocalProposesDraw = false;
  TheScenario().bOtherProposesDraw = false;
}

/***********************************************************************************************
 * Do_Win -- Display winning congratulations. *
 *                                                                                             *
 *    Perform the win the mission process. This will display any winning movies
 *and the score  * screen. Followed by the map selection screen and then the
 *load of the new scenario.      *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/05/1992 JLB : Created. * 01/01/1995 JLB : Carries money forward
 *into next scenario.                                *
 *=============================================================================================*/
void Do_Win() {
  TheMap().Set_Default_Mouse(MOUSE_NORMAL);
  Hide_Mouse();
  TheTheme().Queue_Song(THEME_QUIET);

  /*
  ** If this is a multiplayer game, clear the game's name so we won't respond
  ** to game queries any more (in ServiceRealTime)
  */
  if (TheSession().Type != GAME_NORMAL) {
    base::At(TheSession().GameName, 0) = 0;
  }

  /*
  **	Determine a cosmetic center point for the text.
  */
  const int x =
      TheMap().TacPixelX + (Lepton_To_Pixel(TheMap().TacLeptonWidth) / 2);

  /*
  ** Hack section.  If it's allied scenario 10, variation A, then skip the
  ** score and map selection, don't increment scenario, and set it to
  ** variation B.
  */
  if (TheSession().Type != GAME_NORMAL || !TheScenario().IsSkipScore ||
      TheWorld().ants_enabled()) {
    /*
    **	Announce win to player.
    */
    PixelView& view = TheScreen().visible_view();
    TheMap().Flag_To_Redraw(true);
    TheMap().Render();
    Fancy_Text_Print(view, TXT_SCENARIO_WON, x, 180,
                     &ThePalettes().color_remaps().at(PCOLOR_RED), kTBlack,
                     TPF_CENTER | TPF_VCR | TPF_USE_GRAD_PAL | TPF_DROPSHADOW);
    speech_timer.Set(int64_t{kTimerSecond} * 3);
    while (IsSpeaking()) {
    }
    Speak(VOX_ACCOMPLISHED);
    while (speech_timer.HasTimeLeft() || IsSpeaking()) {
      ServiceRealTime();
    }
  }

  /*
  ** Stop here if this is a multiplayer game.
  */
  if (TheSession().Type != GAME_NORMAL) {
    if (!TheSession().Play) {
      TheSession().GamesPlayed++;
      Multi_Score_Presentation();
      TheSession().CurGame++;
      if (TheSession().CurGame >= MAX_MULTI_GAMES) {
        TheSession().CurGame = MAX_MULTI_GAMES - 1;
      }
    }
    TheGameState().active() = false;
    Show_Mouse();
    return;
  }

  Hide_Mouse();
  TheScreen().visible_page().view().Clear();
  Show_Mouse();
  Play_Movie(TheScenario().WinMovie);

  TheKeyboard().Clear();

  TheWorld().save_tanya() = TheWorld().is_tanya_dead();
  TheScenario().CarryOverTimer =
      static_cast<int>(TheScenario().MissionTimer.Value());
  //	int timer = Scen.MissionTimer;

  /*
  **	Do the ending screens only if not playing back a recorded game.
  */
  if (!TheSession().Play) {
    /*
    **	If the score presentation should be performed, then do
    **	so now.
    */
    TheKeyboard().Clear();
    if (!TheScenario().IsSkipScore) {
      TheWorld().score().Presentation();
    }

    if (TheScenario().IsOneTimeOnly) {
      TheGameState().active() = false;
      Show_Mouse();
      TheWorld().ants_enabled() = false;
      return;
    }

    /*
    ** If this scenario is flagged as ending the game then print the credits and
    *exit.
    */
    if (TheScenario().IsEndOfGame) {
      if (ThePlayer()->ActLike == HOUSE_USSR) {
        Play_Movie(VQ_SOVFINAL);
      } else {
        Play_Movie(VQ_ALLYEND);
      }
      Show_Who_Was_Responsible();
      TheGameState().active() = false;
      Show_Mouse();
      TheWorld().ants_enabled() = false;
      return;
    }

    /*
    ** Hack section.  If it's allied scenario 10, variation A, then skip the
    ** score and map selection, don't increment scenario, and set it to
    ** variation B.
    */
    if (TheWorld().ants_enabled()) {
      // The ant campaign has neither a mission map nor variants.
      TheScenario().AdvanceToNextScenario();
    } else if (TheScenario().IsNoMapSel) {
      // force it to play the second half of scenario 10
      TheScenario().SetScenarioVariant(SCEN_VAR_B);
    } else {
      TheScenario().AdvanceToNextScenario(ChooseMissionVariant());
    }

    TheKeyboard().Clear();
  }

  TheScenario().CarryOverMoney = static_cast<int>(ThePlayer()->Credits);

  /*
  **	If requested, record the scenario's objects in the carry over list
  **	for possible use in a future scenario.
  */
  if (TheScenario().IsToCarryOver) {
    /*
    **	First delete any existing carry over list. Any old list will be
    **	blasted over by the new list -- there is only one logic carryover
    **	list to be maintained.
    */
    TheWorld().carryover().clear();

    /*
    **	Record all objects, that are to be part of the carry over set, into
    **	the carry over list.
    */
    for (int building_index = 0;
         building_index < TheObjectHeaps().building().Count();
         building_index++) {
      BuildingClass* building = TheObjectHeaps().building().Ptr(building_index);

      if (building && !building->IsInLimbo && building->Strength > 0) {
        TheWorld().carryover().emplace_back(building);
      }
    }
    for (int unit_index = 0; unit_index < TheObjectHeaps().unit().Count();
         unit_index++) {
      UnitClass* unit = TheObjectHeaps().unit().Ptr(unit_index);

      if (unit && !unit->IsInLimbo && unit->Strength > 0) {
        TheWorld().carryover().emplace_back(unit);
      }
    }
    for (int infantry_index = 0;
         infantry_index < TheObjectHeaps().infantry().Count();
         infantry_index++) {
      InfantryClass* infantry = TheObjectHeaps().infantry().Ptr(infantry_index);

      if (infantry && !infantry->IsInLimbo && infantry->Strength > 0) {
        TheWorld().carryover().emplace_back(infantry);
      }
    }
    for (int vessel_index = 0; vessel_index < TheObjectHeaps().vessel().Count();
         vessel_index++) {
      VesselClass* vessel = TheObjectHeaps().vessel().Ptr(vessel_index);

      if (vessel && !vessel->IsInLimbo && vessel->Strength > 0) {
        TheWorld().carryover().emplace_back(vessel);
      }
    }
  }

  /*
  ** Generate a new scenario filename
  */
  //	Scen.Set_Scenario_Name(Scen.Scenario, Scen.ScenPlayer, Scen.ScenDir,
  // Scen.ScenVar);
  Start_Scenario(TheScenario().ScenarioName);

  /*
  **	If the mission timer is to be inheriteded from the previous scenario
  *then do it now.
  */
  if (TheScenario().IsInheritTimer) {
    TheScenario().MissionTimer.Set(TheScenario().CarryOverTimer);
    TheScenario().MissionTimer.Start();
  }

  //	PlayerPtr->NukePieces = nukes;

  TheMap().Render();
  ThePalettes().game_palette().Set(kFadePaletteFast, ServiceRealTime);
  //	Fade_Palette_To(GamePalette, kFadePaletteFast, ServiceRealTime);
  Show_Mouse();
}

/***********************************************************************************************
 * Do_Lose -- Display losing comments. *
 *                                                                                             *
 *    Performs the lose mission processing. This will generally display a "would
 *you like      * to replay" dialog and then either reload the scenario or set
 *flags such that the main    * menu will appear. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/05/1992 JLB : Created. *
 *=============================================================================================*/
void Do_Lose() {
  TheMap().Set_Default_Mouse(MOUSE_NORMAL);
  Hide_Mouse();

  TheTheme().Queue_Song(THEME_QUIET);

  /*
  ** If this is a multiplayer game, clear the game's name so we won't respond
  ** to game queries any more (in ServiceRealTime)
  */
  if (TheSession().Type != GAME_NORMAL) {
    base::At(TheSession().GameName, 0) = 0;
  }

  /*
  **	Determine a cosmetic center point for the text.
  */
  const int x =
      TheMap().TacPixelX + (Lepton_To_Pixel(TheMap().TacLeptonWidth) / 2);

  /*
  **	Announce win to player.
  */
  PixelView& view = TheScreen().visible_view();
  Fancy_Text_Print(view, TXT_SCENARIO_LOST, x, 180,
                   &ThePalettes().color_remaps().at(PCOLOR_RED), kTBlack,
                   TPF_CENTER | TPF_VCR | TPF_USE_GRAD_PAL | TPF_DROPSHADOW);
  speech_timer.Set(int64_t{kTimerSecond} * 3);
  while (IsSpeaking()) {
  }
  Speak(VOX_FAIL);
  while (speech_timer.HasTimeLeft() || IsSpeaking()) {
    ServiceRealTime();
  }

  /*
  ** Stop here if this is a multiplayer game.
  */
  if (TheSession().Type != GAME_NORMAL) {
    if (!TheSession().Play) {
      TheSession().GamesPlayed++;
      Multi_Score_Presentation();
      TheSession().CurGame++;
      if (TheSession().CurGame >= MAX_MULTI_GAMES) {
        TheSession().CurGame = MAX_MULTI_GAMES - 1;
      }
    }
    TheGameState().active() = false;
    Show_Mouse();
    return;
  }

  Hide_Mouse();
  TheScreen().visible_page().view().Clear();
  Show_Mouse();
  DLOG(INFO) << "Trying to play lose movie";
  Play_Movie(TheScenario().LoseMovie);

  /*
  ** Start same scenario again
  */
  ThePalettes().game_palette().Set();
  Show_Mouse();
  if (!TheSession().Play &&
      !WWMessageBox().Process(TXT_TO_REPLAY, TXT_YES, TXT_NO)) {
    Hide_Mouse();
    TheKeyboard().Clear();
    Start_Scenario(TheScenario().ScenarioName, false);

    /*
    **	Start the scenario timer with the carried over value if necessary.
    */
    if (TheScenario().IsInheritTimer) {
      TheScenario().MissionTimer.Set(TheScenario().CarryOverTimer);
      TheScenario().MissionTimer.Start();
    }

    TheMap().Render();
  } else {
    Hide_Mouse();
    TheGameState().active() = false;
  }

  ThePalettes().game_palette().Set(kFadePaletteFast, ServiceRealTime);
  Show_Mouse();
}

/***********************************************************************************************
 * Do_Draw -- Parallels Do_Win and Do_Lose, for multiplayer games that end in a
 *draw.
 *=============================================================================================*/
void Do_Draw() {
  TheMap().Set_Default_Mouse(MOUSE_NORMAL);
  Hide_Mouse();

  TheTheme().Queue_Song(THEME_QUIET);

  /*
  ** If this is a multiplayer game, clear the game's name so we won't respond
  ** to game queries any more (in ServiceRealTime)
  */
  if (TheSession().Type != GAME_NORMAL) {
    base::At(TheSession().GameName, 0) = 0;
  }

  /*
  **	Determine a cosmetic center point for the text.
  */
  const int x =
      TheMap().TacPixelX + (Lepton_To_Pixel(TheMap().TacLeptonWidth) / 2);

  /*
  **	Announce win to player.
  */
  PixelView& view = TheScreen().visible_view();
  Fancy_Text_Print(view, TXT_WOL_DRAW, x, 180,
                   &ThePalettes().color_remaps().at(PCOLOR_RED), kTBlack,
                   TPF_CENTER | TPF_VCR | TPF_USE_GRAD_PAL | TPF_DROPSHADOW);
  speech_timer.Set(int64_t{kTimerSecond} * 3);
  while (IsSpeaking()) {
  }
  Speak(VOX_CONTROL_EXIT);
  while (speech_timer.HasTimeLeft() || IsSpeaking()) {
    ServiceRealTime();
  }

  /*
  ** Stop here if this is a multiplayer game.
  */
  if (!TheSession().Play) {
    TheSession().GamesPlayed++;
    Multi_Score_Presentation();
    TheSession().CurGame++;
    if (TheSession().CurGame >= MAX_MULTI_GAMES) {
      TheSession().CurGame = MAX_MULTI_GAMES - 1;
    }
  }
  TheGameState().active() = false;
  Show_Mouse();
}

/***********************************************************************************************
 * Do_Restart -- Handle the restart mission process. *
 *                                                                                             *
 *    This routine is called in the main game loop when the mission must be
 *restarted. This    * routine will throw away the current game and reload the
 *appropriate mission. The         * game will "resume" at the start of the
 *mission.                                          *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/24/1995 JLB : Created. *
 *=============================================================================================*/
void Do_Restart() {
  /*
  ** Start a timer going, before we restart the scenario
  */
  Timer<SystemTickSource> timer;
  timer.Set(int64_t{kTicksPerSecond} * 4);
  TheTheme().Queue_Song(THEME_QUIET);

  WWMessageBox().Process(TXT_RESTARTING, TXT_NONE);

  TheMap().Set_Default_Mouse(MOUSE_NORMAL);
  TheKeyboard().Clear();
  Start_Scenario(TheScenario().ScenarioName, false);

  /*
  **	Start the scenario timer with the carried over value if necessary.
  */
  if (TheScenario().IsInheritTimer) {
    TheScenario().MissionTimer.Set(TheScenario().CarryOverTimer);
    TheScenario().MissionTimer.Start();
  }

  /*
  ** Make sure the message stays displayed for at least 1 second
  */
  while (timer.HasTimeLeft()) {
    ServiceRealTime();
  }
  TheKeyboard().Clear();

  TheMap().Render();
}

BriefingAction Restate_Mission() {
  if (std::string_view(TheScenario().ScenarioName).empty() ||
      std::string_view(TheScenario().BriefingText).empty()) {
    return BriefingAction::kResume;
  }

  // Check if briefing video is available.
  bool has_video = false;
  if (TheScenario().BriefMovie != VQ_NONE) {
    const auto video_filename =
        std::string(VQName.at(TheScenario().BriefMovie)) + ".VQA";
    has_video = GameFileExists(video_filename);
  }

  // Choose buttons based on video availability.
  const int resume_btn = has_video ? TXT_RESUME_MISSION : TXT_OK;
  const int video_btn = has_video ? TXT_VIDEO : TXT_NONE;

  // Display mission briefing.
  const int clicked =
      ShowBriefingMessageBox(TheScenario().BriefingText, resume_btn, video_btn);
  if (clicked == video_btn && has_video) {
    return BriefingAction::kPlayVideo;
  }

  return BriefingAction::kResume;
}

static constexpr int kButton1 = 1;
static constexpr int kButton2 = 2;
static constexpr int kButton3 = 3;
static constexpr uint32_t kBriefingButtonFlag =
    0x8000;  // Set in a pressed button's key.

// Maximum characters to display per page of briefing text.
static constexpr size_t kMaxCharsPerPage = 512;

int ShowBriefingMessageBox(std::string_view msg, int left_btn, int right_btn,
                           bool fade_to_black) {
  if (fade_to_black) {
    ThePalettes().black_palette().Set(kFadePaletteMedium, ServiceRealTime);
  }

  int retval = 0;

  // Track which text ID each button position represents after shifting.
  int left_btn_text_id = left_btn;
  const int right_btn_text_id = right_btn;
  int selection = 0;
  TextButtonClass* buttons[3];
  bool display = true;  // display level
  int realval[5];
  int morebutton = 3;  // which button says "more": 2 or 3?

  const char* b1txt = Text_String(left_btn);
  const char* b2txt = Text_String(right_btn);
  const char* b3txt = [] {
    if (config::kIsFrench) {
      return "SUITE";
    }
    if (config::kIsGerman) {
      return "MEHR";
    }
    return "MORE";
  }();

  const auto briefsnd = MixArchive::RetrieveData("BRIEFING.AUD");

  GadgetClass::Set_Color_Scheme(&ThePalettes().color_remaps().at(PCOLOR_TYPE));

  // If the message fits on one page, hide the "MORE" button.
  if (msg.size() < kMaxCharsPerPage) {
    b3txt = "";
  }

  /*
  ** If there's no text for button one, zero it out.
  */
  if (*b1txt == '\0') {
    b1txt = b2txt;
    b2txt = "";
    left_btn_text_id = right_btn;  // Button 1 now shows right_btn's text.
    if (*b1txt == '\0') {
      b1txt = nullptr;
    }
  }

  /*
  ** If there's no text for button two, zero it out.  However, if there
  ** is text for button three, move its text (always "MORE") to button two,
  ** and set the morebutton flag to point to button two.  Then, clear out
  ** button 3.
  */
  if (*b2txt == '\0') {
    b2txt = nullptr;
    if (*b3txt != '\0') {
      b2txt = b3txt;
      b3txt = "";
      morebutton = 1;
    }
  }

  /*
  ** If there's no text for button three, zero it out.
  */
  if (*b3txt == '\0') {
    b3txt = nullptr;
  }

  PixelView& view = TheScreen().visible_view();
  // The buttons and the page of text are both laid out in this font.
  const FontStyle font = TextFontStyle(TPF_6PT_GRAD | TPF_USE_GRAD_PAL);
  /*
  **	Examine the optional button parameters. Fetch the width and starting
  **	characters for each.
  */
  char b1char = '\0';
  char b2char = '\0';
  char b3char = '\0';           // 1st char of each string
  int bwidth = 0;               // button width and height
  int bheight = 0;
  int numbuttons = 0;
  if (b1txt) {
    b1char = static_cast<char>(toupper(b1txt[0]));

    /*
    **	Build the button list.
    */
    bheight = FontLineHeight(font) + 2;
    bwidth = std::max(StringPixelWidth(font, b1txt) + 8, 80);
    if (b2txt) {
      numbuttons = 2;
      b2char = static_cast<char>(toupper(b2txt[0]));
      bwidth = std::max(StringPixelWidth(font, b2txt) + 8, bwidth);
      //			b1x = x + 10;
      //// left side

      if (b3txt) {
        numbuttons = 3;
        b3char = static_cast<char>(toupper(b3txt[0]));
        bwidth = std::max(StringPixelWidth(font, b3txt) + 8, bwidth);
      }

    } else {
      numbuttons = 1;
      //			b1x = x + ((width - bwidth) >> 1);
      //// centered
    }
  }

  // Determine the portion of text to display on this page.
  // If text is longer than one page, truncate at the last space.
  std::string_view page_text = msg.substr(0, kMaxCharsPerPage - 1);
  if (page_text.size() < msg.size()) {
    const size_t last_space = page_text.rfind(' ');
    if (last_space != std::string_view::npos) {
      page_text = page_text.substr(0, last_space);
    }
  }

  // Buffer for Format_Window_String which modifies the string for word wrap.
  char buffer[kMaxCharsPerPage];
  // Copy to mutable buffer for Format_Window_String (which inserts newlines).
  page_text.copy(buffer, page_text.size());
  base::At(buffer, page_text.size()) = '\0';
  int width = 0;
  int height = 0;
  Format_Window_String(font, buffer, 300, width, height);
  height += numbuttons == 0 ? 30 : 60;

  const int x = (view.width() - width) / 2;
  const int y = (view.height() - height) / 2;

  /*
  **	Initialize the button structures. All are initialized, even though one
  *(or none) may *	actually be added to the button list.
  */
  TextButtonClass button1(kButton1, b1txt, kTpfButton,
                          x + (numbuttons == 1 ? (width - bwidth) / 2 : 10),
                          y + height - (bheight + 5), bwidth);

  TextButtonClass button2(kButton2, b2txt, kTpfButton,
                          x + width - (bwidth + 10), y + height - (bheight + 5),
                          bwidth);

  TextButtonClass button3(kButton3, b3txt, kTpfButton, 0,
                          y + height - (bheight + 5));
  button3.X = x + ((width - button3.Width) / 2);

  TextButtonClass* buttonlist = nullptr;
  int curbutton = 0;

  /*
  **	Add and initialize the buttons to the button list.
  */
  if (numbuttons) {
    buttonlist = &button1;
    base::At(buttons, 0) = &button1;
    base::At(realval, 0) = kButton1;
    if (numbuttons > 2) {
      button3.Add(*buttonlist);
      base::At(buttons, 1) = &button3;
      base::At(realval, 1) = kButton3;
      button2.Add(*buttonlist);
      base::At(buttons, 2) = &button2;
      base::At(realval, 2) = kButton2;
      base::At(buttons, curbutton)->Turn_On();
    } else if (numbuttons == 2) {
      button2.Add(*buttonlist);
      base::At(buttons, 1) = &button2;
      base::At(realval, 1) = kButton2;
      base::At(buttons, curbutton)->Turn_On();
    }
  }

  /*
  **	Draw the dialog.
  */
  Hide_Mouse();

  PaletteClass temp;
  const char* filename = "SOVPAPER.PCX";
  if (!IsSovietHouse(ThePlayer()->Class->House)) {
    filename = "ALIPAPER.PCX";
  }
  Load_Title_Screen(filename, &TheScreen().hidden_view(), temp);
  TheScreen().hidden_view().BlitTo(TheScreen().visible_view());

  static const unsigned char _scorepal[] = {0, 1, 12, 13,  4,   5,   6,  7,
                                            8, 9, 10, 255, 252, 253, 14, 248};
  temp.Set(kFadePaletteMedium, ServiceRealTime);

  // Main Processing Loop.

  int bufindex = 0;

  TheKeyboard().Clear();

  // The text types out in the font the button constructors above selected
  // last (each measures itself in kTpfButton), recoloured through _scorepal.
  // That is not the style the page was wrapped in: it sets letters a pixel
  // closer and lines two pixels closer.
  FontStyle typed = TextFontStyle(kTpfButton);
  std::ranges::copy(_scorepal, typed.palette.begin());
  int xprint = x + 20;
  int yprint = y + 25;
  do {
    char bufprint[2];
    base::At(bufprint, 1) = 0;
    base::At(bufprint, 0) = base::At(buffer, bufindex);
    if (base::At(bufprint, 0) == '\r' || base::At(bufprint, 0) == '@') {
      xprint = x + 20;
      yprint += FontLineHeight(typed);

    } else {
      if (base::At(bufprint, 0) != 20) {
        TheScreen().visible_view().Print(typed, bufprint, xprint, yprint,
                                         kTBlack, kTBlack);
        xprint += CharPixelWidth(typed, base::At(bufprint, 0));
      }
    }
    if (base::At(bufprint, 0) == '\r' || base::At(bufprint, 0) == '@') {
      engine::audio::TheAudio().Play(briefsnd, 255,
                                     TheOptions().Normalize_Volume(135));
      Timer<SystemTickSource> cd;
      cd.Set(5);
      do {
        ServiceRealTime();
      } while (!TheKeyboard().Check() && cd.HasTimeLeft());
    }
  } while (base::At(buffer, ++bufindex));

  Show_Mouse();
  TheKeyboard().Clear();

  if (buttonlist) {
    bool process = true;  // loop while true
    bool pressed = false;
    while (process) {
      if (display) {
        display = false;

        Hide_Mouse();
        // Redraw the buttons.
        buttonlist->Draw_All(view);
        Show_Mouse();
      }

      // Invoke game callback.
      ServiceRealTime();

      // Fetch and process input.
      const KeyNumber input = buttonlist->Input(view);  // user input
      switch (static_cast<uint32_t>(input)) {
        case kBriefingButtonFlag | uint32_t{kButton1}:
          selection = base::At(realval, 0);
          pressed = true;
          break;

        case KN_ESC:
          if (numbuttons > 2) {
            selection = base::At(realval, 1);
            pressed = true;
          } else {
            selection = base::At(realval, 2);
            pressed = true;
          }
          break;

        case kBriefingButtonFlag | uint32_t{kButton2}:
          selection = kButton2;
          pressed = true;
          break;

        case kBriefingButtonFlag | uint32_t{kButton3}:
          selection = base::At(realval, 1);
          pressed = true;
          break;

        case KN_LEFT:
          if (numbuttons > 1) {
            base::At(buttons, curbutton)->Turn_Off();
            base::At(buttons, curbutton)->Flag_To_Redraw();

            curbutton--;
            if (curbutton < 0) {
              curbutton = numbuttons - 1;
            }

            base::At(buttons, curbutton)->Turn_On();
            base::At(buttons, curbutton)->Flag_To_Redraw();
          }
          break;

        case KN_RIGHT:
          if (numbuttons > 1) {
            base::At(buttons, curbutton)->Turn_Off();
            base::At(buttons, curbutton)->Flag_To_Redraw();

            curbutton++;
            if (curbutton > numbuttons - 1) {
              curbutton = 0;
            }

            base::At(buttons, curbutton)->Turn_On();
            base::At(buttons, curbutton)->Flag_To_Redraw();
          }
          break;

        case KN_RETURN:
          selection = curbutton + kButton1;
          pressed = true;
          break;

        // Check 'input' to see if it's the 1st char of button text
        default:
          if (b1char == toupper(KeyboardClass::To_ASCII(
                            static_cast<KeyNumber>(input & 0xFF)))) {
            selection = kButton1;
            pressed = true;
          } else if (b2txt != nullptr &&
                     b2char == toupper(KeyboardClass::To_ASCII(
                                   static_cast<KeyNumber>(input & 0xFF)))) {
            selection = kButton2;
            pressed = true;
          } else if (b3txt != nullptr &&
                     b3char == toupper(KeyboardClass::To_ASCII(
                                   static_cast<KeyNumber>(input & 0xFF)))) {
            selection = kButton3;
            pressed = true;
          }
          break;
      }

      if (pressed) {
        switch (selection) {
          case kButton1:
            retval = 1;
            process = false;
            break;

          case kButton2:
            retval = 0;
            process = false;
            break;

          case kButton3:
            retval = 2;
            process = false;
            break;
          default:
            break;
        }

        pressed = false;
      }
    }
  } else {
    TheKeyboard().Clear();
  }

  // Handle MORE button - recurse to show next page (no fade on subsequent
  // pages).
  if (retval == morebutton - 1 && msg.size() > page_text.size()) {
    return ShowBriefingMessageBox(msg.substr(page_text.size() + 1), left_btn,
                                  right_btn, /*fade_to_black=*/false);
  }

  /*
  ** Restore the screen.
  */
  Hide_Mouse();
  // Now set the palette, depending on if we're going to show the video or
  // go back to the main menu.
  switch (retval) {
    case 0:
    case 1:
      ThePalettes().black_palette().Set(kFadePaletteMedium, ServiceRealTime);
      TheScreen().visible_view().Clear();
      break;
    default:
      break;
  }
  Show_Mouse();

  GadgetClass::Set_Color_Scheme(
      &ThePalettes().color_remaps().at(PCOLOR_DIALOG_BLUE));

  // Convert internal button index to the text ID that was clicked.
  return retval == 1 ? left_btn_text_id : right_btn_text_id;
}

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
void ScenarioClass::Set_Scenario_Name(int scenario, ScenarioPlayerType player,
                                      ScenarioDirType dir,
                                      ScenarioVarType var) {
  Scenario = scenario;
  //	ScenPlayer = player;
  //	ScenDir = dir;
  //	ScenVar = var;

  char c_player = 0;  // character representing player type
  char c_dir = 0;     // character representing direction type
  char c_var = 0;     // character representing variation type
  char fname[engine::platform::kMaxFname + engine::platform::kMaxExt];

  /*
  ** Set the player-type value.
  */
  switch (player) {
    case SCEN_PLAYER_SPAIN:
      c_player = HouseTypeClass::As_Reference(HOUSE_SPAIN).Prefix;
      break;

    case SCEN_PLAYER_GREECE:
      c_player = HouseTypeClass::As_Reference(HOUSE_GREECE).Prefix;
      break;

    case SCEN_PLAYER_USSR:
      c_player = HouseTypeClass::As_Reference(HOUSE_USSR).Prefix;
      break;

    case SCEN_PLAYER_JP:
      c_player = HouseTypeClass::As_Reference(HOUSE_JP).Prefix;
      break;

    /*
    **	Multi player scenario.
    */
    case ScenarioPlayerType::SCEN_PLAYER_NONE:
    case ScenarioPlayerType::SCEN_PLAYER_2PLAYER:
    case ScenarioPlayerType::SCEN_PLAYER_MPLAYER:
    default:
      c_player = HouseTypeClass::As_Reference(HOUSE_MULTI1).Prefix;
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

    default:
    case SCEN_DIR_NONE:
      c_dir = Percent_Chance(50) ? 'W' : 'E';
      break;
  }

  /*
  ** Set the variation value.
  */
  if (var == SCEN_VAR_NONE) {
    /*
    ** Find which variations are available for this scenario
    */
    int available = 0;  // Variations A.. that exist, in order.
    for (const ScenarioVarType candidate :
         magic_enum::enum_values<ScenarioVarType>()) {
      if (candidate == SCEN_VAR_LOSE) {
        break;
      }
      absl::SNPrintF(fname, sizeof(fname), "SC%c%02d%c%c.INI", c_player,
                     scenario, c_dir, 'A' + static_cast<int>(candidate));
      if (!GameFileExists(fname)) {
        break;
      }
      available++;
    }

    if (available == 0) {
      c_var = 'X';  // indicates an error
    } else {
      c_var = static_cast<char>('A' + Random_Pick(0, available - 1));
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
      case ScenarioVarType::SCEN_VAR_LOSE:
      default:
        c_var = 'L';
        break;
    }
  }

  /*
  ** generate the filename
  */
  if (scenario < 100) {
    absl::SNPrintF(ScenarioName, sizeof(ScenarioName), "SC%c%02d%c%c.INI",
                   c_player, scenario, c_dir, c_var);
  } else {
    const char first = static_cast<char>((scenario / 36) + 'A');
    char second = static_cast<char>(scenario % 36);

    if (second < 10) {
      second += '0';
    } else {
      second = static_cast<char>(second - 10 + 'A');
    }

    absl::SNPrintF(ScenarioName, sizeof(ScenarioName), "SC%c%c%c%c%c.INI",
                   c_player, first, second, c_dir, c_var);
  }
}

// The variants are lettered from 'A' in enum order.
static char VariantLetter(const ScenarioVarType variant) {
  return static_cast<char>('A' + static_cast<int>(variant));
}

void ScenarioClass::AdvanceToNextScenario(const ScenarioVarType variant) {
  std::string name = MissionWithNumber(ScenarioName, Scenario + 1);
  if (variant != SCEN_VAR_NONE) {
    name = MissionWithVariant(name, VariantLetter(variant));
  }
  // This also reads the new number back into Scenario.
  Set_Scenario_Name(name.c_str());
}

void ScenarioClass::SetScenarioVariant(const ScenarioVarType variant) {
  base::SafeCopy(ScenarioName,
                 MissionWithVariant(ScenarioName, VariantLetter(variant)));
}

void ScenarioClass::Set_Scenario_Name(const char* name) {
  if (name != nullptr) {
    base::SafeCopy(ScenarioName, name);
    base::At(ScenarioName, std::ssize(ScenarioName) - 1) = '\0';

    char buf[3];
    base::CopyBytes(base::ObjectBytes(buf),
                    std::as_bytes(base::Suffix(ScenarioName, 3)), 2);
    base::At(buf, 2) = '\0';
    if (base::At(buf, 0) > '9' || base::At(buf, 1) > '9') {
      char first = base::At(buf, 0);
      char second = base::At(buf, 1);
      if (first <= '9') {
        first -= '0';
      } else {
        first -= 'A';
      }

      if (second <= '9') {
        second -= '0';
      } else {
        second = static_cast<char>(second - 'A' + 10);
      }

      Scenario = (36 * first) + second;
    } else {
      Scenario = base::ParseIntegerOr<int>(buf, 0);
    }
  }
}

/***********************************************************************************************
 * Read_Scenario_INI -- Read specified scenario INI file. *
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
 * HISTORY: * 10/07/1992 JLB : Created.  V.Grippi added CS check 2/5/97 *
 *=============================================================================================*/
bool Read_Scenario_INI(const char* fname, bool /*unused*/) {
  //	// full INI filename
  //	char fname[engine::platform::kMaxFname+engine::platform::kMaxExt];

  TheWorld().scenario_init()++;

  Clear_Scenario();

  /*
  ** Only force a CD check if this is a single player game or if its
  ** a multiplayer game on an official scenario. If its non-official
  ** (a user scenario) then we dont care which CD is in because the
  ** scenario is stored locally on the hard drive. In this case, we
  ** have already verified its existance. ST 3/1/97 4:52PM.
  */
  // Avoid CD check if official scenario was downloaded.
  if ((TheSession().Type == GAME_NORMAL || TheSession().ScenarioIsOfficial) &&
      !absl::EqualsIgnoreCase(TheScenario().ScenarioName, "download.tmp")) {
    /*
    ** If this is scenario 1 then it should be on all CDs unless its an ant
    *scenario
    */
    if (TheScenario().Scenario == 1 &&
        base::At(TheScenario().ScenarioName, 2) != 'A') {
      TheGameState().required_cd() = -1;
    } else {
      /*
      ** If this is a multiplayer scenario we need to find out if its a
      *counterstrike
      ** scenario. If so then we need CD 2. The original multiplayer scenarios
      *are on
      ** all CDs.
      */
      if (TheSession().Type != GAME_NORMAL) {
        TheGameState().required_cd() = -1;  // default that any CD will do.
        // If it's a counterstrike mission, require the counterstrike CD, unless
        // the Aftermath CD is already in the drive, in which case, leave it
        // there. Note, this works because this section only tests for
        // multiplayer scenarios.
        if (IsMissionCounterstrike(TheScenario().ScenarioName)) {
          TheGameState().required_cd() = 2;
          if (Is_Aftermath_Installed() ||
              Get_CD_Index(SearchPaths::current_cd_drive(), 1 * 60) == 3) {
            TheGameState().required_cd() = 3;
          }
        }
        if (IsMissionAftermath(TheScenario().ScenarioName)) {
          TheGameState().required_cd() = 3;
        }
      } else {
        /*
        ** This is a solo game. If the scenario number is >= 20 or its an ant
        *mission
        ** then we need the counterstrike CD (2)
        */
        if (TheScenario().Scenario >= 20 ||
            base::At(TheScenario().ScenarioName, 2) == 'A') {
          TheGameState().required_cd() = 2;
          if (TheScenario().Scenario >= 36 &&
              base::At(TheScenario().ScenarioName, 2) != 'A') {
            TheGameState().required_cd() = 3;
#ifdef BOGUSCD
            TheGameState().required_cd() = -1;
#endif
          }
        } else {
          /*
          ** This is a solo mission from the original Red Alert. Choose the
          *Soviet or
          ** allied CD depending on the scenario name.
          */
          if (base::At(TheScenario().ScenarioName, 2) == 'U') {
            TheGameState().required_cd() = 1;
          } else {
            if (base::At(TheScenario().ScenarioName, 2) == 'G') {
              TheGameState().required_cd() = 0;
            }
          }
        }
      }
    }
    // If we're asking for a CD swap, check to see if we need to set the palette
    // to avoid a black screen.  If this is a normal RA game, and the CD being
    // requested is an RA CD, then don't set the palette, leave the map screen
    // up.

    const int cd_index = Get_CD_Index(SearchPaths::current_cd_drive(), 1 * 60);
    if ((!Using_DVD() || cd_index != 5) &&
        cd_index != TheGameState().required_cd()) {
      if ((TheGameState().required_cd() == 0 ||
           TheGameState().required_cd() == 1) &&
          TheSession().Type == GAME_NORMAL) {
        TheScreen().visible_view().Clear();
      }
      ThePalettes().game_palette().Set(kFadePaletteFast, ServiceRealTime);
    }
    if (!Force_CD_Available(TheGameState().required_cd())) {
      // ShutDownEngine();
      EmergencyExit(EXIT_FAILURE);
    }
  } else {
    /*
    ** This is a user scenario so any old CD will do.
    */
    TheGameState().required_cd() = -1;
  }

  /*
  **	Create scenario filename and read the file.
  */
  //	absl::SNPrintF(fname, sizeof(fname), "%s.INI", root);
  CCINIClass ini;
  //	file.Cache();

  const auto file = OpenGameFile(fname);
  if (!file || !ini.Load(*file, true)) {
    return false;
  }

  // CCINIClass::Load reports a bad digest as a plain failure, so the old
  // "digest wrong" (result 2) exception for multiplayer maps 1-24 is gone.

  /*
  **	Reset the rules values to their initial settings.
  */
  TheWorld().name_override() = {};
  TheWorld().name_override_id() = {};
  if (TheSession().Type == GAME_NORMAL) {
    TheSpecial().IsShadowGrow = false;
  }

  TheSession().Messages.Reset();
  //	Session.Messages.Add_Message(nullptr, 0, nullptr, PCOLOR_GREEN,
  // TPF_6PT_GRAD|TPF_USE_GRAD_PAL|TPF_FULLSHADOW, 1);
  //	Session.Messages.Add_Message(nullptr, 0, nullptr, PCOLOR_GREEN,
  // TPF_6PT_GRAD|TPF_USE_GRAD_PAL|TPF_FULLSHADOW, 1);
  //	Session.Messages.Add_Message(nullptr, 0, nullptr, PCOLOR_GREEN,
  // TPF_6PT_GRAD|TPF_USE_GRAD_PAL|TPF_FULLSHADOW, 1);
  //	Session.Messages.Add_Message(nullptr, 0, nullptr, PCOLOR_GREEN,
  // TPF_6PT_GRAD|TPF_USE_GRAD_PAL|TPF_FULLSHADOW, 1);
  //	Session.Messages.Add_Message(nullptr, 0, nullptr, PCOLOR_GREEN,
  // TPF_6PT_GRAD|TPF_USE_GRAD_PAL|TPF_FULLSHADOW, 1);
  //	Session.Messages.Add_Message(nullptr, 0, nullptr, PCOLOR_GREEN,
  // TPF_6PT_GRAD|TPF_USE_GRAD_PAL|TPF_FULLSHADOW, 1);
  WeaponTypeClass::As_Pointer(WEAPON_FLAMER)->Sound = VOC_NONE;
  InfantryTypeClass::As_Reference(INFANTRY_THIEF).IsDoubleOwned = false;
  InfantryTypeClass::As_Reference(INFANTRY_E4).IsDoubleOwned = false;
  InfantryTypeClass::As_Reference(INFANTRY_SPY).PrimaryWeapon = nullptr;
  InfantryTypeClass::As_Reference(INFANTRY_SPY).SecondaryWeapon = nullptr;
  InfantryTypeClass::As_Reference(INFANTRY_GENERAL).IsBomber = false;
  UnitTypeClass::As_Reference(UNIT_HARVESTER).IsExploding = false;
  UnitTypeClass::As_Reference(UNIT_ANT1).Level = -1;
  UnitTypeClass::As_Reference(UNIT_ANT2).Level = -1;
  UnitTypeClass::As_Reference(UNIT_ANT3).Level = -1;
  BuildingTypeClass::As_Reference(STRUCT_QUEEN).Level = -1;
  BuildingTypeClass::As_Reference(STRUCT_LARVA1).Level = -1;
  BuildingTypeClass::As_Reference(STRUCT_LARVA2).Level = -1;

  TheRules().General(TheRules().rule_ini());
  TheRules().Recharge(TheRules().rule_ini());
  TheRules().AI(TheRules().rule_ini());
  TheRules().Powerups(TheRules().rule_ini());
  TheRules().Land_Types(TheRules().rule_ini());
  RulesClass::Themes(TheRules().rule_ini());
  TheRules().IQ(TheRules().rule_ini());
  TheRules().Objects(TheRules().rule_ini());
  TheRules().Difficulty(TheRules().rule_ini());
  TheRules().General(TheRules().aftermath_ini());
  TheRules().Recharge(TheRules().aftermath_ini());
  TheRules().AI(TheRules().aftermath_ini());
  TheRules().Powerups(TheRules().aftermath_ini());
  TheRules().Land_Types(TheRules().aftermath_ini());
  RulesClass::Themes(TheRules().aftermath_ini());
  TheRules().IQ(TheRules().aftermath_ini());
  TheRules().Objects(TheRules().aftermath_ini());
  TheRules().Difficulty(TheRules().aftermath_ini());
  /*
  **	Override any rules values specified in this
  **	particular scenario file.
  */
  TheRules().General(ini);
  TheRules().Recharge(ini);
  TheRules().AI(ini);
  TheRules().Powerups(ini);
  TheRules().Land_Types(ini);
  RulesClass::Themes(ini);
  TheRules().IQ(ini);
  TheRules().Objects(ini);
  TheRules().Difficulty(ini);
  /*
  ** Init the Scenario CRC value
  */
  TheWorld().scenario_crc() = 0;

  /*
  **	Fetch the appropriate movie names from the INI file.
  */
  const char* const BASIC = "Basic";
  ini.Get_String(BASIC, "Name", "<none>", TheScenario().Description,
                 sizeof(TheScenario().Description));
  TheScenario().IntroMovie =
      ini.Get_VQType(BASIC, "Intro", TheScenario().IntroMovie);
  TheScenario().BriefMovie =
      ini.Get_VQType(BASIC, "Brief", TheScenario().BriefMovie);
  TheScenario().WinMovie = ini.Get_VQType(BASIC, "Win", TheScenario().WinMovie);
  TheScenario().LoseMovie =
      ini.Get_VQType(BASIC, "Lose", TheScenario().LoseMovie);
  TheScenario().ActionMovie =
      ini.Get_VQType(BASIC, "Action", TheScenario().ActionMovie);
  TheScenario().IsToCarryOver =
      ini.Get_Bool(BASIC, "ToCarryOver", TheScenario().IsToCarryOver);
  TheScenario().IsToInherit =
      ini.Get_Bool(BASIC, "ToInherit", TheScenario().IsToInherit);
  TheScenario().IsInheritTimer =
      ini.Get_Bool(BASIC, "TimerInherit", TheScenario().IsInheritTimer);
  TheScenario().IsEndOfGame =
      ini.Get_Bool(BASIC, "EndOfGame", TheScenario().IsEndOfGame);
  TheScenario().IsTanyaEvac =
      ini.Get_Bool(BASIC, "CivEvac", TheScenario().IsTanyaEvac);
  TheScenario().TransitTheme = ini.Get_ThemeType(BASIC, "Theme", THEME_NONE);
  TheWorld().new_ini_format() = ini.Get_Int(BASIC, "NewINIFormat", 0);
  TheScenario().CarryOverPercent =
      ini.Get_Fixed(BASIC, "CarryOverMoney", TheScenario().CarryOverPercent);
  TheScenario().CarryOverPercent.Saturate(1);
  TheScenario().CarryOverCap =
      ini.Get_Int(BASIC, "CarryOverCap", TheScenario().CarryOverCap);
  TheScenario().IsNoSpyPlane =
      ini.Get_Bool(BASIC, "NoSpyPlane", TheScenario().IsNoSpyPlane);
  TheScenario().IsSkipScore =
      ini.Get_Bool(BASIC, "SkipScore", TheScenario().IsSkipScore);
  TheScenario().IsOneTimeOnly =
      ini.Get_Bool(BASIC, "OneTimeOnly", TheScenario().IsOneTimeOnly);
  TheScenario().IsNoMapSel =
      ini.Get_Bool(BASIC, "SkipMapSelect", TheScenario().IsNoMapSel);
  TheScenario().IsTruckCrate =
      ini.Get_Bool(BASIC, "TruckCrate", TheScenario().IsTruckCrate);
  TheScenario().IsMoneyTiberium =
      ini.Get_Bool(BASIC, "FillSilos", TheScenario().IsMoneyTiberium);
  TheScenario().Percent = ini.Get_Int(BASIC, "Percent", TheScenario().Percent);

  /*
  **	Read in the specific information for each of the house types.  This
  *creates *	the houses of different types.
  */
  HouseClass::Read_INI(ini);
  ServiceRealTime();

  /*
  **	Read in the team-type data. The team types must be created before any
  **	triggers can be created.
  */
  TeamTypeClass::Read_INI(ini);
  ServiceRealTime();

  /*
  **	Assign PlayerPtr by reading the player's house from the INI;
  **	Must be done before any TechnoClass objects are created.
  */
  if (TheSession().Type == GAME_NORMAL) {
    ThePlayer() = HouseClass::As_Pointer(
        ini.Get_HousesType(BASIC, "Player", HOUSE_GREECE));
    ThePlayer()->Assign_Handicap(TheScenario().Difficulty);
    int carryover = 0;
    if (TheScenario().CarryOverCap != -1) {
      carryover = std::min(
          TheScenario().CarryOverMoney * TheScenario().CarryOverPercent,
          TheScenario().CarryOverCap);
    } else {
      carryover = TheScenario().CarryOverMoney * TheScenario().CarryOverPercent;
    }
    ThePlayer()->Credits += carryover;
    ThePlayer()->Control.InitialCredits += carryover;
  } else {
    Assign_Houses();
  }
  ThePlayer()->IsHuman = true;
  ThePlayer()->IsPlayerControl = true;

  /*
  **	Read in the trigger data. The triggers must be created before any other
  **	objects can be initialized.
  */
  TriggerTypeClass::Read_INI(ini);
  ServiceRealTime();

  /*
  **	Read in the map control values. This includes dimensions
  **	as well as theater information.
  */
  TheMap().Read_INI(ini);
  ServiceRealTime();

  //	if (NewINIFormat < 2 || !ini.Is_Present("MapPack")) {
  //		Map.Read_Binary(root, &ScenarioCRC);
  //	}

  /*
  **	Read in and place the 3D terrain objects.
  */
  TerrainClass::Read_INI(ini);
  ServiceRealTime();
  /*
  **	Read in and place the units (all sides).
  */
  UnitClass::Read_INI(ini);
  ServiceRealTime();

  VesselClass::Read_INI(ini);
  ServiceRealTime();

  /*
  **	Read in and place the infantry units (all sides).
  */
  InfantryClass::Read_INI(ini);
  ServiceRealTime();

  /*
  **	Read in and place all the buildings on the map.
  */
  BuildingClass::Read_INI(ini);
  ServiceRealTime();

  /*
  **	Read in the AI's base information.
  */
  TheWorld().base().Read_INI(ini);
  ServiceRealTime();

  /*
  **	Read in any normal overlay objects.
  */
  OverlayClass::Read_INI(ini);
  ServiceRealTime();

  /*
  **	Read in any smudge overlays.
  */
  SmudgeClass::Read_INI(ini);
  ServiceRealTime();

  /*	Moved above ini.Get_TextBlock(...) so Xlat mission.ini could be loaded
  **	If the briefing text could not be found in the INI file, then search
  **	the mission.ini file.  VG 10/17/96
  */
  INIClass mini;
  if (const auto fc = OpenGameFile("MISSION.INI")) {
    mini.Load(*fc);
  }
  mini.Get_TextBlock(fname, TheScenario().BriefingText,
                     sizeof(TheScenario().BriefingText));

  /*
  **	Read in any briefing text.
  */
  if (base::At(TheScenario().BriefingText, 0) == '\0') {
    ini.Get_TextBlock("Briefing", TheScenario().BriefingText,
                      sizeof(TheScenario().BriefingText));
  }
  /*
  **	Perform a final overpass of the map. This handles smoothing of certain
  **	types of terrain (tiberium).
  */
  TheMap().Overpass();
  ServiceRealTime();

  /*
  **	Multi-player last-minute fixups:
  **	- If computer players are disabled, remove all computer-owned houses
  **	- If bases are disabled, create the scenario dynamically
  **	- Remove any flag spot overlays lying around
  **	- If capture-the-flag is enabled, assign flags to cells.
  */
  if (TheSession().Type != GAME_NORMAL /*|| Scen.ScenPlayer == SCEN_PLAYER_2PLAYER || Scen.ScenPlayer == SCEN_PLAYER_MPLAYER*/) {
    /*
    **	If Ghosts are disabled and we're not editing, remove computer players
    **	(Must be done after all objects are read in from the INI)
    */
    if (TheSession().Options.AIPlayers + TheSession().Players.Count() <
            TheRules().MaxPlayers &&
        !TheDebugState().map_editor_active()) {
      Remove_AI_Players();
    }

    /*
    **	Units must be created for each house.  If bases are ON, this routine
    **	will create an MCV along with the units; otherwise, it will just create
    **	a whole bunch of units.  Session.Options.UnitCount is the total #
    * of units *	to create.
    */
    if (!TheDebugState().map_editor_active()) {
      const int save_init =
          TheWorld().scenario_init();  // turn ScenarioInit off
      TheWorld().scenario_init() = 0;
      Create_Units(ini.Get_Bool("Basic", "Official", false));
      TheWorld().scenario_init() = save_init;  // turn ScenarioInit back on
    }

    /*
    **	Place crates if random crates are enabled for
    **	this scenario.
    */
    if (TheSession().Options.Goodies) {
      int count = std::max(TheRules().CrateMinimum, TheSession().NumPlayers);
      count = std::min(count, TheRules().CrateMaximum);
      for (int index = 0; index < count; index++) {
        TheMap().Place_Random_Crate();
      }
    }

    /*
    **	Compute my starting location as the average Coord of all my stuff.
    */
    TheMap().Compute_Start_Pos();
  }

  ServiceRealTime();

  /*
  **	Return with flag saying that the scenario file was read.
  */
  if (Is_Aftermath_Installed() && (TheSession().Type == GAME_SKIRMISH)) {
    TheSession().IsAftermath = TheRules().NewUnitsEnabled = true;
  }

  TheWorld().scenario_init()--;
  return true;
}

/***********************************************************************************************
 * Write_Scenario_INI -- Write the scenario INI file. *
 *                                                                                             *
 * INPUT: * root      root filename for the scenario *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 10/07/1992 JLB : Created. * 05/11/1995 JLB : Updates movie data. *
 *=============================================================================================*/
void Write_Scenario_INI(const char* fname) {
  if constexpr (config::kCheatKeysEnabled) {
    CCINIClass ini;

    /*
    **	Preload the old scenario if it is present because there may
    **	be some fields in the INI that are processed but not written
    **	out. Preloading the scenario will preserve these manually
    **	maintained entries.
    */
    if (const auto file = OpenGameFile(fname)) {
      ini.Load(*file, true);
    }

    static const char* const BASIC = "Basic";
    ini.Clear(BASIC);
    ini.Put_String(BASIC, "Name", TheScenario().Description);
    ini.Put_VQType(BASIC, "Intro", TheScenario().IntroMovie);
    ini.Put_VQType(BASIC, "Brief", TheScenario().BriefMovie);
    ini.Put_VQType(BASIC, "Win", TheScenario().WinMovie);
    ini.Put_VQType(BASIC, "Lose", TheScenario().LoseMovie);
    ini.Put_VQType(BASIC, "Action", TheScenario().ActionMovie);
    ini.Put_HousesType(BASIC, "Player", ThePlayer()->Class->House);
    ini.Put_ThemeType(BASIC, "Theme", TheScenario().TransitTheme);
    ini.Put_Fixed(BASIC, "CarryOverMoney", TheScenario().CarryOverPercent);
    ini.Put_Bool(BASIC, "ToCarryOver", TheScenario().IsToCarryOver);
    ini.Put_Bool(BASIC, "ToInherit", TheScenario().IsToInherit);
    ini.Put_Bool(BASIC, "TimerInherit", TheScenario().IsInheritTimer);
    ini.Put_Bool(BASIC, "CivEvac", TheScenario().IsTanyaEvac);
    ini.Put_Int(BASIC, "NewINIFormat", 3);
    ini.Put_Int(BASIC, "CarryOverCap", TheScenario().CarryOverCap / 100);
    ini.Put_Bool(BASIC, "EndOfGame", TheScenario().IsEndOfGame);
    ini.Put_Bool(BASIC, "NoSpyPlane", TheScenario().IsNoSpyPlane);
    ini.Put_Bool(BASIC, "SkipScore", TheScenario().IsSkipScore);
    ini.Put_Bool(BASIC, "OneTimeOnly", TheScenario().IsOneTimeOnly);
    ini.Put_Bool(BASIC, "SkipMapSelect", TheScenario().IsNoMapSel);
    ini.Put_Bool(BASIC, "Official", true);
    ini.Put_Bool(BASIC, "FillSilos", TheScenario().IsMoneyTiberium);
    ini.Put_Bool(BASIC, "TruckCrate", TheScenario().IsTruckCrate);
    ini.Put_Int(BASIC, "Percent", TheScenario().Percent);

    HouseClass::Write_INI(ini);
    TeamTypeClass::Write_INI(ini);
    TriggerTypeClass::Write_INI(ini);
    TheMap().Write_INI(ini);
    TerrainClass::Write_INI(ini);
    UnitClass::Write_INI(ini);
    VesselClass::Write_INI(ini);
    InfantryClass::Write_INI(ini);
    BuildingClass::Write_INI(ini);
    TheWorld().base().Write_INI(ini);
    OverlayClass::Write_INI(ini);
    SmudgeClass::Write_INI(ini);

    if (!std::string_view(TheScenario().BriefingText).empty()) {
      ini.Put_TextBlock("Briefing", TheScenario().BriefingText);
    }
    //	absl::SNPrintF(fname, sizeof(fname), "%s.INI", root);
    if (const auto rawfile = OpenDiskFile(fname, FileAccess::kWrite)) {
      ini.Save(*rawfile, true);
    }
  }
}

/***********************************************************************************************
 * Assign_Houses -- Assigns multiplayer houses to various players *
 *                                                                                             *
 * This routine assigns all players to a multiplayer house slot; it forms
 *network connections  * to each player.  The Connection ID used is the value
 *for that player's HousesType.			  *
 *                                                                                             *
 * PlayerPtr is also set here.
 **
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * This routine assumes the 'Players' vector has been properly
 *filled in with players'		  * names, addresses, color, etc.
 ** Also, it's assumed that the HouseClass's have all been created &
 *initialized.				  *
 *                                                                                             *
 * HISTORY: * 06/09/1995 BRR : Created. * 07/14/1995 JLB : Records name of
 *player in house structure.                               *
 *=============================================================================================*/
void Assign_Houses() {
  int assigned[kMaxPlayers];
  base::EnumArray<PlayerColorType, bool, 8> color_used{};
  HousesType house = HOUSE_NONE;
  HouseClass* housep = nullptr;
  int color = 0;

  //------------------------------------------------------------------------
  // Initialize
  //------------------------------------------------------------------------
  for (int i = 0; i < kMaxPlayers; i++) {
    base::At(assigned, i) = 0;
    color_used.at(static_cast<PlayerColorType>(i)) = false;
  }

  //	debugprint( "Assign_Houses()\n" );
  //------------------------------------------------------------------------
  // Assign each player in 'Players' to a multiplayer house.  Players will
  // be sorted by their chosen color value (this value must be unique among
  // all the players).
  //------------------------------------------------------------------------
  for (int i = 0; i < TheSession().Players.Count(); i++) {
    //.....................................................................
    // Find the player with the lowest color index
    //.....................................................................
    int index = 0;
    PlayerColorType lowest_color = PCOLOR_NONE;
    for (int j = 0; j < TheSession().Players.Count(); j++) {
      //..................................................................
      // If we've already assigned this house, skip it.
      //..................................................................
      if (base::At(assigned, j)) {
        continue;
      }
      if (lowest_color == PCOLOR_NONE ||
          TheSession().Players.at(j)->Player.Color < lowest_color) {
        lowest_color = TheSession().Players.at(j)->Player.Color;
        index = j;
      }
    }

    //.....................................................................
    // Mark this player as having been assigned.
    //.....................................................................
    base::At(assigned, index) = 1;
    color_used.at(TheSession().Players.at(index)->Player.Color) = true;

    //.....................................................................
    // Assign the lowest-color'd player to the next available slot in the
    // HouseClass array.
    //.....................................................................
    house = static_cast<HousesType>(i + static_cast<int>(HOUSE_MULTI1));
    housep = HouseClass::As_Pointer(house);
    base::SafeCopy(housep->IniName, TheSession().Players.at(index)->Name);
    // A second copy that stays put for the whole game -- see InitialName.
    base::SafeCopy(housep->InitialName, TheSession().Players.at(index)->Name);
    housep->IsHuman = true;
    housep->Init_Data(TheSession().Players.at(index)->Player.Color,
                      TheSession().Players.at(index)->Player.House,
                      TheSession().Options.Credits);
    if (index == 0) {
      ThePlayer() = housep;
    }
    /*
    **	Convert the build level into an actual tech level to assign to the
    *house. *	There isn't a one-to-one correspondence.
    */
    housep->Control.TechLevel = base::At(build_tech, TheWorld().build_level());

    housep->Assign_Handicap(TheScenario().Difficulty);

    //.....................................................................
    // Record where we placed this player
    //.....................................................................
    TheSession().Players.at(index)->Player.ID = house;

    //		debugprint( "Assigned ID of %i to %s\n", house,
    // Session.Players[index]->Name );
  }

  //------------------------------------------------------------------------
  // Now assign computer players to the remaining houses.
  //------------------------------------------------------------------------
  for (int i = static_cast<int>(TheSession().Players.Count());
       i < TheSession().Players.Count() + TheSession().Options.AIPlayers; i++) {
    house = static_cast<HousesType>(i + static_cast<int>(HOUSE_MULTI1));
    housep = HouseClass::As_Pointer(house);
    const HousesType pref_house =
        Percent_Chance(50) ? HOUSE_GREECE : HOUSE_USSR;

    //.....................................................................
    // Pick a color for this house; keep looping until we find one.
    //.....................................................................
    while (true) {
      color = Random_Pick(0, 7);
      if (!color_used.at(static_cast<PlayerColorType>(color))) {
        break;
      }
    }
    color_used.at(static_cast<PlayerColorType>(color)) = true;

    //.....................................................................
    // Set up the house
    //.....................................................................
    //		housep->Control.MaxUnit = 80;
    //		housep->Control.MaxInfantry = 60;
    //		housep->Control.MaxBuilding = 60;
    //		housep->Control.MaxVessel = 60;
    housep->IsHuman = false;
    housep->IsStarted = true;

    base::SafeCopy(housep->IniName, Text_String(TXT_COMPUTER));

    if (TheSession().Type != GAME_NORMAL) {
      housep->IQ = TheRules().MaxIQ;
    }

    housep->Init_Data(static_cast<PlayerColorType>(color), pref_house,
                      TheSession().Options.Credits);
    housep->Control.TechLevel = base::At(build_tech, TheWorld().build_level());
    //		housep->Control.TechLevel = BuildLevel;

    DiffType difficulty = TheScenario().CDifficulty;

    if (TheSession().Players.Count() > 1 && TheRules().IsCompEasyBonus &&
        difficulty > DIFF_EASY) {
      difficulty = static_cast<DiffType>(static_cast<int>(difficulty) - 1);
    }
    housep->Assign_Handicap(difficulty);
  }

  for (int i = static_cast<int>(TheSession().Players.Count()) +
               TheSession().Options.AIPlayers;
       i < TheRules().MaxPlayers; i++) {
    house = static_cast<HousesType>(i + static_cast<int>(HOUSE_MULTI1));
    housep = HouseClass::As_Pointer(house);
    if (housep != nullptr) {
      housep->IsDefeated = true;
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
  int aicount = 0;

  for (int i = 0; i < kMaxPlayers; i++) {
    const auto house =
        static_cast<HousesType>(i + static_cast<int>(HOUSE_MULTI1));
    HouseClass* housep = HouseClass::As_Pointer(house);
    if (!static_cast<bool>(housep->IsHuman)) {
      aicount++;
      if (aicount > TheSession().Options.AIPlayers) {
        housep->Clobber_All();
      }
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
 * INPUT:   official -- Directs the placement logic to use the full set of
 *waypoints rather    * than biasing toward the first four. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 06/09/1995 BRR : Created. *
 *=============================================================================================*/
static void Create_Units(bool official) {
  static const struct {
    int MinLevel;
    UnitType AllyType[2];
    UnitType SovietType[2];
  } utable[] = {{4, {UNIT_MTANK2, UNIT_LTANK}, {UNIT_MTANK, UNIT_NONE}},
                {5, {UNIT_APC, UNIT_NONE}, {UNIT_V2_LAUNCHER, UNIT_NONE}},
                {8, {UNIT_ARTY, UNIT_JEEP}, {UNIT_MTANK, UNIT_NONE}},
                {10, {UNIT_MTANK2, UNIT_MTANK2}, {UNIT_HTANK, UNIT_NONE}}};
  static int num_units[std::size(utable)];  // # of each type of unit to create

  static const struct {
    int MinLevel;
    int AllyCount;
    InfantryType AllyType;
    int SovietCount;
    InfantryType SovietType;
  } itable[] = {
      {0, 1, INFANTRY_E1, 1, INFANTRY_E1},
      {2, 1, INFANTRY_E3, 1, INFANTRY_E2},
      {4, 1, INFANTRY_E3, 1, INFANTRY_E4},

      // removed because of bug B478 (inappropriate infantry given in a bases
      // off scenario).
      //		{5,	1,INFANTRY_RENOVATOR,	1,INFANTRY_RENOVATOR},
      //		{6,	1,INFANTRY_SPY,			1,INFANTRY_DOG},
      //		{10,	1,INFANTRY_THIEF,
      // 1,INFANTRY_DOG}, 		{12,	1,INFANTRY_MEDIC,
      // 2,INFANTRY_DOG}
  };
  static int
      num_infantry[std::size(itable)];  // # of each type of infantry to create

  CELL centroid = 0;  // centroid of this house's stuff
  CELL centerpt = 0;  // centroid for a category of objects, as a CELL

  int u_limit = 0;  // last allowable index of units for this BuildLevel
  int i_limit = 0;  // last allowable index of infantry for this BuildLevel
  TechnoClass* obj = nullptr;  // newly-created object
  int scaleval = 0;            // value to scale # units or infantry

  /*
  **	For the current BuildLevel, find the max allowable index
  * into the tables
  */
  for (int i = 0; i < std::ssize(utable); i++) {
    if (ThePlayer()->Control.TechLevel >= base::At(utable, i).MinLevel) {
      u_limit = i + 1;
    }
  }
  for (int i = 0; i < std::ssize(itable); i++) {
    if (ThePlayer()->Control.TechLevel >= base::At(itable, i).MinLevel) {
      i_limit = i + 1;
    }
  }

  /*
  **	Compute how many of each buildable category to create
  */
  /*
  **	Compute allowed # units
  */
  int tot_units =
      TheSession().Options.UnitCount * 2 / 3;  // total # units to create
  if (u_limit == 0) {
    tot_units = 0;
  }

  /*
  **	Init # of each category to 0
  */
  for (int i = 0; i < u_limit; i++) {
    base::At(num_units, i) = 0;
  }

  /*
  **	Increment # of each category, until we've used up all units
  */
  int j = 0;
  for (int i = 0; i < tot_units; i++) {
    base::At(num_units, j)++;
    j++;
    if (j >= u_limit) {
      j = 0;
    }
  }

  /*
  **	Compute allowed # infantry
  */
  const int tot_infantry =
      TheSession().Options.UnitCount - tot_units;  // total # infantry to create

  /*
  **	Init # of each category to 0
  */
  for (int i = 0; i < i_limit; i++) {
    base::At(num_infantry, i) = 0;
  }

  /*
  **	Increment # of each category, until we've used up all infantry
  */
  j = 0;
  for (int i = 0; i < tot_infantry; i++) {
    base::At(num_infantry, j)++;
    j++;
    if (j >= i_limit) {
      j = 0;
    }
  }

  /*
  **	Build a list of the valid waypoints. This normally shouldn't be
  **	necessary because the scenario level designer should have assigned
  **	valid locations to the first N waypoints, but just in case, this
  **	loop verifies that.
  */
  bool taken[26];
  CELL waypts[26];
  DCHECK(TheRules().MaxPlayers < std::ssize(waypts));
  int num_waypts = 0;

  /*
  **	Calculate the number of waypoints (as a minimum) that will be lifted
  *from the *	mission file. Bias this number so that only the first 4
  *waypoints are used *	if there are 4 or fewer players. Unofficial maps will
  *pick from all the *	available waypoints.
  */
  int look_for = std::max(4, static_cast<int>(TheSession().Players.Count()) +
                                 TheSession().Options.AIPlayers);
  if (!official) {
    look_for = 8;
  }

  for (int waycount = 0; waycount < look_for; waycount++) {
    //	for (int waycount = 0; waycount < max(4,
    // Session.Players.Count()+Session.Options.AIPlayers); waycount++)
    // {
    if (base::At(TheScenario().Waypoint, waycount) != -1) {
      base::At(waypts, num_waypts) = base::At(TheScenario().Waypoint, waycount);
      base::At(taken, num_waypts) = false;
      num_waypts++;
    }
  }

  /*
  **	If there are insufficient waypoints to account for all players, then
  *randomly assign *	starting points until there is enough.
  */
  const int deficiency = look_for - num_waypts;
  //	int deficiency = (Session.Players.Count() +
  // Session.Options.AIPlayers) -
  // num_waypts;
  if (deficiency > 0) {
    for (int index = 0; index < deficiency; index++) {
      CELL trycell = XY_Cell(
          TheMap().MapCellX + Random_Pick(0, TheMap().MapCellWidth - 1),
          TheMap().MapCellY + Random_Pick(0, TheMap().MapCellHeight - 1));

      trycell = TheMap().Nearby_Location(trycell, SPEED_TRACK);
      base::At(waypts, num_waypts) = trycell;
      base::At(taken, num_waypts) = false;
      num_waypts++;
    }
  }

  /*
  **	Loop through all houses.  Computer-controlled houses, with
  *Session.Options.Bases *	ON, are treated as though bases are OFF (since
  *we have no base-building *	AI logic.)
  */
  int numtaken = 0;
  for (int slot = 0; slot < TheSession().MaxPlayers; slot++) {
    const auto house =
        static_cast<HousesType>(static_cast<int>(HOUSE_MULTI1) + slot);
    /*
    **	Get a pointer to this house; if there is none, go to the next house
    */
    HouseClass* hptr = HouseClass::As_Pointer(house);
    if (hptr == nullptr) {
      continue;
    }

    /*
    **	Pick the starting location for this house. The first house just picks
    **	one of the valid locations at random. The other houses pick the furthest
    **	wapoint from the existing houses.
    */
    if (numtaken == 0) {
      const int pick = Random_Pick(0, num_waypts - 1);
      centroid = base::At(waypts, pick);
      base::At(taken, pick) = true;
      numtaken++;
    } else {
      /*
      **	Set all waypoints to have a score of zero in preparation for
      *giving *	a distance score to all waypoints.
      */
      int score[26];
      base::FillBytes(base::ObjectBytes(score), '\0', sizeof(score));

      /*
      **	Scan through all waypoints and give a score as a value of the
      *sum *	of the distances from this waypoint to all taken waypoints.
      */
      for (int index = 0; index < num_waypts; index++) {
        /*
        **	If this waypoint has not already been taken, then accumulate the
        **	sum of the distance between this waypoint and all other taken
        **	waypoints.
        */
        if (!base::At(taken, index)) {
          for (int trypoint = 0; trypoint < num_waypts; trypoint++) {
            if (base::At(taken, trypoint)) {
              base::At(score, index) +=
                  Distance(Cell_Coord(base::At(waypts, index)),
                           Cell_Coord(base::At(waypts, trypoint)));
            }
          }
        }
      }

      /*
      **	Now find the waypoint with the largest score. This waypoint is
      *the one *	that is furthest from all other taken waypoints.
      */
      int best = 0;
      int bestvalue = 0;
      for (int searchindex = 0; searchindex < num_waypts; searchindex++) {
        if (base::At(score, searchindex) > bestvalue || bestvalue == 0) {
          bestvalue = base::At(score, searchindex);
          best = searchindex;
        }
      }

      /*
      **	Assign this best position to the house.
      */
      centroid = base::At(waypts, best);
      base::At(taken, best) = true;
      numtaken++;
    }

    /*
    **	Assign the center of this house to the waypoint location.
    */
    hptr->Center = Cell_Coord(centroid);

    /*
    **	If Bases are ON, human & computer houses are treated differently
    */
    if (TheSession().Options.Bases) {
      /*
      **	- For a human-controlled house:
      **	  - Set 'scaleval' to 1
      **	  - Create an MCV
      **	  - Attach a flag to it for capture-the-flag mode
      */
      scaleval = 1;
      obj = new UnitClass(UNIT_MCV, house);
      if ((!obj->Unlimbo(Cell_Coord(centroid), DIR_N)) &&
          (!Scan_Place_Object(obj, centroid))) {
        delete obj;
        obj = nullptr;
      }

      if (obj != nullptr) {
        hptr->FlagHome = 0;
        hptr->FlagLocation = 0;
        if (TheSpecial().IsCaptureTheFlag) {
          hptr->Flag_Attach(dynamic_cast<UnitClass*>(obj), true);
        }
      }
    } else {
      /*
      **	If bases are OFF, set 'scaleval' to 1 & create a Mobile HQ for
      **	capture-the-flag mode.
      */
      scaleval = 1;
    }

    /*
    **	Create units for this house
    */
    for (int i = 0; i < u_limit; i++) {
      /*
      **	Find the center point for this category.
      */
      centerpt = Clip_Scatter(centroid, 4);

      /*
      **	Place objects; loop through all unit in this category
      */
      for (j = 0; j < base::At(num_units, i) * scaleval; j++) {
        /*
        **	Create an Ally unit
        */
        if (!IsSovietHouse(hptr->ActLike)) {
          for (const auto k : base::At(utable, i).AllyType) {
            if (k != UNIT_NONE) {
              obj = new UnitClass(k, house);
              if (!Scan_Place_Object(obj, centerpt)) {
                delete obj;
              } else {
                if (!hptr->IsHuman) {
                  obj->Set_Mission(MISSION_GUARD_AREA);
                } else {
                  obj->Set_Mission(MISSION_GUARD);
                }
              }
            }
          }
        } else {
          /*
          **	Create a Soviet unit
          */
          for (const auto k : base::At(utable, i).SovietType) {
            if (k != UNIT_NONE) {
              obj = new UnitClass(k, house);
              if (!Scan_Place_Object(obj, centerpt)) {
                delete obj;
              } else {
                if (!hptr->IsHuman) {
                  obj->Set_Mission(MISSION_GUARD_AREA);
                } else {
                  obj->Set_Mission(MISSION_GUARD);
                }
              }
            }
          }
        }
      }
    }

    /*
    **	Create infantry
    */
    for (int i = 0; i < i_limit; i++) {
      /*
      **	Find the center point for this category.
      */
      centerpt = Clip_Scatter(centroid, 4);

      /*
      **	Place objects; loop through all unit in this category
      */
      for (j = 0; j < base::At(num_infantry, i) * scaleval; j++) {
        /*
        **	Create Ally infantry (Note: Unlimbo calls Enter_Idle_Mode(),
        *which *	assigns the infantry to HUNT; we must use Set_Mission()
        *to override *	this state.)
        */
        if (!IsSovietHouse(hptr->ActLike)) {
          for (int k = 0; k < base::At(itable, i).AllyCount; k++) {
            obj = new InfantryClass(base::At(itable, i).AllyType, house);
            if (!Scan_Place_Object(obj, centerpt)) {
              delete obj;
            } else {
              if (!hptr->IsHuman) {
                obj->Set_Mission(MISSION_GUARD_AREA);
              } else {
                obj->Set_Mission(MISSION_GUARD);
              }
            }
          }
        } else {
          /*
          **	Create Soviet infantry
          */
          for (int k = 0; k < base::At(itable, i).SovietCount; k++) {
            obj = new InfantryClass(base::At(itable, i).SovietType, house);
            if (!Scan_Place_Object(obj, centerpt)) {
              delete obj;
            } else {
              if (!hptr->IsHuman) {
                obj->Set_Mission(MISSION_GUARD_AREA);
              } else {
                obj->Set_Mission(MISSION_GUARD);
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

  /*
  **	First try to unlimbo the object in the given cell.
  */
  if (TheMap().In_Radar(cell)) {
    techno = TheMap().at(cell).Cell_Techno();
    if ((!techno || (techno->What_Am_I() == RTTI_INFANTRY &&
                     obj->What_Am_I() == RTTI_INFANTRY)) &&
        obj->Unlimbo(Cell_Coord(cell), DIR_N)) {
      return true;
    }
  }

  /*
  **	Loop through distances from the given center cell; skip the center cell.
  **	For each distance, try placing the object along each rotational
  *direction; *	if none are available, try each direction with a random scatter
  *value. *	If that fails, go to the next distance. *	This ensures
  *that the closest coordinates are filled first.
  */
  for (int dist = 1; dist < 32; dist++) {
    /*
    **	Pick a random starting direction
    */
    FacingType rot = Random_Pick(FACING_N, FACING_NW);  // for object placement

    /*
    **	Try all directions twice
    */
    for (int tryval = 0; tryval < 2; tryval++) {
      /*
      **	Loop through all directions, at this distance.
      */
      for (FacingType fcounter = FACING_N; fcounter <= FACING_NW; fcounter++) {
        bool skipit = false;

        /*
        **	Pick a coordinate along this directional axis
        */
        CELL newcell = Clip_Move(cell, rot, dist);

        /*
        **	If this is our second try at this distance, add a random scatter
        **	to the desired cell, so our units aren't all aligned along
        *spokes.
        */
        if (tryval > 0) {
          newcell = Clip_Scatter(newcell, 1);
        }

        /*
        **	If, by randomly scattering, we've chosen the exact center, skip
        **	it & try another direction.
        */
        if (newcell == cell) {
          skipit = true;
        }

        if (!skipit) {
          /*
          **	Only attempt to Unlimbo the object if:
          **	- there is no techno in the cell
          **	- the techno in the cell & the object are both infantry
          */
          techno = TheMap().at(newcell).Cell_Techno();
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

  /*
  **	Get X & Y coords of given starting cell
  */
  int x = Cell_X(cell);
  int y = Cell_Y(cell);

  /*
  **	Compute our x & y limits
  */
  const int xmin = TheMap().MapCellX;
  const int xmax = xmin + TheMap().MapCellWidth - 1;
  const int ymin = TheMap().MapCellY;
  const int ymax = ymin + TheMap().MapCellHeight - 1;

  /*
  **	Adjust the x-coordinate
  */
  const int xdist = Random_Pick(0, maxdist);
  if (Percent_Chance(50)) {
    x += xdist;
    x = std::min(x, xmax);
  } else {
    x -= xdist;
    x = std::max(x, xmin);
  }

  /*
  **	Adjust the y-coordinate
  */
  const int ydist = Random_Pick(0, maxdist);
  if (Percent_Chance(50)) {
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

  /*
  **	Get X & Y coords of given starting cell
  */
  int x = Cell_X(cell);
  int y = Cell_Y(cell);

  /*
  **	Compute our x & y limits
  */
  const int xmin = TheMap().MapCellX;
  const int xmax = xmin + TheMap().MapCellWidth - 1;
  const int ymin = TheMap().MapCellY;
  const int ymax = ymin + TheMap().MapCellHeight - 1;

  /*
  **	Adjust the x-coordinate
  */
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
    default:
      break;
  }

  /*
  **	Clip to the map
  */
  x = std::clamp(x, xmin, xmax);
  y = std::clamp(y, ymin, ymax);

  return XY_Cell(x, y);
}

void Disect_Scenario_Name(const char* name_data, int& scenario,
                          ScenarioPlayerType& player, ScenarioDirType& dir,
                          ScenarioVarType& var) {
  if (name_data == nullptr) {
    return;
  }

  const std::string_view name(name_data);
  if (name.size() < 7) {
    return;
  }

  /*
  **	Fetch the scenario number.
  */
  char buf[3];
  base::SafeCopy(buf, name.substr(3, 2));
  base::At(buf, 2) = '\0';
  char first = base::At(buf, 0);
  char second = base::At(buf, 1);
  if (first <= '9' && second <= '9') {
    scenario = base::ParseIntegerOr<int>(buf, 0);
  } else {
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
        second = static_cast<char>(second - 'a' + 10);
      } else {
        second = static_cast<char>(second - 'A' + 10);
      }
    }
    scenario = (36 * first) + second;
  }

  /*
  **	Fetch the scenario player (side).
  */
  player = SCEN_PLAYER_GREECE;
  if (name.at(2) == HouseTypeClass::As_Reference(HOUSE_SPAIN).Prefix) {
    player = SCEN_PLAYER_SPAIN;
  }
  if (name.at(2) == HouseTypeClass::As_Reference(HOUSE_GREECE).Prefix) {
    player = SCEN_PLAYER_GREECE;
  }
  if (name.at(2) == HouseTypeClass::As_Reference(HOUSE_USSR).Prefix) {
    player = SCEN_PLAYER_USSR;
  }

  /*
  **	Fetch the direction.
  */
  dir = SCEN_DIR_EAST;
  if (name.at(5) == 'E') {
    dir = SCEN_DIR_EAST;
  } else {
    dir = SCEN_DIR_WEST;
  }

  /*
  **	Fetch the variation.
  */
  var = SCEN_VAR_A;
  var = static_cast<ScenarioVarType>(name.at(6) - 'A' +
                                     static_cast<int>(SCEN_VAR_A));
}
