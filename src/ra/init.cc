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
 *   Load_Title_Page -- Load the background art for the title page. * Obfuscate
 *-- Sufficiently transform parameter to thwart casual hackers. *
 *   Parse_Command_Line -- Parses the command line parameters. * Parse_INI_File
 *-- Parses CONQUER.INI for special options                                  *
 *   Play_Intro -- plays the introduction & logo movies * Save_Recording_Values
 *-- Saves recording values to a recording file                       *
 *   Select_Game -- The game's main menu * Load_Prolog_Page -- Loads the special
 *pre-prolog "please wait" page.                      *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */
#include "ra/init.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/log/log.h"
#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/types.h"
#include "magic_enum/magic_enum.hpp"
#include "port/ex_string.h"
#include "port/platform.h"
#include "port/random_seed.h"
#include "port/safe_string.h"
#include "port/tokenizer.h"
#include "ra/_wsproto.h"
#include "ra/ccini.h"
#include "ra/compat.h"
#include "ra/config.h"
#include "ra/conquer.h"
#include "ra/const.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/externs.h"
#include "ra/gadget.h"
#include "ra/globals.h"
#include "ra/goptions.h"
#include "ra/graphics_loader.h"
#include "ra/heap.h"
#include "ra/house.h"
#include "ra/ini.h"
#include "ra/inline.h"
#include "ra/installation.h"
#include "ra/intro.h"
#include "ra/ipx.h"
#include "ra/ipxaddr.h"
#include "ra/ipxmgr.h"
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
#include "ra/nulldlg.h"
#include "ra/nullmgr.h"
#include "ra/palette.h"
#include "ra/queue.h"
#include "ra/rules.h"
#include "ra/saveload.h"
#include "ra/scenario.h"
#include "ra/session.h"
#include "ra/special.h"
#include "ra/startup.h"
#include "ra/text_ids.h"
#include "ra/theme.h"
#include "ra/type.h"
#include "ra/wsproto.h"
#include "ra/wspudp.h"
#include "sdllib/file.h"
#include "sdllib/file_access.h"
#include "sdllib/font.h"
#include "sdllib/gbuffer.h"
#include "sdllib/iff.h"
#include "sdllib/misc.h"
#include "sdllib/shape.h"
#include "sdllib/ww_audio.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "tech/archive.h"
#include "tech/buff.h"
#include "tech/crc.h"
#include "tech/file_sink.h"
#include "tech/file_source.h"
#include "tech/fixed.h"
#include "tech/ftimer.h"
#include "tech/game_file.h"
#include "tech/memory_file.h"
#include "tech/mix_archive.h"
#include "tech/mpu.h"
#include "tech/number_parse.h"
#include "tech/pk.h"
#include "tech/random.h"
#include "tech/rgb.h"
#include "tech/search_paths.h"
#include "winvq/vqa32/vqaplay.h"

static RemapControlType SidebarScheme;

