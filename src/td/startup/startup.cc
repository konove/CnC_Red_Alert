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

/* $Header:   F:\projects\c&c\vcs\code\startup.cpv   2.17   16 Oct 1995 16:48:12
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : STARTUP.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : October 3, 1994 *
 *                                                                                             *
 *                  Last Update : August 27, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Delete_Swap_Files -- Deletes previously existing swap files. *
 *   ShutDownEngine -- Cleans up library systems before game exit. * main --
 *Initial startup routine (preps library systems). *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/startup/startup.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <span>    // IWYU pragma: keep (used only with an entry point)
#include <string>  // IWYU pragma: keep (used only with an entry point)
#include <string_view>

#include "absl/base/no_destructor.h"
#include "base/buffer.h"  // IWYU pragma: keep (used only with an entry point)
#include "engine/audio/audio_mixer.h"
#include "engine/file/ini_file.h"
#include "engine/file/profile_buffer.h"
#include "engine/net/ipxaddr.h"
#include "engine/net/ipxmgr.h"
#include "engine/stream/byte_stream.h"
#include "game/sim/direction.h"
#include "game/sim/enums.h"  // IWYU pragma: keep (used only with an entry point)
#include "td/assets/palettes.h"  // IWYU pragma: keep (used only with an entry point)
#include "td/assets/screen.h"
#include "td/core/defines.h"
#include "td/data/ini_io.h"
#include "td/net/network.h"
#include "td/net/nullmgr.h"
#include "td/net/session.h"
#include "td/startup/game.h"
#include "td/startup/game_state.h"
#include "td/startup/init.h"
#include "td/startup/startup_options.h"
#include "td/startup/winstub.h"

// The tests that link this file define TD_NO_ENTRY_POINT; these headers
// serve only main().
#ifndef TD_NO_ENTRY_POINT
#include <filesystem>
#include <optional>
#include <vector>

#include "absl/base/log_severity.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/strings/match.h"
#include "base/numeric.h"
#include "engine/file/disk_file.h"
#include "engine/file/file_access.h"
#include "engine/file/search_paths.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/text_window.h"
#include "engine/gfx/wwstd.h"
#include "engine/platform/file_system.h"
#include "engine/window/hardware_cursor.h"
#include "td/ai/special.h"
#include "td/debug/debug_state.h"
#include "td/map/world.h"
#include "td/screens/goptions.h"
#include "td/startup/conquer.h"
#endif  // TD_NO_ENTRY_POINT

#ifdef _WIN32
#include <direct.h>  //chdir
#include <windows.h>

#include <array>

#include "absl/strings/str_split.h"
#include "td/startup/ccdde.h"
#endif

namespace td {

void Delete_Swap_Files();

[[maybe_unused]] static void Read_Setup_Options(ByteStream& config_file,
                                                const StartupOptions& options);

/***********************************************************************************************
 * main -- Initial startup routine (preps library systems). *
 *                                                                                             *
 *    This is the routine that is first called when the program starts up. It
 *basically        * handles the command line parsing and setting up library
 *systems.                         *
 *                                                                                             *
 * INPUT:   argc  -- Number of command line arguments. *
 *                                                                                             *
 *          argv  -- Pointer to array of comman line argument strings. *
 *                                                                                             *
 * OUTPUT:  Returns with execution failure code (if any). *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/20/1995 JLB : Created. *
 *=============================================================================================*/

#ifdef _WIN32
HINSTANCE ProgramInstance;
#endif
extern bool CC95AlreadyRunning;
void Move_Point(int16_t& x, int16_t& y, game::DirType dir, uint16_t distance);

// The one Game; main() creates it and ShutDown() destroys it. nullptr before
// main() and after ShutDown(). The pointer itself is never destroyed: a Game
// still alive at a bare exit() is left to the OS, because tearing it down from
// a static destructor reaches SDL after its video driver has unloaded.
// A null unique_ptr cannot throw.
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
static absl::NoDestructor<std::unique_ptr<Game>> game;

