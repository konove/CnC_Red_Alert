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

/* $Header: /CounterStrike/INIT.CPP 8     3/14/97 5:15p Joe_b $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : INIT.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : January 20, 1992 *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Anim_Init -- Initialize the VQ animation control structure. *
 *   Bootstrap -- Perform the initial bootstrap procedure. * Calculate_CRC --
 *Calculates a one-way hash from a data block.                             *
 *   Init_Authorization -- Verifies that the player is authorized to play the
 *game.            * Init_Bootstrap_Mixfiles -- Registers and caches any
 *mixfiles needed for bootstrapping.    * Init_Bulk_Data -- Initialize the
 *time-consuming mixfile caching.                          * Init_CDROM_Access
 *-- Initialize the CD-ROM access handler.                                *
 *   Init_Color_Remaps -- Initialize the text remap tables. *
 *   Init_Expansion_Files -- Fetch any override expansion mixfiles. * Init_Fonts
 *-- Initialize all the game font pointers. * Init_Game -- Main game
 *initialization routine.                                            *
 *   Init_Heaps -- Initialize the game heaps and buffers. * Init_Keys --
 *Initialize the cryptographic keys.                                           *
 *   Init_Mouse -- Initialize the mouse system. * Init_One_Time_Systems --
 *Initialize internal pointers to the bulk data.                   * Init_Random
 *-- Initializes the random-number generator * Init_Secondary_Mixfiles --
 *Register and cache secondary mixfiles.                         *
 *   Load_Recording_Values -- Loads recording values from recording file *
 *   Load_Title_Page -- Load the background art for the title page. *
 *   Parse_Command_Line -- Parses the command line parameters. * Parse_INI_File
 *-- Parses CONQUER.INI for special options                                  *
 *   Play_Intro -- plays the introduction & logo movies * Save_Recording_Values
 *-- Saves recording values to a recording file                       *
 *   Select_Game -- The game's main menu * Load_Prolog_Page -- Loads the special
 *pre-prolog "please wait" page.                      *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */
#include "ra/init.h"

#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/types.h"
#include "magic_enum/magic_enum.hpp"
#include "port/platform.h"
#include "port/random_seed.h"
#include "ra/assets.h"
#include "ra/ccini.h"
#include "ra/compat.h"
#include "ra/config.h"
#include "ra/conquer.h"
#include "ra/const.h"
#include "ra/debug_state.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/gadget.h"
#include "ra/game_clock.h"
#include "ra/game_state.h"
#include "ra/goptions.h"
#include "ra/graphics_loader.h"
#include "ra/heap.h"
#include "ra/house.h"
#include "ra/ini.h"
#include "ra/inline.h"
#include "ra/input.h"
#include "ra/installation.h"
#include "ra/intro.h"
#include "ra/ipx.h"
#include "ra/ipxaddr.h"
#include "ra/jshell.h"
#include "ra/language.h"
#include "ra/loaddlg.h"
#include "ra/logic.h"
#include "ra/mapedit.h"
#include "ra/menus.h"
#include "ra/mission_id.h"
#include "ra/movie.h"
#include "ra/mplayer.h"
#include "ra/msgbox.h"
#include "ra/msglist.h"
#include "ra/netdlg.h"
#include "ra/network.h"
#include "ra/nulldlg.h"
#include "ra/nullmgr.h"
#include "ra/object_heaps.h"
#include "ra/palette.h"
#include "ra/palettes.h"
#include "ra/queue.h"
#include "ra/rules.h"
#include "ra/saveload.h"
#include "ra/scenario.h"
#include "ra/screen.h"
#include "ra/session.h"
#include "ra/special.h"
#include "ra/startup.h"
#include "ra/startup_options.h"
#include "ra/text_ids.h"
#include "ra/theme.h"
#include "ra/type.h"
#include "ra/type_heaps.h"
#include "ra/winstub.h"
#include "ra/world.h"
#include "ra/wsproto.h"
#include "ra/wspudp.h"
#include "sdllib/file.h"
#include "sdllib/file_access.h"
#include "sdllib/iff.h"
#include "sdllib/misc.h"
#include "sdllib/pixel_buffer.h"
#include "sdllib/shape.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "tech/archive.h"
#include "tech/audio_mixer.h"
#include "tech/file_sink.h"
#include "tech/file_source.h"
#include "tech/fixed.h"
#include "tech/ftimer.h"
#include "tech/game_file.h"
#include "tech/key_phrase_hash.h"
#include "tech/memory_file.h"
#include "tech/mix_archive.h"
#include "tech/number_parse.h"
#include "tech/random.h"
#include "tech/rgb.h"
#include "tech/search_paths.h"
#include "winvq/vqa32/vqaplay.h"

static RemapControlType SidebarScheme;

/****************************************
**	Function prototypes for this module **
*****************************************/
static void Play_Intro(bool sequenced = false);
static bool LogoAlreadyPlayed();
static void MarkLogoPlayed();
static void Init_Color_Remaps();
static void Init_Heaps();
static void Init_Expansion_Files();
static void Init_One_Time_Systems();
static void Init_Fonts();
static void Init_CDROM_Access();
static void Init_Bootstrap_Mixfiles();
static void Init_Secondary_Mixfiles();
static void Init_Mouse();
static void Bootstrap();
// static void Init_Authorization();
static void Init_Bulk_Data();
static void Init_Keys();

static void Init_Random();

#define ATTRACT_MODE_TIMEOUT 3600  // timeout for attract mode

static bool Load_Recording_Values(GameFile& file);
static bool Save_Recording_Values(GameFile& file);

#include "ra/config.h"
#include "ra/expand.h"
#include "ra/wol_main.h"

static std::vector<uint8_t> shape_storage;

/***********************************************************************************************
 * Load_Prolog_Page -- Loads the special pre-prolog "please wait" page. *
 *                                                                                             *
 *    This loads and displays the prolog page that is displayed before the
 *prolog movie        * is played. This page is necessary because there is much
 *loading that occurs before       * the prolog movie is played and looking at a
 *picture is better than looking at a blank    * screen. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 11/03/1996 JLB : Created. *
 *=============================================================================================*/
static void Load_Prolog_Page() {
  Hide_Mouse();
  Load_Title_Screen("PROLOG.PCX", &TheScreen().hidden_view(),
                    ThePalettes().title_palette());
  TheScreen().hidden_view().Blit(TheScreen().visible_view());
  ThePalettes().title_palette().Set();
  Show_Mouse();
}

/***********************************************************************************************
 * Init_Game -- Main game initialization routine. *
 *                                                                                             *
 *    Perform all one-time game initializations here. This includes all *
 *    allocations and table setups. The intro and other one-time startup * tasks
 *are also performed here. *
 *                                                                                             *
 * INPUT:   argc,argv   -- Command line arguments. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   Only call this ONCE! *
 *                                                                                             *
 * HISTORY: * 10/07/1992 JLB : Created. *
 *=============================================================================================*/
bool Init_Game() {
  /*
  **	Initialize the encryption keys.
  */
  Init_Keys();

  /*
  **	Bootstrap as much as possible before error-prone initializations are
  **	performed. This bootstrap process will enable the error message
  **	handler to function.
  */
  Bootstrap();

  /*
  **	Check for an initialize a working mouse pointer. Display error and bail
  *if *	no mouse driver is installed.
  */
  Init_Mouse();

  /*
  **	Initialize access to the CD-ROM and ensure that the CD is inserted. This
  *can, and *	most likely will, result in a visible prompt.
  */
  Init_CDROM_Access();

  if (TheSpecial().IsFromInstall) {
    Load_Prolog_Page();
  }

  /*
  **	Register and cache any secondary mixfiles.
  */
  Init_Secondary_Mixfiles();

  /*
  **	This is a special hack to initialize the heaps that must be in place
  *before the *	rules file is processed. These heaps should properly be
  *allocated as a consequence *	of processing the rules.ini file, but that is a
  *bit beyond the capabilities of *	the rule parser routine (currently).
  */
  TheTypeHeaps().house().Set_Heap(magic_enum::enum_count<HousesType>());
  TheTypeHeaps().building().Set_Heap(magic_enum::enum_count<StructType>());
  TheTypeHeaps().aircraft().Set_Heap(magic_enum::enum_count<AircraftType>());
  TheTypeHeaps().infantry().Set_Heap(magic_enum::enum_count<InfantryType>());
  TheTypeHeaps().bullet().Set_Heap(magic_enum::enum_count<BulletType>());
  TheTypeHeaps().anim().Set_Heap(magic_enum::enum_count<AnimType>());
  TheTypeHeaps().unit().Set_Heap(magic_enum::enum_count<UnitType>());
  TheTypeHeaps().vessel().Set_Heap(magic_enum::enum_count<VesselType>());
  TheTypeHeaps().tmplate().Set_Heap(magic_enum::enum_count<TemplateType>());
  TheTypeHeaps().terrain().Set_Heap(magic_enum::enum_count<TerrainType>());
  TheTypeHeaps().overlay().Set_Heap(magic_enum::enum_count<OverlayType>());
  TheTypeHeaps().smudge().Set_Heap(magic_enum::enum_count<SmudgeType>());

  HouseTypeClass::Init_Heap();
  BuildingTypeClass::Init_Heap();
  AircraftTypeClass::Init_Heap();
  InfantryTypeClass::Init_Heap();
  BulletTypeClass::Init_Heap();
  AnimTypeClass::Init_Heap();
  UnitTypeClass::Init_Heap();
  VesselTypeClass::Init_Heap();
  TemplateTypeClass::Init_Heap();
  TerrainTypeClass::Init_Heap();
  OverlayTypeClass::Init_Heap();
  SmudgeTypeClass::Init_Heap();

  /*
  **	Find and process any rules for this game.
  */
  GameFile fc("RULES.INI");
  if (TheRules().rule_ini().Load(fc, false)) {
    TheRules().Process(TheRules().rule_ini());
  }
  //  Aftermath runtime change 9/29/98
  //	This is safe to do, as only rules for aftermath units are included in
  // this ini.
  if (Is_Aftermath_Installed()) {
    GameFile aftermath_ini("AFTRMATH.INI");
    if (TheRules().aftermath_ini().Load(aftermath_ini, false)) {
      TheRules().Process(TheRules().aftermath_ini());
    }
  }

  TheSession().MaxPlayers = TheRules().MaxPlayers;

  /*
  **	Initialize the game object heaps as well as other rules-dependant buffer
  *allocations.
  */
  Init_Heaps();

  /*
  **	Initialize the animation system.
  */
  Anim_Init();

  /*
  **	Play the startup animation.
  */
  if (!TheSpecial().IsFromInstall) {
    TheScreen().visible_page().Clear();
    if (!LogoAlreadyPlayed()) {
      Play_Intro();
      MarkLogoPlayed();
    }
    base::FillBytes(
        std::as_writable_bytes(PaletteClass::CurrentPalette.bytes()), 0x01,
        768);
    ThePalettes().white_palette().Set();
  } else {
    // The first launch plays the full intro instead, which counts.
    MarkLogoPlayed();
    base::FillBytes(
        std::as_writable_bytes(PaletteClass::CurrentPalette.bytes()), 0x01,
        768);
  }

  /*
  **	Initialize the text remap tables.
  */
  Init_Color_Remaps();

  /*
  **	Get authorization to access the game.
  */
  //	Init_Authorization();
  //	Show_Mouse();

  /*
  **	Set the logic page to the seenpage.
  */
  SetLogicPage(TheScreen().visible_view());

  /*
  **	If not automatically launching into the intro, then display the title
  **	page while the bulk data is cached.
  */
  if (!TheSpecial().IsFromInstall) {
    Load_Title_Page(true);

    Hide_Mouse();
    Fancy_Text_Print(TXT_STAND_BY, 320, 240,
                     &ThePalettes().color_remaps().at(PCOLOR_DIALOG_BLUE),
                     kTBlack, TPF_CENTER | kTpfText | TPF_DROPSHADOW);
    Show_Mouse();

    ThePalettes().title_palette().Set(kFadePaletteSlow);
    ServiceRealTime();
  }

  /*
  **	Initialize the bulk data. This takes the longest time and must be
  *performed once *	before the regular game starts.
  */
  Init_Bulk_Data();

  /*
  **	Initialize the multiplayer score values
  */
  TheSession().GamesPlayed = 0;
  TheSession().NumScores = 0;
  TheSession().CurGame = 0;
  for (auto& i : TheSession().Score) {
    i.Name[0] = '\0';
    i.Wins = 0;
    for (int& kills : i.Kills) {
      kills = -1;  // -1 = this player didn't play this round
    }
  }

  /*
  ** Copy the title screen's palette into the GamePalette & OriginalPalette,
  ** because the options Load routine uses these palettes to set the brightness,
  *etc.
  */
  ThePalettes().game_palette() = ThePalettes().title_palette();
  ThePalettes().original_palette() = ThePalettes().title_palette();

  /*
  **	Read game options, so the GameSpeed is initialized when multiplayer
  ** dialogs are invoked.  (GameSpeed must be synchronized between systems.)
  */
  TheOptions().Load_Settings();

  return true;
}

