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

// Red Alert's process entry point and exit paths: main() sets up the
// library systems, reads the options needed before the window exists, and
// hands over to RunGame(); Prog_End(), EmergencyExit() and the memory-error
// hooks tear it all down again.
//
// Originally STARTUP.CPP by Joe L. Bostic, October 1994.

#include "ra/startup.h"

#include <cstdlib>
#include <cstring>

#include "absl/strings/str_format.h"
#include "ra/externs.h"
#include "ra/game.h"
#include "ra/palette.h"
#include "ra/palettes.h"
#include "ra/screen.h"
#include "sdllib/gbuffer.h"
#include "sdllib/memflag.h"
#include "sdllib/misc.h"
#include "sdllib/timer.h"
#include "tech/audio_mixer.h"

// The test that links this file defines RA_NO_ENTRY_POINT; these headers
// serve only main() and the two helpers it calls.
#ifndef RA_NO_ENTRY_POINT
#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "absl/base/log_severity.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "base/array.h"
#include "base/numeric.h"
#include "port/bytes_of.h"
#include "port/win32/win32_registry.h"
#include "port/win32/win32_system.h"
#include "port/win32/win32_types.h"
#include "ra/config.h"
#include "ra/conquer.h"
#include "ra/debug_state.h"
#include "ra/defines.h"
#include "ra/globals.h"
#include "ra/goptions.h"
#include "ra/ini.h"
#include "ra/init.h"
#include "ra/installation.h"
#include "ra/ipxaddr.h"
#include "ra/ipxmgr.h"
#include "ra/jshell.h"
#include "ra/language.h"
#include "ra/movie.h"
#include "ra/network.h"
#include "ra/nullconn.h"
#include "ra/session.h"
#include "ra/special.h"
#include "ra/startup_options.h"
#include "ra/world.h"
#include "sdllib/drawbuff.h"
#include "sdllib/file.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"
#include "tech/disk_file.h"
#include "tech/search_paths.h"
#endif  // RA_NO_ENTRY_POINT

#ifdef _WIN32
#include "absl/strings/str_split.h"
#include "ra/ipx95.h"

#endif  // _WIN32

// Prints `message` and exits with status 1 without any cleanup. Installed as
// Memory_Error_Exit once the game systems are gone, or when they are being
// torn down anyway.
[[noreturn]] static void ExitWithError(char* message) {
  absl::PrintF("%s\n", message);
  exit(1);
}

// The one Game; main() creates it and ShutDown() destroys it. nullptr before
// main() and after ShutDown().
static Game* game = nullptr;

void ShutDown() {
  // Nothing is left for an allocation failure from here on to clean up.
  Memory_Error_Exit = ExitWithError;
  Prog_End();
  delete game;
  game = nullptr;
}

// The test that links this file defines RA_NO_ENTRY_POINT: it needs
// ShutDown() and Prog_End(), which the engine calls, but not main().
#ifndef RA_NO_ENTRY_POINT

// Hands what the command line asked for to whatever owns it. The screen
// mode, the IPX socket and the bridge network wait for ReadConfigOptions(),
// because the config file asks for them too.
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
  debug_state.set_print_events(options.print_events);
  debug_state.set_check_map(options.check_map);

  Special.IsFromInstall = options.from_install;
  Special.IsInert = options.inert_weapons;
  Special.IsSpeedBuild = options.speed_build;

  TheSession().NetStealth = options.net_stealth;
  TheSession().NetProtect = !options.outside_messages;
  TheSession().Attract = options.attract;
  TheSession().Record = options.record;
  TheSession().Play = options.play;

  if (options.disable_fades) {
    PaletteClass::DisableFades();
  }
  bNoMovies = options.no_movies;
  NoMouseGrab = options.no_mouse_grab;
}

// Reads the config-file options that have to be known before the window and
// the network exist: blit fills, the screen height, the IPX socket and a
// bridge network. The command line wins wherever it asked for the same thing.
static void ReadConfigOptions(const INIClass& ini,
                              const StartupOptions& options) {
  AllowHardwareBlitFills = ini.Get_Bool("Options", "HardwareFills", true);

  // Resolution=yes and -480 both ask for a 480-line mode; Screen::Init()
  // letterboxes the 400-line game area inside it.
  TheScreen().set_mode_height(
      options.tall_screen || ini.Get_Bool("Options", "Resolution", false)
          ? 480
          : Screen::kHeight);

  // Socket is an offset into the dynamic IPX socket range 0x4000-0x7FFF,
  // letting several games share a network without seeing each other.
  if (options.socket.has_value()) {
    TheNetwork().ipx().Set_Socket(*options.socket);
  } else {
    const int socket = ini.Get_Int("Options", "Socket", 0);
    if (socket > 0 && socket < 0x4000) {
      TheNetwork().ipx().Set_Socket(static_cast<uint16_t>(0x4000 + socket));
    }
  }

  // DestNet names a network on the far side of an IPX bridge.
  std::optional<IPXAddressClass> bridge_net = options.bridge_net;
  if (!bridge_net.has_value()) {
    std::array<char, 512> dest_net{};
    // Get_String() returns the length of the trimmed value; 0 if absent.
    const int length = ini.Get_String("Options", "DestNet", nullptr, dest_net,
                                      static_cast<int>(dest_net.size()));
    if (length > 0) {
      bridge_net =
          ParseDestNet(std::string_view(dest_net.data(), base::ToSize(length)));
    }
  }
  if (bridge_net.has_value()) {
    TheSession().IsBridge = 1;
    TheSession().BridgeNet = *bridge_net;
  }
}