#ifndef TD_NO_ENTRY_POINT
// Hands what the command line asked for to whatever owns it. The screen
// mode, the IPX socket, the bridge network and the 1.07 compatibility flag
// wait for Read_Setup_Options(), because the config file asks for them too.
// Fills in the window rows engine_gfx holds the storage for. The first two are
// the screen and the error window, and the system needs them where they
// are.
static void InitWindowList() {
  static constexpr int kRows[kWindowCount][8] = {
      // xbyte, ypixel, bytewid, pixelht, fg, bg, cursor x, cursor y
      {0, 0, 40, 200, kWhite, kBlack, 0, 0},         // Screen.
      {1, 75, 38, 100, kWhite, kBlack, 0, 0},        // Error message.
      {0, 0, 40, 200, kWhite, kLtGrey, 0, 0},        // Tactical map.
      {12, 199 - 42, 16, 42, kLtGrey, kGrey, 0, 0},  // Initial menu.
      {0, 0, 0, 0, 0, 0, 0, 0},                      // Sidebar clipping.
      {5, 30, 30, 140, 0, 0, 0, 0},                  // Scenario editor.
      {0, 0, 0, 0, 0, 0, 0, 0},                      // Custom.
  };
  base::CopyBytes(base::ObjectBytes(WindowList), base::ObjectBytes(kRows),
                  sizeof(WindowList));
}

static void ApplyStartupOptions(const StartupOptions& options) {
  for (const std::string& path : options.search_paths) {
    SearchPaths::Add(path);
  }

  DebugState& debug_state = TheDebugState();
  debug_state.set_developer_mode(options.developer_mode);
  debug_state.set_playtest(options.playtest);
  debug_state.set_map_editor_active(options.map_editor_active);
  debug_state.set_unshroud(options.unshroud);
  debug_state.set_quiet(options.quiet);
  debug_state.set_check_map(options.check_map);

  if (options.easy) {
    TheSpecial().IsHealthBar = true;
    TheSpecial().IsEasy = true;
    TheSpecial().IsDifficult = false;
  }
  if (options.hard) {
    TheSpecial().IsHealthBar = false;
    TheSpecial().IsEasy = false;
    TheSpecial().IsDifficult = true;
  }
  if (options.jurassic) {
    TheSpecial().IsJurassic = true;
    TheGameState().thingies_enabled() = true;
  }
  TheSpecial().IsFromInstall = options.from_install;
  TheSpecial().IsInert = options.inert_weapons;
  TheSpecial().IsSpeedBuild = options.speed_build;
  TheSpecial().IsVisibleTarget = options.visible_target;

  TheSession().record_game() = options.record;
  TheSession().playback_game() = options.playback;
  TheSession().super_record() = options.super_record ? 1 : 0;

  TheNetwork().stealth() = options.net_stealth;
  TheNetwork().protect() = !options.outside_messages;
  TheSession().allow_attract() = options.attract;
  TheSession().solo() = options.solo_net_play;

  engine::window::mouse_grab_disabled = options.no_mouse_grab;
  TheGameState().spawned_from_chat() = options.spawned_from_wchat;
#ifdef JAPANESE
  ForceEnglish = options.force_english;
#endif
}

#ifdef _WIN32
static int Run(HINSTANCE instance, char* command_line, int command_show)
#else   // _WIN32
static int Run(int argc, char* argv[])
#endif  // _WIN32
{
  absl::InitializeLog();
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);

  *game = std::make_unique<Game>();

  // Heap_Dump_Check( "first thing in main" );
  //	malloc(1);

  CCDebugString("C&C95 - Starting up.\n");

  // CD_Test();

  /*
  ** These values return 0x47 if code is working correctly
  */
  //	int temp = Desired_Facing256 (1070, 5419, 1408, 5504);

#ifdef _WIN32
  ProgramInstance = instance;

  // WinMain gets the command line as one string without the program name,
  // possibly ending in a carriage return; arguments are separated by spaces.
  std::array<char, 280> path_to_exe{};
  GetModuleFileName(instance, path_to_exe.data(), path_to_exe.size());
  const std::filesystem::path program_path = path_to_exe.data();

  std::string_view line = command_line;
  line = line.substr(0, line.find('\r'));
  const std::vector<std::string_view> arguments =
      absl::StrSplit(line, ' ', absl::SkipEmpty());
#else
  // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
  const std::span raw_arguments(argv, base::ToSize(argc));
  const std::filesystem::path program_path = raw_arguments.front();
  const std::vector<std::string_view> arguments(raw_arguments.begin() + 1,
                                                raw_arguments.end());
#endif

  // Change to executable's directory (if path is present)
  const auto dir_path = program_path.parent_path();

  if (!dir_path.empty()) {
    std::filesystem::current_path(dir_path);
  }

#ifdef JAPANESE
  ForceEnglish = false;
#endif
  // The parser writes no game state, so these defaults come first and
  // ApplyStartupOptions() lays the command line over them.
#ifdef DEMO
  TheWorld().scenario() = 3;
#else
  TheWorld().scenario() = 1;
