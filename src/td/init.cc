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

/* $Header:   F:\projects\c&c\vcs\code\init.cpv   2.18   16 Oct 1995 16:50:16
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
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
 *   Init_Game -- Main game initialization routine. * Load_Recording_Values --
 *Loads recording values from recording file                       *
 *   Parse_Command_Line -- Parses the command line parameters. * Parse_INI_File
 *-- Parses CONQUER.INI for special options                                  *
 *   Play_Intro -- plays the introduction & logo movies * Save_Recording_Values
 *-- Saves recording values to a recording file                       *
 *   Select_Game -- The game's main menu * Version_Number -- Determines the
 *version number.                                          *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/init.h"

#include <algorithm>
#include <array>
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
#include <vector>

#include "absl/log/log.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/buffer.h"
#include "port/random_seed.h"
#include "port/safe_string.h"
#include "port/tokenizer.h"
#include "sdllib/file.h"
#include "sdllib/file_access.h"
#include "sdllib/font.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/playcd.h"
#include "sdllib/shape.h"
#include "sdllib/timer.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "td/aircraft.h"
#include "td/anim.h"
#include "td/base.h"
#include "td/building.h"
#include "td/bullet.h"
#include "td/config.h"
#include "td/conquer.h"
#include "td/defines.h"
#include "td/dialog.h"
#include "td/expand.h"
#include "td/externs.h"
#include "td/factory.h"
#include "td/globals.h"
#include "td/goptions.h"
#include "td/heap.h"
#include "td/house.h"
#include "td/infantry.h"
#include "td/ini.h"
#include "td/inline.h"
#include "td/intro.h"
#include "td/ipx.h"
#include "td/ipxaddr.h"
#include "td/ipxmgr.h"
#include "td/jshell.h"
#include "td/loaddlg.h"
#include "td/logic.h"
#include "td/mapedit.h"
#include "td/menus.h"
#include "td/mouse.h"
#include "td/mplayer.h"
#include "td/msgbox.h"
#include "td/msglist.h"
#include "td/netdlg.h"
#include "td/nulldlg.h"
#include "td/nullmgr.h"
#include "td/overlay.h"
#include "td/palette.h"
#include "td/queue.h"
#include "td/randomstate.h"
#include "td/saveload.h"
#include "td/scenario.h"
#include "td/score.h"
#include "td/smudge.h"
#include "td/special.h"
#include "td/target.h"
#include "td/tcpip.h"
#include "td/team.h"
#include "td/teamtype.h"
#include "td/template.h"
#include "td/terrain.h"
#include "td/theme.h"
#include "td/trigger.h"
#include "td/type.h"
#include "td/unit.h"
#include "tech/audio_mixer.h"
#include "tech/disk_file.h"
#include "tech/game_file.h"
#include "tech/key_phrase_hash.h"
#include "tech/mix_archive.h"
#include "tech/number_parse.h"
#include "tech/search_paths.h"
#include "winvq/vqa32/vqaplay.h"

#ifdef _WIN32
#include "td/ccdde.h"

#endif

static std::vector<uint8_t> shape_storage;

/****************************************
**	Function prototypes for this module **
*****************************************/
static void Play_Intro(bool for_real = false);

#define ATTRACT_MODE_TIMEOUT 3600  // timeout for attract mode


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
  std::span<const std::byte> temp_mouse_shapes;

  /*
  **	Initialize the game object heaps.
  */
  DLOG(INFO) << "C&C95 - About to enter Units.Set_Heap";
  Units.Set_Heap(kUnitMax);
  DLOG(INFO) << "C&C95 - About to enter Factories.Set_Heap";
  Factories.Set_Heap(kFactoryMax);
  DLOG(INFO) << "C&C95 - About to enter Terrains.Set_Heap";
  Terrains.Set_Heap(kTerrainMax);
  DLOG(INFO) << "C&C95 - About to enter Templates.Set_Heap";
  Templates.Set_Heap(kTemplateMax);
  DLOG(INFO) << "C&C95 - About to enter Smudges.Set_Heap";
  Smudges.Set_Heap(kSmudgeMax);
  DLOG(INFO) << "C&C95 - About to enter Overlays.Set_Heap";
  Overlays.Set_Heap(kOverlayMax);
  DLOG(INFO) << "C&C95 - About to enter Infantry.Set_Heap";
  Infantry.Set_Heap(kInfantryMax);
  DLOG(INFO) << "C&C95 - About to enter Bullets.Set_Heap";
  Bullets.Set_Heap(kBulletMax);
  DLOG(INFO) << "C&C95 - About to enter Buildings.Set_Heap";
  Buildings.Set_Heap(kBuildingMax);
  DLOG(INFO) << "C&C95 - About to enter Anims.Set_Heap";
  Anims.Set_Heap(kAnimMax);
  DLOG(INFO) << "C&C95 - About to enter Aircraft.Set_Heap";
  Aircraft.Set_Heap(kAircraftMax);
  DLOG(INFO) << "C&C95 - About to enter Triggers.Set_Heap";
  Triggers.Set_Heap(kTriggerMax);
  DLOG(INFO) << "C&C95 - About to enter TeamTypes.Set_Heap";
  TeamTypes.Set_Heap(kTeamTypeMax);
  DLOG(INFO) << "C&C95 - About to enter Teams.Set_Heap";
  Teams.Set_Heap(kTeamMax);
  DLOG(INFO) << "C&C95 - About to enter Houses.Set_Heap";
  Houses.Set_Heap(kHouseMax);

  /*
  **	Initialize all the waypoints to invalid values.
  */
  DLOG(INFO) << "C&C95 - About to clear waypoints";
  base::FillBytes(base::ObjectBytes(Waypoint), 0xFF, sizeof(Waypoint));

  /*
  **	Setup the keyboard processor in preparation for the game.
  */
  DLOG(INFO) << "C&C95 - About to do various keyboard inits";
#ifdef FIX_ME_LATER
  Keyboard_Attributes_Off(TRACKEXT | PAUSEON | BREAKON | SCROLLLOCKON |
                          CTRLSON | CTRLCON | PASSBREAKS | FILTERONLY |
                          TASKSWITCHABLE);
#endif  // FIX_ME_LATER
  Keyboard::Clear();
  Kbd.Clear();

  /*
  **	This is the shape staging buffer. It must always be available, so it is
  **	allocated here and never freed. The library sets the globals ShapeBuffer
  **	and ShapeBufferSize to these values, so it can be accessed for other
  **	purposes.
  */
  DLOG(INFO) << "C&C95 - About to call Set_Shape_Buffer";
  shape_storage.resize(SHAPE_BUFFER_SIZE);
  Set_Shape_Buffer(shape_storage);

  /*
  **	Bootstrap enough of the system so that the error dialog box can
  *sucessfully *	be displayed.
  */
  DLOG(INFO) << "C&C95 - About to register CCLOCAL.MIX";
#ifdef DEMO
  (void)MixArchive::Register("DEMOL.MIX");
  MixArchive::Cache("DEMOL.MIX");
#else
  const int temp = RequiredCD;
  RequiredCD = -2;

  (void)MixArchive::Register("CCLOCAL.MIX");
  MixArchive::Cache("CCLOCAL.MIX");
  DLOG(INFO) << "C&C95 - About to register UPDATE.MIX";
  (void)MixArchive::Register("UPDATE.MIX");
  DLOG(INFO) << "C&C95 - About to register UPDATEC.MIX";
  (void)MixArchive::Register("UPDATEC.MIX");
  MixArchive::Cache("UPDATEC.MIX");

#ifdef JAPANESE
  DLOG(INFO) << "C&C95 - About to register LANGUAGE.MIX";
  (void)MixArchive::Register("LANGUAGE.MIX");
#endif  // JAPANESE

  RequiredCD = temp;

#endif
  DLOG(INFO) << "C&C95 - About to load fonts";
  GameFile f("12GREEN.FNT");
  static std::vector<std::byte> green12_font_ptr_storage;
  green12_font_ptr_storage = LoadAllocData(f);
  Green12FontPtr = green12_font_ptr_storage;
  f.Open("12GRNGRD.FNT");
  static std::vector<std::byte> green12_grad_font_ptr_storage;
  green12_grad_font_ptr_storage = LoadAllocData(f);
  Green12GradFontPtr = green12_grad_font_ptr_storage;
  f.Open("8FAT.FNT");
  static std::vector<std::byte> map_font_ptr_storage;
  map_font_ptr_storage = LoadAllocData(f);
  MapFontPtr = map_font_ptr_storage;
  Font8Ptr = MixArchive::RetrieveData(FONT8);
  FontPtr = Font8Ptr;
  Set_Font(FontPtr);
  Font3Ptr = MixArchive::RetrieveData(FONT3);
  //	Font6Ptr = MixArchive::RetrieveData(FONT6);
  f.Open("6POINT.FNT");
  static std::vector<std::byte> font6_ptr_storage;
  font6_ptr_storage = LoadAllocData(f);
  Font6Ptr = font6_ptr_storage;
  // ScoreFontPtr = MixArchive::RetrieveData("12GRNGRD.FNT");	//GRAD12FN");
  // //("SCOREFNT.FNT");
  f.Open("12GRNGRD.FNT");
  static std::vector<std::byte> score_font_ptr_storage;
  score_font_ptr_storage = LoadAllocData(f);
  ScoreFontPtr = score_font_ptr_storage;
  FontLEDPtr = MixArchive::RetrieveData("LED.FNT");
  VCRFontPtr = MixArchive::RetrieveData("VCR.FNT");
  //	GradFont6Ptr = MixArchive::RetrieveData("GRAD6FNT.FNT");
  f.Open("GRAD6FNT.FNT");
  static std::vector<std::byte> grad_font6_ptr_storage;
  grad_font6_ptr_storage = LoadAllocData(f);
  GradFont6Ptr = grad_font6_ptr_storage;
  BlackPalette.assign(768, 0);
  GamePalette.assign(768, 0);
  OriginalPalette.assign(768, 0);
  WhitePalette.assign(768, 0);
  std::ranges::fill(WhitePalette, 63);

  DLOG(INFO) << "C&C95 - About to set palette";
  std::ranges::fill(BlackPalette, 0x01);
  if (!Special.IsFromInstall) {
    Set_Palette(BlackPalette);
  }
  std::ranges::fill(BlackPalette, 0);
  if (!Special.IsFromInstall) {
    Set_Palette(BlackPalette);
    DLOG(INFO) << "C&C95 - About to clear visible page";
    VisiblePage.Clear();
  }

  Set_Palette(GamePalette);

  DLOG(INFO) << "C&C95 - About to set the mouse shape";
  /*
  ** Since there is no mouse shape currently available we need'
  ** to set one of our own.
  */
  if (MouseInstalled) {
    temp_mouse_shapes = MixArchive::RetrieveData("MOUSE.SHP");
    if (!temp_mouse_shapes.empty()) {
      Set_Mouse_Cursor(0, 0, Extract_Shape(temp_mouse_shapes, 0));
      while (Get_Mouse_State() > 1) {
        Show_Mouse();
      }
    }
  }

  DLOG(INFO) << "C&C95 - About to enter wait for focus loop";
  /*
  ** Process the message loop until we are in focus.
  */
  do {
    DLOG(INFO) << "C&C95 - About to call Keyboard::Check";
    Keyboard::Check();
  } while (!GameInFocus);
  AllSurfaces.SurfacesRestored = false;

  DLOG(INFO) << "C&C95 - About to load the language file";
  SystemStrings = MixArchive::RetrieveData(Language_Name("CONQUER"));

  /*
  **	Default palette initialization. Uses the desert palette for convenience,
  **	but only the non terrain specific colors matter.
  */
  GameFile palfile("TEMPERAT.PAL");
  palfile.Read(std::span(GamePalette), 768L);

  if (!MouseInstalled) {
    char buffer[255];
    Set_Palette(GamePalette);
#ifdef GERMAN
    sprintf(buffer, "Command & Conquer kann Ihren Maustreiber nicht finden..");
#else
#ifdef FRENCH
    sprintf(
        buffer,
        "Command & Conquer ne peut pas détecter votre gestionnaire de souris.");
#else
    absl::SNPrintF(buffer, sizeof(buffer),
                   "Command & Conquer is unable to detect your mouse driver.");
#endif
#endif
    CCMessageBox().Process(buffer, TXT_OK);
    Prog_End();
    exit(1);
  }

