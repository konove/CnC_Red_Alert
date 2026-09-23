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

/* $Header:   F:\projects\c&c\vcs\code\conquer.cpv   2.18   16 Oct 1995 16:50:24
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : CONQUER.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 3, 1991 *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * CC_Draw_Shape -- Custom draw shape handler. * Call_Back -- Main
 *game maintenance callback routine.                                      *
 *   Color_Cycle -- Handle the general palette color cycling. *
 *   Disk_Space_Available -- returns bytes of free disk space
 ** Do_Record_Playback -- handles saving/loading map pos & current object *
 *   Fading_Table_Name -- Builds a theater specific fading table name. *
 *   Fetch_Techno_Type -- Convert type and ID into TechnoTypeClass pointer. *
 *   Force_CD_Available -- Ensures that specified CD is available. *
 *   Get_Radar_Icon -- Builds and alloc a radar icon from a shape file *
 *   Handle_Team -- Processes team selection command. * Handle_View -- Either
 *records or restores the tactical view.                              *
 *   KN_To_Facing -- Converts a keyboard input number into a facing value. *
 *   Keyboard_Process -- Processes the tactical map input codes. * Language_Name
 *-- Build filename for current language.                                     *
 *   Main_Game -- Main game startup routine. * Main_Loop -- This is the main
 *game loop (as a single loop).                               * Map_Edit_Loop --
 *a mini-main loop for map edit mode only                                  *
 *   Message_Input -- allows inter-player message input processing *
 *   GameFileVqaIo -- Serves VQ file access. * Name_From_Source -- retrieves
 *the name for the given SourceType                           * Play_Movie --
 *Plays a VQ movie.                                                           *
 *   Source_From_Name -- Converts ASCII name into SourceType. * Sync_Delay --
 *Forces the game into a 15 FPS rate.                                         *
 *   Theater_From_Name -- Converts ASCII name into a theater number. *
 *   Trap_Object -- gets a ptr to object of given type & coord * Unselect_All --
 *Causes all selected objects to become unselected.                         *
 *   VQ_Call_Back -- Maintenance callback used for VQ movies. * Validate_Error
 *-- prints an error message when an object fails validation                 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/conquer.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/log/log.h"
#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "port/env.h"
#include "port/platform.h"
#include "port/safe_string.h"
#include "port/unaligned.h"
#include "sdllib/display.h"
#include "sdllib/font.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/pixel_buffer.h"
#include "sdllib/shape.h"
#include "sdllib/timer.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"
#include "sdllib/wwstd.h"
#include "td/aircraft.h"
#include "td/anim.h"
#include "td/assets.h"
#include "td/audio.h"
#include "td/base.h"
#include "td/building.h"
#include "td/bullet.h"
#include "td/config.h"
#include "td/const.h"
#include "td/debug.h"
#include "td/debug_state.h"
#include "td/defines.h"
#include "td/display.h"
#include "td/event.h"
#include "td/factory.h"
#include "td/foot.h"
#include "td/game_clock.h"
#include "td/game_state.h"
#include "td/goptions.h"
#include "td/heap.h"
#include "td/house.h"
#include "td/infantry.h"
#include "td/init.h"
#include "td/inline.h"
#include "td/input.h"
#include "td/internet.h"
#include "td/interpal.h"
#include "td/ipxaddr.h"
#include "td/ipxgconn.h"
#include "td/ipxmgr.h"
#include "td/jshell.h"
#include "td/keyframe.h"
#include "td/layer.h"
#include "td/logic.h"
#include "td/mapedit.h"
#include "td/mouse.h"
#include "td/mplayer.h"
#include "td/msgbox.h"
#include "td/msglist.h"
#include "td/netdlg.h"
#include "td/network.h"
#include "td/nulldlg.h"
#include "td/nullmgr.h"
#include "td/object.h"
#include "td/object_heaps.h"
#include "td/palette.h"
#include "td/palettes.h"
#include "td/queue.h"
#include "td/randomstate.h"
#include "td/saveload.h"
#include "td/scenario.h"
#include "td/score.h"
#include "td/screen.h"
#include "td/session.h"
#include "td/special.h"
#include "td/startup.h"
#include "td/startup_options.h"
#include "td/stats.h"
#include "td/target.h"
#include "td/tcpip.h"
#include "td/text.h"
#include "td/theme.h"
#include "td/type.h"
#include "td/unit.h"
#include "td/vector.h"
#include "td/winstub.h"
#include "td/world.h"
#include "tech/2keyfbuf.h"
#include "tech/archive.h"
#include "tech/audio_mixer.h"
#include "tech/byte_sink.h"
#include "tech/byte_stream.h"
#include "tech/crc.h"
#include "tech/game_file_vqa_io.h"
#include "tech/mix_archive.h"
#include "tech/search_paths.h"
#include "winvq/vqa32/vqaplay.h"

#ifdef _WIN32
#include "td/ccdde.h"
#endif

// Holds the end of the current frame; Main_Loop() sets it and Sync_Delay()
// waits it out.
static CountDownTimerClass frame_timer{0L};

// Measures how long one frame's logic takes, for the multiplayer frame rate.
static TimerClass process_timer;

// Set by VQ_Call_Back() when the player presses Esc to abort a movie, so
// Play_Movie() knows to clear the half-drawn frame.
static bool movie_broken_out;

// Where the message being typed goes: a broadcast address after F4, or the
// player picked with F1-F3. IPX only.
static IPXAddressClass message_address;

// Turned on at the [SyncBug] trap frame when CheckHeap is set; from then on
// Heap_Dump_Check() dumps the heaps.
static bool check_heap = false;

// The object Trap_Object() found for the [SyncBug] trap, for watching in a
// debugger; nullptr if none.
struct TrapObjectType {
  union {
    AircraftClass* Aircraft;
    AnimClass* Anim;
    BuildingClass* Building;
    BulletClass* Bullet;
    InfantryClass* Infantry;
    UnitClass* Unit;
    void* All;
  } Ptr;
};
static TrapObjectType trap_object = {nullptr};

/****************************************
**	Function prototypes for this module **
*****************************************/
#ifndef DEMO
static void Message_Input(KeyNumType& input);
#endif
static bool Color_Cycle();
static bool Map_Edit_Loop();
static void Trap_Object();

static void Do_Record_Playback();

/***********************************************************************************************
 * Main_Game -- Main game startup routine. *
 *                                                                                             *
 *    This is the first official routine of the game. It handles game
 *initialization and       * the main game loop control. *
 *                                                                                             *
 *    Initialization: *
 *    - Init_Game handles one-time-only inits *
 *    - Select_Game is responsible for initializations required for each new
 *game played       * (these may be different depending on whether a multiplayer
 *game is selected, and       * other parameters) *
 *    - This routine performs any un-inits required, both for each game played,
 *and one-time   *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/01/1994 JLB : Created. *
 *=============================================================================================*/
void Main_Game() {
  bool fade = false;  // don't fade title screen the first time through

  /*
  **	Perform one-time-only initializations
  */
  if (!Init_Game()) {
    return;
  }

  CCDebugString("C&C95 - Game initialisation complete.\n");
  /*
  **	Game processing loop:
  **	1) Select which game to play, or whether to exit (don't fade the palette
  **		on the first game selection, but fade it in on subsequent calls)
  **	2) Invoke either the main-loop routine, or the editor-loop routine,
  **		until they indicate that the user wants to exit the scenario.
  */
  while (Select_Game(fade)) {
    TheWorld().scenario_init() =
        0;  // Kludge.
            //		Theme.Queue_Song(THEME_PICK_ANOTHER);

    fade = true;

    /*
    **	Make the game screen visible, clear the keyboard buffer of spurious
    **	values, and then show the mouse.  This PRESUMES that Select_Game() has
    **	told the map to draw itself.
    */
    Fade_Palette_To(ThePalettes().game_palette(), kFadePaletteMedium, nullptr);
    Keyboard::Clear();

    /*
    ** Only show the mouse if we're not playing back a recording.
    */
    if (TheSession().playback_game()) {
      Hide_Mouse();
    } else {
      Show_Mouse();
    }

    TheGameState().special_dialog() = SDLG_NONE;
    // Start_Profiler();
    if (TheSession().type() == GAME_INTERNET) {
      Register_Game_Start_Time();
      TheNetwork().statistics_sent() = false;
      TheNetwork().packet_later() = nullptr;
      TheNetwork().connection_lost() = false;
    } else {
#ifdef _WIN32
      DDEServer.Disable();
#endif
    }

    TheGameState().in_main_loop() = true;

    if (config::kScenarioEditorEnabled) {
      /*
      **	Scenario-editor version of main-loop processing
      */
      for (;;) {
        /*
        **	Non-scenario-editor-mode: call the game's main loop
        */
        if (!TheDebugState().map_editor_active()) {
          if (Main_Loop()) {
            break;
          }

          if (TheGameState().special_dialog() != SDLG_NONE) {
            // Stop_Profiler();
            switch (TheGameState().special_dialog()) {
              case SDLG_SPECIAL:
                TheMap().Help_Text(TXT_NONE);
                TheMap().Override_Mouse_Shape(MOUSE_NORMAL, false);
                Special_Dialog();
                TheMap().Revert_Mouse_Shape();
                TheGameState().special_dialog() = SDLG_NONE;
                break;

              case SDLG_OPTIONS:
                TheMap().Help_Text(TXT_NONE);
                TheMap().Override_Mouse_Shape(MOUSE_NORMAL, false);
                TheOptions().Process();
                TheMap().Revert_Mouse_Shape();
                TheGameState().special_dialog() = SDLG_NONE;
                break;

              case SDLG_SURRENDER:
                TheMap().Help_Text(TXT_NONE);
                TheMap().Override_Mouse_Shape(MOUSE_NORMAL, false);
                if (Surrender_Dialog()) {
                  TheNetwork().out_list().Add(EventClass(EventClass::DESTRUCT));
                }
                TheGameState().special_dialog() = SDLG_NONE;
                TheMap().Revert_Mouse_Shape();
                break;

              case SpecialDialogType::SDLG_NONE:
              default:
                break;
            }
          }
        } else {
          /*
          **	Scenario-editor-mode: call the editor's main loop
          */
          if (Map_Edit_Loop()) {
            break;
          }
        }
      }
    } else {
      /*
      **	Non-editor version of main-loop processing
      */
      for (;;) {
        /*
        **	Call the game's main loop
        */
        if (Main_Loop()) {
          break;
        }

        /*
        **	If the SpecialDialog flag is set, invoke the given special
        *dialog. *	This must be done outside the main loop, since the
        *dialog will call *	Main_Loop(), allowing the game to run in the
        *background.
        */
        if (TheGameState().special_dialog() != SDLG_NONE) {
          // Stop_Profiler();
          switch (TheGameState().special_dialog()) {
            case SDLG_SPECIAL:
              TheMap().Help_Text(TXT_NONE);
              TheMap().Override_Mouse_Shape(MOUSE_NORMAL, false);
              Special_Dialog();
              TheMap().Revert_Mouse_Shape();
              TheGameState().special_dialog() = SDLG_NONE;
              break;

            case SDLG_OPTIONS:
              TheMap().Help_Text(TXT_NONE);
              TheMap().Override_Mouse_Shape(MOUSE_NORMAL, false);
              TheOptions().Process();
              TheMap().Revert_Mouse_Shape();
              TheGameState().special_dialog() = SDLG_NONE;
              break;

            case SDLG_SURRENDER:
              TheMap().Help_Text(TXT_NONE);
              TheMap().Override_Mouse_Shape(MOUSE_NORMAL, false);
              if (Surrender_Dialog()) {
                TheNetwork().out_list().Add(EventClass(EventClass::DESTRUCT));
              }
              TheGameState().special_dialog() = SDLG_NONE;
              TheMap().Revert_Mouse_Shape();
              break;

            case SpecialDialogType::SDLG_NONE:
            default:
              break;
          }
        }
      }
    }
    // Stop_Profiler();
    TheGameState().in_main_loop() = false;

    if (!TheNetwork().statistics_sent() && TheNetwork().packet_later()) {
      Send_Statistics_Packet();
    }

    /*
    **	Scenario is done; fade palette to black
    */
    Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteSlow, nullptr);
    TheScreen().visible_page().view().Clear();

#ifndef DEMO
    /*
    **	Un-initialize whatever needs it, for each game played.
    **
    **	Shut down either the modem or network; they'll get re-initialized if
    **	the user selections those options again in Select_Game().  This
    **	"re-boots" the modem & network code, which I currently feel is safer
    **	than just letting it hang around.
    ** (Skip this step if we're in playback mode; the modem or net won't have
    ** been initialized in that case.)
    */
    if (TheSession().record_game() || TheSession().playback_game()) {
      TheSession().record_stream().reset();
    }

    if (!TheSession().playback_game()) {
      switch (TheSession().type()) {
        case GAME_NULL_MODEM:
        case GAME_MODEM:
          Modem_Signoff();
          break;

        case GAME_IPX:
          Shutdown_Network();
          break;

        case GAME_INTERNET:
          // Winsock.Close();
        case GameType::GAME_NORMAL:
        default:
          break;
      }
    }

    /*
    **	If we're playing back, the mouse will be hidden; show it.
    ** Also, set all variables back to normal, to return to the main menu.
    */
    if (TheSession().playback_game()) {
      Show_Mouse();
      TheSession().type() = GAME_NORMAL;
      TheSession().playback_game() = false;
    }

    /*
    ** If we were spawned from WChat then dont go back to the main menu - just
    *quit
    **
    ** New: If spawned from WChat then maximise WChat and go back to the main
    *menu after all
    */
#ifdef FORCE_WINSOCK
    if (TheSpecial().IsFromWChat) {
      Shutdown_Network();  // Clear up the pseudo IPX stuff
      TheNetwork().winsock().Close();
      TheSpecial().IsFromWChat = false;
      TheGameState().spawned_from_chat() = false;
#ifdef _WIN32
      DDEServer.Delete_MPlayer_Game_Info();  // Make sure we dont use the same
                                             // start packet twice
#endif
      TheSession().type() = GAME_NORMAL;  // Have to do this or we will got
                                          // straight to the multiplayer menu
      Spawn_WChat(false);        // Will switch back to Wchat. It must be there
                                 // because its been poking us
      // break;
    }
#endif  // FORCE_WINSOCK

#endif  // DEMO
  }

