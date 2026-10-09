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

/* $Header: /counterstrike/NULLMGR.CPP 2     3/07/97 6:40p Steve_tall $ */
/***************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : NULLMGR.CPP                              *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : April 5, 1995
 **
 *                                                                         *
 *                  Last Update : May 1, 1995 [BRR]                        *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions: * NullModemClass::NullModemClass -- class constructor *
 *   NullModemClass::~NullModemClass -- class destructor                   *
 *   NullModemClass::Init -- initialization
 ** NullModemClass::Send_Message -- sends a message
 ** NullModemClass::Get_Message -- polls the Queue for a message
 ** NullModemClass::Service -- main polling routine
 ** NullModemClass::Num_Send -- Returns # of unACK'd send entries
 ** NullModemClass::Num_Receive -- Returns # entries in the receive queue *
 *   NullModemClass::Response_Time -- Returns Queue's avg response time    *
 *   NullModemClass::Reset_Response_Time -- Resets response time computatio*
 *   NullModemClass::Oldest_Send -- Returns ptr to oldest unACK'd send buf *
 *   NullModemClass::Detect_Modem -- Detects and initializes the modem     *
 *   NullModemClass::Dial_Modem -- dials a number passed                   *
 *   NullModemClass::Answer_Modem -- waits for call and answers            *
 *   NullModemClass::Hangup_Modem -- hangs up the modem                    *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "ra/net/nullmgr.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include "base/numeric.h"
#include "base/strings/number_parse.h"
#include "base/strings/safe_string.h"
#include "engine/gfx/font.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/net/nullconn.h"
#include "engine/net/nullmgr.h"
#include "engine/net/serial/modemreg.h"
#include "engine/net/serial/wincomm.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "engine/window/wait_ticks.h"
#include "game/ui/gadget.h"
#include "game/ui/gadget_list.h"
#include "game/ui/input.h"
#include "game/ui/standard_colors.h"
#include "ra/assets/screen.h"
#include "ra/core/defines.h"
#include "ra/data/text.h"
#include "ra/data/text_ids.h"
#include "ra/net/network.h"
#include "ra/net/session.h"
#include "ra/startup/conquer.h"
#include "ra/startup/init.h"
#include "ra/ui/color_scheme.h"
#include "ra/ui/dialog.h"
#include "ra/ui/msgbox.h"
#include "ra/ui/textbtn.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace ra {

using enum engine::window::KeyNumber;

// the following line was taken from Greenleaf's <ibmkeys.h> <asciidef.h>
// because of other define conflicts

// #define ESC 27
// #define NOKEY 0xffff
#define INIT_COMMAND_RETRIES 2

// this time is in milliseconds

#define DEFAULT_TIMEOUT 1500

void (*NullModemClass::OrigAbortModemFunc)(int);

engine::window::KeyNumber NullModemClass::Input;
game::GadgetList NullModemClass::Commands;  // button list

NullModemClass::NullModemClass(int numsend, int numreceive, int maxlen,
                               uint16_t magicnum)
    : engine::net::NullModemClass(numsend, numreceive, maxlen, magicnum) {}

NullModemClass::~NullModemClass() = default;

/***************************************************************************
 * NullModemClass::Init -- initialization                                  *
 *                                                                         *
 * INPUT:                                                                  *
 *		port				address, or 1 for the modem
 * named dev_name              * irq				2-15 (unused)
 *                                        * dev_name			name of
 * the modem                                       * baud
 *	300, 1200, 9600, etc                                    * parity
 *		'O' (odd), 'E' (even), 'N' (none), 'S' (space),         * 'M'
 * (mark)                                              * wordlength
 *	5, 6, 7, or 8                                           * stopbits
 *		1 or 2                                                  *
 *		flowcontrol		1 for hardware flow control
 *                *
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = error     *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *   10/9/1996  ST : Modified to take device name in win32                 *
 *=========================================================================*/
int NullModemClass::Init(int port, int /*irq*/, char* dev_name, int baud,
                         char parity, int wordlength, int stopbits,
                         int flowcontrol) {
  const std::optional<std::string> device =
      Port_Device(port, dev_name, TheNetwork().modem_registry());
  return Open(device ? device->c_str() : nullptr, baud, parity, wordlength,
              stopbits, flowcontrol);
}

