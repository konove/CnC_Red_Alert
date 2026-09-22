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

/* $Header:   F:\projects\c&c\vcs\code\scenario.cpv   2.17   16 Oct 1995
 * 16:52:08   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
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
 *                  Last Update : August 24, 1995 [JLB] *
 *                                                                                             *
 * This module handles the scenario reading and writing. Scenario related * code
 *that is executed between scenario play can also be here. *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Clear_Scenario -- Clears all data in preparation for scenario
 *load.                       * Do_Lose -- Display losing comments. * Do_Restart
 *-- Handle the restart mission process. * Do_Win -- Display winning
 *congratulations.                                                * Fill_In_Data
 *-- Recreate all data that is not loaded with scenario.                       *
 *   Read_Scenario -- Reads a scenario from disk. * Restate_Mission -- Handles
 *restating the mission objective.                               *
 *   Start_Scenario -- Starts the scenario. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/scenario.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "port/safe_string.h"
#include "sdllib/pixel_buffer.h"
#include "sdllib/timer.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "td/aircraft.h"
#include "td/anim.h"
#include "td/audio.h"
#include "td/base.h"
#include "td/building.h"
#include "td/bullet.h"
#include "td/conquer.h"
#include "td/defines.h"
#include "td/dialog.h"
#include "td/ending.h"
#include "td/factory.h"
#include "td/ftimer.h"
#include "td/game_state.h"
#include "td/goptions.h"
#include "td/heap.h"
#include "td/house.h"
#include "td/infantry.h"
#include "td/ini.h"
#include "td/inline.h"
#include "td/interpal.h"
#include "td/jshell.h"
#include "td/logic.h"
#include "td/mapedit.h"
#include "td/msgbox.h"
#include "td/object.h"
#include "td/object_heaps.h"
#include "td/overlay.h"
#include "td/palette.h"
#include "td/palettes.h"
#include "td/score.h"
#include "td/screen.h"
#include "td/session.h"
#include "td/smudge.h"
#include "td/special.h"
#include "td/startup.h"
#include "td/team.h"
#include "td/teamtype.h"
#include "td/template.h"
#include "td/terrain.h"
#include "td/theme.h"
#include "td/trigger.h"
#include "td/type.h"
#include "td/unit.h"
#include "td/vector.h"
#include "td/winstub.h"
#include "td/world.h"
#include "tech/game_file.h"

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
bool Start_Scenario(char* root, bool briefing) {
  if (!Read_Scenario(root)) {
    CCDebugString("C&C95 - Failed to read scenario.\n");
    return false;
  }
  CCDebugString("C&C95 - Scenario read OK.\n");

#ifdef DEMO

  if (briefing) {
    Play_Movie(BriefMovie);
    Play_Movie(ActionMovie, TheWorld().transit_theme());
  }
  TheTheme().Queue_Song(THEME_AOI);

#else

  /*
  ** Install some hacks around the movie playing to account for the choose-
  ** sides introduction.  We don't want an intro movie on scenario 1, and
  ** we don't want a briefing movie on GDI scenario 1.
  */
  if (TheWorld().scenario() < 20 &&
      (!TheSpecial().IsJurassic || !TheGameState().thingies_enabled())) {
    if (TheWorld().scenario() != 1 || TheWorld().whom() == HOUSE_GOOD) {
      Play_Movie(TheWorld().intro_movie());
    }

    if ((TheWorld().scenario() > 1 || TheWorld().whom() == HOUSE_BAD) &&
        briefing) {
      TheGameState().preserve_movie_screen() = TheWorld().scenario() == 1;
      Play_Movie(TheWorld().brief_movie());
    }
    Play_Movie(TheWorld().action_movie(), TheWorld().transit_theme());
    if (TheWorld().transit_theme() == THEME_NONE) {
      TheTheme().Queue_Song(THEME_AOI);
    }
  } else {
    Play_Movie(TheWorld().brief_movie());
    Play_Movie(TheWorld().action_movie(), TheWorld().transit_theme());

#ifdef NEWMENU

    char buffer[25];
    absl::SNPrintF(buffer, sizeof(buffer), "%s.VQA", TheWorld().brief_movie());
    GameFile file(buffer);

    if (TheSession().type() == GAME_NORMAL && !file.IsAvailable()) {
      TheScreen().visible_page().view().Clear();
      Set_Palette(ThePalettes().game_palette());
      //			Show_Mouse();
      /*
      ** Show the mission briefing. Pretend we are inside the main loop so the
      *palette
      ** will be correct on the textured buttons.
      */
      const bool oldinmain = TheGameState().in_main_loop();
      TheGameState().in_main_loop() = true;
      Restate_Mission(TheWorld().scenario_name(), TXT_OK, TXT_NONE);
      TheGameState().in_main_loop() = oldinmain;
      //			Hide_Mouse();
      if (TheWorld().transit_theme() == THEME_NONE) {
        TheTheme().Queue_Song(THEME_AOI);
      }
    }

#endif
  }