#ifdef DEMO
  /*
  **	Add in any override path specified in the conquer.ini file.
  */
  if (strlen(OverridePath)) {
    GameFile::Set_Search_Drives(OverridePath);
  }
#endif

  SearchPaths::Add(".");  // allow running without CD

  DLOG(INFO) << "C&C95 - About to search for CD drives";
  /*
  **	Always try to look at the CD-ROM for data files.
  */
  if (!SearchPaths::HasAny()) {
    /*
    ** If there are no search drives specified then we must be playing
    ** off cd, so read files from there.
    */
    int error = 0;

    do {
      if (!CDList.Get_Number_Of_Drives()) {
        Set_Palette(GamePalette);
        Show_Mouse();
        CCMessageBox().Process(TXT_CD_ERROR1, TXT_OK);
        Prog_End();
        exit(EXIT_FAILURE);
      }
      SearchPaths::SetCdDrive(CDList.Get_First_CD_Drive());

      error = SearchPaths::Add("?:\\");
      switch (error) {
        case 1:
          Set_Palette(GamePalette);
          Show_Mouse();
          CCMessageBox().Process(TXT_CD_ERROR1, TXT_OK);
          Prog_End();
          exit(EXIT_FAILURE);

        case 2:
          Set_Palette(GamePalette);
          Show_Mouse();
          if (CCMessageBox().Process(TXT_CD_DIALOG_1, TXT_OK, TXT_CANCEL) ==
              1) {
            Prog_End();
            exit(EXIT_FAILURE);
          }
          Hide_Mouse();
          break;

        default:
          Show_Mouse();
          if (!Force_CD_Available(RequiredCD)) {
            Prog_End();
            exit(EXIT_FAILURE);
          }
          Hide_Mouse();
          break;
      }
    } while (error);

#ifdef DEMO
    RequiredCD = -2;
#else
    RequiredCD = -1;
#endif
  } else {
    /*
    ** If there are search drives specified then all files are to be
    ** considered local.
    */
    RequiredCD = -2;
  }
#ifndef DEMO
  DLOG(INFO) << "C&C95 - About to register addon mixfiles";
  /*
  **	Before all else, cache any additional mixfiles.
  */
  FindFileState state{};
  if (Find_First_File("SC*.MIX", state)) {
    do {
      // don't cache scores
      if (absl::EqualsIgnoreCase(state.name, "scores.mix")) {
        continue;
      }

      (void)MixArchive::Register(state.name);
      MixArchive::Cache(state.name);
    } while (Find_Next_File(state));
  }
  if (Find_First_File("SS*.MIX", state)) {
    do {
      (void)MixArchive::Register(state.name);
    } while (Find_Next_File(state));
  }
#endif  // DEMO

  DLOG(INFO) << "C&C95 - About to register GENERAL.MIX";
  MixArchive::Unregister("GENERAL.MIX");
  GeneralMix = MixArchive::Register("GENERAL.MIX");

  //	if (!_dos_findfirst("SC*.MIX", _A_NORMAL, &ff)) {
  //		do {
  //			new MixFileClass(ff.name);
  //			MixArchive::Cache(ff.name);
  //		} while(!_dos_findnext(&ff));
  //	}

  /*
  **	Inform the file system of the various MIX files.
  */
#ifdef DEMO
  (void)MixArchive::Register("DEMO.MIX");
  if (GameFile("DEMOM.MIX").IsAvailable()) {
    if (!MoviesMix) {
      MoviesMix = MixArchive::Register("DEMOM.MIX");
    }
    ScoresPresent = true;
    ThemeClass::Scan();
  }

#else
  DLOG(INFO) << "C&C95 - About to register CONQUER.MIX";
  (void)MixArchive::Register("CONQUER.MIX");
  DLOG(INFO) << "C&C95 - About to register TRANSIT.MIX";
  (void)MixArchive::Register("TRANSIT.MIX");

  DLOG(INFO) << "C&C95 - About to register GENERAL.MIX";
  if (!GeneralMix) {
    GeneralMix = MixArchive::Register("GENERAL.MIX");  // Never cached.
  }

  //	if (GameFile("MOVIES.MIX").IsAvailable()) {
  DLOG(INFO) << "C&C95 - About to register MOVIES.MIX";
  if (!MoviesMix) {
    MoviesMix = MixArchive::Register("MOVIES.MIX");  // Never cached.
                                                     //	}
  }

  /*
  **	Register the score mixfile.
  */
  DLOG(INFO) << "C&C95 - About to register SCORES.MIX";
  ScoresPresent = false;
  //	if (GameFile("SCORES.MIX").IsAvailable()) {
  ScoresPresent = true;
  if (!ScoreMix) {
    ScoreMix = MixArchive::Register("SCORES.MIX");
    ThemeClass::Scan();
  }
//	}
#endif

  /*
  **	These are sound card specific, but the install program would have
  **	copied the coorect versions to the hard drive.
  */
  DLOG(INFO) << "C&C95 - About to register SPEECH.MIX";
  if (GameFile("SPEECH.MIX").IsAvailable()) {
    (void)MixArchive::Register("SPEECH.MIX");  // Never cached.
  }
  DLOG(INFO) << "C&C95 - About to register SOUNDS.MIX";
  (void)MixArchive::Register("SOUNDS.MIX");

  /*
  **	Initialize the animation system.
  */
  DLOG(INFO) << "C&C95 - About to initialise the animation system";
  Anim_Init();

  if (SpawnedFromWChat) {
    Special.IsFromWChat = true;
  }

  /*
  **	Play the introduction movies.
  */
  DLOG(INFO) << "C&C95 - About to play the intro movie";
  if (!Special.IsFromInstall && !Special.IsFromWChat) {
    Play_Intro(true);
  }

  /*
  **	Wait for a VSync; during the vertical blank, set the game palette & blit
  **	the title screen.  We must ensure no RGB values in the game palette
  *match *	those in the WWLIB's 'CurrentPalette', or the WWLIB palette-set
  *routine *	will skip that color; the VQ player will have changed that color
  *(behind *	WWLIB's back), so it will be incorrect.
  */
  base::FillBytes(base::ObjectBytes(CurrentPalette), 0x01, 768);

  if (!Special.IsFromInstall) {
    Load_Title_Page(true);
  }

  Hide_Mouse();
  Wait_Vert_Blank();
  if (!Special.IsFromInstall) {
    Set_Palette(Palette);
    HidPage.Blit(SeenBuff);
    Show_Mouse();
  }
  Call_Back();
  //	Window_Dialog_Box(hCCLibrary, "DIALOG_1", MainWindow,
  // MakeProcInstance((FARPROC)Start_Game_Proc, hInstance)); 	if (hCCLibrary)
  // FreeLibrary(hCCLibrary);

#ifdef DEMO
  MixArchive::Cache("DEMO.MIX");
  MixArchive::Cache("SOUNDS.MIX");
#else
  /*
  **	Cache the main game data. This operation can take a very long time.
  */
  MixArchive::Cache("CONQUER.MIX");
  if (Audio.is_open() && !Debug_Quiet) {
    MixArchive::Cache("SOUNDS.MIX");
    if (Special.IsJuvenile) {
      (void)MixArchive::Register("ZOUNDS.MIX");
      MixArchive::Cache("ZOUNDS.MIX");
    }
  }
  Call_Back();
#endif

  //	malloc(2);

#ifdef ONHOLD
  /*
  ** Check for addition options not specified on the command-line.  This must
  ** be done before the One_Time calls, but after the shape buffer is set up.
  */
  Parse_INI_File();
#endif

  /*
  **	Perform one-time game system initializations.
  */
  Call_Back();
  //	malloc(3);
  Map.One_Time();
  //	malloc(4);
  Logic.One_Time();
  //	malloc(5);
  Options.One_Time();

  //	malloc(6);

  ObjectTypeClass::One_Time();
  BuildingTypeClass::One_Time();
  BulletTypeClass::One_Time();

  TemplateTypeClass::One_Time();
  OverlayTypeClass::One_Time();
  SmudgeTypeClass::One_Time();
  TerrainTypeClass::One_Time();
  UnitTypeClass::One_Time();

  InfantryTypeClass::One_Time();
  AnimTypeClass::One_Time();
  AircraftTypeClass::One_Time();
  HouseClass::One_Time();

  /*
  **	Speech holding tank buffer. Since speech does not mix, it can be placed
  **	into a custom holding tank only as large as the largest speech file to
  **	be played.
  */
  SpeechBuffer.resize(SPEECH_BUFFER_SIZE);
  Call_Back();

  /*
  **	WWLIB bug: MouseState is in some undefined state; show the mouse until
  **	it really shows.
  */
  Map.Set_Default_Mouse(MOUSE_NORMAL, false);
  Show_Mouse();
  // #ifdef FIX_ME_LATER
  while (Get_Mouse_State() > 0) {
    Show_Mouse();
  }
  // #endif //FIX_ME_LATER
  Call_Back();

#ifndef DEMO
  /*
  **	Load multiplayer scenario descriptions
  */
  Read_Scenario_Descriptions();
#endif

  /*
  **	Initialize the multiplayer score values
  */
  MPlayerGamesPlayed = 0;
  MPlayerNumScores = 0;
  MPlayerCurGame = 0;
  for (auto& i : MPlayerScore) {
    base::At(i.Name, 0) = '\0';
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
  std::ranges::copy(Palette, GamePalette.begin());
  std::ranges::copy(Palette, OriginalPalette.begin());

  /*
  **	Read game options, so the GameSpeed is initialized when multiplayer
  ** dialogs are invoked.  (GameSpeed must be synchronized between systems.)
  */
  Options.Load_Settings();

  return true;
}

void Uninit_Game() {
  delete MouseClass::ShadowPage;
  MouseClass::ShadowPage = nullptr;
  Map.Free_Cells();

  SpeechBuffer.clear();
  SpeechBuffer.shrink_to_fit();

  SearchPaths::Clear();
  MixArchive::Free_All();

  Units.Set_Heap(0);
  Factories.Set_Heap(0);
  Terrains.Set_Heap(0);
  Templates.Set_Heap(0);
  Smudges.Set_Heap(0);
  Overlays.Set_Heap(0);
  Infantry.Set_Heap(0);
  Bullets.Set_Heap(0);
  Buildings.Set_Heap(0);
  Anims.Set_Heap(0);
  Aircraft.Set_Heap(0);
  Triggers.Set_Heap(0);
  TeamTypes.Set_Heap(0);
  Teams.Set_Heap(0);
  Houses.Set_Heap(0);

  Set_Shape_Buffer({});
  shape_storage.clear();
  shape_storage.shrink_to_fit();
  BlackPalette.clear();
  GamePalette.clear();
  OriginalPalette.clear();
  WhitePalette.clear();

  Palette.clear();
  Palette.clear();  // Prog_End may run again when SDL handles the quit event.
}

extern int ShowCommand;

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
extern int Com_Fake_Scenario_Dialog();
extern int Com_Show_Fake_Scenario_Dialog();

