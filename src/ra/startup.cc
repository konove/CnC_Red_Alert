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

/* $Header: /counterstrike/STARTUP.CPP 6     3/15/97 7:18p Steve_tall $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
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
 *                  Last Update : September 30, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Prog_End -- Cleans up library systems in prep for game exit. *
 *   main -- Initial startup routine (preps library systems). *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/startup.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "port/win32/win32_registry.h"
#include "port/win32/win32_system.h"
#include "ra/config.h"
#include "ra/conquer.h"
#include "ra/defines.h"
#include "ra/externs.h"
#include "ra/globals.h"
#include "ra/goptions.h"
#include "ra/ini.h"
#include "ra/init.h"
#include "ra/ipx.h"
#include "ra/ipxaddr.h"
#include "ra/ipxmgr.h"
#include "ra/jshell.h"
#include "ra/language.h"
#include "ra/nullconn.h"
#include "ra/palette.h"
#include "ra/session.h"
#include "ra/special.h"
#include "sdllib/drawbuff.h"
#include "sdllib/file.h"
#include "sdllib/gbuffer.h"
#include "sdllib/memflag.h"
#include "sdllib/misc.h"
#include "sdllib/playcd.h"
#include "sdllib/timer.h"
#include "sdllib/ww_audio.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"
#include "tech/cdfile.h"
#include "tech/rawfile.h"
#include "tech/wwfile.h"

#ifdef _WIN32
#include <direct.h>  //chdir

#include "ra/ipx95.h"
#endif  // _WIN32

// #include "ra/woldebug.h"

bool Read_Private_Config_Struct(FileClass& file, NewConfigType* config);
void Print_Error_Exit(char* string);

extern void Create_Main_Window(HANDLE instance, int command_show, int width,
                               int height);
extern bool RA95AlreadyRunning;
#ifdef _WIN32
HINSTANCE ProgramInstance;
#endif
void Check_Use_Compressed_Shapes();
void Read_Setup_Options(RawFileClass* config_file);
bool VideoBackBufferAllowed = true;

// #if (ENGLISH)
// #define WINDOW_NAME "Red Alert"
// #endif
//
// #if (FRENCH)
// #define WINDOW_NAME "Alerte Rouge"
// #endif
//
// #if (GERMAN)
// #define WINDOW_NAME "Alarmstufe Rot"
// #endif

/***********************************************************************************************
 * main -- Initial startup routine (preps library systems). *
 *                                                                                             *
 *    This is the routine that is first called when the program starts up. It
 *basically        * handles the command line parsing and setting up library
 *systems.                         *
 *                                                                                             *
 * INPUT:   argc  -- Number of command line arguments. *
 *                                                                                             *
 *          argv  -- Pointer to array of command line argument strings. *
 *                                                                                             *
 * OUTPUT:  Returns with execution failure code (if any). *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/20/1995 JLB : Created. *
 *=============================================================================================*/
#ifdef _WIN32
int PASCAL WinMain(HINSTANCE instance, HINSTANCE, char* command_line,
                   int command_show)
#else   // _WIN32
int main(int argc, char* argv[])
#endif  // _WIN32
{
  absl::InitializeLog();
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);

  if (Ram_Free(MEM_NORMAL) < 7000000) {
    printf(TEXT_NO_RAM);

    return EXIT_FAILURE;
  }

#ifdef _WIN32

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

  /*
  ** Get the full path to the .EXE
  */
  GetModuleFileName(instance, &path_to_exe[0], 132);

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

