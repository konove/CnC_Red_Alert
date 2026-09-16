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

#include "td/mplayer.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "port/bytes_of.h"
#include "port/ex_string.h"
#include "port/safe_string.h"
#include "port/tokenizer.h"
#include "rand.h"
#include "sdllib/file_access.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/shape.h"
#include "sdllib/timer.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "td/conquer.h"
#include "td/control.h"
#include "td/defines.h"
#include "td/dialog.h"
#include "td/externs.h"
#include "td/globals.h"
#include "td/goptions.h"
#include "td/house.h"
#include "td/ini.h"
#include "td/init.h"
#include "td/ipxmgr.h"
#include "td/jshell.h"
#include "td/mapedit.h"
#include "td/msglist.h"
#include "td/nulldlg.h"
#include "td/phone.h"
#include "td/profile.h"
#include "td/rand.h"
#include "td/randomstate.h"
#include "td/special.h"
#include "td/text.h"
#include "td/textbtn.h"
#include "td/vector.h"
#include "tech/game_file.h"
#include "tech/number_parse.h"

static void Garble_Message(std::span<char> buf);

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
  const int factor = SeenBuff.Get_Width() == 320 ? 1 : 2;
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

  const CountDownTimerClass delay;

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
  ControlClass* commands = nullptr;  // the button list

  //
  // If neither IPX or winsock are active then do only the modem serial dialog
  //
  if (Ipx.Is_IPX()) {
    ipx_avail = true;
  }

  TextButtonClass modemserialbtn(
      kButtonModemserial, TXT_MODEM_SERIAL,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
      d_modemserial_x, d_modemserial_y, d_modemserial_w, d_modemserial_h);
  TextButtonClass ipxbtn(
      kButtonIpx, TXT_NETWORK,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, d_ipx_x,
      d_ipx_y, d_ipx_w, d_ipx_h);

  TextButtonClass cancelbtn(
      kButtonCancel, TXT_CANCEL,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, d_cancel_x,
      d_cancel_y, d_cancel_w, d_cancel_h);

  /*
  ------------------------------- Initialize -------------------------------
  */
  Set_Logic_Page(SeenBuff);

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

  Keyboard::Clear();

  Fancy_Text_Print(TXT_NONE, 0, 0, kCcGreen, kTBlack,
                   TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

  /*
  -------------------------- Main Processing Loop --------------------------
  */
  RedrawType display = REDRAW_ALL;  // true = re-draw everything
  bool process = true;              // loop while true
  bool pressed = false;
  while (process) {
    /*
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      display = REDRAW_ALL;
    }

    /*
    ........................ Invoke game callback .........................
    */
    Call_Back();
    /*
    ...................... Refresh display if needed ......................
    */
    if (display != REDRAW_NONE) {
      Hide_Mouse();
      if (display >= REDRAW_BACKGROUND) {
        /*
        ..................... Refresh the backdrop ......................
        */
        Load_Title_Page(true);
        /*
        ..................... Draw the background .......................
        */
        Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);
        Draw_Caption(TXT_SELECT_MPLAYER_GAME, d_dialog_x, d_dialog_y,
                     d_dialog_w);
      }
      /*
      .......................... Redraw buttons ..........................
      */
      if (display >= REDRAW_BUTTONS) {
        commands->Flag_List_To_Redraw();
      }
      Show_Mouse();
      display = REDRAW_NONE;
    }

    /*
    ........................... Get user input ............................
    */
    const KeyNumType input = commands->Input();  // input from user

    /*
    ............................ Process input ............................
    */
    switch (static_cast<int>(input)) {
      case ButtonKey(kButtonModemserial):
        selection = kButtonModemserial;
        pressed = true;
        break;

      case ButtonKey(kButtonIpx):
        selection = kButtonIpx;
        pressed = true;
        break;

      case KN_ESC:
      case ButtonKey(kButtonCancel):
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
      base::At(buttons, curbutton)->Draw_Me(true);

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
  char* entry = nullptr;   // a phone book entry
  char buf[128];           // buffer for parsing INI entry
  int i = 0;

  /*------------------------------------------------------------------------
  Fetch working pointer to the INI staging buffer. Make sure that the buffer
  is cleared out before proceeding.  (Don't use the HidPage for this, since
  the HidPage may be needed for various uncompressions during the INI
  parsing.)
  ------------------------------------------------------------------------*/
  char* buffer = ShapeBuffer;  // INI staging buffer pointer.
  std::ranges::fill(ShapeBufferBytes, 0);

  /*------------------------------------------------------------------------
  Clear the initstring entries
  ------------------------------------------------------------------------*/
  for (i = 0; i < InitStrings.Count(); i++) {
    delete[] InitStrings[i];
  }
  InitStrings.Clear();

  /*------------------------------------------------------------------------
  Clear the dialing entries
  ------------------------------------------------------------------------*/
  for (i = 0; i < PhoneBook.Count(); i++) {
    delete PhoneBook[i];
  }
  PhoneBook.Clear();

  /*------------------------------------------------------------------------
  Create filename and read the file.
  ------------------------------------------------------------------------*/
  GameFile file("CONQUER.INI");
  if (!file.IsAvailable()) {
    return;
  }
  file.Read(std::as_writable_bytes(ShapeBufferBytes)
                .first(ShapeBufferBytes.size() - 1));
  file.Close();

  if (!Special.IsFromWChat) {
    /*------------------------------------------------------------------------
    Get the player's last-used Handle
    ------------------------------------------------------------------------*/
    WWGetPrivateProfileString("MultiPlayer", "Handle", "Noname", MPlayerName,
                              buffer);

    /*------------------------------------------------------------------------
    Get the player's last-used Color
    ------------------------------------------------------------------------*/
    MPlayerPrefColor =
        WWGetPrivateProfileInt("MultiPlayer", "Color", 0, buffer);
    MPlayerHouse = static_cast<HousesType>(WWGetPrivateProfileInt(
        "MultiPlayer", "Side", static_cast<int>(HOUSE_GOOD), buffer));
    CurPhoneIdx =
        WWGetPrivateProfileInt("MultiPlayer", "PhoneIndex", -1, buffer);
  } else {
    CurPhoneIdx = -1;
  }

  TrapCheckHeap = WWGetPrivateProfileInt("MultiPlayer", "CheckHeap", 0, buffer);

  /*------------------------------------------------------------------------
  Read in default serial settings
  ------------------------------------------------------------------------*/
  WWGetPrivateProfileString(
      "SerialDefaults", "ModemName", "NoName",
      std::span(SerialDefaults.ModemName)
          .first(static_cast<std::size_t>(MODEM_NAME_MAX)),
      buffer);
  if ((std::string_view(SerialDefaults.ModemName) == "NoName")) {
    base::At(SerialDefaults.ModemName, 0) = 0;
  }
  WWGetPrivateProfileString("SerialDefaults", "Port", "0",
                            std::span(buf).first(static_cast<std::size_t>(5)),
                            buffer);
  if (const auto value = tech::ParseHex<int>(buf)) {
    SerialDefaults.Port = *value;
  }
  SerialDefaults.IRQ =
      WWGetPrivateProfileInt("SerialDefaults", "IRQ", -1, buffer);
  SerialDefaults.Baud =
      WWGetPrivateProfileInt("SerialDefaults", "Baud", -1, buffer);
  SerialDefaults.Init =
      WWGetPrivateProfileInt("SerialDefaults", "Init", 0, buffer) != 0;
  SerialDefaults.Compression =
      WWGetPrivateProfileInt("SerialDefaults", "Compression", 0, buffer) != 0;
  SerialDefaults.ErrorCorrection =
      WWGetPrivateProfileInt("SerialDefaults", "ErrorCorrection", 0, buffer) !=
      0;
  SerialDefaults.HardwareFlowControl =
      WWGetPrivateProfileInt("SerialDefaults", "HardwareFlowControl", 1,
                             buffer) != 0;
  WWGetPrivateProfileString("SerialDefaults", "DialMethod", "T",
                            std::span(buf).first(static_cast<std::size_t>(2)),
                            buffer);

  // find dial method

  for (i = 0; i < kDialMethods; i++) {
    if (!port::CompareIgnoreCase(
            buf, DialMethodCheck[static_cast<DialMethodType>(i)])) {
      SerialDefaults.DialMethod = static_cast<DialMethodType>(i);
      break;
    }
  }

  // if method not found set to touch tone

  if (i == kDialMethods) {
    SerialDefaults.DialMethod = DIAL_TOUCH_TONE;
  }

  SerialDefaults.InitStringIndex =
      WWGetPrivateProfileInt("SerialDefaults", "InitStringIndex", 0, buffer);

  SerialDefaults.CallWaitStringIndex = WWGetPrivateProfileInt(
      "SerialDefaults", "CallWaitStringIndex", kCallWaitCustom, buffer);

  WWGetPrivateProfileString(
      "SerialDefaults", "CallWaitString", "",
      std::span(SerialDefaults.CallWaitString)
          .first(static_cast<std::size_t>(CWAITSTRBUF_MAX)),
      buffer);

  if (SerialDefaults.IRQ == 0 || SerialDefaults.Baud == 0) {
    SerialDefaults.Port = 0;
    SerialDefaults.IRQ = -1;
    SerialDefaults.Baud = -1;
  }

  /*------------------------------------------------------------------------
  Set 'tbuffer' to point past the actual INI data
  ------------------------------------------------------------------------*/
  std::vector<char> key_storage(std::string_view(buffer).size() + 2);
  auto key_cursor = std::span(key_storage);
  char* tbuffer = key_cursor.data();  // Accumulation buffer of trigger IDs.

  /*------------------------------------------------------------------------
  Read all Base-Scenario names into 'tbuffer'
  ------------------------------------------------------------------------*/
  WWGetPrivateProfileString("InitStrings", nullptr, nullptr, key_cursor,
                            buffer);

  /*------------------------------------------------------------------------
  Read in & store each entry
  ------------------------------------------------------------------------*/
  while (*tbuffer != '\0') {
    entry = new char[INITSTRBUF_MAX];
    // This allocation owns exactly INITSTRBUF_MAX writable characters.
    // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
    const std::span<char> entry_storage(entry, INITSTRBUF_MAX);

    entry[0] = 0;

    WWGetPrivateProfileString("InitStrings", tbuffer, nullptr, entry_storage,
                              buffer);

    strupr(entry);

    InitStrings.Add(entry);

    key_cursor = key_cursor.subspan(std::string_view(tbuffer).size() + 1);
    tbuffer = key_cursor.data();
  }

  // if no entries then have at least one

  if (key_cursor.data() == key_storage.data()) {
    entry = new char[INITSTRBUF_MAX];
    // This allocation owns exactly INITSTRBUF_MAX writable characters.
    // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
    const std::span<char> entry_storage(entry, INITSTRBUF_MAX);
    port::SafeCopy(entry_storage, "ATZ");
    InitStrings.Add(entry);
    SerialDefaults.InitStringIndex = 0;
  }

  /*------------------------------------------------------------------------
  Repeat the process for the phonebook
  ------------------------------------------------------------------------*/
  key_cursor = std::span(key_storage);
  tbuffer = key_cursor.data();

  /*------------------------------------------------------------------------
  Read in all phone book listings.
  Format: Name=PhoneNum,Port,IRQ,Baud,InitString
  ------------------------------------------------------------------------*/

  /*........................................................................
  Read the entry names in
  ........................................................................*/
  WWGetPrivateProfileString("PhoneBook", nullptr, nullptr, key_cursor, buffer);

  while (*tbuffer != '\0') {
    /*.....................................................................
    Create a new phone book entry
    .....................................................................*/
    auto* phone = new PhoneEntryClass();  // a phone book entry

    /*.....................................................................
    Read the entire entry in
    .....................................................................*/
    WWGetPrivateProfileString(
        "PhoneBook", tbuffer, nullptr,
        std::span(buf).first(static_cast<std::size_t>(128)), buffer);

    /*.....................................................................
    Extract name, phone # & serial port settings
    .....................................................................*/
    port::Tokenizer tokens(buf, "|");
    char* tokenptr = tokens.Next();  // ptr to token
    if (tokenptr) {
      port::SafeCopy(phone->Name, tokenptr);
      strupr(phone->Name);
    } else {
      base::At(phone->Name, 0) = 0;
    }

    tokenptr = tokens.Next();
    if (tokenptr) {
      port::SafeCopy(phone->Number, tokenptr);
      strupr(phone->Number);
    } else {
      base::At(phone->Number, 0) = 0;
    }

    tokenptr = tokens.Next();
    if (tokenptr) {
      if (const auto value = tech::ParseHex<int>(tokenptr)) {
        phone->Settings.Port = *value;
      }
    } else {
      phone->Settings.Port = 0;
    }

    tokenptr = tokens.Next();
    if (tokenptr) {
      phone->Settings.IRQ = tech::ParseInteger<int>(tokenptr).value_or(0);
    } else {
      phone->Settings.IRQ = -1;
    }

    tokenptr = tokens.Next();
    if (tokenptr) {
      phone->Settings.Baud = tech::ParseInteger<int>(tokenptr).value_or(0);
    } else {
      phone->Settings.Baud = -1;
    }

    tokenptr = tokens.Next();
    if (tokenptr) {
      phone->Settings.Compression =
          tech::ParseInteger<int>(tokenptr).value_or(0) != 0;
    } else {
      phone->Settings.Compression = false;
    }

    tokenptr = tokens.Next();
    if (tokenptr) {
      phone->Settings.ErrorCorrection =
          tech::ParseInteger<int>(tokenptr).value_or(0) != 0;
    } else {
      phone->Settings.ErrorCorrection = false;
    }

    tokenptr = tokens.Next();
    if (tokenptr) {
      phone->Settings.HardwareFlowControl =
          tech::ParseInteger<int>(tokenptr).value_or(0) != 0;
    } else {
      phone->Settings.HardwareFlowControl = true;
    }

    tokenptr = tokens.Next();
    if (tokenptr) {
      port::SafeCopy(buf, tokenptr);

      // find dial method

      for (i = 0; i < kDialMethods; i++) {
        if (!port::CompareIgnoreCase(
                buf, DialMethodCheck[static_cast<DialMethodType>(i)])) {
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
    if (tokenptr) {
      phone->Settings.InitStringIndex =
          tech::ParseInteger<int>(tokenptr).value_or(0);
    } else {
      phone->Settings.InitStringIndex = 0;
    }

    tokenptr = tokens.Next();
    if (tokenptr) {
      phone->Settings.CallWaitStringIndex =
          tech::ParseInteger<int>(tokenptr).value_or(0);
    } else {
      phone->Settings.CallWaitStringIndex = kCallWaitCustom;
    }

    tokenptr = tokens.Next();
    if (tokenptr) {
      port::SafeCopy(phone->Settings.CallWaitString, tokenptr);
    } else {
      base::At(phone->Settings.CallWaitString, 0) = 0;
    }

    /*.....................................................................
    Add it to our list
    .....................................................................*/
    PhoneBook.Add(phone);

    key_cursor = key_cursor.subspan(std::string_view(tbuffer).size() + 1);
    tbuffer = key_cursor.data();
  }

  /*------------------------------------------------------------------------
  Read special recording playback values, to help find sync bugs
  ------------------------------------------------------------------------*/
  if (PlaybackGame) {
    TrapFrame = WWGetPrivateProfileInt("SyncBug", "Frame", 0x7fffffff, buffer);

    TrapObjType = static_cast<RTTIType>(WWGetPrivateProfileInt(
        "SyncBug", "Type", static_cast<int>(RTTI_NONE), buffer));
    WWGetPrivateProfileString(
        "SyncBug", "Type", "NONE",
        std::span(buf).first(static_cast<std::size_t>(80)), buffer);
    if (!port::CompareIgnoreCase(buf, "AIRCRAFT")) {
      TrapObjType = RTTI_AIRCRAFT;
    } else if (!port::CompareIgnoreCase(buf, "ANIM")) {
      TrapObjType = RTTI_ANIM;
    } else if (!port::CompareIgnoreCase(buf, "BUILDING")) {
      TrapObjType = RTTI_BUILDING;
    } else if (!port::CompareIgnoreCase(buf, "BULLET")) {
      TrapObjType = RTTI_BULLET;
    } else if (!port::CompareIgnoreCase(buf, "INFANTRY")) {
      TrapObjType = RTTI_INFANTRY;
    } else if (!port::CompareIgnoreCase(buf, "UNIT")) {
      TrapObjType = RTTI_UNIT;
    } else {
      TrapObjType = RTTI_NONE;
    }

    WWGetPrivateProfileString(
        "SyncBug", "Coord", "0",
        std::span(buf).first(static_cast<std::size_t>(80)), buffer);
    TrapCoord = tech::ParseHex<uint32_t>(buf).value_or(0);

    WWGetPrivateProfileString(
        "SyncBug", "this", "0",
        std::span(buf).first(static_cast<std::size_t>(80)), buffer);
    if (const auto trap_this = tech::ParseHex<uintptr_t>(buf)) {
      TrapThis = std::bit_cast<void*>(*trap_this);
    }

    WWGetPrivateProfileString(
        "SyncBug", "Cell", "0",
        std::span(buf).first(static_cast<std::size_t>(80)), buffer);
    CELL const cell = tech::ParseInteger<CELL>(buf).value_or(0);
    if (cell) {
      TrapCell = &Map[cell];
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
  GameFile file;
  char entrytext[4];
  char buf[128];  // buffer for parsing INI entry

  /*------------------------------------------------------------------------
  Get a working pointer to the INI staging buffer. Make sure that the buffer
  starts cleared out of any data.
  ------------------------------------------------------------------------*/
  char* buffer = ShapeBuffer;  // INI staging buffer pointer.
  std::ranges::fill(ShapeBufferBytes, 0);

  file.SetName("CONQUER.INI");
  if (file.IsAvailable()) {
    file.Open(FileAccess::kRead);
    file.Read(std::as_writable_bytes(ShapeBufferBytes)
                  .first(ShapeBufferBytes.size() - 1));
    file.Close();
  }

  /*------------------------------------------------------------------------
  Save the player's last-used Handle & Color
  ------------------------------------------------------------------------*/
  WWWritePrivateProfileInt("MultiPlayer", "PhoneIndex", CurPhoneIdx,
                           port::CharBytes(ShapeBufferBytes));
  WWWritePrivateProfileInt("MultiPlayer", "Color", MPlayerPrefColor,
                           port::CharBytes(ShapeBufferBytes));
  WWWritePrivateProfileInt("MultiPlayer", "Side",
                           static_cast<int>(MPlayerHouse),
                           port::CharBytes(ShapeBufferBytes));
  WWWritePrivateProfileString("MultiPlayer", "Handle", MPlayerName,
                              port::CharBytes(ShapeBufferBytes));

  /*------------------------------------------------------------------------
  Clear all existing SerialDefault entries.
  ------------------------------------------------------------------------*/
  WWWritePrivateProfileString("SerialDefaults", nullptr, nullptr,
                              port::CharBytes(ShapeBufferBytes));

  /*------------------------------------------------------------------------
  Save default serial settings in opposite order you want to see them
  ------------------------------------------------------------------------*/
  WWWritePrivateProfileString("SerialDefaults", "CallWaitString",
                              SerialDefaults.CallWaitString,
                              port::CharBytes(ShapeBufferBytes));
  WWWritePrivateProfileInt("SerialDefaults", "CallWaitStringIndex",
                           SerialDefaults.CallWaitStringIndex,
                           port::CharBytes(ShapeBufferBytes));
  WWWritePrivateProfileInt("SerialDefaults", "InitStringIndex",
                           SerialDefaults.InitStringIndex,
                           port::CharBytes(ShapeBufferBytes));
  WWWritePrivateProfileInt("SerialDefaults", "Init",
                           SerialDefaults.Init ? 1 : 0,
                           port::CharBytes(ShapeBufferBytes));
  WWWritePrivateProfileString("SerialDefaults", "DialMethod",
                              DialMethodCheck[SerialDefaults.DialMethod],
                              port::CharBytes(ShapeBufferBytes));
  WWWritePrivateProfileInt("SerialDefaults", "Baud", SerialDefaults.Baud,
                           port::CharBytes(ShapeBufferBytes));
  WWWritePrivateProfileInt("SerialDefaults", "IRQ", SerialDefaults.IRQ,
                           port::CharBytes(ShapeBufferBytes));
  absl::SNPrintF(buf, sizeof(buf), "%x",
                 static_cast<unsigned int>(SerialDefaults.Port));
  WWWritePrivateProfileString("SerialDefaults", "Port", buf,
                              port::CharBytes(ShapeBufferBytes));
  WWWritePrivateProfileString("SerialDefaults", "ModemName",
                              SerialDefaults.ModemName,
                              port::CharBytes(ShapeBufferBytes));
  WWWritePrivateProfileInt("SerialDefaults", "Compression",
                           SerialDefaults.Compression ? 1 : 0,
                           port::CharBytes(ShapeBufferBytes));
  WWWritePrivateProfileInt("SerialDefaults", "ErrorCorrection",
                           SerialDefaults.ErrorCorrection ? 1 : 0,
                           port::CharBytes(ShapeBufferBytes));
  WWWritePrivateProfileInt("SerialDefaults", "HardwareFlowControl",
                           SerialDefaults.HardwareFlowControl ? 1 : 0,
                           port::CharBytes(ShapeBufferBytes));

  /*------------------------------------------------------------------------
  Clear all existing InitString entries.
  ------------------------------------------------------------------------*/
  WWWritePrivateProfileString("InitStrings", nullptr, nullptr,
                              port::CharBytes(ShapeBufferBytes));

  /*------------------------------------------------------------------------
  Save all InitString entries.  In descending order so they come out in
  ascending order.
  ------------------------------------------------------------------------*/
  for (int i = static_cast<int>(InitStrings.Count()) - 1; i >= 0; i--) {
    absl::SNPrintF(buf, sizeof(buf), "%03d", i);
    WWWritePrivateProfileString("InitStrings", buf, InitStrings[i],
                                port::CharBytes(ShapeBufferBytes));
  }

  /*------------------------------------------------------------------------
  Clear all existing Phone Book entries.
  ------------------------------------------------------------------------*/
  WWWritePrivateProfileString("PhoneBook", nullptr, nullptr,
                              port::CharBytes(ShapeBufferBytes));

  /*------------------------------------------------------------------------
  Save all Phone Book entries.
  Format: Entry=Name,PhoneNum,Port,IRQ,Baud,InitString
  ------------------------------------------------------------------------*/
  for (int i = static_cast<int>(PhoneBook.Count()) - 1; i >= 0; i--) {
    absl::SNPrintF(buf, sizeof(buf), "%s|%s|%x|%d|%d|%d|%d|%d|%s|%d|%d|%s",
                   PhoneBook[i]->Name, PhoneBook[i]->Number,
                   static_cast<unsigned int>(PhoneBook[i]->Settings.Port),
                   PhoneBook[i]->Settings.IRQ, PhoneBook[i]->Settings.Baud,
                   PhoneBook[i]->Settings.Compression ? 1 : 0,
                   PhoneBook[i]->Settings.ErrorCorrection ? 1 : 0,
                   PhoneBook[i]->Settings.HardwareFlowControl ? 1 : 0,
                   DialMethodCheck[PhoneBook[i]->Settings.DialMethod],
                   PhoneBook[i]->Settings.InitStringIndex,
                   PhoneBook[i]->Settings.CallWaitStringIndex,
                   PhoneBook[i]->Settings.CallWaitString);
    absl::SNPrintF(entrytext, sizeof(entrytext), "%03d", i);
    WWWritePrivateProfileString("PhoneBook", entrytext, buf,
                                port::CharBytes(ShapeBufferBytes));
  }

  /*------------------------------------------------------------------------
  Write the INI data out to a file.
  ------------------------------------------------------------------------*/
  file.Open(FileAccess::kWrite);
  file.Write(
      std::as_bytes(ShapeBufferBytes).first(std::string_view(buffer).size()));
  file.Close();
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
  GameFile file;
  char fname[20];

  /*------------------------------------------------------------------------
  Clear the scenario description lists
  ------------------------------------------------------------------------*/
  MPlayerScenarios.Clear();
  MPlayerFilenum.Clear();

  /*------------------------------------------------------------------------
  Loop through all possible scenario numbers; if a file is available, add
  its number to the FileNum list.
  ------------------------------------------------------------------------*/
  for (int i = 0; i < 100; i++) {
    Set_Scenario_Name(ScenarioName, i, SCEN_PLAYER_MPLAYER, SCEN_DIR_EAST,
                      SCEN_VAR_A);
    absl::SNPrintF(fname, sizeof(fname), "%s.INI", ScenarioName);
    file.SetName(fname);

    if (file.IsAvailable()) {
      MPlayerFilenum.Add(i);
    }
  }

  /*------------------------------------------------------------------------
  Now, for every file in the FileNum list, read in the INI file, and extract
  its description.
  ------------------------------------------------------------------------*/
  for (int i = 0; i < MPlayerFilenum.Count(); i++) {
    /*.....................................................................
    Fetch working pointer to the INI staging buffer. Make sure that the
    buffer is cleared out before proceeding.
    .....................................................................*/
    char* buffer = ShapeBuffer;  // INI staging buffer pointer.
    std::ranges::fill(ShapeBufferBytes, 0);

    /*.....................................................................
    Create filename and read the file.
    .....................................................................*/
    Set_Scenario_Name(ScenarioName, MPlayerFilenum[i], SCEN_PLAYER_MPLAYER,
                      SCEN_DIR_EAST, SCEN_VAR_A);
    absl::SNPrintF(fname, sizeof(fname), "%s.INI", ScenarioName);
    file.SetName(fname);
    file.Read(std::as_writable_bytes(ShapeBufferBytes)
                  .first(ShapeBufferBytes.size() - 1));
    file.Close();

    /*.....................................................................
    Extract description & add it to the list.
    .....................................................................*/
    WWGetPrivateProfileString("Basic", "Name", "Nulls-Ville",
                              std::span(base::At(MPlayerDescriptions, i))
                                  .first(static_cast<std::size_t>(40)),
                              buffer);
    MPlayerScenarios.Add(base::At(MPlayerDescriptions, i));
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
  MPlayerScenarios.Clear();
  MPlayerFilenum.Clear();

  /*------------------------------------------------------------------------
  Clear the initstring entries
  ------------------------------------------------------------------------*/
  for (int i = 0; i < InitStrings.Count(); i++) {
    delete InitStrings[i];
  }
  InitStrings.Clear();

  /*------------------------------------------------------------------------
  Clear the dialing entries
  ------------------------------------------------------------------------*/
  for (int i = 0; i < PhoneBook.Count(); i++) {
    delete PhoneBook[i];
  }
  PhoneBook.Clear();
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
       static_cast<int>(house) < static_cast<int>(HOUSE_MULTI1) + MPlayerMax;
       house++) {
    HouseClass* ptr = HouseClass::As_Pointer(house);

    if (!ptr || ptr->IsHuman || ptr->IsDefeated) {
      continue;
    }

    /*.....................................................................
    Decode this house's color
    .....................................................................*/
    const int color =
        base::At(MPlayerTColors, static_cast<int>(ptr->RemapColor));

    /*.....................................................................
    We now have a 1/4 chance of echoing one of the human players' messages
    back.
    .....................................................................*/
    if (GameRandomRange(0, 3) == 2) {
      /*..................................................................
      Now we have a 1/3 chance of garbling the human message.
      ..................................................................*/
      if (GameRandomRange(0, 2) == 1) {
        Garble_Message(LastMessage);
      }

      /*..................................................................
      Only add the message if there is one to add.
      ..................................................................*/
      if (!std::string_view(LastMessage).empty()) {
        absl::SNPrintF(txt, sizeof(txt), "%s %s",
                       Text_String(TXT_FROM_COMPUTER), LastMessage);
        Messages.Add_Message(txt, color,
                             TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW,
                             600, 0, 0);
      }
    } else {
      absl::SNPrintF(txt, sizeof(txt), "%s %s", Text_String(TXT_FROM_COMPUTER),
                     Text_String(TXT_COMP_MSG1 + GameRandomRange(0, 12)));
      Messages.Add_Message(txt, color,
                           TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW,
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
  char* words[40];  // ptrs to various words in the phrase

  /*------------------------------------------------------------------------
  Pull off any trailing punctuation
  ------------------------------------------------------------------------*/
  const std::string_view message(buf.data());
  size_t punctuation = message.size();
  while (punctuation > 0 && message.size() - punctuation < sizeof(punct) - 1) {
    const char ch = message[punctuation - 1];
    if (ch != '!' && ch != '.' && ch != '?') {
      break;
    }
    --punctuation;
  }
  port::SafeCopy(punct, message.substr(punctuation));
  buf[punctuation] = '\0';

  for (auto& word : words) {
    word = nullptr;
  }

  /*------------------------------------------------------------------------
  Copy the original buffer
  ------------------------------------------------------------------------*/
  port::SafeCopy(txt, buf.data());

  /*------------------------------------------------------------------------
  Split it up into words
  ------------------------------------------------------------------------*/
  port::Tokenizer tokens(txt, " ");
  char* p = tokens.Next();
  int numwords = 0;  // # words in the phrase
  while (p) {
    base::At(words, numwords) = p;
    numwords++;
    p = tokens.Next();
  }

  /*------------------------------------------------------------------------
  Now randomly put the words back.  Don't use the real random-number
  generator, since different machines will have different LastMessage's,
  and will go out of sync.
  ------------------------------------------------------------------------*/
  buf[0] = 0;
  for (int i = 0; i < numwords; i++) {
    const int j = Sim_IRandom(0, numwords);
    if (base::At(words, j) == nullptr) {  // this word has been used already
      i--;
      continue;
    }
    port::SafeAppend(std::span(buf).first(MAX_MESSAGE_LENGTH),
                     base::At(words, j));
    base::At(words, j) = nullptr;
    if (i < numwords - 1) {
      port::SafeAppend(std::span(buf).first(MAX_MESSAGE_LENGTH), " ");
    }
  }
  port::SafeAppend(std::span(buf).first(MAX_MESSAGE_LENGTH), punct);
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
  const int factor = SeenBuff.Get_Width() == 320 ? 1 : 2;
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
  ControlClass* commands = nullptr;  // the button list

  TextButtonClass okbtn(
      kButtonOk, TXT_OK,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, d_ok_x,
      d_ok_y, d_ok_w, d_ok_h);

  TextButtonClass cancelbtn(
      kButtonCancel, TXT_CANCEL,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, d_cancel_x,
      d_cancel_y, d_cancel_w, d_cancel_h);

  /*
  ------------------------------- Initialize -------------------------------
  */
  Set_Logic_Page(SeenBuff);

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
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      display = REDRAW_ALL;
    }

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
      Hide_Mouse();
      if (display >= REDRAW_BACKGROUND) {
        Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);
        Draw_Caption(TXT_NONE, d_dialog_x, d_dialog_y, d_dialog_w);

        /*
        ....................... Draw the captions .......................
        */
        Fancy_Text_Print(
            Text_String(TXT_SURRENDER), d_dialog_cx, d_dialog_y + d_topmargin,
            kCcGreen, kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
      }

      /*
      ........................ Redraw the buttons ........................
      */
      if (display >= REDRAW_BUTTONS) {
        commands->Flag_List_To_Redraw();
      }
      Show_Mouse();
      display = REDRAW_NONE;
    }

    /*
    ........................... Get user input ............................
    */
    const KeyNumType input = commands->Input();

    /*
    ............................ Process input ............................
    */
    switch (static_cast<int>(input)) {
      case KN_RETURN:
      case ButtonKey(kButtonOk):
        retcode = 1;
        process = false;
        break;

      case KN_ESC:
      case ButtonKey(kButtonCancel):
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
  HiddenPage.Clear();
  Map.Flag_To_Redraw(true);
  Map.Render();

  return retcode;
}
