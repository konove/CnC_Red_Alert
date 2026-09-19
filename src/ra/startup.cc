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
// hands over to RunGame(); Prog_End(), Emergency_Exit() and the memory-error
// hooks tear it all down again.
//
// Originally STARTUP.CPP by Joe L. Bostic, October 1994.

#include "ra/startup.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <span>
#include <string_view>

#include "absl/base/log_severity.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/buffer.h"
#include "port/bytes_of.h"
#include "port/tokenizer.h"
#include "port/win32/win32_registry.h"
#include "port/win32/win32_system.h"
#include "port/win32/win32_types.h"
#include "ra/config.h"
#include "ra/conquer.h"
#include "ra/defines.h"
#include "ra/externs.h"
#include "ra/globals.h"
#include "ra/goptions.h"
#include "ra/ini.h"
#include "ra/init.h"
#include "ra/installation.h"
#include "ra/ipx.h"
#include "ra/ipxaddr.h"
#include "ra/ipxmgr.h"
#include "ra/jshell.h"
#include "ra/language.h"
#include "ra/nullconn.h"
#include "ra/palette.h"
#include "ra/profile.h"
#include "ra/session.h"
#include "ra/special.h"
#include "ra/type.h"
#include "sdllib/drawbuff.h"
#include "sdllib/file.h"
#include "sdllib/gbuffer.h"
#include "sdllib/memflag.h"
#include "sdllib/misc.h"
#include "sdllib/playcd.h"
#include "sdllib/timer.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"
#include "tech/audio_mixer.h"
#include "tech/disk_file.h"
#include "tech/number_parse.h"
#include "tech/search_paths.h"

#ifdef _WIN32
#include <direct.h>  //chdir

#include "ra/ipx95.h"
#endif  // _WIN32

// Prints `string` and exits with status 1 without any cleanup. Installed as
// Memory_Error_Exit once the game systems are gone, or when they are being
// torn down anyway.
[[noreturn]] static void Print_Error_Exit(char* string);

#ifdef _WIN32
HINSTANCE ProgramInstance;
#endif
// Reads the options that have to be known before the window and the network
// exist: blit fills, the screen height, the IPX socket and a bridge network.
static void Read_Setup_Options(DiskFile* config_file);

// Parses the command line, checks the machine can run the game, opens the
// window, sound and video, then runs the game until the player quits.
// Returns the process exit status.
#ifdef _WIN32
int PASCAL WinMain(HINSTANCE instance, HINSTANCE, char* command_line,
                   int command_show)
#else   // _WIN32
int main(int argc, char* argv[])
#endif  // _WIN32
{
  absl::InitializeLog();
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);

  // The original minimum of about 7 MB. The SDL Ram_Free() always reports
  // 64 MB, so this never fails any more.
  if (Ram_Free(MEM_NORMAL) < 7000000) {
    absl::PrintF("%s", kLanguageText.no_ram);

    return EXIT_FAILURE;
  }

#ifdef _WIN32

  // Westwood's own network share: refuse to run a build straight off it.
  if (strstr(command_line, "f:\\projects\\c&c0") != NULL ||
      strstr(command_line, "F:\\PROJECTS\\C&C0") != NULL) {
    MessageBox(0, "Playing off of the network is not allowed.", "Red Alert",
               MB_OK | MB_ICONSTOP);
    return (EXIT_FAILURE);
  }

  int argc;  // Command line argument count
  unsigned command_scan;
  char command_char;
  char* argv[20];  // Pointers to command line arguments
  char path_to_exe[132];

  ProgramInstance = instance;

  // WinMain gets the command line as one string without the program name.
  // Rebuild the DOS-style argc/argv: argv[0] is the full path to the .EXE,
  // the rest point into command_line, which is split in place by writing a
  // terminator over each separating space. The line may end in a carriage
  // return (13) as well as a null. At most 19 arguments are kept.
  GetModuleFileName(instance, &path_to_exe[0], 132);

  argc = 1;
  argv[0] = &path_to_exe[0];

  command_scan = 0;

  do {
    // Skip the spaces before the next argument.
    do {
      command_char = *(command_line + command_scan++);
    } while (command_char == ' ');

    if (command_char != 0 && command_char != 13) {
      argv[argc++] = command_line + command_scan - 1;

      // Find the end of the argument and terminate it there.
      do {
        command_char = *(command_line + command_scan++);
      } while (command_char != ' ' && command_char != 0 && command_char != 13);
      *(command_line + command_scan - 1) = 0;
    }

  } while (command_char != 0 && command_char != 13 && argc < 20);