/***********************************************************************************************
 * NMC::Detect_Port -- Checks that the specified com port exists *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to SerialSettingsType *
 *                                                                                             *
 * OUTPUT:   true if port is valid *
 *                                                                                             *
 * WARNINGS: Win32 version always returns true as win95 shouldnt allow us to
 *open the          * port if it doesnt exist or is in use by the mouse *
 *                                                                                             *
 * HISTORY: * 8/2/96 11:47AM ST : Documented / Win32 support *
 *=============================================================================================*/
DetectPortType NullModemClass::Detect_Port(SerialSettingsType* settings) {
  /*
  ** Create a new modem class for our com port
  */
  if (!SerialPort) {
    SerialPort = std::make_unique<WinModemClass>();
  } else {
    SerialPort->Serial_Port_Close();
  }

  /*
  ** Shift up the baud rate to sensible values
  */
  const int baud = settings->Baud;
  //	if (baud == 14400) baud = 19200;
  //	if (baud == 28800) baud = 38400;

  /*
  ** Translate the port address into a usable device name
  */
  const std::optional<std::string> device = Port_Device(
      settings->Port, settings->ModemName, TheNetwork().modem_registry());
  if (!device) {
    return PORT_INVALID;
  }

  /*
  ** Open the com port
  */
  HANDLE porthandle = SerialPort->Serial_Port_Open(
      device->c_str(), baud, 0, 8, 1, settings->HardwareFlowControl ? 1 : 0);

  if (porthandle == nullptr) {
    return PORT_INVALID;
  }

  SerialPort->Serial_Port_Close();
  return PORT_VALID;
}

/***************************************************************************
 * NullModemClass::Detect_Modem -- Detects and initializes the modem       *
 *                                                                         *
 * INPUT:                                                                  *
 *		settings		ptr to SerialSettings structure
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		-1				init string invalid
 ** 0				no modem found
 ** 1				modem found
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   06/02/1995 DRD : Created.                                             *
 *   8/2/96      ST : Added Win32 support                                  *
 *=========================================================================*/
