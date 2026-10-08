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

/* $Header:   F:\projects\c&c\vcs\code\mplayer.cpv   1.9   16 Oct 1995 16:51:08
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : MPLAYER.CPP *
 *                                                                                             *
 *                   Programmer : Bill Randolph *
 *                                                                                             *
 *                   Start Date : April 14, 1995 *
 *                                                                                             *
 *                  Last Update : July 5, 1995 [BRR] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Select_MPlayer_Game -- prompts user for NULL-Modem, Modem, or
 *Network game                * Read_MultiPlayer_Settings -- reads multi-player
 *settings from conquer.ini                 * Write_MultiPlayer_Settings --
 *writes multi-player settings to conquer.ini                 *
 *   Read_Scenario_Descriptions -- reads multi-player scenario #'s #
 *descriptions              * Free_Scenario_Descriptions -- frees memory for the
 *scenario descriptions                  * Computer_Message -- "sends" a message
 *from the computer                                   * Garble_Message --
 *"garbles" a message                                                     *
 *   Surrender_Dialog -- Prompts user for surrendering *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/net/mplayer.h"

#include <algorithm>
#include <bit>
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

#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/enum_array.h"
#include "base/strings/number_parse.h"
#include "base/strings/safe_string.h"
#include "base/strings/tokenizer.h"
#include "engine/file/game_file.h"
#include "engine/file/ini_file.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/net/ipxmgr.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/sim/enums.h"
#include "game/ui/control.h"
#include "game/ui/input.h"
#include "game/ui/standard_colors.h"
#include "td/assets/screen.h"
#include "td/core/defines.h"
#include "td/core/rand.h"
#include "td/core/randomstate.h"
#include "td/core/special_flags.h"
#include "td/data/ini.h"
#include "td/data/ini_io.h"
#include "td/data/text.h"
#include "td/debug/debug_state.h"
#include "td/editor/mapedit.h"
#include "td/map/world.h"
#include "td/net/network.h"
#include "td/net/nulldlg.h"
#include "td/net/phone.h"
#include "td/net/session.h"
#include "td/objects/house.h"
#include "td/screens/goptions.h"
#include "td/startup/conquer.h"
#include "td/startup/init.h"
#include "td/ui/dialog.h"
#include "td/ui/msglist.h"
#include "td/ui/textbtn.h"

namespace td {

using enum engine::window::KeyNumber;

static void Garble_Message(std::span<char> buf);

// The modem dial prefix for each dial method: tone or pulse.
static constexpr base::EnumArray<DialMethodType, const char*, kDialMethods>
    kDialMethodCheck = {"T", "P"};

// The names of the multiplayer scenarios; MPlayerScenarios points into it.
static char mplayer_descriptions[100][40];

int Choose_Internet_Game();
int Get_Internet_Host_Or_Join();
int Get_IP_Address();
void Show_Internet_Connection_Progress();

/***********************************************************************************************
 * Select_MPlayer_Game -- prompts user for NULL-Modem, Modem, or Network game *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * GAME_NORMAL, GAME_MODEM, etc. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 02/14/1995 BR : Created. *
 *=============================================================================================*/