#endif  // _WIN32

  // Change to executable's directory (if path is present)
  auto dir_path = std::filesystem::path(argv[0]).parent_path();

  if (!dir_path.empty()) {
    std::filesystem::current_path(dir_path);
  }

  //	Westwood Online's own installer left these behind. None of it can
  //	happen here, but it is what the WOL build did on startup.
  if constexpr (config::kWolapiEnabled) {
    //	Look for special wolapi install program, used after the patch to version
    // 3, to install "Shared Internet Components".
    WIN32_FIND_DATA wfd;
    HANDLE hWOLSetupFile = FindFirstFile("wolsetup.exe", &wfd);
    bool bWOLSetupFile = (hWOLSetupFile != INVALID_HANDLE_VALUE);
    //	if( bWOLSetupFile )
    //		debugprint( "Found wolsetup.exe\n" );
    FindClose(hWOLSetupFile);
    //	Look for special registry entry that tells us when the setup exe has
    // done its thing.
    HKEY hKey;
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, Game_Registry_Key(), 0, KEY_READ, &hKey);
    DWORD dwValue;
    DWORD dwBufSize = sizeof(DWORD);
    if (RegQueryValueEx(hKey, "WolapiInstallComplete", nullptr, nullptr,
                        (LPBYTE)&dwValue, &dwBufSize) == ERROR_SUCCESS) {
      //		debugprint( "Found WolapiInstallComplete in registry\n"
      //); 	Setup has finished. Delete the setup exe and remove reg key.
      if (bWOLSetupFile) {
        if (DeleteFile("wolsetup.exe")) {
          RegDeleteValue(hKey, "WolapiInstallComplete");
        }
      } else {
        RegDeleteValue(hKey, "WolapiInstallComplete");
      }
    }
    RegCloseKey(hKey);

    //	I've been having problems getting the patch to delete "conquer.eng",
    // which is present in the game 	directory for 1.08, but which must NOT
    // be present for this version (Aftermath mix files provide the 	string
    // overrides that the 1.08 separate conquer.eng did before Aftermath).
    // Delete conquer.eng if it's found.
    if (FindFirstFile("conquer.eng", &wfd) != INVALID_HANDLE_VALUE) {
      DeleteFile("conquer.eng");
    }
  }

  if (Parse_Command_Line(argc, argv)) {
    InitTickTimer();
    RawFileClass cfile(kConfigFileName);

    Keyboard = new KeyboardClass();

    /*
    ** If there is loads of memory then use uncompressed shapes
    */
    Check_Use_Compressed_Shapes();

    /*
    ** If there is not enough disk space free, don't allow the product to run.
    */
    if (Disk_Space_Available() < kInitFreeDiskSpace) {
      // pretty unlikely, but print something anyway
      printf(TEXT_INSUFFICIENT);
      printf(TEXT_MUST_HAVE, kInitFreeDiskSpace / (1024 * 1024));
      printf("\n");
      ShutdownTickTimer();
      return EXIT_FAILURE;
    }

    if (!cfile.Is_Available()) {
      // just create an empty config, we don't care about most of it anyway
      cfile.Create();
    }

    if (cfile.Is_Available()) {
      Read_Private_Config_Struct(cfile, &NewConfig);

      Read_Setup_Options(&cfile);

      Create_Main_Window(nullptr, 0, ScreenWidth, ScreenHeight);
      SoundOn = Audio_Init(MainWindow, 16, false, 11025 * 2, 0);

      if (!InitDDraw()) {
        return EXIT_FAILURE;
      }

      Options.Adjust_Variables_For_Resolution();

      /*
      ** Install the memory error handler
      */
      Memory_Error = &Memory_Error_Handler;

      WindowList[0][WINDOWWIDTH] = SeenBuff.Get_Width();
      WindowList[0][WINDOWHEIGHT] = SeenBuff.Get_Height();
      WindowList[WINDOW_EDITOR][WINDOWWIDTH] = SeenBuff.Get_Width();
      WindowList[WINDOW_EDITOR][WINDOWHEIGHT] = SeenBuff.Get_Height();

      WWMouse = new WWMouseClass(&SeenBuff, 48, 48);
      MouseInstalled = true;

      CDFileClass::Set_CD_Drive(CDList.Get_First_CD_Drive());

      /*
      ** See if we should run the intro
      */
      INIClass ini;
      ini.Load(cfile);

      /*
      **	Check for forced intro movie run disabling. If the conquer
      **	configuration file says "no", then don't run the intro.
      */
      if (!Special.IsFromInstall) {
        Special.IsFromInstall = ini.Get_Bool("Intro", "PlayIntro", true);
      }
      SlowPalette = ini.Get_Bool("Options", "SlowPalette", false);

      /*
      ** Regardless of whether we should run it or not, here we're
      ** gonna change it to say "no" in the future.
      */
      if (Special.IsFromInstall) {
        BreakoutAllowed = true;
        ini.Put_Bool("Intro", "PlayIntro", false);
        ini.Save(cfile);
      }

      /*
      **	If the intro is being run for the first time, then don't
      **	allow breaking out of it with the <ESC> key.
      */
      if (Special.IsFromInstall) {
        BreakoutAllowed = true;
      }

      Memory_Error_Exit = Print_Error_End_Exit;

      Main_Game(argc, argv);

      VisiblePage.Clear();
      HiddenPage.Clear();
      Memory_Error_Exit = Print_Error_Exit;

      /*
      ** Flag that this is a clean shutdown (not killed with Ctrl-Alt-Del)
      */
      ReadyToQuit = 1;

      /*
      ** Post a message to our message handler to tell it to clean up.
      */
      SDL_Send_Quit();

      /*
      ** Wait until the message handler has dealt with the message
      */
      do {
        Keyboard->Check();
      } while (ReadyToQuit == 1);

      return EXIT_SUCCESS;
    }
    puts(TEXT_SETUP_FIRST);
    Keyboard->Get();

    ShutdownTickTimer();
  }
  /*
  **	Restore the current drive and directory.
  */
  return EXIT_SUCCESS;
}

