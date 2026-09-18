#ifndef TD_NO_ENTRY_POINT
#include "port/bytes_of.h"
#endif
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
 *   Prog_End -- Cleans up library systems in prep for game exit. * main --
 *Initial startup routine (preps library systems). *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>
#include <string_view>
#include <vector>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "port/tokenizer.h"
#include "sdllib/drawbuff.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/timer.h"
#include "sdllib/ww_audio.h"
#include "td/defines.h"
#include "td/externs.h"
#include "td/globals.h"
#include "td/ipx.h"
#include "td/ipxaddr.h"
#include "td/ipxmgr.h"
#include "td/nullmgr.h"
#include "td/profile.h"
#include "tech/disk_file.h"
#include "tech/number_parse.h"

// The two tests that link this file define TD_NO_ENTRY_POINT; these headers
// serve only main().
#ifndef TD_NO_ENTRY_POINT
#include <filesystem>

#include "absl/base/log_severity.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/strings/match.h"
#include "sdllib/file.h"
#include "sdllib/gbuffer.h"
#include "sdllib/memflag.h"
#include "sdllib/playcd.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"
#include "td/conquer.h"
#include "td/goptions.h"
#include "td/init.h"
#include "td/jshell.h"
#include "td/special.h"
#include "tech/search_paths.h"
#endif  // TD_NO_ENTRY_POINT

#ifdef _WIN32
#include <direct.h>  //chdir
#include <windows.h>

#include "td/ccdde.h"
#endif

void Delete_Swap_Files();
[[maybe_unused]] [[noreturn]] static void Print_Error_End_Exit(char* string);
[[maybe_unused]] [[noreturn]] static void Print_Error_Exit(char* string);

[[maybe_unused]] static void Read_Setup_Options(DiskFile* config_file);

bool SpawnedFromWChat = false;

extern "C" {
bool __cdecl Detect_MMX_Availability();
void __cdecl Init_MMX();
}

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
void Move_Point(int16_t& x, int16_t& y, DirType dir, uint16_t distance);

#ifndef TD_NO_ENTRY_POINT
#ifdef _WIN32
int PASCAL WinMain(HINSTANCE instance, HINSTANCE, char* command_line,
                   int command_show)
#else   // _WIN32
int main(int argc, char* argv[])
#endif  // _WIN32
{
  absl::InitializeLog();
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);

  // Heap_Dump_Check( "first thing in main" );
  //	malloc(1);

  CCDebugString("C&C95 - Starting up.\n");

  // CD_Test();

  /*
  ** These values return 0x47 if code is working correctly
  */
  //	int temp = Desired_Facing256 (1070, 5419, 1408, 5504);

  if (Ram_Free(MEM_NORMAL) < 5000000) {
#ifdef GERMAN
    absl::PrintF("Zuwenig Hauptspeicher verfügbar.\n");
#else
#ifdef FRENCH
    absl::PrintF("Mémoire vive (RAM) insuffisante.\n");
#else
    absl::PrintF("Insufficient RAM available.\n");
#endif
#endif
    return EXIT_FAILURE;
  }

  // void *test_buffer = Alloc(20,MEM_NORMAL);

  // memset ((char*)test_buffer, 0, 21);

  // Free(test_buffer);

#ifdef _WIN32
  int argc;  // Command line argument count
  unsigned command_scan;
  char command_char;
  char* argv[20];  // Pointers to command line arguments
  char path_to_exe[280];

  ProgramInstance = instance;

  /*
  ** Get the full path to the .EXE
  */
  GetModuleFileName(instance, &path_to_exe[0], 280);

  /*
  ** First argument is supposed to be a pointer to the .EXE that is running
  **
  */
  argc = 1;  // Set argument count to 1
  argv[0] =
      &path_to_exe[0];  // Set 1st command line argument to point to full path

  /*
  ** Get pointers to command line arguments just like if we were in DOS
  **
  ** The command line we get is cr/zero? terminated.
  **
  */

  command_scan = 0;

  do {
    /*
    ** Scan for non-space character on command line
    */
    do {
      command_char = *(command_line + command_scan++);
    } while (command_char == ' ');

    if (command_char != 0 && command_char != 13) {
      argv[argc++] = command_line + command_scan - 1;

      /*
      ** Scan for space character on command line
      */
      do {
        command_char = *(command_line + command_scan++);
      } while (command_char != ' ' && command_char != 0 && command_char != 13);
      *(command_line + command_scan - 1) = 0;
    }

  } while (command_char != 0 && command_char != 13 && argc < 20);
