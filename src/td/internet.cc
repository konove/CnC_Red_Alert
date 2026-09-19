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

/*************************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S **
 *************************************************************************************
 *                                                                                   *
 *                 Project Name : Command & Conquer - Red Alert *
 *                                                                                   *
 *                    File Name : INTERNET.CPP *
 *                                                                                   *
 *                   Programmer : Steve Tall *
 *                                                                                   *
 *                   Start Date : March 11th, 1996 *
 *                                                                                   *
 *                  Last Update : August 5th, 1996 [ST] *
 *                                                                                   *
 *-----------------------------------------------------------------------------------*
 * Overview: *
 *                                                                                   *
 *  Miscellaneous junk related to H2H internet connection. *
 *                                                                                   *
 *-----------------------------------------------------------------------------------*
 * Functions: * Check_From_WChat -- Interprets start game packet from WChat
 ** Read_Game_Options -- Read the game setup options from the wchat packet
 ** Is_User_WChat_Registered -- retrieve the users wchat entry from registry
 ** Spawn_WChat -- spawns or switches focus to wchat * Spawn_Registration_App --
 *spawns the C&C/Planet westwood registration app			*
 *  Do_The_Internet_Menu_Thang -- Handle case where user clicks on 'Internet'
 *button *
 *                                                                         				*
 *                                                                         				*
 *                                                                         				*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - */
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <span>
#include <string_view>

#include "base/array.h"
#include "port/safe_string.h"
#include "sdllib/drawbuff.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "td/conquer.h"
#include "td/defines.h"
#include "td/dialog.h"
#include "td/externs.h"
#include "td/globals.h"
#include "td/goptions.h"
#include "td/init.h"
#include "td/jshell.h"
#include "td/mplayer.h"
#include "td/msgbox.h"
#include "td/palette.h"
#include "td/profile.h"
#include "td/special.h"
#include "td/tcpip.h"
#include "td/text.h"
#include "td/textbtn.h"
#include "tech/disk_file.h"
#include "tech/game_file.h"
#include "tech/number_parse.h"

#ifdef _WIN32
#include "td/ccdde.h"
#endif

/***************************************************************************
** Internet specific globals
*/
char PlanetWestwoodIPAddress[IP_ADDRESS_MAX] = {
    "206.154.108.87"};                 // IP of server or other player
int32_t PlanetWestwoodPortNumber = 1234;  // Port number to send to
bool PlanetWestwoodIsHost =
    false;  // Flag true if player has control of game options
uint32_t PlanetWestwoodGameID;     // Game ID
uint32_t PlanetWestwoodStartTime;  // Time that game was started
#ifdef _WIN32
HWND WChatHWND = 0;  // Handle to Wchat window.
#endif
bool UseVirtualSubnetServer;
int InternetMaxPlayers;
int WChatMaxAhead;
int WChatSendRate;

int Read_Game_Options();

/***********************************************************************************************
 * Check_From_WChat -- This function reads in C&CSPAWN.INI and interprets it *
 *                     C&CSPAWN.INI is now sent to us by WCHAT via DDE *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Name of C&CSPAWN.INI file. If NULL then get file from DDE Server *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 6/9/96 1:44PM ST : Created *
 *=============================================================================================*/