#ifdef DEMO
  Hide_Mouse();
  Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium, NULL);
  Load_Title_Screen("DEMOPIC.PCX", &TheScreen().hidden_view(),
                    ThePalettes().title_palette());
  TheScreen().hidden_view().BlitTo(TheScreen().visible_view());
  Fade_Palette_To(ThePalettes().title_palette(), kFadePaletteMedium, NULL);
  Clear_KeyBuffer();
  Get_Key();
  Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium, NULL);
//		Show_Mouse();
#else

  /*
  **	Free the scenario description buffers
  */
  Free_Scenario_Descriptions();
#endif

  Uninit_Game();
}

/***********************************************************************************************
 * Keyboard_Process -- Processes the tactical map input codes. *
 *                                                                                             *
 *    This routine is used to process the input codes while the player * has the
 *tactical map displayed. It handles all the keys that * are appropriate to that
 *mode.                                                            *
 *                                                                                             *
 * INPUT:   input -- Input code as returned from Input_Num(). *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/21/1992 JLB : Created. * 07/04/1995 JLB : Handles team and map
 *control hotkeys.                                    *
 *=============================================================================================*/
extern int DebugColour;
void Keyboard_Process(KeyNumType& input) {
  ObjectClass* obj = nullptr;
  int index = 0;

  /*
  **	Don't do anything if there is not keyboard event.
  */
  if (input == KN_NONE) {
    return;
  }

#ifndef DEMO
  /*
  **	For network & modem, process user input for inter-player messages.
  */
  Message_Input(input);
#endif
  /*
  ** Use WWKEY values because KN values have WWKEY_VK_BIT or'd in with them
  ** and we need WWKEY_VK_BIT to still be set if it is.
  */
  const auto plain = static_cast<KeyNumType>(
      input & ~(WWKEY_SHIFT_BIT | WWKEY_ALT_BIT | WWKEY_CTRL_BIT));

  if constexpr (config::kCheatKeysEnabled) {
    if (TheDebugState().developer_mode()) {
      switch (static_cast<int>(input)) {
        case KN_M | KN_SHIFT_BIT:
        case KN_M | KN_ALT_BIT:
        case KN_M | KN_CTRL_BIT:
          ThePlayer()->Credits += 10000;
          break;

        default:
          break;
      }
    }
  }

  if constexpr (config::kVirginCheatKeysEnabled) {
    if (TheDebugState().playtest() && input == (KN_W | KN_ALT_BIT)) {
      ThePlayer()->Blockage = 0;
      ThePlayer()->Flag_To_Win();
    }
  }

  // #ifdef CHEAT_KEYS
  if (/*TheDebugState().playtest() && */ input == (KN_W | KN_ALT_BIT)) {
    ThePlayer()->Blockage = 0;
    ThePlayer()->Flag_To_Win();
  }

  if (TheDebugState().developer_mode() && input == KN_SLASH) {
    if (TheSession().type() != GAME_NORMAL) {
      TheGameState().special_dialog() = SDLG_SPECIAL;
      input = KN_NONE;
    } else {
      Special_Dialog();
    }
  }
  // #endif

  /*
  **	If the options key(s) were pressed, then bring up the options screen.
  */
  if (input == KN_SPACE || input == KN_ESC) {
    TheMap().Help_Text(TXT_NONE);  // Turns off help text.
    Queue_Options();
    input = KN_NONE;
    // DebugColour++;
    // DebugColour &=7;
  }

  /*
  **	Process prerecorded team selection. This will be an addative select
  **	if the SHIFT key is held down. It will create the team if the
  **	CTRL or ALT key is held down.
  */
  int action = 0;
  if (input & WWKEY_SHIFT_BIT) {
    action = 1;
  }
  if (input & WWKEY_ALT_BIT) {
    action = 3;
  }
  if (input & WWKEY_CTRL_BIT) {
    action = 2;
  }

  switch (KN_To_VK(plain)) {
    /*
    **	Center the map around the currently selected objects. If no
    **	objects are selected, then fall into the home case.
    */
    case VK_HOME:
      if (TheWorld().current_object().Count()) {
        TheMap().Center_Map();
        TheMap().Flag_To_Redraw(true);
        break;
      }
      [[fallthrough]];

    /*
    **	Center the map about the construction yard or construction vehicle
    **	if one is present.
    */
    case VK_H:
      for (index = 0; index < TheObjectHeaps().unit().Count(); index++) {
        UnitClass* unit = TheObjectHeaps().unit().Ptr(index);

        if (unit && !unit->IsInLimbo && unit->House == ThePlayer() &&
            *unit == UNIT_MCV) {
          Unselect_All();
          unit->Select();
          break;
        }
      }
      for (index = 0; index < TheObjectHeaps().building().Count(); index++) {
        BuildingClass* building = TheObjectHeaps().building().Ptr(index);

        if (building && !building->IsInLimbo &&
            building->House == ThePlayer() && *building == STRUCT_CONST) {
          Unselect_All();
          building->Select();
          break;
        }
      }
      TheMap().Center_Map();
      TheMap().Flag_To_Redraw(true);
      break;

    /*
    **	Toggle free scrolling mode.
    */
    case VK_F:
      if constexpr (config::kCheatKeysEnabled) {
        TheOptions().IsFreeScroll =
            !static_cast<bool>(TheOptions().IsFreeScroll);
      }
      break;

    /*
    **	If the "N" key is pressed, then select the next object.
    */
    case VK_N:
      if (action) {
        obj = MapEditClass::Prev_Object(TheWorld().current_object().Count()
                                            ? TheWorld().current_object().at(0)
                                            : nullptr);
      } else {
        obj = MapEditClass::Next_Object(TheWorld().current_object().Count()
                                            ? TheWorld().current_object().at(0)
                                            : nullptr);
      }
      if (obj) {
        Unselect_All();
        obj->Select();
        TheMap().Center_Map();
        TheMap().Flag_To_Redraw(true);
      }
      break;

    /*
    ** For multiplayer, 'R' pops up the surrender dialog.
    */
    case VK_R:
      if (/*GameToPlay != GAME_NORMAL &&*/ !ThePlayer()->IsDefeated) {
        TheGameState().special_dialog() = SDLG_SURRENDER;
        input = KN_NONE;
      }
      break;

    /*
    **	Handle making and breaking alliances.
    */
    case VK_A:
      if ((TheSession().type() != GAME_NORMAL ||
           TheDebugState().developer_mode()) &&
          (TheWorld().current_object().Count() && !ThePlayer()->IsDefeated) &&
          (TheWorld().current_object().at(0)->Owner() !=
           ThePlayer()->Class->House)) {
        TheNetwork().out_list().Add(EventClass(
            EventClass::ALLY,
            static_cast<int>(TheWorld().current_object().at(0)->Owner())));
      }

      break;

    /*
    **	Control the remembered tactical location.
    */
    case VK_F7:
    case VK_F8:
    case VK_F9:
    case VK_F10:
      if (!TheDebugState().map_editor_active()) {
        Handle_View(KN_To_VK(plain) - VK_F7, action);
      }
      break;

    /*
    **	Control the custom team select state.
    */
    case VK_1:
    case VK_2:
    case VK_3:
    case VK_4:
    case VK_5:
    case VK_6:
    case VK_7:
    case VK_8:
    case VK_9:
    case VK_0:
      Handle_Team(KN_To_VK(plain) - VK_1, action);
      break;

    /*
    **	All selected units will go into idle mode.
    */
    case VK_S:
      if (TheWorld().current_object().Count()) {
        for (int j = 0; j < TheWorld().current_object().Count(); j++) {
          const ObjectClass* tech = TheWorld().current_object().at(j);

          if (tech && (tech->Can_Player_Move() ||
                       (tech->Can_Player_Fire() &&
                        tech->What_Am_I() != RTTI_BUILDING))) {
            TheNetwork().out_list().Add(
                EventClass(EventClass::IDLE, tech->As_Target()));
          }
        }
      }
      break;

    /*
    **	All selected units will attempt to scatter.
    */
    case VK_X:
      if (TheWorld().current_object().Count()) {
        for (int j = 0; j < TheWorld().current_object().Count(); j++) {
          const ObjectClass* tech = TheWorld().current_object().at(j);

          if (tech && tech->Can_Player_Move()) {
            TheNetwork().out_list().Add(
                EventClass(EventClass::SCATTER, tech->As_Target()));
          }
        }
      }
      break;

    /*
    **	All selected units will attempt to go into guard area mode.
    */
    case VK_G:
      if (TheWorld().current_object().Count()) {
        for (int j = 0; j < TheWorld().current_object().Count(); j++) {
          const ObjectClass* tech = TheWorld().current_object().at(j);

          if (tech && tech->Can_Player_Move() && tech->Can_Player_Fire()) {
            TheNetwork().out_list().Add(
                EventClass(tech->As_Target(), MISSION_GUARD_AREA));
          }
        }
      }
      break;

    default:
      break;
  }

#ifdef NEVER
  FacingType facing = KN_To_Facing(input);

  /*
  **	Scroll the map according to the cursor key pressed.
  */
  if (facing != FACING_NONE) {
    TheMap().Scroll_Map(facing);
    input = 0;
    facing = FACING_NONE;
  }
#endif

#ifdef NEVER
  /*
  **	If the <TAB> key is pressed, then select the next object.
  */
  if (input == KN_TAB) {
    ObjectClass* obj = TheMap().Next_Object(CurrentObject);
    if (obj) {
      if (CurrentObject) {
        CurrentObject->Unselect();
      }
      obj->Select();
    }
  }
#endif

  if constexpr (config::kCheatKeysEnabled) {
    if (TheDebugState().developer_mode() && input &&
        (input & KN_RLSE_BIT) == 0) {
      Debug_Key(input);
    }
  }
}

#ifndef DEMO
/***********************************************************************************************
 * Message_Input -- allows inter-player message input processing *
 *                                                                                             *
 * INPUT: * input		key value
 **
 *                                                                                             *
 * OUTPUT: * none.
 **
 *                                                                                             *
 * WARNINGS: * MAX_MESSAGE_LENGTH has increased over the DOS version.
 *COMPAT_MESSAGE_LENGTH reflects   * * the length of the DOS message and also
 *the length of the message in the packet header.     * To allow transmission of
 *longer messages I split the message into COMPAT_MESSAGE_LENGTH-4  * sized
 *chunks and use the extra space after the zero terminator to specify which
 *segment    * of the whole message this is and also to supply a crc for the
 *string.                      * This allows message segments to arrive out of
 *order and still be displayed correctly.      *
 *                                                                                             *
 * HISTORY: * 05/22/1995 BRR : Created. * 03/26/1995  ST : Modified to break up
 *longer messages into multiple packets               *
 *=============================================================================================*/