/***********************************************************************************************
 * Select_Game -- The game's main menu *
 *                                                                                             *
 * INPUT: * fade		if true, will fade the palette in gradually
 **
 *                                                                                             *
 * OUTPUT: * none.
 **
 *                                                                                             *
 * WARNINGS: * none.
 **
 *                                                                                             *
 * HISTORY: * 06/05/1995 BRR : Created. *
 *=============================================================================================*/
// -NEWGAME and -LOADGAME each start one game. Select_Game() sets this when
// it has acted on the request, so coming back from that game shows the menu
// instead of starting it again.
static bool startup_game_started = false;

bool Select_Game(bool /*fade*/) {
  //	Enums in Select_Game() must match order of buttons in Main_Menu().
  constexpr int kSelTimeout = -1;  // main menu timeout--go into attract mode
  constexpr int kSelNewScenarioCs = 0;    // Expansion scenario to play.
  constexpr int kSelNewScenarioAm = 1;    // Expansion scenario to play.
  constexpr int kSelStartNewGame = 2;     // start a new game
  constexpr int kSelLoadMission = 3;      // load a saved game
  constexpr int kSelMultiplayerGame = 4;  // play modem/null-modem/network game
  constexpr int kSelIntro = 5;            // couch-potato mode
  constexpr int kSelExit = 6;             // exit to DOS
  constexpr int kSelFame = 7;             // view the hall o' fame
  constexpr int kSelNone = 8;             // placeholder default value

  bool gameloaded = false;  // Has the game been loaded from the menu?
  int selection = 0;        // the default selection
  bool process = true;      // false = break out of while loop
  bool display = true;

  const StartupOptions& options = TheStartupOptions();

  // A -QUITFRAME run has ended when the game it started (-NEWGAME or
  // -LOADGAME, each started once, or recording playback) brings control back
  // here. Leave rather than wait at the menu for input that never comes.
  const bool startup_game_pending =
      !startup_game_started &&
      (!options.new_game.empty() || options.load_game >= 0);
  if (options.quit_at_frame >= 0 && !startup_game_pending &&
      !TheSession().Play) {
    return false;
  }

  const int cdcheck = 0;

  Show_Mouse();

  TheRules().NewUnitsEnabled = TheRules().SecretUnitsEnabled =
      false;  // Assume new units disabled, unless specifically .INI enabled or
              // multiplayer negotiations enable it.

  /*
  **	[Re]set any globals that need it, in preparation for a new scenario
  */
  TheGameState().active() = true;
  TheNetwork().do_list().Init();
  TheNetwork().out_list().Init();
  TheGameClock().set_frame(0);
  TheScenario().MissionTimer.Set(0);
  TheScenario().MissionTimer.Stop();
  TheScenario().CDifficulty = DIFF_NORMAL;
  TheScenario().Difficulty = DIFF_NORMAL;
  TheGameState().player_wins() = false;
  TheGameState().player_loses() = false;
  TheSession().ObiWan = false;
  TheDebugState().set_unshroud(false);
  TheMap().Set_Cursor_Shape({});
  TheMap().PendingObjectPtr = nullptr;
  TheMap().PendingObject = nullptr;
  TheMap().PendingHouse = HOUSE_NONE;

  TheSession().ProcessTicks = 0;
  TheSession().ProcessFrames = 0;
  TheSession().DesiredFrameRate = 30;
  TheNetwork().new_max_ahead_frame1() = 0;
  TheNetwork().new_max_ahead_frame2() = 0;

  /*
  **	Init multiplayer game scores.  Let Wins accumulate; just init the
  *current
  ** Kills for this game.  Kills of -1 means this player didn't play this round.
  */
  for (int i = 0; i < MAX_MULTI_GAMES; i++) {
    base::At(base::At(TheSession().Score, i).Kills, TheSession().CurGame) = -1;
  }

  /*
  **	Set default mouse shape
  */
  TheMap().Set_Default_Mouse(MOUSE_NORMAL, false);

  /*
  **	If the last game we played was a multiplayer game, jump right to that
  **	menu by pre-setting 'selection'.
  */
  if (TheSession().Type == GAME_NORMAL) {
    selection = kSelNone;
  } else {
    selection = kSelMultiplayerGame;
  }

  /*
  **	Main menu processing; only do this if we're not in editor mode.
  */
  if (!TheDebugState().map_editor_active()) {
    /*
    **	Menu selection processing loop
    */
    TheTheme().Queue_Song(THEME_CRUS);

    /*
    ** If we're playing back a recording, load all pertinent values & skip
    ** the menu loop.  Hide the now-useless mouse pointer.
    */
    if (TheSession().Play && TheSession().RecordFile.IsAvailable()) {
      if (TheSession().RecordFile.Open(FileAccess::kRead)) {
        if (Load_Recording_Values(TheSession().RecordFile)) {
          process = false;
          TheTheme().Fade_Out();
        } else {
          TheSession().RecordFile.Close();
          TheSession().Play = false;
        }
      } else {
        TheSession().Play = false;
      }
    }

    while (process) {
      /*
      **	Redraw the title page if needed
      */
      if (display) {
        Hide_Mouse();

        /*
        **	Display the title page; fade it in if this is the first time
        **	through the loop, and the 'fade' flag is true
        */
        Load_Title_Page();
        ThePalettes().game_palette() = ThePalettes().title_palette();

        TheScreen().hidden_view().Blit(TheScreen().visible_view());
        //				if (fade) {
        //					WhitePalette.Set();
        //					CCPalette.Set(kFadePaletteSlow,
        // ServiceRealTime); 					fade = false;
        // } else {
        ThePalettes().title_palette().Set();
        //				}

        SetLogicPage(TheScreen().visible_view());
        display = false;
        Show_Mouse();
      }

      /*
      **	Display menu and fetch selection from player.
      */
      if (TheSpecial().IsFromInstall) {
        selection = kSelStartNewGame;
      }

      if (config::kWolapiEnabled && TheNetwork().wolapi() != nullptr) {
        selection = kSelMultiplayerGame;  //	We are returning from a game.
      }

      // -NEWGAME<scenario>: skip the menu and start that scenario as a
      // normal-difficulty campaign game, e.g. -NEWGAMESCG01EA. Used with
      // -QUITFRAME and -SAVESLOT to produce a reference save without a
      // display.
      if (selection == kSelNone && !startup_game_started &&
          !options.new_game.empty()) {
        TheScenario().CDifficulty = DIFF_NORMAL;
        TheScenario().Difficulty = DIFF_NORMAL;
        TheScenario().CarryOverMoney = 0;
        TheWorld().build_level() = 10;
        TheWorld().is_tanya_dead() = false;
        TheWorld().save_tanya() = false;
        TheWorld().whom() = HOUSE_GOOD;
        TheScenario().Set_Scenario_Name((options.new_game + ".INI").c_str());
        startup_game_started = true;
        TheSession().Type = GAME_NORMAL;
        process = false;
        continue;
      }

      // -LOADGAME<n>: skip the menu and load save slot n straight away.
      // Used with -QUITFRAME to drive save/load checks without a display.
      if (selection == kSelNone && !startup_game_started &&
          options.load_game >= 0) {
        const int slot = options.load_game;
        startup_game_started = true;
        if (Load_Game(slot)) {
          TheTheme().Queue_Song(magic_enum::enum_values<ThemeType>().front());
          process = false;
          gameloaded = true;
          continue;
        }
        LOG(ERROR) << "-LOADGAME: could not load slot " << slot;
      }

      if (selection == kSelNone) {
        TheWorld().ants_enabled() = false;
        selection = Main_Menu(ATTRACT_MODE_TIMEOUT);
      }
      ServiceRealTime();

      switch (selection) {
        /*
        **	Pick an expansion scenario.
        */
        case kSelNewScenarioCs:
        case kSelNewScenarioAm:
          TheScenario().CarryOverMoney = 0;
          TheWorld().is_tanya_dead() = false;
          TheWorld().save_tanya() = false;

          if (selection == kSelNewScenarioCs) {
            if (!Force_CD_Available(2)) {
              selection = kSelNone;
              break;
            }
            if (!Expansion_Dialog(true)) {
              selection = kSelNone;
              break;
            }
          } else {
            if (!Force_CD_Available(3)) {
              selection = kSelNone;
              break;
            }
            if (!Expansion_Dialog(false)) {
              selection = kSelNone;
              break;
            }
          }

          switch (Fetch_Difficulty(cdcheck >= 3)) {
            case 0:
              TheScenario().CDifficulty = DIFF_HARD;
              TheScenario().Difficulty = DIFF_EASY;
              break;

            case 1:
              TheScenario().CDifficulty = DIFF_HARD;
              TheScenario().Difficulty = DIFF_NORMAL;
              break;

            case 2:
              TheScenario().CDifficulty = DIFF_NORMAL;
              TheScenario().Difficulty = DIFF_NORMAL;
              break;

            case 3:
              TheScenario().CDifficulty = DIFF_EASY;
              TheScenario().Difficulty = DIFF_NORMAL;
              break;

            case 4:
              TheScenario().CDifficulty = DIFF_EASY;
              TheScenario().Difficulty = DIFF_HARD;
              break;
            default:
              break;
          }
          DLOG(INFO) << "Difficulty: player "
                     << magic_enum::enum_name(TheScenario().Difficulty)
                     << ", computer "
                     << magic_enum::enum_name(TheScenario().CDifficulty);

          TheTheme().Fade_Out();
          TheTheme().Queue_Song(magic_enum::enum_values<ThemeType>().front());
          TheSession().Type = GAME_NORMAL;
          process = false;
          break;

        /*
        **	SEL_START_NEW_GAME: Play the game
        */
        case kSelStartNewGame:
          if (TheSpecial().IsFromInstall) {
            TheScenario().CDifficulty = DIFF_NORMAL;
            TheScenario().Difficulty = DIFF_NORMAL;
          } else {
            switch (Fetch_Difficulty()) {
              case 0:
                TheScenario().CDifficulty = DIFF_HARD;
                TheScenario().Difficulty = DIFF_EASY;
                break;

              case 1:
                TheScenario().CDifficulty = DIFF_HARD;
                TheScenario().Difficulty = DIFF_NORMAL;
                break;

              case 2:
                TheScenario().CDifficulty = DIFF_NORMAL;
                TheScenario().Difficulty = DIFF_NORMAL;
                break;

              case 3:
                TheScenario().CDifficulty = DIFF_EASY;
                TheScenario().Difficulty = DIFF_NORMAL;
                break;

              case 4:
                TheScenario().CDifficulty = DIFF_EASY;
                TheScenario().Difficulty = DIFF_HARD;
                break;
              default:
                break;
            }
          }
          TheScenario().CarryOverMoney = 0;
          TheWorld().build_level() = 10;
          TheWorld().is_tanya_dead() = false;
          TheWorld().save_tanya() = false;
          TheWorld().whom() = HOUSE_GOOD;

          if (!TheSpecial().IsFromInstall) {
            if (TheWorld().ants_enabled()) {
              TheScenario().Set_Scenario_Name("SCA01EA.INI");
            } else {
              switch (WWMessageBox().Process(TXT_CHOOSE, TXT_ALLIES, TXT_CANCEL,
                                             TXT_SOVIET)) {
                case 2:
                  TheScenario().Set_Scenario_Name("SCU01EA.INI");
                  break;
                default:
                  selection = kSelNone;
                  continue;
                case 0:
                  TheScenario().Set_Scenario_Name("SCG01EA.INI");
                  break;
              }
            }
            TheTheme().Fade_Out();
            Load_Title_Page();
          } else {
            TheTheme().Fade_Out();
            PlayFirstLaunchIntro(TheScreen().hidden_view(),
                                 TheScreen().visible_view());
            Hide_Mouse();
            if (TheGameState().current_cd() == 0) {
              TheScenario().Set_Scenario_Name("SCG01EA.INI");
            } else {
              TheScenario().Set_Scenario_Name("SCU01EA.INI");
            }
          }

          TheSession().Type = GAME_NORMAL;
          process = false;
          break;

        /*
        **	Load a saved game.
        */
        case kSelLoadMission:
          if (LoadOptionsClass(LoadOptionsClass::LOAD).Process()) {
            TheTheme().Queue_Song(magic_enum::enum_values<ThemeType>().front());
            process = false;
            gameloaded = true;
          } else {
            display = true;
            selection = kSelNone;
          }
          break;

        /*
        **	SEL_MULTIPLAYER_GAME: set 'Session.Type' to nullptr-modem,
        * modem, or *	network play.
        */
        case kSelMultiplayerGame:
          //	With Westwood Online in charge, coming back here means we are
          //	returning from a game and the menu below is skipped.
          if (!config::kWolapiEnabled || TheNetwork().wolapi() == nullptr) {
            switch (TheSession().Type) {
              /*
              **	If 'Session.Type' isn't already set up for a
              * multiplayer game, *	we must prompt the user for which type
              * of multiplayer game *	they want.
              */
              case GAME_NORMAL:
                TheSession().Type = Select_MPlayer_Game();
                if (TheSession().Type == GAME_NORMAL) {  // 'Cancel'
                  display = true;
                  selection = kSelNone;
                }
                break;

              case GAME_SKIRMISH:
                if (!Com_Scenario_Dialog(true)) {
                  TheSession().Type = Select_MPlayer_Game();
                  if (TheSession().Type == GAME_NORMAL) {  // user hit Cancel
                    display = true;
                    selection = kSelNone;
                  }
                } else {
                  //	Ever hits? Session.Type set to GAME_SKIRMISH
                  // without
                  // user selecting in Select_MPlayer_Game()?
                  //	If mission is Counterstrike, CS CD will be required. But
                  // aftermath units require AM CD.
                  TheSession().IsAftermath =
                      Is_Aftermath_Installed() &&
                      !IsMissionCounterstrike(TheScenario().ScenarioName);
                  //	ajw I'll bet this was needed before also...
                  TheSession().ScenarioIsOfficial =
                      TheSession()
                          .Scenarios.at(TheSession().Options.ScenarioIndex)
                          ->Get_Official();
                }
                break;

              case GAME_NULL_MODEM:
              case GAME_MODEM:
                if (TheSession().Type != GAME_SKIRMISH &&
                    TheNetwork().null_modem().Num_Connections()) {
                  TheNetwork().null_modem().Init_Send_Queue();

                  if ((TheSession().Type == GAME_NULL_MODEM &&
                       TheSession().ModemType == MODEM_NULL_HOST) ||
                      (TheSession().Type == GAME_MODEM &&
                       TheSession().ModemType == MODEM_DIALER)) {
                    if (!Com_Scenario_Dialog()) {
                      TheSession().Type = Select_Serial_Dialog();
                      if (TheSession().Type ==
                          GAME_NORMAL) {  // user hit Cancel
                        display = true;
                        selection = kSelNone;
                      }
                    }
                  } else {
                    if (!Com_Show_Scenario_Dialog()) {
                      TheSession().Type = Select_Serial_Dialog();
                      if (TheSession().Type ==
                          GAME_NORMAL) {  // user hit Cancel
                        display = true;
                        selection = kSelNone;
                      }
                    }
                  }
                } else {
                  TheSession().Type = Select_MPlayer_Game();
                  if (TheSession().Type == GAME_NORMAL) {  // 'Cancel'
                    display = true;
                    selection = kSelNone;
                  }
                }
                break;

              // Back from an Internet game: prompt again, like GAME_NORMAL.
              case GAME_INTERNET:
                TheSession().Type = Select_MPlayer_Game();
                if (TheSession().Type == GAME_NORMAL) {  // 'Cancel'
                  display = true;
                  selection = kSelNone;
                }
                break;
              case GameType::GAME_IPX:
              default:
                break;
            }
          }  //	if( !pWolapi )

          if (config::kWolapiEnabled && TheNetwork().wolapi() != nullptr) {
            TheSession().Type = GAME_INTERNET;
          }
          // debugprint( "Session.Type = %i\n", Session.Type );
          switch (TheSession().Type) {
            /*
            **	Modem, Null-Modem or internet
            */
            case GAME_MODEM:
            case GAME_NULL_MODEM:
            case GAME_SKIRMISH:
              TheTheme().Fade_Out();
              process = false;
              TheOptions().ScoreVolume = TheOptions().MultiScoreVolume;
              break;

            //	With Westwood Online on, this runs the whole lobby; without it,
            //	the game was already set up above and this behaves like the
            //	cases just before.
            case GAME_INTERNET:
              if constexpr (config::kWolapiEnabled) {
                delete TheNetwork().packet_transport();
                TheNetwork().packet_transport() = new UDPInterfaceClass;
                DCHECK(TheNetwork().packet_transport() != nullptr);
                if (TheNetwork().packet_transport()->Init()) {
                  switch (WOL_Main()) {
                    case 1:
                      //	Start game.
                      TheOptions().ScoreVolume = TheOptions().MultiScoreVolume;
                      process = false;
                      TheTheme().Fade_Out();
                      break;
                    case 0:
                      //	User cancelled.
                      TheSession().Type = GAME_NORMAL;
                      display = true;
                      selection = kSelMultiplayerGame;  // SEL_NONE;
                      delete TheNetwork().packet_transport();
                      TheNetwork().packet_transport() = nullptr;
                      break;
                    case -1:
                      //	Patch was downloaded. Exit app.
                      TheTheme().Fade_Out();
                      ThePalettes().black_palette().Set(kFadePaletteSlow);
                      return false;
                    default:
                      break;
                  }
                } else {
                  TheSession().Type = GAME_NORMAL;
                  display = true;
                  selection = kSelMultiplayerGame;  // SEL_NONE;
                  delete TheNetwork().packet_transport();
                  TheNetwork().packet_transport() = nullptr;
                }
              } else {
                TheTheme().Fade_Out();
                process = false;
                TheOptions().ScoreVolume = TheOptions().MultiScoreVolume;
              }
              break;

            /*
            **	Network (IPX): start a new network game.
            */
            case GAME_IPX:
              WWDebugString("RA95 - Game type is IPX.\n");
              /*
              ** Init network system & remote-connect
              */
              delete TheNetwork().packet_transport();
              // we don't even have IPX
              TheNetwork().packet_transport() = new UDPInterfaceClass;
              TheNetwork().packet_transport()->Set_Broadcast_Address(
                  "255.255.255.255");
              WWDebugString("RA95 - About to call Init_Network.\n");
              if (TheSession().Type == GAME_IPX && Init_Network() &&
                  Remote_Connect()) {
                TheOptions().ScoreVolume = TheOptions().MultiScoreVolume;
                process = false;
                TheTheme().Fade_Out();
              } else {  // user hit cancel, or init failed
                TheSession().Type = GAME_NORMAL;
                display = true;
                selection = kSelNone;
                delete TheNetwork().packet_transport();
                TheNetwork().packet_transport() = nullptr;
              }
              break;
            case GameType::GAME_NORMAL:
            default:
              break;
          }
          break;

        /*
        **	Play a VQ
        */
        case kSelIntro:
          TheTheme().Fade_Out();
          if (TheDebugState().developer_mode()) {
            Play_Intro(TheDebugState().developer_mode());
          } else {
            Hide_Mouse();
            TheScreen().visible_page().Clear();
            Show_Mouse();
            Play_Movie(VQ_INTRO_MOVIE, THEME_NONE,
                       true);  // no transition picture to briefing
            TheKeyboard().Clear();
            Play_Movie(VQ_SIZZLE, THEME_NONE, true);
            Play_Movie(VQ_SIZZLE2, THEME_NONE, true);
            //						Play_Movie(VQ_INTRO_MOVIE,
            // THEME_NONE, false);		// has transitino picture to
            // briefing
          }
          TheTheme().Queue_Song(THEME_CRUS);
          display = true;
          selection = kSelNone;
          break;

        /*
        **	Exit to DOS.
        */
        case kSelExit:
          // No palette fade: the window closes right after, so it would
          // only delay the exit.
          TheTheme().Fade_Out();
          return false;

        /*
        **	Display the hall of fame.
        */
        case kSelFame:
          break;

        case kSelTimeout:
          if (TheSession().Attract && TheSession().RecordFile.IsAvailable()) {
            TheSession().Play = true;
            if (TheSession().RecordFile.Open(FileAccess::kRead)) {
              if (Load_Recording_Values(TheSession().RecordFile)) {
                process = false;
                TheTheme().Fade_Out();
              } else {
                TheSession().RecordFile.Close();
                TheSession().Play = false;
                selection = kSelNone;
              }
            } else {
              TheSession().Play = false;
              selection = kSelNone;
            }
          } else {
            selection = kSelNone;
          }
          break;

        default:
          break;
      }
    }
  } else {
    /*
    ** For TheDebugState().map_editor_active() (editor) mode to load scenario
    */
    TheScenario().Set_Scenario_Name("SCG01EA.INI");
  }

  /*
  **	Don't carry stray keystrokes into game.
  */
  TheKeyboard().Clear();

  /*
  ** Initialize the random number generator(s)
  */
  Init_Random();

  /*
  ** Save initialization values if we're recording this game.
  */
  if (TheSession().Record) {
    if (TheSession().RecordFile.Open(FileAccess::kWrite)) {
      Save_Recording_Values(TheSession().RecordFile);
    } else {
      TheSession().Record = false;
    }
  }

  switch (TheSession().Type) {
    case GAME_MODEM:
    case GAME_NULL_MODEM:
    case GAME_IPX:
      if (!TheSession().IsAftermath) {
        TheRules().NewUnitsEnabled = TheRules().SecretUnitsEnabled = false;
      } else {
        TheRules().NewUnitsEnabled = true;
      }
      //			debugprint( "Non Internet game: NewUnitsEnabled
      //= %i\n", NewUnitsEnabled );
      break;
    case GAME_INTERNET:
      if (!config::kWolapiEnabled || TheNetwork().wolapi() == nullptr) {
        //				debugprint( "pWolapi is
        // null on internet
        // game!" );
        Fatal("pWolapi is null on internet game!");
      }
      // if( pWolapi->bEnableNewAftermathUnits )
      if (TheSession().IsAftermath) {
        TheRules().NewUnitsEnabled = true;
      } else {
        TheRules().NewUnitsEnabled = TheRules().SecretUnitsEnabled = false;
      }
      //			debugprint( "Internet game: NewUnitsEnabled =
      //%i\n", NewUnitsEnabled );
      break;
    case GameType::GAME_NORMAL:
    case GameType::GAME_SKIRMISH:
    default:
      break;
  }
  /*
  **	Load the scenario.  Specify variation 'A' for the editor; for the game,
  **	don't specify a variation, to make 'Set_Scenario_Name()' pick a random
  *one. *	Skip this if we've already loaded a save-game.
  */
  if (!gameloaded && !TheSession().LoadGame) {
    //		if (TheDebugState().map_editor_active()) {
    //			Set_Scenario_Name(Scen.ScenarioName, Scen.Scenario,
    // Scen.ScenPlayer, Scen.ScenDir, SCEN_VAR_A); 		}  else {
    //			Set_Scenario_Name(Scen.ScenarioName, Scen.Scenario,
    // Scen.ScenPlayer, Scen.ScenDir);
    //		}

    /*
    ** Start_Scenario() changes the palette; so, fade out & clear the screen
    ** before calling it.
    */
    Hide_Mouse();

    if (selection != kSelStartNewGame) {
      ThePalettes().black_palette().Set(kFadePaletteMedium, ServiceRealTime);
      TheScreen().hidden_page().Clear();
      TheScreen().visible_page().Clear();
    }
    Show_Mouse();
    if (!Start_Scenario(TheScenario().ScenarioName)) {
      return false;
    }
    if (TheSpecial().IsFromInstall) {
      Show_Mouse();
    }
    TheSpecial().IsFromInstall = false;
  }

  /*
  **	For multiplayer games, initialize the inter-player message system.
  **	Do this after loading the scenario, so the map's upper-left corner is
  **	properly set.
  */
  TheSession().Messages.Init(
      TheMap().TacPixelX, TheMap().TacPixelY,  // x,y for messages
      6,                                       // max # msgs
      MAX_MESSAGE_LENGTH - 14,                 // max msg length
      14,                                      // font height in pixels
      -1, -1,                   // x,y for edit line (appears above msgs)
      0,                        // BG		1,
                                // // enable edit overflow
      20,                       // min,
      MAX_MESSAGE_LENGTH - 14,  //    max for trimming overflow
      Lepton_To_Pixel(TheMap().TacLeptonWidth));  // Width in pixels of buffer

  if (TheSession().Type != GAME_NORMAL && TheSession().Type != GAME_SKIRMISH &&
      !TheSession().Play) {
    TheSession().Create_Connections();
  }

  /*
  ** If this isnt an internet game that set the unit build rate to its default
  *value
  */
  if (TheSession().Type != GAME_INTERNET) {
    TheRules().UnitBuildPenalty = 100;
  }

  /*
  **	Hide the visible_view; force the map to render one frame.  The caller
  * can *	then fade the palette in. *	(If we loaded a game, this step
  * will fade out the title screen.  If we *	started a scenario,
  * Start_Scenario() will have played a couple of VQ *	movies, which will have
  * cleared the screen to black already.)
  */
  ServiceRealTime();
  Hide_Mouse();
  ThePalettes().black_palette().Set(kFadePaletteMedium, ServiceRealTime);
  TheScreen().hidden_page().Clear();
  TheScreen().visible_page().Clear();
  Show_Mouse();
  SetLogicPage(TheScreen().visible_view());
  /*
  ** Sidebar is always active in hi-res.
  */
  if (!TheDebugState().map_editor_active()) {
    TheMap().Activate(1);
  }
  TheMap().Flag_To_Redraw();
  ServiceRealTime();
  TheMap().Render();

  return true;
}