#endif
  TheWorld().scen_player() = SCEN_PLAYER_GDI;
  TheWorld().scen_dir() = game::SCEN_DIR_EAST;
  TheWorld().whom() = HOUSE_GOOD;
  TheSpecial().Init();

  const std::optional<StartupOptions> options = Parse_Command_Line(arguments);
  if (options.has_value()) {
    InitWindowList();
    (*game)->set_startup_options(*options);
    ApplyStartupOptions(*options);

    /*
    ** If there is not enough disk space free, dont allow the product to run.
    */

    if (FreeDiskSpace() < INIT_FREE_DISK_SPACE) {
      // pretty unlikely
      ShutDown();
      return EXIT_FAILURE;
    }

    // A kReadWrite open fails outright on a read-only CONQUER.INI (or a
    // read-only directory when there is none yet), which used to start fine
    // under stdio. Probe for the file and create it with a throwaway kWrite
    // open if missing, then reopen it kRead: that succeeds whether or not
    // the file is writable. The stream only needs to stay open for
    // Read_Setup_Options, not for the rest of the play session, so it is
    // scoped to this one call rather than held open by the outer if below.
    bool config_available = false;
    if (FindExistingFile("CONQUER.INI").has_value() ||
        OpenDiskFile("CONQUER.INI", FileAccess::kWrite)) {
      if (const auto cfile = OpenDiskFile("CONQUER.INI")) {
        Read_Setup_Options(*cfile, *options);
        config_available = true;
      }
    }

    if (config_available) {
      CCDebugString("C&C95 - Creating main window.\n");

      Create_Main_Window(nullptr, 0, Screen::kWidth, TheScreen().mode_height());
      CCDebugString("C&C95 - Initialising audio.\n");

      TheGameState().sound_on() =
          engine::audio::TheAudio().Open(11025 * 2, /*stereo=*/false);

      ThePalettes().title_palette() = engine::gfx::Palette();

      CCDebugString("C&C95 - Setting video mode.\n");
      if (!TheScreen().Init()) {
        CCDebugString("C&C95 - Failed to set video mode.\n");
        ShutDown();
        return EXIT_FAILURE;
      }

      CCDebugString("C&C95 - Adjusting variables for resolution.\n");
      TheOptions().Adjust_Variables_For_Resolution();

      CCDebugString("C&C95 - Setting palette.\n");
      /////////Set_Palette(Palette);

      WindowList[0][kWindowWidth] = TheScreen().visible_view().width() / 8;
      WindowList[0][kWindowHeight] = TheScreen().visible_view().height();

      CCDebugString("C&C95 - Grabbing the mouse.\n");
      engine::window::HardwareCursor::GrabMouse();

      /*
      ** See if we should run the intro
      */
      CCDebugString("C&C95 - Reading CONQUER.INI.\n");
      engine::file::IniFile conquer_ini;
      if (const auto config_read = OpenDiskFile("CONQUER.INI")) {
        LoadIniText(engine::file::ReadProfileText(*config_read), conquer_ini);
      }

      /*
      **	Check for forced intro movie run disabling. If the conquer
      **	configuration file says "no", then don't run the intro.
      */
      char tempbuff[5];
      conquer_ini.GetString(
          "Intro", "PlayIntro", "Yes",
          std::span(tempbuff).first(static_cast<std::size_t>(4)));
      TheSpecial().IsFromInstall = !absl::EqualsIgnoreCase(tempbuff, "No") &&
                                   !TheGameState().spawned_from_chat();
      ThePalettes().set_slow_palette(
          conquer_ini.GetInt("Options", "SlowPalette", 1) != 0);

#ifdef DEMO
      /*
      **	Check for override directory path for CD searches.
      */
      std::array<char, 128> cd_path{};
      conquer_ini.GetString("CD", "Path", ".", cd_path);
      TheGameState().override_path() = cd_path.data();
#endif

      /*
      ** Regardless of whether we should run it or not, here we're
      ** gonna change it to say "no" in the future.
      */
      conquer_ini.PutString("Intro", "PlayIntro", "No");
      SaveIniFile("CONQUER.INI", conquer_ini);

#ifdef _WIN32
      CCDebugString(
          "C&C95 - Checking availability of C&CSPAWN.INI packet from WChat.\n");
      if (DDEServer.Get_MPlayer_Game_Info()) {
        CCDebugString("C&C95 - C&CSPAWN.INI packet available.\n");
        Check_From_WChat(NULL);
      } else {
        CCDebugString("C&C95 - C&CSPAWN.INI packet not arrived yet.\n");
        // Check_From_WChat("C&CSPAWN.INI");
        // if (Special.IsFromWChat){
        //	DDEServer.Disable();
        // }
      }
#endif

      /*
      **	If the intro is being run for the first time, then don't
      **	allow breaking out of it with the <ESC> key.
      */
      if (TheSpecial().IsFromInstall) {
        TheGameState().breakout_allowed() = false;
      }

      CCDebugString("C&C95 - Entering main game.\n");
      Main_Game();

      TheScreen().visible_page().view().Clear();
      TheScreen().hidden_page().view().Clear();

      CCDebugString("C&C95 - About to exit.\n");
      ShutDown();
      return EXIT_SUCCESS;
    }
#ifdef GERMAN
    puts("Bitte erst das SETUP-Programm starten.\n");
#else
#ifdef FRENCH
    puts("Lancez d'abord le programme de configuration SETUP.\n");
#else
    puts("Run SETUP program first.");
    puts("\n");
#endif
#endif

    //		Remove_Keyboard_Interrupt();
    ShutDown();
    return EXIT_FAILURE;
  }

  /*
  **	Restore the current drive and directory.
  */

  ShutDown();
  return EXIT_SUCCESS;
}