/* Initialize DirectDraw and surfaces */
bool InitDDraw() {
  bool video_success = false;

  /* Set 640x400 video mode. If its not available then try for 640x480 */
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
    VisiblePage.Init(ScreenWidth, ScreenHeight, nullptr, 0,
                     GBC_VISIBLE | GBC_VIDEOMEM);
    HiddenPage.Init(ScreenWidth, ScreenHeight, nullptr, 0, GBC_NONE);
  }

  if (ScreenHeight == 480) {
    ScreenHeight = 400;
  }

  int yoff = VisiblePage.Get_Height() == 480 ? 40 : 0;

  SeenBuff.Attach(&VisiblePage, 0, yoff, ScreenWidth, ScreenHeight);
  HidPage.Attach(&HiddenPage, 0, yoff, ScreenWidth, ScreenHeight);

  return true;
}

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
  Sound_End();
  if (WWMouse) {
    delete WWMouse;
    WWMouse = nullptr;
  }
  ShutdownTickTimer();

  // Release owning members of ObjectTypeClass-derived objects in all global
  // type heaps. The custom heap allocator (TFixedIHeapClass) never calls
  // destructors when it frees its buffer, so RAII members (unique_ptr, variant
  // holding vector) must be released explicitly before global destruction.
  auto reset_object_type = [](ObjectTypeClass* obj) {
    obj->DimensionData.reset();
    obj->RadarIcon.reset();
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
  printf("%s\n", string);
  exit(1);
}

void Print_Error_Exit(char* string) {
  printf("%s\n", string);
  exit(1);
}

/***********************************************************************************************
 * Emergency_Exit -- Function to call when we want to exit unexpectedly. * Use
 *this function instead of exit(n) so everything is properly cleaned up.*
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Code to return to the OS *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 3/13/97 1:32AM ST : Created *
 *=============================================================================================*/
[[noreturn]] void Emergency_Exit(int code) {
  /*
  ** Clear out the video buffers so we dont glitch when we lose focus
  */
  VisiblePage.Clear();
  HiddenPage.Clear();
  BlackPalette.Set();
  Memory_Error_Exit = Print_Error_Exit;

  /*
  ** Flag that this is an emergency shut down - not a clean shutdown but
  ** not killed with Ctrl-Alt-Del either.
  */
  ReadyToQuit = 3;

  /*
  ** Post a message to our message handler to tell it to clean up.
  */
  SDL_Send_Quit();

  /*
  ** Wait until the message handler has dealt with the message
  */
  do {
    Keyboard->Check();
  } while (ReadyToQuit == 3);

  exit(code);
}

/***********************************************************************************************
 * Read_Setup_Options -- Read stuff in from the INI file that we need to know
 *sooner           *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Ptr to config file class *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 6/7/96 4:09PM ST : Created * 09/30/1996 JLB : Uses INI class. *
 *=============================================================================================*/
void Read_Setup_Options(RawFileClass* config_file) {
  if (config_file->Is_Available()) {
    INIClass ini;

    ini.Load(*config_file);

    /*
    ** Read in the boolean options
    */
    VideoBackBufferAllowed = ini.Get_Bool("Options", "VideoBackBuffer", true);
    AllowHardwareBlitFills = ini.Get_Bool("Options", "HardwareFills", true);

    ScreenHeight = ini.Get_Bool("Options", "Resolution", false) ? 480 : 400;

    /*
    ** See if an alternative socket number has been specified
    */
    int socket = ini.Get_Int("Options", "Socket", 0);
    if (socket > 0) {
      socket += 0x4000;
      if (socket >= 0x4000 && socket < 0x8000) {
        Ipx.Set_Socket(socket);
      }
    }

    /*
    ** See if a destination network has been specified
    */
    char netbuf[512];
    memset(netbuf, 0, sizeof(netbuf));
    char* netptr = netbuf;
    bool found =
        ini.Get_String("Options", "DestNet", nullptr, netbuf, sizeof(netbuf));

    if (found && netptr != nullptr && strlen(netbuf)) {
      NetNumType net;
      NetNodeType node;

      /*
      ** Scan the string, pulling off each address piece
      */
      int i = 0;
      char* p = strtok(netbuf, ".");
      unsigned int x = 0;
      while (p != nullptr) {
        sscanf(p, "%x", &x);  // convert from hex string to int
        if (i < 4) {
          net[i] = static_cast<unsigned char>(x);  // fill NetNum
        } else {
          node[i - 4] = static_cast<unsigned char>(x);  // fill NetNode
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
    }
  }
}