// The Westwood logo movie plays only until one launch movie has been shown;
// otherwise it is 10.5 s of every start. The original game already showed
// its full intro once, on the first launch ([Intro] PlayIntro, read in
// startup.cc), but then played the logo on every launch after it.
// [Intro] LogoPlayed in the config file records that a launch movie was
// shown; delete the key to see the logo again. These read the file directly
// because Options.Load_Settings() runs after the logo.
static constexpr char kLogoPlayedSection[] = "Intro";
static constexpr char kLogoPlayedEntry[] = "LogoPlayed";

static bool LogoAlreadyPlayed() {
  GameFile file(kConfigFileName);
  INIClass ini;
  if (file.IsAvailable()) {
    ini.Load(file);
  }
  return ini.Get_Bool(kLogoPlayedSection, kLogoPlayedEntry, false);
}

static void MarkLogoPlayed() {
  // A -NOMOVIES run never showed the movie, so it must not use up the one
  // showing.
  if (bNoMovies) {
    return;
  }
  GameFile file(kConfigFileName);
  INIClass ini;
  // Keep every other setting in the file.
  if (file.IsAvailable()) {
    ini.Load(file);
  }
  ini.Put_Bool(kLogoPlayedSection, kLogoPlayedEntry, true);
  ini.Save(file);
}