GameType Select_MPlayer_Game() {
  const int factor = TheScreen().visible_view().width() == 320 ? 1 : 2;
  bool ipx_avail = false;
  /*........................................................................
  Dialog & button dimensions
  ........................................................................*/
  const int d_dialog_w = 190 * factor;
  const int d_dialog_h = 26 * 4 * factor;
  const int d_dialog_x = ((320 * factor) - d_dialog_w) / 2;
  //	d_dialog_y = ((200 - d_dialog_h) / 2),
  const int d_dialog_y = ((136 * factor) - d_dialog_h) / 2;
  const int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);

  const int d_txt6_h = 11 * factor;
  const int d_margin = 7 * factor;

  const int d_modemserial_w = 80 * factor;
  const int d_modemserial_h = 9 * factor;
  const int d_modemserial_x = d_dialog_cx - (d_modemserial_w / 2);
  const int d_modemserial_y = d_dialog_y + d_margin + d_txt6_h + d_margin;
  const int d_ipx_w = 80 * factor;
  const int d_ipx_h = 9 * factor;
  const int d_ipx_x = d_dialog_cx - (d_ipx_w / 2);
  const int d_ipx_y = d_modemserial_y + d_modemserial_h + (2 * factor);
  //	int 	d_ipx_y = d_internet_y + d_internet_h + 2*factor;

  const int d_cancel_w = 60 * factor;
  const int d_cancel_h = 9 * factor;
  const int d_cancel_x = d_dialog_cx - (d_cancel_w / 2);
  const int d_cancel_y = d_ipx_y + d_ipx_h + d_margin;

  /*........................................................................
  Button enumerations:
  ........................................................................*/
  constexpr int kButtonModemserial = 100;
  constexpr int kButtonIpx = 101;
  constexpr int kButtonCancel = 102;

  constexpr int kNumOfButtons = 3;
  int number_of_buttons = kNumOfButtons;
  /*........................................................................
  Redraw values: in order from "top" to "bottom" layer of the dialog
  ........................................................................*/
  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_BUTTONS = 1,     // includes map interior & coord values
    REDRAW_BACKGROUND = 2,  // includes box, map bord, key, coord labels, btns
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;
  /*........................................................................
  Dialog variables:
  ........................................................................*/
  GameType retval = GAME_NORMAL;  // return value
  int selection = 0;
  TextButtonClass* buttons[kNumOfButtons];

  /*........................................................................
  Buttons
  ........................................................................*/
  game::ControlClass* commands = nullptr;  // the button list

  //
  // If neither IPX or winsock are active then do only the modem serial dialog
  //
  if (TheNetwork().ipx().Is_IPX()) {
    ipx_avail = true;
  }

  TextButtonClass modemserialbtn(
      kButtonModemserial, TXT_MODEM_SERIAL, FontType::k6PointGradient,
      TPF_CENTER | TPF_USE_GRAD_PAL | TPF_NOSHADOW, d_modemserial_x,
      d_modemserial_y, d_modemserial_w, d_modemserial_h);
  TextButtonClass ipxbtn(kButtonIpx, TXT_NETWORK, FontType::k6PointGradient,
                         TPF_CENTER | TPF_USE_GRAD_PAL | TPF_NOSHADOW, d_ipx_x,
                         d_ipx_y, d_ipx_w, d_ipx_h);

  TextButtonClass cancelbtn(kButtonCancel, TXT_CANCEL,
                            FontType::k6PointGradient,
                            TPF_CENTER | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                            d_cancel_x, d_cancel_y, d_cancel_w, d_cancel_h);

  /*
  ------------------------------- Initialize -------------------------------
  */
  PixelView& view = TheScreen().visible_view();

  /*
  ............................ Create the list .............................
  */
  commands = &modemserialbtn;
  if (ipx_avail) {
    ipxbtn.Add_Tail(*commands);
  }
  cancelbtn.Add_Tail(*commands);

  /*
  ......................... Fill array of button ptrs ......................
  */
  int curbutton = 0;
  base::At(buttons, 0) = &modemserialbtn;
  if (ipx_avail) {
    base::At(buttons, 1) = &ipxbtn;
    base::At(buttons, 2) = &cancelbtn;
  } else {
    base::At(buttons, 1) = &cancelbtn;
    number_of_buttons--;
  }

  base::At(buttons, curbutton)->Turn_On();

  game::TheKeyboard().Clear();

  /*
  -------------------------- Main Processing Loop --------------------------
  */
  RedrawType display = REDRAW_ALL;  // true = re-draw everything
  bool process = true;              // loop while true
  bool pressed = false;
  while (process) {

    /*
    ........................ Invoke game callback .........................
    */
    Call_Back();
    /*
    ...................... Refresh display if needed ......................
    */
    if (display != REDRAW_NONE) {
      game::TheMouse().Hide();
      if (display >= REDRAW_BACKGROUND) {
        /*
        ..................... Refresh the backdrop ......................
        */
        Load_Title_Page(true);
        /*
        ..................... Draw the background .......................
        */
        Dialog_Box(view, d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);
        Draw_Caption(view, TXT_SELECT_MPLAYER_GAME, d_dialog_x, d_dialog_y,
                     d_dialog_w);
      }
      /*
      .......................... Redraw buttons ..........................
      */
      if (display >= REDRAW_BUTTONS) {
        commands->Flag_List_To_Redraw();
      }
      game::TheMouse().Show();
      display = REDRAW_NONE;
    }

    /*
    ........................... Get user input ............................
    */
    const engine::window::KeyNumber input =
        commands->Input(view).key;  // input from user

    /*
    ............................ Process input ............................
    */
    switch (static_cast<int>(input)) {
      case engine::window::ButtonKey(kButtonModemserial):
        selection = kButtonModemserial;
        pressed = true;
        break;

      case engine::window::ButtonKey(kButtonIpx):
        selection = kButtonIpx;
        pressed = true;
        break;

      case KN_ESC:
      case engine::window::ButtonKey(kButtonCancel):
        selection = kButtonCancel;
        pressed = true;
        break;

      case KN_UP:
        base::At(buttons, curbutton)->Turn_Off();
        base::At(buttons, curbutton)->Flag_To_Redraw();
        curbutton--;
        if (curbutton < 0) {
          curbutton = number_of_buttons - 1;
        }
        base::At(buttons, curbutton)->Turn_On();
        base::At(buttons, curbutton)->Flag_To_Redraw();
        break;

      case KN_DOWN:
        base::At(buttons, curbutton)->Turn_Off();
        base::At(buttons, curbutton)->Flag_To_Redraw();
        curbutton++;
        if (curbutton > number_of_buttons - 1) {
          curbutton = 0;
        }
        base::At(buttons, curbutton)->Turn_On();
        base::At(buttons, curbutton)->Flag_To_Redraw();
        break;

      case KN_RETURN:
        // Read the id off the button itself. The array skips the IPX button
        // when IPX is unavailable, so ids and indices are not interchangeable.
        selection = static_cast<int>(base::At(buttons, curbutton)->ID);
        pressed = true;
        break;

      default:
        break;
    }

    if (pressed) {
      //
      // to make sure the selection is correct in case they used the mouse
      //
      base::At(buttons, curbutton)->Turn_Off();
      base::At(buttons, curbutton)->Flag_To_Redraw();
      for (int index = 0; index < number_of_buttons; index++) {
        if (std::cmp_equal(base::At(buttons, index)->ID, selection)) {
          curbutton = index;
          break;
        }
      }
      base::At(buttons, curbutton)->Turn_On();
      //			buttons[curbutton]->Flag_To_Redraw();
      base::At(buttons, curbutton)->IsPressed = true;
      base::At(buttons, curbutton)->Draw_Me(view, true);

      switch (selection) {
        case kButtonModemserial:

          //
          // Pop up the modem/serial/com port dialog
          //
          retval = Select_Serial_Dialog();

          if (retval != GAME_NORMAL) {
            process = false;
          } else {
            base::At(buttons, curbutton)->IsPressed = false;
            display = REDRAW_ALL;
          }
          break;

        case kButtonIpx:
          retval = GAME_IPX;
          process = false;
          break;

        case kButtonCancel:
          retval = GAME_NORMAL;
          process = false;
          break;
        default:
          break;
      }

      pressed = false;
    }
  }
  return retval;
}