// A single legacy dialog loop; splitting it is a refactor of its own.
// NOLINTNEXTLINE(readability-function-size)
bool Select_Game(bool fade) {
  if (DebugQuitAtFrame >= 0 && DebugNewGame.empty() && DebugLoadGame < 0) {
    return false;
  }
  constexpr int kSelTimeout = -1;  // main menu timeout--go into attract mode
#ifdef NEWMENU
  constexpr int kSelNewScenario =
      kSelTimeout + 1;  // Expansion scenario to play.
#endif
  constexpr int kSelStartNewGame = kSelNewScenario + 1;  // start a new game
#ifdef BONUS_MISSIONS
  constexpr int kSelBonusMissions = kSelStartNewGame + 1;
#endif  // BONUS_MISSIONS
  constexpr int kSelInternet = kSelBonusMissions + 1;
  constexpr int kSelLoadMission = kSelInternet + 1;  // load a saved game
  constexpr int kSelMultiplayerGame =
      kSelLoadMission + 1;  // play modem/null-modem/network game
  constexpr int kSelIntro = kSelMultiplayerGame + 1;  // replay the intro
  constexpr int kSelExit = kSelIntro + 1;             // exit to DOS
  constexpr int kSelFame = kSelExit + 1;              // view the hall of fame
  constexpr int kSelNone = kSelFame + 1;  // placeholder default value
  bool gameloaded = false;  // Has the game been loaded from the menu?
  int selection = 0;        // the default selection
  bool process = true;      // false = break out of while loop
  bool display = true;
  CountDownTimerClass count;
  int cd_index = 0;

  /*
  ** Enable the DDE Server so we can get internet start game packets from WChat
  */
#ifdef _WIN32
  DDEServer.Enable();
#endif

  if (Special.IsFromInstall) {
    {
      display = false;
      Show_Mouse();
    }
  }

  /*
  **	[Re]set any globals that need it, in preparation for a new scenario
  */
  GameActive = true;
  DoList.Init();
  OutList.Init();
  Frame = 0;
  PlayerWins = false;
  PlayerLoses = false;
  MPlayerObiWan = false;
  Debug_Unshroud = false;
  Map.Set_Cursor_Shape({});
  Map.PendingObjectPtr = nullptr;
  Map.PendingObject = nullptr;
  Map.PendingHouse = HOUSE_NONE;

  /*
  ** Initialize multiplayer-protocol-specific variables:
  ** If CommProtocol MULTI_E_COMP is used, you must:
  ** Init FrameSendRate to a sensible value (3 is good)
  ** Init MPlayerMaxAhead to an even multiple of FrameSendRate, and it must
  **   be at least 2 * MPlayerMaxAhead
  */
  CommProtocol = COMM_PROTOCOL_SINGLE_NO_COMP;
  if (!Special.IsFromWChat) {
    FrameSendRate = 3;
  }

  ProcessTicks = 0;
  ProcessFrames = 0;
  DesiredFrameRate = 30;
  // #if(TIMING_FIX)
  NewMaxAheadFrame1 = 0;
  NewMaxAheadFrame2 = 0;
  // #endif

  /*
  **	Init multiplayer game scores.  Let Wins accumulate; just init the
  *current
  ** Kills for this game.  Kills of -1 means this player didn't play this round.
  */
  for (int i = 0; i < MAX_MULTI_GAMES; i++) {
    base::At(base::At(MPlayerScore, i).Kills, MPlayerCurGame) = -1;
  }

  /*
  **	Set default mouse shape
  */
  Map.Set_Default_Mouse(MOUSE_NORMAL, false);

  /*
  **	If the last game we played was a multiplayer game, jump right to that
  **	menu by pre-setting 'selection'.
  */
  if (GameToPlay == GAME_NORMAL) {
    selection = kSelNone;
  } else {
    selection = kSelMultiplayerGame;
  }

  /*
  **	Main menu processing; only do this if we're not in editor mode.
  */
  if (!Debug_Map) {
    /*
    **	Menu selection processing loop
    */
    ScenarioInit++;
    Theme.Queue_Song(THEME_MAP1);
    ScenarioInit--;

    /*
    ** If we're playing back a recording, load all pertinant values & skip
    ** the menu loop.  Hide the now-useless mouse pointer.
    */
    if (PlaybackGame && RecordFile.IsAvailable()) {
      if (RecordFile.Open(FileAccess::kRead)) {
        Load_Recording_Values();
        process = false;
        Theme.Fade_Out();
      } else {
        PlaybackGame = false;
      }
    }

    /*
    ** Handle case where we were spawned from Wchat
    */
    if (SpawnedFromWChat) {
      Special.IsFromInstall =
          false;  // Dont play intro if we were spawned from wchat
      selection = kSelInternet;
      Theme.Queue_Song(THEME_NONE);
      GameToPlay = GAME_INTERNET;
      display = false;
      Set_Logic_Page(SeenBuff);
    }

    while (process) {
      if (DebugNewGame.size() >= 5) {
        Scenario = tech::ParseIntegerOr<int>(
            std::string_view{DebugNewGame}.substr(3, 2), 0);
        ScenPlayer =
            DebugNewGame.at(2) == 'B' ? SCEN_PLAYER_NOD : SCEN_PLAYER_GDI;
        Whom = ScenPlayer == SCEN_PLAYER_NOD ? HOUSE_BAD : HOUSE_GOOD;
        GameToPlay = GAME_NORMAL;
        process = false;
        continue;
      }
      if (DebugLoadGame >= 0) {
        const int slot = DebugLoadGame;
        DebugLoadGame = -1;
        if (!Load_Game(slot)) {
          LOG(ERROR) << "-LOADGAME: could not load slot " << slot;
          return false;
        }
        gameloaded = true;
        process = false;
        continue;
      }
      /*
      ** If we have just received input focus again after running in the
      *background then
      ** we need to redraw.
      */
      if (AllSurfaces.SurfacesRestored) {
        AllSurfaces.SurfacesRestored = false;
        display = true;
      }

      /*
      **	Redraw the title page if needed
      */
      if (display) {
        Hide_Mouse();

        /*
        **	Display the title page; fade it in if this is the first time
        **	through the loop, and the 'fade' flag is true
        */
        Load_Title_Page(true);
        std::ranges::copy(Palette, GamePalette.begin());

        if (fade) {
          Fade_Palette_To(Palette, kFadePaletteSlow, Call_Back);
          fade = false;
        }

        Set_Logic_Page(SeenBuff);
        if constexpr (config::kVirginCheatKeysEnabled) {
          Fancy_Text_Print(
              "V.%d%s", SeenBuff.Get_Width() - 1, SeenBuff.Get_Height() - 10,
              kGrey, kTBlack, TPF_6POINT | TPF_FULLSHADOW | TPF_RIGHT,
              Version_Number(), VersionText, FOREIGN_VERSION_NUMBER);
        } else {
#ifdef DEMO
          Version_Number();
          Fancy_Text_Print("DEMO V%s", SeenBuff.Get_Width() - 1,
                           SeenBuff.Get_Height() - 10, kGrey, kTBlack,
                           TPF_6POINT | TPF_FULLSHADOW | TPF_RIGHT,
                           VersionText);
#else
          Fancy_Text_Print("V.%d%s", SeenBuff.Get_Width() - 1,
                           SeenBuff.Get_Height() - 10, kGrey, kTBlack,
                           TPF_6POINT | TPF_FULLSHADOW | TPF_RIGHT,
                           Version_Number(), VersionText);
#endif
        }
        display = false;
        Show_Mouse();
      }

      /*
      **	Display menu and fetch selection from player.
      */
      if (Special.IsFromInstall) {
        selection = kSelStartNewGame;
        Theme.Queue_Song(THEME_NONE);
      }

#ifdef _WIN32
      /*
      ** Handle case where we were spawned from Wchat
      */
      if (Special.IsFromWChat && DDEServer.Get_MPlayer_Game_Info()) {
        Check_From_WChat(NULL);
        selection = kSelMultiplayerGame;
        Theme.Queue_Song(THEME_NONE);
        GameToPlay = GAME_INTERNET;
      } else {
        /*
        ** We werent spawned but we could still receive a DDE packet from wchat
        */
        if (DDEServer.Get_MPlayer_Game_Info()) {
          Check_From_WChat(NULL);
          /*
          ** Make sure top and bottom of screen are clear in 640x480 mode
          */
          if (ScreenHeight == 480) {
            VisiblePage.Fill_Rect(0, 0, 639, 40, 0);
            VisiblePage.Fill_Rect(0, 440, 639, 479, 0);
          }
        }
      }
#endif

      if (selection == kSelNone) {
        //				selection = Main_Menu(0);
        selection = Main_Menu(ATTRACT_MODE_TIMEOUT);
      }
      Call_Back();

      switch (selection) {
#ifdef NEWMENU

        case kSelInternet:
          /*
          ** Only call up the internet menu code if we dont already have connect
          *info from WChat
          */
#ifdef _WIN32
          if (!DDEServer.Get_MPlayer_Game_Info()) {
            DLOG(INFO) << "C&C95 - About to call Internet Menu.";
            if (Do_The_Internet_Menu_Thang() &&
                DDEServer.Get_MPlayer_Game_Info()) {
              DLOG(INFO) << "C&C95 - About to call Check_From_WChat.";
              Check_From_WChat(NULL);
              selection = kSelMultiplayerGame;
              display = false;
              GameToPlay = GAME_INTERNET;
            } else {
              selection = kSelNone;
              display = true;
            }
          } else {
            DLOG(INFO) << "C&C95 - About to call Check_From_WChat.";
            Check_From_WChat(NULL);
            display = false;
            GameToPlay = GAME_INTERNET;
            selection = kSelMultiplayerGame;
          }
#endif
          break;

        /*
        **	Pick an expansion scenario.
        */
        case kSelNewScenario:
          CarryOverMoney = 0;
          if (Expansion_Dialog()) {
            Theme.Fade_Out();
            //						Theme.Queue_Song(THEME_AOI);
            GameToPlay = GAME_NORMAL;
            process = false;
          } else {
            display = true;
            selection = kSelNone;
          }
          break;

#ifdef BONUS_MISSIONS

        /*
        **	User selected to play a bonus scenario.
        */
        case kSelBonusMissions:
          CarryOverMoney = 0;

          /*
          ** Ensure that CD1 or CD2 is in the drive. These missions
          ** are not on the covert CD.
          */
          cd_index = Get_CD_Index(SearchPaths::current_cd_drive(), 1 * 60);
          /*
          ** If cd_index == 2 then its a covert CD
          */
          if (cd_index == 2) {
            RequiredCD = 0;
            if (!Force_CD_Available(RequiredCD)) {
              Prog_End();
              exit(EXIT_FAILURE);
            }
          }

          if (Bonus_Dialog()) {
            Theme.Fade_Out();
            GameToPlay = GAME_NORMAL;
            process = false;
          } else {
            display = true;
            selection = kSelNone;
          }
          break;

#endif  // BONUS_MISSIONS

#endif

        /*
        **	SEL_START_NEW_GAME: Play the game
        */
        case kSelStartNewGame:
          CarryOverMoney = 0;

#ifdef DEMO
          Hide_Mouse();
          Fade_Palette_To(BlackPalette, kFadePaletteMedium, Call_Back);
          Load_Title_Screen("PREPICK.PCX", &HidPage, Palette);
          HidPage.Blit(SeenBuff);
          Fade_Palette_To(Palette, kFadePaletteMedium, Call_Back);
          Clear_KeyBuffer();
          while (!Check_Key_Num()) {
            Call_Back();
          }
          Get_Key_Num();
          Fade_Palette_To(BlackPalette, kFadePaletteMedium, Call_Back);
          Show_Mouse();

          Scenario = 1;
          BuildLevel = 1;
#else
          Scenario = 1;
          BuildLevel = 1;
#endif
          ScenPlayer = SCEN_PLAYER_GDI;
          ScenDir = SCEN_DIR_EAST;
          Whom = HOUSE_GOOD;

#ifndef DEMO
          Theme.Fade_Out();
          Choose_Side();
#endif

          /*
          ** If user is playing special mode, do NOT change Whom; leave it set
          *to
          ** GDI or NOD.  Ini.cpp will set the player's ActLike to mirror the
          ** Whom value.
          */
          if (Special.IsJurassic && AreThingiesEnabled) {
            ScenPlayer = SCEN_PLAYER_JP;
            ScenDir = SCEN_DIR_EAST;
          }

          GameToPlay = GAME_NORMAL;
          process = false;
          break;

        /*
        **	Load a saved game.
        */
        case kSelLoadMission:
          if (LoadOptionsClass(LoadOptionsClass::LOAD).Process()) {
            // Theme.Fade_Out();
            Theme.Queue_Song(THEME_AOI);
            process = false;
            gameloaded = true;
          } else {
            display = true;
            selection = kSelNone;
          }
          break;

        /*
        **	SEL_MULTIPLAYER_GAME: set 'GameToPlay' to NULL-modem, modem, or
        **	network play.
        */
        case kSelMultiplayerGame:

#ifdef DEMO
          Hide_Mouse();
          Set_Palette(BlackPalette);
          Load_Title_Screen("DEMOPIC.PCX", &HidPage, Palette);
          HidPage.Blit(SeenBuff);
          Fade_Palette_To(Palette, kFadePaletteMedium, Call_Back);
          Clear_KeyBuffer();
          while (!Check_Key()) {
            Call_Back();
          }
          Get_Key();
          Fade_Palette_To(BlackPalette, kFadePaletteMedium, Call_Back);
          Show_Mouse();
          display = true;
          fade = true;
          selection = kSelNone;
#else
          switch (GameToPlay) {
            /*
            **	If 'GameToPlay' isn't already set up for a multiplayer game,
            **	we must prompt the user for which type of multiplayer game
            **	they want.
            */
            case GAME_NORMAL:
              GameToPlay = Select_MPlayer_Game();
              if (GameToPlay == GAME_NORMAL) {  // 'Cancel'
                display = true;
                selection = kSelNone;
              }
              break;

            case GAME_NULL_MODEM:
            case GAME_MODEM:
              if (NullModem.Num_Connections()) {
                NullModem.Init_Send_Queue();

                if ((GameToPlay == GAME_NULL_MODEM &&
                     ModemGameToPlay == MODEM_NULL_HOST) ||
                    (GameToPlay == GAME_MODEM &&
                     ModemGameToPlay == MODEM_DIALER)) {
                  if (!Com_Scenario_Dialog()) {
                    GameToPlay = Select_Serial_Dialog();
                    if (GameToPlay == GAME_NORMAL) {  // user hit Cancel
                      display = true;
                      selection = kSelNone;
                    }
                  }
                } else {
                  if (!Com_Show_Scenario_Dialog()) {
                    GameToPlay = Select_Serial_Dialog();
                    if (GameToPlay == GAME_NORMAL) {  // user hit Cancel
                      display = true;
                      selection = kSelNone;
                    }
                  }
                }
              } else {
                GameToPlay = Select_MPlayer_Game();
                if (GameToPlay == GAME_NORMAL) {  // 'Cancel'
                  display = true;
                  selection = kSelNone;
                }
              }
              break;

#ifdef FORCE_WINSOCK
            /*
            ** Handle being spawned from WChat. Intermnet play based on IPX code
            *now.
            */
            case GAME_INTERNET:
              DLOG(INFO) << "C&C95 - case GAME_INTERNET:";
              if (Special.IsFromWChat) {
                // MessageBox (NULL, "About to restore focus to C&C95", "C&C95",
                // MB_OK);
                DLOG(INFO) << "C&C95 - About to give myself focus.";

                DLOG(INFO) << "C&C95 - About to initialise Winsock.";
                if (Winsock.Init()) {
                  DLOG(INFO) << "C&C95 - About to read multiplayer settings.";
                  Read_MultiPlayer_Settings();
                  Server = PlanetWestwoodIsHost;

                  DLOG(INFO) << "C&C95 - About to set addresses.";
                  Winsock.Set_Host_Address(PlanetWestwoodIPAddress);

                  DLOG(INFO)
                      << "C&C95 - About to call Start_Server or Start_Client.";
                  if (Server) {
                    ModemGameToPlay = INTERNET_HOST;
                    Winsock.Start_Server();
                  } else {
                    ModemGameToPlay = INTERNET_JOIN;
                    Winsock.Start_Client();
                  }

                  // #if (0)
                  /*
                  ** Flush out any pending packets from a previous game.
                  */
                  DLOG(INFO) << "C&C95 - About to flush packet queue.";
                  DLOG(INFO) << "C&C95 - Allocating scrap memory.";
                  std::array<std::byte, 1024> temp_buffer{};

                  DLOG(INFO) << "C&C95 - Creating timer class instance.";
                  CountDownTimerClass ptimer;

                  DLOG(INFO) << "C&C95 - Entering read loop.";
                  while (Winsock.Read(temp_buffer, 1024)) {
                    DLOG(INFO) << "C&C95 - Discarding a packet.";
                    ptimer.Set(30, true);
                    while (ptimer.Time()) {
                    }
                    DLOG(INFO) << "C&C95 - Ready to check for more packets.";
                  }
                  DLOG(INFO) << "C&C95 - About to delete scrap memory.";

                } else {
                  DLOG(INFO) << "C&C95 - Winsock failed to initialise.";
                  GameToPlay = GAME_NORMAL;
                  selection = kSelExit;
                  Special.IsFromWChat = false;
                  break;
                }

                DLOG(INFO) << "C&C95 - About to call Init_Network.";
                Init_Network();

#ifdef _WIN32
                if (DDEServer.Get_MPlayer_Game_Info()) {
                  DLOG(INFO) << "C&C95 - About to call Read_Game_Options.";
                  Read_Game_Options(NULL);
                } else
#endif
                  Read_Game_Options("C&CSPAWN.INI");

                if (Server) {
                  DLOG(INFO) << "C&C95 - About to call Server_Remote_Connect.";
                  if (Server_Remote_Connect()) {
                    DLOG(INFO)
                        << "C&C95 - Server_Remote_Connect returned success.";
                    break;
                  }
                  /*
                   ** We failed to connect to the other player
                   *
                   *   SEND FAILURE PACKET TO WCHAT HERE !!!!!
                   *
                   */
                  Winsock.Close();
                  GameToPlay = GAME_NORMAL;
                  selection = kSelNone;
#ifdef _WIN32
                  DDEServer.Delete_MPlayer_Game_Info();  // Make sure we dont
                                                         // go round in an
                                                         // infinite loop
#endif
                  // Special.IsFromWChat = false;
                  break;
                }
                DLOG(INFO) << "C&C95 - About to call Client_Remote_Connect.";
                if (Client_Remote_Connect()) {
                  DLOG(INFO)
                      << "C&C95 - Client_Remote_Connect returned success.";
                  break;
                }
                /*
                 ** We failed to connect to the other player
                 *
                 *   SEND FAILURE PACKET TO WCHAT HERE !!!!!
                 *
                 */
                Winsock.Close();
                GameToPlay = GAME_NORMAL;
                selection = kSelNone;
#ifdef _WIN32
                DDEServer.Delete_MPlayer_Game_Info();  // Make sure we dont
                                                       // go round in an
                                                       // infinite loop
#endif
                // Special.IsFromWChat = false;
                break;
              }
              GameToPlay = Select_MPlayer_Game();
              if (GameToPlay == GAME_NORMAL) {  // 'Cancel'
                display = true;
                selection = kSelNone;
              }
              break;

#endif  // FORCE_WINSOCK
            case GameType::GAME_IPX:
            default:
              break;
          }

          switch (GameToPlay) {
            /*
            **	Internet, Modem or Null-Modem
            */
            case GAME_MODEM:
            case GAME_NULL_MODEM:
            case GAME_INTERNET:
              Theme.Fade_Out();
              ScenPlayer = SCEN_PLAYER_2PLAYER;
              ScenDir = SCEN_DIR_EAST;
              process = false;
              Options.ScoreVolume = 0;
              break;

            /*
            **	Network (IPX): start a new network game.
            */
            case GAME_IPX:
              /*
              ** Init network system & remote-connect
              */
              if (Init_Network() && Remote_Connect()) {
                Options.ScoreVolume = 0;
                ScenPlayer = SCEN_PLAYER_MPLAYER;
                ScenDir = SCEN_DIR_EAST;
                process = false;
                Theme.Fade_Out();
              } else {  // user hit cancel, or init failed
                GameToPlay = GAME_NORMAL;
                display = true;
                selection = kSelNone;
              }
              break;
            case GameType::GAME_NORMAL:
            default:
              break;
          }
#endif
          break;

        /*
        **	Play a VQ
        */
        case kSelIntro:
          Theme.Fade_Out();
          Theme.Stop();
          Call_Back();

          Force_CD_Available(-1);
          Play_Intro(false);
          Hide_Mouse();

          // verify existence of movie file before playing this sequence.
          if (GameFile("TRAILER.VQA").IsAvailable()) {
            Fade_Palette_To(BlackPalette, kFadePaletteMedium, Call_Back);
            VisiblePage.Clear();
            if (GameFile("ATTRACT2.CPS").IsAvailable()) {
              GameFile f("ATTRACT2.CPS");
              Load_Uncompress(f, SysMemPage, SysMemPage, Palette);
              SysMemPage.Scale(SeenBuff, 0, 0, 0, 0, 320, 199, 640, 398);
              Fade_Palette_To(Palette, kFadePaletteMedium, Call_Back);
            }
            Clear_KeyBuffer();
            count.Set(int64_t{kTimerSecond} * 3);
            while (count.Time()) {
              Call_Back();
            }
            Fade_Palette_To(BlackPalette, kFadePaletteMedium, Call_Back);

            Play_Movie("TRAILER");  // Red Alert teaser.
          }

          if (GameFile("SIZZLE.VQA").IsAvailable()) {
            Fade_Palette_To(BlackPalette, kFadePaletteMedium, Call_Back);
            VisiblePage.Clear();
            if (GameFile("ATTRACT2.CPS").IsAvailable()) {
              GameFile f("ATTRACT2.CPS");
              Load_Uncompress(f, SysMemPage, SysMemPage, Palette);
              SysMemPage.Scale(SeenBuff, 0, 0, 0, 0, 320, 199, 640, 398);
              Fade_Palette_To(Palette, kFadePaletteMedium, Call_Back);
            }
            Clear_KeyBuffer();
            count.Set(int64_t{kTimerSecond} * 3);
            while (count.Time()) {
              Call_Back();
            }
            Fade_Palette_To(BlackPalette, kFadePaletteMedium, Call_Back);

            Play_Movie("SIZZLE");  // Red Alert teaser.
          }

          if (GameFile("SIZZLE2.VQA").IsAvailable()) {
            Fade_Palette_To(BlackPalette, kFadePaletteMedium, Call_Back);
            VisiblePage.Clear();
            if (GameFile("ATTRACT2.CPS").IsAvailable()) {
              GameFile f("ATTRACT2.CPS");
              Load_Uncompress(f, SysMemPage, SysMemPage, Palette);
              SysMemPage.Scale(SeenBuff, 0, 0, 0, 0, 320, 199, 640, 398);
              Fade_Palette_To(Palette, kFadePaletteMedium, Call_Back);
            }
            Clear_KeyBuffer();
            count.Set(int64_t{kTimerSecond} * 3);
            while (count.Time()) {
              Call_Back();
            }
            Fade_Palette_To(BlackPalette, kFadePaletteMedium, Call_Back);

            Play_Movie("SIZZLE2");  // Red Alert teaser.
          }

          Fade_Palette_To(BlackPalette, kFadePaletteMedium, Call_Back);
          VisiblePage.Clear();
          if (GameFile("ATTRACT2.CPS").IsAvailable()) {
            GameFile f("ATTRACT2.CPS");
            Load_Uncompress(f, SysMemPage, SysMemPage, Palette);
            SysMemPage.Scale(SeenBuff, 0, 0, 0, 0, 320, 199, 640, 398);
            Fade_Palette_To(Palette, kFadePaletteMedium, Call_Back);
          }
          Clear_KeyBuffer();
          count.Set(int64_t{kTimerSecond} * 3);
          while (count.Time()) {
            Call_Back();
          }
          Fade_Palette_To(BlackPalette, kFadePaletteMedium, Call_Back);

          Play_Movie("CC2TEASE");
          Show_Mouse();

          ScenarioInit++;
          Theme.Play_Song(THEME_MAP1);
          ScenarioInit--;
          display = true;
          fade = true;
          selection = kSelNone;
          break;

        /*
        **	Exit to DOS.
        */
        case kSelExit:
#ifdef JAPANESE
          Hide_Mouse();
#endif
          Theme.Fade_Out();
          Fade_Palette_To(BlackPalette, kFadePaletteSlow, nullptr);
#ifdef JAPANESE
          VisiblePage.Clear();
#endif
          return false;

        /*
        **	Display the hall of fame.
        */
        case kSelFame:
          break;

        case kSelTimeout:
          if (AllowAttract && RecordFile.IsAvailable()) {
            PlaybackGame = true;
            if (RecordFile.Open(FileAccess::kRead)) {
              Load_Recording_Values();
              process = false;
              Theme.Fade_Out();
            } else {
              PlaybackGame = false;
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
    ** For Debug_Map (editor) mode, if JP option is on, set to load that
    *scenario
    */
    Scenario = 1;
    if (Special.IsJurassic && AreThingiesEnabled) {
      ScenPlayer = SCEN_PLAYER_JP;
      ScenDir = SCEN_DIR_EAST;
    }
  }
  DLOG(INFO) << "C&C95 - About to start game initialisation.";
#ifdef FORCE_WINSOCK
  if (GameToPlay == GAME_INTERNET) {
    CommProtocol = COMM_PROTOCOL_MULTI_E_COMP;
    if (!Special.IsFromWChat) {
      FrameSendRate = 5;  // 3;
    }
  }
#endif  // FORCE_WINSOCK
  /*
  **	Don't carry stray keystrokes into game.
  */
  Kbd.Clear();

  /*
  **	Initialize the random number Seed.  For multiplayer, this will have been
  *done
  ** in the connection dialogs.  For single-player games, AND if we're not
  *playing
  ** back a recording, init the Seed to a random value.
  */
  if (GameToPlay == GAME_NORMAL && !PlaybackGame) {
    Seed = port::RandomSeed();
  }

  /*
  ** If user has specified a desired random number seed, use it for multiplayer
  *games
  */
  if (CustomSeed != 0) {
    Seed = CustomSeed;
  }

  /*
  ** Save initialization values if we're recording this game.
  ** This must be done after 'Seed' has been initialized.
  */
  if (RecordGame) {
    if (RecordFile.Open(FileAccess::kWrite)) {
      Save_Recording_Values();
    } else {
      RecordGame = false;
    }
  }

  /*
  **	Initialize the random-number generator.
  */
  // Loading already restored the exact stream positions.
  if (!gameloaded) {
    SeedGameRandom(static_cast<uint32_t>(Seed));
  }

  /*
  **	Load the scenario.  Specify variation 'A' for the editor; for the game,
  **	don't specify a variation, to make 'Set_Scenario_Name()' pick a random
  *one. *	Skip this if we've already loaded a save-game.
  */
  if (!gameloaded) {
    if (DebugNewGame.size() >= 5) {
      port::SafeCopy(ScenarioName, DebugNewGame.c_str());
      DebugNewGame.clear();
    } else if (Debug_Map) {
      Set_Scenario_Name(ScenarioName, Scenario, ScenPlayer, ScenDir,
                        SCEN_VAR_A);
    } else {
      Set_Scenario_Name(ScenarioName, Scenario, ScenPlayer, ScenDir);
    }

    /*
    ** Start_Scenario() changes the palette; so, fade out & clear the screen
    ** before calling it.
    */
    Hide_Mouse();

    if (selection != kSelStartNewGame) {
      Fade_Palette_To(BlackPalette, kFadePaletteMedium, Call_Back);
      HiddenPage.Clear();
      VisiblePage.Clear();
    }
    Show_Mouse();

    Special.IsFromInstall = 0;
    DLOG(INFO) << "C&C95 - Starting scenario.";
    if (!Start_Scenario(ScenarioName)) {
      return false;
    }
    DLOG(INFO) << "C&C95 - Scenario started OK.";
    if (DebugGlobalsTest) {
      Score.Score = 101;
      Score.NKilled = 2; Score.GKilled = 3; Score.CKilled = 4;
      Score.NBKilled = 5; Score.GBKilled = 6; Score.CBKilled = 7;
      Score.NHarvested = 8; Score.GHarvested = 9; Score.CHarvested = 10;
      Score.ElapsedTime = static_cast<int64_t>(uint64_t{1} << 40);
      Base.House = HOUSE_GOOD;
      Base.Nodes.Clear();
      BaseNodeClass node;
      node.Type = STRUCT_POWER;
      node.Coord = Cell_Coord(1000);
      Base.Nodes.Add(node);
      node.Type = STRUCT_REFINERY;
      node.Coord = Cell_Coord(1200);
      Base.Nodes.Add(node);
      CurrentObject.Clear();
      if (Units.Count() < 2) {
        return false;
      }
      CurrentObject.Add(Units.Ptr(1));
      CurrentObject.Add(Units.Ptr(0));
      base::At(Waypoint, 20) = 1234;
      CarryOverMoney = 13579;
      CarryOverPercent = 42;
      base::At(Views, 3) = 2345;
      EndCountDown = 700;
    }
    if (DebugMapTest) {
      for (CELL cell = 0; cell < 16; ++cell) {
        if (Map.In_Radar(cell)) {
          LOG(ERROR) << "-MAPTEST: fixture cells must be outside the playable map";
          return false;
        }
        Map.at(cell).Reset();
      }
      // Isolate fields formerly omitted by the sparse-cell predicate.
      Map.at(0).IsPlot = true;
      Map.at(1).IsCursorHere = true;
      Map.at(2).IsWaypoint = true;
      Map.at(3).IsRadarCursor = true;
      Map.at(4).IsFlagged = true;
      Map.at(5).TIcon = 7;
      Map.at(6).OverlayData = 3;
      Map.at(7).SmudgeData = 2;
      Map.at(8).Owner = HOUSE_GOOD;
      Map.at(9).InfType = HOUSE_BAD;
      Map.at(10).Overlay = OVERLAY_BRICK_WALL;
      Map.at(10).Recalc_Attributes();
      Map.at(10).Overlay = OVERLAY_NONE;
      auto* trigger = new TriggerClass;
      if (!trigger || Units.Count() == 0) {
        return false;
      }
      trigger->AttachCount = 2;
      Map.at(11).IsTrigger = Map.at(12).IsTrigger = true;
      CellTriggers.at(11) = CellTriggers.at(12) = trigger;
      Map.at(13).OccupierPtr = Units.Ptr(0);
      base::At(Map.at(14).Overlappers, 2) = Units.Ptr(0);
      Map.at(15).Flag.Composite = 2;
      Map.TotalValue = static_cast<int64_t>(uint64_t{1} << 35);
      auto* pending = new BuildingClass(STRUCT_POWER, PlayerPtr->Class->House);
      if (!pending) {
        return false;
      }
      Map.PendingObjectPtr = pending;
      Map.PendingObject = &pending->Class_Of();
      Map.PendingHouse = PlayerPtr->Class->House;
      Map.Set_Cursor_Shape(Map.PendingObject->Occupy_List(true));
    }
    if (DebugMobileTest) {
      const HousesType house = PlayerPtr->Class->House;
      auto* vehicle = new UnitClass(UNIT_APC, house);
      auto* passenger = new InfantryClass(INFANTRY_E1, house);
      auto* plane = new AircraftClass(AIRCRAFT_ORCA, house);
      if (!vehicle || !passenger || !plane) {
        LOG(ERROR) << "-MOBILETEST: fixture allocation failed";
        return false;
      }
      // Non-default limbo state supplements the campaign's moving/firing units.
      vehicle->Attach(passenger);
      vehicle->Kills = 51;
      passenger->Kills = 73;
      plane->Kills = 95;
      vehicle->Flagged = HOUSE_BAD;
      vehicle->Tiberium = 7;
      vehicle->IsHarvesting = true;
      vehicle->IsReturning = true;
      vehicle->IsTurretLockedDown = true;
      vehicle->Reload = 173;
      vehicle->SecondaryFacing.Set(DIR_NE);
      vehicle->SecondaryFacing = DIR_SW;
      base::At(vehicle->Path, 0) = FACING_NE;
      base::At(vehicle->Path, 1) = FACING_E;
      base::At(vehicle->Path, 2) = FACING_NONE;
      vehicle->PathDelay = 181;
      vehicle->BaseAttackTimer = 217;
      vehicle->TryTryAgain = 3;
      vehicle->ArchiveTarget = plane->As_Target();
      vehicle->NavCom = plane->As_Target();
      vehicle->SuspendedNavCom = passenger->As_Target();
      vehicle->IsNewNavCom = true;
      vehicle->IsPlanningToLook = true;
      vehicle->IsDeploying = true;
      vehicle->IsRotating = true;
      vehicle->IsFiring = true;
      vehicle->IsUnloading = true;
      vehicle->Speed = 103;
      vehicle->Group = 4;
      passenger->Doing = DO_PRONE;
      passenger->Comment = 193;
      passenger->IsTechnician = true;
      passenger->IsStoked = true;
      passenger->IsProne = true;
      passenger->IsBoxing = true;
      passenger->Fear = 81;
      plane->SecondaryFacing.Set(DIR_SE);
      plane->SecondaryFacing = DIR_N;
      bool launched = false;
      if (Units.Count() != 0) {
        const CELL start = Coord_Cell(Units.Ptr(0)->Coord);
        for (int offset = 1; offset <= 16; ++offset) {
          const CELL cell = static_cast<CELL>(start + offset);
          if (cell < MAP_CELL_TOTAL && Map.In_Radar(cell) &&
              plane->Unlimbo(Cell_Coord(cell), DIR_E)) {
            launched = true;
            break;
          }
        }
      }
      if (!launched) {
        LOG(ERROR) << "-MOBILETEST: could not launch aircraft";
        return false;
      }
      plane->Assign_Destination(::As_Target(Coord_Cell(plane->Coord + 0x800)));
      plane->Assign_Mission(MISSION_MOVE);
      plane->Set_Speed(123);
    }
    if (DebugBuildingTest) {
      const HousesType house = PlayerPtr->Class->House;
      // Limbo fixtures preserve non-default fields without building AI replacing
      // them before the save. Campaign buildings exercise normal AI separately.
      auto* building = new BuildingClass(STRUCT_WEAP, house);
      auto* peer = new BuildingClass(STRUCT_REPAIR, house);
      auto* passenger = new InfantryClass(INFANTRY_E1, house);
      auto* factory = new FactoryClass;
      if (!building || !peer || !passenger || !factory ||
          !factory->Set(UnitTypeClass::As_Reference(UNIT_JEEP), *PlayerPtr) ||
          !factory->Start() ||
          building->Transmit_Message(RADIO_HELLO, peer) != RADIO_ROGER) {
        LOG(ERROR) << "-BUILDINGTEST: could not create linked fixtures";
        return false;
      }
      building->Factory = factory;
      building->Attach(passenger);
      building->Kills = 75;
      building->PurchasePrice = 1234;
      building->Mission = MISSION_REPAIR;
      building->SuspendedMission = MISSION_GUARD;
      building->MissionQueue = MISSION_UNLOAD;
      building->Status = 3;
      building->CountDown = 175;
      building->PlacementDelay = 210;
      building->LastStrength = building->Strength - 20;
      building->WhomToRepay = peer->As_Target();
      building->WhoLastHurtMe = HOUSE_BAD;
      building->BState = BSTATE_ACTIVE;
      building->QueueBState = BSTATE_IDLE;
      building->IsCaptured = true;
      building->IsRepairing = true;
      building->IsWrenchVisible = true;
      building->IsReadyToCommence = true;
      building->IsGoingToBlow = true;
      building->IsSurvivorless = true;
      building->IsCharging = true;
      building->IsCharged = true;
      building->IsTickedOff = true;
      building->IsCloakable = true;
      building->IsLeader = true;
      building->IsALoaner = true;
      building->IsLocked = true;
      building->IsInRecoilState = true;
      building->IsTethered = true;
      building->IsDiscoveredByComputer = true;
      building->IsALemon = true;
      building->IsSecondShot = true;
      building->Cloak = CLOAKING;
      building->CloakingDevice.Set_Stage(4);
      building->CloakingDevice.Set_Rate(9);
      building->TarCom = peer->As_Target();
      building->SuspendedTarCom = passenger->As_Target();
      building->PrimaryFacing.Set(DIR_NE);
      building->PrimaryFacing = DIR_SW;
      building->Arm = 19;
      building->Ammo = 11;
      building->FlashCount = 23;
      building->IsBlushing = true;
      building->Set_Stage(7);
      building->Set_Rate(13);
      building->Open_Door(7, 18);
    }
    if (DebugWorldTest) {
      if (Units.Count() == 0) {
        LOG(ERROR) << "-WORLDTEST: scenario needs a unit";
        return false;
      }
      UnitClass* owner = Units.Ptr(0);
      // Keep normally short-lived placement objects in limbo across the save.
      auto* ground = new TemplateClass(TEMPLATE_CLEAR1);
      auto* overlay = new OverlayClass(OVERLAY_CONCRETE);
      auto* smudge = new SmudgeClass(SMUDGE_CRATER1);
      auto* terrain = new TerrainClass(TERRAIN_TREE1, -1);
      auto* bullet = new BulletClass(BULLET_HE);
      auto* trigger = new TriggerClass;
      auto* anim = new AnimClass(ANIM_SMOKE_PUFF, owner->Coord, 90, 10);
      if (!ground || !overlay || !smudge || !terrain || !bullet || !trigger ||
          !anim) {
        LOG(ERROR) << "-WORLDTEST: fixture allocation failed";
        return false;
      }
      trigger->AttachCount = 5;
      ground->Trigger = overlay->Trigger = smudge->Trigger = terrain->Trigger =
          bullet->Trigger = trigger;
      ground->Next = terrain;
      overlay->Next = ground;
      smudge->Next = terrain;
      terrain->Next = owner;
      terrain->Set_Stage(2);
      terrain->Set_Rate(7);
      bullet->Next = ground;
      bullet->Payback = owner;
      bullet->PrimaryFacing.Set(DIR_NE);
      bullet->PrimaryFacing = DIR_SE;
      bullet->Fly_Speed(127, MPH_FAST);
      bullet->Arm_Fuse(owner->Coord, owner->Coord + 0x200, 95, 5);
      bullet->Assign_Target(owner->As_Target());
      auto* missile = new BulletClass(BULLET_SSM);
      const COORDINATE destination = owner->Coord + 0x600;
      if (missile == nullptr) {
        return false;
      }
      missile->Payback = owner;
      missile->Assign_Target(::As_Target(Coord_Cell(destination)));
      if (!missile->Unlimbo(owner->Coord, DIR_E)) {
        LOG(ERROR) << "-WORLDTEST: could not launch missile";
        return false;
      }
      missile->Fly_Speed(127, MPH_SLOW);
      missile->Arm_Fuse(owner->Coord, destination, 95, 5);
      missile->Strength = 0;
      anim->Attach_To(owner);
      anim->Owner = owner->House->Class->House;
    }
    if (DebugTeamTest) {
      UnitClass* member = nullptr;
      for (int i = 0; i < Units.Count(); ++i) {
        if (Units.Ptr(i)->House == PlayerPtr && !Units.Ptr(i)->IsInLimbo) {
          member = Units.Ptr(i);
          break;
        }
      }
      auto* type = new TeamTypeClass;
      if (member == nullptr || type == nullptr) {
        LOG(ERROR) << "-TEAMTEST: no member or team type slot";
        return false;
      }
      type->Set_Name("saveteam");
      type->House = PlayerPtr->Class->House;
      type->MaxAllowed = 1;
      type->ClassCount = 1;
      base::At(type->Class, 0) = member->Class;
      base::At(type->DesiredNum, 0) = 1;
      type->MissionCount = 2;
      base::At(type->MissionList, 0) = {TMISSION_GUARD, 100};
      base::At(type->MissionList, 1) = {TMISSION_LOOP, 0};
      auto* team = type->Create_One_Of();
      if (team == nullptr || !team->Add(member)) {
        LOG(ERROR) << "-TEAMTEST: could not create a populated team";
        return false;
      }
      team->Force_Active();
      team->SuspendTimer = 150;
    }
    if (DebugFactoryTest) {
      auto* factory = new FactoryClass;
      if (factory == nullptr ||
          !factory->Set(UnitTypeClass::As_Reference(UNIT_JEEP), *PlayerPtr) ||
          !factory->Start()) {
        LOG(ERROR) << "-FACTORYTEST: could not start production";
        return false;
      }
    }
  }

  /*
  **	For multiplayer games, initialize the inter-player message system.
  **	Do this after loading the scenario, so the map's upper-left corner is
  **	properly set.
  */
  DLOG(INFO) << "C&C95 - Initialising message system.";
  const int factor = SeenBuff.Get_Width() == 320 ? 1 : 2;
  Messages.Init(Map.TacPixelX, Map.TacPixelY, 6, MAX_MESSAGE_LENGTH,
                (6 * factor) + 1);

  /*
  **	Hide the SeenBuff; force the map to render one frame.  The caller can
  **	then fade the palette in.
  **	(If we loaded a game, this step will fade out the title screen.  If we
  **	started a scenario, Start_Scenario() will have played a couple of VQ
  **	movies, which will have cleared the screen to black already.)
  */
  DLOG(INFO) << "C&C95 - About to call Call_Back.";
  Call_Back();

  /*
  ** This is desperately sad isnt it?
  */
  Hide_Mouse();
  Hide_Mouse();
  Hide_Mouse();
  Hide_Mouse();
  WWMouse->Erase_Mouse(&HidPage, true);

  Fade_Palette_To(BlackPalette, kFadePaletteMedium, Call_Back);
  HiddenPage.Clear();
  VisiblePage.Clear();
  Set_Logic_Page(SeenBuff);
  Map.Flag_To_Redraw();
  Call_Back();
  Map.Render();
  // Show_Mouse();

  /*
  ** Special hack initialization of 'MPlayerMaxAhead' to accommodate the
  ** compression protocol technology.
  */
#ifdef FORCE_WINSOCK
  if (CommProtocol == COMM_PROTOCOL_MULTI_E_COMP && GameToPlay != GAME_NORMAL) {
    if (!Special.IsFromWChat) {
      MPlayerMaxAhead = FrameSendRate * 3;  // 2;
    } else {
      MPlayerMaxAhead = WChatMaxAhead;
      FrameSendRate = WChatSendRate;
    }
  }
#endif  // FORCE_WINSOCK

  if (Debug_Map) {
    while (Get_Mouse_State() > 1) {
      Show_Mouse();
    }
  }

  return true;
}

/***********************************************************************************************
 * Play_Intro -- plays the introduction & logo movies *
 *                                                                                             *
 * INPUT: * for_real			if true, this function plays the "real"
 *intro; otherwise, it plays		  * a delicious smorgasbord of visual
 *delights, guaranteed to titillate		  * the ocular & auditory nerve
 *pathways.
 ** Well, it plays movies, anyway.
 **
 *                                                                                             *
 * OUTPUT: * none.
 **
 *                                                                                             *
 * WARNINGS: * none.
 **
 *                                                                                             *
 * HISTORY: * 06/06/1995 BRR : Created. *
 *=============================================================================================*/
static void Play_Intro(bool for_real) {
  const bool playright = !Key_Down(KN_LCTRL) || !Key_Down(KN_RCTRL);
  static int _counter = -1;
  static const char* _names[] = {
#ifdef DEMO
      "LOGO",

#else

      "INTRO2",
      // #ifdef CHEAT_KEYS
      "GDIEND1", "GDIEND2", "GDIFINA", "GDIFINB", "AIRSTRK", "AKIRA", "BANNER",
      "BCANYON", "BKGROUND", "BOMBAWAY", "BOMBFLEE", "BURDET1", "BURDET2",
      "CC2TEASE", "CONSYARD", "DESFLEES", "DESKILL", "DESOLAT", "DESSWEEP",
      "FLAG", "FLYY", "FORESTKL", "GAMEOVER", "GDI1", "GDI10", "GDI11", "GDI12",
      "GDI13", "GDI14", "GDI15", "GDI2", "GDI3", "GDI3LOSE", "GDI4A", "GDI4B",
      "GDI5", "GDI6", "GDI7", "GDI8A", "GDI8B", "GDI9", "GDILOSE", "GUNBOAT",
      "HELLVALY", "INSITES", "KANEPRE", "LANDING", "LOGO", "NAPALM", "NITEJUMP",
      "NOD1", "NOD10A", "NOD10B", "NOD11", "NOD12", "NOD13", "NOD1PRE", "NOD2",
      "NOD3", "NOD4A", "NOD4B", "NOD5", "NOD6", "NOD7A", "NOD7B", "NOD8",
      "NOD9", "NODEND1", "NODEND2", "NODEND3", "NODEND4", "NODFINAL",
      "NODFLEES", "NODLOSE", "NODSWEEP", "NUKE", "OBEL", "PARATROP", "PINTLE",
      "PLANECRA", "PODIUM", "REFINT", "RETRO", "SABOTAGE", "SAMDIE", "SAMSITE",
      "SEIGE", "SETHPRE", "SPYCRASH", "STEALTH", "SUNDIAL", "TANKGO",
      "TANKKILL", "TBRINFO1", "TBRINFO2", "TBRINFO3", "TIBERFX", "TRTKIL_D",
      "TURTKILL", "VISOR",
// #endif
#endif
      nullptr};

  Keyboard::Clear();
  if (for_real) {
    Hide_Mouse();
    Play_Movie("LOGO", THEME_NONE, false);
    Show_Mouse();
  } else {
    if (!Debug_Flag) {
      _counter = 0;
    } else {
      if (playright) {
        _counter++;
      }
      if (_counter == -1) {
        _counter = 0;
      }
    }
    Hide_Mouse();
    Play_Movie(base::At(_names, _counter), THEME_NONE);
    Show_Mouse();
    if (!base::At(_names, _counter)) {
      _counter = -1;
    }
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
  VQA_DefaultConfig(&AnimControl);
  //	void const * font = Load_Font(FONT8);
  //	AnimControl.EVAFont = (char *)font;
  //	AnimControl.CapFont = (char *)font;

  AnimControl.DrawFlags = VQACFGF_TOPLEFT;
  AnimControl.DrawFlags |= VQACFGF_BUFFER;

  AnimControl.DrawFlags |= VQACFGF_NOSKIP;

  // AnimControl.X1 =0;
  // AnimControl.Y1 =0;
  AnimControl.FrameRate = -1;
  AnimControl.DrawRate = -1;

  AnimControl.DrawerCallback = VQ_Call_Back;
  AnimControl.EventHandler = VQ_Event_Handler;
  AnimControl.ImageWidth = 320;
  AnimControl.ImageHeight = 200;
  AnimControl.Vmode = 0;
  AnimControl.ImageBuf = SysMemPage.Get_Bytes();
  // AnimControl.VBIBit = VertBlank;
  // AnimControl.DrawFlags |= VQACFGF_TOPLEFT;
  AnimControl.OptionFlags |= VQAOPTF_CAPTIONS | VQAOPTF_EVA;

  if (SlowPalette) {
    AnimControl.OptionFlags |= VQAOPTF_SLOWPAL;
  }

  //	AnimControl.AudioBuf = (unsigned char *)HidPage.Get_Buffer();
  //	AnimControl.AudioBufSize = 32768U;
  // AnimControl.DigiCard = NewConfig.DigitCard;
  // AnimControl.HMIBufSize = 8192;
  // AnimControl.Volume = 0x00FF;
  // AnimControl.AudioRate = 22050;
  //	if (NewConfig.Speed) AnimControl.AudioRate = 11025;
  AnimControl.AudioDeviceID = Audio.device_id();
  AnimControl.AudioCallback = Audio.extra_callback_slot();
  AnimControl.AudioSpec = Audio.output_spec();
  // if (!Debug_Quiet && Audio.is_open()) {
  // AnimControl.OptionFlags |= VQAOPTF_AUDIO;
  //}
}

// Applies "-DESTNET<address>": up to ten dot-separated hex bytes, the first
// four the IPX network and the rest the node, naming the network across a
// bridge. A malformed address, or one shorter than four bytes, is ignored.
// `address` is the text after "-DESTNET" and is tokenized in place. Split out
// of Parse_Command_Line() so the std::optional below does not make clang-tidy
// run its optional-access dataflow over that whole function.
static void ApplyDestNetArgument(char* address) {
  NetNumType net;
  NetNodeType node;

  /*
  ** Scan the command-line string, pulling off each address piece
  */
  int i = 0;
  port::Tokenizer tokens(address, ".");
  const char* p = tokens.Next();
  while (p) {
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
    p = tokens.Next();
  }

  /*
  ** If all the address components were successfully read, fill in the
  ** BridgeNet with a broadcast address to the network across the bridge.
  */
  if (i >= 4) {
    IsBridge = 1;
    base::FillBytes(base::ObjectBytes(node), 0xff, 6);
    BridgeNet = IPXAddressClass(net, node);
  }
}

// Applies "-SOCKET<offset>": the IPX socket is 0x4000 plus an offset in
// [0, 0x4000). Anything else leaves the socket alone.
static void ApplySocketArgument(std::string_view offset_text) {
  const auto offset = tech::ParseInteger<int>(offset_text);
  if (offset && *offset >= 0 && *offset < 0x4000) {
    Ipx.Set_Socket(static_cast<uint16_t>(*offset + 0x4000));
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
bool Parse_Command_Line(std::span<char*> arguments) {
  /*
  **	Parse the command line and set globals to reflect the parameters
  **	passed in.
  */
#ifdef DEMO
  Scenario = 3;
#else
  Scenario = 1;
#endif
  ScenPlayer = SCEN_PLAYER_GDI;
  ScenDir = SCEN_DIR_EAST;
  Whom = HOUSE_GOOD;
  Special.Init();

  Debug_Map = false;
  //	Debug_Play_Map = false;
  Debug_Unshroud = false;

  for (char* argument :
       arguments.subspan(std::min<size_t>(1, arguments.size()))) {
    const std::string original_arg = argument;
    std::string string = absl::AsciiStrToUpper(original_arg);

    if (string.starts_with("-SEED")) {
      CustomSeed = tech::ParseIntegerOr<uint16_t>(
          string.substr(5), static_cast<uint16_t>(CustomSeed));
      continue;
    }
    if (string.starts_with("-NEWGAME")) {
      DebugNewGame = string.substr(8);
      if (DebugNewGame.size() < 3) {
        return false;
      }
      continue;
    }
    if (string.starts_with("-LOADGAME")) {
      DebugLoadGame = tech::ParseIntegerOr<int>(string.substr(9), -1);
      continue;
    }
    if (string.starts_with("-QUITFRAME")) {
      DebugQuitAtFrame = tech::ParseIntegerOr<int>(string.substr(10), -1);
      continue;
    }
    if (string.starts_with("-SAVESLOT")) {
      DebugSaveSlot = tech::ParseIntegerOr<int>(string.substr(9), -1);
      continue;
    }
    if (std::string_view(string) == "-GLOBALTEST") {
      DebugGlobalsTest = true;
      continue;
    }
    if (std::string_view(string) == "-MAPTEST") {
      DebugMapTest = true;
      continue;
    }
    if (std::string_view(string) == "-MOBILETEST") {
      DebugMobileTest = true;
      continue;
    }
    if (std::string_view(string) == "-BUILDINGTEST") {
      DebugBuildingTest = true;
      continue;
    }
    if (std::string_view(string) == "-WORLDTEST") {
      DebugWorldTest = true;
      continue;
    }
    if (std::string_view(string) == "-TEAMTEST") {
      DebugTeamTest = true;
      continue;
    }
    if (std::string_view(string) == "-FACTORYTEST") {
      DebugFactoryTest = true;
      continue;
    }
    if (std::string_view(string) == "-NOMOVIES") {
      DebugNoMovies = true;
      continue;
    }

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
#ifdef GERMAN
      puts(
          "Command & Conquer (c) 1995,1996 Westwood Studios\r\n"
          "Parameter:\r\n"
          //						"  -CD<Pfad> = Suchpfad
          // für Daten-Dateien festlegen.\r\n"
          "  -DESTNET  = Netzwerkkennung des Zielrechners festlegen\r\n"
          "              (Syntax: DESTNETxx.xx.xx.xx)\r\n"
          "  -SOCKET   = Kennung des Netzwerk-Sockets (0 - 16383)\n"
          "  -STEALTH  = Namen im Mehrspieler-Modus verstecken "
          "(\"Boss-Modus\")\r\n"
          "  -MESSAGES = Mitteilungen von außerhalb des Spiels zulassen\r\n"
          //					"  -ELITE    = Fortgeschrittene
          // KI und Gefechtstechniken.\r\n"
          "\r\n");
#else
#ifdef FRENCH
      puts(
          "Command & Conquer (c) 1995, Westwood Studios\r\n"
          "Paramètres:\r\n"
          //						"  -CD<chemin d'accès> =
          // Recherche des fichiers dans le\r\n"
          // " répertoire indiqué.\r\n"
          "  -DESTNET  = Spécifier le numéro de réseau du système de "
          "destination\r\n"
          "              (Syntaxe: DESTNETxx.xx.xx.xx)\r\n"
          "  -SOCKET   = ID poste réseau (0 à 16383)\r\n"
          "  -STEALTH  = Cacher les noms en mode multijoueurs (\"Mode "
          "Boss\")\r\n"
          "  -MESSAGES = Autorise les messages extérieurs à ce jeu.\r\n"
          "\r\n");
#else
      puts(
          "Command & Conquer (c) 1995, 1996 Westwood Studios\r\n"
          "Parameters:\r\n"
#ifdef NEVER
          "  CHEAT     = Enable debug keys.\r\n"
          "  -EDITOR    = Enable scenario editor.\r\n"
#endif
          //						"  -CD<path> = Set
          // search path for data files.\r\n"
          "  -DESTNET  = Specify Network Number of destination system\r\n"
          "              (Syntax: DESTNETxx.xx.xx.xx)\r\n"
          "  -SOCKET   = Network Socket ID (0 - 16383)\n"
          "  -STEALTH  = Hide multiplayer names (\"Boss mode\")\r\n"
          "  -MESSAGES = Allow messages from outside this game.\r\n"
          "  -o        = Enable compatability with version 1.07.\r\n"
#ifdef JAPANESE
          "  -ENGLISH  = Enable English keyboard compatibility.\r\n"
#endif
//					"  -ELITE    = Advanced AI and combat
// characteristics.\r\n"
#ifdef NEVER
          "  -O[options]= Special control options;\r\n"
          "     1 : Tiberium grows.\r\n"
          "     2 : Tiberium grows and spreads.\r\n"
          "     A : Aggressive player unit defense enabled.\r\n"
          "     B : Bargraphs always displayed.\r\n"
          "     C : Capture the flag mode.\r\n"
          "     E : Elite defense mode disable (attacker advantage).\r\n"
          "     D : Deploy reversal allowed for construction yard.\r\n"
          "     F : Fleeing from direct immediate threats is enabled.\r\n"
          "     H : Hussled recharge time.\r\n"
          "     G : Growth for Tiberium slowed in multiplay.\r\n"
          "     I : Inert weapons -- no damage occurs.\r\n"
          "     J : 7th grade sound effects.\r\n"
          "     N : Name the civilians and buildings.\r\n"
          "     P : Path algorithm displayed as it works.\r\n"
          "     Q : Quiet mode (no sound).\r\n"
          "     R : Road pieces are not added to buildings.\r\n"
          "     T : Three point turns for wheeled vehicles.\r\n"
          "     U : U can target and burn trees.\r\n"
          "     V : Show target selection by opponent.\r\n"
          "     X : Make a recording of a multiplayer game.\r\n"
          "     Y : Play a recording of a multiplayer game.\r\n"
          "     Z : Disaster containment team.\r\n"
#endif
          "\r\n");
#endif
#endif
      return false;
    }

    bool processed = true;
    switch (HashKeyPhrase(string)) {
      /*
      **	Signal that easy mode is active.
      */
      case PARM_EASY:
        Special.IsHealthBar = true;
        Special.IsEasy = true;
        Special.IsDifficult = false;
        break;

      /*
      **	Signal that hard mode is active.
      */
      case PARM_HARD:
        Special.IsHealthBar = false;
        Special.IsEasy = false;
        Special.IsDifficult = true;
        break;

      case PARM_PLAYTEST:
        if constexpr (config::kVirginCheatKeysEnabled) {
          Debug_Playtest = true;
        }
        break;

      case PARM_CHEATERIK:
      case PARM_CHEATADAM:
      case PARM_CHEATMIKE:
      case PARM_CHEATDAVID:
      case PARM_CHEATPHIL:
      case PARM_CHEATBILL:
      case PARM_CHEAT_STEVET:
        Debug_Playtest = true;
        Debug_Flag = true;
        break;

      case PARM_EDITORBILL:
      case PARM_EDITORERIK:
        Debug_Map = true;
        Debug_Unshroud = true;
        Debug_Flag = true;
        break;

      case PARM_SPECIAL:
        Special.IsJurassic = true;
        AreThingiesEnabled = true;
        break;

      /*
      ** Special flag - is C&C being run from the install program?
      */
      case PARM_INSTALL:
#ifndef DEMO
        Special.IsFromInstall = true;
#endif
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
        Debug_Check_Map = true;
        continue;
      }
    }

    /*
    **	Older version override.
    */
    if (absl::EqualsIgnoreCase(string, "-O") ||
        absl::EqualsIgnoreCase(string, "-0")) {
      IsV107 = true;
      continue;
    }

    /*
    **	File search path override.
    */
    if (string.contains("-CD")) {
      SearchPaths::Add(original_arg.substr(3));
      continue;
    }
#ifdef JAPANESE
    /*
    ** Enable english-compatible keyboard
    */
    if (absl::EqualsIgnoreCase(string, "-ENGLISH")) {
      ForceEnglish = true;
      continue;
    }
#endif

    /*
    **	Specify destination connection for network play
    */
    if (string.contains("-DESTNET")) {
      ApplyDestNetArgument(std::span(string).subspan(8).data());
      continue;
    }

    /*
    **	Specify socket ID, as an offset from 0x4000.
    */
    if (string.contains("-SOCKET")) {
      ApplySocketArgument(
          std::string_view(string).substr(std::string_view("-SOCKET").size()));
      continue;
    }

    /*
    **	Set the Net Stealth option
    */
    if (string.contains("-STEALTH")) {
      NetStealth = true;
      continue;
    }

    /*
    **	Set the Net Protection option
    */
    if (string.contains("-MESSAGES")) {
      NetProtect = false;
      continue;
    }

    /*
    **	Allow "attract" mode
    */
    if (string.contains("-ATTRACT")) {
      AllowAttract = true;
      continue;
    }

    /*
    ** Disable mouse grabbing for debugging
    */
    if (string.contains("-NOMOUSEGRAB")) {
      NoMouseGrab = true;
      continue;
    }

    /*
    ** Set screen to 640x480 instead of 640x400
    */
    if (string.contains("-480")) {
      ScreenHeight = 480;
      continue;
    }

    /*
    ** Check for spawn from WChat
    */
    if (string.contains("-WCHAT")) {
      SpawnedFromWChat = true;
    }

    /*
    ** Allow use of MMX instructions
    */
    if (string.contains("-MMX")) {
      MMXAvailable = true;
      continue;
    }

    if constexpr (config::kCheatKeysEnabled) {
      /*
      **	Allow solo net play
      */
      if (absl::EqualsIgnoreCase(string, "-HANSOLO")) {
        MPlayerSolo = true;
        continue;
      }
    }

#ifdef NEVER
    /*
    **	Handle the prog init differently in this case.
    */
    if (string.contains("-V")) {
      continue;
    }
#endif

#ifdef FIX_ME_LATER
    /*
    ** look for passed-in video mode to default to
    */
    if (absl::StartsWithIgnoreCase(string, "-V")) {
      Set_Video_Mode(
          MCGA_MODE);  // do this to get around first_time variable...
      Set_Original_Video_Mode(atoi(string + 2));
      continue;
    }
#endif  // FIX_ME_LATER

    /*
    **	Special command line control parsing.
    */
    if (string.starts_with("-X")) {
      for (const char code : std::string_view(string).substr(2)) {
        switch (toupper(code)) {
#ifdef ONHOLD
          /*
          **	Should human generated sound effects be used?
          */
          case 'J':
            Special.IsJuvenile = true;
            break;
#endif

          /*
          **	Inert weapons -- no units take damage.
          */
          case 'I':
            if constexpr (config::kCheatKeysEnabled) {
              Special.IsInert = true;
            }
            break;

          /*
          **	Hussled recharge timer.
          */
          case 'H':
            if constexpr (config::kCheatKeysEnabled) {
              Special.IsSpeedBuild = true;
            }
            break;

          /*
          **	Turn on super-record mode, which thrashes your disk terribly,
          ** but is really really cool.  Well, sometimes it is, anyway.
          ** At least, it can be.  Once in a while.
          ** This flag tells the recording system to re-open the file for
          ** each write, so the recording survives a crash.
          */
          case 'S':
            if constexpr (config::kCheatKeysEnabled) {
              SuperRecord = 1;
            }
            break;

          /*
          **	"Record" a multi-player game
          */
          case 'X':
            if constexpr (config::kCheatKeysEnabled) {
              RecordGame = true;
            }
            break;

          /*
          **	"Play Back" a multi-player game
          */
          case 'Y':
            if constexpr (config::kCheatKeysEnabled) {
              PlaybackGame = true;
            }
            break;

#ifdef ONHOLD
          /*
          **	Bonus scenario enable.
          */
          case 'Z':
            Special.IsJurassic = true;
            break;
#endif

          /*
          **	Quiet mode override control.
          */
          case 'Q':
            Debug_Quiet = true;
            break;

          /*
          **	Target selection by human opponent (network/modem play) will
          **	be visible to the player?
          */
          case 'V':
            if constexpr (config::kCheatKeysEnabled) {
              Special.IsVisibleTarget = true;
            }
            break;

          default:
#ifdef GERMAN
            puts("Ungültiger Parameter.\n");
#else
#ifdef FRENCH
            puts("Commande d'option invalide.\n");
#else
            puts("Invalid option switch.\n");
#endif
#endif
            return false;
        }
      }

      continue;
    }
  }
  return true;
}

#ifdef ONHOLD
/***********************************************************************************************
 * Parse_INI_File -- Parses CONQUER.INI for certain options *
 *                                                                                             *
 * INPUT: * none.
 **
 *                                                                                             *
 * OUTPUT: * none.
 **
 *                                                                                             *
 * WARNINGS: * none.
 **
 *                                                                                             *
 * HISTORY: * 08/18/1995 BRR : Created. *
 *=============================================================================================*/
void Parse_INI_File() {
  char* buffer;  // INI staging buffer pointer.
  char buf[128];
  static char section[40];
  static char entry[40];
  static char name[40];
  int len;
  int i;

  /*
  ** These arrays store the coded version of the names Geologic, Period, &
  *Jurassic.
  ** Decode them by subtracting 83.  For you curious types, the names look like:
  ** ��¿º��
  ** ��ż·
  ** ��Ŵ�Ƽ�
  ** If these INI entries aren't found, the IsJurassic flag does nothing.
  */
  static char coded_section[] = {154, 184, 194, 191, 194, 186, 188, 182, 0};
  static char coded_entry[] = {163, 184, 197, 188, 194, 183, 0};
  static char coded_name[] = {157, 200, 197, 180, 198, 198, 188, 182, 0};

  /*------------------------------------------------------------------------
  Fetch working pointer to the INI staging buffer. Make sure that the buffer
  is cleared out before proceeding.
  ------------------------------------------------------------------------*/
  buffer = (char*)ShapeBuffer;
  memset(buffer, '\0', ShapeBufferSize);

  /*------------------------------------------------------------------------
  Decode the desired section, entry, & name
  ------------------------------------------------------------------------*/
  strcpy(section, coded_section);
  len = strlen(coded_section);
  for (i = 0; i < len; i++) {
    section[i] -= 83;
  }

  strcpy(entry, coded_entry);
  len = strlen(coded_entry);
  for (i = 0; i < len; i++) {
    entry[i] -= 83;
  }

  strcpy(name, coded_name);
  len = strlen(coded_name);
  for (i = 0; i < len; i++) {
    name[i] -= 83;
  }

  /*------------------------------------------------------------------------
  Create filename and read the file.
  ------------------------------------------------------------------------*/
  GameFile file("CONQUER.INI");
  if (!file.IsAvailable()) {
    return;
  } else {
    file.Read(buffer, ShapeBufferSize - 1);
  }
  file.Close();

  WWGetPrivateProfileString(section, entry, "", buf, buffer);

  if (absl::EqualsIgnoreCase(buf, name)) {
    AreThingiesEnabled = true;
  }

  memset(section, 0, sizeof(section));
  memset(entry, 0, sizeof(entry));
  memset(name, 0, sizeof(name));
}
#endif

/***********************************************************************************************
 * Version_Number -- Determines the version number. *
 *                                                                                             *
 *    This routine will determine the version number by analyzing the date and
 *teim that the   * program was compiled and then generating a unique version
 *number based on it. The        * version numbers are guaranteed to be larger
 *for later dates.                             *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the version number. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/24/1995 JLB : Created. *
 *=============================================================================================*/
int Version_Number() {
#ifdef OBSOLETE
  static bool initialized = false;
  static int version;
  static char* date = __DATE__;
  static char* time = __TIME__;
  static const char* months = "JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC";

  if (!initialized) {
    char* ptr;
    char* tok;

    /*
    **	Fetch the month and place in the first two digit positions.
    */
    strupr(date);
    tok = strtok(date, " ");
    ptr = strstr(months, tok);
    if (ptr) {
      version = (((ptr - months) / 3) + 1) * 10000;
    }

    /*
    **	Fetch the date and place that in the next two digit positions.
    */
    tok = strtok(NULL, " ");
    if (tok) {
      version += atoi(tok) * 100;
    }

    /*
    **	Fetch the time and place that in the last two digit positions.
    */
    tok = strtok(time, ": ");
    if (tok) {
      version += atoi(tok);
    }

    /*
    **	Fetch the virgin text file (if present).
    */
    DiskFile file("VERSION.TXT");
    if (file.IsAvailable()) {
      file.ReadObject(VersionText);
      VersionText[sizeof(VersionText) - 1] = '\0';
      while (VersionText[sizeof(VersionText) - 1] == '\r') {
        VersionText[sizeof(VersionText) - 1] = '\0';
      }
    } else {
      VersionText[0] = '\0';
    }

    initialized = true;
  }
  return (version);
#endif

#ifdef FRENCH
  sprintf(VersionText, ".02");  // Win95 french version number
#endif                          // FRENCH

#ifdef GERMAN
  sprintf(VersionText, ".01");  // Win95 german version number
#endif                          // GERMAN

#ifdef JAPANESE
  sprintf(VersionText, ".01");  // Win95 german version number
#endif                          // GERMAN

#if !(defined(FRENCH) || defined(GERMAN) || defined(JAPANESE))
  absl::SNPrintF(VersionText, sizeof(VersionText),
                 ".07");        // Win95 USA version number
#endif                          // FRENCH | GERMAN

  DiskFile file("VERSION.TXT");
  char version[16];
  base::FillBytes(base::ObjectBytes(version), 0, sizeof(version));
  if (file.IsAvailable()) {
    file.ReadObject(version);
  }
  port::SafeAppend(
      VersionText,
      std::string_view(version,
                       static_cast<size_t>(std::ranges::find(version, '\0') -
                                           std::begin(version))));

#ifdef FRENCH
  return (1);  // Win95 french version number
#endif         // FRENCH

#ifdef GERMAN
  return (1);  // Win95 german version number
#endif         // GERMAN

#ifdef JAPANESE
  return (1);  // Win95 german version number
#endif         // GERMAN

#if !(defined(FRENCH) || defined(GERMAN) || defined(JAPANESE))
  return 1;  // Win95 USA version number
#endif       // FRENCH | GERMAN
}

/***************************************************************************
 * Save_Recording_Values -- Saves recording values to a recording file     *
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
 *   05/15/1995 BRR : Created.                                             *
 *=========================================================================*/
void Save_Recording_Values() {
  RecordFile.WriteObject(GameToPlay);
  RecordFile.WriteObject(ModemGameToPlay);
  RecordFile.WriteObject(BuildLevel);
  RecordFile.WriteObject(MPlayerName);
  RecordFile.WriteObject(MPlayerPrefColor);
  RecordFile.WriteObject(MPlayerColorIdx);
  RecordFile.WriteObject(MPlayerHouse);
  RecordFile.WriteObject(MPlayerLocalID);
  RecordFile.WriteObject(MPlayerCount);
  RecordFile.WriteObject(MPlayerBases);
  RecordFile.WriteObject(MPlayerCredits);
  RecordFile.WriteObject(MPlayerTiberium);
  RecordFile.WriteObject(MPlayerGoodies);
  RecordFile.WriteObject(MPlayerGhosts);
  RecordFile.WriteObject(MPlayerUnitCount);
  RecordFile.WriteObject(MPlayerID);
  RecordFile.WriteObject(MPlayerHouses);
  RecordFile.WriteObject(Seed);
  RecordFile.WriteObject(Scenario);
  RecordFile.WriteObject(ScenPlayer);
  RecordFile.WriteObject(ScenDir);
  RecordFile.WriteObject(Whom);
  RecordFile.WriteObject(Special);
  RecordFile.WriteObject(Options);
  RecordFile.WriteObject(FrameSendRate);
  RecordFile.WriteObject(CommProtocol);

  if (SuperRecord) {
    RecordFile.Close();
  }
}

/***************************************************************************
 * Load_Recording_Values -- Loads recording values from recording file     *
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
 *   05/15/1995 BRR : Created.                                             *
 *=========================================================================*/
void Load_Recording_Values() {
  Read_MultiPlayer_Settings();

  RecordFile.ReadObject(GameToPlay);
  RecordFile.ReadObject(ModemGameToPlay);
  RecordFile.ReadObject(BuildLevel);
  RecordFile.ReadObject(MPlayerName);
  RecordFile.ReadObject(MPlayerPrefColor);
  RecordFile.ReadObject(MPlayerColorIdx);
  RecordFile.ReadObject(MPlayerHouse);
  RecordFile.ReadObject(MPlayerLocalID);
  RecordFile.ReadObject(MPlayerCount);
  RecordFile.ReadObject(MPlayerBases);
  RecordFile.ReadObject(MPlayerCredits);
  RecordFile.ReadObject(MPlayerTiberium);
  RecordFile.ReadObject(MPlayerGoodies);
  RecordFile.ReadObject(MPlayerGhosts);
  RecordFile.ReadObject(MPlayerUnitCount);
  RecordFile.ReadObject(MPlayerID);
  RecordFile.ReadObject(MPlayerHouses);
  RecordFile.ReadObject(Seed);
  RecordFile.ReadObject(Scenario);
  RecordFile.ReadObject(ScenPlayer);
  RecordFile.ReadObject(ScenDir);
  RecordFile.ReadObject(Whom);
  RecordFile.ReadObject(Special);
  RecordFile.ReadObject(Options);
  RecordFile.ReadObject(FrameSendRate);
  RecordFile.ReadObject(CommProtocol);
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
  Load_Title_Screen("HTITLE.PCX", &HidPage, Palette);

  if (visible) {
    HidPage.Blit(SeenBuff);
  }
}