/***********************************************************************************************
 * Play_Intro -- plays the introduction & logo movies *
 *                                                                                             *
 * INPUT: *
 *                                                                                             *
 * OUTPUT: * none.
 **
 *                                                                                             *
 * WARNINGS: * none.
 **
 *                                                                                             *
 * HISTORY: * 06/06/1995 BRR : Created. * 05/08/1996 JLB : Modified for Red
 *Alert and direction control.                            *
 *=============================================================================================*/
static void Play_Intro(bool sequenced) {
  static VQType _counter = magic_enum::enum_values<VQType>().front();

  TheKeyboard().Clear();
  if (sequenced) {
    // Play the movies from the last down to the first, then wrap.
    if (_counter <= magic_enum::enum_values<VQType>().front()) {
      _counter = magic_enum::enum_values<VQType>().back();
    }
    if (_counter == VQ_REDINTRO) {
      _counter--;
    }
    if (_counter == VQ_TITLE) {
      _counter--;
    }
    Hide_Mouse();
    TheScreen().visible_page().Clear();
    Show_Mouse();
    Play_Movie(static_cast<VQType>(_counter--), THEME_NONE);

    //		Show_Mouse();
  } else {
    Hide_Mouse();
    TheScreen().visible_page().Clear();
    Show_Mouse();
    Play_Movie(VQ_REDINTRO, THEME_NONE, false);
  }
}

/***********************************************************************************************
 * Anim_Init -- Initialize the VQ animation control structure. *
 *                                                                                             *
 *    VQ animations are controlled by a structure passed to the VQ player. This
 *routine        * initializes the structure to values required by C&C. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   Only need to call this routine once at the beginning of the game.
 **
 *                                                                                             *
 * HISTORY: * 12/20/1994 JLB : Created. *
 *=============================================================================================*/
void Anim_Init() {
  /* Configure player with INI file */
  VQA_DefaultConfig(&TheGameState().anim_control());
  TheGameState().anim_control().DrawFlags = VQACFGF_TOPLEFT;
  TheGameState().anim_control().DrawFlags |= VQACFGF_BUFFER;
  // AnimControl.DrawFlags |= VQACFGF_NODRAW;
  // BG - M. Grayford says turn this off
  // AnimControl.DrawFlags |= VQACFGF_NOSKIP;

  TheGameState().anim_control().DrawFlags |= VQACFGF_NOSKIP;
  TheGameState().anim_control().FrameRate = -1;
  TheGameState().anim_control().DrawRate = -1;
  TheGameState().anim_control().DrawerCallback = VQ_Call_Back;
  TheGameState().anim_control().EventHandler = VQ_Event_Handler;
  TheGameState().anim_control().ImageWidth = 320;
  TheGameState().anim_control().ImageHeight = 200;
  TheGameState().anim_control().ImageBuf =
      TheScreen().sys_mem_page().Get_Bytes();
  if (TheScreen().is_vq640()) {
    TheGameState().anim_control().ImageWidth = 640;
    TheGameState().anim_control().ImageHeight = 400;
    TheGameState().anim_control().ImageBuf = TheScreen().vq640().Get_Bytes();
  }
  TheGameState().anim_control().Vmode = 0;
  TheGameState().anim_control().OptionFlags |= VQAOPTF_CAPTIONS | VQAOPTF_EVA;
  if (ThePalettes().slow_palette()) {
    TheGameState().anim_control().OptionFlags |= VQAOPTF_SLOWPAL;
  }
  TheGameState().anim_control().AudioDeviceID = TheAudio().device_id();
  TheGameState().anim_control().AudioCallback =
      TheAudio().extra_callback_slot();
  TheGameState().anim_control().AudioSpec = TheAudio().output_spec();
}