/***********************************************************************************************
 * Read_MultiPlayer_Settings -- reads multi-player settings from conquer.ini *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 02/14/1995 BR : Created. *
 *=============================================================================================*/
void Read_MultiPlayer_Settings() {
  /*------------------------------------------------------------------------
  Create filename and read the file. Without one the lists stay empty and
  everything else keeps its value.
  ------------------------------------------------------------------------*/
  engine::file::IniFile ini;
  if (!LoadIniFile("CONQUER.INI", ini)) {
    TheNetwork().init_strings().clear();
    TheNetwork().phone_book().clear();
    return;
  }
  ReadMultiPlayerSettings(ini);
}

// Returns `index` if it selects an init string (-1 means send none), else 0:
// the file may name a string the list lost, and the dialer indexes the list
// with it. The list is never empty here, ReadMultiPlayerSettings() adds "ATZ".
static int ValidInitStringIndex(const int index) {
  if (index == -1 ||
      (index >= 0 && index < std::ssize(TheNetwork().init_strings()))) {
    return index;
  }
  return 0;
}

void ReadMultiPlayerSettings(const engine::file::IniFile& ini) {
  int i = 0;

  /*------------------------------------------------------------------------
  Clear the initstring entries
  ------------------------------------------------------------------------*/
  TheNetwork().init_strings().clear();

  /*------------------------------------------------------------------------
  Clear the dialing entries
  ------------------------------------------------------------------------*/
  TheNetwork().phone_book().clear();

  if (!TheSpecialFlags().from_wchat) {
    /*------------------------------------------------------------------------
    Get the player's last-used Handle
    ------------------------------------------------------------------------*/
    base::SafeCopy(TheSession().player_name(),
                   ini.GetString("MultiPlayer", "Handle", "Noname"));

    /*------------------------------------------------------------------------
    Get the player's last-used Color
    ------------------------------------------------------------------------*/
    TheSession().preferred_color() = ini.GetInt("MultiPlayer", "Color", 0);
    TheSession().house() = static_cast<HousesType>(
        ini.GetInt("MultiPlayer", "Side", static_cast<int>(HOUSE_GOOD)));
    TheNetwork().current_phone_index() =
        ini.GetInt("MultiPlayer", "PhoneIndex", -1);
  } else {
    TheNetwork().current_phone_index() = -1;
  }

  TheDebugState().trap_check_heap() = ini.GetInt("MultiPlayer", "CheckHeap", 0);

  /*------------------------------------------------------------------------
  Read in default serial settings
  ------------------------------------------------------------------------*/
  base::SafeCopy(std::span(TheNetwork().serial_defaults().ModemName)
                     .first(static_cast<std::size_t>(MODEM_NAME_MAX)),
                 ini.GetString("SerialDefaults", "ModemName", "NoName"));
  if ((std::string_view(TheNetwork().serial_defaults().ModemName) ==
       "NoName")) {
    base::At(TheNetwork().serial_defaults().ModemName, 0) = 0;
  }
  if (const auto value =
          base::ParseHex<int>(ini.GetString("SerialDefaults", "Port", "0"))) {
    TheNetwork().serial_defaults().Port = *value;
  }
  TheNetwork().serial_defaults().IRQ = ini.GetInt("SerialDefaults", "IRQ", -1);
  TheNetwork().serial_defaults().Baud =
      ini.GetInt("SerialDefaults", "Baud", -1);
  TheNetwork().serial_defaults().Init =
      ini.GetInt("SerialDefaults", "Init", 0) != 0;
  TheNetwork().serial_defaults().Compression =
      ini.GetInt("SerialDefaults", "Compression", 0) != 0;
  TheNetwork().serial_defaults().ErrorCorrection =
      ini.GetInt("SerialDefaults", "ErrorCorrection", 0) != 0;
  TheNetwork().serial_defaults().HardwareFlowControl =
      ini.GetInt("SerialDefaults", "HardwareFlowControl", 1) != 0;
  // Only the first character names the method.
  const std::string dial_method =
      ini.GetString("SerialDefaults", "DialMethod", "T").substr(0, 1);

  // find dial method

  for (i = 0; i < kDialMethods; i++) {
    if (absl::EqualsIgnoreCase(
            dial_method, kDialMethodCheck.at(static_cast<DialMethodType>(i)))) {
      TheNetwork().serial_defaults().DialMethod =
          static_cast<DialMethodType>(i);
      break;
    }
  }

  // if method not found set to touch tone

  if (i == kDialMethods) {
    TheNetwork().serial_defaults().DialMethod = DIAL_TOUCH_TONE;
  }

  TheNetwork().serial_defaults().InitStringIndex =
      ini.GetInt("SerialDefaults", "InitStringIndex", 0);

  TheNetwork().serial_defaults().CallWaitStringIndex =
      ini.GetInt("SerialDefaults", "CallWaitStringIndex", kCallWaitCustom);

  base::SafeCopy(std::span(TheNetwork().serial_defaults().CallWaitString)
                     .first(static_cast<std::size_t>(CWAITSTRBUF_MAX)),
                 ini.GetString("SerialDefaults", "CallWaitString", ""));

  if (TheNetwork().serial_defaults().IRQ == 0 ||
      TheNetwork().serial_defaults().Baud == 0) {
    TheNetwork().serial_defaults().Port = 0;
    TheNetwork().serial_defaults().IRQ = -1;
    TheNetwork().serial_defaults().Baud = -1;
  }

  /*------------------------------------------------------------------------
  Read in & store each entry, in the order the file lists them
  ------------------------------------------------------------------------*/
  for (const engine::file::IniEntry& init : ini.Entries("InitStrings")) {
    auto entry = std::make_unique<char[]>(INITSTRBUF_MAX);
    // This allocation owns exactly INITSTRBUF_MAX writable characters.
    // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
    const std::span<char> entry_storage(entry.get(), INITSTRBUF_MAX);

    base::SafeCopy(entry_storage, init.value);

    std::ranges::transform(base::MutableCString(entry.get()), entry.get(),
                           absl::ascii_toupper);

    TheNetwork().init_strings().push_back(std::move(entry));
  }

  // if no entries then have at least one

  if (TheNetwork().init_strings().empty()) {
    auto entry = std::make_unique<char[]>(INITSTRBUF_MAX);
    // This allocation owns exactly INITSTRBUF_MAX writable characters.
    // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
    const std::span<char> entry_storage(entry.get(), INITSTRBUF_MAX);
    base::SafeCopy(entry_storage, "ATZ");
    TheNetwork().init_strings().push_back(std::move(entry));
    TheNetwork().serial_defaults().InitStringIndex = 0;
  }
  TheNetwork().serial_defaults().InitStringIndex =
      ValidInitStringIndex(TheNetwork().serial_defaults().InitStringIndex);

  /*------------------------------------------------------------------------
  Read in all phone book listings, in the order the file lists them.
  Format: Name=PhoneNum,Port,IRQ,Baud,InitString
  ------------------------------------------------------------------------*/
  for (const engine::file::IniEntry& book_entry : ini.Entries("PhoneBook")) {
    /*.....................................................................
    Create a new phone book entry
    .....................................................................*/
    auto phone = std::make_unique<PhoneEntryClass>();  // a phone book entry

    /*.....................................................................
    Read the entire entry in
    .....................................................................*/
    const std::string_view listing = book_entry.value;

    /*.....................................................................
    Extract name, phone # & serial port settings
    .....................................................................*/
    base::Tokenizer tokens(listing, "|");
    std::string_view tokenptr = tokens.Next();  // current field
    if (!tokenptr.empty()) {
      base::SafeCopy(phone->Name, tokenptr);
      std::ranges::transform(base::MutableCString(phone->Name), phone->Name,
                             absl::ascii_toupper);
    } else {
      base::At(phone->Name, 0) = 0;
    }

    tokenptr = tokens.Next();
    if (!tokenptr.empty()) {
      base::SafeCopy(phone->Number, tokenptr);
      std::ranges::transform(base::MutableCString(phone->Number), phone->Number,
                             absl::ascii_toupper);
    } else {
      base::At(phone->Number, 0) = 0;
    }

    tokenptr = tokens.Next();
    if (!tokenptr.empty()) {
      if (const auto value = base::ParseHex<int>(tokenptr)) {
        phone->Settings.Port = *value;
      }
    } else {
      phone->Settings.Port = 0;
    }

    tokenptr = tokens.Next();
    if (!tokenptr.empty()) {
      phone->Settings.IRQ = base::ParseIntegerOr<int>(tokenptr, 0);
    } else {
      phone->Settings.IRQ = -1;
    }

    tokenptr = tokens.Next();
    if (!tokenptr.empty()) {
      phone->Settings.Baud = base::ParseIntegerOr<int>(tokenptr, 0);
    } else {
      phone->Settings.Baud = -1;
    }

    tokenptr = tokens.Next();
    if (!tokenptr.empty()) {
      phone->Settings.Compression = base::ParseIntegerOr<int>(tokenptr, 0) != 0;
    } else {
      phone->Settings.Compression = false;
    }

    tokenptr = tokens.Next();
    if (!tokenptr.empty()) {
      phone->Settings.ErrorCorrection =
          base::ParseIntegerOr<int>(tokenptr, 0) != 0;
    } else {
      phone->Settings.ErrorCorrection = false;
    }

    tokenptr = tokens.Next();
    if (!tokenptr.empty()) {
      phone->Settings.HardwareFlowControl =
          base::ParseIntegerOr<int>(tokenptr, 0) != 0;
    } else {
      phone->Settings.HardwareFlowControl = true;
    }

    tokenptr = tokens.Next();
    if (!tokenptr.empty()) {
      // find dial method

      for (i = 0; i < kDialMethods; i++) {
        if (absl::EqualsIgnoreCase(
                tokenptr,
                kDialMethodCheck.at(static_cast<DialMethodType>(i)))) {
          phone->Settings.DialMethod = static_cast<DialMethodType>(i);
          break;
        }
      }

      // if method not found set to touch tone

      if (i == kDialMethods) {
        phone->Settings.DialMethod = DIAL_TOUCH_TONE;
      }
    } else {
      phone->Settings.DialMethod = DIAL_TOUCH_TONE;
    }

    tokenptr = tokens.Next();
    if (!tokenptr.empty()) {
      phone->Settings.InitStringIndex =
          ValidInitStringIndex(base::ParseIntegerOr<int>(tokenptr, 0));
    } else {
      phone->Settings.InitStringIndex = 0;
    }

    tokenptr = tokens.Next();
    if (!tokenptr.empty()) {
      phone->Settings.CallWaitStringIndex =
          base::ParseIntegerOr<int>(tokenptr, 0);
    } else {
      phone->Settings.CallWaitStringIndex = kCallWaitCustom;
    }

    tokenptr = tokens.Next();
    if (!tokenptr.empty()) {
      base::SafeCopy(phone->Settings.CallWaitString, tokenptr);
    } else {
      base::At(phone->Settings.CallWaitString, 0) = 0;
    }

    /*.....................................................................
    Add it to our list
    .....................................................................*/
    TheNetwork().phone_book().push_back(std::move(phone));
  }

  /*------------------------------------------------------------------------
  Read special recording playback values, to help find sync bugs
  ------------------------------------------------------------------------*/
  if (TheSession().playback_game()) {
    TheDebugState().trap_frame() = ini.GetInt("SyncBug", "Frame", 0x7fffffff);

    TheDebugState().trap_object_type() = static_cast<RTTIType>(
        ini.GetInt("SyncBug", "Type", static_cast<int>(RTTI_NONE)));
    const std::string type_name = ini.GetString("SyncBug", "Type", "NONE");
    if (absl::EqualsIgnoreCase(type_name, "AIRCRAFT")) {
      TheDebugState().trap_object_type() = RTTI_AIRCRAFT;
    } else if (absl::EqualsIgnoreCase(type_name, "ANIM")) {
      TheDebugState().trap_object_type() = RTTI_ANIM;
    } else if (absl::EqualsIgnoreCase(type_name, "BUILDING")) {
      TheDebugState().trap_object_type() = RTTI_BUILDING;
    } else if (absl::EqualsIgnoreCase(type_name, "BULLET")) {
      TheDebugState().trap_object_type() = RTTI_BULLET;
    } else if (absl::EqualsIgnoreCase(type_name, "INFANTRY")) {
      TheDebugState().trap_object_type() = RTTI_INFANTRY;
    } else if (absl::EqualsIgnoreCase(type_name, "UNIT")) {
      TheDebugState().trap_object_type() = RTTI_UNIT;
    } else {
      TheDebugState().trap_object_type() = RTTI_NONE;
    }

    TheDebugState().trap_coord() = Coordinate::FromPacked(
        base::ParseHexOr<uint32_t>(ini.GetString("SyncBug", "Coord", "0"), 0));

    if (const auto trap_this =
            base::ParseHex<uintptr_t>(ini.GetString("SyncBug", "this", "0"))) {
      TheDebugState().trap_this() = std::bit_cast<void*>(*trap_this);
    }
  }
}