void Check_From_WChat(const char* wchat_name) {
#ifndef DEMO

  char default_string[] = {"Error"};
  char key_string[256];
  std::array<char, 8192> ini_storage{};
  char* ini_file = nullptr;
  DiskFile wchat_file;

  /*
  ** Get a pointer to C&CSPAWN.INI either by reading it from disk or getting it
  *from
  ** the DDE server.
  */
  if (wchat_name) {
    ini_file = ini_storage.data();
  } else {
#ifdef _WIN32
    ini_file = DDEServer.Get_MPlayer_Game_Info();
#endif
  }

  if (wchat_name) {
    wchat_file.SetName(wchat_name);
  }

  if (!wchat_name || wchat_file.IsAvailable()) {
    /*
    ** Read the ini file from disk if we founf it there
    */
    if (wchat_name) {
      wchat_file.Read(std::span(ini_storage).first(ini_storage.size() - 1),
                      std::min<int64_t>(wchat_file.Size(), 8191));
    }

    /*
    ** Get the IP address
    */
    base::At(key_string, 0) = 0;

    WWGetPrivateProfileString("Internet", "Address", default_string, key_string,
                              ini_file);

    if ((std::string_view(key_string) == default_string)) {
      if (wchat_name) {
      }
      return;
    }
    port::SafeCopy(PlanetWestwoodIPAddress, key_string);

    /*
    ** Get the port number
    */
    base::At(key_string, 0) = 0;

    WWGetPrivateProfileString("Internet", "Port", default_string, key_string,
                              ini_file);

    if ((std::string_view(key_string) == default_string)) {
      if (wchat_name) {
      }
      return;
    }

    PlanetWestwoodPortNumber = tech::ParseIntegerOr<int>(key_string, 0);

    /*
    ** Get host or client
    */
    base::At(key_string, 0) = 0;

    WWGetPrivateProfileString("Internet", "Host", default_string, key_string,
                              ini_file);

    if ((std::string_view(key_string) == default_string)) {
      if (wchat_name) {
      }
      return;
    }

    PlanetWestwoodIsHost = std::string_view(key_string).contains('1');

    UseVirtualSubnetServer =
        WWGetPrivateProfileInt("Internet", "UseVSS", 0, ini_file) != 0;

    Special.IsFromWChat = true;
  }

  if (wchat_name) {
  }

#else  // DEMO

  wchat_name = wchat_name;

#endif  // DEMO
}

// EventClass Wibble;

/***************************************************************************
 * Read_Game_Options -- reads multiplayer game options from disk           *
 *                                                                         *
 * This routine is used for multiplayer games which read the game options
 ** from disk, rather than through a connection dialog.
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		name of C&CSPAWN.INI file. Null if data should be got from DDE
 *server* * OUTPUT: * 1 = OK, 0 = error
 **
 *                                                                         *
 * WARNINGS:                          \                                     *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   01/11/1996 BRR : Created.                                             *
 *=========================================================================*/
int Read_Game_Options(const char* name) {
  std::array<char, 8192> ini_storage{};
  char* buffer = nullptr;

  char filename[256] = {"INVALID.123"};

  if (name) {
    port::SafeCopy(filename, name);
  }

  /*------------------------------------------------------------------------
  Create filename and read the file.
  ------------------------------------------------------------------------*/
  GameFile file(filename);

  if (name && !file.IsAvailable()) {
    return 0;
  }
  if (name) {
    buffer = ini_storage.data();  // INI staging buffer pointer.

    file.Read(std::span(ini_storage).first(8191));
    file.Close();
  } else {
#ifdef _WIN32
    buffer = DDEServer.Get_MPlayer_Game_Info();
#endif
  }

  /*------------------------------------------------------------------------
  Get the player's name
  ------------------------------------------------------------------------*/
  WWGetPrivateProfileString("Options", "Handle", "Noname", MPlayerName, buffer);
  port::SafeCopy(MPlayerGameName, MPlayerName);
  MPlayerColorIdx = WWGetPrivateProfileInt("Options", "Color", 0, buffer);
  MPlayerPrefColor = MPlayerColorIdx;
  MPlayerHouse = static_cast<HousesType>(WWGetPrivateProfileInt(
      "Options", "Side", static_cast<int>(HOUSE_GOOD), buffer));

  MPlayerCredits = WWGetPrivateProfileInt("Options", "Credits", 0, buffer);
  MPlayerBases = WWGetPrivateProfileInt("Options", "Bases", 0, buffer);
  MPlayerTiberium = WWGetPrivateProfileInt("Options", "Tiberium", 0, buffer);
  MPlayerGoodies = WWGetPrivateProfileInt("Options", "Crates", 0, buffer);
  MPlayerGhosts = WWGetPrivateProfileInt("Options", "AI", 0, buffer);
  BuildLevel = WWGetPrivateProfileInt("Options", "BuildLevel", 0, buffer);
  MPlayerUnitCount = WWGetPrivateProfileInt("Options", "UnitCount", 0, buffer);
  Seed = WWGetPrivateProfileInt("Options", "Seed", 0, buffer);
  Special.IsCaptureTheFlag = static_cast<unsigned>(
      WWGetPrivateProfileInt("Options", "CaptureTheFlag", 0, buffer));
  // externs.h declares these unsigned long; the INI stores them as ints.
  PlanetWestwoodGameID = static_cast<uint32_t>(
      WWGetPrivateProfileInt("Internet", "GameID", 0, buffer));
  PlanetWestwoodStartTime = static_cast<uint32_t>(
      WWGetPrivateProfileInt("Internet", "StartTime", 0, buffer));

  InternetMaxPlayers =
      WWGetPrivateProfileInt("Internet", "MaxPlayers", 2, buffer);

  if (MPlayerTiberium) {
    Special.IsTGrowth = 1;
    Special.IsTSpread = 1;
  } else {
    Special.IsTGrowth = 0;
    Special.IsTSpread = 0;
  }
  ScenarioIdx = WWGetPrivateProfileInt("Options", "Scenario", 0, buffer);
  Scenario = ScenarioIdx;  // MPlayerFilenum[ScenarioIdx];

  Options.GameSpeed = 0;

  MPlayerLocalID = static_cast<unsigned char>(
      Build_MPlayerID(MPlayerColorIdx, MPlayerHouse));

  MPlayerMaxAhead = WChatMaxAhead =
      WWGetPrivateProfileInt("Timing", "MaxAhead", 9, buffer);
  FrameSendRate = WChatSendRate =
      WWGetPrivateProfileInt("Timing", "SendRate", 3, buffer);

  if (name) {
  }
  return 1;
}