// Kept out of Parse_Command_Line() so the std::optional below does not make
// clang-tidy run its optional-access dataflow over that whole function.
std::optional<IPXAddressClass> ParseDestNet(const std::string_view address) {
  NetNumType net;
  NetNodeType node;

  /*
  ** Scan the command-line string, pulling off each address piece
  */
  int i = 0;
  for (const std::string_view piece :
       absl::StrSplit(address, '.', absl::SkipEmpty())) {
    const auto byte = tech::ParseHex<uint8_t>(piece);
    if (!byte || i >= 10) {
      i = 0;  // Reject the address instead of accepting a partial network.
      break;
    }
    if (i < 4) {
      base::At(net, i) = *byte;  // fill NetNum
    } else {
      base::At(node, i - 4) = *byte;  // fill NetNode
    }
    i++;
  }

  if (i < 4) {
    return std::nullopt;
  }
  // The node becomes a broadcast address, so packets reach every machine on
  // the network across the bridge.
  base::FillBytes(base::ObjectBytes(node), 0xff, 6);
  return IPXAddressClass(net, node);
}

// Returns the IPX socket "-SOCKET<offset>" asks for: 0x4000 plus an offset in
// [0, 0x4000). Anything else leaves the socket alone.
static std::optional<uint16_t> ParseSocketArgument(
    std::string_view offset_text) {
  const auto offset = tech::ParseInteger<int>(offset_text);
  if (!offset || *offset < 0 || *offset >= 0x4000) {
    return std::nullopt;
  }
  return static_cast<uint16_t>(*offset + 0x4000);
}

std::optional<StartupOptions> Parse_Command_Line(
    const std::span<const std::string_view> arguments) {
  StartupOptions options;

  for (const std::string_view argument : arguments) {
    const std::string upper_argument = absl::AsciiStrToUpper(argument);
    const std::string_view string = upper_argument;

    /*
    **	Print usage text only if requested.
    */
    if (absl::EqualsIgnoreCase("/?", string) ||
        absl::EqualsIgnoreCase("-?", string) ||
        absl::EqualsIgnoreCase("-h", string) ||
        absl::EqualsIgnoreCase("/h", string)) {
      /*
      **	Unrecognized command line parameter... Display usage
      **	and then exit.
      */
      absl::PrintF("%s\n", kLanguageText.options);
      return std::nullopt;
    }

    bool processed = true;
    const uint32_t ob = HashKeyPhrase(string);

    /*
    **	Check to see if the parameter is a cheat enabling one.
    */
    for (const uint32_t code : CheatCodes) {
      if (code == 0) {
        break;
      }
      if (code == ob) {
        options.playtest = true;
        options.developer_mode = true;
        break;
      }
    }

    /*
    **	Check to see if the parameter is a cheat enabling one.
    */
    for (const uint32_t code : PlayCodes) {
      if (code == 0) {
        break;
      }
      if (code == ob) {
        options.playtest = true;
        options.developer_mode = true;
        break;
      }
    }

    /*
    **	Check to see if the parameter is a scenario editor
    **	enabling one.
    */
    for (const uint32_t code : EditorCodes) {
      if (code == 0) {
        break;
      }
      if (code == ob) {
        options.map_editor_active = true;
        options.unshroud = true;
        options.developer_mode = true;
        options.playtest = true;
        break;
      }
    }

    switch (ob) {
      case kParmPlaytest:
        if constexpr (config::kVirginCheatKeysEnabled) {
          options.playtest = true;
        }
        break;

      /*
      ** Special flag - is C&C being run from the install program?
      */
      case kParmInstall:
        options.from_install = true;
        break;

      default:
        processed = false;
        break;
    }
    if (processed) {
      continue;
    }

    if constexpr (config::kCheatKeysEnabled) {
      /*
      **	Scenario Editor Mode
      */
      if (absl::EqualsIgnoreCase(string, "-CHECKMAP")) {
        options.check_map = true;
        continue;
      }
    }

    /*
    **	File search path override.
    */
    if (string.contains("-CD")) {
      // The original argument keeps the case of the path for Unix.
      options.search_paths.emplace_back(argument.substr(3));
      continue;
    }

    /*
    **	Specify destination connection for network play
    */
    if (string.contains("-DESTNET")) {
      // A malformed address leaves any earlier one alone.
      const std::optional<IPXAddressClass> bridge_net =
          ParseDestNet(string.substr(8));
      if (bridge_net.has_value()) {
        options.bridge_net = bridge_net;
      }
      continue;
    }

    /*
    **	Specify socket ID, as an offset from 0x4000.
    */
    if (string.contains("-SOCKET")) {
      // An out-of-range offset leaves any earlier one alone.
      const std::optional<uint16_t> socket = ParseSocketArgument(
          string.substr(std::string_view("-SOCKET").size()));
      if (socket.has_value()) {
        options.socket = socket;
      }
      continue;
    }

    /*
    **	Set the Net Stealth option
    */
    if (string.contains("-STEALTH")) {
      options.net_stealth = true;
      continue;
    }

    /*
    **	Set the Net Protection option
    */
    if (string.contains("-MESSAGES")) {
      options.outside_messages = true;
      continue;
    }

    /*
    **	Allow "attract" mode
    */
    if (string.contains("-ATTRACT")) {
      options.attract = true;
      continue;
    }

    /*
    ** Set screen to 640x480 instead of 640x400
    */
    if (string.contains("-480")) {
      options.tall_screen = true;
      continue;
    }

    if constexpr (config::kCheatKeysEnabled) {
      // Specify the random number seed (for debugging)
      if (string.contains("-SEED")) {
        options.custom_seed = tech::ParseIntegerOr<uint16_t>(
            string.substr(std::string_view("-SEED").size()),
            options.custom_seed);
        continue;
      }
    }

    if (std::string_view(string) == "-NOMOVIES") {
      options.no_movies = true;
      continue;
    }

    // Developer switches for save-game checks; see Select_Game and
    // RunFrame.
    if (string.starts_with("-LOADGAME")) {
      options.load_game = tech::ParseIntegerOr<int>(string.substr(9), -1);
      continue;
    }
    if (string.starts_with("-QUITFRAME")) {
      options.quit_at_frame = tech::ParseIntegerOr<int>(string.substr(10), -1);
      // Nobody watches an automated run; its fades only add wall time.
      options.disable_fades = true;
      continue;
    }
    if (std::string_view(string) == "-NOFADE") {
      options.disable_fades = true;
      continue;
    }
    if (string.starts_with("-NEWGAME")) {
      options.new_game = string.substr(8);
      continue;
    }
    if (string.starts_with("-SAVESLOT")) {
      options.save_slot = tech::ParseIntegerOr<int>(string.substr(9), -1);
      continue;
    }

    /*
    ** Disable mouse grabbing for debugging
    */
    if (string.contains("-NOMOUSEGRAB")) {
      options.no_mouse_grab = true;
    }

    /*
    **	Special command line control parsing.
    */
    if (absl::StartsWithIgnoreCase(string, "-X")) {
      for (const char code : string.substr(2)) {
        if constexpr (config::kCheatKeysEnabled) {
          switch (code) {
            case 'I':
              options.inert_weapons = true;
              continue;
            case 'H':
              options.speed_build = true;
              continue;
            case 'X':
              options.record = true;
              continue;
            case 'Y':
              options.play = true;
              continue;
            case 'P':
              options.print_events = true;
              continue;
            default:
              break;
          }
        }

        if (code == 'Q') {
          options.quiet = true;
        } else {
          absl::PrintF("%s\n", kLanguageText.invalid_option);
          return std::nullopt;
        }
      }
    }
  }
  return options;
}

/***************************************************************************
 * Init_Random -- Initializes the random-number generator                  *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
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
 *   12/04/1995 BRR : Created.                                             *
 *=========================================================================*/
void Init_Random() {
  //
  // If we've loaded a multiplayer save game, return now; the random #
  // class is loaded along with ScenarioClass.
  //
  if (TheSession().LoadGame) {
    return;
  }

  //
  // If we're playing a recording, the Seed is loaded in
  // Load_Recording_Values().  Just init the random # and return.
  //
  if (TheSession().Play) {
    RandNumb = TheWorld().seed();
    TheScenario().sync_rng_.set_seed(static_cast<uint32_t>(TheWorld().seed()));
    return;
  }

  /*
  **	Initialize the random number Seed.  For multiplayer, this
  * will have been done
  ** in the connection dialogs.  For single-player games, AND if we're not
  *playing
  ** back a recording, init the Seed to a random value.
  */
  if (TheSession().Type == GAME_NORMAL ||
      (TheSession().Type == GAME_SKIRMISH && !TheSession().Play)) {
    /*
    ** Set the optional user-specified seed
    */
    if (TheStartupOptions().custom_seed != 0) {
      TheWorld().seed() = TheStartupOptions().custom_seed;
    } else {
      TheWorld().seed() = port::RandomSeed();
    }
  }

  /*
  **	Initialize the random-number generators
  */
  TheScenario().sync_rng_.set_seed(static_cast<uint32_t>(TheWorld().seed()));
  RandNumb = TheWorld().seed();
}

/***********************************************************************************************
 * Load_Title_Page -- Load the background art for the title page. *
 *                                                                                             *
 *    This routine will load the background art in a machine independent format.
 *There is      * different art required for the hi-res and lo-res versions of
 *the game.                   *
 *                                                                                             *
 * INPUT:   visible  -- Should the title page art be copied to the visible page
 *by this        * routine? *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   Be sure the mouse is hidden if the image is to be copied to the
 *visible page.   *
 *                                                                                             *
 * HISTORY: * 06/03/1996 JLB : Created. *
 *=============================================================================================*/
void Load_Title_Page(bool visible) {
  Load_Title_Screen("TITLE.PCX", &TheScreen().hidden_view(),
                    ThePalettes().title_palette());

  if (visible) {
    TheScreen().hidden_view().Blit(TheScreen().visible_view());
  }
}

/***********************************************************************************************
 * Init_Color_Remaps -- Initialize the text remap tables. *
 *                                                                                             *
 *    There are various color scheme remap tables that are dependant upon the
 *color remap      * information embedded within the palette control file. This
 *routine will fetch that       * data and build the text remap tables as
 *indicated.                                       *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/03/1996 JLB : Created. *
 *=============================================================================================*/