/****************************************
**	Function prototypes for this module **
*****************************************/
static void Play_Intro(bool sequenced = false);
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
#include "ra/wolapiob.h"

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
  Load_Title_Screen("PROLOG.PCX", &HidPage, CCPalette);
  HidPage.Blit(SeenBuff);
  CCPalette.Set();
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
bool Init_Game(int /*unused*/, char* /*unused*/[]) {
  /*
  **	Allocate the benchmark tracking objects only if the machine and
  **	compile flags indicate.
  */
  if constexpr (config::kCheatKeysEnabled) {
    if (Processor() >= 2) {
      Benches.resize(magic_enum::enum_count<BenchType>());
    }
  }

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

  if (Special.IsFromInstall) {
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
  HouseTypes.Set_Heap(magic_enum::enum_count<HousesType>());
  BuildingTypes.Set_Heap(magic_enum::enum_count<StructType>());
  AircraftTypes.Set_Heap(magic_enum::enum_count<AircraftType>());
  InfantryTypes.Set_Heap(magic_enum::enum_count<InfantryType>());
  BulletTypes.Set_Heap(magic_enum::enum_count<BulletType>());
  AnimTypes.Set_Heap(magic_enum::enum_count<AnimType>());
  UnitTypes.Set_Heap(magic_enum::enum_count<UnitType>());
  VesselTypes.Set_Heap(magic_enum::enum_count<VesselType>());
  TemplateTypes.Set_Heap(magic_enum::enum_count<TemplateType>());
  TerrainTypes.Set_Heap(magic_enum::enum_count<TerrainType>());
  OverlayTypes.Set_Heap(magic_enum::enum_count<OverlayType>());
  SmudgeTypes.Set_Heap(magic_enum::enum_count<SmudgeType>());

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
  if (RuleINI.Load(fc, false)) {
    Rule.Process(RuleINI);
  }
  //  Aftermath runtime change 9/29/98
  //	This is safe to do, as only rules for aftermath units are included in
  // this ini.
  if (Is_Aftermath_Installed()) {
    GameFile aftermath_ini("AFTRMATH.INI");
    if (AftermathINI.Load(aftermath_ini, false)) {
      Rule.Process(AftermathINI);
    }
  }

  Session.MaxPlayers = Rule.MaxPlayers;

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
  if (!Special.IsFromInstall) {
    VisiblePage.Clear();
    Play_Intro();
    base::FillBytes(
        std::as_writable_bytes(PaletteClass::CurrentPalette.bytes()), 0x01,
        768);
    WhitePalette.Set();
  } else {
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
  Set_Logic_Page(SeenBuff);

  /*
  **	If not automatically launching into the intro, then display the title
  **	page while the bulk data is cached.
  */
  if (!Special.IsFromInstall) {
    Load_Title_Page(true);

    Hide_Mouse();
    Fancy_Text_Print(TXT_STAND_BY, 320, 240,
                     &ColorRemaps.at(PCOLOR_DIALOG_BLUE), kTBlack,
                     TPF_CENTER | kTpfText | TPF_DROPSHADOW);
    Show_Mouse();

    CCPalette.Set(kFadePaletteSlow);
    Call_Back();
  }

  /*
  **	Initialize the bulk data. This takes the longest time and must be
  *performed once *	before the regular game starts.
  */
  Init_Bulk_Data();

  /*
  **	Initialize the multiplayer score values
  */
  Session.GamesPlayed = 0;
  Session.NumScores = 0;
  Session.CurGame = 0;
  for (auto& i : Session.Score) {
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
  GamePalette = CCPalette;
  OriginalPalette = CCPalette;

  /*
  **	Read game options, so the GameSpeed is initialized when multiplayer
  ** dialogs are invoked.  (GameSpeed must be synchronized between systems.)
  */
  Options.Load_Settings();

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

  // A -QUITFRAME run has ended when the game it started (-NEWGAME or
  // -LOADGAME, both consumed on use, or recording playback) brings control
  // back here. Leave rather
  // than wait at the menu for input that never comes.
  if (DebugQuitAtFrame >= 0 && DebugNewGame.empty() && DebugLoadGame < 0 &&
      !Session.Play) {
    return false;
  }

  const int cdcheck = 0;

  Show_Mouse();

  NewUnitsEnabled = SecretUnitsEnabled =
      false;  // Assume new units disabled, unless specifically .INI enabled or
              // multiplayer negotiations enable it.

  /*
  **	[Re]set any globals that need it, in preparation for a new scenario
  */
  GameActive = true;
  DoList.Init();
  OutList.Init();
  Frame = 0;
  Scen.MissionTimer.Set(0);
  Scen.MissionTimer.Stop();
  Scen.CDifficulty = DIFF_NORMAL;
  Scen.Difficulty = DIFF_NORMAL;
  PlayerWins = false;
  PlayerLoses = false;
  Session.ObiWan = false;
  Debug_Unshroud = false;
  Map.Set_Cursor_Shape({});
  Map.PendingObjectPtr = nullptr;
  Map.PendingObject = nullptr;
  Map.PendingHouse = HOUSE_NONE;

  Session.ProcessTicks = 0;
  Session.ProcessFrames = 0;
  Session.DesiredFrameRate = 30;
  NewMaxAheadFrame1 = 0;
  NewMaxAheadFrame2 = 0;

  /*
  **	Init multiplayer game scores.  Let Wins accumulate; just init the
  *current
  ** Kills for this game.  Kills of -1 means this player didn't play this round.
  */
  for (int i = 0; i < MAX_MULTI_GAMES; i++) {
    base::At(base::At(Session.Score, i).Kills, Session.CurGame) = -1;
  }

  /*
  **	Set default mouse shape
  */
  Map.Set_Default_Mouse(MOUSE_NORMAL, false);

  /*
  **	If the last game we played was a multiplayer game, jump right to that
  **	menu by pre-setting 'selection'.
  */
  if (Session.Type == GAME_NORMAL) {
    selection = kSelNone;
  } else {
    selection = kSelMultiplayerGame;
  }

  /*
  **	Main menu processing; only do this if we're not in editor mode.
  */
  if (!MapEditorActive) {
    /*
    **	Menu selection processing loop
    */
    Theme.Queue_Song(THEME_CRUS);

    /*
    ** If we're playing back a recording, load all pertinent values & skip
    ** the menu loop.  Hide the now-useless mouse pointer.
    */
    if (Session.Play && Session.RecordFile.IsAvailable()) {
      if (Session.RecordFile.Open(FileAccess::kRead)) {
        if (Load_Recording_Values(Session.RecordFile)) {
          process = false;
          Theme.Fade_Out();
        } else {
          Session.RecordFile.Close();
          Session.Play = false;
        }
      } else {
        Session.Play = false;
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
        GamePalette = CCPalette;

        HidPage.Blit(SeenBuff);
        //				if (fade) {
        //					WhitePalette.Set();
        //					CCPalette.Set(kFadePaletteSlow,
        // Call_Back); 					fade = false;
        // } else {
        CCPalette.Set();
        //				}

        Set_Logic_Page(SeenBuff);
        display = false;
        Show_Mouse();
      }

      /*
      **	Display menu and fetch selection from player.
      */
      if (Special.IsFromInstall) {
        selection = kSelStartNewGame;
      }

      if (config::kWolapiEnabled && pWolapi != nullptr) {
        selection = kSelMultiplayerGame;  //	We are returning from a game.
      }

      // -NEWGAME<scenario>: skip the menu and start that scenario as a
      // normal-difficulty campaign game, e.g. -NEWGAMESCG01EA. Used with
      // -QUITFRAME and -SAVESLOT to produce a reference save without a
      // display.
      if (selection == kSelNone && !DebugNewGame.empty()) {
        Scen.CDifficulty = DIFF_NORMAL;
        Scen.Difficulty = DIFF_NORMAL;
        Scen.CarryOverMoney = 0;
        BuildLevel = 10;
        IsTanyaDead = false;
        SaveTanya = false;
        Whom = HOUSE_GOOD;
        Scen.Set_Scenario_Name((DebugNewGame + ".INI").c_str());
        DebugNewGame.clear();
        Session.Type = GAME_NORMAL;
        process = false;
        continue;
      }

      // -LOADGAME<n>: skip the menu and load save slot n straight away.
      // Used with -QUITFRAME to drive save/load checks without a display.
      if (selection == kSelNone && DebugLoadGame >= 0) {
        const int slot = DebugLoadGame;
        DebugLoadGame = -1;
        if (Load_Game(slot)) {
          Theme.Queue_Song(magic_enum::enum_values<ThemeType>().front());
          process = false;
          gameloaded = true;
          continue;
        }
        LOG(ERROR) << "-LOADGAME: could not load slot " << slot;
      }

      if (selection == kSelNone) {
        AntsEnabled = false;
        selection = Main_Menu(ATTRACT_MODE_TIMEOUT);
      }
      Call_Back();

      switch (selection) {
        /*
        **	Pick an expansion scenario.
        */
        case kSelNewScenarioCs:
        case kSelNewScenarioAm:
          Scen.CarryOverMoney = 0;
          IsTanyaDead = false;
          SaveTanya = false;

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
              Scen.CDifficulty = DIFF_HARD;
              Scen.Difficulty = DIFF_EASY;
              break;

            case 1:
              Scen.CDifficulty = DIFF_HARD;
              Scen.Difficulty = DIFF_NORMAL;
              break;

            case 2:
              Scen.CDifficulty = DIFF_NORMAL;
              Scen.Difficulty = DIFF_NORMAL;
              break;

            case 3:
              Scen.CDifficulty = DIFF_EASY;
              Scen.Difficulty = DIFF_NORMAL;
              break;

            case 4:
              Scen.CDifficulty = DIFF_EASY;
              Scen.Difficulty = DIFF_HARD;
              break;
            default:
              break;
          }
          DLOG(INFO) << "Difficulty: player "
                     << magic_enum::enum_name(Scen.Difficulty) << ", computer "
                     << magic_enum::enum_name(Scen.CDifficulty);

          Theme.Fade_Out();
          Theme.Queue_Song(magic_enum::enum_values<ThemeType>().front());
          Session.Type = GAME_NORMAL;
          process = false;
          break;

        /*
        **	SEL_START_NEW_GAME: Play the game
        */
        case kSelStartNewGame:
          if (Special.IsFromInstall) {
            Scen.CDifficulty = DIFF_NORMAL;
            Scen.Difficulty = DIFF_NORMAL;
          } else {
            switch (Fetch_Difficulty()) {
              case 0:
                Scen.CDifficulty = DIFF_HARD;
                Scen.Difficulty = DIFF_EASY;
                break;

              case 1:
                Scen.CDifficulty = DIFF_HARD;
                Scen.Difficulty = DIFF_NORMAL;
                break;

              case 2:
                Scen.CDifficulty = DIFF_NORMAL;
                Scen.Difficulty = DIFF_NORMAL;
                break;

              case 3:
                Scen.CDifficulty = DIFF_EASY;
                Scen.Difficulty = DIFF_NORMAL;
                break;

              case 4:
                Scen.CDifficulty = DIFF_EASY;
                Scen.Difficulty = DIFF_HARD;
                break;
              default:
                break;
            }
          }
          Scen.CarryOverMoney = 0;
          BuildLevel = 10;
          IsTanyaDead = false;
          SaveTanya = false;
          Whom = HOUSE_GOOD;

          if (!Special.IsFromInstall) {
            if (AntsEnabled) {
              Scen.Set_Scenario_Name("SCA01EA.INI");
            } else {
              switch (WWMessageBox().Process(TXT_CHOOSE, TXT_ALLIES, TXT_CANCEL,
                                             TXT_SOVIET)) {
                case 2:
                  Scen.Set_Scenario_Name("SCU01EA.INI");
                  break;
                default:
                  selection = kSelNone;
                  continue;
                case 0:
                  Scen.Set_Scenario_Name("SCG01EA.INI");
                  break;
              }
            }
            Theme.Fade_Out();
            Load_Title_Page();
          } else {
            Theme.Fade_Out();
            Choose_Side();
            Hide_Mouse();
            if (CurrentCD == 0) {
              Scen.Set_Scenario_Name("SCG01EA.INI");
            } else {
              Scen.Set_Scenario_Name("SCU01EA.INI");
            }
          }

          Session.Type = GAME_NORMAL;
          process = false;
          break;

        /*
        **	Load a saved game.
        */
        case kSelLoadMission:
          if (LoadOptionsClass(LoadOptionsClass::LOAD).Process()) {
            Theme.Queue_Song(magic_enum::enum_values<ThemeType>().front());
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
          if (!config::kWolapiEnabled || pWolapi == nullptr) {
            switch (Session.Type) {
              /*
              **	If 'Session.Type' isn't already set up for a multiplayer
              *game, *	we must prompt the user for which type of multiplayer
              *game *	they want.
              */
              case GAME_NORMAL:
                Session.Type = Select_MPlayer_Game();
                if (Session.Type == GAME_NORMAL) {  // 'Cancel'
                  display = true;
                  selection = kSelNone;
                }
                break;

              case GAME_SKIRMISH:
                if (!Com_Scenario_Dialog(true)) {
                  Session.Type = Select_MPlayer_Game();
                  if (Session.Type == GAME_NORMAL) {  // user hit Cancel
                    display = true;
                    selection = kSelNone;
                  }
                } else {
                  //	Ever hits? Session.Type set to GAME_SKIRMISH without
                  // user selecting in Select_MPlayer_Game()?
                  //	If mission is Counterstrike, CS CD will be required. But
                  // aftermath units require AM CD.
                  bAftermathMultiplayer =
                      Is_Aftermath_Installed() &&
                      !IsMissionCounterstrike(Scen.ScenarioName);
                  //	ajw I'll bet this was needed before also...
                  Session.ScenarioIsOfficial =
                      Session.Scenarios.at(Session.Options.ScenarioIndex)
                          ->Get_Official();
                }
                break;

              case GAME_NULL_MODEM:
              case GAME_MODEM:
                if (Session.Type != GAME_SKIRMISH &&
                    NullModem.Num_Connections()) {
                  NullModem.Init_Send_Queue();

                  if ((Session.Type == GAME_NULL_MODEM &&
                       Session.ModemType == MODEM_NULL_HOST) ||
                      (Session.Type == GAME_MODEM &&
                       Session.ModemType == MODEM_DIALER)) {
                    if (!Com_Scenario_Dialog()) {
                      Session.Type = Select_Serial_Dialog();
                      if (Session.Type == GAME_NORMAL) {  // user hit Cancel
                        display = true;
                        selection = kSelNone;
                      }
                    }
                  } else {
                    if (!Com_Show_Scenario_Dialog()) {
                      Session.Type = Select_Serial_Dialog();
                      if (Session.Type == GAME_NORMAL) {  // user hit Cancel
                        display = true;
                        selection = kSelNone;
                      }
                    }
                  }
                } else {
                  Session.Type = Select_MPlayer_Game();
                  if (Session.Type == GAME_NORMAL) {  // 'Cancel'
                    display = true;
                    selection = kSelNone;
                  }
                }
                break;

              // Back from an Internet game: prompt again, like GAME_NORMAL.
              case GAME_INTERNET:
                Session.Type = Select_MPlayer_Game();
                if (Session.Type == GAME_NORMAL) {  // 'Cancel'
                  display = true;
                  selection = kSelNone;
                }
                break;
              case GameType::GAME_IPX:
              default:
                break;
            }
          }  //	if( !pWolapi )

          if (config::kWolapiEnabled && pWolapi != nullptr) {
            Session.Type = GAME_INTERNET;
          }
          // debugprint( "Session.Type = %i\n", Session.Type );
          switch (Session.Type) {
            /*
            **	Modem, Null-Modem or internet
            */
            case GAME_MODEM:
            case GAME_NULL_MODEM:
            case GAME_SKIRMISH:
              Theme.Fade_Out();
              process = false;
              Options.ScoreVolume = Options.MultiScoreVolume;
              break;

            //	With Westwood Online on, this runs the whole lobby; without it,
            //	the game was already set up above and this behaves like the
            //	cases just before.
            case GAME_INTERNET:
              if constexpr (config::kWolapiEnabled) {
                delete PacketTransport;
                PacketTransport = new UDPInterfaceClass;
                assert(PacketTransport != nullptr);
                if (PacketTransport->Init()) {
                  switch (WOL_Main()) {
                    case 1:
                      //	Start game.
                      Options.ScoreVolume = Options.MultiScoreVolume;
                      process = false;
                      Theme.Fade_Out();
                      break;
                    case 0:
                      //	User cancelled.
                      Session.Type = GAME_NORMAL;
                      display = true;
                      selection = kSelMultiplayerGame;  // SEL_NONE;
                      delete PacketTransport;
                      PacketTransport = nullptr;
                      break;
                    case -1:
                      //	Patch was downloaded. Exit app.
                      Theme.Fade_Out();
                      BlackPalette.Set(kFadePaletteSlow);
                      return false;
                    default:
                      break;
                  }
                } else {
                  Session.Type = GAME_NORMAL;
                  display = true;
                  selection = kSelMultiplayerGame;  // SEL_NONE;
                  delete PacketTransport;
                  PacketTransport = nullptr;
                }
              } else {
                Theme.Fade_Out();
                process = false;
                Options.ScoreVolume = Options.MultiScoreVolume;
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
              delete PacketTransport;
              // we don't even have IPX
              PacketTransport = new UDPInterfaceClass;
              PacketTransport->Set_Broadcast_Address("255.255.255.255");
              WWDebugString("RA95 - About to call Init_Network.\n");
              if (Session.Type == GAME_IPX && Init_Network() &&
                  Remote_Connect()) {
                Options.ScoreVolume = Options.MultiScoreVolume;
                process = false;
                Theme.Fade_Out();
              } else {  // user hit cancel, or init failed
                Session.Type = GAME_NORMAL;
                display = true;
                selection = kSelNone;
                delete PacketTransport;
                PacketTransport = nullptr;
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
          Theme.Fade_Out();
          if (Debug_Flag) {
            Play_Intro(Debug_Flag);
          } else {
            Hide_Mouse();
            VisiblePage.Clear();
            Show_Mouse();
            Play_Movie(VQ_INTRO_MOVIE, THEME_NONE,
                       true);  // no transition picture to briefing
            Keyboard->Clear();
            Play_Movie(VQ_SIZZLE, THEME_NONE, true);
            Play_Movie(VQ_SIZZLE2, THEME_NONE, true);
            //						Play_Movie(VQ_INTRO_MOVIE,
            // THEME_NONE, false);		// has transitino picture to
            // briefing
          }
          Theme.Queue_Song(THEME_CRUS);
          display = true;
          selection = kSelNone;
          break;

        /*
        **	Exit to DOS.
        */
        case kSelExit:
          Theme.Fade_Out();
          BlackPalette.Set(kFadePaletteSlow);
          return false;

        /*
        **	Display the hall of fame.
        */
        case kSelFame:
          break;

        case kSelTimeout:
          if (Session.Attract && Session.RecordFile.IsAvailable()) {
            Session.Play = true;
            if (Session.RecordFile.Open(FileAccess::kRead)) {
              if (Load_Recording_Values(Session.RecordFile)) {
                process = false;
                Theme.Fade_Out();
              } else {
                Session.RecordFile.Close();
                Session.Play = false;
                selection = kSelNone;
              }
            } else {
              Session.Play = false;
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
    ** For MapEditorActive (editor) mode to load scenario
    */
    Scen.Set_Scenario_Name("SCG01EA.INI");
  }

  /*
  **	Don't carry stray keystrokes into game.
  */
  Keyboard->Clear();

  /*
  ** Initialize the random number generator(s)
  */
  Init_Random();

  /*
  ** Save initialization values if we're recording this game.
  */
  if (Session.Record) {
    if (Session.RecordFile.Open(FileAccess::kWrite)) {
      Save_Recording_Values(Session.RecordFile);
    } else {
      Session.Record = false;
    }
  }

  switch (Session.Type) {
    case GAME_MODEM:
    case GAME_NULL_MODEM:
    case GAME_IPX:
      if (!bAftermathMultiplayer) {
        NewUnitsEnabled = SecretUnitsEnabled = false;
      } else {
        NewUnitsEnabled = true;
      }
      //			debugprint( "Non Internet game: NewUnitsEnabled
      //= %i\n", NewUnitsEnabled );
      break;
    case GAME_INTERNET:
      if (!config::kWolapiEnabled || pWolapi == nullptr) {
        //				debugprint( "pWolapi is null on internet
        // game!" );
        Fatal("pWolapi is null on internet game!");
      }
      // if( pWolapi->bEnableNewAftermathUnits )
      if (bAftermathMultiplayer) {
        NewUnitsEnabled = true;
      } else {
        NewUnitsEnabled = SecretUnitsEnabled = false;
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
  if (!gameloaded && !Session.LoadGame) {
    //		if (MapEditorActive) {
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
      BlackPalette.Set(kFadePaletteMedium, Call_Back);
      HiddenPage.Clear();
      VisiblePage.Clear();
    }
    Show_Mouse();
    if (!Start_Scenario(Scen.ScenarioName)) {
      return false;
    }
    if (Special.IsFromInstall) {
      Show_Mouse();
    }
    Special.IsFromInstall = false;
  }

  /*
  **	For multiplayer games, initialize the inter-player message system.
  **	Do this after loading the scenario, so the map's upper-left corner is
  **	properly set.
  */
  Session.Messages.Init(
      Map.TacPixelX, Map.TacPixelY,  // x,y for messages
      6,                             // max # msgs
      MAX_MESSAGE_LENGTH - 14,       // max msg length
      14,                            // font height in pixels
      -1, -1,                        // x,y for edit line (appears above msgs)
      0,                             // BG		1,
                                     // // enable edit overflow
      20,                            // min,
      MAX_MESSAGE_LENGTH - 14,       //    max for trimming overflow
      Lepton_To_Pixel(Map.TacLeptonWidth));  // Width in pixels of buffer

  if (Session.Type != GAME_NORMAL && Session.Type != GAME_SKIRMISH &&
      !Session.Play) {
    Session.Create_Connections();
  }

  /*
  ** If this isnt an internet game that set the unit build rate to its default
  *value
  */
  if (Session.Type != GAME_INTERNET) {
    UnitBuildPenalty = 100;
  }

  /*
  **	Hide the SeenBuff; force the map to render one frame.  The caller can
  **	then fade the palette in.
  **	(If we loaded a game, this step will fade out the title screen.  If we
  **	started a scenario, Start_Scenario() will have played a couple of VQ
  **	movies, which will have cleared the screen to black already.)
  */
  Call_Back();
  Hide_Mouse();
  BlackPalette.Set(kFadePaletteMedium, Call_Back);
  HiddenPage.Clear();
  VisiblePage.Clear();
  Show_Mouse();
  Set_Logic_Page(SeenBuff);
  /*
  ** Sidebar is always active in hi-res.
  */
  if (!MapEditorActive) {
    Map.Activate(1);
  }
  Map.Flag_To_Redraw();
  Call_Back();
  Map.Render();

  return true;
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

  Keyboard->Clear();
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
    VisiblePage.Clear();
    Show_Mouse();
    Play_Movie(static_cast<VQType>(_counter--), THEME_NONE);

    //		Show_Mouse();
  } else {
    Hide_Mouse();
    VisiblePage.Clear();
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
GraphicBufferClass VQ640(640, 400, {});
void Anim_Init() {
  /* Configure player with INI file */
  VQA_DefaultConfig(&AnimControl);
  AnimControl.DrawFlags = VQACFGF_TOPLEFT;
  AnimControl.DrawFlags |= VQACFGF_BUFFER;
  // AnimControl.DrawFlags |= VQACFGF_NODRAW;
  // BG - M. Grayford says turn this off
  // AnimControl.DrawFlags |= VQACFGF_NOSKIP;

  AnimControl.DrawFlags |= VQACFGF_NOSKIP;
  AnimControl.FrameRate = -1;
  AnimControl.DrawRate = -1;
  AnimControl.DrawerCallback = VQ_Call_Back;
  AnimControl.EventHandler = VQ_Event_Handler;
  AnimControl.ImageWidth = 320;
  AnimControl.ImageHeight = 200;
  AnimControl.ImageBuf = SysMemPage.Get_Bytes();
  if (IsVQ640) {
    AnimControl.ImageWidth = 640;
    AnimControl.ImageHeight = 400;
    AnimControl.ImageBuf = VQ640.Get_Bytes();
  }
  AnimControl.Vmode = 0;
  AnimControl.OptionFlags |= VQAOPTF_CAPTIONS | VQAOPTF_EVA;
  if (SlowPalette) {
    AnimControl.OptionFlags |= VQAOPTF_SLOWPAL;
  }
  AnimControl.AudioDeviceID = Get_Audio_Device();
  AnimControl.AudioCallback = Get_Audio_Callback_Ptr();
  AnimControl.AudioSpec = Get_Audio_Spec();
}

/***********************************************************************************************
 * Parse_Command_Line -- Parses the command line parameters. *
 *                                                                                             *
 *    This routine should be called before the graphic mode is initialized. It
 *examines the    * command line parameters and sets the appropriate globals. If
 *there is an error, then     * it outputs a command summary and then returns
 *false.                                     *
 *                                                                                             *
 * INPUT:   argc  -- The number of command line arguments. *
 *                                                                                             *
 *          argv  -- Pointer to character string array that holds the individual
 *arguments.    *
 *                                                                                             *
 * OUTPUT:  bool; Was the command line parsed successfully? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/18/1995 JLB : Created. *
 *=============================================================================================*/
bool Parse_Command_Line(std::span<char*> arguments) {
  /*
  **	Parse the command line and set globals to reflect the parameters
  **	passed in.
  */
  Whom = HOUSE_GOOD;
  Special.Init();

  MapEditorActive = false;
  Debug_Unshroud = false;

  for (auto* const argument :
       arguments.subspan(std::min(size_t{1}, arguments.size()))) {
    const std::string original_arg = argument;  // Copy for preserving case.
    const std::string_view string = strupr(argument);

    /*
    **	Print usage text only if requested.
    */
    if (port::CompareIgnoreCase("/?", string) == 0 ||
        port::CompareIgnoreCase("-?", string) == 0 ||
        port::CompareIgnoreCase("-h", string) == 0 ||
        port::CompareIgnoreCase("/h", string) == 0) {
      /*
      **	Unrecognized command line parameter... Display usage
      **	and then exit.
      */
      absl::PrintF("%s\n", kLanguageText.options);
      return false;
    }

    bool processed = true;
    const uint32_t ob = Obfuscate(argument);

    /*
    **	Check to see if the parameter is a cheat enabling one.
    */
    for (const uint32_t code : CheatCodes) {
      if (code == 0) {
        break;
      }
      if (code == ob) {
        Debug_Playtest = true;
        Debug_Flag = true;
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
        Debug_Playtest = true;
        Debug_Flag = true;
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
        MapEditorActive = true;
        Debug_Unshroud = true;
        Debug_Flag = true;
        Debug_Playtest = true;
        break;
      }
    }

    switch (ob) {
      case kParmPlaytest:
        if constexpr (config::kVirginCheatKeysEnabled) {
          Debug_Playtest = true;
        }
        break;

      /*
      ** Special flag - is C&C being run from the install program?
      */
      case kParmInstall:
        Special.IsFromInstall = true;
        // If uncommented, will disable the <ESC> key during the first movie
        // run.
        //				BreakoutAllowed = false;
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
      if (port::CompareIgnoreCase(string, "-CHECKMAP") == 0) {
        Debug_Check_Map = true;
        continue;
      }
    }

    /*
    **	File search path override.
    */
    if (string.contains("-CD")) {
      // Use original arg to preserve case-sensitive path on Unix systems
      SearchPaths::Add(original_arg.substr(3));
      continue;
    }

    /*
    **	Specify destination connection for network play
    */
    if (string.contains("-DESTNET")) {
      NetNumType net;
      NetNodeType node;

      /*
      ** Scan the command-line string, pulling off each address piece
      */
      int i = 0;
      port::Tokenizer tokens(port::MutableCString(argument).subspan(8).data(),
                             ".");
      while (const char* p = tokens.Next()) {
        const auto byte = tech::ParseHex<uint8_t>(p);
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

      /*
      ** If all the address components were successfully read, fill in the
      ** BridgeNet with a broadcast address to the network across the bridge.
      */
      if (i >= 4) {
        Session.IsBridge = 1;
        base::FillBytes(base::ObjectBytes(node), 0xff, 6);
        Session.BridgeNet = IPXAddressClass(net, node);
      }
      continue;
    }

    /*
    **	Specify socket ID, as an offset from 0x4000.
    */
    if (string.contains("-SOCKET")) {
      const auto offset = tech::ParseInteger<int>(
          string.substr(std::string_view("-SOCKET").size()));
      if (offset && *offset >= 0 && *offset < 0x4000) {
        Ipx.Set_Socket(static_cast<uint16_t>(*offset + 0x4000));
      }
      continue;
    }

    /*
    **	Set the Net Stealth option
    */
    if (string.contains("-STEALTH")) {
      Session.NetStealth = true;
      continue;
    }

    /*
    **	Set the Net Protection option
    */
    if (string.contains("-MESSAGES")) {
      Session.NetProtect = false;
      continue;
    }

    /*
    **	Allow "attract" mode
    */
    if (string.contains("-ATTRACT")) {
      Session.Attract = true;
      continue;
    }

    /*
    ** Set screen to 640x480 instead of 640x400
    */
    if (string.contains("-480")) {
      ScreenHeight = 480;
      continue;
    }

    if constexpr (config::kCheatKeysEnabled) {
      // Specify the random number seed (for debugging)
      if (string.contains("-SEED")) {
        CustomSeed = tech::ParseInteger<uint16_t>(
                         string.substr(std::string_view("-SEED").size()))
                         .value_or(CustomSeed);
        continue;
      }
    }

    if (std::string_view(string) == "-NOMOVIES") {
      bNoMovies = true;
      continue;
    }

    /*
    ** Disable mouse grabbing for debugging
    */
    // Developer switches for save-game checks; see Select_Game and
    // Main_Loop.
    if (string.starts_with("-LOADGAME")) {
      DebugLoadGame = tech::ParseInteger<int>(string.substr(9)).value_or(-1);
      continue;
    }
    if (string.starts_with("-QUITFRAME")) {
      DebugQuitAtFrame =
          tech::ParseInteger<int>(string.substr(10)).value_or(-1);
      continue;
    }
    if (string.starts_with("-NEWGAME")) {
      DebugNewGame = string.substr(8);
      continue;
    }
    if (string.starts_with("-SAVESLOT")) {
      DebugSaveSlot = tech::ParseInteger<int>(string.substr(9)).value_or(-1);
      continue;
    }

    if (string.contains("-NOMOUSEGRAB")) {
      NoMouseGrab = true;
    }

    /*
    **	Special command line control parsing.
    */
    if (absl::StartsWithIgnoreCase(string, "-X")) {
      for (const char code : string.substr(2)) {
        if constexpr (config::kCheatKeysEnabled) {
          switch (code) {
            case 'I':
              Special.IsInert = true;
              continue;
            case 'H':
              Special.IsSpeedBuild = true;
              continue;
            case 'X':
              Session.Record = true;
              continue;
            case 'Y':
              Session.Play = true;
              continue;
            case 'P':
              Debug_Print_Events = true;
              continue;
            default:
              break;
          }
        }

        if (code == 'Q') {
          Debug_Quiet = true;
        } else {
          absl::PrintF("%s\n", kLanguageText.invalid_option);
          return false;
        }
      }
    }
  }
  return true;
}

/***********************************************************************************************
 * Obfuscate -- Sufficiently transform parameter to thwart casual hackers. *
 *                                                                                             *
 *    This routine borrows from CRC and PGP technology to sufficiently alter the
 *parameter     * in order to make it difficult to reverse engineer the key
 *phrase. This is designed to    * be used for hidden game options that will be
 *released at a later time over Westwood's    * Web page or through magazine
 *hint articles.                                              *
 *                                                                                             *
 *    This algorithm is cryptographically categorized as a "one way hash". *
 *                                                                                             *
 *    Since this is a one way transformation, it becomes much more difficult to
 *reverse        * engineer the pass phrase even if the resultant pass code is
 *known. This has an added     * benefit of making this algorithm immune to
 *traditional cryptographic attacks.            *
 *                                                                                             *
 *    The largest strength of this transformation algorithm lies in the
 *restriction on the     * source vector being legal ASCII uppercase characters.
 *This restriction alone makes even  * a simple CRC transformation practically
 *impossible to reverse engineer. This algorithm   * uses far more than a simple
 *CRC transformation to achieve added strength from advanced   * attack methods.
 **
 *                                                                                             *
 * INPUT:   string   -- Pointer to the key phrase that will be transformed into
 *a code.        *
 *                                                                                             *
 * OUTPUT:  Returns with the code that the key phrase is translated into. *
 *                                                                                             *
 * WARNINGS:   A zero length pass phrase results in a 0x00000000 result code. *
 *                                                                                             *
 * HISTORY: * 08/19/1995 JLB : Created. *
 *=============================================================================================*/
uint32_t Obfuscate(const char* string) {
  char buffer[128];

  if (!string) {
    return 0;
  }
  base::FillBytes(base::ObjectBytes(buffer), '\xA5', sizeof(buffer));

  /*
  **	Copy key phrase into a working buffer. This hides any transformation
  *done *	to the string.
  */
  port::SafeCopy(buffer, string);
  int length = static_cast<int>(std::string_view(buffer).size());

  /*
  **	Only upper case letters are significant.
  */
  strupr(buffer);

  /*
  **	Ensure that only visible ASCII characters compose the key phrase. This
  **	discourages the direct forced illegal character input method of attack.
  */
  for (int index = 0; index < length; index++) {
    if (!isgraph(base::At(buffer, index))) {
      base::At(buffer, index) = static_cast<char>('A' + (index % 26));
    }
  }

  /*
  **	Increase the strength of even short pass phrases by extending the
  **	length to be at least a minimum number of characters. This helps prevent
  **	a weak pass phrase from compromising the obfuscation process. This
  **	process also forces the key phrase to be an even multiple of four.
  **	This is necessary to support the cypher process that occurs later.
  */
  if (length < 16 || length % 4 != 0) {
    const int maxlen = std::max(((length + 3) / 4) * 4, 16);
    int index = 0;
    for (index = length; index < maxlen; index++) {
      const int mixed = static_cast<uint8_t>('?') ^
                        static_cast<uint8_t>(base::At(buffer, index - length));
      base::At(buffer, index) = static_cast<char>('A' + ((mixed + index) % 26));
    }
    length = index;
    base::At(buffer, length) = '\0';
  }

  /*
  **	Transform the buffer into a number. This transformation is character
  **	order dependant.
  */
  uint32_t code = CrcEngine::Compute(buffer);

  /*
  **	Record a copy of this initial transformation to be used in a later
  **	self referential transformation.
  */
  const uint32_t copy = code;

  /*
  **	Reverse the character string and combine with the previous
  *transformation. *	This doubles the workload of trying to reverse engineer
  *the CRC calculation.
  */
  strrev(buffer);
  code ^= CrcEngine::Compute(buffer);

  /*
  **	Perform a self referential transformation. This makes a reverse
  *engineering *	by using a cause and effect attack more difficult.
  */
  code = code ^ copy;

  /*
  **	Unroll and combine the code value into the pass phrase and then perform
  **	another self referential transformation. Although this is a trivial
  *cypher *	process, it gives the sophisticated hacker false hope since the
  *strong *	cypher process occurs later.
  */
  strrev(buffer);  // Restore original string order.
  for (int index = 0; index < length; index++) {
    code ^= static_cast<unsigned char>(base::At(buffer, index));
    const auto temp = static_cast<unsigned char>(code);
    base::At(buffer, index) =
        static_cast<char>(static_cast<uint8_t>(base::At(buffer, index)) ^ temp);
    code >>= 8;
    code |= uint32_t{temp} << 24;
  }

  /*
  **	Introduce loss into the vector. This strengthens the key against
  *traditional *	cryptographic attack engines. Since this also weakens
  *the key against *	unconventional attacks, the loss is limited to less than
  *10%.
  */
  for (int index = 0; index < length; index++) {
    static const unsigned char _lossbits[] = {0x00, 0x08, 0x00, 0x20,
                                              0x00, 0x04, 0x10, 0x00};
    static const unsigned char _addbits[] = {0x10, 0x00, 0x00, 0x80,
                                             0x40, 0x00, 0x00, 0x04};

    base::At(buffer, index) =
        static_cast<char>(static_cast<uint8_t>(base::At(buffer, index)) |
                          base::At(_addbits, index % std::ssize(_addbits)));
    base::At(buffer, index) =
        static_cast<char>(static_cast<uint8_t>(base::At(buffer, index)) &
                          static_cast<uint8_t>(~base::At(
                              _lossbits, index % std::ssize(_lossbits))));
  }

  /*
  **	Perform a general cypher transformation on the vector
  **	and use the vector itself as the cypher key. This is a variation on the
  **	cypher process used in PGP. It is a very strong cypher process with no
  *known *	weaknesses. However, in this case, the cypher key is the vector
  *itself and this *	opens up a weakness against attacks that have access to
  *this transformation *	algorithm. The sheer workload of reversing this
  *transformation should be enough *	to discourage even the most determined
  *hackers.
  */
  for (int index = 0; index < length; index += 4) {
    // The original read these bytes as signed char and computed in signed
    // 16-bit values. Unsigned ones give the same result: the transformation
    // below uses only +, * and ^, whose low 8 bits depend only on the low 8
    // bits of their operands, and only those low 8 bits are stored back into
    // the buffer.
    const uint16_t key1 = static_cast<unsigned char>(base::At(buffer, index));
    const uint16_t key2 =
        static_cast<unsigned char>(base::At(buffer, index + 1));
    const uint16_t key3 =
        static_cast<unsigned char>(base::At(buffer, index + 2));
    const uint16_t key4 =
        static_cast<unsigned char>(base::At(buffer, index + 3));
    uint16_t val1 = key1;
    uint16_t val2 = key2;
    uint16_t val3 = key3;
    uint16_t val4 = key4;

    val1 = static_cast<uint16_t>(val1 * key1);
    val2 = static_cast<uint16_t>(val2 + key2);
    val3 = static_cast<uint16_t>(val3 + key3);
    val4 = static_cast<uint16_t>(val4 * key4);

    const uint16_t s3 = val3;
    val3 = static_cast<uint16_t>(val3 ^ val1);
    val3 = static_cast<uint16_t>(val3 * key1);
    const uint16_t s2 = val2;
    val2 = static_cast<uint16_t>(val2 ^ val4);
    val2 = static_cast<uint16_t>(val2 + val3);
    val2 = static_cast<uint16_t>(val2 * key3);
    val3 = static_cast<uint16_t>(val3 + val2);

    val1 = static_cast<uint16_t>(val1 ^ val2);
    val4 = static_cast<uint16_t>(val4 ^ val3);

    val2 = static_cast<uint16_t>(val2 ^ s3);
    val3 = static_cast<uint16_t>(val3 ^ s2);

    base::At(buffer, index) = static_cast<char>(val1);
    base::At(buffer, index + 1) = static_cast<char>(val2);
    base::At(buffer, index + 2) = static_cast<char>(val3);
    base::At(buffer, index + 3) = static_cast<char>(val4);
  }

  /*
  **	Convert this final vector into a cypher key code to be
  **	returned by this routine.
  */
  return CrcEngine::Compute(buffer);
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
  if (Session.LoadGame) {
    return;
  }

  //
  // If we're playing a recording, the Seed is loaded in
  // Load_Recording_Values().  Just init the random # and return.
  //
  if (Session.Play) {
    RandNumb = Seed;
    Scen.sync_rng_.set_seed(static_cast<uint32_t>(Seed));
    return;
  }

  /*
  **	Initialize the random number Seed.  For multiplayer, this will have been
  *done
  ** in the connection dialogs.  For single-player games, AND if we're not
  *playing
  ** back a recording, init the Seed to a random value.
  */
  if (Session.Type == GAME_NORMAL ||
      (Session.Type == GAME_SKIRMISH && !Session.Play)) {
    /*
    ** Set the optional user-specified seed
    */
    if (CustomSeed != 0) {
      Seed = CustomSeed;
    } else {
      Seed = port::RandomSeed();
    }
  }

  /*
  **	Initialize the random-number generators
  */
  Scen.sync_rng_.set_seed(static_cast<uint32_t>(Seed));
  RandNumb = Seed;
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
  Load_Title_Screen("TITLE.PCX", &HidPage, CCPalette);

  if (visible) {
    HidPage.Blit(SeenBuff);
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

  SysMemPage.Clear();
  Load_Picture("PALETTE.CPS", SysMemPage, SysMemPage, {}, BM_DEFAULT);
  SysMemPage.Blit(HidPage);
  for (const PlayerColorType pcolor :
       magic_enum::enum_values<PlayerColorType>()) {
    auto& ptr = ColorRemaps.at(pcolor).RemapTable;

    for (int color = 0; color < 256; color++) {
      base::At(ptr, color) = static_cast<unsigned char>(color);
    }

    for (int index = 0; index < 16; index++) {
      base::At(ptr, HidPage.Get_Pixel(index, 0)) = static_cast<unsigned char>(
          HidPage.Get_Pixel(index, static_cast<int>(pcolor)));
    }
    for (int index = 0; index < 6; index++) {
      base::At(ColorRemaps.at(pcolor).FontRemap, 10 + index) =
          static_cast<unsigned char>(
              HidPage.Get_Pixel(2 + index, static_cast<int>(pcolor)));
    }
    ColorRemaps.at(pcolor).BrightColor = kWhite;
    //		ColorRemaps[pcolor].BrightColor = HidPage.Get_Pixel(1,
    // static_cast<int>(pcolor));
    ColorRemaps.at(pcolor).Color = static_cast<unsigned char>(
        HidPage.Get_Pixel(4, static_cast<int>(pcolor)));

    ColorRemaps.at(pcolor).Shadow = static_cast<unsigned char>(
        HidPage.Get_Pixel(10, static_cast<int>(pcolor)));
    ColorRemaps.at(pcolor).Background = static_cast<unsigned char>(
        HidPage.Get_Pixel(9, static_cast<int>(pcolor)));
    ColorRemaps.at(pcolor).Corners = static_cast<unsigned char>(
        HidPage.Get_Pixel(7, static_cast<int>(pcolor)));
    ColorRemaps.at(pcolor).Highlight = static_cast<unsigned char>(
        HidPage.Get_Pixel(4, static_cast<int>(pcolor)));
    ColorRemaps.at(pcolor).Bright = static_cast<unsigned char>(
        HidPage.Get_Pixel(0, static_cast<int>(pcolor)));
    ColorRemaps.at(pcolor).Underline = static_cast<unsigned char>(
        HidPage.Get_Pixel(0, static_cast<int>(pcolor)));
    ColorRemaps.at(pcolor).Bar = static_cast<unsigned char>(
        HidPage.Get_Pixel(6, static_cast<int>(pcolor)));

    /*
    **	This must grab from column 4 because the multiplayer color dialog
    *palette counts *	on this to be true.
    */
    ColorRemaps.at(pcolor).Box = static_cast<unsigned char>(
        HidPage.Get_Pixel(4, static_cast<int>(pcolor)));
  }

  /*
  ** Now do the special dim grey scheme
  */
  for (int color = 0; color < 256; color++) {
    base::At(GreyScheme.RemapTable, color) = static_cast<unsigned char>(color);
  }
  // The palette index in the low byte of the pixel read from the grey row.
  const auto GreyPixel = [](int x) {
    return static_cast<uint8_t>(
        HidPage.Get_Pixel(x, static_cast<int>(PCOLOR_GREY)));
  };
  for (int index = 0; index < 6; index++) {
    base::At(GreyScheme.FontRemap, 10 + index) = GreyPixel(9 + index);
  }
  GreyScheme.BrightColor = GreyPixel(3);
  GreyScheme.Color = GreyPixel(7);

  GreyScheme.Shadow =
      base::At(ColorRemaps.at(PCOLOR_GREY).RemapTable, GreyPixel(15));
  GreyScheme.Background =
      base::At(ColorRemaps.at(PCOLOR_GREY).RemapTable, GreyPixel(14));
  GreyScheme.Corners =
      base::At(ColorRemaps.at(PCOLOR_GREY).RemapTable, GreyPixel(13));
  GreyScheme.Highlight =
      base::At(ColorRemaps.at(PCOLOR_GREY).RemapTable, GreyPixel(9));
  GreyScheme.Bright =
      base::At(ColorRemaps.at(PCOLOR_GREY).RemapTable, GreyPixel(5));
  GreyScheme.Underline =
      base::At(ColorRemaps.at(PCOLOR_GREY).RemapTable, GreyPixel(5));
  GreyScheme.Bar =
      base::At(ColorRemaps.at(PCOLOR_GREY).RemapTable, GreyPixel(11));
  GreyScheme.Box =
      base::At(ColorRemaps.at(PCOLOR_GREY).RemapTable, GreyPixel(11));

  /*
  ** Set up the metallic remap table for the font that prints over the tabs
  */
  base::FillBytes(base::ObjectBytes(MetalScheme), 4, sizeof(MetalScheme));
  for (int color_counter = 0; color_counter < 16; color_counter++) {
    base::At(MetalScheme.FontRemap, color_counter) =
        static_cast<unsigned char>(color_counter);
  }
  MetalScheme.FontRemap[1] = 128;
  MetalScheme.FontRemap[2] = 12;
  MetalScheme.FontRemap[3] = 13;
  MetalScheme.FontRemap[4] = 14;
  MetalScheme.Color = 128;
  MetalScheme.Background = 0;
  MetalScheme.Underline = 128;

  /*
  ** Set up the font remap table for the mission briefing font
  */
  for (int colr = 0; colr < 16; colr++) {
    base::At(ColorRemaps.at(PCOLOR_TYPE).FontRemap, colr) =
        static_cast<unsigned char>(
            HidPage.Get_Pixel(colr, static_cast<int>(PCOLOR_TYPE)));
  }

  ColorRemaps.at(PCOLOR_TYPE).Shadow = 11;
  ColorRemaps.at(PCOLOR_TYPE).Background = 10;
  ColorRemaps.at(PCOLOR_TYPE).Corners = 10;
  ColorRemaps.at(PCOLOR_TYPE).Highlight = 9;
  ColorRemaps.at(PCOLOR_TYPE).Bright = 15;
  ColorRemaps.at(PCOLOR_TYPE).Underline = 11;
  ColorRemaps.at(PCOLOR_TYPE).Bar = 11;
  ColorRemaps.at(PCOLOR_TYPE).Box = 10;
  ColorRemaps.at(PCOLOR_TYPE).BrightColor = 15;
  ColorRemaps.at(PCOLOR_TYPE).Color = 9;

  GadgetClass::Set_Color_Scheme(&ColorRemaps.at(PCOLOR_DIALOG_BLUE));
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
  Vessels.Set_Heap(Rule.VesselMax);
  Units.Set_Heap(Rule.UnitMax);
  Factories.Set_Heap(Rule.FactoryMax);
  Terrains.Set_Heap(Rule.TerrainMax);
  Templates.Set_Heap(Rule.TemplateMax);
  Smudges.Set_Heap(Rule.SmudgeMax);
  Overlays.Set_Heap(Rule.OverlayMax);
  Infantry.Set_Heap(Rule.InfantryMax);
  Bullets.Set_Heap(Rule.BulletMax);
  Buildings.Set_Heap(Rule.BuildingMax);
  Anims.Set_Heap(Rule.AnimMax);
  Aircraft.Set_Heap(Rule.AircraftMax);
  Triggers.Set_Heap(Rule.TriggerMax);
  TeamTypes.Set_Heap(Rule.TeamTypeMax);
  Teams.Set_Heap(Rule.TeamMax);
  Houses.Set_Heap(kHouseMax);
  TriggerTypes.Set_Heap(Rule.TrigTypeMax);
  //	Weapons.Set_Heap(Rule.WeaponMax);

  /*
  **	Speech holding tank buffer. Since speech does not mix, it can be placed
  **	into a custom holding tank only as large as the largest speech file to
  **	be played.
  */
  for (int index = 0; index < std::ssize(SpeechBuffer); index++) {
    base::At(SpeechBuffer, index).resize(kSpeechBufferSize);
    base::At(SpeechRecord, index) = VOX_NONE;
    assert(!base::At(SpeechBuffer, index).empty());
  }

  /*
  **	Allocate the theater buffer block.
  */
  TheaterBuffer = new Buffer(kTheaterBufferSize);
  assert(TheaterBuffer != nullptr);
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
      if (port::CompareIgnoreCase(state.name, "scores.mix") == 0) {
        continue;
      }
      MixArchive::Register(state.name, &FastKey);
      MixArchive::Cache(state.name);
    } while (Find_Next_File(state));
  }
  if (Find_First_File("SS*.MIX", state)) {
    do {
      MixArchive::Register(state.name, &FastKey);
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
  Call_Back();
  Map.One_Time();
  Logic.One_Time();
  Options.One_Time();
  Session.One_Time();

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
static void Init_Fonts() {
  Metal12FontPtr = MixArchive::RetrieveData("12METFNT.FNT");
  MapFontPtr = MixArchive::RetrieveData("HELP.FNT");
  Font6Ptr = MixArchive::RetrieveData("6POINT.FNT");
  GradFont6Ptr = MixArchive::RetrieveData("GRAD6FNT.FNT");
  EditorFont = MixArchive::RetrieveData("EDITFNT.FNT");
  Font8Ptr = MixArchive::RetrieveData("8POINT.FNT");
  FontPtr = Font8Ptr;
  Set_Font(FontPtr);
  Font3Ptr = MixArchive::RetrieveData("3POINT.FNT");
  ScoreFontPtr = MixArchive::RetrieveData("SCOREFNT.FNT");
  FontLEDPtr = MixArchive::RetrieveData("LED.FNT");
  VCRFontPtr = MixArchive::RetrieveData("VCR.FNT");
  TypeFontPtr =
      MixArchive::RetrieveData("8POINT.FNT");  //("TYPE.FNT"); //VG 10/17/96
}

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
  VisiblePage.Clear();
  HidPage.Clear();

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
          VisiblePage.Clear();
          GamePalette.Set();
          Show_Mouse();
          WWMessageBox().Process(TXT_CD_ERROR1, TXT_OK);
          // Prog_End();
          Emergency_Exit(EXIT_FAILURE);

        case 2:
          VisiblePage.Clear();
          GamePalette.Set();
          Show_Mouse();
          if (WWMessageBox().Process(TXT_CD_DIALOG_1, TXT_OK, TXT_CANCEL) ==
              1) {
            // Prog_End();
            Emergency_Exit(EXIT_FAILURE);
          }
          Hide_Mouse();
          break;

        default:
          VisiblePage.Clear();
          Show_Mouse();
          if (!Force_CD_Available(RequiredCD)) {
            // Prog_End();
            Emergency_Exit(EXIT_FAILURE);
          }
          Hide_Mouse();
          break;
      }
    } while (error);

    RequiredCD = -1;
  } else {
    /*
    ** If there are search drives specified then all files are to be
    ** considered local.
    */
    RequiredCD = -2;
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
static void Init_Bootstrap_Mixfiles() {
  const int temp = RequiredCD;
  RequiredCD = -2;

  if constexpr (config::kWolapiEnabled) {
    GameFile fileWolapiMix("WOLAPI.MIX");
    if (fileWolapiMix.IsAvailable()) {
      MixArchive::Register("WOLAPI.MIX", &FastKey);
      MixArchive::Cache("WOLAPI.MIX");
    }
  }

  GameFile file2("EXPAND2.MIX");
  if (file2.IsAvailable()) {
    MixArchive::Register("EXPAND2.MIX", &FastKey);
    bool ok = MixArchive::Cache("EXPAND2.MIX");
    assert(ok);

    MixArchive::Register("HIRES1.MIX", &FastKey);
    ok = MixArchive::Cache("HIRES1.MIX");
    assert(ok);
  }

  GameFile file("EXPAND.MIX");
  if (file.IsAvailable()) {
    MixArchive::Register("EXPAND.MIX", &FastKey);
    const bool ok = MixArchive::Cache("EXPAND.MIX");
    assert(ok);
  }

  MixArchive::Register("REDALERT.MIX", &FastKey);

  /*
  **	Bootstrap enough of the system so that the error dialog box can
  *successfully *	be displayed.
  */
  MixArchive::Register("LOCAL.MIX", &FastKey);  // Cached.
  bool ok = MixArchive::Cache("LOCAL.MIX");
  assert(ok);

  MixArchive::Register("HIRES.MIX", &FastKey);
  ok = MixArchive::Cache("HIRES.MIX");
  assert(ok);

  MixArchive::Register("NCHIRES.MIX",
                       &FastKey);  // Non-cached hires stuff incl VQ palettes

  RequiredCD = temp;
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
  if (GameFile("MAIN1.MIX").IsAvailable()) {
    // MAIN1-4 from steam

    // extract the extra missions from the expansion "discs"
    // (they don't contain the base missions)
    if (GameFile("MAIN3.MIX").IsAvailable() &&
        !GameFile("GENERAL3.MIX").IsAvailable()) {
      const MixArchive* tmp = MixArchive::Register("MAIN3.MIX", &FastKey);
      Extract("GENERAL.MIX", "GENERAL3.MIX");
      delete tmp;
    }

    if (GameFile("MAIN4.MIX").IsAvailable() &&
        !GameFile("GENERAL4.MIX").IsAvailable()) {
      const MixArchive* tmp = MixArchive::Register("MAIN4.MIX", &FastKey);
      Extract("GENERAL.MIX", "GENERAL4.MIX");
      Extract("SCORES.MIX", "SCORES.MIX");  // also extract scores
      delete tmp;
    }

    // load the first two to get both movies
    MixArchive::Register("MAIN2.MIX", &FastKey);
    MixArchive::Register("MAIN1.MIX", &FastKey);

    // load extra missions
    MixArchive::Register("GENERAL4.MIX", &FastKey);
    MixArchive::Register("GENERAL3.MIX", &FastKey);
  } else {
    // assume regular/TFD files
    MainMix = MixArchive::Register("MAIN.MIX", &FastKey);
    assert(MainMix != nullptr);
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
  ConquerMix = MixArchive::Register("CONQUER.MIX", &FastKey);  // Cached.
  //	MixArchive::Register("TRANSIT.MIX", &FastKey);

  if (GeneralMix == nullptr) {
    GeneralMix =
        MixArchive::Register("GENERAL.MIX", &FastKey);  // Never cached.
  }

  if (GameFile("MOVIES1.MIX").IsAvailable()) {
    MoviesMix = MixArchive::Register("MOVIES1.MIX", &FastKey);  // Never cached.
  }
  // load both sets of movies if possible
  if (GameFile("MOVIES2.MIX").IsAvailable()) {
    MoviesMix = MixArchive::Register("MOVIES2.MIX", &FastKey);  // Never cached.
  }
  assert(MoviesMix != nullptr);

  /*
  **	Register the score mixfile.
  */
  ScoresPresent = true;
  ScoreMix = MixArchive::Register("SCORES.MIX", &FastKey);
  ThemeClass::Scan();

  /*
  **	These are sound card specific, but the install program would have
  **	copied the correct versions to the hard drive.
  */
  MixArchive::Register("SPEECH.MIX", &FastKey);   // Never cached.
  MixArchive::Register("SOUNDS.MIX", &FastKey);   // Cached.
  MixArchive::Register("RUSSIAN.MIX", &FastKey);  // Cached.
  MixArchive::Register("ALLIES.MIX", &FastKey);   // Cached.
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
  BlackPalette.Set();

  /*
  **	Be sure to short circuit the CD-ROM check if there is a CD-ROM override
  **	path.
  */
  if (SearchPaths::HasAny()) {
    RequiredCD = -2;
  }

  /*
  ** Process the message loop until we are in focus. We need to be in focus to
  *read pixels from
  ** the screen.
  */
  do {
    Keyboard->Check();
  } while (!GameInFocus);
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
  Keyboard->Clear();

  /*
  **	This is the shape staging buffer. It must always be available, so it is
  **	allocated here and never freed. The library sets the globals ShapeBuffer
  **	and ShapeBufferSize to these values, so it can be accessed for other
  **	purposes.
  */
  shape_storage.resize(kShapeBufferSize);
  Set_Shape_Buffer(shape_storage);

  // The .ENG suffix is the same in every language build: localized releases
  // ship a translated CONQUER.ENG under the same name.
  SystemStrings = MixArchive::RetrieveData("CONQUER.ENG");
  DebugStrings = MixArchive::RetrieveData("DEBUG.ENG");

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
  base::CopyBytes(std::as_writable_bytes(GamePalette.bytes()), palette_data,
                  768);
  WhitePalette.at(0) = BlackPalette.at(0);
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
  if (MouseInstalled) {
    const auto temp_mouse_shapes = MixArchive::RetrieveData("MOUSE.SHP");
    if (!temp_mouse_shapes.empty()) {
      Set_Mouse_Cursor(0, 0, Extract_Shape(temp_mouse_shapes, 0));
      while (Get_Mouse_State() > 1) {
        Show_Mouse();
      }
    }
  } else {
    GamePalette.Set();
    GamePalette.Set();
    VisiblePage.Clear();
    WWMessageBox().Process(kLanguageText.no_mouse, TXT_OK);
    // Prog_End();
    Emergency_Exit(1);
  }

  Map.Set_Default_Mouse(MOUSE_NORMAL, false);
  Show_Mouse();
  while (Get_Mouse_State() > 1) {
    Show_Mouse();
  }
  Call_Back();
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
  if (SampleType != SAMPLE_NONE && !Debug_Quiet) {
    MixArchive::Cache("SOUNDS.MIX");
    MixArchive::Cache("RUSSIAN.MIX");
    MixArchive::Cache("ALLIES.MIX");
  }
  Call_Back();

  /*
  **	Fetch the tutorial message data.
  */
  INIClass ini;
  GameFile fc("TUTORIAL.INI");
  ini.Load(fc);
  TutorialTextData.clear();
  for (int index = 0; index < std::ssize(TutorialTextOffsets); ++index) {
    base::At(TutorialTextOffsets, index) = 0xFFFF;
    char buffer[128];
    char num[10];
    absl::SNPrintF(num, sizeof(num), "%d", index);
    if (ini.Get_String("Tutorial", num, "", buffer, sizeof(buffer))) {
      const auto text = std::string_view(buffer);
      if (TutorialTextData.size() + text.size() + 1 > 0xFFFF) {
        break;
      }
      base::At(TutorialTextOffsets, index) =
          static_cast<uint16_t>(TutorialTextData.size());
      TutorialTextData.insert(TutorialTextData.end(), text.begin(), text.end());
      TutorialTextData.push_back('\0');
    }
  }

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

  FastKey = ini.Get_PKey(true);
  if constexpr (config::kScenarioEditorEnabled) {
    SlowKey = ini.Get_PKey(false);
  }
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
 ** save some data not needed specifically by a save-game file (ie Seed).
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
  ar(Session);
  Session.SerializePlayers(ar);
  ar(BuildLevel, Debug_Unshroud, Seed, Scen.Scenario, Scen.ScenarioName,
     Whom, Special, Options);
  if constexpr (Archive::kIsReading) {
    Scen.ScenarioName[sizeof(Scen.ScenarioName) - 1] = '\0';
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