int NullModemClass::Detect_Modem(SerialSettingsType* settings, bool reconnect) {
  /*------------------------------------------------------------------------
  Button Enumerations
  ------------------------------------------------------------------------*/
  int error_count = 0;

  int width = 0;
  int height = 0;  // dialog dimensions
  char buffer[80 * 3];

  /*
  ** Get resolution factor
  */
  //	int	factor = visible_view.width()/320;

  /*------------------------------------------------------------------------
  Determine the dimensions of the text to be used for the dialog box.
  These dimensions will control how the dialog box looks.
  ------------------------------------------------------------------------*/
  base::SafeCopy(buffer, TextString(TXT_INITIALIZING_MODEM));

  PixelView& view = TheScreen().visible_view();

  const int lines = Format_Window_String(TextFontStyle(kTextFont, kTextFlags),
                                         buffer, view.height(), width, height);

  width = std::max(width, 180);
  width += 80;
  height += 80;

  const int x = (view.width() - width) / 2;
  const int y = (view.height() - height) / 2;

  /*------------------------------------------------------------------------
  Draw the dialog
  ------------------------------------------------------------------------*/
  game::TheMouse().Hide();
  if (!reconnect) {
    Load_Title_Page(true);
  }

  Dialog_Box(x, y, width, height);
  Draw_Caption(view, TXT_NONE, x, y, width);

  if (lines == 1) {
    Fancy_Text_Print(view, buffer, x + (width / 2), y + 25, Get_Color_Scheme(),
                     game::kTransparentBlack, kTextFont,
                     kTextFlags | TPF_CENTER);
  } else {
    Fancy_Text_Print(view, buffer, x + 40, y + 25, Get_Color_Scheme(),
                     game::kTransparentBlack, kTextFont, kTextFlags);
  }

  game::TheMouse().Show();

  /*
  ** OK, lets not mess about any more. Just turn on echo, verbose, and result
  *codes
  ** before we even begin. At least this way when we get an error later on we
  *have already
  ** removed all the steps we use to try and recover.
  ** The timeouts need to be quite small in case the modem is turned off.
  */

  /*
  ** Turn on result codes.
  */
  Send_Modem_Command("ATQ0", '\r', buffer, 81, DEFAULT_TIMEOUT / 2, 2);

  /*
  ** Make result codes verbose.
  */
  Send_Modem_Command("ATV1", '\r', buffer, 81, DEFAULT_TIMEOUT / 2, 2);

  /*
  ** Turn on echo.
  */
  Send_Modem_Command("ATE1", '\r', buffer, 81, DEFAULT_TIMEOUT / 2, 2);

  ModemVerboseOn = true;
  ModemEchoOn = true;

  /*
  ** Try sending a plain old AT command to the modem. Now that we have
  *theoretically
  ** turned on verbose result codes we should get an 'OK' back.
  **
  */
  int status = Send_Modem_Command("AT", '\r', buffer, 81, DEFAULT_TIMEOUT, 2);

  if (status < ASSUCCESS) {
    return 0;
  }

  /*
  ** Send the user supplied modem init string
  */
  if (settings->InitStringIndex != -1) {
    const std::string initStr =
        TheSession().InitStrings.at(settings->InitStringIndex)->data();

    std::istringstream tokenStream(initStr);
    std::string token;

    while (std::getline(tokenStream, token, '|')) {
      // Handle consecutive delimiters
      if (token.empty()) {
        continue;
      }

      status = Send_Modem_Command(token.c_str(), '\r', buffer, 81, 3000, 1);

      if (status < ASSUCCESS) {
        if (WWMessageBox().Process(TXT_ERROR_NO_INIT, TXT_IGNORE, TXT_CANCEL)) {
          return 0;
        }
        error_count++;
        break;
      }
    }
  } else {
  }

  if (settings->Port == 1 && TheNetwork().modem_registry()) {
    // Helper lambda to handle the "Append AT -> Send -> Check Error" pattern.
    // Captures context to access 'buffer' and other necessary variables.
    const auto sendInitCommand = [&](const char* cmdSuffix, int errorMsgId,
                                     int timeout = DEFAULT_TIMEOUT) -> bool {
      if (!cmdSuffix) {
        return true;  // Nothing to send, proceed.
      }

      std::string fullCommand = "AT";
      fullCommand += cmdSuffix;

      const int result =
          Send_Modem_Command(fullCommand.c_str(), '\r', buffer, 81, timeout, 1);

      // Stop initialization only when the command failed and the user clicked
      // "Cancel" (Process returns true for it); success or "Ignore" continues.
      return result == kModemCmdOk || result == kModemCmd0 ||
             !WWMessageBox().Process(errorMsgId, TXT_IGNORE, TXT_CANCEL);
    };

    // 1. Flow Control
    const char* flowCmd =
        settings->HardwareFlowControl
            ? TheNetwork().modem_registry()->Get_Modem_Hardware_Flow_Control()
            : TheNetwork().modem_registry()->Get_Modem_No_Flow_Control();
    const int flowTimeout =
        settings->HardwareFlowControl ? 300 : DEFAULT_TIMEOUT;

    if (!sendInitCommand(flowCmd, TXT_NO_FLOW_CONTROL_RESPONSE, flowTimeout)) {
      return 0;
    }

    // 2. Compression
    const char* compCmd =
        settings->Compression
            ? TheNetwork().modem_registry()->Get_Modem_Compression_Enable()
            : TheNetwork().modem_registry()->Get_Modem_Compression_Disable();

    if (!sendInitCommand(compCmd, TXT_NO_COMPRESSION_RESPONSE)) {
      return 0;
    }

    // 3. Error Correction
    const char* errCmd =
        settings->ErrorCorrection
            ? TheNetwork().modem_registry()->Get_Modem_Error_Correction_Enable()
            : TheNetwork()
                  .modem_registry()
                  ->Get_Modem_Error_Correction_Disable();

    if (!sendInitCommand(errCmd, TXT_NO_ERROR_CORRECTION_RESPONSE)) {
      return 0;
    }
  }

  /*
  ** We require that auto-answer be disabled so turn it off now.
  */
  status = Send_Modem_Command("ATS0=0", '\r', buffer, 81, DEFAULT_TIMEOUT,
                              INIT_COMMAND_RETRIES);
  if (status != kModemCmdOk) {
    if (WWMessageBox().Process(TXT_ERROR_NO_DISABLE, TXT_IGNORE, TXT_CANCEL)) {
      return 0;
    }
    error_count++;
  }

  /*
  ** If we had an unreasonable number of ignored errors then return failure
  */
  if (error_count >= 3) {
    WWMessageBox().Process(TXT_ERROR_TOO_MANY, TXT_OK);
    return 0;
  }

  return 1;
}