#endif  // TD_NO_ENTRY_POINT

/***********************************************************************************************
 * ShutDownEngine -- Cleans up library systems in prep for game exit. *
 *                                                                                             *
 *    This routine should be called before the game terminates. It handles
 *cleaning up         * library systems so that a graceful return to the host
 *operating system is achieved.      *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/20/1995 JLB : Created. *
 *=============================================================================================*/
void ShutDownEngine() {
#ifndef DEMO
  if (TheSession().type() == GAME_MODEM ||
      TheSession().type() == GAME_NULL_MODEM) {
    NullModemClass::Change_IRQ_Priority(0);
  }
#endif
  CCDebugString("C&C95 - About to call CloseAudio.\n");
  engine::audio::TheAudio().Close();
  CCDebugString("C&C95 - Returned from CloseAudio.\n");
  CCDebugString("C&C95 - Releasing the mouse.\n");
  engine::window::HardwareCursor::ReleaseMouse();
}

void ShutDown() {
  // Everything ShutDownEngine() takes down belongs to the Game, so there is
  // nothing to do before one is built or after one is gone. A test that
  // reaches an error exit without a Game gets here too.
  if (*game == nullptr) {
    return;
  }

  ShutDownEngine();
  game->reset();
}

/***********************************************************************************************
 * Read_Setup_Options -- Read stuff in from the INI file that we need to know
 *sooner           *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 6/7/96 4:09PM ST : Created *
 *=============================================================================================*/
void Read_Setup_Options(ByteStream& config_file,
                        const StartupOptions& options) {
  engine::file::IniFile ini;
  LoadIniText(engine::file::ReadProfileText(config_file), ini);
  ApplySetupOptions(ini, options);
}

void ApplySetupOptions(const engine::file::IniFile& ini,
                       const StartupOptions& options) {
  // Resolution=yes and -480 both ask for a 480-line mode; Screen::Init()
  // letterboxes the 400-line game area inside it.
  TheScreen().set_mode_height(
      options.tall_screen || ini.GetInt("Options", "Resolution", 0) != 0
          ? 480
          : Screen::kHeight);
  TheGameState().compatibility_v107() =
      options.compatibility_v107 ||
      ini.GetInt("Options", "Compatibility", 0) != 0;

  /*
  ** See if an alternative socket number has been specified
  */
  if (options.socket.has_value()) {
    TheNetwork().ipx().Set_Socket(*options.socket);
  } else {
    const int socket = ini.GetInt("Options", "Socket", 0);
    if (socket > 0 && socket < 0x4000) {
      TheNetwork().ipx().Set_Socket(static_cast<uint16_t>(0x4000 + socket));
    }
  }

  /*
  ** See if a destination network has been specified. A missing or empty
  ** DestNet gives the empty text, which ParseDestNet() rejects.
  */
  std::optional<engine::net::IPXAddressClass> bridge_net = options.bridge_net;
  if (!bridge_net.has_value()) {
    bridge_net = ParseDestNet(ini.GetString("Options", "DestNet", ""));
  }
  if (bridge_net.has_value()) {
    TheNetwork().is_bridge() = 1;
    TheNetwork().bridge_net() = *bridge_net;
  }
}

}  // namespace td

#ifndef TD_NO_ENTRY_POINT
#ifdef _WIN32
int PASCAL WinMain(HINSTANCE instance, HINSTANCE, char* command_line,
                   int command_show) {
  return td::Run(instance, command_line, command_show);
}
#else   // _WIN32
int main(int argc, char* argv[]) { return td::Run(argc, argv); }
#endif  // _WIN32
#endif  // TD_NO_ENTRY_POINT