/***********************************************************************************************
 * Write_MultiPlayer_Settings -- writes multi-player settings to conquer.ini *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 02/14/1995 BR : Created. *
 *=============================================================================================*/
void Write_MultiPlayer_Settings() {
  // The file may hold entries and sections this routine does not write, which
  // loading it first carries over.
  engine::file::IniFile ini;
  LoadIniFile("CONQUER.INI", ini);
  WriteMultiPlayerSettings(ini);
  SaveIniFile("CONQUER.INI", ini);
}

void WriteMultiPlayerSettings(engine::file::IniFile& ini) {
  /*------------------------------------------------------------------------
  Save the player's last-used Handle & Color
  ------------------------------------------------------------------------*/
  ini.PutInt("MultiPlayer", "PhoneIndex", TheNetwork().current_phone_index());
  ini.PutInt("MultiPlayer", "Color", TheSession().preferred_color());
  ini.PutInt("MultiPlayer", "Side", static_cast<int>(TheSession().house()));
  // An empty Handle drops the entry, and a missing one reads as "Noname", as
  // the empty one the old writer stored did.
  ini.PutString("MultiPlayer", "Handle", TheSession().player_name());

  /*------------------------------------------------------------------------
  Clear all existing SerialDefault entries.
  ------------------------------------------------------------------------*/
  ini.Clear("SerialDefaults");

  /*------------------------------------------------------------------------
  Save default serial settings. A text left empty is not stored; the reader's
  default for it is the empty text, which is what it would have read.
  ------------------------------------------------------------------------*/
  const SerialSettingsType& serial = TheNetwork().serial_defaults();
  ini.PutString("SerialDefaults", "CallWaitString", serial.CallWaitString);
  ini.PutInt("SerialDefaults", "CallWaitStringIndex",
             serial.CallWaitStringIndex);
  ini.PutInt("SerialDefaults", "InitStringIndex", serial.InitStringIndex);
  ini.PutInt("SerialDefaults", "Init", serial.Init ? 1 : 0);
  ini.PutString("SerialDefaults", "DialMethod",
                kDialMethodCheck.at(serial.DialMethod));
  ini.PutInt("SerialDefaults", "Baud", serial.Baud);
  ini.PutInt("SerialDefaults", "IRQ", serial.IRQ);
  ini.PutString("SerialDefaults", "Port",
                absl::StrFormat("%x", static_cast<unsigned int>(serial.Port)));
  ini.PutString("SerialDefaults", "ModemName", serial.ModemName);
  ini.PutInt("SerialDefaults", "Compression", serial.Compression ? 1 : 0);
  ini.PutInt("SerialDefaults", "ErrorCorrection",
             serial.ErrorCorrection ? 1 : 0);
  ini.PutInt("SerialDefaults", "HardwareFlowControl",
             serial.HardwareFlowControl ? 1 : 0);

  /*------------------------------------------------------------------------
  Clear all existing InitString entries.
  ------------------------------------------------------------------------*/
  ini.Clear("InitStrings");

  /*------------------------------------------------------------------------
  Save all InitString entries, which land in the file in the order written.
  The list never holds an empty string (the dialog refuses to add one and the
  reader drops one), so the indexes into it stay valid.
  ------------------------------------------------------------------------*/
  int stored = 0;
  for (const auto& init_string : TheNetwork().init_strings()) {
    ini.PutString("InitStrings", absl::StrFormat("%03d", stored++),
                  init_string.get());
  }

  /*------------------------------------------------------------------------
  Clear all existing Phone Book entries.
  ------------------------------------------------------------------------*/
  ini.Clear("PhoneBook");

  /*------------------------------------------------------------------------
  Save all Phone Book entries, in the order of the book.
  Format: Entry=Name|PhoneNum|Port|IRQ|Baud|Compression|ErrorCorrection|
          HardwareFlowControl|DialMethod|InitStringIndex|CallWaitStringIndex|
          CallWaitString
  ------------------------------------------------------------------------*/
  int entry_number = 0;
  for (const auto& entry : TheNetwork().phone_book()) {
    const PhoneEntryClass& phone = *entry;
    ini.PutString(
        "PhoneBook", absl::StrFormat("%03d", entry_number++),
        absl::StrFormat(
            "%s|%s|%x|%d|%d|%d|%d|%d|%s|%d|%d|%s", phone.Name, phone.Number,
            static_cast<unsigned int>(phone.Settings.Port), phone.Settings.IRQ,
            phone.Settings.Baud, phone.Settings.Compression ? 1 : 0,
            phone.Settings.ErrorCorrection ? 1 : 0,
            phone.Settings.HardwareFlowControl ? 1 : 0,
            kDialMethodCheck.at(phone.Settings.DialMethod),
            phone.Settings.InitStringIndex, phone.Settings.CallWaitStringIndex,
            phone.Settings.CallWaitString));
  }
}