#endif

  /*
  ** Set the options values, since the palette has been initialized by
  *Read_Scenario
  */
  CCDebugString("C&C95 - About to call Options.Set.\n");
  TheOptions().Set();
  CCDebugString("C&C95 - About to return from Start_Scenario.\n");
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
bool Read_Scenario(char* root) {
  CCDebugString("C&C95 - In Read_Scenario.\n");
  Clear_Scenario();
  TheWorld().scenario_init()++;
  if (Read_Scenario_Ini(root)) {
    Fill_In_Data();
  } else {
    Fade_Palette_To(ThePalettes().game_palette(), kFadePaletteFast, Call_Back);
    Show_Mouse();
    CCMessageBox().Process(TXT_UNABLE_READ_SCENARIO);
    Hide_Mouse();
    return false;
  }
  TheWorld().scenario_init()--;
  CCDebugString("C&C95 - Leaving Read_Scenario.\n");
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
  **	Bring up the score display on the radar map when starting a multiplayer
  **	game.
  */
  if (TheSession().type() != GAME_NORMAL) {
    TheMap().Player_Names(true);
  }

  TheWorld().scenario_init()--;
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
  TheWorld().end_count_down() = kTicksPerSecond * 30;
  TheWorld().crate_count() = 0;
  TheWorld().crate_timer() = 0;
  TheWorld().crate_maker() = false;

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

  FactoryClass::Init();

  TheWorld().base().Init();

  TheWorld().current_object().Clear();
}

/***********************************************************************************************
 * Do_Win -- Display winning congratulations. *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT: *
 *                                                                                             *
 * OUTPUT: *
 *                                                                                             *
 * WARNINGS: *
 *                                                                                             *
 * HISTORY: * 08/05/1992 JLB : Created. * 01/01/1995 JLB : Carries money forward
 *into next scenario.                                *
 *=============================================================================================*/