static void Message_Input(KeyNumType& input) {
  char txt[MAX_MESSAGE_LENGTH + 12];
  int sent_so_far = 0;
  uint16_t magic_number = 0;
  uint16_t crc = 0;
  const int factor = TheScreen().visible_view().width() == 320 ? 1 : 2;

  /*
  **	Check keyboard input for a request to send a message.
  **	The 'to' argument for Add_Edit is prefixed to the message buffer; the
  **	message buffer is big enough for the 'to' field plus MAX_MESSAGE_LENGTH.
  **	To send the message, calling Get_Edit_Buf retrieves the buffer minus the
  **	'to' portion.  At the other end, the buffer allocated to display the
  **	message must be MAX_MESSAGE_LENGTH plus the size of "From: xxx (house)".
  */
  if (input >= KN_F1 && input < KN_F1 + TheSession().max_players() &&
      TheSession().messages().Get_Edit_Buf() == nullptr) {
    base::FillBytes(base::ObjectBytes(txt), 0, 40);

    /*
    **	For a serial game, send a message on F1 or F4; set 'txt' to the
    **	"Message:" string & add an editable message to the list.
    */
    if (TheSession().type() == GAME_NULL_MODEM ||
        TheSession().type() == GAME_MODEM) {
      //|| GameToPlay == GAME_INTERNET) {
      if (input == KN_F1 || input == KN_F1 + TheSession().max_players() - 1) {
        port::SafeCopy(txt, Text_String(TXT_MESSAGE));  // "Message:"

        TheSession().messages().Add_Edit(
            base::At(TheSession().text_colors(), TheSession().color_index()),
            TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW, txt,
            180 * factor);

        TheMap().Flag_To_Redraw(false);
      }
    } else {
      /*
      **	For a network game:
      **	F1-F3 = "To <name> (house):" (only allowed if we're not in
      *ObiWan mode) *	F4 = "To All:"
      */
      if (TheSession().type() == GAME_IPX ||
          TheSession().type() == GAME_INTERNET) {
        if (input == KN_F1 + TheSession().max_players() - 1 &&
            TheSession().messages().Get_Edit_Buf() == nullptr) {
          message_address = IPXAddressClass();           // set to broadcast
          port::SafeCopy(txt, Text_String(TXT_TO_ALL));  // "To All:"

          TheSession().messages().Add_Edit(
              base::At(TheSession().text_colors(), TheSession().color_index()),
              TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW, txt,
              180 * factor);

          TheMap().Flag_To_Redraw(false);
        } else {
          if ((TheSession().messages().Get_Edit_Buf() == nullptr) &&
              (input - KN_F1 < TheNetwork().ipx().Num_Connections() &&
               !TheSession().obi_wan())) {
            const int id = TheNetwork().ipx().Connection_ID(input - KN_F1);
            message_address = *TheNetwork().ipx().Connection_Address(id);
            Format_Runtime_Text(txt, sizeof(txt), Text_String(TXT_TO),
                                TheNetwork().ipx().Connection_Name(id));

            TheSession().messages().Add_Edit(
                base::At(TheSession().text_colors(),
                         TheSession().color_index()),
                TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW, txt,
                180 * factor);

            TheMap().Flag_To_Redraw(false);
          }
        }
      }
    }
  }

  /*
  ** Function key input is meaningless beyond this point
  */
  if (input >= KN_F1 && input <= KN_F10) {
    return;
  }
  if (input >= KN_F11 && input <= KN_F12) {
    return;
  }

  /*
  **	Process message-system input; send the message out if RETURN is hit.
  */
  const int rc = TheSession().messages().Input(input);

  /*
  **	If a single character has been added to an edit buffer, update the
  *display.
  */
  if (rc == 1) {
    TheMap().Flag_To_Redraw(false);
  }

  /*
  **	If backspace was hit, redraw the map.  This assumes the map is going to
  **	completely refresh all cells covered by the messages.  Set
  *DisplayClass's *	IsToRedraw to true to tell it to re-compute the cells
  *that it needs to *	redraw.
  */
  if (rc == 2) {
    TheMap().Flag_To_Redraw(false);
    TheMap().IsDisplayToRedraw = true;
  }

  /*
  **	Send a message
  */
  if (rc == 3) {
    /*.....................................................................
    Store this message in our LastMessage buffer; the computer may send
    us a version of it later.
    .....................................................................*/
    if (!std::string_view(TheSession().messages().Get_Edit_Buf()).empty()) {
      port::SafeCopy(TheSession().last_message(),
                     TheSession().messages().Get_Edit_Buf());
    }

    const int message_length = static_cast<int>(
        std::string_view(TheSession().messages().Get_Edit_Buf()).size());

    int32_t actual_message_size = 0;
    std::span<char> the_string;

    /*
    **	Serial game: fill in a SerialPacketType & send it.
    **	(Note: The size of the SerialPacketType.Command must be the same as
    **	the EventClass.Type!)
    */
    if (TheSession().type() == GAME_NULL_MODEM ||
        TheSession().type() == GAME_MODEM) {
      //|| GameToPlay==GAME_INTERNET) {

      sent_so_far = 0;
      magic_number = MESSAGE_HEAD_MAGIC_NUMBER;
      crc = static_cast<uint16_t>(
          CrcEngine::Compute(TheSession().messages().Get_Edit_Buf()) & 0xffff);

      while (sent_so_far < message_length) {
        SerialPacketType packet{.Command = SERIAL_MESSAGE,
                                .Name = {},
                                .Version = 0,
                                .House = HOUSE_NONE,
                                .Color = 0,
                                .Scenario = 0,
                                .Credits = 0,
                                .IsBases = 0,
                                .IsTiberium = 0,
                                .IsGoodies = 0,
                                .IsGhosties = 0,
                                .BuildLevel = 0,
                                .UnitCount = 0,
                                .Seed = 0,
                                .Special = {},
                                .GameSpeed = 0,
                                .ResponseTime = 0,
                                .Message = {},
                                .ID = 0};
        auto* serial_packet = &packet;

        serial_packet->Command = SERIAL_MESSAGE;
        port::SafeCopy(serial_packet->Name, TheSession().player_name());
        port::SafeCopy(
            std::span(serial_packet->Message).first(COMPAT_MESSAGE_LENGTH - 4),
            std::string_view(TheSession().messages().Get_Edit_Buf())
                .substr(base::ToSize(sent_so_far)));

        /*
        ** Steve I's stuff for splitting message on word boundries
        */
        actual_message_size = COMPAT_MESSAGE_LENGTH - 5;

        /* Start at the end of the message and find a space with 10 chars. */
        the_string = serial_packet->Message;
        while (COMPAT_MESSAGE_LENGTH - 5 - actual_message_size < 10 &&
               base::At(the_string, base::ToSize(actual_message_size)) != ' ') {
          --actual_message_size;
        }
        if (base::At(the_string, base::ToSize(actual_message_size)) == ' ') {
          /* Now delete the extra characters after the space (they musnt print)
           */
          for (int j = 0; j < COMPAT_MESSAGE_LENGTH - 5 - actual_message_size;
               j++) {
            base::At(the_string, base::ToSize(j + actual_message_size)) =
                static_cast<char>(0xff);
          }
        } else {
          actual_message_size = COMPAT_MESSAGE_LENGTH - 5;
        }

        base::At(serial_packet->Message, COMPAT_MESSAGE_LENGTH - 5) = 0;
        /*
        ** Flag this message segment as either a message head or a message tail.
        */
        port::WriteUnaligned(base::ObjectBytes(serial_packet->Message)
                                 .subspan(COMPAT_MESSAGE_LENGTH - 4),
                             magic_number);
        port::WriteUnaligned(base::ObjectBytes(serial_packet->Message)
                                 .subspan(COMPAT_MESSAGE_LENGTH - 2),
                             crc);
        serial_packet->ID = TheSession().local_id();

        TheNetwork().null_modem().Send_Message(base::ObjectBytes(packet),
                                               sizeof(SerialPacketType), 1);

        magic_number++;
        sent_so_far =
            sent_so_far + actual_message_size;  // COMPAT_MESSAGE_LENGTH-5;
      }

    } else {
      /*
      **	Network game: fill in a GlobalPacketType & send it.
      */
      if (TheSession().type() == GAME_IPX ||
          TheSession().type() == GAME_INTERNET) {
        sent_so_far = 0;
        magic_number = MESSAGE_HEAD_MAGIC_NUMBER;
        crc = static_cast<uint16_t>(
            CrcEngine::Compute(TheSession().messages().Get_Edit_Buf()) &
            0xffff);

        while (sent_so_far < message_length) {
          TheNetwork().global_packet().Command = NET_MESSAGE;
          port::SafeCopy(TheNetwork().global_packet().Name,
                         TheSession().player_name());
          port::SafeCopy(
              std::span(TheNetwork().global_packet().Message.Buf)
                  .first(COMPAT_MESSAGE_LENGTH - 4),
              std::string_view(TheSession().messages().Get_Edit_Buf())
                  .substr(base::ToSize(sent_so_far)));

          /*
          ** Steve I's stuff for splitting message on word boundries
          */
          actual_message_size = COMPAT_MESSAGE_LENGTH - 5;

          /* Start at the end of the message and find a space with 10 chars. */
          the_string = TheNetwork().global_packet().Message.Buf;
          while (COMPAT_MESSAGE_LENGTH - 5 - actual_message_size < 10 &&
                 base::At(the_string, base::ToSize(actual_message_size)) !=
                     ' ') {
            --actual_message_size;
          }
          if (base::At(the_string, base::ToSize(actual_message_size)) == ' ') {
            /* Now delete the extra characters after the space (they musnt
             * print) */
            for (int j = 0; j < COMPAT_MESSAGE_LENGTH - 5 - actual_message_size;
                 j++) {
              base::At(the_string, base::ToSize(j + actual_message_size)) =
                  static_cast<char>(0xff);
            }
          } else {
            actual_message_size = COMPAT_MESSAGE_LENGTH - 5;
          }

          base::At(TheNetwork().global_packet().Message.Buf,
                   COMPAT_MESSAGE_LENGTH - 5) = 0;
          /*
          ** Flag this message segment as either a message head or a message
          *tail.
          */
          port::WriteUnaligned(
              base::ObjectBytes(TheNetwork().global_packet().Message.Buf)
                  .subspan(COMPAT_MESSAGE_LENGTH - 4),
              magic_number);
          port::WriteUnaligned(
              base::ObjectBytes(TheNetwork().global_packet().Message.Buf)
                  .subspan(COMPAT_MESSAGE_LENGTH - 2),
              crc);

          TheNetwork().global_packet().Message.ID = TheSession().local_id();
          TheNetwork().global_packet().Message.NameCRC =
              Compute_Name_CRC(TheSession().game_name());

          /*
          **	If 'F4' was hit, message_address will be a broadcast address;
          *send *	the message to every player we have a connection with.
          */
          if (message_address.Is_Broadcast()) {
            for (int i = 0; i < TheNetwork().ipx().Num_Connections(); i++) {
              TheNetwork().ipx().Send_Global_Message(
                  base::ObjectBytes(TheNetwork().global_packet()),
                  sizeof(GlobalPacketType), 1,
                  TheNetwork().ipx().Connection_Address(
                      TheNetwork().ipx().Connection_ID(i)));
              TheNetwork().ipx().Service();
            }
          } else {
            /*
            **	Otherwise, message_address contains the exact address to send
            * to. *	Send to that address only.
            */
            TheNetwork().ipx().Send_Global_Message(
                base::ObjectBytes(TheNetwork().global_packet()),
                sizeof(GlobalPacketType), 1, &message_address);
            TheNetwork().ipx().Service();
          }

          magic_number++;
          sent_so_far =
              sent_so_far + actual_message_size;  // COMPAT_MESSAGE_LENGTH-5;
        }
      }
    }

    /*
    **	Tell the map to completely update itself, since a message is now
    *missing.
    */
    TheMap().Flag_To_Redraw(true);
  }
}
#endif

/***********************************************************************************************
 * Color_Cycle -- Handle the general palette color cycling. *
 *                                                                                             *
 *    This is a maintenance routine that handles the color cycling. It should be
 *called as     * often as necessary to achieve smooth color cycling effects --
 *at least 8 times a second. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  true if palette changed *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/31/1994 JLB : Created. * 06/10/1994 JLB : Uses new cycle color
 *values.                                             * 12/21/1994 JLB : Handles
 *text fade color.                                                 *
 *=============================================================================================*/
bool Color_Cycle() {
  static CountDownTimerClass _timer(0L);
  static CountDownTimerClass _ftimer(0L);
  static bool _up = false;
  bool changed = false;

  /*
  **	Process the fading white color. It is used for the radar box and other
  *glowing *	game interface elements.
  */
  if (!_ftimer.Time()) {
    _ftimer.Set(kTimerSecond / 8);

/*
**	Pulse the pulsing text color.
*/
#define STEP_RATE 5
    if (_up) {
      ThePalettes().game_palette().at(767) += STEP_RATE;
      ThePalettes().game_palette().at(766) += STEP_RATE;
      ThePalettes().game_palette().at(765) += STEP_RATE;
      if (ThePalettes().game_palette().at(767) > MAX_CYCLE_COLOR) {
        ThePalettes().game_palette().at(767) = MAX_CYCLE_COLOR;
        ThePalettes().game_palette().at(766) = MAX_CYCLE_COLOR;
        ThePalettes().game_palette().at(765) = MAX_CYCLE_COLOR;
        _up = false;
      }
    } else {
      ThePalettes().game_palette().at(767) -= STEP_RATE;
      ThePalettes().game_palette().at(766) -= STEP_RATE;
      ThePalettes().game_palette().at(765) -= STEP_RATE;
      if (static_cast<unsigned>(ThePalettes().game_palette().at(767)) <
          MIN_CYCLE_COLOR) {
        ThePalettes().game_palette().at(767) = MIN_CYCLE_COLOR;
        ThePalettes().game_palette().at(766) = MIN_CYCLE_COLOR;
        ThePalettes().game_palette().at(765) = MIN_CYCLE_COLOR;
        _up = true;
      }
    }
    changed = true;
  }

  /*
  **	Process the color cycling effects -- water.
  */
  if (!_timer.Time()) {
    unsigned char colors[3];

    _timer.Set(kTimerSecond / 4);

    const auto palette_bytes =
        std::as_writable_bytes(std::span(ThePalettes().game_palette()));
    base::CopyBytes(base::ObjectBytes(colors),
                    palette_bytes.subspan(base::ToSize(
                        (CYCLE_COLOR_START + CYCLE_COLOR_COUNT - 1) * 3)),
                    sizeof(colors));
    base::MoveBytes(
        palette_bytes.subspan(base::ToSize((CYCLE_COLOR_START + 1) * 3)),
        palette_bytes.subspan(base::ToSize(CYCLE_COLOR_START * 3)),
        base::ToSize((CYCLE_COLOR_COUNT - 1) * 3));
    base::CopyBytes(palette_bytes.subspan(base::ToSize(CYCLE_COLOR_START * 3)),
                    base::ObjectBytes(colors), sizeof(colors));
    changed = true;
  }

  /*
  **	If any of the processing functions changed the palette, then this
  *palette must be *	passed to the system.
  */
  if (changed) {
    Wait_Vert_Blank();
    Set_Palette(ThePalettes().game_palette());
    return true;
  }
  return false;
}