/***********************************************************************************************
 * Read_Scenario_Descriptions -- reads multi-player scenario #'s # descriptions
 **
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 02/14/1995 BR : Created. *
 *=============================================================================================*/
void Read_Scenario_Descriptions() {
  char fname[20];

  /*------------------------------------------------------------------------
  Clear the scenario description lists
  ------------------------------------------------------------------------*/
  TheSession().scenarios().clear();
  TheSession().scenario_files().clear();

  /*------------------------------------------------------------------------
  Loop through all possible scenario numbers; if a file is available, add
  its number to the FileNum list.
  ------------------------------------------------------------------------*/
  for (int i = 0; i < 100; i++) {
    Set_Scenario_Name(TheWorld().scenario_name(), i, SCEN_PLAYER_MPLAYER,
                      game::SCEN_DIR_EAST, SCEN_VAR_A);
    absl::SNPrintF(fname, sizeof(fname), "%s.INI", TheWorld().scenario_name());

    if (GameFileExists(fname)) {
      TheSession().scenario_files().push_back(i);
    }
  }

  /*------------------------------------------------------------------------
  Now, for every file in the FileNum list, read in the INI file, and extract
  its description.
  ------------------------------------------------------------------------*/
  for (int i = 0; i < std::ssize(TheSession().scenario_files()); i++) {
    /*.....................................................................
    Create filename and read the file.
    .....................................................................*/
    Set_Scenario_Name(TheWorld().scenario_name(),
                      base::At(TheSession().scenario_files(), i),
                      SCEN_PLAYER_MPLAYER, game::SCEN_DIR_EAST, SCEN_VAR_A);
    absl::SNPrintF(fname, sizeof(fname), "%s.INI", TheWorld().scenario_name());
    engine::file::IniFile scenario;
    LoadIniFile(fname, scenario);

    /*.....................................................................
    Extract description & add it to the list.
    .....................................................................*/
    base::SafeCopy(base::At(mplayer_descriptions, i),
                   scenario.GetString("Basic", "Name", "Nulls-Ville"));
    TheSession().scenarios().push_back(base::At(mplayer_descriptions, i));
  }
}