/***************************************************************************
 * NullModemClass::Dial_Modem -- dials a number passed                     *
 *                                                                         *
 * INPUT:                                                                  *
 *		settings		ptr to SerialSettings structure
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		status		DialStatus
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   06/02/1995 DRD : Created.                                             *
 *   8/2/96      ST : Win32 support                                        *
 *=========================================================================*/
// Not const: dials through the serial port.
// NOLINTNEXTLINE(readability-make-member-function-const)
DialStatusType NullModemClass::Dial_Modem(const char* string,
                                          DialMethodType method,
                                          bool reconnect) {
  /*
  ** Get the resolution factor
  */
  //	int factor = visible_view.width()/320;

  /*------------------------------------------------------------------------
  Button Enumerations
  ------------------------------------------------------------------------*/
  constexpr int kButtonCancel = 100;

  /*------------------------------------------------------------------------
  Dialog variables
  ------------------------------------------------------------------------*/
  bool process = true;  // process while true
  DialStatusType dialstatus = DIAL_ERROR;

  int width = 0;
  int height = 0;  // dialog dimensions
  /*------------------------------------------------------------------------
  Determine the dimensions of the text to be used for the dialog box.
  These dimensions will control how the dialog box looks.
  ------------------------------------------------------------------------*/
  const char* buffer_const =
      TextString(reconnect ? TXT_MODEM_CONNERR_REDIALING : TXT_DIALING);

  std::string buffer(buffer_const);

  PixelView& view = TheScreen().visible_view();

  const FontStyle font = TextFontStyle(kTextFont, kTextFlags);
  Format_Window_String(font, std::span(buffer), view.height(), width, height);

  const int text_width = width;
  width = std::max(width, 180);
  width += 80;
  height += 120;

  const int x = (view.width() - width) / 2;
  const int y = (view.height() - height) / 2;

  TextButtonClass cancelbtn(
      kButtonCancel, TXT_CANCEL, kButtonFont, kButtonFlags,
      x + ((width - (StringPixelWidth(font, TextString(TXT_CANCEL)) + 16)) / 2),
      y + height - (FontLineHeight(font) + 4) - 20);

  /*------------------------------------------------------------------------
  Create the list
  ------------------------------------------------------------------------*/
  Commands.Clear();
  Commands.Add(cancelbtn);

  Commands.FlagToRedraw();

  /*------------------------------------------------------------------------
  Draw the dialog
  ------------------------------------------------------------------------*/
  game::TheMouse().Hide();
  if (!reconnect) {
    Load_Title_Page(true);
  }

  Dialog_Box(x, y, width, height);
  Draw_Caption(view, TXT_NONE, x, y, width);

  Fancy_Text_Print(view, buffer.c_str(), (view.width() / 2) - (text_width / 2),
                   y + 50, Get_Color_Scheme(), game::kTransparentBlack,
                   kTextFont, kTextFlags);

  Commands.DrawAll(view);
  game::TheMouse().Show();

  /*
  ** Start waiting for connection response
  */
  SerialPort->Set_Modem_Dial_Type(static_cast<WinCommDialMethodType>(method));
  /*
  ** Clear out any old modem results that might be hanging around
  */
  SerialPort->Get_Modem_Result(60, buffer.c_str(), 81);
  /*
  ** Dial that sucker
  */
  SerialPort->Dial_Modem(string);

  /*
  ** Sets up the ability to abort modem commands when any input is in the
  ** Keyboard buffer.  This also calls the game CallBack().
  */
  Setup_Abort_Modem();

  /*------------------------------------------------------------------------
  Main Processing Loop
  ------------------------------------------------------------------------*/
  process = true;
  int delay = ModemWaitCarrier;
  while (process) {

    delay = SerialPort->Get_Modem_Result(delay, buffer.c_str(), 81);

    /*.....................................................................
    Process input
    .....................................................................*/
    switch (static_cast<int>(Input)) {
      case KN_ESC:
      case engine::window::ButtonKey(kButtonCancel):
        dialstatus = DIAL_CANCELED;
        process = false;
        break;

      default:
        break;
    }

    if (process) {
      if (buffer.starts_with("CON")) {
        TheNetwork().modem_response() = buffer;
        dialstatus = DIAL_CONNECTED;
        process = false;
      } else if (buffer.starts_with("BUSY")) {
        dialstatus = DIAL_BUSY;
        process = false;
      } else if (buffer.starts_with("NO C")) {
        dialstatus = DIAL_NO_CARRIER;
        process = false;
      } else if (buffer.starts_with("NO D")) {
        dialstatus = DIAL_NO_DIAL_TONE;
        process = false;
      } else if (buffer.starts_with("ERRO")) {
        dialstatus = DIAL_ERROR;
        process = false;
      }
    }

    if (delay <= 0) {
      process = false;
    }
  }

  Remove_Abort_Modem();
  // cancelbtn is a local of this function: empty the static list before the
  // button goes.
  Commands.Clear();

  return dialstatus;

} /* end of Dial_Modem */