// Parses the command line, checks the machine can run the game, opens the
// window, sound and video, then runs the game until the player quits.
// Returns the process exit status.
#ifdef _WIN32
int PASCAL WinMain(HINSTANCE instance, HINSTANCE, char* command_line,
                   int command_show)
#else   // _WIN32
int main(const int argc, char* argv[])
#endif  // _WIN32
{
  absl::InitializeLog();
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);

  game = new Game();

#ifdef _WIN32

  // Westwood's own network share: refuse to run a build straight off it.
  if (strstr(command_line, "f:\\projects\\c&c0") != NULL ||
      strstr(command_line, "F:\\PROJECTS\\C&C0") != NULL) {
    MessageBox(0, "Playing off of the network is not allowed.", "Red Alert",
               MB_OK | MB_ICONSTOP);
    return (EXIT_FAILURE);
  }

  // WinMain gets the command line as one string without the program name,
  // possibly ending in a carriage return; arguments are separated by spaces.
  std::array<char, 260> path_to_exe{};
  GetModuleFileName(instance, path_to_exe.data(), path_to_exe.size());
  const std::filesystem::path program_path = path_to_exe.data();

  std::string_view line = command_line;
  line = line.substr(0, line.find('\r'));
  const std::vector<std::string_view> arguments =
      absl::StrSplit(line, ' ', absl::SkipEmpty());

#else  // _WIN32

  // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
  const std::span raw_arguments(argv, static_cast<size_t>(argc));
  const std::filesystem::path program_path = raw_arguments.front();
  const std::vector<std::string_view> arguments(raw_arguments.begin() + 1,
                                                raw_arguments.end());