/***********************************************************************************************
 * Call_Back -- Main game maintenance callback routine. *
 *                                                                                             *
 *    This routine handles all the "real time" processing that needs to * occur.
 *This includes palette fading and sound updating. It needs * to be called as
 *often as possible.                                                       *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/07/1992 JLB : Created. *
 *=============================================================================================*/
void Call_Back() {
#ifndef DEMO
  int i = 0;
  int id = 0;
  int color = 0;
  uint16_t magic_number = 0;
  uint16_t crc = 0;
#endif

  /*
  **	Score maintenance
  */
  if (TheAudio().is_open()) {
    TheTheme().AI();
    Speak_AI();
  }

#ifndef DEMO
  /*
  **	Network maintenance
  */
  if (TheSession().type() == GAME_IPX || TheSession().type() == GAME_INTERNET) {
    TheNetwork().ipx().Service();

    /*
    ** Read packets only if the game is "closed", so we don't steal global
    ** messages from the connection dialogs.
    */
    if ((!TheNetwork().is_open()) &&
        TheNetwork().ipx().Get_Global_Message(
            base::ObjectBytes(TheNetwork().global_packet()),
            &TheNetwork().global_packet_length(),
            &TheNetwork().global_address(), &TheNetwork().product_id()) &&
        (TheNetwork().product_id() == IPXGlobalConnClass::kCommandAndConquer))

    {
      /*
      **	If this is another player signing off, remove the connection &
      **	mark that player's house as non-human, so the computer will take
      **	it over.
      */
      if (TheNetwork().global_packet().Command == NET_SIGN_OFF) {
        for (i = 0; i < TheNetwork().ipx().Num_Connections(); i++) {
          id = TheNetwork().ipx().Connection_ID(i);

          if ((std::string_view(TheNetwork().global_packet().Name) ==
               TheNetwork().ipx().Connection_Name(id)) &&
              TheNetwork().global_address() ==
                  *TheNetwork().ipx().Connection_Address(id)) {
            CCDebugString("C&C95 = Destroying connection due to sign off\n");
            Destroy_Connection(id, 0);
          }
        }
      } else {
        /*
        **	Process a message from another user.
        */
        if (TheNetwork().global_packet().Command == NET_MESSAGE) {
          bool msg_ok = false;
          char txt[80];

          /*
          ** If NetProtect is set, make sure this message came from within
          ** this game.
          */
          if (!TheNetwork().protect()) {
            msg_ok = true;
          } else {
            msg_ok = TheNetwork().global_packet().Message.NameCRC ==
                     Compute_Name_CRC(TheSession().game_name());
          }

          if (msg_ok) {
            Format_Runtime_Text(txt, sizeof(txt), Text_String(TXT_FROM),
                                TheNetwork().global_packet().Name,
                                TheNetwork().global_packet().Message.Buf);
            magic_number = port::ReadUnaligned<uint16_t>(
                base::ObjectBytes(TheNetwork().global_packet().Message.Buf)
                    .subspan(COMPAT_MESSAGE_LENGTH - 4));
            crc = port::ReadUnaligned<uint16_t>(
                base::ObjectBytes(TheNetwork().global_packet().Message.Buf)
                    .subspan(COMPAT_MESSAGE_LENGTH - 2));
            color = static_cast<int>(MPlayerID_To_ColorIndex(
                TheNetwork().global_packet().Message.ID));
            TheSession().messages().Add_Message(
                txt, base::At(TheSession().text_colors(), color),
                TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW, 600,
                magic_number, crc);

            /*
            **	Tell the map to do a partial update (just to force the
            *messages *	to redraw).
            */
            TheMap().Flag_To_Redraw(false);

            /*
            **	Save this message in our last-message buffer
            */
            if (!std::string_view(TheNetwork().global_packet().Message.Buf)
                     .empty()) {
              port::SafeCopy(TheSession().last_message(),
                             TheNetwork().global_packet().Message.Buf);
            }
          }
        } else {
          Process_Global_Packet(&TheNetwork().global_packet(),
                                &TheNetwork().global_address());
        }
      }
    }
  }

  /*
  **	Modem and Null Modem maintenance
  */
  if (TheSession().type() == GAME_NULL_MODEM ||
      (TheSession().type() == GAME_MODEM && TheNetwork().modem_service())) {
    //|| GameToPlay == GAME_INTERNET) {
    TheNetwork().null_modem().Service();
  }
#endif

  TheDisplay().EndFrame();
}

/***********************************************************************************************
 * Language_Name -- Build filename for current language. *
 *                                                                                             *
 *    This routine attaches a language specific suffix to the base * filename
 *provided. Typical use of this is when loading language * specific files at
 *game initialization time.                                              *
 *                                                                                             *
 * INPUT:   basename -- Base name to append language specific * extension to. *
 *                                                                                             *
 * OUTPUT:  Returns with pointer to completed filename. *
 *                                                                                             *
 * WARNINGS:   The return pointer value is valid only until the next time * this
 *routine is called.                                                         *
 *                                                                                             *
 * HISTORY: * 10/07/1992 JLB : Created. *
 *=============================================================================================*/
const char* Language_Name(const char* basename) {
  static char _fullname[port::kMaxFname + port::kMaxExt];

  if (!basename) {
    return nullptr;
  }

  absl::SNPrintF(_fullname, sizeof(_fullname), "%s.ENG", basename);
  return _fullname;
}

/***********************************************************************************************
 * Source_From_Name -- Converts ASCII name into SourceType. *
 *                                                                                             *
 *    This routine is used to convert an ASCII name representing a * SourceType
 *into the actual SourceType value. Typically, this is * used when processing
 *the scenario INI file.                                              *
 *                                                                                             *
 * INPUT:   name  -- The ASCII source name to process. *
 *                                                                                             *
 * OUTPUT:  Returns with the SourceType represented by the name * specified. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 04/17/1994 JLB : Created. *
 *=============================================================================================*/
SourceType Source_From_Name(const char* name) {
  if (name) {
    for (SourceType source = SOURCE_FIRST; source < SOURCE_COUNT; source++) {
      if (absl::EqualsIgnoreCase(SourceName.at(source), name)) {
        return source;
      }
    }
  }
  return SOURCE_NONE;
}

/***********************************************************************************************
 * Name_From_Source -- retrieves the name for the given SourceType
 **
 *                                                                         						  *
 * INPUT: * source		SourceType to get the name for
 **
 *                                                                         						  *
 * OUTPUT: * name of SourceType
 **
 *                                                                         						  *
 * WARNINGS: * none.
 **
 *                                                                         						  *
 * HISTORY: * 11/15/1994 BR : Created. *
 *=============================================================================================*/
const char* Name_From_Source(SourceType source) {
  if (static_cast<unsigned>(source) < static_cast<unsigned>(SOURCE_COUNT)) {
    return SourceName.at(source);
  }
  return "None";
}

/***********************************************************************************************
 * Theater_From_Name -- Converts ASCII name into a theater number. *
 *                                                                                             *
 *    This routine converts an ASCII representation of a theater and converts it
 *into a        * matching theater number. If no match was found, then
 *THEATER_NONE is returned.           *
 *                                                                                             *
 * INPUT:   name  -- Pointer to ASCII name to convert. *
 *                                                                                             *
 * OUTPUT:  Returns with the name converted into a theater number. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/01/1994 JLB : Created. *
 *=============================================================================================*/
TheaterType Theater_From_Name(const char* name) {

  if (name) {
    for (TheaterType index = THEATER_DESERT; index < THEATER_COUNT; index++) {
      if (absl::EqualsIgnoreCase(name, Theaters.at(index).Name)) {
        return index;
      }
    }
  }
  return THEATER_NONE;
}

/***********************************************************************************************
 * KN_To_Facing -- Converts a keyboard input number into a facing value. *
 *                                                                                             *
 *    This routine determine which compass direction is represented by the
 *keyboard value      * provided. It is used for map scrolling and other
 *directional control operations from     * the keyboard. *
 *                                                                                             *
 * INPUT:   input -- The KN number to convert. *
 *                                                                                             *
 * OUTPUT:  Returns with the facing type that the keyboard number represents. If
 *it could      * not be translated, then FACING_NONE is returned. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/28/1994 JLB : Created. *
 *=============================================================================================*/
FacingType KN_To_Facing(int input) {
  const uint32_t key = static_cast<uint32_t>(input) &
                       ~(WWKEY_ALT_BIT | WWKEY_SHIFT_BIT | WWKEY_CTRL_BIT);
  switch (key) {
    case KN_LEFT:
      return FACING_W;

    case KN_RIGHT:
      return FACING_E;

    case KN_UP:
      return FACING_N;

    case KN_DOWN:
      return FACING_S;

    case KN_UPLEFT:
      return FACING_NW;

    case KN_UPRIGHT:
      return FACING_NE;

    case KN_DOWNLEFT:
      return FACING_SW;

    case KN_DOWNRIGHT:
      return FACING_SE;
    default:
      break;
  }
  return FACING_NONE;
}

/***********************************************************************************************
 * Sync_Delay -- Forces the game into a 15 FPS rate. *
 *                                                                                             *
 *    This routine will wait until the timer for the current frame has expired
 *before          * returning. It is called at the end of every game loop in
 *order to force the game loop    * to run at a fixed rate. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   Will delay for up to 1/15 of a second. *
 *                                                                                             *
 * HISTORY: * 01/04/1995 JLB : Created. * 03/06/1995 JLB : Fixed. *
 *=============================================================================================*/
static void Sync_Delay() {
  /*
  **	Delay one tick.
  */
  while (frame_timer.Time()) {
    Color_Cycle();
    Call_Back();

    if (TheGameState().special_dialog() == SDLG_NONE) {
      TheMouse()->Erase_Mouse(&TheScreen().hidden_view(), true);
      KeyNumType input = KN_NONE;
      int x = 0;
      int y = 0;
      TheMouse()->Erase_Mouse(&TheScreen().hidden_view(), true);
      TheMap().Input(input, x, y);
      if (input) {
        Keyboard_Process(input);
      }
      TheMap().Render();
    }
  }
  Color_Cycle();
  Call_Back();
}

/***********************************************************************************************
 * Main_Loop -- This is the main game loop (as a single loop). *
 *                                                                                             *
 *    This function will perform one game loop. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Should the game end? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/01/1994 JLB : Created. *
 *=============================================================================================*/