/***************************************************************************
 * NullModemClass::Answer_Modem -- waits for call and answers              *
 *                                                                         *
 * INPUT:                                                                  *
 *		reconnect	whether this is to reconnect
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		status		DialStatus
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   06/02/1995 DRD : Created.                                             *
 *   8/2/96      ST : Added Win32 support                                  *
 *=========================================================================*/
// Not const: answers through the serial port.
// NOLINTNEXTLINE(readability-make-member-function-const)
DialStatusType NullModemClass::Answer_Modem(bool reconnect) {
  /*
  ** Get the resolution factor
  */
  //	int factor 		= (visible_view.width() == 320) ? 1 : 2;

  /*------------------------------------------------------------------------
  Button Enumerations
  ------------------------------------------------------------------------*/
  constexpr int kButtonCancel = 100;

  /*------------------------------------------------------------------------
  Redraw values: in order from "top" to "bottom" layer of the dialog
  ------------------------------------------------------------------------*/
  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_BUTTONS = 1,
    REDRAW_BACKGROUND = 2,
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;

  /*------------------------------------------------------------------------
  Dialog variables
  ------------------------------------------------------------------------*/
  RedrawType display = REDRAW_ALL;  // redraw level
  bool process = true;              // process while true
  DialStatusType dialstatus = DIAL_ERROR;
  bool ring = false;

  int width = 0;
  int height = 0;  // dialog dimensions
  char text_buffer[80 * 3];
  char comm_buffer[80 * 3];

  /*------------------------------------------------------------------------
  Determine the dimensions of the text to be used for the dialog box.
  These dimensions will control how the dialog box looks.
  ------------------------------------------------------------------------*/
  if (reconnect) {
    base::SafeCopy(text_buffer, TextString(TXT_MODEM_CONNERR_WAITING));
  } else {
    base::SafeCopy(text_buffer, TextString(TXT_WAITING_FOR_CALL));
  }

  PixelView& view = TheScreen().visible_view();

  const FontStyle font = TextFontStyle(kTextFont, kTextFlags);
  Format_Window_String(font, text_buffer, view.height(), width, height);

  int text_width = width;
  width = std::max(width, 180);
  width += 80;
  height += 120;

  int x = (view.width() - width) / 2;
  int y = (view.height() - height) / 2;

  TextButtonClass cancelbtn(
      kButtonCancel, TXT_CANCEL, kButtonFont, kButtonFlags,
      x + ((width - (StringPixelWidth(font, TextString(TXT_CANCEL)) + 16)) / 2),
      y + height - (FontLineHeight(font) + 4) - 20);

  /*------------------------------------------------------------------------
  Initialize
  ------------------------------------------------------------------------*/
  Load_Title_Page(true);

  Input = KN_NONE;

  /*------------------------------------------------------------------------
  Create the list
  ------------------------------------------------------------------------*/
  Commands.Clear();
  Commands.Add(cancelbtn);

  Commands.FlagToRedraw();

  /*
  ** Sets up the ability to abort modem commands when any input is in the
  ** Keyboard buffer.  This also calls the game CallBack() and Input().
  */
  Setup_Abort_Modem();

  /*------------------------------------------------------------------------
  Main Processing Loop
  ------------------------------------------------------------------------*/
  process = true;
  int delay = 60000;
  while (process) {

    /*.....................................................................
    Refresh display if needed
    .....................................................................*/
    if (display != REDRAW_NONE) {
      game::TheMouse().Hide();
      if (display >= REDRAW_BACKGROUND) {
        /*...............................................................
        Refresh the backdrop
        ...............................................................*/
        if (!reconnect) {
          Load_Title_Page(true);
        }

        /*...............................................................
        Draw the background
        ...............................................................*/
        Dialog_Box(x, y, width, height);

        /*...............................................................
        Draw the labels
        ...............................................................*/
        Draw_Caption(view, TXT_NONE, x, y, width);

        Fancy_Text_Print(
            view, text_buffer, (view.width() / 2) - (text_width / 2), y + 50,
            Get_Color_Scheme(), game::kTransparentBlack, kTextFont, kTextFlags);

        Commands.DrawAll(view);
      }
      game::TheMouse().Show();
      display = REDRAW_NONE;
    }

    delay = SerialPort->Get_Modem_Result(delay, comm_buffer, 81);

    /*.....................................................................
    Process input
    .....................................................................*/
    if (!Input) {
      Input = Commands.Input(view).key;
    }
    switch (static_cast<int>(Input)) {
      case KN_ESC:
      case engine::window::ButtonKey(kButtonCancel):
        dialstatus = DIAL_CANCELED;
        process = false;
        break;

      default:
        break;
    }

    if (process) {
      if (std::string_view(comm_buffer).starts_with("RING")) {
        base::SafeCopy(text_buffer, TextString(TXT_ANSWERING));

        Format_Window_String(font, text_buffer, view.height(), width, height);

        text_width = width;
        width = std::max(width, 180);
        width += 80;
        height += 120;

        x = (view.width() - width) / 2;
        y = (view.height() - height) / 2;

        static constexpr unsigned char kAnswerCommand[] = {'A', 'T', 'A', '\r'};
        SerialPort->Write_To_Serial_Port(
            kAnswerCommand, static_cast<int>(sizeof(kAnswerCommand)));

        ring = true;
        delay = ModemWaitCarrier;
        display = REDRAW_ALL;
      } else if (std::string_view(comm_buffer).starts_with("CON")) {
        TheNetwork().modem_response() = comm_buffer;
        dialstatus = DIAL_CONNECTED;
        process = false;
      } else if (std::string_view(comm_buffer).starts_with("BUSY")) {
        dialstatus = DIAL_BUSY;
        process = false;
      } else if (std::string_view(comm_buffer).starts_with("NO C")) {
        dialstatus = DIAL_NO_CARRIER;
        process = false;
      } else if (std::string_view(comm_buffer).starts_with("ERRO")) {
        dialstatus = DIAL_ERROR;
        WWMessageBox().Process(TXT_ERROR_ERROR, TXT_OK);
        process = false;
      }
    }

    if (delay <= 0) {
      if (ring) {
        if (SerialPort->Get_Modem_Status() & kCdSet) {
          TheNetwork().modem_response() = "Connected";
          dialstatus = DIAL_CONNECTED;
        } else {
          dialstatus = DIAL_ERROR;
          WWMessageBox().Process(TXT_ERROR_TIMEOUT, TXT_OK);
        }
        process = false;
      } else {
        delay = 60000;
      }
    }
  }

  Remove_Abort_Modem();
  // cancelbtn is a local of this function: empty the static list before the
  // button goes.
  Commands.Clear();

  return dialstatus;

} /* end of Answer_Modem */