#endif  // _WIN32

  // Run from the executable's directory, so the relative paths to the config
  // file and the local MIX files resolve however the game was launched.
  const auto exe_dir = program_path.parent_path();

  if (!exe_dir.empty()) {
    std::filesystem::current_path(exe_dir);
  }

  // Westwood Online's own installer left these behind. None of it can
  // happen here, but it is what the WOL build did on startup.
  if constexpr (config::kWolapiEnabled) {
    // The version 3 patch shipped wolsetup.exe to install the "Shared
    // Internet Components"; the registry value says it has finished.
    WIN32_FIND_DATA find_data{};
    HANDLE setup_exe = FindFirstFile("wolsetup.exe", &find_data);
    const bool setup_exe_found = (setup_exe != INVALID_HANDLE_VALUE);
    FindClose(setup_exe);
    HKEY wol_key = nullptr;
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, Game_Registry_Key(), 0, KEY_READ,
                 &wol_key);
    DWORD install_complete = 0;
    DWORD value_size = sizeof(DWORD);
    // Once setup has finished, delete the setup exe, and drop the registry
    // value only once the exe is gone, so a failed delete is retried next
    // launch.
    if (RegQueryValueEx(wol_key, "WolapiInstallComplete", nullptr, nullptr,
                        port::BytesOf(install_complete),
                        &value_size) == ERROR_SUCCESS &&
        (!setup_exe_found || DeleteFile("wolsetup.exe"))) {
      RegDeleteValue(wol_key, "WolapiInstallComplete");
    }
    RegCloseKey(wol_key);

    // The 1.08 patch left a loose conquer.eng in the game directory, and the
    // patch to this version had trouble deleting it. It must not be there:
    // the Aftermath MIX files now carry the string overrides it used to, and
    // a loose file would shadow them.
    if (FindFirstFile("conquer.eng", &find_data) != INVALID_HANDLE_VALUE) {
      DeleteFile("conquer.eng");
    }
  }

  // The parser writes no game state, so these defaults come first and
  // ApplyStartupOptions() lays the command line over them.
  TheWorld().whom() = HOUSE_GOOD;
  Special.Init();

  const std::optional<StartupOptions> options = Parse_Command_Line(arguments);
  if (!options.has_value()) {
    // The usage text or the invalid-option message has been printed.
    ShutDown();
    return EXIT_SUCCESS;
  }
  game->set_startup_options(*options);
  ApplyStartupOptions(*options);

  InitTickTimer();
  DiskFile config_file(kConfigFileName);

  Keyboard = new KeyboardClass();

  // Refuse to start without 8 MB free for save games and the config file.
  if (Disk_Space_Available() < kInitFreeDiskSpace) {
    absl::PrintF("%s", kLanguageText.insufficient_disk);
    absl::PrintF("%s\n",
                 MustHaveDiskSpaceText(kInitFreeDiskSpace / (1024 * 1024)));
    ShutDown();
    return EXIT_FAILURE;
  }

  // The original installer wrote the config file. Without one, start from
  // an empty file: every option has a default.
  if (!config_file.IsAvailable()) {
    config_file.Create();
  }

  if (!config_file.IsAvailable()) {
    // The config file could neither be opened nor created. There is no
    // window yet to read a key from, so report and leave.
    absl::PrintF("%s\n", kLanguageText.setup_first);
    ShutDown();
    return EXIT_FAILURE;
  }

  INIClass ini;
  ini.Load(config_file);

  // Sets the mode height, so it has to come before the window is opened.
  ReadConfigOptions(ini, *options);

  Create_Main_Window(nullptr, 0, Screen::kWidth, TheScreen().mode_height());
  // 22050 Hz mono.
  SoundOn = Audio.Open(11025 * 2, /*stereo=*/false);

  if (!TheScreen().Init()) {
    ShutDown();
    return EXIT_FAILURE;
  }

  Options.Adjust_Variables_For_Resolution();

  Memory_Error = &Memory_Error_Handler;

  // The full-screen and editor windows cover the visible viewport, whose
  // size is only known now that the video mode is set.
  base::At(WindowList[0], kWindowWidth) =
      TheScreen().visible_view().Get_Width();
  base::At(WindowList[0], kWindowHeight) =
      TheScreen().visible_view().Get_Height();
  base::At(WindowList[static_cast<int>(WINDOW_EDITOR)], kWindowWidth) =
      TheScreen().visible_view().Get_Width();
  base::At(WindowList[static_cast<int>(WINDOW_EDITOR)], kWindowHeight) =
      TheScreen().visible_view().Get_Height();

  WWMouse = new WWMouseClass(&TheScreen().visible_view(), 48, 48);
  MouseInstalled = true;

  // SDL enumerates no CD drives, so there is no drive letter to record. -1
  // rather than 0 keeps "?:\" in the search path list: SearchPaths::Scan()
  // drops that placeholder when the CD drive is 0, and the bootstrap in
  // init.cc reports a missing CD and quits when the list comes back empty.
  SearchPaths::SetCdDrive(-1);

  // IsFromInstall means "first launch after installing": play the intro
  // movie. The installer used to write PlayIntro=yes; with no entry it
  // still defaults to yes, so a fresh install sees the movie once.
  if (!Special.IsFromInstall) {
    Special.IsFromInstall = ini.Get_Bool("Intro", "PlayIntro", true);
  }
  ThePalettes().set_slow_palette(ini.Get_Bool("Options", "SlowPalette", false));

  // Whatever happens next, the intro has now been shown once: write
  // PlayIntro=no so later launches go straight to the menu. Tiberian
  // Dawn forbids skipping this first-run intro with <ESC>; Red Alert
  // shipped allowing it.
  if (Special.IsFromInstall) {
    BreakoutAllowed = true;
    ini.Put_Bool("Intro", "PlayIntro", false);
    ini.Save(config_file);
  }

  // While the game runs an out-of-memory exit still has everything to
  // clean up; after RunGame() returns it only has to report.
  Memory_Error_Exit = CleanUpAndExitWithError;

  RunGame();

  TheScreen().visible_page().Clear();
  TheScreen().hidden_page().Clear();
  ShutDown();
  return EXIT_SUCCESS;
}

#endif  // RA_NO_ENTRY_POINT

void Prog_End() {
  Audio.Close();
  delete WWMouse;
  WWMouse = nullptr;
  ShutdownTickTimer();
}

void CleanUpAndExitWithError(char* message) {
  ShutDown();
  ExitWithError(message);
}

[[noreturn]] void EmergencyExit(const int exit_code) {
  // Blank the screen first, so nothing glitches while the window loses focus
  // on the way out.
  TheScreen().visible_page().Clear();
  TheScreen().hidden_page().Clear();
  ThePalettes().black_palette().Set();
  ShutDown();
  exit(exit_code);
}