static void Init_Color_Remaps() {
  /*
  **	Setup the remap tables.  PALETTE.CPS contains a special set of pixels in
  ** the upper-left corner.  Each row of 16 pixels is one range of colors.  The
  ** first row represents unity (the default color units are drawn in); rows
  ** after that are the remap colors.
  */

  TheScreen().sys_mem_page().Clear();
  Load_Picture("PALETTE.CPS", TheScreen().sys_mem_page(),
               TheScreen().sys_mem_page(), {}, BM_DEFAULT);
  TheScreen().sys_mem_page().Blit(TheScreen().hidden_view());
  for (const PlayerColorType pcolor :
       magic_enum::enum_values<PlayerColorType>()) {
    auto& ptr = ThePalettes().color_remaps().at(pcolor).RemapTable;

    for (int color = 0; color < 256; color++) {
      base::At(ptr, color) = static_cast<unsigned char>(color);
    }

    for (int index = 0; index < 16; index++) {
      base::At(ptr, TheScreen().hidden_view().GetPixel(index, 0)) =
          static_cast<unsigned char>(TheScreen().hidden_view().GetPixel(
              index, static_cast<int>(pcolor)));
    }
    for (int index = 0; index < 6; index++) {
      base::At(ThePalettes().color_remaps().at(pcolor).FontRemap, 10 + index) =
          static_cast<unsigned char>(TheScreen().hidden_view().GetPixel(
              2 + index, static_cast<int>(pcolor)));
    }
    ThePalettes().color_remaps().at(pcolor).BrightColor = kWhite;
    //		ColorRemaps[pcolor].BrightColor = hidden_view.GetPixel(1,
    // static_cast<int>(pcolor));
    ThePalettes().color_remaps().at(pcolor).Color = static_cast<unsigned char>(
        TheScreen().hidden_view().GetPixel(4, static_cast<int>(pcolor)));

    ThePalettes().color_remaps().at(pcolor).Shadow = static_cast<unsigned char>(
        TheScreen().hidden_view().GetPixel(10, static_cast<int>(pcolor)));
    ThePalettes().color_remaps().at(pcolor).Background =
        static_cast<unsigned char>(
            TheScreen().hidden_view().GetPixel(9, static_cast<int>(pcolor)));
    ThePalettes().color_remaps().at(pcolor).Corners =
        static_cast<unsigned char>(
            TheScreen().hidden_view().GetPixel(7, static_cast<int>(pcolor)));
    ThePalettes().color_remaps().at(pcolor).Highlight =
        static_cast<unsigned char>(
            TheScreen().hidden_view().GetPixel(4, static_cast<int>(pcolor)));
    ThePalettes().color_remaps().at(pcolor).Bright = static_cast<unsigned char>(
        TheScreen().hidden_view().GetPixel(0, static_cast<int>(pcolor)));
    ThePalettes().color_remaps().at(pcolor).Underline =
        static_cast<unsigned char>(
            TheScreen().hidden_view().GetPixel(0, static_cast<int>(pcolor)));
    ThePalettes().color_remaps().at(pcolor).Bar = static_cast<unsigned char>(
        TheScreen().hidden_view().GetPixel(6, static_cast<int>(pcolor)));

    /*
    **	This must grab from column 4 because the multiplayer color dialog
    *palette counts *	on this to be true.
    */
    ThePalettes().color_remaps().at(pcolor).Box = static_cast<unsigned char>(
        TheScreen().hidden_view().GetPixel(4, static_cast<int>(pcolor)));
  }

  /*
  ** Now do the special dim grey scheme
  */
  for (int color = 0; color < 256; color++) {
    base::At(ThePalettes().grey_scheme().RemapTable, color) =
        static_cast<unsigned char>(color);
  }
  // The palette index in the low byte of the pixel read from the grey row.
  const auto GreyPixel = [](int x) {
    return static_cast<uint8_t>(
        TheScreen().hidden_view().GetPixel(x, static_cast<int>(PCOLOR_GREY)));
  };
  for (int index = 0; index < 6; index++) {
    base::At(ThePalettes().grey_scheme().FontRemap, 10 + index) =
        GreyPixel(9 + index);
  }
  ThePalettes().grey_scheme().BrightColor = GreyPixel(3);
  ThePalettes().grey_scheme().Color = GreyPixel(7);

  ThePalettes().grey_scheme().Shadow = base::At(
      ThePalettes().color_remaps().at(PCOLOR_GREY).RemapTable, GreyPixel(15));
  ThePalettes().grey_scheme().Background = base::At(
      ThePalettes().color_remaps().at(PCOLOR_GREY).RemapTable, GreyPixel(14));
  ThePalettes().grey_scheme().Corners = base::At(
      ThePalettes().color_remaps().at(PCOLOR_GREY).RemapTable, GreyPixel(13));
  ThePalettes().grey_scheme().Highlight = base::At(
      ThePalettes().color_remaps().at(PCOLOR_GREY).RemapTable, GreyPixel(9));
  ThePalettes().grey_scheme().Bright = base::At(
      ThePalettes().color_remaps().at(PCOLOR_GREY).RemapTable, GreyPixel(5));
  ThePalettes().grey_scheme().Underline = base::At(
      ThePalettes().color_remaps().at(PCOLOR_GREY).RemapTable, GreyPixel(5));
  ThePalettes().grey_scheme().Bar = base::At(
      ThePalettes().color_remaps().at(PCOLOR_GREY).RemapTable, GreyPixel(11));
  ThePalettes().grey_scheme().Box = base::At(
      ThePalettes().color_remaps().at(PCOLOR_GREY).RemapTable, GreyPixel(11));

  /*
  ** Set up the metallic remap table for the font that prints over the tabs
  */
  base::FillBytes(base::ObjectBytes(ThePalettes().metal_scheme()), 4,
                  sizeof(ThePalettes().metal_scheme()));
  for (int color_counter = 0; color_counter < 16; color_counter++) {
    base::At(ThePalettes().metal_scheme().FontRemap, color_counter) =
        static_cast<unsigned char>(color_counter);
  }
  ThePalettes().metal_scheme().FontRemap[1] = 128;
  ThePalettes().metal_scheme().FontRemap[2] = 12;
  ThePalettes().metal_scheme().FontRemap[3] = 13;
  ThePalettes().metal_scheme().FontRemap[4] = 14;
  ThePalettes().metal_scheme().Color = 128;
  ThePalettes().metal_scheme().Background = 0;
  ThePalettes().metal_scheme().Underline = 128;

  /*
  ** Set up the font remap table for the mission briefing font
  */
  for (int colr = 0; colr < 16; colr++) {
    base::At(ThePalettes().color_remaps().at(PCOLOR_TYPE).FontRemap, colr) =
        static_cast<unsigned char>(TheScreen().hidden_view().GetPixel(
            colr, static_cast<int>(PCOLOR_TYPE)));
  }

  ThePalettes().color_remaps().at(PCOLOR_TYPE).Shadow = 11;
  ThePalettes().color_remaps().at(PCOLOR_TYPE).Background = 10;
  ThePalettes().color_remaps().at(PCOLOR_TYPE).Corners = 10;
  ThePalettes().color_remaps().at(PCOLOR_TYPE).Highlight = 9;
  ThePalettes().color_remaps().at(PCOLOR_TYPE).Bright = 15;
  ThePalettes().color_remaps().at(PCOLOR_TYPE).Underline = 11;
  ThePalettes().color_remaps().at(PCOLOR_TYPE).Bar = 11;
  ThePalettes().color_remaps().at(PCOLOR_TYPE).Box = 10;
  ThePalettes().color_remaps().at(PCOLOR_TYPE).BrightColor = 15;
  ThePalettes().color_remaps().at(PCOLOR_TYPE).Color = 9;

  GadgetClass::Set_Color_Scheme(
      &ThePalettes().color_remaps().at(PCOLOR_DIALOG_BLUE));
  //	GadgetClass::Set_Color_Scheme(&ColorRemaps[PCOLOR_BLUE]);
}

/***********************************************************************************************
 * Init_Heaps -- Initialize the game heaps and buffers. *
 *                                                                                             *
 *    This routine will allocate the game heaps and buffers. The rules file has
 *already been   * processed by the time that this routine is called. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/03/1996 JLB : Created. *
 *=============================================================================================*/
static void Init_Heaps() {
  /*
  **	Initialize the game object heaps.
  */
  TheObjectHeaps().vessel().Set_Heap(TheRules().VesselMax);
  TheObjectHeaps().unit().Set_Heap(TheRules().UnitMax);
  TheObjectHeaps().factory().Set_Heap(TheRules().FactoryMax);
  TheObjectHeaps().terrain().Set_Heap(TheRules().TerrainMax);
  TheObjectHeaps().tmplate().Set_Heap(TheRules().TemplateMax);
  TheObjectHeaps().smudge().Set_Heap(TheRules().SmudgeMax);
  TheObjectHeaps().overlay().Set_Heap(TheRules().OverlayMax);
  TheObjectHeaps().infantry().Set_Heap(TheRules().InfantryMax);
  TheObjectHeaps().bullet().Set_Heap(TheRules().BulletMax);
  TheObjectHeaps().building().Set_Heap(TheRules().BuildingMax);
  TheObjectHeaps().anim().Set_Heap(TheRules().AnimMax);
  TheObjectHeaps().aircraft().Set_Heap(TheRules().AircraftMax);
  TheObjectHeaps().trigger().Set_Heap(TheRules().TriggerMax);
  TheObjectHeaps().team_type().Set_Heap(TheRules().TeamTypeMax);
  TheObjectHeaps().team().Set_Heap(TheRules().TeamMax);
  TheObjectHeaps().house().Set_Heap(kHouseMax);
  TheObjectHeaps().trigger_type().Set_Heap(TheRules().TrigTypeMax);
  //	Weapons.Set_Heap(Rule.WeaponMax);

}

/***********************************************************************************************
 * Init_Expansion_Files -- Fetch any override expansion mixfiles. *
 *                                                                                             *
 *    This routine will search for and register/cache any override mixfiles
 *found.             *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/03/1996 JLB : Created. *
 *=============================================================================================*/
static void Init_Expansion_Files() {
  /*
  **	Before all else, cache any additional mixfiles.
  */
  FindFileState state{};
  if (Find_First_File("SC*.MIX", state)) {
    do {
      // scores shouldn't be loaded here but may be found if main has been
      // extracted
      if (absl::EqualsIgnoreCase(state.name, "scores.mix")) {
        continue;
      }
      MixArchive::Register(state.name, &TheAssets().mix_key());
      MixArchive::Cache(state.name);
    } while (Find_Next_File(state));
  }
  if (Find_First_File("SS*.MIX", state)) {
    do {
      MixArchive::Register(state.name, &TheAssets().mix_key());
    } while (Find_Next_File(state));
  }
}