/***********************************************************************************************
 * Get_Registry_Sub_Key -- search a registry key for a sub-key *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    handle of key to search * text to search for * true if old key
 *should be closed when new key opened                              *
 *                                                                                             *
 * OUTPUT:   handle to the key we found or 0 *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 1/12/96 2:11PM ST : Created *
 *=============================================================================================*/
#ifdef _WIN32
extern HKEY Get_Registry_Sub_Key(HKEY base_key, char* search_key, BOOL close);
#endif

/***********************************************************************************************
 * Is_User_WChat_Registered -- retrieve the users wchat entry from the registry
 **
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:  Nothing *
 *                                                                                             *
 * OUTPUT:   true if users wchat entry was found in the registry *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 1/12/96 2:13PM ST : Created *
 *=============================================================================================*/
static bool Is_User_WChat_Registered(char* /*buffer*/, int /*buffer_len*/) {
  return false;
}

/***********************************************************************************************
 * Spawn_WChat -- spawns or switches focus to wchat *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    can launch. If set then we are allowed to launch WChat if not
 *already running     *
 *                                                                                             *
 * OUTPUT:   True if wchat was spawned *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 6/8/96 12:33PM ST : Created *
 *=============================================================================================*/
bool Poke_WChat();
bool Spawn_WChat(bool /*can_launch*/) {
  return false;
}

/***********************************************************************************************
 * Spawn_Registration_App -- spawns the C&C/Planet westwood registration app *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   True if app was spawned *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 6/8/96 12:33PM ST : Created *
 *=============================================================================================*/
static bool Spawn_Registration_App() { return false; }

/***********************************************************************************************
 * Do_The_Internet_Menu_Thang -- Handle case where user clicks on 'Internet'
 *button            *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 6/7/96 8:30PM ST : Created *
 *=============================================================================================*/
bool Do_The_Internet_Menu_Thang() {
#ifdef _WIN32
  // The payload WChat expects with a connection-failed notice.
  char packet[10] = {"Hello"};
#endif
#ifndef DEMO

  const int factor = SeenBuff.Get_Width() == 320 ? 1 : 2;

  /*
  ** Dialog & button dimensions
  */
  const int d_dialog_w = 120 * factor;                       // dialog width
  const int d_dialog_h = 80 * factor;                        // dialog height
  const int d_dialog_x = ((320 * factor) - d_dialog_w) / 2;  // dialog x-coord
  const int d_dialog_y = ((200 * factor) - d_dialog_h) / 2;  // centered y-coord
  const int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);     // center x-coord

#if (defined(GERMAN) || defined(FRENCH))
  int d_cancel_w = 50 * factor;
#else
  const int d_cancel_w = 40 * factor;
#endif
  const int d_cancel_h = 9 * factor;
  const int d_cancel_x = d_dialog_cx - (d_cancel_w / 2);
  const int d_cancel_y = d_dialog_y + d_dialog_h - (20 * factor);

#if (defined(GERMAN) || defined(FRENCH))
  int width = 160 * factor;
  int height = 80 * factor;
#else
  int width = 120 * factor;
  int height = 80 * factor;
#endif  // GERMAN | FRENCH

  // Format_Window_String inserts line breaks in place, so format a copy rather
  // than the shared string table.
  char buffer[80 * 3];
  port::SafeCopy(buffer, Text_String(TXT_CONNECTING));
  Fancy_Text_Print(TXT_NONE, 0, 0, kTBlack, kTBlack,
                   TPF_6PT_GRAD | TPF_NOSHADOW);
  Format_Window_String(buffer, SeenBuff.Get_Height(), width, height);