#endif  // _WIN32

  // Run from the executable's directory, so the relative paths to the config
  // file and the local MIX files resolve however the game was launched.
  const auto dir_path = std::filesystem::path(argv[0]).parent_path();

  if (!dir_path.empty()) {
    std::filesystem::current_path(dir_path);
  }

  // Westwood Online's own installer left these behind. None of it can
  // happen here, but it is what the WOL build did on startup.
  if constexpr (config::kWolapiEnabled) {
    // The version 3 patch shipped wolsetup.exe to install the "Shared
    // Internet Components"; the registry value says it has finished.
    WIN32_FIND_DATA wfd{};
    HANDLE hWOLSetupFile = FindFirstFile("wolsetup.exe", &wfd);
    const bool bWOLSetupFile = (hWOLSetupFile != INVALID_HANDLE_VALUE);
    FindClose(hWOLSetupFile);
    HKEY hKey = nullptr;
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, Game_Registry_Key(), 0, KEY_READ, &hKey);
    DWORD dwValue = 0;
    DWORD dwBufSize = sizeof(DWORD);
    if (RegQueryValueEx(hKey, "WolapiInstallComplete", nullptr, nullptr,
                        port::BytesOf(dwValue), &dwBufSize) == ERROR_SUCCESS) {
      // Setup has finished: delete the setup exe, and drop the registry value
      // only once the exe is gone, so a failed delete is retried next launch.
      if (bWOLSetupFile) {
        if (DeleteFile("wolsetup.exe")) {
          RegDeleteValue(hKey, "WolapiInstallComplete");
        }
      } else {
        RegDeleteValue(hKey, "WolapiInstallComplete");
      }
    }
    RegCloseKey(hKey);

    // The 1.08 patch left a loose conquer.eng in the game directory, and the
    // patch to this version had trouble deleting it. It must not be there:
    // the Aftermath MIX files now carry the string overrides it used to, and
    // a loose file would shadow them.
    if (FindFirstFile("conquer.eng", &wfd) != INVALID_HANDLE_VALUE) {
      DeleteFile("conquer.eng");
    }
  }

  // The process entry point supplies argc valid argv elements. Windows builds
  // populate the local array and count above under the same contract.
  // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
  const std::span arguments(argv, static_cast<size_t>(argc));
  if (Parse_Command_Line(arguments)) {
    InitTickTimer();
    DiskFile cfile(kConfigFileName);

    Keyboard = new KeyboardClass();

    // Refuse to start without 8 MB free for save games and the config file.
    if (Disk_Space_Available() < kInitFreeDiskSpace) {
      absl::PrintF("%s", kLanguageText.insufficient_disk);
      absl::PrintF("%s\n",
                   MustHaveDiskSpaceText(kInitFreeDiskSpace / (1024 * 1024)));
      ShutdownTickTimer();
      return EXIT_FAILURE;
    }

    // The original installer wrote the config file. Without one, start from
    // an empty file: every option has a default.
    if (!cfile.IsAvailable()) {
      cfile.Create();
    }

    if (cfile.IsAvailable()) {
      Read_Private_Config_Struct(cfile, &NewConfig);

      // Sets ScreenHeight, so it has to come before the window is opened.
      Read_Setup_Options(&cfile);

      Create_Main_Window(nullptr, 0, ScreenWidth, ScreenHeight);
      // 22050 Hz mono.
      SoundOn = Audio.Open(11025 * 2, /*stereo=*/false);

      if (!InitDDraw()) {
        return EXIT_FAILURE;
      }

      Options.Adjust_Variables_For_Resolution();

      Memory_Error = &Memory_Error_Handler;

      // The full-screen and editor windows cover the visible viewport, whose
      // size is only known now that the video mode is set.
      base::At(WindowList[0], kWindowWidth) = SeenBuff.Get_Width();
      base::At(WindowList[0], kWindowHeight) = SeenBuff.Get_Height();
      base::At(WindowList[static_cast<int>(WINDOW_EDITOR)], kWindowWidth) =
          SeenBuff.Get_Width();
      base::At(WindowList[static_cast<int>(WINDOW_EDITOR)], kWindowHeight) =
          SeenBuff.Get_Height();

      WWMouse = new WWMouseClass(&SeenBuff, 48, 48);
      MouseInstalled = true;

      SearchPaths::SetCdDrive(CDList.Get_First_CD_Drive());

      // IsFromInstall means "first launch after installing": play the intro
      // movie. The installer used to write PlayIntro=yes; with no entry it
      // still defaults to yes, so a fresh install sees the movie once.
      INIClass ini;
      ini.Load(cfile);

      if (!Special.IsFromInstall) {
        Special.IsFromInstall = ini.Get_Bool("Intro", "PlayIntro", true);
      }
      SlowPalette = ini.Get_Bool("Options", "SlowPalette", false);

      // Whatever happens next, the intro has now been shown once: write
      // PlayIntro=no so later launches go straight to the menu.
      if (Special.IsFromInstall) {
        BreakoutAllowed = true;
        ini.Put_Bool("Intro", "PlayIntro", false);
        ini.Save(cfile);
      }

      // Tiberian Dawn forbids skipping the first-run intro with <ESC>. Red
      // Alert shipped with that assignment changed to true, so the movie can
      // always be skipped, which makes this block a repeat of the one above.
      if (Special.IsFromInstall) {
        BreakoutAllowed = true;
      }

      // While the game runs an out-of-memory exit still has everything to
      // clean up; after RunGame() returns it only has to report.
      Memory_Error_Exit = Print_Error_End_Exit;

      RunGame();

      VisiblePage.Clear();
      HiddenPage.Clear();
      Memory_Error_Exit = Print_Error_Exit;

      // ReadyToQuit is the shutdown handshake with the message handler:
      // 1 is a clean quit, and the Windows handler answered 2 once it had
      // closed everything down. The SDL handler for
      // the quit event calls Prog_End() and exit(0) itself, so on SDL this
      // loop only pumps events until that happens and the return below is
      // never reached.
      ReadyToQuit = 1;

      SDL_Send_Quit();

      do {
        Keyboard->Check();
      } while (ReadyToQuit == 1);

      return EXIT_SUCCESS;
    }
    // The config file could neither be opened nor created. There is no
    // window yet to read a key from, so report and leave.
    absl::PrintF("%s\n", kLanguageText.setup_first);
    ShutdownTickTimer();
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

bool InitDDraw() {
  bool video_success = false;

  // The game draws 640x400. A 400-line mode is asked for first; failing
  // that, 480 lines with the picture letterboxed in the middle.
  if (ScreenHeight == 400) {
    if (Set_Video_Mode(MainWindow, ScreenWidth, ScreenHeight, 8)) {
      video_success = true;
    } else {
      if (Set_Video_Mode(MainWindow, ScreenWidth, 480, 8)) {
        video_success = true;
        ScreenHeight = 480;
      }
    }
  } else {
    if (Set_Video_Mode(MainWindow, ScreenWidth, ScreenHeight, 8)) {
      video_success = true;
    }
  }

  if (!video_success) {
    ShutdownTickTimer();

    return false;
  }

  {
    VisiblePage.Init(ScreenWidth, ScreenHeight, {}, 0,
                     GBC_VISIBLE | GBC_VIDEOMEM);
    HiddenPage.Init(ScreenWidth, ScreenHeight, {}, 0, GBC_NONE);
  }

  // The pages are the full mode; from here on ScreenHeight is the 400-line
  // game area, and SeenBuff/HidPage are views of it 40 lines down in a
  // 480-line mode.
  if (ScreenHeight == 480) {
    ScreenHeight = 400;
  }

  const int yoff = VisiblePage.Get_Height() == 480 ? 40 : 0;

  SeenBuff.Attach(&VisiblePage, 0, yoff, ScreenWidth, ScreenHeight);
  HidPage.Attach(&HiddenPage, 0, yoff, ScreenWidth, ScreenHeight);

  return true;
}

void __cdecl Prog_End() {
  Audio.Close();
  if (WWMouse) {
    delete WWMouse;
    WWMouse = nullptr;
  }
  ShutdownTickTimer();

  // Release owning members of ObjectTypeClass-derived objects in all global
  // type heaps. The custom heap allocator (TFixedIHeapClass) never calls
  // destructors when it frees its buffer, so RAII members (unique_ptr, variant
  // holding vector) must be released explicitly before global destruction.
  const auto reset_object_type = [](ObjectTypeClass* obj) {
    obj->DimensionData.clear();
    obj->RadarIcon.clear();
    obj->ClearImage();
  };
  for (int i = 0; i < AircraftTypes.Count(); i++) {
    reset_object_type(AircraftTypes.Ptr(i));
  }
  for (int i = 0; i < AnimTypes.Count(); i++) {
    reset_object_type(AnimTypes.Ptr(i));
  }
  for (int i = 0; i < BuildingTypes.Count(); i++) {
    reset_object_type(BuildingTypes.Ptr(i));
  }
  for (int i = 0; i < BulletTypes.Count(); i++) {
    reset_object_type(BulletTypes.Ptr(i));
  }
  for (int i = 0; i < InfantryTypes.Count(); i++) {
    reset_object_type(InfantryTypes.Ptr(i));
  }
  for (int i = 0; i < OverlayTypes.Count(); i++) {
    reset_object_type(OverlayTypes.Ptr(i));
  }
  for (int i = 0; i < SmudgeTypes.Count(); i++) {
    reset_object_type(SmudgeTypes.Ptr(i));
  }
  for (int i = 0; i < TemplateTypes.Count(); i++) {
    reset_object_type(TemplateTypes.Ptr(i));
  }
  for (int i = 0; i < TerrainTypes.Count(); i++) {
    reset_object_type(TerrainTypes.Ptr(i));
  }
  for (int i = 0; i < UnitTypes.Count(); i++) {
    reset_object_type(UnitTypes.Ptr(i));
  }
  for (int i = 0; i < VesselTypes.Count(); i++) {
    reset_object_type(VesselTypes.Ptr(i));
  }
}

void Print_Error_End_Exit(char* string) {
  Prog_End();
  absl::PrintF("%s\n", string);
  exit(1);
}

void Print_Error_Exit(char* string) {
  absl::PrintF("%s\n", string);
  exit(1);
}

[[noreturn]] void Emergency_Exit(int code) {
  // Blank the screen first, so nothing glitches while the window loses focus
  // on the way out.
  VisiblePage.Clear();
  HiddenPage.Clear();
  BlackPalette.Set();
  Memory_Error_Exit = Print_Error_Exit;

  // Clean up here rather than through the quit handler's handshake (see
  // main()): the SDL handler ends in exit(0), which would lose `code`.
  Prog_End();
  VisiblePage.Un_Init();
  HiddenPage.Un_Init();
  exit(code);
}

void Read_Setup_Options(DiskFile* config_file) {
  if (config_file->IsAvailable()) {
    INIClass ini;

    ini.Load(*config_file);

    AllowHardwareBlitFills = ini.Get_Bool("Options", "HardwareFills", true);

    // Resolution=yes asks for a 480-line mode; InitDDraw() letterboxes the
    // 400-line game area inside it.
    ScreenHeight = ini.Get_Bool("Options", "Resolution", false) ? 480 : 400;

    // Socket is an offset into the dynamic IPX socket range 0x4000-0x7FFF,
    // letting several games share a network without seeing each other.
    const int socket = ini.Get_Int("Options", "Socket", 0);
    if (socket > 0 && socket < 0x4000) {
      Ipx.Set_Socket(static_cast<uint16_t>(0x4000 + socket));
    }

    // DestNet names a network on the far side of an IPX bridge, as dotted
    // hex bytes: four for the network number, optionally followed by up to
    // six node bytes.
    char netbuf[512];
    base::FillBytes(base::ObjectBytes(netbuf), 0, sizeof(netbuf));
    const char* netptr = netbuf;
    const bool found = ini.Get_String("Options", "DestNet", nullptr, netbuf,
                                      sizeof(netbuf)) != 0;

    if (found && netptr != nullptr && !std::string_view(netbuf).empty()) {
      NetNumType net;
      NetNodeType node;

      // i counts the bytes read: 0-3 are the network number, 4-9 the node.
      int i = 0;
      port::Tokenizer tokens(netbuf, ".");
      while (const char* p = tokens.Next()) {
        const auto byte = tech::ParseHex<uint8_t>(p);
        if (!byte || i >= 10) {
          i = 0;  // Reject the address instead of accepting a partial network.
          break;
        }
        if (i < 4) {
          base::At(net, i) = *byte;
        } else {
          base::At(node, i - 4) = *byte;
        }
        i++;
      }

      // Only the network number matters: the node is replaced by the
      // broadcast node, so packets reach every machine across the bridge.
      if (i >= 4) {
        Session.IsBridge = 1;
        base::FillBytes(base::ObjectBytes(node), 0xff, 6);
        Session.BridgeNet = IPXAddressClass(net, node);
      }
    }
  }
}