bool Main_Loop() {
  KeyNumType input = KN_NONE;  // Player input.
  int x = 0;
  int y = 0;

  //	InMainLoop = true;

  /*
  ** I think I'm gonna cry if this makes it work
  */
  if (Get_Mouse_State()) {
    Show_Mouse();
  }

  /*
  ** Call the focus loss handler
  */
  Check_For_Focus_Loss();

  /*
  ** Sync-bug trapping code
  */
  if (CurrentFrame() >= TheDebugState().trap_frame()) {
    Trap_Object();
  }

  //
  // Initialize our AI processing timer
  //
  process_timer.Set(0, true);

  if (TheDebugState().trap_check_heap()) {
    check_heap = true;
  }

  if constexpr (config::kCheatKeysEnabled) {
    Heap_Dump_Check("After Trap");
  }

  /*
  **	If there is no theme playing, but it looks like one is required, then
  *start one *	playing. This is usually the symptom of there being no
  *transition score.
  */
  if (TheAudio().is_open() && TheTheme().What_Is_Playing() == THEME_NONE) {
    TheTheme().Queue_Song(THEME_PICK_ANOTHER);
  }

  /*
  **	Setup the timer so that the Main_Loop function processes at the correct
  *rate.
  */
  if (TheSession().type() != GAME_NORMAL &&
      TheSession().comm_protocol() == COMM_PROTOCOL_MULTI_E_COMP) {
    const int framedelay = 60 / TheSession().desired_frame_rate();
    frame_timer.Set(framedelay);
  } else {
    frame_timer.Set(TheOptions().GameSpeed);
  }

  /*
  **	Update the display, unless we're inside a dialog.
  */
  if ((!TheSession().playback_game()) &&
      (TheGameState().special_dialog() == SDLG_NONE &&
       TheGameState().in_focus())) {
    TheMouse()->Erase_Mouse(&TheScreen().hidden_view(), true);
    TheMap().Input(input, x, y);
    if (input) {
      Keyboard_Process(input);
    }
    //			HidPage.Lock();
    TheMap().Render();
    //			HidPage.Unlock();
  }

  /*
  ** Save map's position & selected objects, if we're recording the game.
  */
  if (TheSession().record_game() || TheSession().playback_game()) {
    Do_Record_Playback();
  }

  /*
  ** Sort the map's ground layer by y-coordinate value.  This is done
  ** outside the IsToRedraw check, for the purposes of game sync'ing
  ** between machines; this way, all machines will sort the Map's
  ** layer in the same way, and any processing done that's based on
  ** the order of this layer will sync on different machines.
  */
  MouseClass::Layer.at(LAYER_GROUND).Sort();

  //	Heap_Dump_Check( "Before Logic.AI" );

  /*
  **	AI logic operations are performed here.
  */
  TheWorld().logic().AI();

  //	Heap_Dump_Check( "After Logic.AI" );

  /*
  **	Manage the inter-player message list.  If Manage() returns true, it
  *means *	a message has expired & been removed, and the entire map must be
  *updated.
  */
  if (TheSession().messages().Manage()) {
    TheScreen().hidden_page().view().Clear();
    TheMap().Flag_To_Redraw(true);
  }

  //
  // Measure how long it took to process the AI
  //
  TheSession().process_ticks() =
      static_cast<int>(TheSession().process_ticks() + process_timer.Time());
  TheSession().process_frames()++;

  //	Heap_Dump_Check( "Before Queue_AI" );

  /*
  **	Process all commands that are ready to be processed.
  */
  Queue_AI();

  // Heap_Dump_Check( "After Queue_AI" );

  /*
  **	Keep track of elapsed time in the game.
  */
  TheWorld().score().ElapsedTime += kTimerSecond / kTicksPerSecond;

  Call_Back();

  // Heap_Dump_Check( "After Call_Back" );

  /*
  **	Perform any win/lose code as indicated by the global control flags.
  */
  if (TheWorld().end_count_down()) {
    TheWorld().end_count_down()--;
  }

  /*
  **	Check for player wins or loses according to global event flag.
  */

  if (TheGameState().player_wins()) {
    if (TheSession().type() == GAME_INTERNET &&
        !TheNetwork().statistics_sent()) {
      Register_Game_End_Time();
      Send_Statistics_Packet();
    }

    TheMouse()->Erase_Mouse(&TheScreen().hidden_view(), true);
    TheGameState().player_loses() = false;
    TheGameState().player_wins() = false;
    TheGameState().player_restarts() = false;
    TheMap().Help_Text(TXT_NONE);
    Do_Win();
  }
  if (TheGameState().player_loses()) {
    if (TheSession().type() == GAME_INTERNET &&
        !TheNetwork().statistics_sent()) {
      Register_Game_End_Time();
      Send_Statistics_Packet();
    }

    TheMouse()->Erase_Mouse(&TheScreen().hidden_view(), true);
    TheGameState().player_wins() = false;
    TheGameState().player_loses() = false;
    TheGameState().player_restarts() = false;
    TheMap().Help_Text(TXT_NONE);
    Do_Lose();
  }
  if (TheGameState().player_restarts()) {
    TheMouse()->Erase_Mouse(&TheScreen().hidden_view(), true);
    TheGameState().player_wins() = false;
    TheGameState().player_loses() = false;
    TheGameState().player_restarts() = false;
    TheMap().Help_Text(TXT_NONE);
    Do_Restart();
  }

  /*
  **	The frame logic has been completed. Increment the frame
  **	counter.
  */
  TheGameClock().Advance();

  // Record mobile-object state and optionally save before ending a smoke run.
  if (TheStartupOptions().quit_at_frame >= 0) {
    for (int index = 0; index < TheObjectHeaps().unit().Count(); ++index) {
      const UnitClass* unit = TheObjectHeaps().unit().Ptr(index);
      LOG(INFO) << "frame " << CurrentFrame() << " unit "
                << unit->Class->IniName << " coord " << unit->Coord
                << " mission " << static_cast<int>(unit->Mission) << " navcom "
                << unit->NavCom;
    }
    for (int index = 0; index < TheObjectHeaps().infantry().Count(); ++index) {
      const InfantryClass* infantry = TheObjectHeaps().infantry().Ptr(index);
      LOG(INFO) << "frame " << CurrentFrame() << " infantry "
                << TheObjectHeaps().infantry().ID(infantry) << " coord "
                << infantry->Coord << " mission "
                << static_cast<int>(infantry->Mission) << " navcom "
                << infantry->NavCom;
    }
    // Compare every serialized field of migrated objects in smoke runs.
    const auto log_heap = [](auto& heap, const char* kind) {
      for (int index = 0; index < heap.Count(); ++index) {
        // Stream directly to hex so growing field lists cannot be truncated.
        class HexSink : public ByteSink {
         public:
          std::string fields;
          bool Write(std::span<const std::byte> bytes) override {
            constexpr char hex[] = "0123456789abcdef";
            for (const std::byte byte : bytes) {
              const auto value = std::to_integer<uint8_t>(byte);
              fields += base::At(hex, value >> 4);
              fields += base::At(hex, value & 15);
            }
            return true;
          }
        } sink;
        ArchiveWriter writer(sink);
        heap.Ptr(index)->Serialize(writer);
        LOG(INFO) << "frame " << CurrentFrame() << " " << kind << " "
                  << heap.ID(heap.Ptr(index)) << " fields " << sink.fields;
      }
    };
    log_heap(TheObjectHeaps().factory(), "factory");
    log_heap(TheObjectHeaps().trigger(), "trigger");
    log_heap(TheObjectHeaps().team_type(), "teamtype");
    log_heap(TheObjectHeaps().team(), "team");
    log_heap(TheObjectHeaps().house(), "house");
    log_heap(TheObjectHeaps().tmplate(), "template");
    log_heap(TheObjectHeaps().overlay(), "overlay");
    log_heap(TheObjectHeaps().smudge(), "smudge");
    log_heap(TheObjectHeaps().anim(), "anim");
    log_heap(TheObjectHeaps().terrain(), "terrain");
    log_heap(TheObjectHeaps().bullet(), "bullet");
    log_heap(TheObjectHeaps().building(), "building");
    log_heap(TheObjectHeaps().unit(), "unitstate");
    log_heap(TheObjectHeaps().infantry(), "infantrystate");
    log_heap(TheObjectHeaps().aircraft(), "aircraftstate");
    class MapHashSink : public ByteSink {
     public:
      bool trace = port::GetEnv("TD_MAP_TRACE").has_value();
      std::string fields;
      uint64_t hash = 14695981039346656037ULL;
      bool Write(std::span<const std::byte> bytes) override {
        for (const std::byte byte : bytes) {
          const auto value = std::to_integer<uint8_t>(byte);
          hash = (hash ^ value) * 1099511628211ULL;
          if (trace) {
            constexpr char hex[] = "0123456789abcdef";
            fields += base::At(hex, value >> 4);
            fields += base::At(hex, value & 15);
          }
        }
        return true;
      }
    } map_sink;
    ArchiveWriter map_writer(map_sink);
    TheMap().Serialize(map_writer);
    LOG(INFO) << "frame " << CurrentFrame() << " mapstate " << map_sink.hash;
    MapHashSink globals_sink;
    ArchiveWriter globals_writer(globals_sink);
    TheWorld().score().Serialize(globals_writer);
    TheWorld().base().Serialize(globals_writer);
    TheWorld().logic().Serialize(globals_writer);
    for (auto& layer : MouseClass::Layer) {
      layer.Serialize(globals_writer);
    }
    Save_Misc_Values(globals_writer);
    LOG(INFO) << "frame " << CurrentFrame() << " globalstate "
              << globals_sink.hash;

    if (map_sink.trace && (CurrentFrame() == 60 || CurrentFrame() == 61)) {
      // Keep each record below the logger's message-size limit.
      for (size_t offset = 0; offset < map_sink.fields.size(); offset += 2048) {
        LOG(INFO) << "frame " << CurrentFrame() << " mapfields " << offset
                  << " " << map_sink.fields.substr(offset, 2048);
      }
    }

    for (int i = 0; i < TheObjectHeaps().team_type().Count(); ++i) {
      const int id =
          TheObjectHeaps().team_type().ID(TheObjectHeaps().team_type().Ptr(i));
      LOG(INFO) << "frame " << CurrentFrame() << " teamcount " << id << " "
                << static_cast<int>(base::At(TeamClass::Number, id));
    }
    if (CurrentFrame() >= TheStartupOptions().quit_at_frame) {
      if (TheStartupOptions().save_slot >= 0) {
        char description[] = "debug";
        if (!Save_Game(TheStartupOptions().save_slot, description)) {
          LOG(ERROR) << "-SAVESLOT: could not save slot "
                     << TheStartupOptions().save_slot;
        }
      }
      TheGameState().active() = false;
      return true;
    }
  }

  /*
  ** Very rarely, the human players will get a message from the computer.
  */
  if (TheSession().type() != GAME_NORMAL && TheSession().ghosts() &&
      GameRandomRange(0, 10000) == 1) {
    Computer_Message();
  }

  /*
  ** Is there a memory trasher altering the map??
  */
  if (TheDebugState().check_map() && (!TheMap().Validate())) {
    const char* error_msg = nullptr;
    const char* stop_msg = nullptr;
    const char* continue_msg = nullptr;
    if constexpr (config::kBuildLanguage == config::BuildLanguage::German) {
      error_msg = "Kartenfehler!";
      stop_msg = "Halt";
      continue_msg = "Weiter";
    } else if constexpr (config::kBuildLanguage ==
                         config::BuildLanguage::French) {
      error_msg = "Erreur de carte!";
      stop_msg = "Stop";
      continue_msg = "Continuer";
    } else {
      error_msg = "Map Error!";
      stop_msg = "Stop";
      continue_msg = "Continue";
    }
    if (CCMessageBox().Process(error_msg, stop_msg, continue_msg) == 0) {
      TheGameState().active() = false;
    }
    TheMap().Validate();  // give debugger a chance to catch it
  }

  Sync_Delay();
  //	InMainLoop = false;
  return !TheGameState().active();
}

/***************************************************************************
 * Map_Edit_Loop -- a mini-main loop for map edit mode only                *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/19/1994 BR : Created.                                              *
 *=========================================================================*/
bool Map_Edit_Loop() {
  /*
  **	Redraw the map.
  */
  TheMap().Render();

  /*
  **	Get user input (keys, mouse clicks).
  */
  KeyNumType input = KN_NONE;

  int x = 0;
  int y = 0;
  TheMap().Input(input, x, y);

  /*
  **	Process keypress.
  */
  if (input) {
    Keyboard_Process(input);
  }

  Call_Back();  // maintains Theme.AI() for music
  Color_Cycle();

  return (!TheGameState().active());
}

/***************************************************************************
 * Go_Editor -- Enables/disables the map editor
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		flag		true = go into editor mode; false = go into game
 *mode			*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   10/19/1994 BR : Created.                                              *
 *=========================================================================*/
void Go_Editor(bool flag) {
  /*
  **	Go into Scenario Editor mode
  */
  if (flag) {
    TheDebugState().set_map_editor_active(true);
    TheDebugState().set_unshroud(true);

    /*
    ** Un-select any selected objects
    */
    Unselect_All();

    /*
    ** Turn off the sidebar if it's on
    */
    TheMap().Activate(0);

    /*
    ** Reset the map's Button list for the new mode
    */
    TheMap().Init_IO();

    /*
    ** Force a complete redraw of the screen
    */
    TheScreen().hidden_page().view().Clear();
    TheMap().Flag_To_Redraw(true);
    TheMap().Render();

  } else {
    /*
    **	Go into normal game mode
    */
    TheDebugState().set_map_editor_active(false);
    TheDebugState().set_unshroud(false);

    /*
    ** Un-select any selected objects
    */
    Unselect_All();

    /*
    ** Reset the map's Button list for the new mode
    */
    TheMap().Init_IO();

    /*
    ** Force a complete redraw of the screen
    */
    TheScreen().hidden_page().view().Clear();
    TheMap().Flag_To_Redraw(true);
    TheMap().Render();
  }
}

/***********************************************************************************************
 * Play_Movie -- Plays a VQ movie. *
 *                                                                                             *
 *    Use this routine to play a VQ movie. It will disptach the specified movie
 *to the         * VQ player. The routine will not return until the movie has
 *finished playing.             *
 *                                                                                             *
 * INPUT:   file  -- The file object that contains the movie. *
 *                                                                                             *
 *          anim  -- The anim control and configuration structure that controls
 *how the        * movie will be played. *
 *                                                                                             *
 *          clrscrn -- Set to 1 to clear the screen when the movie is over *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/19/1994 JLB : Created. *
 *=============================================================================================*/
void Play_Movie(const char* name, ThemeType theme, bool clear_screen) {
  if (TheStartupOptions().no_movies) {
    return;
  }
  /*
  ** Don't play movies in editor mode
  */
  if (TheDebugState().map_editor_active()) {
    return;
  }

  /*
  ** Don't play movies in multiplayer mode
  */
  if (TheSession().type() != GAME_NORMAL) {
    return;
  }

  if (name) {
    const auto fullname =
        std::filesystem::path(name).replace_extension(".VQA").string();

    /*
    **	Reset the anim control structure.
    */
    Anim_Init();
    Discard_VQ_Palette_Change();

    /*
    **	Prepare to play a movie. First hide the mouse and stop any score that is
    *playing. *	While the score (if any) is fading to silence, fade the palette
    *to black as well. *	When the palette has finished fading, wait until
    *the score has finished fading *	before launching the movie.
    */
    Hide_Mouse();
    // Theme.Stop();
    // Theme.AI();
    TheTheme().Queue_Song(theme);
    if (!TheGameState().preserve_movie_screen()) {
      Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium,
                      Call_Back);
      TheScreen().visible_page().view().Clear();
      std::ranges::fill(ThePalettes().black_palette(), 0x01);
      Set_Palette(ThePalettes().black_palette());
      std::ranges::fill(ThePalettes().black_palette(), 0x00);
    }
    TheGameState().preserve_movie_screen() = false;
    Keyboard::Clear();

    VqaPlayer player;
    GameFileVqaIo movie_io;  // Must outlive the open movie.
    player.SetIo(&movie_io);

    if (!TheDebugState().quiet() && TheAudio().is_open()) {
      TheGameState().anim_control().OptionFlags |= VQAOPTF_AUDIO;
    } else {
      TheGameState().anim_control().OptionFlags &= ~VQAOPTF_AUDIO;
    }

    if (player.Open(fullname.c_str(), &TheGameState().anim_control()) == 0) {
      movie_broken_out = false;
      // Suspend_Audio_Thread();

      // Set_Palette(BlackPalette);
      TheScreen().sys_mem_page().view().Clear();
      TheGameState().in_movie() = true;
      player.Play(VQAMODE_RUN);
      player.Close();
      // Resume_Audio_Thread();
      TheGameState().in_movie() = false;
      /*
      **	Any movie that ends prematurely must have the screen
      **	cleared to avoid any unexpected palette glitches.
      */
      if (movie_broken_out) {
        clear_screen = true;
        TheScreen().visible_page().view().Clear();
        movie_broken_out = false;
      }
    }

    /*
    **	Presume that the screen is left in a garbage state as well as the
    *palette *	being in an unknown condition. Recover from this by clearing the
    *screen and *	forcing the palette to black.
    */
    if (clear_screen) {
      TheScreen().visible_page().view().Clear();
      std::ranges::fill(ThePalettes().black_palette(), 0x01);
      Set_Palette(ThePalettes().black_palette());
      std::ranges::fill(ThePalettes().black_palette(), 0x00);
      Set_Palette(ThePalettes().black_palette());
    }
    Show_Mouse();
  }
}