void Do_Win() {
  TheMap().Set_Default_Mouse(MOUSE_NORMAL);
  Hide_Mouse();

  /*
  ** If this is a multiplayer game, clear the game's name so we won't respond
  ** to game queries any more (in Call_Back)
  */
  if (TheSession().type() != GAME_NORMAL) {
    base::At(TheSession().game_name(), 0) = 0;
  }

  /*
  **	Determine a cosmetic center point for the text.
  */
  const int x =
      TheMap().TacPixelX + (Lepton_To_Pixel(TheMap().TacLeptonWidth) / 2);
  const int y =
      TheMap().TacPixelY + (Lepton_To_Pixel(TheMap().TacLeptonHeight) / 2) - 32;

  /*
  **	Announce win to player.
  */
  PixelView& view = TheScreen().visible_view();
#if !(defined(GERMAN) || defined(FRENCH))
  Fancy_Text_Print(view, TXT_MISSION, x, y, kWhite, kTBlack,
                   TPF_CENTER | TPF_VCR);
#endif
  Fancy_Text_Print(view, TXT_SCENARIO_WON, x, y + 30, kWhite, kTBlack,
                   TPF_CENTER | TPF_VCR);
  TheGameState().speech_timer().Set(int64_t{kTimerSecond} * 3);
  Stop_Speaking();
  Speak(VOX_ACCOMPLISHED);
  while (TheGameState().speech_timer().Time() || Is_Speaking()) {
    Call_Back();
  }

  /*
  ** Stop here if this is a multiplayer game.
  */
  if (TheSession().type() != GAME_NORMAL) {
    if (!TheSession().playback_game()) {
      TheSession().games_played()++;
      Multi_Score_Presentation();
      TheSession().current_game()++;
      if (TheSession().current_game() >= MAX_MULTI_GAMES) {
        TheSession().current_game() = MAX_MULTI_GAMES - 1;
      }
    }
    TheGameState().active() = false;
    Show_Mouse();
    return;
  }

  /*
  **	Play the winning movie and then start the next scenario.
  */
  if (TheGameState().required_cd() != -2) {
    if (TheWorld().scenario() >= 20 && TheWorld().scenario() < 60 &&
        TheSession().type() == GAME_NORMAL) {
      TheGameState().required_cd() = 2;
    } else {
      if (TheWorld().scenario() >= 60) {
        TheGameState().required_cd() = -1;
      } else {
        if (ThePlayer()->Class->House == HOUSE_GOOD) {
          TheGameState().required_cd() = 0;
        } else {
          TheGameState().required_cd() = 1;
        }
      }
    }
  }

#ifndef DEMO
  Play_Movie(TheWorld().win_movie());
#endif

  Keyboard::Clear();

  /*
  **	Do the ending screens only if not playing back a recorded game.
  */
  if (!TheSession().playback_game()) {
#ifdef DEMO

    switch (TheWorld().scenario()) {
      case 1:
        TheWorld().score().Show();
        TheWorld().scenario() = 10;
        break;

      case 10:
        TheWorld().score().Show();
        TheWorld().scenario() = 6;
        break;

      default:
        TheWorld().score().Show();
        GDI_Ending();
        TheGameState().active() = false;
        Show_Mouse();
        return;
        //				Prog_End();
        //				exit(0);
        //				break;
    }

#else

#ifdef NEWMENU
    if (TheWorld().scenario() >= 20) {
      Keyboard::Clear();
      TheWorld().score().Show();
      TheGameState().active() = false;
      Show_Mouse();
      return;
    }
#endif

    if (ThePlayer()->Class->House == HOUSE_BAD && TheWorld().scenario() == 13) {
      Nod_Ending();
      // Prog_End();
      // exit(0);
      TheScreen().visible_view().Clear();
      Show_Mouse();
      TheGameState().active() = false;
      return;
    }
    if (ThePlayer()->Class->House == HOUSE_GOOD &&
        TheWorld().scenario() == 15) {
      GDI_Ending();
      // Prog_End();
      // exit(0);
      TheScreen().visible_view().Clear();
      Show_Mouse();
      TheGameState().active() = false;
      return;
    }

    if (TheSpecial().IsJurassic && TheGameState().thingies_enabled() &&
        TheWorld().scenario() == 5) {
      ShutDown();
      exit(0);
    }

    if (!TheSpecial().IsJurassic || !TheGameState().thingies_enabled()) {
      Keyboard::Clear();
      TheWorld().score().Show();

      /*
      **	Skip scenario #7 if the airfield was blown up.
      */
      if (TheWorld().scenario() == 6 &&
          ThePlayer()->Class->House == HOUSE_GOOD &&
          TheWorld().sabotaged_type() == STRUCT_AIRSTRIP) {
        TheWorld().scenario()++;
      }

      Map_Selection();
    }
    TheWorld().scenario()++;
#endif
    Keyboard::Clear();
  }

  TheWorld().carry_over_money() = static_cast<int>(ThePlayer()->Credits);

  const unsigned pieces = ThePlayer()->NukePieces;

  /*
  ** Generate a new scenario filename
  */
  Set_Scenario_Name(TheWorld().scenario_name(), TheWorld().scenario(),
                    TheWorld().scen_player(), TheWorld().scen_dir(),
                    TheWorld().scen_var());
  Start_Scenario(TheWorld().scenario_name());

  ThePlayer()->NukePieces = static_cast<uint8_t>(pieces);

  /*
  **	Destroy the building that was sabotaged in the previous scenario. This
  *only *	applies to GDI mission #7.
  */
  if (TheWorld().sabotaged_type() != STRUCT_NONE &&
      TheWorld().scenario() == 7 && ThePlayer()->Class->House == HOUSE_GOOD) {
    for (int index = 0; index < TheObjectHeaps().building().Count(); index++) {
      BuildingClass* building = TheObjectHeaps().building().Ptr(index);

      if (building && !building->IsInLimbo && building->House != ThePlayer() &&
          building->Class->Type == TheWorld().sabotaged_type()) {
        building->Limbo();
        delete building;
        break;
      }
    }

    /*
    **	Remove the building from the prebuild list.
    */
    for (int index = 0; index < TheWorld().base().Nodes.Count(); index++) {
      const BaseNodeClass* node = TheWorld().base().Get_Node(index);

      if (node && node->Type == TheWorld().sabotaged_type()) {
        TheWorld().base().Nodes.Delete(index);
        break;
      }
    }
  }
  TheWorld().sabotaged_type() = STRUCT_NONE;

  TheMap().Render();
  Fade_Palette_To(ThePalettes().game_palette(), kFadePaletteFast, Call_Back);
  Show_Mouse();
}