/***********************************************************************************************
 * Init_One_Time_Systems -- Initialize internal pointers to the bulk data. *
 *                                                                                             *
 *    This performs the one-time processing required after the bulk data has
 *been cached but   * before the game actually starts. Typically, this routine
 *extracts pointers to all the    * embedded data sub-files within the main game
 *data mixfile. This routine must be called   * AFTER the bulk data has been
 *cached.                                                     *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   Call this routine AFTER the bulk data has been cached. *
 *                                                                                             *
 * HISTORY: * 06/03/1996 JLB : Created. *
 *=============================================================================================*/
static void Init_One_Time_Systems() {
  ServiceRealTime();
  TheMap().One_Time();
  TheWorld().logic().One_Time();
  TheOptions().One_Time();
  TheSession().One_Time();

  ObjectTypeClass::One_Time();
  BuildingTypeClass::One_Time();
  BulletTypeClass::One_Time();
  HouseTypeClass::One_Time();
  TemplateTypeClass::One_Time();
  OverlayTypeClass::One_Time();
  SmudgeTypeClass::One_Time();
  TerrainTypeClass::One_Time();
  UnitTypeClass::One_Time();
  VesselTypeClass::One_Time();
  InfantryTypeClass::One_Time();
  AnimTypeClass::One_Time();
  AircraftTypeClass::One_Time();
  HouseClass::One_Time();
}

/***********************************************************************************************
 * Init_Fonts -- Initialize all the game font pointers. *
 *                                                                                             *
 *    This routine is used to fetch pointers to the game fonts. The mixfile
 *containing these   * fonts must have been previously cached. This routine is a
 *necessary prerequisite to      * displaying any dialogs or printing any text.
 **
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/03/1996 JLB : Created. *
 *=============================================================================================*/
static void Init_Fonts() { TheAssets().LoadFonts(); }

/***********************************************************************************************
 * Init_CDROM_Access -- Initialize the CD-ROM access handler. *
 *                                                                                             *
 *    This routine is called to setup the CD-ROM access or emulation handler. It
 *will ensure   * that the appropriate CD-ROM is present (dependant on the
 *RequiredCD global).             *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   The fonts, palettes, and other bootstrap systems must have been
 *initialized     * prior to calling this routine since this routine will quite
 *likely display      * a dialog box requesting the appropriate CD be inserted.
 **
 *                                                                                             *
 * HISTORY: * 06/03/1996 JLB : Created. *
 *=============================================================================================*/
static void Init_CDROM_Access() {
  TheScreen().visible_page().Clear();
  TheScreen().hidden_view().Clear();

  //	Determine if we're going to be running from a DVD.
  //	The entire session will either require a DVD, or the regular CDs. Never
  // both. 	Call Using_DVD() to determine which case it is. 	Here we
  // set the value that Using_DVD() returns.
  Determine_If_Using_DVD();
  //	Force_CD_Available() is modified when Using_DVD() is true so that all
  // requests become requests for the DVD.

  /*
  **	Always try to look at the CD-ROM for data files.
  */
  if (!SearchPaths::HasAny()) {
    /*
    **	This call is needed because of a side effect of this function. It will
    *examine the *	CD-ROMs attached to this computer and set the
    *appropriate status values. Without this *	call, the "?:\\" could not be
    *filled in correctly.
    */
    Force_CD_Available(-1);

    /*
    ** If there are no search drives specified then we must be playing
    ** off cd, so read files from there.
    */
    int error = 0;

    do {
      error = SearchPaths::Add("?:\\");
      switch (error) {
        case 1:
          TheScreen().visible_page().Clear();
          ThePalettes().game_palette().Set();
          Show_Mouse();
          WWMessageBox().Process(TXT_CD_ERROR1, TXT_OK);
          // Prog_End();
          EmergencyExit(EXIT_FAILURE);

        case 2:
          TheScreen().visible_page().Clear();
          ThePalettes().game_palette().Set();
          Show_Mouse();
          if (WWMessageBox().Process(TXT_CD_DIALOG_1, TXT_OK, TXT_CANCEL) ==
              1) {
            // Prog_End();
            EmergencyExit(EXIT_FAILURE);
          }
          Hide_Mouse();
          break;

        default:
          TheScreen().visible_page().Clear();
          Show_Mouse();
          if (!Force_CD_Available(TheGameState().required_cd())) {
            // Prog_End();
            EmergencyExit(EXIT_FAILURE);
          }
          Hide_Mouse();
          break;
      }
    } while (error);

    TheGameState().required_cd() = -1;
  } else {
    /*
    ** If there are search drives specified then all files are to be
    ** considered local.
    */
    TheGameState().required_cd() = -2;
  }
}

/***********************************************************************************************
 * Init_Bootstrap_Mixfiles -- Registers and caches any mixfiles needed for
 *bootstrapping.      *
 *                                                                                             *
 *    This routine will register the initial mixfiles that are required to
 *display error       * messages and get input from the player. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   Be sure to call this routine before any dialogs would be
 *displayed to the       * player. *
 *                                                                                             *
 * HISTORY: * 06/03/1996 JLB : Created. *
 *=============================================================================================*/
// Caches a registered mixfile, or exits if it cannot be read. Registration
// only proves the file exists, so a failure here means it is truncated or
// corrupt. Damaged data files are a user setup problem, not a bug: report and
// exit quietly rather than abort with a stack trace.
static void Cache_Or_Exit(const char* name) {
  if (!MixArchive::Cache(name)) {
    LOG(QFATAL) << "Cannot read " << name
                << ": the game data files are missing or damaged.";
  }
}

static void Init_Bootstrap_Mixfiles() {
  const int temp = TheGameState().required_cd();
  TheGameState().required_cd() = -2;

  if constexpr (config::kWolapiEnabled) {
    GameFile fileWolapiMix("WOLAPI.MIX");
    if (fileWolapiMix.IsAvailable()) {
      MixArchive::Register("WOLAPI.MIX", &TheAssets().mix_key());
      MixArchive::Cache("WOLAPI.MIX");
    }
  }

  GameFile file2("EXPAND2.MIX");
  if (file2.IsAvailable()) {
    MixArchive::Register("EXPAND2.MIX", &TheAssets().mix_key());
    Cache_Or_Exit("EXPAND2.MIX");

    MixArchive::Register("HIRES1.MIX", &TheAssets().mix_key());
    Cache_Or_Exit("HIRES1.MIX");
  }

  GameFile file("EXPAND.MIX");
  if (file.IsAvailable()) {
    MixArchive::Register("EXPAND.MIX", &TheAssets().mix_key());
    Cache_Or_Exit("EXPAND.MIX");
  }

  MixArchive::Register("REDALERT.MIX", &TheAssets().mix_key());

  /*
  **	Bootstrap enough of the system so that the error dialog box can
  *successfully *	be displayed.
  */
  MixArchive::Register("LOCAL.MIX", &TheAssets().mix_key());  // Cached.
  Cache_Or_Exit("LOCAL.MIX");

  MixArchive::Register("HIRES.MIX", &TheAssets().mix_key());
  Cache_Or_Exit("HIRES.MIX");

  MixArchive::Register(
      "NCHIRES.MIX",
      &TheAssets().mix_key());  // Non-cached hires stuff incl VQ palettes

  TheGameState().required_cd() = temp;
}

/***********************************************************************************************
 * Init_Secondary_Mixfiles -- Register and cache secondary mixfiles. *
 *                                                                                             *
 *    This routine is used to register the mixfiles that are needed for main
 *menu processing.  * Call this routine before the main menu is display and
 *processed.                         *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/03/1996 JLB : Created. *
 *=============================================================================================*/
// #define DENZIL_MIXEXTRACT
static void Extract(const char* filename, const char* outname);

static void Init_Secondary_Mixfiles() {
  Assets::DiscArchives& archives = TheAssets().disc_archives();

  if (GameFile("MAIN1.MIX").IsAvailable()) {
    // MAIN1-4 from steam

    // extract the extra missions from the expansion "discs"
    // (they don't contain the base missions)
    if (GameFile("MAIN3.MIX").IsAvailable() &&
        !GameFile("GENERAL3.MIX").IsAvailable()) {
      const MixArchive* tmp =
          MixArchive::Register("MAIN3.MIX", &TheAssets().mix_key());
      Extract("GENERAL.MIX", "GENERAL3.MIX");
      delete tmp;
    }

    if (GameFile("MAIN4.MIX").IsAvailable() &&
        !GameFile("GENERAL4.MIX").IsAvailable()) {
      const MixArchive* tmp =
          MixArchive::Register("MAIN4.MIX", &TheAssets().mix_key());
      Extract("GENERAL.MIX", "GENERAL4.MIX");
      Extract("SCORES.MIX", "SCORES.MIX");  // also extract scores
      delete tmp;
    }

    // load the first two to get both movies
    MixArchive::Register("MAIN2.MIX", &TheAssets().mix_key());
    MixArchive::Register("MAIN1.MIX", &TheAssets().mix_key());

    // load extra missions
    MixArchive::Register("GENERAL4.MIX", &TheAssets().mix_key());
    MixArchive::Register("GENERAL3.MIX", &TheAssets().mix_key());
  } else {
    // assume regular/TFD files
    archives.main = MixArchive::Register("MAIN.MIX", &TheAssets().mix_key());
    DCHECK(archives.main != nullptr);
  }

// Denzil extract mixfile
#ifdef DENZIL_MIXEXTRACT
  Extract("CONQUER.MIX", "o:\\projects\\radvd\\data\\extract\\conquer.mix");
  Extract("EDHI.MIX", "o:\\projects\\radvd\\data\\extract\\edhi.mix");
  Extract("EDLO.MIX", "o:\\projects\\radvd\\data\\extract\\edlo.mix");
  Extract("GENERAL.MIX", "o:\\projects\\radvd\\data\\extract\\general.mix");
  Extract("INTERIOR.MIX", "o:\\projects\\radvd\\data\\extract\\interior.mix");
  Extract("MOVIES2.MIX", "o:\\projects\\radvd\\data\\extract\\movies2.mix");
  Extract("SCORES.MIX", "o:\\projects\\radvd\\data\\extract\\scores.mix");
  Extract("SNOW.MIX", "o:\\projects\\radvd\\data\\extract\\snow.mix");
  Extract("SOUNDS.MIX", "o:\\projects\\radvd\\data\\extract\\sounds.mix");
  Extract("RUSSIAN.MIX", "o:\\projects\\radvd\\data\\extract\\russian.mix");
  Extract("ALLIES.MIX", "o:\\projects\\radvd\\data\\extract\\allies.mix");
  Extract("TEMPERAT.MIX", "o:\\projects\\radvd\\data\\extract\\temperat.mix");
#endif

  /*
  **	Inform the file system of the various MIX files.
  */
  MixArchive::Register("CONQUER.MIX", &TheAssets().mix_key());  // Cached.
  //	MixArchive::Register("TRANSIT.MIX", &TheAssets().mix_key());

  if (archives.general == nullptr) {
    archives.general = MixArchive::Register(
        "GENERAL.MIX", &TheAssets().mix_key());  // Never cached.
  }

  if (GameFile("MOVIES1.MIX").IsAvailable()) {
    archives.movies = MixArchive::Register(
        "MOVIES1.MIX", &TheAssets().mix_key());  // Never cached.
  }
  // load both sets of movies if possible
  if (GameFile("MOVIES2.MIX").IsAvailable()) {
    archives.movies = MixArchive::Register(
        "MOVIES2.MIX", &TheAssets().mix_key());  // Never cached.
  }
  DCHECK(archives.movies != nullptr);

  /*
  **	Register the score mixfile.
  */
  TheGameState().scores_present() = true;
  archives.score = MixArchive::Register("SCORES.MIX", &TheAssets().mix_key());
  ThemeClass::Scan();

  /*
  **	These are sound card specific, but the install program would have
  **	copied the correct versions to the hard drive.
  */
  MixArchive::Register("SPEECH.MIX", &TheAssets().mix_key());   // Never cached.
  MixArchive::Register("SOUNDS.MIX", &TheAssets().mix_key());   // Cached.
  MixArchive::Register("RUSSIAN.MIX", &TheAssets().mix_key());  // Cached.
  MixArchive::Register("ALLIES.MIX", &TheAssets().mix_key());   // Cached.
}