/***********************************************************************************************
 * Free_Scenario_Descriptions -- frees memory for the scenario descriptions *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 06/05/1995 BRR : Created. *
 *=============================================================================================*/
void Free_Scenario_Descriptions() {

  /*------------------------------------------------------------------------
  Clear the scenario descriptions & filenames
  ------------------------------------------------------------------------*/
  TheSession().scenarios().clear();
  TheSession().scenario_files().clear();

  /*------------------------------------------------------------------------
  Clear the initstring entries
  ------------------------------------------------------------------------*/
  TheNetwork().init_strings().clear();

  /*------------------------------------------------------------------------
  Clear the dialing entries
  ------------------------------------------------------------------------*/
  TheNetwork().phone_book().clear();
}

/***************************************************************************
 * Computer_Message -- "sends" a message from the computer                 *
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
 *   06/06/1995 BRR : Created.                                             *
 *=========================================================================*/
void Computer_Message() {
  char txt[160];

  /*------------------------------------------------------------------------
  Find the computer house that the message will be from
  ------------------------------------------------------------------------*/
  for (HousesType house = HOUSE_MULTI1;
       static_cast<int>(house) <
       static_cast<int>(HOUSE_MULTI1) + TheSession().max_players();
       house++) {
    HouseClass* ptr = HouseClass::As_Pointer(house);

    if (!ptr || ptr->IsHuman || ptr->IsDefeated) {
      continue;
    }

    /*.....................................................................
    Decode this house's color
    .....................................................................*/
    const int color =
        base::At(TheSession().text_colors(), static_cast<int>(ptr->RemapColor));

    /*.....................................................................
    We now have a 1/4 chance of echoing one of the human players' messages
    back.
    .....................................................................*/
    if (GameRandomRange(0, 3) == 2) {
      /*..................................................................
      Now we have a 1/3 chance of garbling the human message.
      ..................................................................*/
      if (GameRandomRange(0, 2) == 1) {
        Garble_Message(TheSession().last_message());
      }

      /*..................................................................
      Only add the message if there is one to add.
      ..................................................................*/
      if (!std::string_view(TheSession().last_message()).empty()) {
        absl::SNPrintF(txt, sizeof(txt), "%s %s",
                       Text_String(TXT_FROM_COMPUTER),
                       TheSession().last_message());
        TheSession().messages().Add_Message(
            txt, color, FontType::k6PointGradient,
            TPF_USE_GRAD_PAL | TPF_FULLSHADOW, 600, 0, 0);
      }
    } else {
      absl::SNPrintF(txt, sizeof(txt), "%s %s", Text_String(TXT_FROM_COMPUTER),
                     Text_String(TXT_COMP_MSG1 + GameRandomRange(0, 12)));
      TheSession().messages().Add_Message(txt, color, FontType::k6PointGradient,
                                          TPF_USE_GRAD_PAL | TPF_FULLSHADOW,
                                          600, 0, 0);
    }

    return;
  }
}