/***********************************************************************************************
 * Do_Lose -- Display losing comments. *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT: *
 *                                                                                             *
 * OUTPUT: *
 *                                                                                             *
 * WARNINGS: *
 *                                                                                             *
 * HISTORY: * 08/05/1992 JLB : Created. *
 *=============================================================================================*/
void Do_Lose() {
  TheMap().Set_Default_Mouse(MOUSE_NORMAL);
  Hide_Mouse();

  /*
  ** If this is a multiplayer game, clear the game's name so we won't respond
  ** to game queries any more (in Call_Back)
  */
  if (TheSession().type() != GAME_NORMAL) {
    base::At(TheSession().game_name(), 0) = 0;
  }

  /*
  **	Determine a cosmetic center point for the text.
  */
  const int x =
      TheMap().TacPixelX + (Lepton_To_Pixel(TheMap().TacLeptonWidth) / 2);
  const int y =
      TheMap().TacPixelY + (Lepton_To_Pixel(TheMap().TacLeptonHeight) / 2) - 32;

  /*
  **	Announce win to player.
  */
  PixelView& view = TheScreen().visible_view();
  Fancy_Text_Print(view, TXT_MISSION, x, y, kWhite, kTBlack,
                   TPF_CENTER | TPF_VCR);
  Fancy_Text_Print(view, TXT_SCENARIO_LOST, x, y + 30, kWhite, kTBlack,
                   TPF_CENTER | TPF_VCR);
  TheGameState().speech_timer().Set(int64_t{kTimerSecond} * 3);
  Stop_Speaking();
  Speak(VOX_FAIL);
  while (TheGameState().speech_timer().Time() || Is_Speaking()) {
    Call_Back();
  }

#ifdef OBSOLETE
  if (Debug_Play_Map) {
    Go_Editor(true);
    Show_Mouse();
    return;
  }
#endif

  /*
  ** Stop here if this is a multiplayer game.
  */
  if (TheSession().type() != GAME_NORMAL) {
    if (!TheSession().playback_game()) {
      TheSession().games_played()++;
      Multi_Score_Presentation();
      TheSession().current_game()++;
      if (TheSession().current_game() >= MAX_MULTI_GAMES) {
        TheSession().current_game() = MAX_MULTI_GAMES - 1;
      }
    }
    TheGameState().active() = false;
    Show_Mouse();
    return;
  }

  Play_Movie(TheWorld().lose_movie());

  /*
  ** Start same scenario again
  */
  Set_Palette(ThePalettes().game_palette());
  Show_Mouse();
  if (!TheSession().playback_game() &&
      !CCMessageBox().Process(TXT_TO_REPLAY, TXT_YES, TXT_NO)) {
    Hide_Mouse();
    Keyboard::Clear();
    Start_Scenario(TheWorld().scenario_name(), false);
    TheMap().Render();
  } else {
    Hide_Mouse();
    TheGameState().active() = false;
  }

  Fade_Palette_To(ThePalettes().game_palette(), kFadePaletteFast, Call_Back);
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
  const bool hidden = Get_Mouse_State() != 0;

  if (hidden) {
    Show_Mouse();
  }
  CCMessageBox().Process(TXT_RESTARTING, TXT_NONE);
  TheMap().Set_Default_Mouse(MOUSE_NORMAL);
  Keyboard::Clear();
  Start_Scenario(TheWorld().scenario_name(), false);
  if (hidden) {
    Hide_Mouse();
  }
  Keyboard::Clear();
  TheMap().Render();
}