/***************************************************************************
 * NullModemClass::Hangup_Modem -- hangs up the modem                      *
 *                                                                         *
 * INPUT:                                                                  *
 *		none
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		status		successful or not
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   06/02/1995 DRD : Created.                                             *
 *   8/2/96         : Added Win32 support                                  *
 *=========================================================================*/
// Not const: hangs up through the serial port.
// NOLINTNEXTLINE(readability-make-member-function-const)
bool NullModemClass::Hangup_Modem() {
  char buffer[81];
  char escape[4];

  /*
  **	Turn modem servicing off in the callback routine.
  */
  TheSession().ModemService = false;

  int status = Send_Modem_Command("AT", '\r', buffer, 81, DEFAULT_TIMEOUT, 1);

  if (status == kModemCmdOk) {
    TheSession().ModemService = true;
    return true;
  }

  /*
  ** Toggle DTR low then high
  */
  SerialPort->Set_Serial_DTR(false);
  engine::window::WaitTicks(3200 / 60);
  SerialPort->Set_Serial_DTR(true);

  status = Send_Modem_Command("AT", '\r', buffer, 81, DEFAULT_TIMEOUT, 1);

  if (status == kModemCmdOk) {
    TheSession().ModemService = true;
    return true;
  }

  int delay = ModemGuardTime;
  while (delay > 0) {
    delay = SerialPort->Get_Modem_Result(delay, buffer, 81);
  }

  /*
  ** Send modem break commmand
  */
  escape[0] = ModemEscapeCode;
  escape[1] = ModemEscapeCode;
  escape[2] = ModemEscapeCode;
  escape[3] = 0;

  SerialPort->Write_To_Serial_Port(escape, 3);

  delay = ModemGuardTime;
  while (delay > 0) {
    delay = SerialPort->Get_Modem_Result(delay, buffer, 81);

    if (std::string_view(buffer).starts_with("OK")) {
      break;
    }
  }

  /*
  ** Send the hangup command
  */
  status = Send_Modem_Command("ATH", '\r', buffer, 81, ModemHangupDelay, 1);

  if (status == kModemCmdOk) {
  } else {
    TheSession().ModemService = true;
    return false;
  }

  /*
  ** Send spurious ATZ command for no apparent reason
  */
  status = Send_Modem_Command("ATZ", '\r', buffer, 81, 5000, 1);

  if (status != kModemCmdOk) {
    TheSession().ModemService = true;
    return false;
  }

  TheSession().ModemService = true;
  return true;

} /* end of Hangup_Modem */