/***************************************************************************
 * Garble_Message -- "garbles" a message                                   *
 *                                                                         *
 * INPUT:                                                                  *
 *      buf      buffer to garble; stores output message                   *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/06/1995 BRR : Created.                                             *
 *=========================================================================*/
static void Garble_Message(std::span<char> buf) {
  char txt[80];
  char punct[20];   // for punctuation
  std::string_view words[40];  // the words of the phrase; empty once used

  /*------------------------------------------------------------------------
  Pull off any trailing punctuation
  ------------------------------------------------------------------------*/
  const std::string_view message(buf.data());
  size_t punctuation = message.size();
  while (punctuation > 0 && message.size() - punctuation < sizeof(punct) - 1) {
    const char ch = message.at(punctuation - 1);
    if (ch != '!' && ch != '.' && ch != '?') {
      break;
    }
    --punctuation;
  }
  base::SafeCopy(punct, message.substr(punctuation));
  base::At(buf, punctuation) = '\0';

  /*------------------------------------------------------------------------
  Copy the original buffer
  ------------------------------------------------------------------------*/
  base::SafeCopy(txt, buf.data());

  /*------------------------------------------------------------------------
  Split it up into words
  ------------------------------------------------------------------------*/
  base::Tokenizer tokens(txt, " ");
  std::string_view p = tokens.Next();
  int numwords = 0;  // # words in the phrase
  while (!p.empty()) {
    base::At(words, numwords) = p;
    numwords++;
    p = tokens.Next();
  }

  /*------------------------------------------------------------------------
  Now randomly put the words back.  Don't use the real random-number
  generator, since different machines will have different LastMessage's,
  and will go out of sync.
  ------------------------------------------------------------------------*/
  base::At(buf, 0) = 0;
  for (int i = 0; i < numwords; i++) {
    const int j = Sim_IRandom(0, numwords);
    if (base::At(words, j).empty()) {  // this word has been used already
      i--;
      continue;
    }
    base::SafeAppend(std::span(buf).first(MAX_MESSAGE_LENGTH),
                     base::At(words, j));
    base::At(words, j) = {};
    if (i < numwords - 1) {
      base::SafeAppend(std::span(buf).first(MAX_MESSAGE_LENGTH), " ");
    }
  }
  base::SafeAppend(std::span(buf).first(MAX_MESSAGE_LENGTH), punct);
}