/***********************************************************************************************
 * Unselect_All -- Causes all selected objects to become unselected. *
 *                                                                                             *
 *    This routine will unselect all objects that are currently selected. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/19/1995 JLB : Created. *
 *=============================================================================================*/
void Unselect_All() {
  while (TheWorld().current_object().Count()) {
    TheWorld().current_object().at(0)->Unselect();
  }
}

/***********************************************************************************************
 * Fading_Table_Name -- Builds a theater specific fading table name. *
 *                                                                                             *
 *    This routine builds a standard fading table name. This name is dependant
 *on the theater  * being played, since each theater has its own palette. *
 *                                                                                             *
 * INPUT:   base  -- The base name of this fading table. The base name can be no
 *longer than   * seven characters. *
 *                                                                                             *
 *          theater  -- The theater that this fading table is specific to. *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the constructed fading table filename.
 *This pointer is   * valid until this function is called again. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/19/1995 JLB : Created. *
 *=============================================================================================*/
std::string Fading_Table_Name(const char* base, TheaterType theater) {
  // Build filename: first character of theater root + base name + .MRF
  // extension
  const auto root =
      std::string(1, base::At(Theaters.at(theater).Root, 0)) + base;
  const auto file_path = std::filesystem::path(root).replace_extension(".MRF");
  return file_path.string();
}

/***********************************************************************************************
 * Get_Radar_Icon -- Builds and alloc a radar icon from a shape file *
 *                                                                                             *
 * INPUT:      void const * shapefile - pointer to a key framed shapefile * int
 *shapenum          - shape to extract from shapefile                         *
 *                                                                                             *
 * OUTPUT:     void const *           - 3/3 icon set of shape from file *
 *                                                                                             *
 * HISTORY: * 04/12/1995 PWG : Created. * 05/10/1995 JLB : Handles a null
 *shapefile pointer.                                        *
 *=============================================================================================*/
std::vector<uint8_t> Get_Radar_Icon(std::span<const std::byte> shapefile,
                                    int shapenum, int frames, int zoomfactor) {
  static constexpr int kOffsets[] = {0, 0, -1, 1, 0, -1, 1, -1, 1};
  if (shapefile.empty() || shapenum < 0 || zoomfactor <= 0 || zoomfactor > 24) {
    return {};
  }
  const int pixel_width = Get_Build_Frame_Width(shapefile);
  const int pixel_height = Get_Build_Frame_Height(shapefile);
  const int icon_width = (pixel_width + 12) / 24;
  const int icon_height = (pixel_height + 12) / 24;
  if (frames == -1) {
    frames = Get_Build_Frame_Count(shapefile) - shapenum;
  }
  if (frames <= 0 || icon_width <= 0 || icon_height <= 0) {
    return {};
  }
  const auto frame_pixels = base::ToSize(icon_width) *
                            base::ToSize(icon_height) *
                            base::ToSize(zoomfactor) * base::ToSize(zoomfactor);
  std::vector<uint8_t> result(2 + (frame_pixels * base::ToSize(frames)));
  result.at(0) = static_cast<uint8_t>(icon_width);
  result.at(1) = static_cast<uint8_t>(icon_height);
  const int step = 24 / zoomfactor;
  size_t out = 2;
  for (int frame = 0; frame < frames; ++frame) {
    const auto pixels =
        Build_Frame(shapefile, static_cast<uint16_t>(shapenum + frame),
                    TheScreen().sys_mem_page().bytes());
    if (pixels.empty()) {
      out += frame_pixels;
      continue;
    }
    for (int icon_y = 0; icon_y < icon_height; ++icon_y) {
      for (int icon_x = 0; icon_x < icon_width; ++icon_x) {
        for (int y = 0; y < zoomfactor; ++y) {
          for (int x = 0; x < zoomfactor; ++x) {
            const int get_x = (icon_x * 24) + (x * step) + (zoomfactor / 2);
            const int get_y = (icon_y * 24) + (y * step) + (zoomfactor / 2);
            uint8_t pixel = 0;
            if (get_x < pixel_width && get_y < pixel_height) {
              for (const int offset : kOffsets) {
                const int sample_x = get_x - offset;
                const int sample_y = get_y - offset;
                if (sample_x < 0 || sample_x >= pixel_width || sample_y < 0 ||
                    sample_y >= pixel_height) {
                  continue;
                }
                pixel = base::At(
                    pixels, base::ToSize((sample_y * pixel_width) + sample_x));
                if (pixel == kLtGreen) {
                  pixel = 0;
                }
                if (pixel != 0) {
                  break;
                }
              }
            }
            result.at(out++) = pixel;
          }
        }
      }
    }
  }
  return result;
}

void CC_Texture_Fill(PixelView& view, std::span<const std::byte> shapefile,
                     int shapenum, int xpos, int ypos, int width, int height) {
  if (shapefile.empty() || shapenum < 0) {
    return;
  }
  const auto pixels =
      Build_Frame(shapefile, static_cast<uint16_t>(shapenum), ShapeBufferBytes);
  const int source_width = Get_Build_Frame_Width(shapefile);
  const int source_height = Get_Build_Frame_Height(shapefile);
  if (pixels.empty() || source_width == 0 || source_height == 0 ||
      !view.Lock()) {
    return;
  }
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      view.PutPixel(
          xpos + x, ypos + y,
          base::At(pixels, base::ToSize(((y % source_height) * source_width) +
                                        (x % source_width))));
    }
  }
  view.Unlock();
}

/***********************************************************************************************
 * CC_Draw_Shape -- Custom draw shape handler. *
 *                                                                                             *
 *    All draw shape calls will route through this function. It handles all
 *draws for          * C&C. Such draws always occur to the logical page and
 *assume certain things about         * the parameters passed. *
 *                                                                                             *
 * INPUT:   shapefile   -- Pointer to the shape data file. This data file
 *contains all the     * embedded shapes. *
 *                                                                                             *
 *          shapenum    -- The shape number within the shapefile that will be
 *drawn.           *
 *                                                                                             *
 *          x,y         -- The pixel coordinates to draw the shape. *
 *                                                                                             *
 *          window      -- The clipping window to use. *
 *                                                                                             *
 *          flags       -- The custom draw shape flags. This controls how the
 *parameters       * are used (if any). *
 *                                                                                             *
 *          fadingdata  -- If SHAPE_FADING is desired, then this points to the
 *fading          * data table. *
 *                                                                                             *
 *          ghostdata   -- If SHAPE_GHOST is desired, then this points to the
 *ghost remap      * table. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 02/21/1995 JLB : Created. *
 *=============================================================================================*/
void CC_Draw_Shape(PixelView& view, std::span<const std::byte> shapefile,
                   int shapenum, int x, int y, WindowNumberType window,
                   ShapeFlags_Type flags, std::span<const uint8_t> fadingdata,
                   std::span<const uint8_t> ghostdata) {
  if (!shapefile.empty() && shapenum >= 0) {
    /*
    ** Build frame returns a pointer now instead of the shapes length
    */
    const auto shape_size = Build_Frame(
        shapefile, static_cast<uint16_t>(shapenum), ShapeBufferBytes);
    if (Get_Last_Frame_Length() > ShapeBufferSize) {
      LOG(ERROR) << "Shape buffer too small: need " << Get_Last_Frame_Length()
                 << " bytes, buffer is only " << ShapeBufferSize;
    }

    if (!shape_size.empty()) {
      PixelView draw_window(
          view.buffer(),
          (base::At(base::At(WindowList, static_cast<int>(window)), kWindowX) *
           8) +
              view.x_pos(),
          base::At(base::At(WindowList, static_cast<int>(window)), kWindowY) +
              view.y_pos(),
          base::At(base::At(WindowList, static_cast<int>(window)),
                   kWindowWidth) *
              8,
          base::At(base::At(WindowList, static_cast<int>(window)),
                   kWindowHeight));

      const auto shape_pointer = std::as_writable_bytes(shape_size);

      /*
      **	Special shadow drawing code (used for aircraft and bullets).
      */
      if ((flags & (SHAPE_FADING | SHAPE_PREDATOR)) ==
          (SHAPE_FADING | SHAPE_PREDATOR)) {
        flags = flags & ~(SHAPE_FADING | SHAPE_PREDATOR);
        flags = flags | SHAPE_GHOST;
        ghostdata = MouseClass::SpecialGhost;
      }

      int predoffset = static_cast<int>(CurrentFrame());

      if (x > base::At(base::At(WindowList, static_cast<int>(window)),
                       kWindowWidth) *
                  4) {
        predoffset = -predoffset;
      }

      if (draw_window.Lock()) {
        const ShapeEffects effects{
            .ghost_table = ghostdata,
            .fading_table = fadingdata,
            .fading_count = 1,
            .predator_offset = predoffset,
        };
        Buffer_Frame_To_Page(x, y, Get_Build_Frame_Width(shapefile),
                             Get_Build_Frame_Height(shapefile), shape_pointer,
                             draw_window, flags | SHAPE_TRANS, effects);
      }
      draw_window.Unlock();
    }
  }
}

/***********************************************************************************************
 * Fetch_Techno_Type -- Convert type and ID into TechnoTypeClass pointer. *
 *                                                                                             *
 *    This routine will convert the supplied RTTI type number and the ID value
 *into a valid    * TechnoTypeClass pointer. If there is an error in conversion,
 *then NULL is returned.      *
 *                                                                                             *
 * INPUT:   type  -- RTTI type of the techno class object. *
 *                                                                                             *
 *          id    -- Integer representation of the techno sub type number. *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the techno type class object specified or
 *NULL if the    * conversion could not occur. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/08/1995 JLB : Created. *
 *=============================================================================================*/
const TechnoTypeClass* Fetch_Techno_Type(RTTIType type, int id) {
  switch (type) {
    case RTTI_UNITTYPE:
    case RTTI_UNIT:
      return &UnitTypeClass::As_Reference(static_cast<UnitType>(id));

    case RTTI_BUILDINGTYPE:
    case RTTI_BUILDING:
      return &BuildingTypeClass::As_Reference(static_cast<StructType>(id));

    case RTTI_INFANTRYTYPE:
    case RTTI_INFANTRY:
      return &InfantryTypeClass::As_Reference(static_cast<InfantryType>(id));

    case RTTI_AIRCRAFTTYPE:
    case RTTI_AIRCRAFT:
      return &AircraftTypeClass::As_Reference(static_cast<AircraftType>(id));
    case RTTIType::RTTI_NONE:
    case RTTIType::RTTI_TERRAIN:
    case RTTIType::RTTI_ABSTRACTTYPE:
    case RTTIType::RTTI_ANIM:
    case RTTIType::RTTI_ANIMTYPE:
    case RTTIType::RTTI_BULLET:
    case RTTIType::RTTI_BULLETTYPE:
    case RTTIType::RTTI_OVERLAY:
    case RTTIType::RTTI_OVERLAYTYPE:
    case RTTIType::RTTI_SMUDGE:
    case RTTIType::RTTI_SMUDGETYPE:
    case RTTIType::RTTI_TEAM:
    case RTTIType::RTTI_TEMPLATE:
    case RTTIType::RTTI_TEMPLATETYPE:
    case RTTIType::RTTI_TERRAINTYPE:
    case RTTIType::RTTI_OBJECT:
    case RTTIType::RTTI_SPECIAL:
    default:
      break;
  }
  return nullptr;
}

/***************************************************************************
 * Trap_Object -- gets a ptr to object of given type & coord               *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/02/1995 BRR : Created.                                             *
 *=========================================================================*/
