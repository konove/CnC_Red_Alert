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
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iterator>
#include <memory>
#include <string>

#include "absl/strings/match.h"
#include "port/ex_string.h"
#include "ra/_wsproto.h"
#include "ra/ccfile.h"
#include "ra/ccini.h"
#include "ra/compat.h"
#include "ra/config.h"
#include "ra/conquer.h"
#include "ra/const.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/event.h"
#include "ra/externs.h"
#include "ra/gadget.h"
#include "ra/globals.h"
#include "ra/goptions.h"
#include "ra/graphics_loader.h"
#include "ra/heap.h"
#include "ra/house.h"
#include "ra/ini.h"
#include "ra/inline.h"
#include "ra/internet.h"
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
#include "ra/monoc.h"
#include "ra/mplayer.h"
#include "ra/msgbox.h"
#include "ra/msglist.h"
#include "ra/netdlg.h"
#include "ra/nulldlg.h"
#include "ra/nullmgr.h"
#include "ra/palette.h"
#include "ra/queue.h"
#include "ra/rules.h"
#include "ra/scenario.h"
#include "ra/session.h"
#include "ra/special.h"
#include "ra/startup.h"
#include "ra/theme.h"
#include "ra/type.h"
#include "ra/vector.h"
#include "ra/vector_dynamic.h"
#include "ra/wsproto.h"
#include "ra/wspudp.h"
#include "sdllib/file.h"
#include "sdllib/font.h"
#include "sdllib/gbuffer.h"
#include "sdllib/iff.h"
#include "sdllib/misc.h"
#include "sdllib/shape.h"
#include "sdllib/timer.h"
#include "sdllib/ww_audio.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "tech/bench.h"
#include "tech/buff.h"
#include "tech/crc.h"
#include "tech/fixed.h"
#include "tech/ftimer.h"
#include "tech/mpu.h"
#include "tech/pk.h"
#include "tech/ramfile.h"
#include "tech/random.h"
#include "tech/rawfile.h"
#include "tech/rgb.h"
#include "tech/rndstraw.h"
#include "winvq/vqa32/vqaplay.h"

#ifdef DONGLE
#include "cbn_.h"
#endif

#ifdef MPEGMOVIE  // Denzil 6/25/98
#include "ra/mpgset.h"
#endif

RemapControlType SidebarScheme;

extern bool bNoMovies;

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

extern "C" {
extern long RandNumb;
}
#ifndef WIN32
static int UsePageFaultHandler = 1;  // 1 = install PFH
#endif                               // WIN32

void Init_Random();

#define ATTRACT_MODE_TIMEOUT 3600  // timeout for attract mode

bool Load_Recording_Values(CCFileClass& file);
bool Save_Recording_Values(CCFileClass& file);

#ifdef WOLAPI_INTEGRATION
extern int WOL_Main();
#include "WolapiOb.h"
extern WolapiObject* pWolapi;
#endif

bool Expansion_Dialog(bool bCounterstrike);