#endif

  // Change to executable's directory (if path is present)
  const auto dir_path = std::filesystem::path(argv[0]).parent_path();

  if (!dir_path.empty()) {
    std::filesystem::current_path(dir_path);
  }

#ifdef JAPANESE
  ForceEnglish = false;
#endif
  // main receives argc valid argument pointers from the C++ runtime. On
  // Windows, the local argv array above is bounded by its command-line parser.
  // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
  const std::span<char*> arguments(argv, base::ToSize(argc));
  if (Parse_Command_Line(arguments)) {
    InitTickTimer();
    TickCount.Start();

    DiskFile cfile("CONQUER.INI");

    /*
    ** If there is not enough disk space free, dont allow the product to run.
    */

    if (Disk_Space_Available() < INIT_FREE_DISK_SPACE) {
      // pretty unlikely
      ShutdownTickTimer();
      return EXIT_FAILURE;
    }

    SearchPaths::SetCdDrive(CDList.Get_First_CD_Drive());

    if (!cfile.IsAvailable()) {
      // just create an empty config, we don't care about most of it anyway
      cfile.Create();
    }

    if (cfile.IsAvailable()) {
      const auto config_data = port::CharBytes(Load_Alloc_Data(cfile));
      char* cdata = config_data.data();
      Read_Private_Config_Struct(cdata, &NewConfig);
      delete[] cdata;
      Read_Setup_Options(&cfile);

      CCDebugString("C&C95 - Creating main window.\n");

      Create_Main_Window(nullptr, 0, ScreenWidth, ScreenHeight);
      CCDebugString("C&C95 - Initialising audio.\n");

      SoundOn = Audio_Init(MainWindow, 16, false, 11025 * 2, 0);

      Palette.assign(768, 0);

      bool video_success = false;
      CCDebugString("C&C95 - Setting video mode.\n");
      /*
      ** Set 640x400 video mode. If its not available then try for 640x480
      */
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
        CCDebugString("C&C95 - Failed to set video mode.\n");
        ShutdownTickTimer();
        Palette.clear();
        Palette.clear();
        return EXIT_FAILURE;
      }

      CCDebugString("C&C95 - Initialising video surfaces.\n");

      {
        VisiblePage.Init(ScreenWidth, ScreenHeight, {}, 0,
                         GBC_VISIBLE | GBC_VIDEOMEM);
        HiddenPage.Init(ScreenWidth, ScreenHeight, {}, 0,
                        static_cast<GBC_Enum>(0));
      }

      if (ScreenHeight == 480) {
        ScreenHeight = 400;
      }

      const int yoff = VisiblePage.Get_Height() == 480 ? 40 : 0;

      SeenBuff.Attach(&VisiblePage, 0, yoff, ScreenWidth, ScreenHeight);
      HidPage.Attach(&HiddenPage, 0, yoff, ScreenWidth, ScreenHeight);

      CCDebugString("C&C95 - Adjusting variables for resolution.\n");
      Options.Adjust_Variables_For_Resolution();

      CCDebugString("C&C95 - Setting palette.\n");
      /////////Set_Palette(Palette);

      WindowList[0][kWindowWidth] = SeenBuff.Get_Width() / 8;
      WindowList[0][kWindowHeight] = SeenBuff.Get_Height();

      /*
      ** Install the memory error handler
      */
      Memory_Error = &Memory_Error_Handler;

      CCDebugString("C&C95 - Creating mouse class.\n");
      WWMouse = new WWMouseClass(&SeenBuff, 32, 32);
      //			MouseInstalled = Install_Mouse(32,24,320,200);
      MouseInstalled = true;

      /*
      ** See if we should run the intro
      */
      CCDebugString("C&C95 - Reading CONQUER.INI.\n");
      std::vector<char> profile_storage(64000);
      char* buffer = profile_storage.data();
      cfile.Read(std::as_writable_bytes(std::span(profile_storage))
                     .first(profile_storage.size() - 1));

      /*
      **	Check for forced intro movie run disabling. If the conquer
      **	configuration file says "no", then don't run the intro.
      */
      char tempbuff[5];
      WWGetPrivateProfileString(
          "Intro", "PlayIntro", "Yes",
          std::span(tempbuff).first(static_cast<std::size_t>(4)), buffer);
      Special.IsFromInstall =
          !absl::EqualsIgnoreCase(tempbuff, "No") && !SpawnedFromWChat;
      SlowPalette =
          WWGetPrivateProfileInt("Options", "SlowPalette", 1, buffer) != 0;

#ifdef DEMO
      /*
      **	Check for override directory path for CD searches.
      */
      WWGetPrivateProfileString("CD", "Path", ".", OverridePath, buffer);
#endif

      /*
      ** Regardless of whether we should run it or not, here we're
      ** gonna change it to say "no" in the future.
      */
      WWWritePrivateProfileString("Intro", "PlayIntro", "No", profile_storage);
      cfile.Write(std::as_bytes(std::span(profile_storage))
                      .first(std::string_view(buffer).size()));

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
      if (Special.IsFromInstall) {
        BreakoutAllowed = false;
      }

      Memory_Error_Exit = Print_Error_End_Exit;

      CCDebugString("C&C95 - Entering main game.\n");
      Main_Game(argc, argv);

      VisiblePage.Clear();
      HiddenPage.Clear();
      //			Set_Video_Mode(RESET_MODE);

      Memory_Error_Exit = Print_Error_Exit;

      CCDebugString("C&C95 - About to exit.\n");
      ReadyToQuit = true;
      SDL_Send_Quit();
      do {
        Keyboard::Check();
      } while (ReadyToQuit);

      CCDebugString("C&C95 - Returned from final message loop.\n");
      // Prog_End();
      // Invalidate_Cached_Icons();
      // VisiblePage.Un_Init();
      // HiddenPage.Un_Init();
      // AllSurfaces.Release();
      // Reset_Video_Mode();
      // Stop_Profiler();
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
    Kbd.Get();
#endif

    //		Remove_Keyboard_Interrupt();
    ShutdownTickTimer();

    if (!Palette.empty()) {
      Palette.clear();
      Palette.clear();
    }
  }

  /*
  **	Restore the current drive and directory.
  */