void Trap_Object() {
  trap_object.Ptr.All = nullptr;

  switch (TheDebugState().trap_object_type()) {
    case RTTI_AIRCRAFT:
      for (int i = 0; i < TheObjectHeaps().aircraft().Count(); i++) {
        if (TheObjectHeaps().aircraft().Ptr(i)->Coord ==
                TheDebugState().trap_coord() ||
            TheObjectHeaps().aircraft().Ptr(i) == TheDebugState().trap_this()) {
          trap_object.Ptr.Aircraft = TheObjectHeaps().aircraft().Ptr(i);
          break;
        }
      }
      break;

    case RTTI_ANIM:
      for (int i = 0; i < TheObjectHeaps().anim().Count(); i++) {
        if (TheObjectHeaps().anim().Ptr(i)->Coord ==
                TheDebugState().trap_coord() ||
            TheObjectHeaps().anim().Ptr(i) == TheDebugState().trap_this()) {
          trap_object.Ptr.Anim = TheObjectHeaps().anim().Ptr(i);
          break;
        }
      }
      break;

    case RTTI_BUILDING:
      for (int i = 0; i < TheObjectHeaps().building().Count(); i++) {
        if (TheObjectHeaps().building().Ptr(i)->Coord ==
                TheDebugState().trap_coord() ||
            TheObjectHeaps().building().Ptr(i) == TheDebugState().trap_this()) {
          trap_object.Ptr.Building = TheObjectHeaps().building().Ptr(i);
          break;
        }
      }
      break;

    case RTTI_BULLET:
      for (int i = 0; i < TheObjectHeaps().bullet().Count(); i++) {
        if (TheObjectHeaps().bullet().Ptr(i)->Coord ==
                TheDebugState().trap_coord() ||
            TheObjectHeaps().bullet().Ptr(i) == TheDebugState().trap_this()) {
          trap_object.Ptr.Bullet = TheObjectHeaps().bullet().Ptr(i);
          break;
        }
      }
      break;

    case RTTI_INFANTRY:
      for (int i = 0; i < TheObjectHeaps().infantry().Count(); i++) {
        if (TheObjectHeaps().infantry().Ptr(i)->Coord ==
                TheDebugState().trap_coord() ||
            TheObjectHeaps().infantry().Ptr(i) == TheDebugState().trap_this()) {
          trap_object.Ptr.Infantry = TheObjectHeaps().infantry().Ptr(i);
          break;
        }
      }
      break;

    case RTTI_UNIT:
      for (int i = 0; i < TheObjectHeaps().unit().Count(); i++) {
        if (TheObjectHeaps().unit().Ptr(i)->Coord ==
                TheDebugState().trap_coord() ||
            TheObjectHeaps().unit().Ptr(i) == TheDebugState().trap_this()) {
          trap_object.Ptr.Unit = TheObjectHeaps().unit().Ptr(i);
          break;
        }
      }
      break;

    /*
    ** Last-ditch find-the-object-right-now-darnit loop
    */
    case RTTI_NONE:
      for (int i = 0; i < TheObjectHeaps().aircraft().Count(); i++) {
        if (TheObjectHeaps().aircraft().Raw_Ptr(i)->Coord ==
                TheDebugState().trap_coord() ||
            TheObjectHeaps().aircraft().Raw_Ptr(i) ==
                TheDebugState().trap_this()) {
          trap_object.Ptr.Aircraft = TheObjectHeaps().aircraft().Raw_Ptr(i);
          TheDebugState().trap_object_type() = RTTI_AIRCRAFT;
          return;
        }
      }
      for (int i = 0; i < TheObjectHeaps().anim().Count(); i++) {
        if (TheObjectHeaps().anim().Raw_Ptr(i)->Coord ==
                TheDebugState().trap_coord() ||
            TheObjectHeaps().anim().Raw_Ptr(i) == TheDebugState().trap_this()) {
          trap_object.Ptr.Anim = TheObjectHeaps().anim().Raw_Ptr(i);
          TheDebugState().trap_object_type() = RTTI_ANIM;
          return;
        }
      }
      for (int i = 0; i < TheObjectHeaps().building().Count(); i++) {
        if (TheObjectHeaps().building().Raw_Ptr(i)->Coord ==
                TheDebugState().trap_coord() ||
            TheObjectHeaps().building().Raw_Ptr(i) ==
                TheDebugState().trap_this()) {
          trap_object.Ptr.Building = TheObjectHeaps().building().Raw_Ptr(i);
          TheDebugState().trap_object_type() = RTTI_BUILDING;
          return;
        }
      }
      for (int i = 0; i < TheObjectHeaps().bullet().Count(); i++) {
        if (TheObjectHeaps().bullet().Raw_Ptr(i)->Coord ==
                TheDebugState().trap_coord() ||
            TheObjectHeaps().bullet().Raw_Ptr(i) ==
                TheDebugState().trap_this()) {
          trap_object.Ptr.Bullet = TheObjectHeaps().bullet().Raw_Ptr(i);
          TheDebugState().trap_object_type() = RTTI_BULLET;
          return;
        }
      }
      for (int i = 0; i < TheObjectHeaps().infantry().Count(); i++) {
        if (TheObjectHeaps().infantry().Raw_Ptr(i)->Coord ==
                TheDebugState().trap_coord() ||
            TheObjectHeaps().infantry().Raw_Ptr(i) ==
                TheDebugState().trap_this()) {
          trap_object.Ptr.Infantry = TheObjectHeaps().infantry().Raw_Ptr(i);
          TheDebugState().trap_object_type() = RTTI_INFANTRY;
          return;
        }
      }
      for (int i = 0; i < TheObjectHeaps().unit().Count(); i++) {
        if (TheObjectHeaps().unit().Raw_Ptr(i)->Coord ==
                TheDebugState().trap_coord() ||
            TheObjectHeaps().unit().Raw_Ptr(i) == TheDebugState().trap_this()) {
          trap_object.Ptr.Unit = TheObjectHeaps().unit().Raw_Ptr(i);
          TheDebugState().trap_object_type() = RTTI_UNIT;
          return;
        }
      }
      [[fallthrough]];

    case RTTIType::RTTI_INFANTRYTYPE:
    case RTTIType::RTTI_UNITTYPE:
    case RTTIType::RTTI_AIRCRAFTTYPE:
    case RTTIType::RTTI_BUILDINGTYPE:
    case RTTIType::RTTI_TERRAIN:
    case RTTIType::RTTI_ABSTRACTTYPE:
    case RTTIType::RTTI_ANIMTYPE:
    case RTTIType::RTTI_BULLETTYPE:
    case RTTIType::RTTI_OVERLAY:
    case RTTIType::RTTI_OVERLAYTYPE:
    case RTTIType::RTTI_SMUDGE:
    case RTTIType::RTTI_SMUDGETYPE:
    case RTTIType::RTTI_TEAM:
    case RTTIType::RTTI_TEMPLATE:
    case RTTIType::RTTI_TEMPLATETYPE:
    case RTTIType::RTTI_TERRAINTYPE:
    case RTTIType::RTTI_OBJECT:
    case RTTIType::RTTI_SPECIAL:
    default:
      break;
  }
}

/***********************************************************************************************
 * VQ_Call_Back -- Maintenance callback used for VQ movies. *
 *                                                                                             *
 *    This routine is called every frame of the VQ movie as it is being played.
 *If this        * routine returns non-zero, then the movie will stop. *
 *                                                                                             *
 * INPUT:   buffer   -- Pointer to the image buffer for the current frame. *
 *                                                                                             *
 *          frame    -- The frame number about to be displayed. *
 *                                                                                             *
 * OUTPUT:  Should the movie be stopped? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/24/1995 JLB : Created. *
 *=============================================================================================*/

int32_t VQ_Call_Back(unsigned char* /*unused*/, int32_t /*unused*/) {
  int key = 0;
  if (Keyboard::Check()) {
    key = Keyboard::Get();
    Keyboard::Clear();
  }

  Check_VQ_Palette_Set();

  Interpolate_2X_Scale(&TheScreen().sys_mem_page(), &TheScreen().visible_view(),
                       nullptr);

  // Call_Back();
  if ((TheGameState().breakout_allowed() || TheDebugState().developer_mode()) &&
      key == KN_ESC) {
    Keyboard::Clear();
    movie_broken_out = true;
    return 1;
  }

  if (!TheGameState().in_focus()) {
    VQA_PauseAudio();
    while (!TheGameState().in_focus()) {
      Keyboard::Check();
      Check_For_Focus_Loss();
    }
  }

  TheDisplay().EndFrame();

  return 0;
}

int32_t VQ_Event_Handler(uint32_t event, void* /*buffer*/, int32_t /*nbytes*/) {
  // vsync while waiting for frame
  if (event == VQAEVENT_SYNC) {
    TheDisplay().EndFrame();
  }
  return 0;
}

/***********************************************************************************************
 * Handle_Team -- Processes team selection command. *
 *                                                                                             *
 *    This routine will handle creation and selection of pseudo teams that the
 *player can      * create or control. A team in this sense is an arbitrary
 *grouping of units such that      * rapid selection control is allowed. *
 *                                                                                             *
 * INPUT:   team  -- The logical team number to process. *
 *                                                                                             *
 *          action-- The action to perform on this team: * 0 - Toggle the select
 *state for all members of this team.                 * 1 - Select the members
 *of this team.                                      * 2 - Make all selected
 *objects members of this team.                       *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/27/1995 JLB : Created. *
 *=============================================================================================*/
void Handle_Team(int team, int action) {
  TheGameState().allow_voice() = true;
  switch (action) {
    /*
    **	Toggle the team selection. If the team is selected, then merely unselect
    *it. If the *	team is not selected, then unselect all others before
    *selecting this team.
    */
    case 3:
    case 0:

      /*
      **	If a non team member is currently selected, then deselect all
      *objects *	before selecting this team.
      */
      if (TheWorld().current_object().Count()) {
        switch (TheWorld().current_object().at(0)->What_Am_I()) {
          case RTTI_UNIT:
          case RTTI_INFANTRY:
          case RTTI_AIRCRAFT:
            if (std::cmp_not_equal(
                    dynamic_cast<FootClass*>(TheWorld().current_object().at(0))
                        ->Group,
                    team)) {
              Unselect_All();
            }
            break;
          case RTTIType::RTTI_NONE:
          case RTTIType::RTTI_INFANTRYTYPE:
          case RTTIType::RTTI_UNITTYPE:
          case RTTIType::RTTI_AIRCRAFTTYPE:
          case RTTIType::RTTI_BUILDING:
          case RTTIType::RTTI_BUILDINGTYPE:
          case RTTIType::RTTI_TERRAIN:
          case RTTIType::RTTI_ABSTRACTTYPE:
          case RTTIType::RTTI_ANIM:
          case RTTIType::RTTI_ANIMTYPE:
          case RTTIType::RTTI_BULLET:
          case RTTIType::RTTI_BULLETTYPE:
          case RTTIType::RTTI_OVERLAY:
          case RTTIType::RTTI_OVERLAYTYPE:
          case RTTIType::RTTI_SMUDGE:
          case RTTIType::RTTI_SMUDGETYPE:
          case RTTIType::RTTI_TEAM:
          case RTTIType::RTTI_TEMPLATE:
          case RTTIType::RTTI_TEMPLATETYPE:
          case RTTIType::RTTI_TERRAINTYPE:
          case RTTIType::RTTI_OBJECT:
          case RTTIType::RTTI_SPECIAL:
          default:
            break;
        }
      }
      for (int index = 0; index < TheObjectHeaps().unit().Count(); index++) {
        UnitClass* obj = TheObjectHeaps().unit().Ptr(index);
        if ((obj && !obj->IsInLimbo && std::cmp_equal(obj->Group, team) &&
             obj->House == ThePlayer()) &&
            (!obj->IsSelected)) {
          obj->Select();
          TheGameState().allow_voice() = false;
        }
      }
      for (int index = 0; index < TheObjectHeaps().infantry().Count();
           index++) {
        InfantryClass* obj = TheObjectHeaps().infantry().Ptr(index);
        if ((obj && !obj->IsInLimbo && std::cmp_equal(obj->Group, team) &&
             obj->House == ThePlayer()) &&
            (!obj->IsSelected)) {
          obj->Select();
          TheGameState().allow_voice() = false;
        }
      }
      for (int index = 0; index < TheObjectHeaps().aircraft().Count();
           index++) {
        AircraftClass* obj = TheObjectHeaps().aircraft().Ptr(index);
        if ((obj && !obj->IsInLimbo && std::cmp_equal(obj->Group, team) &&
             obj->House == ThePlayer()) &&
            (!obj->IsSelected)) {
          obj->Select();
          TheGameState().allow_voice() = false;
        }
      }

      /*
      **	Center the map around the team if the ALT key was pressed too.
      */
      if (action == 3) {
        TheMap().Center_Map();
        TheMap().Flag_To_Redraw(true);
      }
      break;

    /*
    **	Additive selection of team.
    */
    case 1:
      for (int index = 0; index < TheObjectHeaps().unit().Count(); index++) {
        UnitClass* obj = TheObjectHeaps().unit().Ptr(index);
        if ((obj && !obj->IsInLimbo && std::cmp_equal(obj->Group, team) &&
             obj->House == ThePlayer()) &&
            (!obj->IsSelected)) {
          obj->Select();
          TheGameState().allow_voice() = false;
        }
      }
      for (int index = 0; index < TheObjectHeaps().infantry().Count();
           index++) {
        InfantryClass* obj = TheObjectHeaps().infantry().Ptr(index);
        if ((obj && !obj->IsInLimbo && std::cmp_equal(obj->Group, team) &&
             obj->House == ThePlayer()) &&
            (!obj->IsSelected)) {
          obj->Select();
          TheGameState().allow_voice() = false;
        }
      }
      for (int index = 0; index < TheObjectHeaps().aircraft().Count();
           index++) {
        AircraftClass* obj = TheObjectHeaps().aircraft().Ptr(index);
        if ((obj && !obj->IsInLimbo && std::cmp_equal(obj->Group, team) &&
             obj->House == ThePlayer()) &&
            (!obj->IsSelected)) {
          obj->Select();
          TheGameState().allow_voice() = false;
        }
      }
      break;

    /*
    **	Create the team.
    */
    case 2:
      for (int index = 0; index < TheObjectHeaps().unit().Count(); index++) {
        UnitClass* obj = TheObjectHeaps().unit().Ptr(index);
        if (obj && !obj->IsInLimbo && obj->House == ThePlayer()) {
          if (std::cmp_equal(obj->Group, team)) {
            obj->Group = 0xFF;  // No team.
          }
          if (obj->IsSelected) {
            obj->Group = static_cast<unsigned char>(team);
          }
        }
      }
      for (int index = 0; index < TheObjectHeaps().infantry().Count();
           index++) {
        InfantryClass* obj = TheObjectHeaps().infantry().Ptr(index);
        if (obj && !obj->IsInLimbo && obj->House == ThePlayer()) {
          if (std::cmp_equal(obj->Group, team)) {
            obj->Group = 0xFF;  // No team.
          }
          if (obj->IsSelected) {
            obj->Group = static_cast<unsigned char>(team);
          }
        }
      }
      for (int index = 0; index < TheObjectHeaps().aircraft().Count();
           index++) {
        AircraftClass* obj = TheObjectHeaps().aircraft().Ptr(index);
        if (obj && !obj->IsInLimbo && obj->House == ThePlayer()) {
          if (std::cmp_equal(obj->Group, team)) {
            obj->Group = 0xFF;  // No team.
          }
          if (obj->IsSelected) {
            obj->Group = static_cast<unsigned char>(team);
          }
        }
      }
      break;
    default:
      break;
  }
  TheGameState().allow_voice() = true;
}