/***********************************************************************************************
 * NMC::Setup_Modem_Echo -- Sets the echo callback function pointer *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Ptr to callback function *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/2/96 12:48PM ST : Documented and added WIn32 support *
 *=============================================================================================*/
void NullModemClass::Setup_Modem_Echo(void (*func)(char c)) {
  SerialPort->Set_Echo_Function(func);
}

/***********************************************************************************************
 * NMC::Remove_Modem_Echo -- Set the echo function callback pointer to null *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/2/96 12:50PM ST : Documented / Win32 support added *
 *=============================================================================================*/
void NullModemClass::Remove_Modem_Echo() {
  //	Smart_Printf( "Remove Echo modem code\n" );
  SerialPort->Set_Echo_Function(nullptr);
}

/***********************************************************************************************
 * NMC::Print_EchoBuf -- Print out the contents of the echo buffer *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/2/96 12:51PM ST : Documented *
 *=============================================================================================*/
void NullModemClass::Print_EchoBuf() {
  for (int i = 0; std::cmp_less(
           i,
           std::string_view(TheNetwork().null_modem().EchoBuf.data()).size());
       i++) {
    if (TheNetwork().null_modem().EchoBuf.at(base::ToSize(i)) == '\r') {
      TheNetwork().null_modem().EchoBuf.at(base::ToSize(i)) = 1;
    } else {
      if (TheNetwork().null_modem().EchoBuf.at(base::ToSize(i)) == '\n') {
        TheNetwork().null_modem().EchoBuf.at(base::ToSize(i)) = 2;
      }
    }
  }
  //	Smart_Printf( "Echo buffer length %d (%s)\n",
  // NullModem.EchoCount,
  // NullModem.EchoBuf );
}

/***********************************************************************************************
 * NMC::Reset_EchoBuf -- Empties the echo buffer *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/2/96 12:51PM ST : Documented *
 *=============================================================================================*/
void NullModemClass::Reset_EchoBuf() {
  EchoBuf.front() = 0;
  EchoCount = 0;
}

/***********************************************************************************************
 * NMC::Abort_Modem -- Checks for user input so that modem operations can be
 *aborted           *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   ASUSERABORT if abort key pressed. ASSUCESS otherwise. *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/2/96 12:52PM ST : Documented *
 *=============================================================================================*/
