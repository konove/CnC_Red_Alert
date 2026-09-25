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
 * Functions: *
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
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/log/log.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "engine/base/array.h"
#include "engine/base/buffer.h"
#include "engine/base/strings/number_parse.h"
#include "engine/base/strings/safe_string.h"
#include "engine/platform/file_system.h"
#include "engine/platform/random_seed.h"
#include "engine/platform/timer.h"
#include "engine/stream/byte_stream.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/pixel_buffer.h"
#include "sdllib/shape.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "td/aircraft.h"
#include "td/anim.h"
#include "td/assets.h"
#include "td/base.h"
#include "td/building.h"
#include "td/bullet.h"
#include "td/config.h"
#include "td/conquer.h"
#include "td/debug_state.h"
#include "td/defines.h"
#include "td/dialog.h"
#include "td/expand.h"
#include "td/factory.h"
#include "td/game_clock.h"
#include "td/game_state.h"
#include "td/goptions.h"
#include "td/heap.h"
#include "td/house.h"
#include "td/infantry.h"
#include "td/ini.h"
#include "td/inline.h"
#include "td/input.h"
#include "td/intro.h"
#include "td/ipx.h"
#include "td/ipxaddr.h"
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
#include "td/network.h"
#include "td/nulldlg.h"
#include "td/nullmgr.h"
#include "td/object_heaps.h"
#include "td/overlay.h"
#include "td/palette.h"
#include "td/palettes.h"
#include "td/queue.h"
#include "td/randomstate.h"
#include "td/saveload.h"
#include "td/scenario.h"
#include "td/score.h"
#include "td/screen.h"
#include "td/session.h"
#include "td/smudge.h"
#include "td/special.h"
#include "td/startup.h"
#include "td/startup_options.h"
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
#include "td/winstub.h"
#include "td/world.h"
#include "tech/audio_mixer.h"
#include "tech/disk_file.h"
#include "tech/file_access.h"
#include "tech/game_file.h"
#include "tech/key_phrase_hash.h"
#include "tech/mix_archive.h"
#include "tech/search_paths.h"

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
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   Only call this ONCE! *
 *                                                                                             *
 * HISTORY: * 10/07/1992 JLB : Created. *
 *=============================================================================================*/
bool Init_Game() {
  Assets::DiscArchives& archives = TheAssets().disc_archives();
  std::span<const std::byte> temp_mouse_shapes;

  /*
  **	Initialize the game object heaps.
  */
  DLOG(INFO) << "C&C95 - About to enter Units.Set_Heap";
  TheObjectHeaps().unit().Set_Heap(kUnitMax);
  DLOG(INFO) << "C&C95 - About to enter Factories.Set_Heap";
  TheObjectHeaps().factory().Set_Heap(kFactoryMax);
  DLOG(INFO) << "C&C95 - About to enter Terrains.Set_Heap";
  TheObjectHeaps().terrain().Set_Heap(kTerrainMax);
  DLOG(INFO) << "C&C95 - About to enter Templates.Set_Heap";
  TheObjectHeaps().tmplate().Set_Heap(kTemplateMax);
  DLOG(INFO) << "C&C95 - About to enter Smudges.Set_Heap";
  TheObjectHeaps().smudge().Set_Heap(kSmudgeMax);
  DLOG(INFO) << "C&C95 - About to enter Overlays.Set_Heap";
  TheObjectHeaps().overlay().Set_Heap(kOverlayMax);
  DLOG(INFO) << "C&C95 - About to enter Infantry.Set_Heap";
  TheObjectHeaps().infantry().Set_Heap(kInfantryMax);
  DLOG(INFO) << "C&C95 - About to enter Bullets.Set_Heap";
  TheObjectHeaps().bullet().Set_Heap(kBulletMax);
  DLOG(INFO) << "C&C95 - About to enter Buildings.Set_Heap";
  TheObjectHeaps().building().Set_Heap(kBuildingMax);
  DLOG(INFO) << "C&C95 - About to enter Anims.Set_Heap";
  TheObjectHeaps().anim().Set_Heap(kAnimMax);
  DLOG(INFO) << "C&C95 - About to enter Aircraft.Set_Heap";
  TheObjectHeaps().aircraft().Set_Heap(kAircraftMax);
  DLOG(INFO) << "C&C95 - About to enter Triggers.Set_Heap";
  TheObjectHeaps().trigger().Set_Heap(kTriggerMax);
  DLOG(INFO) << "C&C95 - About to enter TeamTypes.Set_Heap";
  TheObjectHeaps().team_type().Set_Heap(kTeamTypeMax);
  DLOG(INFO) << "C&C95 - About to enter Teams.Set_Heap";
  TheObjectHeaps().team().Set_Heap(kTeamMax);
  DLOG(INFO) << "C&C95 - About to enter Houses.Set_Heap";
  TheObjectHeaps().house().Set_Heap(kHouseMax);

  /*
  **	Initialize all the waypoints to invalid values.
  */
  DLOG(INFO) << "C&C95 - About to clear waypoints";
  base::FillBytes(base::ObjectBytes(TheWorld().waypoint()), 0xFF,
                  sizeof(TheWorld().waypoint()));

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
  TheKeyboard().Clear();

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
  const int temp = TheGameState().required_cd();
  TheGameState().required_cd() = -2;

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

  TheGameState().required_cd() = temp;

#endif
  DLOG(INFO) << "C&C95 - About to load fonts";
  TheAssets().LoadFonts();
  ThePalettes().black_palette().assign(768, 0);
  ThePalettes().game_palette().assign(768, 0);
  ThePalettes().original_palette().assign(768, 0);
  ThePalettes().white_palette().assign(768, 0);
  std::ranges::fill(ThePalettes().white_palette(), 63);

  DLOG(INFO) << "C&C95 - About to set palette";
  std::ranges::fill(ThePalettes().black_palette(), 0x01);
  if (!TheSpecial().IsFromInstall) {
    Set_Palette(ThePalettes().black_palette());
  }
  std::ranges::fill(ThePalettes().black_palette(), 0);
  if (!TheSpecial().IsFromInstall) {
    Set_Palette(ThePalettes().black_palette());
    DLOG(INFO) << "C&C95 - About to clear visible page";
    TheScreen().visible_page().view().Clear();
  }

  Set_Palette(ThePalettes().game_palette());

  DLOG(INFO) << "C&C95 - About to set the mouse shape";
  /*
  ** Since there is no mouse shape currently available we need'
  ** to set one of our own.
  */
  if (TheMouse() != nullptr) {
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
  } while (!TheGameState().in_focus());

  DLOG(INFO) << "C&C95 - About to load the language file";
  TheAssets().LoadStrings();

  /*
  **	Default palette initialization. Uses the desert palette for convenience,
  **	but only the non terrain specific colors matter.
  */
  if (const auto palfile = OpenGameFile("TEMPERAT.PAL")) {
    palfile->Read(std::span(ThePalettes().game_palette()), 768L);
  }

  if (TheMouse() == nullptr) {
    char buffer[255];
    Set_Palette(ThePalettes().game_palette());
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
    ShutDown();
    exit(1);
  }

#ifdef DEMO
  /*
  **	Add in any override path specified in the conquer.ini file.
  */
  if (!TheGameState().override_path().empty()) {
    GameFile::Set_Search_Drives(TheGameState().override_path().c_str());
  }
#endif

  SearchPaths::Add(".");  // allow running without CD

  /*
  **	Always try to look at the CD-ROM for data files.
  */
  if (!SearchPaths::HasAny()) {
    // Without a search path there is nowhere to read the data from. The
    // original fell back on the CD drive here, but SDL enumerates no drives,
    // so the scan for the disc had nothing to scan and this always ended in
    // the same error.
    Set_Palette(ThePalettes().game_palette());
    Show_Mouse();
    CCMessageBox().Process(TXT_CD_ERROR1, TXT_OK);
    ShutDown();
    exit(EXIT_FAILURE);
  }

  /*
  ** If there are search drives specified then all files are to be
  ** considered local.
  */
  TheGameState().required_cd() = -2;
#ifndef DEMO
  DLOG(INFO) << "C&C95 - About to register addon mixfiles";
  /*
  **	Before all else, cache any additional mixfiles.
  */
  for (const FoundFile& found : FindFiles("SC*.MIX")) {
    // don't cache scores
    if (absl::EqualsIgnoreCase(found.name, "scores.mix")) {
      continue;
    }

    (void)MixArchive::Register(found.name);
    MixArchive::Cache(found.name);
  }
  for (const FoundFile& found : FindFiles("SS*.MIX")) {
    (void)MixArchive::Register(found.name);
  }
#endif  // DEMO

  DLOG(INFO) << "C&C95 - About to register GENERAL.MIX";
  MixArchive::Unregister("GENERAL.MIX");
  archives.general = MixArchive::Register("GENERAL.MIX");

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
    if (archives.movies == nullptr) {
      archives.movies = MixArchive::Register("DEMOM.MIX");
    }
    TheGameState().scores_present() = true;
    ThemeClass::Scan();
  }

