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
#include "td/net/internet.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <span>
#include <string>
#include <string_view>

#include "base/strings/number_parse.h"
#include "base/strings/safe_string.h"
#include "engine/file/disk_file.h"
#include "engine/file/disk_stream.h"
#include "engine/file/game_file.h"
#include "engine/file/ini_file.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/wwstd.h"
#include "engine/stream/byte_stream.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/ui/input.h"
#include "td/assets/palette.h"
#include "td/assets/palettes.h"
#include "td/assets/screen.h"
#include "td/core/defines.h"
#include "td/core/jshell.h"
#include "td/core/special_flags.h"
#include "td/data/ini_io.h"
#include "td/data/text.h"
#include "td/map/world.h"
#include "td/net/mplayer.h"
#include "td/net/network.h"
#include "td/net/session.h"
#include "td/net/tcpip.h"
#include "td/screens/goptions.h"
#include "td/startup/conquer.h"
#include "td/startup/game_state.h"
#include "td/startup/init.h"
#include "td/ui/dialog.h"
#include "td/ui/msgbox.h"
#include "td/ui/textbtn.h"

#ifdef _WIN32
#include "td/startup/ccdde.h"
#endif

namespace td {

#ifdef _WIN32
HWND WChatHWND = 0;  // Handle to Wchat window.
#endif

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

  const std::string_view default_string = "Error";
  engine::file::IniFile ini;
  std::unique_ptr<DiskStream> wchat_file;

  /*
  ** Get C&CSPAWN.INI either by reading it from disk or getting it from the DDE
  ** server. Without either the database stays empty and every lookup below
  ** returns "Error".
  */
  if (wchat_name) {
    wchat_file = OpenDiskFile(wchat_name);
    if (wchat_file) {
      LoadIniText(engine::file::ReadIniText(*wchat_file), ini);
    }
  } else {
#ifdef _WIN32
    if (const char* dde_text = DDEServer.Get_MPlayer_Game_Info()) {
      LoadIniText(dde_text, ini);
    }
#endif
  }