/***********************************************************************************************
 * Restate_Mission -- Handles restating the mission objective. *
 *                                                                                             *
 *    This routine will display the mission objective (as text). It will also
 *give the         * option to redisplay the mission briefing video. *
 *                                                                                             *
 * INPUT:   name  -- The scenario name. This is the unique identifier for the
 *scenario         * briefing text as it appears in the "MISSION.INI" file. *
 *                                                                                             *
 * OUTPUT:  Returns the response from the dialog. This will either be 1 if the
 *video was       * requested, or 0 if the return to game options button was
 *selected.                 *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/23/1995 JLB : Created. * 08/06/1995 JLB : Uses preloaded
 *briefing text.                                            *
 *=============================================================================================*/
bool Restate_Mission(const char* name, int right_btn, int left_btn) {
  if (name) {
#ifdef JAPANESE
    char fname[14];
    port::SafeCopy(fname, name);
    port::SafeAppend(fname, ".CPS");

    if (GameFile(fname).IsAvailable()) {
      CCMessageBox box(TXT_NONE, true);
      return (box.Process(fname, right_btn, left_btn));
    }
#else
    /*
    **	Make sure that if there is no briefing movie, that the briefing text is
    **	the only option available.
    */
    bool brief = true;
#ifdef NEWMENU
    char buffer[25];
    char buffer1[25];
    absl::SNPrintF(buffer, sizeof(buffer), "%s.VQA", TheWorld().brief_movie());
    absl::SNPrintF(buffer1, sizeof(buffer1), "%s.VQA",
                   TheWorld().action_movie());
    GameFile file1(buffer);
    GameFile file2(buffer1);
    if (!file1.IsAvailable() && !file2.IsAvailable()) {
      right_btn = TXT_OK;
      left_btn = TXT_NONE;
      brief = false;
    }
#endif

    /*
    **	If mission object text was found, then display it.
    */
    if (!std::string_view(TheWorld().briefing_text()).empty()) {
      static char _buff[512];

      port::SafeCopy(_buff, TheWorld().briefing_text());
      // port::SafeCopy(_ShapeBuffer, BriefingText);

      const bool hidden = Get_Mouse_State() != 0;
      if (hidden) {
        Show_Mouse();
      }

      if (CCMessageBox(TXT_OBJECTIVE).Process(_buff, right_btn, left_btn)) {
        if (hidden) {
          Hide_Mouse();
        }
        return true;
      }
      if (hidden) {
        Hide_Mouse();
      }
      if (!brief) {
        return true;
      }
      return false;
    }
#endif
  }
  return false;
}