#else
  DLOG(INFO) << "C&C95 - About to register CONQUER.MIX";
  (void)MixArchive::Register("CONQUER.MIX");
  DLOG(INFO) << "C&C95 - About to register TRANSIT.MIX";
  (void)MixArchive::Register("TRANSIT.MIX");

  DLOG(INFO) << "C&C95 - About to register GENERAL.MIX";
  if (archives.general == nullptr) {
    archives.general = MixArchive::Register("GENERAL.MIX");  // Never cached.
  }

  //	if (GameFile("MOVIES.MIX").IsAvailable()) {
  DLOG(INFO) << "C&C95 - About to register MOVIES.MIX";
  if (archives.movies == nullptr) {
    archives.movies = MixArchive::Register("MOVIES.MIX");  // Never cached.
                                                           //	}
  }

  /*
  **	Register the score mixfile.
  */
  DLOG(INFO) << "C&C95 - About to register SCORES.MIX";
  TheGameState().scores_present() = false;
  //	if (GameFile("SCORES.MIX").IsAvailable()) {
  TheGameState().scores_present() = true;
  if (archives.score == nullptr) {
    archives.score = MixArchive::Register("SCORES.MIX");
    ThemeClass::Scan();
  }
//	}
#endif

  /*
  **	These are sound card specific, but the install program would have
  **	copied the coorect versions to the hard drive.
  */
  DLOG(INFO) << "C&C95 - About to register SPEECH.MIX";
  if (GameFileExists("SPEECH.MIX")) {
    (void)MixArchive::Register("SPEECH.MIX");  // Never cached.
  }
  DLOG(INFO) << "C&C95 - About to register SOUNDS.MIX";
  (void)MixArchive::Register("SOUNDS.MIX");

  if (TheGameState().spawned_from_chat()) {
    TheSpecial().IsFromWChat = true;
  }

  /*
  **	Play the introduction movies.
  */
  DLOG(INFO) << "C&C95 - About to play the intro movie";
  if (!TheSpecial().IsFromInstall && !TheSpecial().IsFromWChat) {
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

  if (!TheSpecial().IsFromInstall) {
    Load_Title_Page(true);
  }

  Hide_Mouse();
  Wait_Vert_Blank();
  if (!TheSpecial().IsFromInstall) {
    Set_Palette(ThePalettes().title_palette());
    TheScreen().hidden_view().BlitTo(TheScreen().visible_view());
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
  if (TheAudio().is_open() && !TheDebugState().quiet()) {
    MixArchive::Cache("SOUNDS.MIX");
    if (TheSpecial().IsJuvenile) {
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
  TheMap().One_Time();
  //	malloc(4);
  TheWorld().logic().One_Time();
  //	malloc(5);
  TheOptions().One_Time();

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

  Call_Back();

  /*
  **	WWLIB bug: MouseState is in some undefined state; show the mouse until
  **	it really shows.
  */
  TheMap().Set_Default_Mouse(MOUSE_NORMAL, false);
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
  TheSession().games_played() = 0;
  TheSession().score_count() = 0;
  TheSession().current_game() = 0;
  for (auto& i : TheSession().scores()) {
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
  std::ranges::copy(ThePalettes().title_palette(),
                    ThePalettes().game_palette().begin());
  std::ranges::copy(ThePalettes().title_palette(),
                    ThePalettes().original_palette().begin());

  /*
  **	Read game options, so the GameSpeed is initialized when multiplayer
  ** dialogs are invoked.  (GameSpeed must be synchronized between systems.)
  */
  TheOptions().Load_Settings();

  return true;
}

void Uninit_Game() {
  // The audio thread keeps mixing whatever is playing, straight out of the
  // speech buffer and the MIX archives freed below; stop it first.
  TheAudio().Close();

  delete MouseClass::ShadowPage;
  MouseClass::ShadowPage = nullptr;
  TheMap().Free_Cells();

  SearchPaths::Clear();
  MixArchive::Free_All();

  TheObjectHeaps().unit().Set_Heap(0);
  TheObjectHeaps().factory().Set_Heap(0);
  TheObjectHeaps().terrain().Set_Heap(0);
  TheObjectHeaps().tmplate().Set_Heap(0);
  TheObjectHeaps().smudge().Set_Heap(0);
  TheObjectHeaps().overlay().Set_Heap(0);
  TheObjectHeaps().infantry().Set_Heap(0);
  TheObjectHeaps().bullet().Set_Heap(0);
  TheObjectHeaps().building().Set_Heap(0);
  TheObjectHeaps().anim().Set_Heap(0);
  TheObjectHeaps().aircraft().Set_Heap(0);
  TheObjectHeaps().trigger().Set_Heap(0);
  TheObjectHeaps().team_type().Set_Heap(0);
  TheObjectHeaps().team().Set_Heap(0);
  TheObjectHeaps().house().Set_Heap(0);

  Set_Shape_Buffer({});
  shape_storage.clear();
  shape_storage.shrink_to_fit();
  ThePalettes().black_palette().clear();
  ThePalettes().game_palette().clear();
  ThePalettes().original_palette().clear();
  ThePalettes().white_palette().clear();
  ThePalettes().title_palette().clear();
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

// -NEWGAME and -LOADGAME each start one game. The startup path sets this
// when it has acted on the request, so coming back from that game shows the
// menu instead of starting it again.
static bool startup_game_started = false;

// A single legacy dialog loop; splitting it is a refactor of its own.
// NOLINTNEXTLINE(readability-function-size)
bool Select_Game(bool fade) {
  const StartupOptions& options = TheStartupOptions();

  // A -QUITFRAME run has ended when the game it started (-NEWGAME or
  // -LOADGAME, each started once) brings control back here. Leave rather
  // than wait at the menu for input that never comes.
  const bool startup_game_pending =
      !startup_game_started &&
      (!options.new_game.empty() || options.load_game >= 0);
  if (options.quit_at_frame >= 0 && !startup_game_pending) {
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

  if (TheSpecial().IsFromInstall) {
    {
      display = false;
      Show_Mouse();
    }
  }

  /*
  **	[Re]set any globals that need it, in preparation for a new scenario
  */
  TheGameState().active() = true;
  TheNetwork().do_list().Init();
  TheNetwork().out_list().Init();
  TheGameClock().set_frame(0);
  TheGameState().player_wins() = false;
  TheGameState().player_loses() = false;
  TheSession().obi_wan() = false;
  TheDebugState().set_unshroud(false);
  TheMap().Set_Cursor_Shape({});
  TheMap().PendingObjectPtr = nullptr;
  TheMap().PendingObject = nullptr;
  TheMap().PendingHouse = HOUSE_NONE;

  /*
  ** Initialize multiplayer-protocol-specific variables:
  ** If CommProtocol MULTI_E_COMP is used, you must:
  ** Init FrameSendRate to a sensible value (3 is good)
  ** Init MPlayerMaxAhead to an even multiple of FrameSendRate, and it must
  **   be at least 2 * MPlayerMaxAhead
  */
  TheSession().comm_protocol() = COMM_PROTOCOL_SINGLE_NO_COMP;
  if (!TheSpecial().IsFromWChat) {
    TheSession().frame_send_rate() = 3;
  }

  TheSession().process_ticks() = 0;
  TheSession().process_frames() = 0;
  TheSession().desired_frame_rate() = 30;
  // #if(TIMING_FIX)
  TheNetwork().new_max_ahead_frame1() = 0;
  TheNetwork().new_max_ahead_frame2() = 0;
  // #endif

  /*
  **	Init multiplayer game scores.  Let Wins accumulate; just init the
  *current
  ** Kills for this game.  Kills of -1 means this player didn't play this round.
  */
  for (int i = 0; i < MAX_MULTI_GAMES; i++) {
    base::At(base::At(TheSession().scores(), i).Kills,
             TheSession().current_game()) = -1;
  }

  /*
  **	Set default mouse shape
  */
  TheMap().Set_Default_Mouse(MOUSE_NORMAL, false);

  /*
  **	If the last game we played was a multiplayer game, jump right to that
  **	menu by pre-setting 'selection'.
  */
  if (TheSession().type() == GAME_NORMAL) {
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
    TheWorld().scenario_init()++;
    TheTheme().Queue_Song(THEME_MAP1);
    TheWorld().scenario_init()--;

    /*
    ** If we're playing back a recording, load all pertinant values & skip
    ** the menu loop.  Hide the now-useless mouse pointer.
    */
    if (TheSession().playback_game()) {
      if (std::unique_ptr<ByteStream> record =
              OpenGameFile(TheSession().record_file_name())) {
        Load_Recording_Values(*record);
        TheSession().record_stream() = std::move(record);
        process = false;
        TheTheme().Fade_Out();
      } else {
        TheSession().playback_game() = false;
      }
    }

    /*
    ** Handle case where we were spawned from Wchat
    */
    if (TheGameState().spawned_from_chat()) {
      TheSpecial().IsFromInstall =
          false;  // Dont play intro if we were spawned from wchat
      selection = kSelInternet;
      TheTheme().Queue_Song(THEME_NONE);
      TheSession().type() = GAME_INTERNET;
      display = false;
    }

    while (process) {
      if (!startup_game_started && options.new_game.size() >= 5) {
        TheWorld().scenario() = tech::ParseIntegerOr<int>(
            std::string_view{options.new_game}.substr(3, 2), 0);
        TheWorld().scen_player() =
            options.new_game.at(2) == 'B' ? SCEN_PLAYER_NOD : SCEN_PLAYER_GDI;
        TheWorld().whom() = TheWorld().scen_player() == SCEN_PLAYER_NOD
                                ? HOUSE_BAD
                                : HOUSE_GOOD;
        TheSession().type() = GAME_NORMAL;
        process = false;
        continue;
      }
      if (!startup_game_started && options.load_game >= 0) {
        const int slot = options.load_game;
        startup_game_started = true;
        if (!Load_Game(slot)) {
          LOG(ERROR) << "-LOADGAME: could not load slot " << slot;
          return false;
        }
        gameloaded = true;
        process = false;
        continue;
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
        std::ranges::copy(ThePalettes().title_palette(),
                          ThePalettes().game_palette().begin());

        if (fade) {
          Fade_Palette_To(ThePalettes().title_palette(), kFadePaletteSlow,
                          Call_Back);
          fade = false;
        }

        PixelView& view = TheScreen().visible_view();
        if constexpr (config::kVirginCheatKeysEnabled) {
          Fancy_Text_Print(
              view, "V.%d%s", TheScreen().visible_view().width() - 1,
              TheScreen().visible_view().height() - 10, kGrey, kTBlack,
              TPF_6POINT | TPF_FULLSHADOW | TPF_RIGHT, Version_Number(),
              TheGameState().version_text(), FOREIGN_VERSION_NUMBER);
        } else {
#ifdef DEMO
          Version_Number();
          Fancy_Text_Print(view, "DEMO V%s",
                           TheScreen().visible_view().width() - 1,
                           TheScreen().visible_view().height() - 10, kGrey,
                           kTBlack, TPF_6POINT | TPF_FULLSHADOW | TPF_RIGHT,
                           TheGameState().version_text());
#else
          Fancy_Text_Print(view, "V.%d%s",
                           TheScreen().visible_view().width() - 1,
                           TheScreen().visible_view().height() - 10, kGrey,
                           kTBlack, TPF_6POINT | TPF_FULLSHADOW | TPF_RIGHT,
                           Version_Number(), TheGameState().version_text());
#endif
        }
        display = false;
        Show_Mouse();
      }

      /*
      **	Display menu and fetch selection from player.
      */
      if (TheSpecial().IsFromInstall) {
        selection = kSelStartNewGame;
        TheTheme().Queue_Song(THEME_NONE);
      }

#ifdef _WIN32
      /*
      ** Handle case where we were spawned from Wchat
      */
      if (TheSpecial().IsFromWChat && DDEServer.Get_MPlayer_Game_Info()) {
        Check_From_WChat(NULL);
        selection = kSelMultiplayerGame;
        TheTheme().Queue_Song(THEME_NONE);
        TheSession().type() = GAME_INTERNET;
      } else {
        /*
        ** We werent spawned but we could still receive a DDE packet from wchat
        */
        if (DDEServer.Get_MPlayer_Game_Info()) {
          Check_From_WChat(NULL);
          /*
          ** Make sure top and bottom of screen are clear in 640x480 mode
          */
          if (TheScreen().mode_height() == 480) {
            TheScreen().visible_page().FillRect(0, 0, 639, 40, 0);
            TheScreen().visible_page().FillRect(0, 440, 639, 479, 0);
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
              TheSession().type() = GAME_INTERNET;
            } else {
              selection = kSelNone;
              display = true;
            }
          } else {
            DLOG(INFO) << "C&C95 - About to call Check_From_WChat.";
            Check_From_WChat(NULL);
            display = false;
            TheSession().type() = GAME_INTERNET;
            selection = kSelMultiplayerGame;
          }
#endif
          break;

        /*
        **	Pick an expansion scenario.
        */
        case kSelNewScenario:
          TheWorld().carry_over_money() = 0;
          if (Expansion_Dialog()) {
            TheTheme().Fade_Out();
            //						Theme.Queue_Song(THEME_AOI);
            TheSession().type() = GAME_NORMAL;
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
          TheWorld().carry_over_money() = 0;

          /*
          ** Ensure that CD1 or CD2 is in the drive. These missions
          ** are not on the covert CD.
          */
          cd_index = Get_CD_Index(SearchPaths::current_cd_drive(), 1 * 60);
          /*
          ** If cd_index == 2 then its a covert CD
          */
          if (cd_index == 2) {
            TheGameState().required_cd() = 0;
            if (!Force_CD_Available(TheGameState().required_cd())) {
              ShutDown();
              exit(EXIT_FAILURE);
            }
          }

          if (Bonus_Dialog()) {
            TheTheme().Fade_Out();
            TheSession().type() = GAME_NORMAL;
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
          TheWorld().carry_over_money() = 0;

#ifdef DEMO
          Hide_Mouse();
          Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium,
                          Call_Back);
          Load_Title_Screen("PREPICK.PCX", &TheScreen().hidden_view(),
                            ThePalettes().title_palette());
          TheScreen().hidden_view().BlitTo(TheScreen().visible_view());
          Fade_Palette_To(ThePalettes().title_palette(), kFadePaletteMedium,
                          Call_Back);
          Clear_KeyBuffer();
          while (!Check_Key_Num()) {
            Call_Back();
          }
          Get_Key_Num();
          Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium,
                          Call_Back);
          Show_Mouse();

          TheWorld().scenario() = 1;
          TheWorld().build_level() = 1;
#else
          TheWorld().scenario() = 1;
          TheWorld().build_level() = 1;
#endif
          TheWorld().scen_player() = SCEN_PLAYER_GDI;
          TheWorld().scen_dir() = SCEN_DIR_EAST;
          TheWorld().whom() = HOUSE_GOOD;

#ifndef DEMO
          TheTheme().Fade_Out();
          Choose_Side();
#endif

          /*
          ** If user is playing special mode, do NOT change Whom; leave it set
          *to
          ** GDI or NOD.  Ini.cpp will set the player's ActLike to mirror the
          ** Whom value.
          */
          if (TheSpecial().IsJurassic && TheGameState().thingies_enabled()) {
            TheWorld().scen_player() = SCEN_PLAYER_JP;
            TheWorld().scen_dir() = SCEN_DIR_EAST;
          }

          TheSession().type() = GAME_NORMAL;
          process = false;
          break;

        /*
        **	Load a saved game.
        */
        case kSelLoadMission:
          if (LoadOptionsClass(LoadOptionsClass::LOAD).Process()) {
            // Theme.Fade_Out();
            TheTheme().Queue_Song(THEME_AOI);
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
          Set_Palette(ThePalettes().black_palette());
          Load_Title_Screen("DEMOPIC.PCX", &TheScreen().hidden_view(),
                            ThePalettes().title_palette());
          TheScreen().hidden_view().BlitTo(TheScreen().visible_view());
          Fade_Palette_To(ThePalettes().title_palette(), kFadePaletteMedium,
                          Call_Back);
          Clear_KeyBuffer();
          while (!Check_Key()) {
            Call_Back();
          }
          Get_Key();
          Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium,
                          Call_Back);
          Show_Mouse();
          display = true;
          fade = true;
          selection = kSelNone;
#else
          switch (TheSession().type()) {
            /*
            **	If 'GameToPlay' isn't already set up for a multiplayer game,
            **	we must prompt the user for which type of multiplayer game
            **	they want.
            */
            case GAME_NORMAL:
              TheSession().type() = Select_MPlayer_Game();
              if (TheSession().type() == GAME_NORMAL) {  // 'Cancel'
                display = true;
                selection = kSelNone;
              }
              break;

            case GAME_NULL_MODEM:
            case GAME_MODEM:
              if (TheNetwork().null_modem().Num_Connections()) {
                TheNetwork().null_modem().Init_Send_Queue();

                if ((TheSession().type() == GAME_NULL_MODEM &&
                     TheNetwork().modem_game_type() == MODEM_NULL_HOST) ||
                    (TheSession().type() == GAME_MODEM &&
                     TheNetwork().modem_game_type() == MODEM_DIALER)) {
                  if (!Com_Scenario_Dialog()) {
                    TheSession().type() = Select_Serial_Dialog();
                    if (TheSession().type() ==
                        GAME_NORMAL) {  // user hit Cancel
                      display = true;
                      selection = kSelNone;
                    }
                  }
                } else {
                  if (!Com_Show_Scenario_Dialog()) {
                    TheSession().type() = Select_Serial_Dialog();
                    if (TheSession().type() ==
                        GAME_NORMAL) {  // user hit Cancel
                      display = true;
                      selection = kSelNone;
                    }
                  }
                }
              } else {
                TheSession().type() = Select_MPlayer_Game();
                if (TheSession().type() == GAME_NORMAL) {  // 'Cancel'
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
              if (TheSpecial().IsFromWChat) {
                // MessageBox (NULL, "About to restore focus to C&C95", "C&C95",
                // MB_OK);
                DLOG(INFO) << "C&C95 - About to give myself focus.";

                DLOG(INFO) << "C&C95 - About to initialise Winsock.";
                if (TheNetwork().winsock().Init()) {
                  DLOG(INFO) << "C&C95 - About to read multiplayer settings.";
                  Read_MultiPlayer_Settings();
                  TheNetwork().is_server() = TheNetwork().westwood_is_host();

                  DLOG(INFO) << "C&C95 - About to set addresses.";
                  TheNetwork().winsock().Set_Host_Address(
                      TheNetwork().westwood_address());

                  DLOG(INFO)
                      << "C&C95 - About to call Start_Server or Start_Client.";
                  if (TheNetwork().is_server()) {
                    TheNetwork().modem_game_type() = INTERNET_HOST;
                    TheNetwork().winsock().Start_Server();
                  } else {
                    TheNetwork().modem_game_type() = INTERNET_JOIN;
                    TheNetwork().winsock().Start_Client();
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
                  while (TheNetwork().winsock().Read(temp_buffer, 1024)) {
                    DLOG(INFO) << "C&C95 - Discarding a packet.";
                    ptimer.Set(30, true);
                    while (ptimer.Time()) {
                    }
                    DLOG(INFO) << "C&C95 - Ready to check for more packets.";
                  }
                  DLOG(INFO) << "C&C95 - About to delete scrap memory.";

                } else {
                  DLOG(INFO) << "C&C95 - Winsock failed to initialise.";
                  TheSession().type() = GAME_NORMAL;
                  selection = kSelExit;
                  TheSpecial().IsFromWChat = false;
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

                if (TheNetwork().is_server()) {
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
                  TheNetwork().winsock().Close();
                  TheSession().type() = GAME_NORMAL;
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
                TheNetwork().winsock().Close();
                TheSession().type() = GAME_NORMAL;
                selection = kSelNone;
#ifdef _WIN32
                DDEServer.Delete_MPlayer_Game_Info();  // Make sure we dont
                                                       // go round in an
                                                       // infinite loop
#endif
                // Special.IsFromWChat = false;
                break;
              }
              TheSession().type() = Select_MPlayer_Game();
              if (TheSession().type() == GAME_NORMAL) {  // 'Cancel'
                display = true;
                selection = kSelNone;
              }
              break;

#endif  // FORCE_WINSOCK
            case GameType::GAME_IPX:
            default:
              break;
          }

          switch (TheSession().type()) {
            /*
            **	Internet, Modem or Null-Modem
            */
            case GAME_MODEM:
            case GAME_NULL_MODEM:
            case GAME_INTERNET:
              TheTheme().Fade_Out();
              TheWorld().scen_player() = SCEN_PLAYER_2PLAYER;
              TheWorld().scen_dir() = SCEN_DIR_EAST;
              process = false;
              TheOptions().ScoreVolume = 0;
              break;

            /*
            **	Network (IPX): start a new network game.
            */
            case GAME_IPX:
              /*
              ** Init network system & remote-connect
              */
              if (Init_Network() && Remote_Connect()) {
                TheOptions().ScoreVolume = 0;
                TheWorld().scen_player() = SCEN_PLAYER_MPLAYER;
                TheWorld().scen_dir() = SCEN_DIR_EAST;
                process = false;
                TheTheme().Fade_Out();
              } else {  // user hit cancel, or init failed
                TheSession().type() = GAME_NORMAL;
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
          TheTheme().Fade_Out();
          TheTheme().Stop();
          Call_Back();

          Force_CD_Available(-1);
          Play_Intro(false);
          Hide_Mouse();

          // verify existence of movie file before playing this sequence.
          if (GameFileExists("TRAILER.VQA")) {
            Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium,
                            Call_Back);
            TheScreen().visible_page().view().Clear();
            if (const auto file = OpenGameFile("ATTRACT2.CPS")) {
              Load_Uncompress(*file, TheScreen().sys_mem_page().bytes(),
                              TheScreen().sys_mem_page().bytes(),
                              ThePalettes().title_palette());
              TheScreen().sys_mem_page().view().Scale(
                  TheScreen().visible_view(), 0, 0, 0, 0, 320, 199, 640, 398);
              Fade_Palette_To(ThePalettes().title_palette(), kFadePaletteMedium,
                              Call_Back);
            }
            Clear_KeyBuffer();
            count.Set(int64_t{kTimerSecond} * 3);
            while (count.Time()) {
              Call_Back();
            }
            Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium,
                            Call_Back);

            Play_Movie("TRAILER");  // Red Alert teaser.
          }

          if (GameFileExists("SIZZLE.VQA")) {
            Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium,
                            Call_Back);
            TheScreen().visible_page().view().Clear();
            if (const auto file = OpenGameFile("ATTRACT2.CPS")) {
              Load_Uncompress(*file, TheScreen().sys_mem_page().bytes(),
                              TheScreen().sys_mem_page().bytes(),
                              ThePalettes().title_palette());
              TheScreen().sys_mem_page().view().Scale(
                  TheScreen().visible_view(), 0, 0, 0, 0, 320, 199, 640, 398);
              Fade_Palette_To(ThePalettes().title_palette(), kFadePaletteMedium,
                              Call_Back);
            }
            Clear_KeyBuffer();
            count.Set(int64_t{kTimerSecond} * 3);
            while (count.Time()) {
              Call_Back();
            }
            Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium,
                            Call_Back);

            Play_Movie("SIZZLE");  // Red Alert teaser.
          }

          if (GameFileExists("SIZZLE2.VQA")) {
            Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium,
                            Call_Back);
            TheScreen().visible_page().view().Clear();
            if (const auto file = OpenGameFile("ATTRACT2.CPS")) {
              Load_Uncompress(*file, TheScreen().sys_mem_page().bytes(),
                              TheScreen().sys_mem_page().bytes(),
                              ThePalettes().title_palette());
              TheScreen().sys_mem_page().view().Scale(
                  TheScreen().visible_view(), 0, 0, 0, 0, 320, 199, 640, 398);
              Fade_Palette_To(ThePalettes().title_palette(), kFadePaletteMedium,
                              Call_Back);
            }
            Clear_KeyBuffer();
            count.Set(int64_t{kTimerSecond} * 3);
            while (count.Time()) {
              Call_Back();
            }
            Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium,
                            Call_Back);

            Play_Movie("SIZZLE2");  // Red Alert teaser.
          }

          Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium,
                          Call_Back);
          TheScreen().visible_page().view().Clear();
          if (const auto file = OpenGameFile("ATTRACT2.CPS")) {
            Load_Uncompress(*file, TheScreen().sys_mem_page().bytes(),
                            TheScreen().sys_mem_page().bytes(),
                            ThePalettes().title_palette());
            TheScreen().sys_mem_page().view().Scale(
                TheScreen().visible_view(), 0, 0, 0, 0, 320, 199, 640, 398);
            Fade_Palette_To(ThePalettes().title_palette(), kFadePaletteMedium,
                            Call_Back);
          }
          Clear_KeyBuffer();
          count.Set(int64_t{kTimerSecond} * 3);
          while (count.Time()) {
            Call_Back();
          }
          Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium,
                          Call_Back);

          Play_Movie("CC2TEASE");
          Show_Mouse();

          TheWorld().scenario_init()++;
          TheTheme().Play_Song(THEME_MAP1);
          TheWorld().scenario_init()--;
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
          TheTheme().Fade_Out();
          Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteSlow,
                          nullptr);
#ifdef JAPANESE
          TheScreen().visible_page().Clear();
#endif
          return false;

        /*
        **	Display the hall of fame.
        */
        case kSelFame:
          break;

        case kSelTimeout:
          if (std::unique_ptr<ByteStream> record =
                  TheSession().allow_attract()
                      ? OpenGameFile(TheSession().record_file_name())
                      : nullptr) {
            TheSession().playback_game() = true;
            Load_Recording_Values(*record);
            TheSession().record_stream() = std::move(record);
            process = false;
            TheTheme().Fade_Out();
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
    ** For TheDebugState().map_editor_active() (editor) mode, if JP option is
    * on, set to load that scenario
    */
    TheWorld().scenario() = 1;
    if (TheSpecial().IsJurassic && TheGameState().thingies_enabled()) {
      TheWorld().scen_player() = SCEN_PLAYER_JP;
      TheWorld().scen_dir() = SCEN_DIR_EAST;
    }
  }
  DLOG(INFO) << "C&C95 - About to start game initialisation.";
#ifdef FORCE_WINSOCK
  if (TheSession().type() == GAME_INTERNET) {
    TheSession().comm_protocol() = COMM_PROTOCOL_MULTI_E_COMP;
    if (!TheSpecial().IsFromWChat) {
      TheSession().frame_send_rate() = 5;  // 3;
    }
  }
#endif  // FORCE_WINSOCK
  /*
  **	Don't carry stray keystrokes into game.
  */
  TheKeyboard().Clear();

  /*
  **	Initialize the random number Seed.  For multiplayer, this will have been
  *done
  ** in the connection dialogs.  For single-player games, AND if we're not
  *playing
  ** back a recording, init the Seed to a random value.
  */
  if (TheSession().type() == GAME_NORMAL && !TheSession().playback_game()) {
    TheWorld().seed() = port::RandomSeed();
  }

  /*
  ** If user has specified a desired random number seed, use it for multiplayer
  *games
  */
  if (TheStartupOptions().custom_seed != 0) {
    TheWorld().seed() = TheStartupOptions().custom_seed;
  }

  /*
  ** Save initialization values if we're recording this game.
  ** This must be done after 'Seed' has been initialized.
  */
  if (TheSession().record_game()) {
    if (std::unique_ptr<ByteStream> record =
            OpenGameFile(TheSession().record_file_name(), FileAccess::kWrite)) {
      Save_Recording_Values(*record);
      TheSession().record_stream() = std::move(record);
    } else {
      TheSession().record_game() = false;
    }
  }

  /*
  **	Initialize the random-number generator.
  */
  // Loading already restored the exact stream positions.
  if (!gameloaded) {
    SeedGameRandom(static_cast<uint32_t>(TheWorld().seed()));
  }

  /*
  **	Load the scenario.  Specify variation 'A' for the editor; for the game,
  **	don't specify a variation, to make 'Set_Scenario_Name()' pick a random
  *one. *	Skip this if we've already loaded a save-game.
  */
  if (!gameloaded) {
    if (!startup_game_started && TheStartupOptions().new_game.size() >= 5) {
      port::SafeCopy(TheWorld().scenario_name(),
                     TheStartupOptions().new_game.c_str());
      startup_game_started = true;
    } else if (TheDebugState().map_editor_active()) {
      Set_Scenario_Name(TheWorld().scenario_name(), TheWorld().scenario(),
                        TheWorld().scen_player(), TheWorld().scen_dir(),
                        SCEN_VAR_A);
    } else {
      Set_Scenario_Name(TheWorld().scenario_name(), TheWorld().scenario(),
                        TheWorld().scen_player(), TheWorld().scen_dir());
    }

    /*
    ** Start_Scenario() changes the palette; so, fade out & clear the screen
    ** before calling it.
    */
    Hide_Mouse();

    if (selection != kSelStartNewGame) {
      Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium,
                      Call_Back);
      TheScreen().hidden_page().view().Clear();
      TheScreen().visible_page().view().Clear();
    }
    Show_Mouse();

    TheSpecial().IsFromInstall = 0;
    DLOG(INFO) << "C&C95 - Starting scenario.";
    if (!Start_Scenario(TheWorld().scenario_name())) {
      return false;
    }
    DLOG(INFO) << "C&C95 - Scenario started OK.";
    if (TheStartupOptions().globals_test) {
      TheWorld().score().Score = 101;
      TheWorld().score().NKilled = 2;
      TheWorld().score().GKilled = 3;
      TheWorld().score().CKilled = 4;
      TheWorld().score().NBKilled = 5;
      TheWorld().score().GBKilled = 6;
      TheWorld().score().CBKilled = 7;
      TheWorld().score().NHarvested = 8;
      TheWorld().score().GHarvested = 9;
      TheWorld().score().CHarvested = 10;
      TheWorld().score().ElapsedTime = static_cast<int64_t>(uint64_t{1} << 40);
      TheWorld().base().House = HOUSE_GOOD;
      TheWorld().base().Nodes.Clear();
      BaseNodeClass node;
      node.Type = STRUCT_POWER;
      node.Coord = Cell_Coord(1000);
      TheWorld().base().Nodes.Add(node);
      node.Type = STRUCT_REFINERY;
      node.Coord = Cell_Coord(1200);
      TheWorld().base().Nodes.Add(node);
      TheWorld().current_object().Clear();
      if (TheObjectHeaps().unit().Count() < 2) {
        return false;
      }
      TheWorld().current_object().Add(TheObjectHeaps().unit().Ptr(1));
      TheWorld().current_object().Add(TheObjectHeaps().unit().Ptr(0));
      base::At(TheWorld().waypoint(), 20) = 1234;
      TheWorld().carry_over_money() = 13579;
      TheWorld().carry_over_percent() = 42;
      base::At(TheWorld().views(), 3) = 2345;
      TheWorld().end_count_down() = 700;
    }
    if (TheStartupOptions().map_test) {
      for (CELL cell = 0; cell < 16; ++cell) {
        if (TheMap().In_Radar(cell)) {
          LOG(ERROR) << "-MAPTEST: fixture cells must be outside the playable map";
          return false;
        }
        TheMap().at(cell).Reset();
      }
      // Isolate fields formerly omitted by the sparse-cell predicate.
      TheMap().at(0).IsPlot = true;
      TheMap().at(1).IsCursorHere = true;
      TheMap().at(2).IsWaypoint = true;
      TheMap().at(3).IsRadarCursor = true;
      TheMap().at(4).IsFlagged = true;
      TheMap().at(5).TIcon = 7;
      TheMap().at(6).OverlayData = 3;
      TheMap().at(7).SmudgeData = 2;
      TheMap().at(8).Owner = HOUSE_GOOD;
      TheMap().at(9).InfType = HOUSE_BAD;
      TheMap().at(10).Overlay = OVERLAY_BRICK_WALL;
      TheMap().at(10).Recalc_Attributes();
      TheMap().at(10).Overlay = OVERLAY_NONE;
      auto* trigger = new TriggerClass;
      if (!trigger || TheObjectHeaps().unit().Count() == 0) {
        return false;
      }
      trigger->AttachCount = 2;
      TheMap().at(11).IsTrigger = TheMap().at(12).IsTrigger = true;
      TheWorld().cell_triggers().at(11) = TheWorld().cell_triggers().at(12) =
          trigger;
      TheMap().at(13).OccupierPtr = TheObjectHeaps().unit().Ptr(0);
      base::At(TheMap().at(14).Overlappers, 2) = TheObjectHeaps().unit().Ptr(0);
      TheMap().at(15).Flag.Composite = 2;
      TheMap().TotalValue = static_cast<int64_t>(uint64_t{1} << 35);
      auto* pending =
          new BuildingClass(STRUCT_POWER, ThePlayer()->Class->House);
      if (!pending) {
        return false;
      }
      TheMap().PendingObjectPtr = pending;
      TheMap().PendingObject = &pending->Class_Of();
      TheMap().PendingHouse = ThePlayer()->Class->House;
      TheMap().Set_Cursor_Shape(TheMap().PendingObject->Occupy_List(true));
    }
    if (TheStartupOptions().mobile_test) {
      const HousesType house = ThePlayer()->Class->House;
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
      if (TheObjectHeaps().unit().Count() != 0) {
        const CELL start = Coord_Cell(TheObjectHeaps().unit().Ptr(0)->Coord);
        for (int offset = 1; offset <= 16; ++offset) {
          const CELL cell = static_cast<CELL>(start + offset);
          if (cell < MAP_CELL_TOTAL && TheMap().In_Radar(cell) &&
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
    if (TheStartupOptions().building_test) {
      const HousesType house = ThePlayer()->Class->House;
      // Limbo fixtures preserve non-default fields without building AI replacing
      // them before the save. Campaign buildings exercise normal AI separately.
      auto* building = new BuildingClass(STRUCT_WEAP, house);
      auto* peer = new BuildingClass(STRUCT_REPAIR, house);
      auto* passenger = new InfantryClass(INFANTRY_E1, house);
      auto* factory = new FactoryClass;
      if (!building || !peer || !passenger || !factory ||
          !factory->Set(UnitTypeClass::As_Reference(UNIT_JEEP), *ThePlayer()) ||
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
    if (TheStartupOptions().world_test) {
      if (TheObjectHeaps().unit().Count() == 0) {
        LOG(ERROR) << "-WORLDTEST: scenario needs a unit";
        return false;
      }
      UnitClass* owner = TheObjectHeaps().unit().Ptr(0);
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
    if (TheStartupOptions().team_test) {
      UnitClass* member = nullptr;
      for (int i = 0; i < TheObjectHeaps().unit().Count(); ++i) {
        if (TheObjectHeaps().unit().Ptr(i)->House == ThePlayer() &&
            !TheObjectHeaps().unit().Ptr(i)->IsInLimbo) {
          member = TheObjectHeaps().unit().Ptr(i);
          break;
        }
      }
      auto* type = new TeamTypeClass;
      if (member == nullptr || type == nullptr) {
        LOG(ERROR) << "-TEAMTEST: no member or team type slot";
        return false;
      }
      type->Set_Name("saveteam");
      type->House = ThePlayer()->Class->House;
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
    if (TheStartupOptions().factory_test) {
      auto* factory = new FactoryClass;
      if (factory == nullptr ||
          !factory->Set(UnitTypeClass::As_Reference(UNIT_JEEP), *ThePlayer()) ||
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
  const int factor = TheScreen().visible_view().width() == 320 ? 1 : 2;
  TheSession().messages().Init(TheMap().TacPixelX, TheMap().TacPixelY, 6,
                               MAX_MESSAGE_LENGTH, (6 * factor) + 1);

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
  TheMouse()->Erase_Mouse(&TheScreen().hidden_view(), true);

  Fade_Palette_To(ThePalettes().black_palette(), kFadePaletteMedium, Call_Back);
  TheScreen().hidden_page().view().Clear();
  TheScreen().visible_page().view().Clear();
  TheMap().Flag_To_Redraw();
  Call_Back();
  TheMap().Render();
  // Show_Mouse();

  /*
  ** Special hack initialization of 'MPlayerMaxAhead' to accommodate the
  ** compression protocol technology.
  */
#ifdef FORCE_WINSOCK
  if (TheSession().comm_protocol() == COMM_PROTOCOL_MULTI_E_COMP &&
      TheSession().type() != GAME_NORMAL) {
    if (!TheSpecial().IsFromWChat) {
      TheSession().max_ahead() = TheSession().frame_send_rate() * 3;  // 2;
    } else {
      TheSession().max_ahead() = TheNetwork().chat_max_ahead();
      TheSession().frame_send_rate() = TheNetwork().chat_send_rate();
    }
  }
#endif  // FORCE_WINSOCK

  if (TheDebugState().map_editor_active()) {
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
    if (!TheDebugState().developer_mode()) {
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

// Split out of Parse_Command_Line() so the std::optional below does not make
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

// The two helpers below exist for the same reason ParseDestNet() does: an
// optional touched anywhere in Parse_Command_Line() makes clang-tidy run its
// optional-access dataflow over that whole function, which costs ~8 seconds of
// analysis on this file.
static void ApplyDestNet(const std::string_view address,
                         StartupOptions& options) {
  // A malformed address leaves any earlier one alone.
  const std::optional<IPXAddressClass> bridge_net = ParseDestNet(address);
  if (bridge_net.has_value()) {
    options.bridge_net = bridge_net;
  }
}

static void ApplySocket(const std::string_view offset_text,
                        StartupOptions& options) {
  // An out-of-range offset leaves any earlier one alone.
  const std::optional<uint16_t> socket = ParseSocketArgument(offset_text);
  if (socket.has_value()) {
    options.socket = socket;
  }
}

std::optional<StartupOptions> Parse_Command_Line(
    const std::span<const std::string_view> arguments) {
  StartupOptions options;

  for (const std::string_view argument : arguments) {
    const std::string string = absl::AsciiStrToUpper(argument);

    if (string.starts_with("-SEED")) {
      options.custom_seed =
          tech::ParseIntegerOr<uint16_t>(string.substr(5), options.custom_seed);
      continue;
    }
    if (string.starts_with("-NEWGAME")) {
      options.new_game = string.substr(8);
      if (options.new_game.size() < 3) {
        return std::nullopt;
      }
      continue;
    }
    if (string.starts_with("-LOADGAME")) {
      options.load_game = tech::ParseIntegerOr<int>(string.substr(9), -1);
      continue;
    }
    if (string.starts_with("-QUITFRAME")) {
      options.quit_at_frame = tech::ParseIntegerOr<int>(string.substr(10), -1);
      continue;
    }
    if (string.starts_with("-SAVESLOT")) {
      options.save_slot = tech::ParseIntegerOr<int>(string.substr(9), -1);
      continue;
    }
    if (std::string_view(string) == "-GLOBALTEST") {
      options.globals_test = true;
      continue;
    }
    if (std::string_view(string) == "-MAPTEST") {
      options.map_test = true;
      continue;
    }
    if (std::string_view(string) == "-MOBILETEST") {
      options.mobile_test = true;
      continue;
    }
    if (std::string_view(string) == "-BUILDINGTEST") {
      options.building_test = true;
      continue;
    }
    if (std::string_view(string) == "-WORLDTEST") {
      options.world_test = true;
      continue;
    }
    if (std::string_view(string) == "-TEAMTEST") {
      options.team_test = true;
      continue;
    }
    if (std::string_view(string) == "-FACTORYTEST") {
      options.factory_test = true;
      continue;
    }
    if (std::string_view(string) == "-NOMOVIES") {
      options.no_movies = true;
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
      return std::nullopt;
    }

    bool processed = true;
    switch (HashKeyPhrase(string)) {
      /*
      **	Signal that easy mode is active.
      */
      case PARM_EASY:
        options.easy = true;
        options.hard = false;
        break;

      /*
      **	Signal that hard mode is active.
      */
      case PARM_HARD:
        options.easy = false;
        options.hard = true;
        break;

      case PARM_PLAYTEST:
        if constexpr (config::kVirginCheatKeysEnabled) {
          options.playtest = true;
        }
        break;

      case PARM_CHEATERIK:
      case PARM_CHEATADAM:
      case PARM_CHEATMIKE:
      case PARM_CHEATDAVID:
      case PARM_CHEATPHIL:
      case PARM_CHEATBILL:
      case PARM_CHEAT_STEVET:
        options.playtest = true;
        options.developer_mode = true;
        break;

      case PARM_EDITORBILL:
      case PARM_EDITORERIK:
        options.map_editor_active = true;
        options.unshroud = true;
        options.developer_mode = true;
        break;

      case PARM_SPECIAL:
        options.jurassic = true;
        break;

      /*
      ** Special flag - is C&C being run from the install program?
      */
      case PARM_INSTALL:
#ifndef DEMO
        options.from_install = true;
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
        options.check_map = true;
        continue;
      }
    }

    /*
    **	Older version override.
    */
    if (absl::EqualsIgnoreCase(string, "-O") ||
        absl::EqualsIgnoreCase(string, "-0")) {
      options.compatibility_v107 = true;
      continue;
    }

    /*
    **	File search path override.
    */
    if (string.contains("-CD")) {
      // The original argument keeps the case of the path for Unix.
      options.search_paths.emplace_back(argument.substr(3));
      continue;
    }
#ifdef JAPANESE
    /*
    ** Enable english-compatible keyboard
    */
    if (absl::EqualsIgnoreCase(string, "-ENGLISH")) {
      options.force_english = true;
      continue;
    }
#endif

    /*
    **	Specify destination connection for network play
    */
    if (string.contains("-DESTNET")) {
      ApplyDestNet(std::string_view(string).substr(8), options);
      continue;
    }

    /*
    **	Specify socket ID, as an offset from 0x4000.
    */
    if (string.contains("-SOCKET")) {
      ApplySocket(
          std::string_view(string).substr(std::string_view("-SOCKET").size()),
          options);
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
    ** Disable mouse grabbing for debugging
    */
    if (string.contains("-NOMOUSEGRAB")) {
      options.no_mouse_grab = true;
      continue;
    }

    /*
    ** Set screen to 640x480 instead of 640x400
    */
    if (string.contains("-480")) {
      options.tall_screen = true;
      continue;
    }

    /*
    ** Check for spawn from WChat
    */
    if (string.contains("-WCHAT")) {
      options.spawned_from_wchat = true;
    }

    /*
    ** Allow use of MMX instructions
    */
    if (string.contains("-MMX")) {
      options.mmx_available = true;
      continue;
    }

    if constexpr (config::kCheatKeysEnabled) {
      /*
      **	Allow solo net play
      */
      if (absl::EqualsIgnoreCase(string, "-HANSOLO")) {
        options.solo_net_play = true;
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
            TheSpecial().IsJuvenile = true;
            break;
#endif

          /*
          **	Inert weapons -- no units take damage.
          */
          case 'I':
            if constexpr (config::kCheatKeysEnabled) {
              options.inert_weapons = true;
            }
            break;

          /*
          **	Hussled recharge timer.
          */
          case 'H':
            if constexpr (config::kCheatKeysEnabled) {
              options.speed_build = true;
            }
            break;

          /*
          **	Turn on super-record mode, which thrashes your disk terribly,
          ** but is really really cool.  Well, sometimes it is, anyway.
          ** At least, it can be.  Once in a while.
          ** This flag tells the recording system to flush the file to disk
          ** every frame, so the recording survives a crash.
          */
          case 'S':
            if constexpr (config::kCheatKeysEnabled) {
              options.super_record = true;
            }
            break;

          /*
          **	"Record" a multi-player game
          */
          case 'X':
            if constexpr (config::kCheatKeysEnabled) {
              options.record = true;
            }
            break;

          /*
          **	"Play Back" a multi-player game
          */
          case 'Y':
            if constexpr (config::kCheatKeysEnabled) {
              options.playback = true;
            }
            break;

#ifdef ONHOLD
          /*
          **	Bonus scenario enable.
          */
          case 'Z':
            TheSpecial().IsJurassic = true;
            break;
#endif

          /*
          **	Quiet mode override control.
          */
          case 'Q':
            options.quiet = true;
            break;

          /*
          **	Target selection by human opponent (network/modem play) will
          **	be visible to the player?
          */
          case 'V':
            if constexpr (config::kCheatKeysEnabled) {
              options.visible_target = true;
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
            return std::nullopt;
        }
      }

      continue;
    }
  }
  return options;
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
    TheGameState().thingies_enabled() = true;
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
      file.ReadObject(TheGameState().version_text());
      TheGameState().version_text()[sizeof(TheGameState().version_text()) - 1] =
          '\0';
      while (TheGameState()
                 .version_text()[sizeof(TheGameState().version_text()) - 1] ==
             '\r') {
        TheGameState()
            .version_text()[sizeof(TheGameState().version_text()) - 1] = '\0';
      }
    } else {
      TheGameState().version_text()[0] = '\0';
    }

    initialized = true;
  }
  return (version);
#endif

#ifdef FRENCH
  sprintf(TheGameState().version_text(), ".02");  // Win95 french version number
#endif                          // FRENCH

#ifdef GERMAN
  sprintf(TheGameState().version_text(), ".01");  // Win95 german version number
#endif                          // GERMAN

#ifdef JAPANESE
  sprintf(TheGameState().version_text(), ".01");  // Win95 german version number
#endif                          // GERMAN

#if !(defined(FRENCH) || defined(GERMAN) || defined(JAPANESE))
  absl::SNPrintF(TheGameState().version_text(),
                 sizeof(TheGameState().version_text()),
                 ".07");        // Win95 USA version number
#endif                          // FRENCH | GERMAN

  char version[16];
  base::FillBytes(base::ObjectBytes(version), 0, sizeof(version));
  if (const auto file = OpenDiskFile("VERSION.TXT")) {
    file->ReadObject(version);
  }
  port::SafeAppend(
      TheGameState().version_text(),
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
 *      file   the open recording                                          *
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
void Save_Recording_Values(ByteStream& file) {
  file.WriteObject(TheSession().type());
  file.WriteObject(TheNetwork().modem_game_type());
  file.WriteObject(TheWorld().build_level());
  file.WriteObject(TheSession().player_name());
  file.WriteObject(TheSession().preferred_color());
  file.WriteObject(TheSession().color_index());
  file.WriteObject(TheSession().house());
  file.WriteObject(TheSession().local_id());
  file.WriteObject(TheSession().player_count());
  file.WriteObject(TheSession().bases());
  file.WriteObject(TheSession().credits());
  file.WriteObject(TheSession().tiberium());
  file.WriteObject(TheSession().crates());
  file.WriteObject(TheSession().ghosts());
  file.WriteObject(TheSession().unit_count());
  file.WriteObject(TheSession().player_ids());
  file.WriteObject(TheSession().player_houses());
  file.WriteObject(TheWorld().seed());
  file.WriteObject(TheWorld().scenario());
  file.WriteObject(TheWorld().scen_player());
  file.WriteObject(TheWorld().scen_dir());
  file.WriteObject(TheWorld().whom());
  file.WriteObject(TheSpecial());
  file.WriteObject(TheOptions());
  file.WriteObject(TheSession().frame_send_rate());
  file.WriteObject(TheSession().comm_protocol());

  if (TheSession().super_record() && !file.Flush()) {
    DLOG(WARNING) << "Save_Recording_Values: Flush failed, disk may be full";
  }
}

/***************************************************************************
 * Load_Recording_Values -- Loads recording values from recording file     *
 *                                                                         *
 * INPUT:                                                                  *
 *      file   the open recording                                          *
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
void Load_Recording_Values(ByteStream& file) {
  Read_MultiPlayer_Settings();

  file.ReadObject(TheSession().type());
  file.ReadObject(TheNetwork().modem_game_type());
  file.ReadObject(TheWorld().build_level());
  file.ReadObject(TheSession().player_name());
  file.ReadObject(TheSession().preferred_color());
  file.ReadObject(TheSession().color_index());
  file.ReadObject(TheSession().house());
  file.ReadObject(TheSession().local_id());
  file.ReadObject(TheSession().player_count());
  file.ReadObject(TheSession().bases());
  file.ReadObject(TheSession().credits());
  file.ReadObject(TheSession().tiberium());
  file.ReadObject(TheSession().crates());
  file.ReadObject(TheSession().ghosts());
  file.ReadObject(TheSession().unit_count());
  file.ReadObject(TheSession().player_ids());
  file.ReadObject(TheSession().player_houses());
  file.ReadObject(TheWorld().seed());
  file.ReadObject(TheWorld().scenario());
  file.ReadObject(TheWorld().scen_player());
  file.ReadObject(TheWorld().scen_dir());
  file.ReadObject(TheWorld().whom());
  file.ReadObject(TheSpecial());
  file.ReadObject(TheOptions());
  file.ReadObject(TheSession().frame_send_rate());
  file.ReadObject(TheSession().comm_protocol());
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
  Load_Title_Screen("HTITLE.PCX", &TheScreen().hidden_view(),
                    ThePalettes().title_palette());

  if (visible) {
    TheScreen().hidden_view().BlitTo(TheScreen().visible_view());
  }
}