#if (defined(GERMAN) || defined(FRENCH))
  d_dialog_w = width + 25 * factor;
  d_dialog_x = ((320 * factor - d_dialog_w) / 2);  // dialog x-coord
  d_cancel_x = d_dialog_cx - (d_cancel_w / 2);
#endif

  /*
  ** Button Enumerations
  */
  constexpr int kButtonCancel = 100;

  /*
  ** Buttons
  */
  // TextButtonClass *buttons;
  // // button list

  TextButtonClass cancelbtn(
      kButtonCancel, TXT_CANCEL,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
      // #if (GERMAN | FRENCH)
      //		d_cancel_x, d_cancel_y);
      // #else
      d_cancel_x, d_cancel_y, d_cancel_w, d_cancel_h);
  // #endif

  // buttons = &cancelbtn;

  Fancy_Text_Print(TXT_NONE, 0, 0, kCcGreen, kTBlack,
                   TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

  char users_name[256];
  const int buffer_len = sizeof(users_name);
  KeyNumType input = KN_NONE;

  if (!Special.IsFromWChat && !SpawnedFromWChat) {
    /*
    ** If the user is registered with Planet Westwood then spawn WChat.
    */
    if (Is_User_WChat_Registered(users_name, buffer_len)) {
      GameStatisticsPacketSent = false;
      if (!Spawn_WChat(true)) {
        Set_Logic_Page(SeenBuff);
        Load_Title_Page(true);
        Set_Palette(Palette);
        CCMessageBox().Process(TXT_ERROR_UNABLE_TO_RUN_WCHAT, TXT_OK);
        LogicPage->Clear();
        return false;
      }
    } else {
      Set_Logic_Page(SeenBuff);
      Load_Title_Page(true);
      Set_Palette(Palette);
      if (CCMessageBox().Process(TXT_EXPLAIN_REGISTRATION, TXT_REGISTER,
                                 TXT_CANCEL)) {
        LogicPage->Clear();
        return false;
      }
      LogicPage->Clear();
      Spawn_Registration_App();
      return false;
    }
  }

  /*
  **
  ** User is registered and we spawned WChat. Wait for a game start message from
  *WChat.
  **
  */

  bool process = true;
  bool display = true;

  while (process) {
    /*
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      display = true;
    }

    if (display) {
      Set_Logic_Page(SeenBuff);

      Hide_Mouse();
      /*
      ** Redraw backgound & dialog box
      */
      Load_Title_Page(true);
      Set_Palette(Palette);

      Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);

      /*
      ** Dialog & Field labels
      */
      Draw_Caption(TXT_NONE, d_dialog_x, d_dialog_y, d_dialog_w);

      Fancy_Text_Print(buffer, d_dialog_cx - (width / 2),
                       d_dialog_y + (25 * factor), kCcGreen, kTBlack,
                       TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

      // cancelbtn.Zap();
      // buttons = &cancelbtn;

      /*
      .................... Rebuild the button list ....................
      */
      // buttons->Draw_All();
      cancelbtn.Draw_Me(true);

      Show_Mouse();
      display = false;
    }

#ifdef _WIN32
    /*
    ** See if the game start packet has arrived from wchat yet.
    */
    if (DDEServer.Get_MPlayer_Game_Info()) {
      // MessageBox (NULL, "About to restore focus to C&C95", "C&C95", MB_OK);
      // SetForegroundWindow ( MainWindow );
      // ShowWindow ( MainWindow, SW_SHOWMAXIMIZED	);
      return (true);
    }
#endif

    // input = buttons->Input();
    input = cancelbtn.Input();

    /*
    ---------------------------- Process input ----------------------------
    */
    switch (static_cast<int>(input)) {
      /*
      ** Cancel. Just return to the main menu
      */
      case KN_ESC:
      case ButtonKey(kButtonCancel):
        process = false;
#ifdef _WIN32
        Send_Data_To_DDE_Server(packet, strlen(packet),
                                DDEServerClass::DDE_CONNECTION_FAILED);
#endif
        GameStatisticsPacketSent = false;
        Spawn_WChat(false);
        break;
      default:
        break;
    }
  }

#endif  // DEMO

  return false;
}