/***************************************************************************
 * Surrender_Dialog -- Prompts user for surrendering                       *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      0 = user cancels, 1 = user wants to surrender.                     *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   07/05/1995 BRR : Created.                                             *
 *=========================================================================*/
int Surrender_Dialog() {
  const int factor = TheScreen().visible_view().width() == 320 ? 1 : 2;
  /*........................................................................
  Dialog & button dimensions
  ........................................................................*/
  const int d_dialog_w = 170 * factor;                       // dialog width
  const int d_dialog_h = 53 * factor;                        // dialog height
  const int d_dialog_x = ((320 * factor) - d_dialog_w) / 2;  // centered x-coord
  const int d_dialog_y = ((200 * factor) - d_dialog_h) / 2;  // centered y-coord
  const int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);  // coord of x-center

  const int d_margin = 5 * factor;      // margin width/height
  const int d_topmargin = 20 * factor;  // top margin

  const int d_ok_w = 45 * factor;                                  // ok width
  const int d_ok_h = 9 * factor;                                   // ok height
  const int d_ok_x = d_dialog_cx - d_ok_w - (5 * factor);          // ok x
  const int d_ok_y = d_dialog_y + d_dialog_h - d_ok_h - d_margin;  // ok y

  const int d_cancel_w = 45 * factor;                 // cancel width
  const int d_cancel_h = 9 * factor;                  // cancel height
  const int d_cancel_x = d_dialog_cx + (5 * factor);  // cancel x
  const int d_cancel_y =
      d_dialog_y + d_dialog_h - d_cancel_h - d_margin;  // cancel y

  /*........................................................................
  Button enumerations
  ........................................................................*/
  constexpr int kButtonOk = 100;
  constexpr int kButtonCancel = 101;

  /*........................................................................
  Redraw values: in order from "top" to "bottom" layer of the dialog
  ........................................................................*/
  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_BUTTONS = 1,
    REDRAW_BACKGROUND = 2,
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;

  /*........................................................................
  Dialog variables
  ........................................................................*/
  int retcode = 0;

  /*........................................................................
  Buttons
  ........................................................................*/
  game::ControlClass* commands = nullptr;  // the button list

  TextButtonClass okbtn(kButtonOk, TXT_OK, FontType::k6PointGradient,
                        TPF_CENTER | TPF_USE_GRAD_PAL | TPF_NOSHADOW, d_ok_x,
                        d_ok_y, d_ok_w, d_ok_h);

  TextButtonClass cancelbtn(kButtonCancel, TXT_CANCEL,
                            FontType::k6PointGradient,
                            TPF_CENTER | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                            d_cancel_x, d_cancel_y, d_cancel_w, d_cancel_h);

  /*
  ------------------------------- Initialize -------------------------------
  */
  PixelView& view = TheScreen().visible_view();

  /*
  ......................... Create the button list .........................
  */
  commands = &okbtn;
  cancelbtn.Add_Tail(*commands);

  /*
  -------------------------- Main Processing Loop --------------------------
  */
  RedrawType display = REDRAW_ALL;  // requested redraw level
  bool process = true;              // loop while true
  while (process) {

    /*
    ........................ Invoke game callback .........................
    */
    if (Main_Loop()) {
      retcode = 0;
      process = false;
    }

    /*
    ...................... Refresh display if needed ......................
    */
    if (display != REDRAW_NONE) {
      /*
      ...................... Display the dialog box ......................
      */
      game::TheMouse().Hide();
      if (display >= REDRAW_BACKGROUND) {
        Dialog_Box(view, d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);
        Draw_Caption(view, TXT_NONE, d_dialog_x, d_dialog_y, d_dialog_w);

        /*
        ....................... Draw the captions .......................
        */
        Fancy_Text_Print(view, Text_String(TXT_SURRENDER), d_dialog_cx,
                         d_dialog_y + d_topmargin, kCcGreen, kTransparentBlack,
                         FontType::k6PointGradient,
                         TPF_CENTER | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
      }

      /*
      ........................ Redraw the buttons ........................
      */
      if (display >= REDRAW_BUTTONS) {
        commands->Flag_List_To_Redraw();
      }
      game::TheMouse().Show();
      display = REDRAW_NONE;
    }

    /*
    ........................... Get user input ............................
    */
    const engine::window::KeyNumber input = commands->Input(view).key;

    /*
    ............................ Process input ............................
    */
    switch (static_cast<int>(input)) {
      case KN_RETURN:
      case engine::window::ButtonKey(kButtonOk):
        retcode = 1;
        process = false;
        break;

      case KN_ESC:
      case engine::window::ButtonKey(kButtonCancel):
        retcode = 0;
        process = false;
        break;

      default:
        break;
    }
  }

  /*
  --------------------------- Redraw the display ---------------------------
  */
  TheScreen().hidden_page().view().Clear();
  TheMap().Flag_To_Redraw(true);
  TheMap().Render();

  return retcode;
}

}  // namespace td