#ifdef NOT_FOR_WIN95
  _dos_setdrive(olddrive, &drivecount);
  chdir(oldpath);
#endif  // NOT_FOR_WIN95

  return EXIT_SUCCESS;
}

#endif  // TD_NO_ENTRY_POINT

/***********************************************************************************************
 * Prog_End -- Cleans up library systems in prep for game exit. *
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
void __cdecl Prog_End() {
#ifndef DEMO
  if (GameToPlay == GAME_MODEM || GameToPlay == GAME_NULL_MODEM) {
    NullModemClass::Change_IRQ_Priority(0);
  }
#endif
  CCDebugString("C&C95 - About to call Sound_End.\n");
  Sound_End();
  CCDebugString("C&C95 - Returned from Sound_End.\n");
  if (WWMouse) {
    CCDebugString("C&C95 - Deleting mouse object.\n");
    delete WWMouse;
    WWMouse = nullptr;
  }
  CCDebugString("C&C95 - Deleting tick timer.\n");
  ShutdownTickTimer();

  if (!Palette.empty()) {
    CCDebugString("C&C95 - Deleting palette object.\n");
    Palette.clear();
    Palette.shrink_to_fit();
  }
}

void Print_Error_End_Exit(char* string) {
  absl::PrintF("%s\n", string);
  Get_Key();
  Prog_End();
  absl::PrintF("%s\n", string);
  exit(1);
}

void Print_Error_Exit(char* string) {
  absl::PrintF("%s\n", string);
  exit(1);
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
void Read_Setup_Options(DiskFile* config_file) {
  std::vector<char> profile_storage(base::ToSize(config_file->Size() + 1));
  char* buffer = profile_storage.data();

  if (config_file->IsAvailable()) {
    config_file->Read(std::as_writable_bytes(std::span(profile_storage))
                          .first(profile_storage.size() - 1));

    AllowHardwareBlitFills =
        WWGetPrivateProfileInt("Options", "HardwareFills", 1, buffer) != 0;
    ScreenHeight =
        WWGetPrivateProfileInt("Options", "Resolution", 0, buffer) ? 480 : 400;
    IsV107 = WWGetPrivateProfileInt("Options", "Compatibility", 0, buffer) != 0;

    /*
    ** See if an alternative socket number has been specified
    */
    int socket = WWGetPrivateProfileInt("Options", "Socket", 0, buffer);
    if (socket > 0) {
      socket += 0x4000;
      if (socket >= 0x4000 && socket < 0x8000) {
        Ipx.Set_Socket(static_cast<uint16_t>(socket));
      }
    }

    /*
    ** See if a destination network has been specified
    */
    char netbuf[512];
    base::FillBytes(base::ObjectBytes(netbuf), 0, sizeof(netbuf));
    const char* netptr = WWGetPrivateProfileString("Options", "DestNet",
                                                   nullptr, netbuf, buffer);

    if (netptr && !std::string_view(netbuf).empty()) {
      NetNumType net;
      NetNodeType node;

      /*
      ** Scan the string, pulling off each address piece
      */
      int i = 0;
      port::Tokenizer tokens(netbuf, ".");
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
  }
}