  if (!wchat_name || wchat_file) {
    /*
    ** Get the IP address
    */
    const std::string address =
        ini.GetString("Internet", "Address", default_string);

    if (address == default_string) {
      if (wchat_name) {
      }
      return;
    }
    base::SafeCopy(TheNetwork().westwood_address(), address);

    /*
    ** Get the port number
    */
    const std::string port = ini.GetString("Internet", "Port", default_string);

    if (port == default_string) {
      if (wchat_name) {
      }
      return;
    }

    TheNetwork().westwood_port() = base::ParseIntegerOr<int>(port, 0);

    /*
    ** Get host or client
    */
    const std::string host = ini.GetString("Internet", "Host", default_string);

    if (host == default_string) {
      if (wchat_name) {
      }
      return;
    }

    TheNetwork().westwood_is_host() = host.contains('1');

    TheSpecialFlags().from_wchat = true;
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
  engine::file::IniFile ini;

  char filename[256] = {"INVALID.123"};

  if (name) {
    base::SafeCopy(filename, name);
  }

  /*------------------------------------------------------------------------
  Create filename and read the file.
  ------------------------------------------------------------------------*/
  std::unique_ptr<ByteStream> file;
  if (name) {
    file = OpenGameFile(filename);
    if (!file) {
      return 0;
    }
  }
  if (name) {
    LoadIniText(engine::file::ReadIniText(*file), ini);
  } else {
#ifdef _WIN32
    // No packet from the DDE server leaves the database empty, so every
    // setting below takes its default.
    if (const char* dde_text = DDEServer.Get_MPlayer_Game_Info()) {
      LoadIniText(dde_text, ini);
    }
#endif
  }

  /*------------------------------------------------------------------------
  Get the player's name
  ------------------------------------------------------------------------*/
  base::SafeCopy(TheSession().player_name(),
                 ini.GetString("Options", "Handle", "Noname"));
  base::SafeCopy(TheSession().game_name(), TheSession().player_name());
  TheSession().color_index() = ini.GetInt("Options", "Color", 0);
  TheSession().preferred_color() = TheSession().color_index();
  TheSession().house() = static_cast<HousesType>(
      ini.GetInt("Options", "Side", static_cast<int>(HOUSE_GOOD)));

  TheSession().credits() = ini.GetInt("Options", "Credits", 0);
  TheSession().bases() = ini.GetInt("Options", "Bases", 0);
  TheSession().tiberium() = ini.GetInt("Options", "Tiberium", 0);
  TheSession().crates() = ini.GetInt("Options", "Crates", 0);
  TheSession().ghosts() = ini.GetInt("Options", "AI", 0);
  TheWorld().build_level() = ini.GetInt("Options", "BuildLevel", 0);
  TheSession().unit_count() = ini.GetInt("Options", "UnitCount", 0);
  TheWorld().seed() = ini.GetInt("Options", "Seed", 0);
  TheSpecialFlags().capture_the_flag =
      static_cast<unsigned>(ini.GetInt("Options", "CaptureTheFlag", 0));
  // externs.h declares these unsigned long; the INI stores them as ints.
  TheNetwork().westwood_game_id() =
      static_cast<uint32_t>(ini.GetInt("Internet", "GameID", 0));
  TheNetwork().westwood_start_time() =
      static_cast<uint32_t>(ini.GetInt("Internet", "StartTime", 0));

  TheNetwork().internet_max_players() = ini.GetInt("Internet", "MaxPlayers", 2);

  if (TheSession().tiberium()) {
    TheSpecialFlags().tiberium_grows = 1;
    TheSpecialFlags().tiberium_spreads = 1;
  } else {
    TheSpecialFlags().tiberium_grows = 0;
    TheSpecialFlags().tiberium_spreads = 0;
  }
  TheSession().scenario_index() = ini.GetInt("Options", "Scenario", 0);
  TheWorld().scenario() =
      TheSession().scenario_index();  // MPlayerFilenum[ScenarioIdx];

  TheOptions().GameSpeed = 0;

  TheSession().local_id() = static_cast<unsigned char>(
      Build_MPlayerID(TheSession().color_index(), TheSession().house()));

  TheSession().max_ahead() = TheNetwork().chat_max_ahead() =
      ini.GetInt("Timing", "MaxAhead", 9);
  TheSession().frame_send_rate() = TheNetwork().chat_send_rate() =
      ini.GetInt("Timing", "SendRate", 3);

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

  PixelView& view = TheScreen().visible_view();
  const int factor = view.width() == 320 ? 1 : 2;

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
  base::SafeCopy(buffer, Text_String(TXT_CONNECTING));
  Format_Window_String(TextFontStyle(FontType::k6PointGradient, TPF_NOSHADOW),
                       buffer, view.height(), width, height);

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

  TextButtonClass cancelbtn(kButtonCancel, TXT_CANCEL,
                            FontType::k6PointGradient,
                            TPF_CENTER | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                            // #if (GERMAN | FRENCH)
                            //		d_cancel_x, d_cancel_y);
                            // #else
                            d_cancel_x, d_cancel_y, d_cancel_w, d_cancel_h);
  // #endif

  // buttons = &cancelbtn;

  char users_name[256];
  const int buffer_len = sizeof(users_name);
  engine::window::KeyNumber input = engine::window::KN_NONE;

  if (!TheSpecialFlags().from_wchat && !TheGameState().spawned_from_chat()) {
    /*
    ** If the user is registered with Planet Westwood then spawn WChat.
    */
    if (Is_User_WChat_Registered(users_name, buffer_len)) {
      TheNetwork().statistics_sent() = false;
      if (!Spawn_WChat(true)) {
        Load_Title_Page(true);
        Set_Palette(ThePalettes().title_palette());
        CCMessageBox().Process(TXT_ERROR_UNABLE_TO_RUN_WCHAT, TXT_OK);
        view.Clear();
        return false;
      }
    } else {
      Load_Title_Page(true);
      Set_Palette(ThePalettes().title_palette());
      if (CCMessageBox().Process(TXT_EXPLAIN_REGISTRATION, TXT_REGISTER,
                                 TXT_CANCEL)) {
        view.Clear();
        return false;
      }
      view.Clear();
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

    if (display) {
      game::TheMouse().Hide();
      /*
      ** Redraw backgound & dialog box
      */
      Load_Title_Page(true);
      Set_Palette(ThePalettes().title_palette());

      Dialog_Box(view, d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);

      /*
      ** Dialog & Field labels
      */
      Draw_Caption(view, TXT_NONE, d_dialog_x, d_dialog_y, d_dialog_w);

      Fancy_Text_Print(view, buffer, d_dialog_cx - (width / 2),
                       d_dialog_y + (25 * factor), kCcGreen, kTBlack,
                       FontType::k6PointGradient,
                       TPF_USE_GRAD_PAL | TPF_NOSHADOW);

      // cancelbtn.Zap();
      // buttons = &cancelbtn;

      /*
      .................... Rebuild the button list ....................
      */
      // buttons->Draw_All();
      cancelbtn.Draw_Me(view, true);

      game::TheMouse().Show();
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
    input = cancelbtn.Input(view).key;

    /*
    ---------------------------- Process input ----------------------------
    */
    switch (static_cast<int>(input)) {
      /*
      ** Cancel. Just return to the main menu
      */
      case engine::window::KN_ESC:
      case engine::window::ButtonKey(kButtonCancel):
        process = false;
#ifdef _WIN32
        Send_Data_To_DDE_Server(packet, strlen(packet),
                                DDEServerClass::DDE_CONNECTION_FAILED);
#endif
        TheNetwork().statistics_sent() = false;
        Spawn_WChat(false);
        break;
      default:
        break;
    }
  }

#endif  // DEMO

  return false;
}

}  // namespace td