int NullModemClass::Abort_Modem() {
  /*
  ** Button Enumerations
  */
  constexpr int kButtonCancel = 100;

  /*
  ** Invoke game callback
  */
  ServiceRealTime();

  /*
  ** Get user input. The serial port installs this as an int(*)() abort
  ** callback, so it cannot be handed a view; the only two functions that
  ** install it, Dial_Modem() and Answer_Modem(), both draw their dialog to
  ** the visible page, which is the page this used to find in the global.
  */
  PixelView& view = TheScreen().visible_view();
  Input = Commands.Input(view).key;

  switch (static_cast<int>(Input)) {
    case KN_ESC:
    case engine::window::ButtonKey(kButtonCancel):
      return ASUSERABORT;
    default:
      break;
  }

  return ASSUCCESS;

} /* end of Abort_Modem */

/***********************************************************************************************
 * NMC::Setup_Abort_Modem -- sets the modem abort function pointer *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/2/96 2:59PM ST : Documented / Win32 support added *
 *=============================================================================================*/
void NullModemClass::Setup_Abort_Modem() {
  SerialPort->Set_Abort_Function(Abort_Modem);
}

/***********************************************************************************************
 * NMC::Remove_Abort_Modem -- Removes the modem abort function pointer *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/2/96 3:01PM ST : Documented / Win32 support added *
 *=============================================================================================*/
void NullModemClass::Remove_Abort_Modem() {
  SerialPort->Set_Abort_Function(nullptr);
}

/***********************************************************************************************
 * NMC::Change_IRQ_Priority -- Increases the priority of the serial interrupt *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Interrupt request number *
 *                                                                                             *
 * OUTPUT:   ASSUCCESS if changed *
 *                                                                                             *
 * WARNINGS: The Win32 version of this function does nothing. * Priorities are
 *controlled by windoze                                              *
 *                                                                                             *
 * HISTORY: * 8/2/96 3:03PM ST : Documented / Win32 support added *
 *=============================================================================================*/
int NullModemClass::Change_IRQ_Priority(int /*irq*/) {
  return ASSUCCESS;
} /* end of Change_IRQ_Priority */

/***********************************************************************************************
 * NMC::Get_Modem_Status -- returns status of modem control bits *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Modem status *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/2/96 3:06PM ST : Documented / Win32 support added *
 *=============================================================================================*/
uint32_t NullModemClass::Get_Modem_Status() {
  char buffer[81];

  // Modem status is a small bit mask (CTS/DSR/RI/CD).
  uint32_t modemstatus = SerialPort->Get_Modem_Status();

  const int status =
      Send_Modem_Command("AT", '\r', buffer, 81, DEFAULT_TIMEOUT, 1);

  if (status == kModemCmdOk) {
    modemstatus &= ~kCdSet;
  }

  return modemstatus;

} /* end of Get_Modem_Status */

/***********************************************************************************************
 * NMC::Send_Modem_Command -- Sends an 'AT' command to the modem and gets the
 *response         *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    command to send to modem. e.g. 'ATZ' * terminator byte for command
 *string                                                * buffer to put modem
 *response into                                                 * length of
 *above buffer                                                            *
 *           delay to wait for response * number of times to retry when modem
 *doesnt respond                                *
 *                                                                                             *
 * OUTPUT:   input delay less the time it took the modem to respond *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/2/96 3:09PM ST : Documented / Win32 support added *
 *=============================================================================================*/
int NullModemClass::Send_Modem_Command(const char* command, char terminator,
                                       char* buffer, int buflen, int delay,
                                       int retries) {
  return SerialPort->Send_Command_To_Modem(command, terminator, buffer, buflen,
                                           delay, retries);
}

/***********************************************************************************************
 * NMC::Verify_And_Convert_To_Int -- converts a text string of numbers to an int
 **
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to buffer *
 *                                                                                             *
 * OUTPUT:   value of text number in buffer *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/2/96 3:13PM ST : Documented *
 *=============================================================================================*/
int NullModemClass::Verify_And_Convert_To_Int(char* buffer) {
  int value = 0;
  const int len = static_cast<int>(std::string_view(buffer).size());

  for (int i = 0; i < len; i++) {
    if (!isdigit(static_cast<unsigned char>(
            std::string_view(buffer).at(base::ToSize(i))))) {
      value = -1;
      break;
    }
  }

  if (value == 0) {
    value = base::ParseIntegerOr<int>(buffer, 0);
  }

  return value;

} /* end of Verify_And_Convert_To_Int */

/*************************** end of nullmgr.cpp ****************************/

}  // namespace ra