/***********************************************************************************************
 * Handle_View -- Either records or restores the tactical view. *
 *                                                                                             *
 *    This routine is used to record or restore the current map tactical view. *
 *                                                                                             *
 * INPUT:   view  -- The view number to work with. *
 *                                                                                             *
 *          action-- The action to perform with this view number. * 0  = Restore
 *the view to this previously remembered location.            * 1  =  Record the
 *current view location.                                   *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/04/1995 JLB : Created. *
 *=============================================================================================*/
void Handle_View(int view, int action) {
  if (static_cast<unsigned>(view) <
      sizeof(TheWorld().views()) / sizeof(base::At(TheWorld().views(), 0))) {
    if (action == 0) {
      TheMap().Set_Tactical_Position(
          Cell_Coord(base::At(TheWorld().views(), view)) & 0xFF00FF00L);
      TheMap().Flag_To_Redraw(true);
    } else {
      base::At(TheWorld().views(), view) = Coord_Cell(TheMap().TacticalCoord);
    }
  }
}

void Heap_Dump_Check(const char* string) {
  if constexpr (config::kCheatKeysEnabled) {
    if (!check_heap) {  // check the heap?
      return;
    }

    //	TheDebugState().set_heap_dump(true);

    Smart_Printf("%s\n", string);

    //	TheDebugState().set_heap_dump(false);
  }
}

// #ifndef ROR_NOT_READY
// #define ROR_NOT_READY 21
// #endif

/***********************************************************************************************
 * Get_CD_Index -- returns the volume type of the CD in the given drive *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    drive number * timeout *
 *                                                                                             *
 * OUTPUT:   0 = gdi * 1 = nod * 2 = covert * -1 = non C&C *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 5/21/96 5:27PM ST : Created *
 *=============================================================================================*/
int Get_CD_Index(int /*cd_drive*/, int /*timeout*/) {
  return -1;  // this may be a problem
}

/***********************************************************************************************
 * Force_CD_Available -- Ensures that specified CD is available. *
 *                                                                                             *
 *    Call this routine when you need to ensure that the specified CD is
 *actually in the       * CD-ROM drive. *
 *                                                                                             *
 * INPUT:   cd    -- The CD that must be available. This will either be "0" for
 *the GDI CD, or * "1" for the Nod CD. If either CD will qualify, then pass in
 *"-1".         *
 *                                                                                             *
 * OUTPUT:  Is the CD inserted and available? If false is returned, then this
 *indicates that   * the player pressed <CANCEL>. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/11/1995 JLB : Created. * 05/22/1996  ST : Handles multiple CD
 *drives / CD changers                                 *
 *=============================================================================================*/
bool Force_CD_Available(int cd) {
#ifndef DEMO
  static int _last = -1;
#endif
  static unsigned char _palette[768];
  static unsigned char
      _hold[16];  // Saved copy of the font palette (g_font_palette).
  static std::span<const std::byte> font;
  static const char* _volid[] = {"GDI", "NOD", "COVERT"};

  int new_cd_drive = 0;
  char buffer[128];

  ThemeType theme_playing = THEME_NONE;

  /*
  ** If the required CD is set to -2 then it means that the file is present
  ** on the local hard drive and we shouldn't have to worry about it.
  */
  if (cd == -2) {
    return true;
  }

  /*
  ** Find out if the CD in the current drive is the one we are looking for
  */
  const int current_drive = SearchPaths::current_cd_drive();
  int cd_index = Get_CD_Index(current_drive, 1 * 60);
  if ((cd_index >= 0) && (cd == cd_index || cd == -1)) {
    /*
    ** The required CD is still in the CD drive we used last time
    */
    new_cd_drive = current_drive;
  }

  /*
  ** Flag that we will have to restart the theme
  */
  theme_playing = TheTheme().What_Is_Playing();
  TheTheme().Stop();

  if (!new_cd_drive) {
    /*
    ** Check the last CD drive we used if its different from the current one
    */
    const int last_drive = SearchPaths::last_cd_drive();
    /*
    ** Make sure the last drive is valid and it isnt the current drive
    */
    if (last_drive && last_drive != SearchPaths::current_cd_drive()) {
      /*
      ** Find out if there is a C&C cd in the last drive and if so is it the one
      *we are looking for
      ** Give it a nice big timeout so the CD changer has time to swap the discs
      */
      cd_index = Get_CD_Index(last_drive, 10 * 60);
      if ((cd_index >= 0) && (cd == cd_index || cd == -1)) {
        /*
        ** The required CD is in the CD drive we used last time
        */
        new_cd_drive = last_drive;
      }
    }
  }

  /*
  ** Lordy. No sign of that blimming CD anywhere. Search all the CD drives
  ** then if we still cant find it prompt the user to insert it.
  */
  if (!new_cd_drive) {
    // The original walked every CD drive here looking for the disc. There is
    // no drive to walk, so all that is left is to ask for the disc in the one
    // the search path already points at, over and over until the player
    // cancels.
    for (;;) {
      /*
      **	Prompt to insert the CD into the drive.
      */
      if (cd == -1) {
        absl::SNPrintF(buffer, sizeof(buffer), "%s",
                       Text_String(TXT_CD_DIALOG_1));
      } else {
        if (cd == 2) {
          absl::SNPrintF(buffer, sizeof(buffer), "%s",
                         Text_String(TXT_CD_DIALOG_3));
        } else {
          // 0 or 1?
          Format_Runtime_Text(buffer, sizeof(buffer),
                              Text_String(TXT_CD_DIALOG_2), cd + 1,
                              base::At(_volid, cd));
        }
      }
      // The theme was already stopped above, and the only way out of this
      // loop is the cancel below, so there is nothing to remember here.
      TheTheme().Stop();
      int hidden = Get_Mouse_State();
      font = g_font;
      std::ranges::copy(CurrentPalette, std::begin(_palette));
      std::ranges::copy(g_font_palette, std::begin(_hold));

      /*
      **	Only set the palette if necessary.
      */
      Set_Palette(ThePalettes().game_palette());

      /*
      ** Pretend we are in the game, even if we arent
      */
      const bool old_in_main_loop = TheGameState().in_main_loop();
      TheGameState().in_main_loop() = true;

      Keyboard::Clear();

      while (Get_Mouse_State()) {
        Show_Mouse();
      }

      if (CCMessageBox().Process(buffer, TXT_OK, TXT_CANCEL, TXT_NONE, true) ==
          1) {
        Hide_Mouse();
        TheGameState().in_main_loop() = old_in_main_loop;
        return false;
      }
      while (hidden--) {
        Hide_Mouse();
      }
      Set_Palette(_palette);
      SetFont(font);
      SetFontPalette(_hold);
      TheGameState().in_main_loop() = old_in_main_loop;
    }
  }

#ifndef DEMO

  SearchPaths::SetCdDrive(new_cd_drive);
  SearchPaths::Refresh();

  /*
  **	If it broke out of the query for CD-ROM loop, then this means that the
  **	CD-ROM has been inserted.
  */
  if (cd > -1 && _last != cd) {
    _last = cd;

    TheTheme().Stop();

    Assets::DiscArchives& archives = TheAssets().disc_archives();
    delete archives.movies;
    delete archives.general;
    delete archives.score;

    archives.movies = MixArchive::Register("MOVIES.MIX");
    archives.general = MixArchive::Register("GENERAL.MIX");
    archives.score = MixArchive::Register("SCORES.MIX");
    ThemeClass::Scan();
  }
#endif

  if (theme_playing != THEME_NONE) {
    TheTheme().Queue_Song(theme_playing);
  }

  return true;
}

/***********************************************************************************************
 * Validate_Error -- prints an error message when an object fails validation *
 *                                                                                             *
 * INPUT: * name		name of object type that failed
 **
 *                                                                                             *
 * OUTPUT: * none.
 **
 *                                                                                             *
 * WARNINGS: * none.
 **
 *                                                                                             *
 * HISTORY: * 08/15/1995 BRR : Created. *
 *=============================================================================================*/
void Validate_Error(const char* name) {
  // Callers only validate in cheat-key builds, where a bad object pointer is a
  // programmer error. Abort rather than exit(0), so the failure is visible to
  // shells and test runners instead of looking like a clean shutdown.
  ShutDown();
  LOG(FATAL) << name << " object error!";
}

/***********************************************************************************************
 * Do_Record_Playback -- handles saving/loading map pos & current object *
 *                                                                                             *
 * INPUT: * none.
 **
 *                                                                                             *
 * OUTPUT: * none.
 **
 *                                                                                             *
 * WARNINGS: * none.
 **
 *                                                                                              *
 * HISTORY: * 08/15/1995 BRR : Created. *
 *=============================================================================================*/
static void Do_Record_Playback() {
  int count = 0;
  TARGET tgt = 0;
  COORDINATE coord = 0;
  uint32_t sum = 0;
  uint32_t sum2 = 0;
  uint32_t ltgt = 0;

  // Null only outside a recorded or played-back game.
  ByteStream* const record = TheSession().record_stream().get();

  /*------------------------------------------------------------------------
  Record a game
  ------------------------------------------------------------------------*/
  if (TheSession().record_game() && record != nullptr) {
    /*.....................................................................
    Save the map's location
    .....................................................................*/
    record->WriteObject(TheMap().DesiredTacticalCoord);

    /*.....................................................................
    Save the current object list count
    .....................................................................*/
    count = static_cast<int>(TheWorld().current_object().Count());
    record->WriteObject(count);

    /*.....................................................................
    Save a CRC of the selected-object list.
    .....................................................................*/
    sum = 0;
    for (int i = 0; i < count; i++) {
      ltgt =
          static_cast<uint32_t>(TheWorld().current_object().at(i)->As_Target());
      sum += ltgt;
    }
    record->WriteObject(sum);

    /*.....................................................................
    Save all selected objects.
    .....................................................................*/
    for (int i = 0; i < count; i++) {
      tgt = TheWorld().current_object().at(i)->As_Target();
      record->WriteObject(tgt);
    }

    /*.....................................................................
    For 'SuperRecord', push the frame to disk now, so a crash keeps it.
    .....................................................................*/
    if (TheSession().super_record() && !record->Flush()) {
      DLOG(WARNING) << "Do_Record_Playback: Flush failed, disk may be full";
    }
  }

  /*------------------------------------------------------------------------
  Play back a game ("attract" mode)
  ------------------------------------------------------------------------*/
  if (TheSession().playback_game() && record != nullptr) {
    /*.....................................................................
    Read & set the map's location.
    .....................................................................*/
    if (record->ReadObject(coord) && coord != TheMap().DesiredTacticalCoord) {
      TheMap().Set_Tactical_Position(coord);
    }

    if (record->ReadObject(count)) {
      /*..................................................................
      Compute a CRC of the current object-selection list.
      ..................................................................*/
      sum = 0;
      for (int i = 0; i < TheWorld().current_object().Count(); i++) {
        ltgt = static_cast<uint32_t>(
            TheWorld().current_object().at(i)->As_Target());
        sum += ltgt;
      }

      /*..................................................................
      Load the CRC of the objects on disk; if it doesn't match, select
      all objects as they're loaded.
      ..................................................................*/
      record->ReadObject(sum2);
      if (sum2 != sum) {
        Unselect_All();
      }

      TheGameState().allow_voice() = true;

      for (int i = 0; i < count; i++) {
        if (record->ReadObject(tgt)) {
          ObjectClass* obj = As_Object(tgt);
          if (obj && sum2 != sum) {
            obj->Select();
            TheGameState().allow_voice() = false;
          }
        }
      }

      TheGameState().allow_voice() = true;
    }

    /*.....................................................................
    The map isn't drawn in playback mode, so draw it here.
    .....................................................................*/
    TheMap().Render();
  }
}
/***************************************************************************
 * HIRES_RETRIEVE -- retrieves a resolution dependant file
 **
 *                                                                         *
 * INPUT:		char * file name of the file to retrieve
 **
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/25/1996     : Created.                                             *
 *=========================================================================*/
std::span<const std::byte> Hires_Retrieve(const char* name) {
  char filename[30];

  if (Screen::kWidth != 320) {
    absl::SNPrintF(filename, sizeof(filename), "H%s", name);
  } else {
    port::SafeCopy(filename, name);
  }
  return MixArchive::RetrieveData(filename);
}
// The static sidebar buttons call this during static initialization, before
// there is a Screen, so it must not ask one.
int Get_Resolution_Factor() { return Screen::kWidth == 320 ? 0 : 1; }