extern bool Is_Mission_Counterstrike(char* file_name);

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
  HidPage.Blit(SeenPage);
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
bool Init_Game(int, char*[]) {
  /*
  **	Allocate the benchmark tracking objects only if the machine and
  **	compile flags indicate.
  */
  if constexpr (config::kCheatKeysEnabled) {
    if (Processor() >= 2) {
      Benches = new Benchmark[BENCH_COUNT];
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
  HouseTypes.Set_Heap(HOUSE_COUNT);
  BuildingTypes.Set_Heap(STRUCT_COUNT);
  AircraftTypes.Set_Heap(AIRCRAFT_COUNT);
  InfantryTypes.Set_Heap(INFANTRY_COUNT);
  BulletTypes.Set_Heap(BULLET_COUNT);
  AnimTypes.Set_Heap(ANIM_COUNT);
  UnitTypes.Set_Heap(UNIT_COUNT);
  VesselTypes.Set_Heap(VESSEL_COUNT);
  TemplateTypes.Set_Heap(TEMPLATE_COUNT);
  TerrainTypes.Set_Heap(TERRAIN_COUNT);
  OverlayTypes.Set_Heap(OVERLAY_COUNT);
  SmudgeTypes.Set_Heap(SMUDGE_COUNT);

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
  CCFileClass fc("RULES.INI");
  if (RuleINI.Load(fc, false)) {
    Rule.Process(RuleINI);
  }
  //  Aftermath runtime change 9/29/98
  //	This is safe to do, as only rules for aftermath units are included in
  // this ini.
  if (Is_Aftermath_Installed()) {
    CCFileClass fc("AFTRMATH.INI");
    if (AftermathINI.Load(fc, false)) {
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

#ifdef MPEGMOVIE  // Denzil 6/15/98
  if (Using_DVD()) {
#ifdef MCIMPEG
    MciMovie = new MCIMovie(MainWindow);
#endif
    MpgSettings = new MPGSettings(nullptr);  // RawFileClass(CONFIG_FILE_NAME));
  }
#endif

  /*
  **	Play the startup animation.
  */
  if (!Special.IsFromInstall && !Special.IsFromWChat) {
    VisiblePage.Clear();
    //		Mono_Printf("Playing Intro\n");
    Play_Intro();
    memset(CurrentPalette, 0x01, 768);
    WhitePalette.Set();
  } else {
    memset(CurrentPalette, 0x01, 768);
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
    Fancy_Text_Print(TXT_STAND_BY, 160 * RESFACTOR, 120 * RESFACTOR,
                     &ColorRemaps[PCOLOR_DIALOG_BLUE], TBLACK,
                     TPF_CENTER | TPF_TEXT | TPF_DROPSHADOW);
    Show_Mouse();

    CCPalette.Set(FADE_PALETTE_SLOW);
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
  for (int i = 0; i < MAX_MULTI_NAMES; i++) {
    Session.Score[i].Name[0] = '\0';
    Session.Score[i].Wins = 0;
    for (int j = 0; j < MAX_MULTI_GAMES; j++) {
      Session.Score[i].Kills[j] =
          -1;  // -1 = this player didn't play this round
    }
  }

  /*
  ** Copy the title screen's palette into the GamePalette & OriginalPalette,
  ** because the options Load routine uses these palettes to set the brightness,
  *etc.
  */
  GamePalette = CCPalette;
  //	InGamePalette = CCPalette;
  OriginalPalette = CCPalette;

  /*
  **	Read game options, so the GameSpeed is initialized when multiplayer
  ** dialogs are invoked.  (GameSpeed must be synchronized between systems.)
  */
  Options.Load_Settings();

  return true;
}

#ifdef WINSOCK_IPX  //	Steve Tall missed this one - ajw
extern bool Get_Broadcast_Addresses();
#endif

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
  enum {
    SEL_TIMEOUT = -1,      // main menu timeout--go into attract mode
    SEL_NEW_SCENARIO_CS,   // Expansion scenario to play.
    SEL_NEW_SCENARIO_AM,   // Expansion scenario to play.
    SEL_START_NEW_GAME,    // start a new game
    SEL_LOAD_MISSION,      // load a saved game
    SEL_MULTIPLAYER_GAME,  // play modem/null-modem/network game
    SEL_INTRO,             // couch-potato mode
    SEL_EXIT,              // exit to DOS
    SEL_FAME,              // view the hall o' fame
    SEL_NONE,              // placeholder default value
  };

  bool gameloaded = false;  // Has the game been loaded from the menu?
  int selection;            // the default selection
  bool process = true;      // false = break out of while loop
  bool display = true;

#ifdef DONGLE
  /* These where added by ColinM for the dongle checking */
  short iRet = 0;
  unsigned short iPortNr = 1; /* automatic port scan enabled */
  unsigned char cSCodeSER[] = "\x41\x42";
  unsigned long ulIdRet = 0;
  unsigned char cBoxName[] = "\x00\x00";
#endif

  int cdcheck = 0;

  //	#ifndef DVD // Denzil - We want the menu screen			ajw No
  // we don't 	if (Special.IsFromInstall) { 		display = false;
  //	}
  //	#endif

  Show_Mouse();

  NewUnitsEnabled = SecretUnitsEnabled =
      0;  // Assume new units disabled, unless specifically .INI enabled or
          // multiplayer negotiations enable it.

#ifndef WOLAPI_INTEGRATION
#ifdef _WIN32
  /*
  ** Enable the DDE Server so we can get internet start game packets from WChat
  */
  DDEServer.Enable();
#endif  // WIN32
#endif  //	!WOLAPI_INTEGRATION

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
  Map.Set_Cursor_Shape(nullptr);
  Map.PendingObjectPtr = nullptr;
  Map.PendingObject = nullptr;
  Map.PendingHouse = HOUSE_NONE;

  Session.ProcessTicks = 0;
  Session.ProcessFrames = 0;
  Session.DesiredFrameRate = 30;
#if (TIMING_FIX)
  NewMaxAheadFrame1 = 0;
  NewMaxAheadFrame2 = 0;
#endif

/* ColinM added to check for dongle */
#ifdef DONGLE
  iRet = CbN_BoxReady(iPortNr, cBoxName);
  if (cBoxName[0] != 0xc5 && cBoxName[0] != 0xc9) {
    WWMessageBox().Process(
        "Please ensure dongle is attached. Run the dongle batch file too.",
        TXT_OK);
    Emergency_Exit(EXIT_FAILURE);
  }

  iRet = CbN_ReadSER(iPortNr, cSCodeSER, &ulIdRet);
  if (ulIdRet != 0xa0095) {
    WWMessageBox().Process(
        "Please ensure dongle is attached. Run the dongle batch file too.",
        TXT_OK);
    Emergency_Exit(EXIT_FAILURE);
  }
#endif

  /*
  **	Init multiplayer game scores.  Let Wins accumulate; just init the
  *current
  ** Kills for this game.  Kills of -1 means this player didn't play this round.
  */
  for (int i = 0; i < MAX_MULTI_GAMES; i++) {
    Session.Score[i].Kills[Session.CurGame] = -1;
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
    selection = SEL_NONE;
  } else {
    selection = SEL_MULTIPLAYER_GAME;
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
    if (Session.Play && Session.RecordFile.Is_Available()) {
      if (Session.RecordFile.Open(READ)) {
        Load_Recording_Values(Session.RecordFile);
        process = false;
        Theme.Fade_Out();
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

        HidPage.Blit(SeenPage);
        //				if (fade) {
        //					WhitePalette.Set();
        //					CCPalette.Set(FADE_PALETTE_SLOW,
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
        selection = SEL_START_NEW_GAME;
      }

#ifndef WOLAPI_INTEGRATION
#if defined(_WIN32) && !defined(INTERNET_OFF)  // Denzil 5/1/98 - Internet play
      /*
      ** Handle case where we were spawned from Wchat and our start game
      **  packet has already arrived
      */
      if (Special.IsFromWChat && DDEServer.Get_MPlayer_Game_Info()) {
        Check_From_WChat(nullptr);
        selection = SEL_MULTIPLAYER_GAME;
        Theme.Queue_Song(THEME_QUIET);
        Session.Type = GAME_INTERNET;
      } else {
        /*
        ** We werent spawned but we could still receive a DDE packet from wchat
        */
        if (DDEServer.Get_MPlayer_Game_Info()) {
          Check_From_WChat(nullptr);
          /*
          ** Make sure top and bottom of screen are clear in 640x480 mode
          */
          if (ScreenHeight == 480) {
            VisiblePage.Fill_Rect(0, 0, 639, 40, 0);
            VisiblePage.Fill_Rect(0, 440, 639, 479, 0);
          }
        }
      }
#endif  // WIN32
#endif

#ifdef WOLAPI_INTEGRATION
      if (pWolapi) {
        selection = SEL_MULTIPLAYER_GAME;  //	We are returning from a game.
      }
#endif

      if (selection == SEL_NONE) {
        AntsEnabled = false;
        selection = Main_Menu(ATTRACT_MODE_TIMEOUT);
      }
      Call_Back();

      switch (selection) {
        /*
        **	Pick an expansion scenario.
        */
        case SEL_NEW_SCENARIO_CS:
        case SEL_NEW_SCENARIO_AM:
          Scen.CarryOverMoney = 0;
          IsTanyaDead = false;
          SaveTanya = false;

          if (selection == SEL_NEW_SCENARIO_CS) {
            if (!Force_CD_Available(2)) {
              selection = SEL_NONE;
              break;
            }
            if (!Expansion_Dialog(true)) {
              selection = SEL_NONE;
              break;
            }
          } else {
            if (!Force_CD_Available(3)) {
              selection = SEL_NONE;
              break;
            }
            if (!Expansion_Dialog(false)) {
              selection = SEL_NONE;
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
          }

          Theme.Fade_Out();
          Theme.Queue_Song(THEME_FIRST);
          Session.Type = GAME_NORMAL;
          process = false;
          break;

        /*
        **	SEL_START_NEW_GAME: Play the game
        */
        case SEL_START_NEW_GAME:
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
                  selection = SEL_NONE;
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
#ifdef DVD  // Denzil			ajw Presumably a bug fix.
            Choose_Side();
            Hide_Mouse();
#else
            Hide_Mouse();
            Choose_Side();
#endif
            if (CurrentCD == 0) {
              Scen.Set_Scenario_Name("SCG01EA.INI");
            } else {
              Scen.Set_Scenario_Name("SCU01EA.INI");
            }
          }

          Session.Type = GAME_NORMAL;
          process = false;
          break;

          //				#if defined(MPEGMOVIE) // Denzil 6/25/98
          //				case SEL_MOVIESETTINGS:
          //					MpgSettings->Dialog();
          //					display = true;
          //					selection = SEL_NONE;
          //				break;
          //				#endif

        /*
        **	Load a saved game.
        */
        case SEL_LOAD_MISSION:
          if (LoadOptionsClass(LoadOptionsClass::LOAD).Process()) {
            Theme.Queue_Song(THEME_FIRST);
            process = false;
            gameloaded = true;
          } else {
            display = true;
            selection = SEL_NONE;
          }
          break;

        /*
        **	SEL_MULTIPLAYER_GAME: set 'Session.Type' to nullptr-modem,
        * modem, or *	network play.
        */
        case SEL_MULTIPLAYER_GAME:
#ifdef WOLAPI_INTEGRATION
          if (!pWolapi) {
#endif
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
                  selection = SEL_NONE;
                }
                break;

              case GAME_SKIRMISH:
                if (!Com_Scenario_Dialog(true)) {
                  Session.Type = Select_MPlayer_Game();
                  if (Session.Type == GAME_NORMAL) {  // user hit Cancel
                    display = true;
                    selection = SEL_NONE;
                  }
                } else {
                  //	Ever hits? Session.Type set to GAME_SKIRMISH without
                  // user selecting in Select_MPlayer_Game()?
                  //	If mission is Counterstrike, CS CD will be required. But
                  // aftermath units require AM CD.
                  bAftermathMultiplayer =
                      Is_Aftermath_Installed() &&
                      !Is_Mission_Counterstrike(Scen.ScenarioName);
                  //	ajw I'll bet this was needed before also...
                  Session.ScenarioIsOfficial =
                      Session.Scenarios[Session.Options.ScenarioIndex]
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
                        selection = SEL_NONE;
                      }
                    }
                  } else {
                    if (!Com_Show_Scenario_Dialog()) {
                      Session.Type = Select_Serial_Dialog();
                      if (Session.Type == GAME_NORMAL) {  // user hit Cancel
                        display = true;
                        selection = SEL_NONE;
                      }
                    }
                  }
                } else {
                  Session.Type = Select_MPlayer_Game();
                  if (Session.Type == GAME_NORMAL) {  // 'Cancel'
                    display = true;
                    selection = SEL_NONE;
                  }
                }
                break;

#ifndef WOLAPI_INTEGRATION
#if defined(WIN32) && !defined(INTERNET_OFF)  // Denzil 5/1/98 - Internet play
              /*
              ** Handle being spawned from WChat. Internet play based on IPX
              *code.
              */
              case GAME_INTERNET:  //	ajw		No longer hit.
              {
                if (Special.IsFromWChat) {
#ifndef PORTABLE
                  /*
                  ** Give myself focus.
                  */
                  SetForegroundWindow(MainWindow);
                  ShowWindow(MainWindow, ShowCommand);
#endif
#ifdef WINSOCK_IPX

                  delete PacketTransport;
                  PacketTransport = new UDPInterfaceClass;
                  assert(PacketTransport != nullptr);

                  if (PacketTransport->Init()) {
                    WWDebugString(
                        "RA95 - About to read multiplayer settings.\n");
                    Session.Read_MultiPlayer_Settings();

                    WWDebugString(
                        "RA95 - About to call Start_Server or Start_Client.\n");
                    PacketTransport->Start_Listening();

                    /*
                    ** Flush out any pending packets from a previous game.
                    */
                    PacketTransport->Discard_In_Buffers();
                    PacketTransport->Discard_Out_Buffers();

                  } else {
                    delete PacketTransport;
                    PacketTransport = nullptr;
                    WWDebugString("RA95 - Winsock failed to initialise.\n");
                    Session.Type = GAME_NORMAL;
                    selection = SEL_EXIT;
                    Special.IsFromWChat = false;
                    break;
                  }

                  WWDebugString("RA95 - About to call Init_Network.\n");
                  Init_Network();

#else   // WINSOCK_IPX

                  WWDebugString("RA95 - About to initialise Winsock.\n");
                  if (Winsock.Init()) {
                    WWDebugString(
                        "RA95 - About to read multiplayer settings.\n");
                    Session.Read_MultiPlayer_Settings();
                    Server = PlanetWestwoodIsHost;

                    WWDebugString("RA95 - About to set addresses.\n");
                    Winsock.Set_Host_Address(PlanetWestwoodIPAddress);

                    WWDebugString(
                        "RA95 - About to call Start_Server or Start_Client.\n");
                    if (Server) {
                      Winsock.Start_Server();
                    } else {
                      Winsock.Start_Client();
                    }

                    /*
                    ** Flush out any pending packets from a previous game.
                    */
                    WWDebugString("RA95 - About to flush packet queue.\n");
                    WWDebugString("RA95 - Allocating scrap memory.\n");
                    char* temp_buffer = new char[1024];

                    WWDebugString("RA95 - Creating timer class instance.\n");
                    CountDownTimerClass ptimer;

                    WWDebugString("RA95 - Entering read loop.\n");
                    while (Winsock.Read(temp_buffer, 1024)) {
                      WWDebugString("RA95 - Discarding a packet.\n");
                      ptimer.Set(30, true);
                      while (ptimer.Time()) {
                      };
                      WWDebugString(
                          "RA95 - Ready to check for more packets.\n");
                    }
                    WWDebugString("RA95 - About to delete scrap memory.\n");
                    delete temp_buffer;

                  } else {
                    WWDebugString("RA95 - Winsock failed to initialise.\n");
                    Session.Type = GAME_NORMAL;
                    selection = SEL_EXIT;
                    Special.IsFromWChat = false;
                    break;
                  }
#endif  // WINSOCK_IPX
                  WWDebugString("RA95 - About to call Init_Network.\n");
                  Init_Network();

#ifdef _WIN32
                  if (DDEServer.Get_MPlayer_Game_Info()) {
                    WWDebugString("RA95 - About to call Read_Game_Options.\n");
                    Read_Game_Options(nullptr);
                  } else
#endif
                    Read_Game_Options("C&CSPAWN.INI");

#ifdef WINSOCK_IPX
                  WWDebugString("RA95 - About to set addresses.\n");
                  PacketTransport->Set_Broadcast_Address(
                      PlanetWestwoodIPAddress);
#endif  // WINSOCK_IPX
                  if (PlanetWestwoodIsHost) {
                    WWDebugString(
                        "RA95 - About to call Server_Remote_Connect.\n");
                    if (Server_Remote_Connect()) {
                      WWDebugString(
                          "RA95 - Server_Remote_Connect returned success.\n");
                      break;
                    }
/*
 ** We failed to connect to the other player
 */
#ifdef WINSOCK_IPX
                    delete PacketTransport;
                    PacketTransport = nullptr;
#else   // WINSOCK_IPX
                    Winsock.Close();
#endif  // WINSOCK_IPX
                    Session.Type = GAME_NORMAL;
                    selection = SEL_NONE;
#ifdef _WIN32
                    DDEServer.Delete_MPlayer_Game_Info();  // Make sure we
                                                           // dont go round in
                                                           // an infinite loop
#endif
                    break;
                  }
                  WWDebugString(
                      "RA95 - About to call Client_Remote_Connect.\n");
                  if (Client_Remote_Connect()) {
                    WWDebugString(
                        "RA95 - Client_Remote_Connect returned success.\n");
                    break;
                  }
/*
 ** We failed to connect to the other player
 */
#ifdef WINSOCK_IPX
                  delete PacketTransport;
                  PacketTransport = nullptr;
#else   // WINSOCK_IPX
                  Winsock.Close();
#endif  // WINSOCK_IPX
                  Session.Type = GAME_NORMAL;
                  selection = SEL_NONE;
#ifdef _WIN32
                  DDEServer.Delete_MPlayer_Game_Info();  // Make sure we
                                                         // dont go round in
                                                         // an infinite loop
#endif
                  break;
                }
                Session.Type = Select_MPlayer_Game();
                if (Session.Type == GAME_NORMAL) {  // 'Cancel'
                  display = true;
                  selection = SEL_NONE;
                }
              } break;

#endif  // WIN32
#endif  //	!WOLAPI_INTEGRATION
            }
#ifdef WOLAPI_INTEGRATION
          }  //	if( !pWolapi )

          if (pWolapi) {
            Session.Type = GAME_INTERNET;
          }
#endif
          // debugprint( "Session.Type = %i\n", Session.Type );
          switch (Session.Type) {
            /*
            **	Modem, Null-Modem or internet
            */
            case GAME_MODEM:
            case GAME_NULL_MODEM:
#ifndef WOLAPI_INTEGRATION
            case GAME_INTERNET:
#endif
            case GAME_SKIRMISH:
              Theme.Fade_Out();
              process = false;
              Options.ScoreVolume = Options.MultiScoreVolume;
              break;

#ifdef WOLAPI_INTEGRATION  //	implies also WINSOCK_IPX
            case GAME_INTERNET:
              if (PacketTransport) {
                delete PacketTransport;
              }
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
                    selection = SEL_MULTIPLAYER_GAME;  // SEL_NONE;
                    delete PacketTransport;
                    PacketTransport = nullptr;
                    break;
                  case -1:
                    //	Patch was downloaded. Exit app.
                    Theme.Fade_Out();
                    BlackPalette.Set(FADE_PALETTE_SLOW);
                    return false;
                }
              } else {
                Session.Type = GAME_NORMAL;
                display = true;
                selection = SEL_MULTIPLAYER_GAME;  // SEL_NONE;
                delete PacketTransport;
                PacketTransport = nullptr;
              }
              break;
#endif

            /*
            **	Network (IPX): start a new network game.
            */
            case GAME_IPX:
              WWDebugString("RA95 - Game type is IPX.\n");
              /*
              ** Init network system & remote-connect
              */
#ifdef WINSOCK_IPX
              delete PacketTransport;
#ifdef PORTABLE
              // we don't even have IPX
              PacketTransport = new UDPInterfaceClass;
              PacketTransport->Set_Broadcast_Address((char*)"255.255.255.255");
#else
              //							if
              //(WWMessageBox().Process("Select a protocol to use for network
              // play.", "UDP", "IPX")) {
              PacketTransport = new IPXInterfaceClass;
              assert(PacketTransport != nullptr);
//							}else{
//								PacketTransport
//= new UDPInterfaceClass;	//IPXInterfaceClass;
// assert ( PacketTransport != nullptr);
// if (!Get_Broadcast_Addresses()) {
// Session.Type = GAME_NORMAL;
// display = true;
// selection = SEL_NONE;
// delete PacketTransport;
// PacketTransport = nullptr;
// break;
//								}
//							}
#endif
#endif  // WINSOCK_IPX
              WWDebugString("RA95 - About to call Init_Network.\n");
              if (Session.Type == GAME_IPX && Init_Network() &&
                  Remote_Connect()) {
                Options.ScoreVolume = Options.MultiScoreVolume;
                process = false;
                Theme.Fade_Out();
              } else {  // user hit cancel, or init failed
                Session.Type = GAME_NORMAL;
                display = true;
                selection = SEL_NONE;
#ifdef WINSOCK_IPX
                delete PacketTransport;
                PacketTransport = nullptr;
#endif  // WINSOCK_IPX
              }
              break;

#if (TEN)
            /*
            **	TEN: jump straight into the game
            */
            case GAME_TEN:
              if (Init_TEN()) {
                Options.ScoreVolume = Options.MultiScoreVolume;
                process = false;
                Theme.Fade_Out();
              } else {
                WWMessageBox().Process("Unable to initialize TEN!");
                // Prog_End();
                Emergency_Exit(1);
              }
              break;
#endif  // TEN

#if (MPATH)
            /*
            **	MPATH: jump straight into the game
            */
            case GAME_MPATH:
              if (Init_MPATH()) {
                Options.ScoreVolume = Options.MultiScoreVolume;
                process = false;
                Theme.Fade_Out();
              } else {
                WWMessageBox().Process("Unable to initialize MPATH!");
                // Prog_End();
                Emergency_Exit(1);
              }
              break;
#endif  // MPATH
          }
          break;

        /*
        **	Play a VQ
        */
        case SEL_INTRO:
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
          selection = SEL_NONE;
          break;

        /*
        **	Exit to DOS.
        */
        case SEL_EXIT:
          Theme.Fade_Out();
          BlackPalette.Set(FADE_PALETTE_SLOW);
          return false;

        /*
        **	Display the hall of fame.
        */
        case SEL_FAME:
          break;

        case SEL_TIMEOUT:
          if (Session.Attract && Session.RecordFile.Is_Available()) {
            Session.Play = true;
            if (Session.RecordFile.Open(READ)) {
              Load_Recording_Values(Session.RecordFile);
              process = false;
              Theme.Fade_Out();
            } else {
              Session.Play = false;
              selection = SEL_NONE;
            }
          } else {
            selection = SEL_NONE;
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
    if (Session.RecordFile.Open(WRITE)) {
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
#ifdef WOLAPI_INTEGRATION
      if (!pWolapi)
#endif
      {
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

    if (selection != SEL_START_NEW_GAME) {
      BlackPalette.Set(FADE_PALETTE_MEDIUM, Call_Back);
      HiddenPage.Clear();
      VisiblePage.Clear();
    }
    Show_Mouse();
    // Mono_Printf("About to call Start Scenario with %s\n", Scen.ScenarioName);
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
      7 * RESFACTOR,                 // font height in pixels
      -1, -1,                        // x,y for edit line (appears above msgs)
      0,                             // BG		1,
                                     // // enable edit overflow
      20,                            // min,
      MAX_MESSAGE_LENGTH - 14,       //    max for trimming overflow
      Lepton_To_Pixel(Map.TacLeptonWidth));  // Width in pixels of buffer

  if (Session.Type != GAME_NORMAL && Session.Type != GAME_SKIRMISH &&
      !Session.Play) {
    if (Session.Type == GAME_TEN) {
#if (TEN)
      Session.Create_TEN_Connections();
#endif  // TEN
    } else if (Session.Type == GAME_MPATH) {
#if (MPATH)
      Session.Create_MPATH_Connections();
#endif
    } else {
      Session.Create_Connections();
    }
  }

  /*
  ** If this isnt an internet game that set the unit build rate to its default
  *value
  */
  if (Session.Type != GAME_INTERNET) {
    UnitBuildPenalty = 100;
  }

  /*
  **	Hide the SeenPage; force the map to render one frame.  The caller can
  **	then fade the palette in.
  **	(If we loaded a game, this step will fade out the title screen.  If we
  **	started a scenario, Start_Scenario() will have played a couple of VQ
  **	movies, which will have cleared the screen to black already.)
  */
  Call_Back();
  Hide_Mouse();
  BlackPalette.Set(FADE_PALETTE_MEDIUM, Call_Back);
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

#ifdef WOLAPI_INTEGRATION

  // ajw debugging only
  //						debugprint( "Debugging
  // Session...\n" ); 						debugprint(
  // "Session.Players count is %i.\n", Session.Players.Count() );
  for (i = 0; i < Session.Players.Count(); i++) {
    NetNumType net;
    NetNodeType node;
    Session.Players[i]->Address.Get_Address(net, node);
    //							debugprint( "Player %i,
    //%s, color %i, ip %i.%i.%i.%i.%i.%i\n", i, Session.Players[i]->Name,
    //								Session.Players[i]->Player.Color,
    // node[0], node[1], node[2], node[3], node[4], node[5] );
  }
  //						debugprint(
  //"PlanetWestwoodPortNumber is %i\n", PlanetWestwoodPortNumber );

#endif

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
  static VQType _counter = VQ_FIRST;

  Keyboard->Clear();
  if (sequenced) {
    if (_counter <= VQ_FIRST) {
      _counter = VQ_COUNT;
    }
    if (_counter == VQ_COUNT) {
      _counter--;
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
#if RESFACTOR == 2
    Play_Movie(VQ_REDINTRO, THEME_NONE, false);
#else
    Play_Movie(VQ_TITLE, THEME_NONE, false);
#endif
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
#ifdef MOVIE640
GraphicBufferClass VQ640(640, 400, nullptr);
#endif
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
  AnimControl.ImageBuf = SysMemPage.Get_Offset();
#ifdef MOVIE640
  if (IsVQ640) {
    AnimControl.ImageWidth = 640;
    AnimControl.ImageHeight = 400;
    AnimControl.ImageBuf = VQ640.Get_Offset();
  }
#endif
  AnimControl.Vmode = 0;
  AnimControl.OptionFlags |= VQAOPTF_CAPTIONS | VQAOPTF_EVA;
  if (SlowPalette) {
    AnimControl.OptionFlags |= VQAOPTF_SLOWPAL;
  }
  AnimControl.AudioDeviceID = Get_Audio_Device();
  AnimControl.AudioCallback = Get_Audio_Callback_Ptr();
  AnimControl.AudioSpec = Get_Audio_Spec();
  if (MonoClass::Is_Enabled()) {
    AnimControl.OptionFlags |= VQAOPTF_MONO;
  }
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
bool Parse_Command_Line(int argc, char* argv[]) {
  /*
  **	Parse the command line and set globals to reflect the parameters
  **	passed in.
  */
  Whom = HOUSE_GOOD;
  Special.Init();

  MapEditorActive = false;
  Debug_Unshroud = false;

  for (int index = 1; index < argc; index++) {
    std::string original_arg = argv[index];  // Copy for preserving case.
    char* string = strupr(argv[index]);      // Pointer to argument.

    /*
    **	Print usage text only if requested.
    */
    if (stricmp("/?", string) == 0 || stricmp("-?", string) == 0 ||
        stricmp("-h", string) == 0 || stricmp("/h", string) == 0) {
      /*
      **	Unrecognized command line parameter... Display usage
      **	and then exit.
      */
      puts(TEXT_OPTIONS);
      return false;
    }

    bool processed = true;
    long ob = Obfuscate(string);

    /*
    **	Check to see if the parameter is a cheat enabling one.
    */
    const long* optr = &CheatCodes[0];
    while (*optr) {
      if (*optr++ == ob) {
        Debug_Playtest = true;
        Debug_Flag = true;
        break;
      }
    }

    /*
    **	Check to see if the parameter is a cheat enabling one.
    */
    optr = &PlayCodes[0];
    while (*optr) {
      if (*optr++ == ob) {
        Debug_Playtest = true;
        Debug_Flag = true;
        break;
      }
    }

    /*
    **	Check to see if the parameter is a scenario editor
    **	enabling one.
    */
    optr = &EditorCodes[0];
    while (*optr) {
      if (*optr++ == ob) {
        MapEditorActive = true;
        Debug_Unshroud = true;
        Debug_Flag = true;
        Debug_Playtest = true;
        break;
      }
    }

    switch (ob) {
      case PARM_PLAYTEST:
        if constexpr (config::kVirginCheatKeysEnabled) {
          Debug_Playtest = true;
        }
        break;

      /*
      ** Special flag - is C&C being run from the install program?
      */
      case PARM_INSTALL:
        Special.IsFromInstall = true;
        // If uncommented, will disable the <ESC> key during the first movie
        // run.
        //				BreakoutAllowed = false;
        break;

#if (TEN)
      case PARM_ALLOW_SOLO:
        Session.AllowSolo = 1;
        break;
#endif

#if (MPATH)
      case PARM_ALLOW_SOLO:
        Session.AllowSolo = 1;
        break;
#endif

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
      if (stricmp(string, "-CHECKMAP") == 0) {
        Debug_Check_Map = true;
        continue;
      }
    }

    /*
    **	File search path override.
    */
    if (strstr(string, "-CD")) {
      // Use original arg to preserve case-sensitive path on Unix systems
      CCFileClass::Add_Search_Drives(original_arg.substr(3));
      continue;
    }

#if (0)
    /*
    ** Build speed modifier
    */
    if (strstr(string, "-UNITRATE:")) {
      int unit_rate;
      sscanf(string, "-UNITRATE:%d", &unit_rate);
      UnitBuildPenalty = unit_rate;
    }
#endif  //(0)

    /*
    **	Specify destination connection for network play
    */
    if (strstr(string, "-DESTNET")) {
      NetNumType net;
      NetNodeType node;

      /*
      ** Scan the command-line string, pulling off each address piece
      */
      int i = 0;
      char* p = strtok(string + 8, ".");
      while (p) {
        int x;

        sscanf(p, "%x", &x);  // convert from hex string to int
        if (i < 4) {
          net[i] = static_cast<char>(x);  // fill NetNum
        } else {
          node[i - 4] = static_cast<char>(x);  // fill NetNode
        }
        i++;
        p = strtok(nullptr, ".");
      }

      /*
      ** If all the address components were successfully read, fill in the
      ** BridgeNet with a broadcast address to the network across the bridge.
      */
      if (i >= 4) {
        Session.IsBridge = 1;
        memset(node, 0xff, 6);
        Session.BridgeNet = IPXAddressClass(net, node);
      }
      continue;
    }

    /*
    **	Specify socket ID, as an offset from 0x4000.
    */
    if (strstr(string, "-SOCKET")) {
      unsigned short socket;

      socket = static_cast<unsigned short>(atoi(string + strlen("SOCKET")));
      socket += 0x4000;
      if (socket >= 0x4000 && socket < 0x8000) {
        Ipx.Set_Socket(socket);
      }
      continue;
    }

    /*
    **	Set the Net Stealth option
    */
    if (strstr(string, "-STEALTH")) {
      Session.NetStealth = true;
      continue;
    }

    /*
    **	Set the Net Protection option
    */
    if (strstr(string, "-MESSAGES")) {
      Session.NetProtect = false;
      continue;
    }

    /*
    **	Allow "attract" mode
    */
    if (strstr(string, "-ATTRACT")) {
      Session.Attract = true;
      continue;
    }

    /*
    ** Set screen to 640x480 instead of 640x400
    */
    if (strstr(string, "-480")) {
      ScreenHeight = 480;
      continue;
    }

    if constexpr (config::kCheatKeysEnabled) {
      // Specify the random number seed (for debugging)
      if (strstr(string, "-SEED")) {
        CustomSeed = static_cast<unsigned short>(atoi(string + strlen("SEED")));
        continue;
      }
    }

#if (TEN)
    /*
    **	Enable TEN
    */
    if (strstr(string, "TEN")) {
      if constexpr (config::kCheatKeysEnabled) {
        Debug_Flag = true;
        MonoClass::Enable();
      }

      Session.Type = GAME_TEN;
      Special.IsFromInstall = false;
      //
      // Create the Ten network manager.  This allows us to keep
      // the packet queues clean even while we're initializing the game,
      // so the queues don't fill up in case we're slow, or the user
      // didn't insert a CD.
      //
      Ten = new TenConnManClass();
      Ten->Init();
      strcpy(Session.OptionsFile, "OPTIONS.INI");
      Ten->Flush_All();
      continue;
    }

    /*
    **	Set the game options filename
    */
    if (strstr(string, "OPTIONS:")) {
      strcpy(Session.OptionsFile, string + 8);
      continue;
    }
#endif  // TEN

#if (MPATH)
    /*
    **	Enable MPATH
    */
    if (strstr(string, "MPATH")) {
      if constexpr (config::kCheatKeysEnabled) {
        Debug_Flag = true;
        MonoClass::Enable();
      }

      Session.Type = GAME_MPATH;
      Special.IsFromInstall = false;
      //
      // Create the MPath network manager.  This allows us to keep
      // the packet queues clean even while we're initializing the game,
      // so the queues don't fill up in case we're slow, or the user
      // didn't insert a CD.
      //
      MPath = new MPlayerManClass();
      MPath->Init();
      strcpy(Session.OptionsFile, "OPTIONS.INI");
      MPath->Flush_All();
      continue;
    }

    /*
    **	Set the game options filename
    */
    if (strstr(string, "OPTIONS:")) {
      strcpy(Session.OptionsFile, string + 8);
      continue;
    }
#endif  // MPATH

    if constexpr (config::kCheatKeysEnabled) {
      if (strstr(string, "-NOMOVIES")) {
        bNoMovies = true;
      }
    }

    /*
    ** Disable mouse grabbing for debugging
    */
    if (strstr(string, "-NOMOUSEGRAB")) {
      extern bool NoMouseGrab;
      NoMouseGrab = true;
    }

    /*
    **	Special command line control parsing.
    */
    if (absl::StartsWithIgnoreCase(string, "-X")) {
      string += strlen("-X");
      while (*string) {
        const char code = *string++;

        if constexpr (config::kCheatKeysEnabled) {
          switch (code) {
            case 'M':
              MonoClass::Enable();
              continue;
            case 'I':
              Special.IsInert = true;
              continue;
            case 'H':
              Special.IsSpeedBuild = true;
              continue;
            case 'X':
              Session.Record = 1;
              continue;
            case 'Y':
              Session.Play = 1;
              continue;
            case 'P':
              Debug_Print_Events = true;
              continue;
            default:
              break;
          }
        }

        switch (code) {
          case 'Q':
            Debug_Quiet = true;
            break;

          default:
            puts(TEXT_INVALID);
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
long Obfuscate(const char* string) {
  char buffer[128];

  if (!string) {
    return 0;
  }
  memset(buffer, '\xA5', sizeof(buffer));

  /*
  **	Copy key phrase into a working buffer. This hides any transformation
  *done *	to the string.
  */
  strncpy(buffer, string, sizeof(buffer));
  buffer[sizeof(buffer) - 1] = '\0';
  int length = strlen(buffer);

  /*
  **	Only upper case letters are significant.
  */
  strupr(buffer);

  /*
  **	Ensure that only visible ASCII characters compose the key phrase. This
  **	discourages the direct forced illegal character input method of attack.
  */
  for (int index = 0; index < length; index++) {
    if (!isgraph(buffer[index])) {
      buffer[index] = 'A' + index % 26;
    }
  }

  /*
  **	Increase the strength of even short pass phrases by extending the
  **	length to be at least a minimum number of characters. This helps prevent
  **	a weak pass phrase from compromising the obfuscation process. This
  **	process also forces the key phrase to be an even multiple of four.
  **	This is necessary to support the cypher process that occurs later.
  */
  if (length < 16 || length & 0x03) {
    int maxlen = std::max(length + 3 & 0x00FC, 16);
    int index;
    for (index = length; index < maxlen; index++) {
      buffer[index] = 'A' + (('?' ^ buffer[index - length]) + index) % 26;
    }
    length = index;
    buffer[length] = '\0';
  }

  /*
  **	Transform the buffer into a number. This transformation is character
  **	order dependant.
  */
  int32_t code = CrcEngine::Compute(buffer);

  /*
  **	Record a copy of this initial transformation to be used in a later
  **	self referential transformation.
  */
  int32_t copy = code;

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
    code ^= static_cast<unsigned char>(buffer[index]);
    unsigned char temp = static_cast<unsigned char>(code);
    buffer[index] ^= temp;
    code >>= 8;
    code |= static_cast<long>(temp) << 24;
  }

  /*
  **	Introduce loss into the vector. This strengthens the key against
  *traditional *	cryptographic attack engines. Since this also weakens
  *the key against *	unconventional attacks, the loss is limited to less than
  *10%.
  */
  for (int index = 0; index < length; index++) {
    static unsigned char _lossbits[] = {0x00, 0x08, 0x00, 0x20,
                                        0x00, 0x04, 0x10, 0x00};
    static unsigned char _addbits[] = {0x10, 0x00, 0x00, 0x80,
                                       0x40, 0x00, 0x00, 0x04};

    buffer[index] |= _addbits[index % std::size(_addbits)];
    buffer[index] &= ~_lossbits[index % std::size(_lossbits)];
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
    short key1 = buffer[index];
    short key2 = buffer[index + 1];
    short key3 = buffer[index + 2];
    short key4 = buffer[index + 3];
    short val1 = key1;
    short val2 = key2;
    short val3 = key3;
    short val4 = key4;

    val1 *= key1;
    val2 += key2;
    val3 += key3;
    val4 *= key4;

    short s3 = val3;
    val3 ^= val1;
    val3 *= key1;
    short s2 = val2;
    val2 ^= val4;
    val2 += val3;
    val2 *= key3;
    val3 += val2;

    val1 ^= val2;
    val4 ^= val3;

    val2 ^= s3;
    val3 ^= s2;

    buffer[index] = val1;
    buffer[index + 1] = val2;
    buffer[index + 2] = val3;
    buffer[index + 3] = val4;
  }

  /*
  **	Convert this final vector into a cypher key code to be
  **	returned by this routine.
  */
  code = CrcEngine::Compute(buffer);

  /*
  **	Return the final code value.
  */
  return static_cast<uint32_t>(code);
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
#ifdef PORTABLE
  int ms = Get_Time_Ms();
  CryptRandom.Seed_Byte(ms);
  // grab some more bits from somewhere?
#elifdef WIN32

  /*
  **	Gather some "random" bits from the system timer. Actually, only the
  **	low order millisecond bits are secure. The other bits could be
  **	easily guessed from the system clock (most clocks are fairly accurate
  **	and thus predictable).
  */
  SYSTEMTIME t;
  GetSystemTime(&t);
  CryptRandom.Seed_Byte(t.wMilliseconds);
  CryptRandom.Seed_Bit(t.wSecond);
  CryptRandom.Seed_Bit(t.wSecond >> 1);
  CryptRandom.Seed_Bit(t.wSecond >> 2);
  CryptRandom.Seed_Bit(t.wSecond >> 3);
  CryptRandom.Seed_Bit(t.wSecond >> 4);
  CryptRandom.Seed_Bit(t.wMinute);
  CryptRandom.Seed_Bit(t.wMinute >> 1);
  CryptRandom.Seed_Bit(t.wMinute >> 2);
  CryptRandom.Seed_Bit(t.wMinute >> 3);
  CryptRandom.Seed_Bit(t.wMinute >> 4);
  CryptRandom.Seed_Bit(t.wHour);
  CryptRandom.Seed_Bit(t.wDay);
  CryptRandom.Seed_Bit(t.wDayOfWeek);
  CryptRandom.Seed_Bit(t.wMonth);
  CryptRandom.Seed_Bit(t.wYear);
#else

  /*
  **	Gather some "random" bits from the DOS mode timer.
  */
  struct timeb t;
  ftime(&t);
  CryptRandom.Seed_Byte(t.millitm);
  CryptRandom.Seed_Byte(t.time);
#endif

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
    Scen.RandomNumber = Seed;
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
      Session.Type == GAME_SKIRMISH && !Session.Play) {
    /*
    ** Set the optional user-specified seed
    */
    if (CustomSeed != 0) {
      Seed = CustomSeed;
    } else {
      srand(time(nullptr));
      Seed = rand();
    }
  }

  /*
  **	Initialize the random-number generators
  */
  Scen.RandomNumber = Seed;
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
#if RESFACTOR == 2
  Load_Title_Screen("TITLE.PCX", &HidPage, CCPalette);
#else
  Load_Picture("TITLE.CPS", *HidPage.Get_Graphic_Buffer(),
               *HidPage.Get_Graphic_Buffer(), CCPalette, BM_DEFAULT);
#endif

  if (visible) {
    HidPage.Blit(SeenPage);
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

#if RESFACTOR == 2
  SysMemPage.Clear();
  Load_Picture("PALETTE.CPS", SysMemPage, SysMemPage, nullptr, BM_DEFAULT);
  SysMemPage.Blit(HidPage);
#else
  Load_Picture("PALETTE.CPS", HiddenPage, HiddenPage, nullptr, BM_DEFAULT);
#endif
  for (PlayerColorType pcolor = PCOLOR_FIRST; pcolor < PCOLOR_COUNT; pcolor++) {
    unsigned char* ptr = ColorRemaps[pcolor].RemapTable;

    for (int color = 0; color < 256; color++) {
      ptr[color] = color;
    }

    for (int index = 0; index < 16; index++) {
      ptr[HidPage.Get_Pixel(index, 0)] = HidPage.Get_Pixel(index, pcolor);
    }
    for (int index = 0; index < 6; index++) {
      ColorRemaps[pcolor].FontRemap[10 + index] =
          HidPage.Get_Pixel(2 + index, pcolor);
    }
    ColorRemaps[pcolor].BrightColor = WHITE;
    //		ColorRemaps[pcolor].BrightColor = HidPage.Get_Pixel(1, pcolor);
    ColorRemaps[pcolor].Color = HidPage.Get_Pixel(4, pcolor);

    ColorRemaps[pcolor].Shadow = HidPage.Get_Pixel(10, pcolor);
    ColorRemaps[pcolor].Background = HidPage.Get_Pixel(9, pcolor);
    ColorRemaps[pcolor].Corners = HidPage.Get_Pixel(7, pcolor);
    ColorRemaps[pcolor].Highlight = HidPage.Get_Pixel(4, pcolor);
    ColorRemaps[pcolor].Bright = HidPage.Get_Pixel(0, pcolor);
    ColorRemaps[pcolor].Underline = HidPage.Get_Pixel(0, pcolor);
    ColorRemaps[pcolor].Bar = HidPage.Get_Pixel(6, pcolor);

    /*
    **	This must grab from column 4 because the multiplayer color dialog
    *palette counts *	on this to be true.
    */
    ColorRemaps[pcolor].Box = HidPage.Get_Pixel(4, pcolor);
  }

  /*
  ** Now do the special dim grey scheme
  */
  for (int color = 0; color < 256; color++) {
    GreyScheme.RemapTable[color] = color;
  }
  for (int index = 0; index < 6; index++) {
    GreyScheme.FontRemap[10 + index] =
        HidPage.Get_Pixel(9 + index, PCOLOR_GREY) & 0x00FF;
  }
  GreyScheme.BrightColor = HidPage.Get_Pixel(3, PCOLOR_GREY) & 0x00FF;
  GreyScheme.Color = HidPage.Get_Pixel(7, PCOLOR_GREY) & 0x00FF;

  GreyScheme.Shadow =
      ColorRemaps[PCOLOR_GREY]
          .RemapTable[HidPage.Get_Pixel(15, PCOLOR_GREY) & 0x00FF];
  GreyScheme.Background =
      ColorRemaps[PCOLOR_GREY]
          .RemapTable[HidPage.Get_Pixel(14, PCOLOR_GREY) & 0x00FF];
  GreyScheme.Corners =
      ColorRemaps[PCOLOR_GREY]
          .RemapTable[HidPage.Get_Pixel(13, PCOLOR_GREY) & 0x00FF];
  GreyScheme.Highlight =
      ColorRemaps[PCOLOR_GREY]
          .RemapTable[HidPage.Get_Pixel(9, PCOLOR_GREY) & 0x00FF];
  GreyScheme.Bright =
      ColorRemaps[PCOLOR_GREY]
          .RemapTable[HidPage.Get_Pixel(5, PCOLOR_GREY) & 0x00FF];
  GreyScheme.Underline =
      ColorRemaps[PCOLOR_GREY]
          .RemapTable[HidPage.Get_Pixel(5, PCOLOR_GREY) & 0x00FF];
  GreyScheme.Bar = ColorRemaps[PCOLOR_GREY]
                       .RemapTable[HidPage.Get_Pixel(11, PCOLOR_GREY) & 0x00FF];
  GreyScheme.Box = ColorRemaps[PCOLOR_GREY]
                       .RemapTable[HidPage.Get_Pixel(11, PCOLOR_GREY) & 0x00FF];

  /*
  ** Set up the metallic remap table for the font that prints over the tabs
  */
  memset(&MetalScheme, 4, sizeof(MetalScheme));
  for (int color_counter = 0; color_counter < 16; color_counter++) {
    MetalScheme.FontRemap[color_counter] = color_counter;
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
    ColorRemaps[PCOLOR_TYPE].FontRemap[colr] =
        HidPage.Get_Pixel(colr, PCOLOR_TYPE);
  }

  ColorRemaps[PCOLOR_TYPE].Shadow = 11;
  ColorRemaps[PCOLOR_TYPE].Background = 10;
  ColorRemaps[PCOLOR_TYPE].Corners = 10;
  ColorRemaps[PCOLOR_TYPE].Highlight = 9;
  ColorRemaps[PCOLOR_TYPE].Bright = 15;
  ColorRemaps[PCOLOR_TYPE].Underline = 11;
  ColorRemaps[PCOLOR_TYPE].Bar = 11;
  ColorRemaps[PCOLOR_TYPE].Box = 10;
  ColorRemaps[PCOLOR_TYPE].BrightColor = 15;
  ColorRemaps[PCOLOR_TYPE].Color = 9;

  GadgetClass::Set_Color_Scheme(&ColorRemaps[PCOLOR_DIALOG_BLUE]);
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
  Houses.Set_Heap(HOUSE_MAX);
  TriggerTypes.Set_Heap(Rule.TrigTypeMax);
  //	Weapons.Set_Heap(Rule.WeaponMax);

  /*
  **	Speech holding tank buffer. Since speech does not mix, it can be placed
  **	into a custom holding tank only as large as the largest speech file to
  **	be played.
  */
  for (int index = 0; index < ARRAY_SIZE(SpeechBuffer); index++) {
    SpeechBuffer[index] = new char[SPEECH_BUFFER_SIZE];
    SpeechRecord[index] = VOX_NONE;
    assert(SpeechBuffer[index] != nullptr);
  }

  /*
  **	Allocate the theater buffer block.
  */
  TheaterBuffer = new Buffer(THEATER_BUFFER_SIZE);
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
  FindFileState state;
  if (Find_First_File("SC*.MIX", state)) {
    do {
      // scores shouldn't be loaded here but may be found if main has been
      // extracted
      if (stricmp(state.name, "scores.mix") == 0) {
        continue;
      }
      MFCD::Register(state.name, &FastKey, &CryptRandom);
      MFCD::Cache(state.name);
    } while (Find_Next_File(state));
  }
  if (Find_First_File("SS*.MIX", state)) {
    do {
      MFCD::Register(state.name, &FastKey, &CryptRandom);
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
  Metal12FontPtr = MFCD::Retrieve("12METFNT.FNT");
  MapFontPtr = MFCD::Retrieve("HELP.FNT");
  Font6Ptr = MFCD::Retrieve("6POINT.FNT");
  GradFont6Ptr = MFCD::Retrieve("GRAD6FNT.FNT");
  EditorFont = MFCD::Retrieve("EDITFNT.FNT");
  Font8Ptr = MFCD::Retrieve("8POINT.FNT");
  FontPtr = (char*)Font8Ptr;
  Set_Font(FontPtr);
  Font3Ptr = MFCD::Retrieve("3POINT.FNT");
  ScoreFontPtr = MFCD::Retrieve("SCOREFNT.FNT");
  FontLEDPtr = MFCD::Retrieve("LED.FNT");
  VCRFontPtr = MFCD::Retrieve("VCR.FNT");
  TypeFontPtr = MFCD::Retrieve("8POINT.FNT");  //("TYPE.FNT"); //VG 10/17/96
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
  if (!CCFileClass::Is_There_Search_Drives()) {
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
    int error;

    do {
      error = CCFileClass::Add_Search_Drives("?:\\");
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
  int temp = RequiredCD;
  RequiredCD = -2;

#ifdef WOLAPI_INTEGRATION
  CCFileClass fileWolapiMix("WOLAPI.MIX");
  if (fileWolapiMix.Is_Available()) {
    MFCD::Register("WOLAPI.MIX", &FastKey, &CryptRandom);
    MFCD::Cache("WOLAPI.MIX");
  }
#endif

  CCFileClass file2("EXPAND2.MIX");
  if (file2.Is_Available()) {
    MFCD::Register("EXPAND2.MIX", &FastKey, &CryptRandom);
    bool ok = MFCD::Cache("EXPAND2.MIX");
    assert(ok);

#if RESFACTOR == 2
    MFCD::Register("HIRES1.MIX", &FastKey, &CryptRandom);
    ok = MFCD::Cache("HIRES1.MIX");
    assert(ok);
#else
    MFCD::Register("LORES1.MIX", &FastKey, &CryptRandom);
    ok = MFCD::Cache("LORES1.MIX");
    assert(ok);
#endif
  }

  CCFileClass file("EXPAND.MIX");
  if (file.Is_Available()) {
    MFCD::Register("EXPAND.MIX", &FastKey, &CryptRandom);
    bool ok = MFCD::Cache("EXPAND.MIX");
    assert(ok);
  }

  MFCD::Register("REDALERT.MIX", &FastKey, &CryptRandom);

  /*
  **	Bootstrap enough of the system so that the error dialog box can
  *successfully *	be displayed.
  */
  MFCD::Register("LOCAL.MIX", &FastKey, &CryptRandom);  // Cached.
  bool ok = MFCD::Cache("LOCAL.MIX");
  assert(ok);

#if RESFACTOR == 2
  MFCD::Register("HIRES.MIX", &FastKey, &CryptRandom);
  ok = MFCD::Cache("HIRES.MIX");
  assert(ok);

  MFCD::Register("NCHIRES.MIX", &FastKey,
                 &CryptRandom);  // Non-cached hires stuff incl VQ palettes
#else
  MFCD::Register("LORES.MIX", &FastKey, &CryptRandom);
  ok = MFCD::Cache("LORES.MIX");
  assert(ok);
#endif  // WIN32

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
void Extract(char* filename, char* outfile);

static void Init_Secondary_Mixfiles() {
  if (CCFileClass("MAIN1.MIX").Is_Available()) {
    // MAIN1-4 from steam

    // extract the extra missions from the expansion "discs"
    // (they don't contain the base missions)
    if (CCFileClass("MAIN3.MIX").Is_Available() &&
        !CCFileClass("GENERAL3.MIX").Is_Available()) {
      MFCD* tmp = MFCD::Register("MAIN3.MIX", &FastKey, &CryptRandom);
      Extract("GENERAL.MIX", "GENERAL3.MIX");
      delete tmp;
    }

    if (CCFileClass("MAIN4.MIX").Is_Available() &&
        !CCFileClass("GENERAL4.MIX").Is_Available()) {
      MFCD* tmp = MFCD::Register("MAIN4.MIX", &FastKey, &CryptRandom);
      Extract("GENERAL.MIX", "GENERAL4.MIX");
      Extract("SCORES.MIX", "SCORES.MIX");  // also extract scores
      delete tmp;
    }

    // load the first two to get both movies
    MFCD::Register("MAIN2.MIX", &FastKey, &CryptRandom);
    MFCD::Register("MAIN1.MIX", &FastKey, &CryptRandom);

    // load extra missions
    MFCD::Register("GENERAL4.MIX", &FastKey, &CryptRandom);
    MFCD::Register("GENERAL3.MIX", &FastKey, &CryptRandom);
  } else {
    // assume regular/TFD files
    MainMix = MFCD::Register("MAIN.MIX", &FastKey, &CryptRandom);
    assert(MainMix != nullptr);
  }

// Denzil extract mixfile
#ifdef DENZIL_MIXEXTRACT
#if (0)
  Extract("CONQUER.MIX", "o:\\projects\\radvd\\data\\extract\\conquer.mix");
  Extract("EDHI.MIX", "o:\\projects\\radvd\\data\\extract\\edhi.mix");
  Extract("EDLO.MIX", "o:\\projects\\radvd\\data\\extract\\edlo.mix");
  Extract("GENERAL.MIX", "o:\\projects\\radvd\\data\\extract\\general.mix");
  Extract("INTERIOR.MIX", "o:\\projects\\radvd\\data\\extract\\interior.mix");
  Extract("MOVIES1.MIX", "o:\\projects\\radvd\\data\\extract\\movies1.mix");
  Extract("SCORES.MIX", "o:\\projects\\radvd\\data\\extract\\scores.mix");
  Extract("SNOW.MIX", "o:\\projects\\radvd\\data\\extract\\snow.mix");
  Extract("SOUNDS.MIX", "o:\\projects\\radvd\\data\\extract\\sounds.mix");
  Extract("RUSSIAN.MIX", "o:\\projects\\radvd\\data\\extract\\russian.mix");
  Extract("ALLIES.MIX", "o:\\projects\\radvd\\data\\extract\\allies.mix");
  Extract("TEMPERAT.MIX", "o:\\projects\\radvd\\data\\extract\\temperat.mix");
#else
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
#endif

  /*
  **	Inform the file system of the various MIX files.
  */
  ConquerMix =
      MFCD::Register("CONQUER.MIX", &FastKey, &CryptRandom);  // Cached.
  //	MFCD::Register("TRANSIT.MIX", &FastKey, &CryptRandom);

  if (GeneralMix == nullptr) {
    GeneralMix =
        MFCD::Register("GENERAL.MIX", &FastKey, &CryptRandom);  // Never cached.
  }

  if (CCFileClass("MOVIES1.MIX").Is_Available()) {
    MoviesMix =
        MFCD::Register("MOVIES1.MIX", &FastKey, &CryptRandom);  // Never cached.
  }
  // load both sets of movies if possible
  if (CCFileClass("MOVIES2.MIX").Is_Available()) {
    MoviesMix =
        MFCD::Register("MOVIES2.MIX", &FastKey, &CryptRandom);  // Never cached.
  }
  assert(MoviesMix != nullptr);

  /*
  **	Register the score mixfile.
  */
  ScoresPresent = true;
  ScoreMix = MFCD::Register("SCORES.MIX", &FastKey, &CryptRandom);
  ThemeClass::Scan();

  /*
  **	These are sound card specific, but the install program would have
  **	copied the correct versions to the hard drive.
  */
  MFCD::Register("SPEECH.MIX", &FastKey, &CryptRandom);   // Never cached.
  MFCD::Register("SOUNDS.MIX", &FastKey, &CryptRandom);   // Cached.
  MFCD::Register("RUSSIAN.MIX", &FastKey, &CryptRandom);  // Cached.
  MFCD::Register("ALLIES.MIX", &FastKey, &CryptRandom);   // Cached.
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
  if (CCFileClass::Is_There_Search_Drives()) {
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
  **	Perform any special debug-only processing. This includes preparing the
  **	monochrome screen.
  */
  Mono_Clear_Screen();

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

#ifndef WIN32
  /*
  **	Install the hard error handler.
  */
  _harderr(harderr_handler);  // BG: Install hard error handler

  /*
  ** Install a Page Fault handler
  */
  if (UsePageFaultHandler) {
    Install_Page_Fault_Handle();
  }
#endif

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
  Set_Shape_Buffer(new unsigned char[SHAPE_BUFFER_SIZE], SHAPE_BUFFER_SIZE);

  SystemStrings = MFCD::RetrieveData(Language_Name("CONQUER"));
  DebugStrings = MFCD::RetrieveData("DEBUG.ENG");

  /*
  **	Default palette initialization.
  */
  const void* palette_data = MFCD::Retrieve("TEMPERAT.PAL");
  if (!palette_data) {
    fprintf(stderr,
            "ERROR: Cannot find TEMPERAT.PAL - game data files not found!\n");
    fprintf(stderr, "Please specify data directory with: -CD<path>\n");
    fprintf(stderr,
            "Example: ./rasdl "
            "-CD\"/home/konsto/.local/share/Steam/steamapps/common/"
            "Command & Conquer Red Alert\"\n");
    exit(1);
  }
  memmove(&GamePalette[0], palette_data, 768L);
  WhitePalette[0] = BlackPalette[0];
  //	GamePalette.Set();

  /*
  **	Initialize expansion files (if present). Expansion files must be located
  **	in the current directory.
  */
  Init_Expansion_Files();

  SidebarScheme.Background = BLACK;
  SidebarScheme.Corners = LTGREY;
  SidebarScheme.Shadow = DKGREY;
  SidebarScheme.Highlight = WHITE;
  SidebarScheme.Color = LTGREY;
  SidebarScheme.Bright = WHITE;
  SidebarScheme.BrightColor = WHITE;
  SidebarScheme.Box = LTGREY;
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
#if defined(WIN32) && !defined(PORTABLE)
  ShowCursor(false);
#endif
  if (MouseInstalled) {
    const void* temp_mouse_shapes = MFCD::Retrieve("MOUSE.SHP");
    if (temp_mouse_shapes) {
      Set_Mouse_Cursor(0, 0, Extract_Shape(temp_mouse_shapes, 0));
      while (Get_Mouse_State() > 1) {
        Show_Mouse();
      }
    }
  } else {
    char buffer[255];
    GamePalette.Set();
    GamePalette.Set();
    sprintf(buffer, TEXT_NO_MOUSE);
    VisiblePage.Clear();
    WWMessageBox().Process(buffer, TXT_OK);
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
  MFCD::Cache("CONQUER.MIX");
  if (SampleType != 0 && !Debug_Quiet) {
    MFCD::Cache("SOUNDS.MIX");
    MFCD::Cache("RUSSIAN.MIX");
    MFCD::Cache("ALLIES.MIX");
  }
  Call_Back();

  /*
  **	Fetch the tutorial message data.
  */
  INIClass ini;
  CCFileClass fc("TUTORIAL.INI");
  ini.Load(fc);
  int totallen = 0;
  for (int index = 0; index < ARRAY_SIZE(TutorialTextOffsets); index++) {
    TutorialTextOffsets[index] = 0xFFFF;

    char buffer[128];
    char num[10];
    sprintf(num, "%d", index);
    if (ini.Get_String("Tutorial", num, "", buffer, sizeof(buffer))) {
      totallen += strlen(buffer) + 1;
    }
  }

  // now allocate and copy
  TutorialTextData = new char[totallen];
  char* textptr = (char*)TutorialTextData;

  for (int index = 0; index < ARRAY_SIZE(TutorialTextOffsets); index++) {
    char num[10];
    sprintf(num, "%d", index);
    int textoffset = textptr - TutorialTextData;
    if (ini.Get_String("Tutorial", num, "", textptr, totallen - textoffset)) {
      TutorialTextOffsets[index] = textoffset;
      textptr += strlen(textptr) + 1;
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
  RAMFileClass file(keys.data(), keys.size());
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
bool Save_Recording_Values(CCFileClass& file) {
  Session.Save(file);
  file.Write(&BuildLevel, sizeof(BuildLevel));
  file.Write(&Debug_Unshroud, sizeof(Debug_Unshroud));
  file.Write(&Seed, sizeof(Seed));
  file.Write(&Scen.Scenario, sizeof(Scen.Scenario));
  file.Write(Scen.ScenarioName, sizeof(Scen.ScenarioName));
  file.Write(&Whom, sizeof(Whom));
  file.Write(&Special, sizeof(SpecialClass));
  file.Write(&Options, sizeof(GameOptionsClass));

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
bool Load_Recording_Values(CCFileClass& file) {
  Session.Load(file);
  file.Read(&BuildLevel, sizeof(BuildLevel));
  file.Read(&Debug_Unshroud, sizeof(Debug_Unshroud));
  file.Read(&Seed, sizeof(Seed));
  file.Read(&Scen.Scenario, sizeof(Scen.Scenario));
  file.Read(Scen.ScenarioName, sizeof(Scen.ScenarioName));
  file.Read(&Whom, sizeof(Whom));
  file.Read(&Special, sizeof(SpecialClass));
  file.Read(&Options, sizeof(GameOptionsClass));
  return true;
}

extern "C" {
void __PRO() {
  //	printf("_pro\n");
}
}

void Extract(char* filename, char* outname) {
  CCFileClass inFile(filename);
  CCFileClass outFile(outname);

  inFile.Open();
  outFile.Open(WRITE);

  auto buffer = std::make_unique<char[]>(32768);

  unsigned long size = inFile.Size();
  unsigned long bytes;

  while (size > 0) {
    bytes = inFile.Read(buffer.get(), 32768);
    outFile.Write(buffer.get(), bytes);
    size -= bytes;
  }
}

bool bUsingDVD = false;

const char* Game_Registry_Key();

//***********************************************************************************************
bool Is_DVD_Installed() {
#ifdef _WIN32
  bool bInstalled;
  HKEY hKey;
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, Game_Registry_Key(), 0, KEY_READ,
                   &hKey) != ERROR_SUCCESS) {
    return false;
  }
  DWORD dwValue;
  DWORD dwBufSize = sizeof(DWORD);
  if (RegQueryValueEx(hKey, "DVD", 0, nullptr, (LPBYTE)&dwValue, &dwBufSize) !=
      ERROR_SUCCESS) {
    bInstalled = false;
  } else {
    bInstalled = (bool)dwValue;  //	(Presumably true, if it's there...)
  }

  RegCloseKey(hKey);

  return bInstalled;
#else
  return true;
#endif
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