/***********************************************************************************************
 * Bootstrap -- Perform the initial bootstrap procedure. *
 *                                                                                             *
 *    This routine will load and initialize the game engine such that a dialog
 *box could be    * displayed. Because this is very critical, call this routine
 *before any other game        * initialization code. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/03/1996 JLB : Created. *
 *=============================================================================================*/
static void Bootstrap() {
  ThePalettes().black_palette().Set();

  /*
  **	Be sure to short circuit the CD-ROM check if there is a CD-ROM override
  **	path.
  */
  if (SearchPaths::HasAny()) {
    TheGameState().required_cd() = -2;
  }

  /*
  ** Process the message loop until we are in focus. We need to be in focus to
  *read pixels from
  ** the screen.
  */
  do {
    TheKeyboard().Check();
  } while (!TheGameState().in_focus());
  AllSurfaces.SurfacesRestored = false;

  /*
  **	Register and make resident all local mixfiles with particular emphasis
  **	on the mixfiles that are necessary to display and error messages and
  **	process further initialization.
  */
  Init_Bootstrap_Mixfiles();

  /*
  **	Initialize the resident font pointers.
  */
  Init_Fonts();

  /*
  **	Setup the keyboard processor in preparation for the game.
  */
  TheKeyboard().Clear();

  /*
  **	This is the shape staging buffer. It must always be available, so it is
  **	allocated here and never freed. The library sets the globals ShapeBuffer
  **	and ShapeBufferSize to these values, so it can be accessed for other
  **	purposes.
  */
  shape_storage.resize(kShapeBufferSize);
  Set_Shape_Buffer(shape_storage);

  TheAssets().LoadStrings();

  /*
  **	Default palette initialization.
  */
  const auto palette_data = MixArchive::RetrieveData("TEMPERAT.PAL");
  if (palette_data.empty()) {
    // Missing data files are a user setup problem, not a bug: report and exit
    // quietly rather than abort with a stack trace.
    LOG(QFATAL) << "Cannot find TEMPERAT.PAL: game data files not found. "
                   "Specify the data directory with -CD<path>, for example "
                   "-CD\"<Steam library>/steamapps/common/Command & Conquer "
                   "Red Alert\"";
  }
  base::CopyBytes(std::as_writable_bytes(ThePalettes().game_palette().bytes()),
                  palette_data, 768);
  ThePalettes().white_palette().at(0) = ThePalettes().black_palette().at(0);
  //	GamePalette.Set();

  /*
  **	Initialize expansion files (if present). Expansion files must be located
  **	in the current directory.
  */
  Init_Expansion_Files();

  SidebarScheme.Background = kBlack;
  SidebarScheme.Corners = kLtGrey;
  SidebarScheme.Shadow = DKGREY;
  SidebarScheme.Highlight = kWhite;
  SidebarScheme.Color = kLtGrey;
  SidebarScheme.Bright = kWhite;
  SidebarScheme.BrightColor = kWhite;
  SidebarScheme.Box = kLtGrey;
  GadgetClass::Set_Color_Scheme(&SidebarScheme);
}

/***********************************************************************************************
 * Init_Mouse -- Initialize the mouse system. *
 *                                                                                             *
 *    This routine will ensure that a valid mouse driver is present and a
 *working mouse        * pointer can be displayed. The mouse is hidden when this
 *routine exits.                   *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/03/1996 JLB : Created. *
 *=============================================================================================*/
static void Init_Mouse() {
  /*
  ** Since there is no mouse shape currently available we need
  ** to set one of our own.
  */
  if (TheMouse() != nullptr) {
    const auto temp_mouse_shapes = MixArchive::RetrieveData("MOUSE.SHP");
    if (!temp_mouse_shapes.empty()) {
      Set_Mouse_Cursor(0, 0, Extract_Shape(temp_mouse_shapes, 0));
      while (Get_Mouse_State() > 1) {
        Show_Mouse();
      }
    }
  } else {
    ThePalettes().game_palette().Set();
    ThePalettes().game_palette().Set();
    TheScreen().visible_page().Clear();
    WWMessageBox().Process(kLanguageText.no_mouse, TXT_OK);
    // Prog_End();
    EmergencyExit(1);
  }

  TheMap().Set_Default_Mouse(MOUSE_NORMAL, false);
  Show_Mouse();
  while (Get_Mouse_State() > 1) {
    Show_Mouse();
  }
  ServiceRealTime();
  Hide_Mouse();
}

/***********************************************************************************************
 * Init_Bulk_Data -- Initialize the time-consuming mixfile caching. *
 *                                                                                             *
 *    This routine is called to handle the time consuming process of game
 *initialization.      * The title page will be displayed when this routine is
 *called.                            *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   This routine will take a very long time. *
 *                                                                                             *
 * HISTORY: * 06/03/1996 JLB : Created. *
 *=============================================================================================*/
static void Init_Bulk_Data() {
  /*
  **	Cache the main game data. This operation can take a very long time.
  */
  MixArchive::Cache("CONQUER.MIX");
  if (TheAudio().is_open() && !TheDebugState().quiet()) {
    MixArchive::Cache("SOUNDS.MIX");
    MixArchive::Cache("RUSSIAN.MIX");
    MixArchive::Cache("ALLIES.MIX");
  }
  ServiceRealTime();

  /*
  **	Fetch the tutorial message data.
  */
  TheAssets().LoadTutorialText();

  /*
  **	Perform one-time game system initializations.
  */
  Init_One_Time_Systems();
}

/***********************************************************************************************
 * Init_Keys -- Initialize the cryptographic keys. *
 *                                                                                             *
 *    This routine will initialize the fast cryptographic key. It will also
 *initialize the     * slow one if this is a scenario editor version of the
 *game.                               *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/08/1996 JLB : Created. *
 *=============================================================================================*/
static void Init_Keys() {
  std::string keys = GetKeys();
  MemoryFile file(std::as_writable_bytes(std::span(keys)));
  INIClass ini;
  ini.Load(file);

  TheAssets().set_mix_key(ini.Get_PKey(true));
}

/***************************************************************************
 * Save_Recording_Values -- Saves multiplayer-specific values              *
 *                                                                         *
 * This routine saves multiplayer values that need to be restored for a * save
 *game.  In addition to saving the random # seed for this scenario, 	* it
 * saves the contents of the actual random number generator; this 	*
 * ensures that the random # sequencer will pick up where it left off when
 ** the game was saved.
 ** This routine also saves the header for a Recording file, so it must
 ** save some data not needed specifically by a save-game file (ie
 * Seed).
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		file		file to save to
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		true = success, false = failure
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   09/28/1995 BRR : Created.                                             *
 *=========================================================================*/
template <class Archive>
static void SerializeRecording(Archive& ar) {
  ar.Section(FourCC("RARC"));
  int32_t version = kSaveGameVersion;
  ar(version);
  if constexpr (Archive::kIsReading) {
    if (!ar.ok() || version != kSaveGameVersion) {
      ar.Fail("unsupported recording version");
      return;
    }
  }
  ar(TheSession());
  TheSession().SerializePlayers(ar);
  // The switch keeps its place in the record, so a local stands in for it.
  bool unshroud = TheDebugState().unshroud();
  ar(TheWorld().build_level(), unshroud, TheWorld().seed(),
     TheScenario().Scenario, TheScenario().ScenarioName, TheWorld().whom(),
     TheSpecial(), TheOptions());
  if constexpr (Archive::kIsReading) {
    TheDebugState().set_unshroud(unshroud);
    TheScenario().ScenarioName[sizeof(TheScenario().ScenarioName) - 1] = '\0';
  }
}

bool Save_Recording_Values(GameFile& file) {
  FileSink pipe(file);
  ArchiveWriter writer(pipe);
  SerializeRecording(writer);
  return true;
}

/***************************************************************************
 * Load_Recording_Values -- Loads multiplayer-specific values              *
 *                                                                         *
 * INPUT:                                                                  *
 *		file			file to load from
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		true = success, false = failure
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   09/28/1995 BRR : Created.                                             *
 *=========================================================================*/
bool Load_Recording_Values(GameFile& file) {
  FileSource straw(file);
  ArchiveReader reader(straw);
  SerializeRecording(reader);
  return reader.ok();
}

void Extract(const char* filename, const char* outname) {
  GameFile inFile(filename);
  GameFile outFile(outname);

  inFile.Open();
  outFile.Open(FileAccess::kWrite);

  std::array<char, 32768> buffer{};

  int64_t size = inFile.Size();

  while (size > 0) {
    const base::ssize bytes = inFile.Read(std::span(buffer), 32768);
    if (bytes <= 0) {
      break;
    }
    outFile.Write(std::span(buffer), bytes);
    size -= bytes;
  }
}

static bool bUsingDVD = false;

//***********************************************************************************************
// Whether the installer recorded a DVD edition. Off Windows there is no
// installer, and the disc logic treats the data on disk as the DVD.
static bool Is_DVD_Installed() {
  if constexpr (port::kIsWindows) {
    return ReadInstallerFlag("DVD");
  } else {
    return true;
  }
}

//***********************************************************************************************
bool Determine_If_Using_DVD() {
  //	Determines if the user has a DVD currently available. If they do, we'll
  // use it throughout the 	session. Else we won't check for it again and
  // will always ask for CDs.
  if (Is_DVD_Installed()) {
    // User hit cancel. Allow things to progress normally. They will be
    // prompted for a Red Alert disk as usual.
    bUsingDVD = Force_CD_Available(5);
  } else {
    bUsingDVD = false;
  }

  return bUsingDVD;
}

//***********************************************************************************************
bool Using_DVD() { return bUsingDVD; }
