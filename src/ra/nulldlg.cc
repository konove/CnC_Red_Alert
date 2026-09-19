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

/* $Header: /counterstrike/NULLDLG.CPP 14    3/17/97 1:05a Steve_tall $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : NULLDLG.CPP                              *
 *                                                                         *
 *                   Programmer : Bill R. Randolph                         *
 *                                                                         *
 *                   Start Date : 04/29/95                                 *
 *                                                                         *
 *                  Last Update : Jan. 21, 1997 [V.Grippi]                     *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   Build_InitString_Listbox -- [re]builds the initstring entry listbox   *
 *   Build_Phone_Listbox -- [re]builds the phone entry listbox             *
 *   Com_Scenario_Dialog -- Serial game scenario selection dialog
 ** Com_Settings_Dialog -- Lets user select serial port settings          *
 *   Destroy_Null_Connection -- destroys the given connection
 ** Edit_Phone_Dialog -- lets user edit a phone book entry                *
 *   Init_Null_Modem -- Initializes Null Modem communications              *
 ** Phone_Dialog -- Lets user edit phone directory & dial                 *
 *   Reconnect_Null_Modem -- allows user to reconnect
 ** Select_Serial_Dialog -- Serial Communications menu dialog             *
 *   Shutdown_Modem -- Shuts down modem/null-modem communications          *
 *   Test_Null_Modem -- Null-Modem test routine                            *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "ra/nulldlg.h"

#include <algorithm>
#include <bit>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "magic_enum/magic_enum.hpp"
#include "port/platform.h"
#include "port/random_seed.h"
#include "port/safe_string.h"
#include "port/unaligned.h"
#include "ra/ccini.h"
#include "ra/cheklist.h"
#include "ra/colrlist.h"
#include "ra/config.h"
#include "ra/conquer.h"
#include "ra/coord.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/drop.h"
#include "ra/edit.h"
#include "ra/event.h"
#include "ra/expand.h"
#include "ra/externs.h"
#include "ra/gadget.h"
#include "ra/gauge.h"
#include "ra/globals.h"
#include "ra/goptions.h"
#include "ra/house.h"
#include "ra/ini.h"
#include "ra/init.h"
#include "ra/inline.h"
#include "ra/installation.h"
#include "ra/jshell.h"
#include "ra/list.h"
#include "ra/mapedit.h"
#include "ra/mission_id.h"
#include "ra/mplayer.h"
#include "ra/msgbox.h"
#include "ra/msglist.h"
#include "ra/netdlg.h"
#include "ra/nullmgr.h"
#include "ra/palette.h"
#include "ra/rules.h"
#include "ra/saveload.h"
#include "ra/scenario.h"
#include "ra/session.h"
#include "ra/slider.h"
#include "ra/special.h"
#include "ra/startup.h"
#include "ra/statbtn.h"
#include "ra/text_ids.h"
#include "ra/textbtn.h"
#include "ra/theme.h"
#include "ra/type.h"
#include "ra/vector_dynamic.h"
#include "ra/version.h"
#include "ra/wol_main.h"
#include "ra/ww_audio.h"
#include "sdllib/drawbuff.h"
#include "sdllib/font.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/modemreg.h"
#include "sdllib/timer.h"
#include "sdllib/wincomm.h"
#include "sdllib/ww_audio.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"
#include "sdllib/wwstd.h"
#include "tech/fixed.h"
#include "tech/ftimer.h"
#include "tech/game_file.h"
#include "tech/mix_archive.h"
#include "tech/number_parse.h"
#include "tech/search_paths.h"

ModemRegistryEntryClass* ModemRegistry = nullptr;  // Ptr to modem registry data


// #include "WolDebug.h"

//
// how much time (ticks) to go by before thinking other system
// is not responding.
//
#define PACKET_SENDING_TIMEOUT 1800
#define PACKET_CANCEL_TIMEOUT 900

// extern char const *ForMisStr[];

//
// how much time (ticks) to go by before sending another packet
// of game options or serial connect.
//
#define PACKET_RETRANS_TIME 30
#define PACKET_REDRAW_TIME 60

static int Reconnect_Null_Modem();
static int Com_Settings_Dialog(SerialSettingsType* settings);
static int Phone_Dialog();
static void Build_Init_String_Listbox(ListClass* list, EditClass* edit,
                                      std::span<char> buf, int* index);
static void Build_Phone_Listbox(ListClass* list, EditClass* edit,
                                std::span<char> buf);
static int Edit_Phone_Dialog(PhoneEntryClass* phone);
static bool Dial_Modem(SerialSettingsType* settings, bool reconnect);
static bool Answer_Modem(SerialSettingsType* settings, bool reconnect);
static void Modem_Echo(char c);

static SerialPacketType SendPacket;
static SerialPacketType ReceivePacket;
static char TheirName[MPLAYER_NAME_MAX];
static PlayerColorType TheirColor;
static HousesType TheirHouse;
static std::string DialString;
static SerialSettingsType* DialSettings;

#define PCOLOR_BROWN PCOLOR_GREY


/***************************************************************************
 * Init_Null_Modem -- Initializes Null Modem communications                *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		true = OK, false = error
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   04/29/1995 BRR : Created.                                             *
 *   8/2/96      ST : Win32 support added                                  *
 *=========================================================================*/
bool Init_Null_Modem(SerialSettingsType* settings) {
  return NullModem.Init(settings->Port, settings->IRQ, settings->ModemName,
                        settings->Baud, 0, 8, 1,
                        settings->HardwareFlowControl ? 1 : 0) != 0;
}

/***************************************************************************
 * Shutdown_Modem -- Shuts down modem/null-modem communications            *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   04/29/1995 BRR : Created.                                             *
 *=========================================================================*/
void Shutdown_Modem() {
  if ((!Session.Play) && (Session.Type == GAME_MODEM)) {
    NullModem.Hangup_Modem();
  }

  NullModemClass::Change_IRQ_Priority(0);  // reset priority of interrupts

  //
  // close port
  //
  NullModem.Shutdown();
}

/***************************************************************************
 * Modem_Signoff -- sends EXIT event
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   08/03/1995 DRD : Created.                                             *
 *=========================================================================*/
void Modem_Signoff() {
  EventClass event;

  if (!Session.Play) {
    /*
    ** Send a sign-off packet
    */
    event.Type = EventClass::EXIT;
    NullModem.Send_Message(base::ObjectBytes(event), sizeof(EventClass), 0);
    NullModem.Send_Message(base::ObjectBytes(event), sizeof(EventClass), 0);

    const int64_t starttime = TickCount.Value();
    while (TickCount.Value() - starttime < 30) {
      NullModem.Service();
    }
  }
}

/***************************************************************************
 * Test_Null_Modem -- Null-Modem test routine                              *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      0 = failure to connect; 1 = I'm the game owner, 2 = I'm not        *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   04/29/1995 BRR : Created.                                             *
 *   8/2/96      ST : Win32 support added                                  *
 *=========================================================================*/
int Test_Null_Modem() {
  /*
  ** Get the resolution factor
  */
  //	int factor			= (SeenBuff.Get_Width() == 320) ? 1 : 2;

  /*
  ** Button Enumerations
  */
  constexpr int kButtonCancel = 100;

  /*
  ** Dialog variables
  */
  bool process = true;  // process while true

  int retval = 0;
  int packetlen = 0;

  int width = 0;
  int height = 0;  // dialog dimensions
  char buffer[80 * 3];
  RemapControlType* scheme = GadgetClass::Get_Color_Scheme();

  /*
  ** Buttons
  */

  /*
  **	Determine the dimensions of the text to be used for the dialog box.
  **	These dimensions will control how the dialog box looks.
  */
  port::SafeCopy(buffer, Text_String(TXT_WAITING_CONNECT));
  Fancy_Text_Print(TXT_NONE, 0, 0, nullptr, kTBlack,
                   TPF_6PT_GRAD | TPF_NOSHADOW);
  Format_Window_String(buffer, SeenBuff.Get_Height(), width, height);

  width = std::max(width, 100);
  width += 80;
  height += 120;

  const int x = (SeenBuff.Get_Width() - width) / 2;
  const int y = (SeenBuff.Get_Height() - height) / 2;

  TextButtonClass cancelbtn(
      kButtonCancel, TXT_CANCEL, kTpfButton,
      x + ((width - (String_Pixel_Width(Text_String(TXT_CANCEL)) + 16)) / 2),
      y + height - (FontHeight + FontYSpacing + 4) - 20);

  /*
  ** Initialize
  */
  Set_Logic_Page(SeenBuff);
  process = true;

  /*
  ** Create the list
  */
  GadgetClass* commands = &cancelbtn;  // button list

  commands->Flag_List_To_Redraw();

  /*
  ** Draw the dialog
  */
  Hide_Mouse();
  Load_Title_Page(true);

  Dialog_Box(x, y, width, height);
  Draw_Caption(TXT_NONE, x, y, width);

  Fancy_Text_Print(buffer, x + 40, y + 50, scheme, kTBlack, kTpfText);

  commands->Draw_All();
  while (Get_Mouse_State() > 0) {
    Show_Mouse();
  }


  /*
  ** Check for a packet.  If we detect one, the other system has already been
  ** started.  Wait 1/2 sec for him to receive my ACK, then exit with success.
  ** Note: The initial time must be a little longer than the resend delay.
  ** 	Just in case we just missed the packet.
  */
  int64_t starttime = TickCount.Value();
  while (TickCount.Value() - starttime < 80) {
    NullModem.Service();
    if ((NullModem.Get_Message(base::ObjectBytes(ReceivePacket), &packetlen) >
         0) &&
        (ReceivePacket.Command == SERIAL_CONNECT)) {
      starttime = TickCount.Value();
      while (TickCount.Value() - starttime < 30) {
        NullModem.Service();
      }
      process = false;
      retval = 2;
      break;
    }
  }

  /*
  ** Send a packet across.  As long as Num_Send() is non-zero, the other system
  ** hasn't received it yet.
  */
  if (process) {
    base::FillBytes(base::ObjectBytes(SendPacket), 0, sizeof(SerialPacketType));
    SendPacket.Command = SERIAL_CONNECT;
    //
    // put time from start of game for determining the host in case of tie.
    //
    SendPacket.ScenarioInfo.Seed = static_cast<int>(TickCount.Value());
    SendPacket.ID = static_cast<unsigned char>(std::bit_cast<uintptr_t>(
        &buffer[0]));  // address of buffer for more uniqueness.

    NullModem.Send_Message(base::ObjectBytes(SendPacket), sizeof(SendPacket),
                           1);

    starttime = TickCount.Value();
    while (TickCount.Value() - starttime < 80) {
      NullModem.Service();
      if ((NullModem.Get_Message(base::ObjectBytes(ReceivePacket), &packetlen) >
           0) &&
          (ReceivePacket.Command == SERIAL_CONNECT)) {
        starttime = TickCount.Value();
        while (TickCount.Value() - starttime < 30) {
          NullModem.Service();
        }

        //
        // whoever has the highest time is the host
        //
        if (ReceivePacket.ScenarioInfo.Seed > SendPacket.ScenarioInfo.Seed) {
          process = false;
          retval = 2;
        } else if (ReceivePacket.ScenarioInfo.Seed ==
                   SendPacket.ScenarioInfo.Seed) {
          if (ReceivePacket.ID > SendPacket.ID) {
            process = false;
            retval = 2;
            //
            // if they are equal then it's a loopback cable or a modem
            //
          } else if (ReceivePacket.ID == SendPacket.ID) {
            process = false;
            retval = 3;
          }
        }

        break;
      }
    }
  }

  starttime = TickCount.Value();

  /*
  ** Main Processing Loop
  */
  while (process) {
    /*
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      commands->Draw_All();
    }
    /*
    ** Invoke game callback
    */
    ServiceRealTime();

    /*
    ** Get user input
    */
    const KeyNumType input = commands->Input();

    /*
    ** Process input
    */
    switch (static_cast<int>(input)) {
      case KN_ESC:
      case ButtonKey(kButtonCancel):
        retval = 0;
        process = false;
        break;

      default:
        break;
    }
    /*
    ** Service the connection.
    */
    NullModem.Service();
    if (NullModem.Num_Send() == 0) {
      if (NullModem.Get_Message(base::ObjectBytes(ReceivePacket), &packetlen) >
          0) {
        if (ReceivePacket.Command == SERIAL_CONNECT) {
          starttime = TickCount.Value();
          while (TickCount.Value() - starttime < 30) {
            NullModem.Service();
          }

          //
          // whoever has the highest time is the host
          //
          if (ReceivePacket.ScenarioInfo.Seed > SendPacket.ScenarioInfo.Seed) {
            process = false;
            retval = 2;

          } else if (ReceivePacket.ScenarioInfo.Seed ==
                     SendPacket.ScenarioInfo.Seed) {
            if (ReceivePacket.ID > SendPacket.ID) {
              process = false;
              retval = 2;

              //
              // if they are equal then it's a loopback cable or a modem
              //
            } else if (ReceivePacket.ID == SendPacket.ID) {
              process = false;
              retval = 3;
            }
          }

        } else {
          retval = 0;
          process = false;
        }
      } else {
        retval = 1;
        process = false;
      }
    }

    if (TickCount.Value() - starttime > 3600) {  // only wait 1 minute
      retval = 0;
      process = false;
    }
  } /* end of while */

  return retval;
}

/***************************************************************************
 * Reconnect_Modem -- allows user to reconnect
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		0 = failure to connect; 1 = connect OK
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   04/29/1995 BRR : Created.                                             *
 *=========================================================================*/
int Reconnect_Modem() {
  int status = 0;
  uint32_t modemstatus = 0;

  switch (Session.ModemType) {
    case MODEM_NULL_HOST:
    case MODEM_NULL_JOIN:
      status = Reconnect_Null_Modem();
      break;

    case MODEM_DIALER:
      modemstatus = NullModemClass::Get_Modem_Status();
      if (modemstatus & kCdSet) {
        status = Reconnect_Null_Modem();
      } else {
        status = Dial_Modem(DialSettings, true) ? 1 : 0;
      }
      break;

    case MODEM_ANSWERER:
      modemstatus = NullModemClass::Get_Modem_Status();
      if (modemstatus & kCdSet) {
        status = Reconnect_Null_Modem();
      } else {
        status = Answer_Modem(DialSettings, true) ? 1 : 0;
      }
      break;
    default:
      break;
  }

  return status;
}

/***************************************************************************
 * Reconnect_Null_Modem -- allows user to reconnect
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		0 = failure to connect; 1 = connect OK
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   04/29/1995 BRR : Created.                                             *
 *=========================================================================*/
static int Reconnect_Null_Modem() {
  /*
  ** Button Enumerations
  */
  constexpr int kButtonCancel = 100;

  /*
  ** Dialog variables
  */
  bool process = true;  // process while true

  int retval = 0;
  int64_t lastmsgtime = 0;
  int packetlen = 0;
  RemapControlType* scheme = GadgetClass::Get_Color_Scheme();

  int width = 0;
  int height = 0;  // dialog dimensions
  char buffer[80 * 3];

  /*
  ** Buttons
  */

  /*
  **	Determine the dimensions of the text to be used for the dialog box.
  **	These dimensions will control how the dialog box looks.
  */
  port::SafeCopy(buffer, Text_String(TXT_NULL_CONNERR_CHECK_CABLES));
  Fancy_Text_Print(TXT_NONE, 0, 0, nullptr, kTBlack,
                   TPF_6PT_GRAD | TPF_NOSHADOW);
  Format_Window_String(buffer, SeenBuff.Get_Height(), width, height);

  width = std::max(width, 100);
  width += 80;
  height += 120;

  const int x = (SeenBuff.Get_Width() - width) / 2;
  const int y = (SeenBuff.Get_Height() - height) / 2;

  TextButtonClass cancelbtn(
      kButtonCancel, TXT_CANCEL, kTpfButton,
      x + ((width - (String_Pixel_Width(Text_String(TXT_CANCEL)) + 16)) / 2),
      y + height - (FontHeight + FontYSpacing + 4) - 20);

  /*
  ** Initialize
  */
  Set_Logic_Page(SeenBuff);
  process = true;

  /*
  ** Create the list
  */
  GadgetClass* commands = &cancelbtn;  // button list

  commands->Flag_List_To_Redraw();

  /*
  ** Draw the dialog
  */
  Hide_Mouse();

  Dialog_Box(x, y, width, height);
  Draw_Caption(TXT_NONE, x, y, width);

  Fancy_Text_Print(buffer, x + 40, y + 50, scheme, kTBlack, kTpfText);

  commands->Draw_All();
  Show_Mouse();

  /*
  ** Main Processing Loop
  */
  int64_t starttime = lastmsgtime = TickCount.Value();
  while (process) {
    /*
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      commands->Draw_All();
    }
    /*
    ** Invoke game callback
    */
    ServiceRealTime();

    /*
    ** Get user input
    */
    const KeyNumType input = commands->Input();

    /*
    ** Process input
    */
    switch (static_cast<int>(input)) {
      case KN_ESC:
      case ButtonKey(kButtonCancel):
        retval = 0;
        process = false;
        break;

      default:
        break;
    }
    /*
    ** Service the connection.
    */
    NullModem.Service();

    /*
    ** Resend our message if it's time
    */
    if (TickCount.Value() - starttime > PACKET_RETRANS_TIME) {
      starttime = TickCount.Value();
      base::FillBytes(base::ObjectBytes(SendPacket), 0,
                      sizeof(SerialPacketType));
      SendPacket.Command = SERIAL_CONNECT;
      SendPacket.ID = static_cast<unsigned char>(Session.ColorIdx);
      NullModem.Send_Message(base::ObjectBytes(SendPacket), sizeof(SendPacket),
                             0);
    }

    /*
    ** Check for an incoming message
    */
    if (NullModem.Get_Message(base::ObjectBytes(ReceivePacket), &packetlen) >
        0) {
      lastmsgtime = TickCount.Value();

      if (ReceivePacket.Command == SERIAL_CONNECT) {
        // are we getting our own packets back??

        if (ReceivePacket.ID == static_cast<unsigned char>(Session.ColorIdx)) {
          WWMessageBox().Process(TXT_SYSTEM_NOT_RESPONDING);
          retval = 0;
          break;
        }

        /*
        ** OK, we got our message; now we have to make certain the other
        ** guy gets his, so send him one with an ACK required.
        */
        base::FillBytes(base::ObjectBytes(SendPacket), 0,
                        sizeof(SerialPacketType));
        SendPacket.Command = SERIAL_CONNECT;
        SendPacket.ID = static_cast<unsigned char>(Session.ColorIdx);
        NullModem.Send_Message(base::ObjectBytes(SendPacket),
                               sizeof(SendPacket), 1);
        starttime = TickCount.Value();
        while (TickCount.Value() - starttime < 60) {
          NullModem.Service();
        }
        retval = 1;
        process = false;
      }
    }

    //
    // timeout if we do not get any packets
    //
    if (TickCount.Value() - lastmsgtime > PACKET_CANCEL_TIMEOUT) {
      retval = 0;
      process = false;
    }

  } /* end of while */

  return retval;
}

/***********************************************************************************************
 * Destroy_Null_Connection -- destroys the given connection
 **
 *                                                                         						  *
 * Call this routine when a connection goes bad, or another player signs off.
 **
 *                                                                         						  *
 * INPUT: * id			connection ID to destroy; this should be the
 *HousesType of the player being	  * "destroyed".
 ** error		0 = user signed off; 1 = connection error
 **
 *                                                                         						  *
 * OUTPUT: * none.
 **
 *                                                                         						  *
 * WARNINGS: * none.
 **
 *                                                                         						  *
 * HISTORY: * 07/31/1995 DRD : Created. *
 *=============================================================================================*/
void Destroy_Null_Connection(int id, int error) {
  char txt[80];

  if (Session.NumPlayers == 1) {
    return;
  }

  /*
  **	Do nothing if the house isn't human.
  */
  HouseClass* housep = HouseClass::As_Pointer(static_cast<HousesType>(id));
  if (!housep || !housep->IsHuman) {
    return;
  }

  /*
  **	Create a message to display to the user
  */
  txt[0] = '\0';
  switch (error) {
    case 1:
      Format_Runtime_Text(txt, sizeof(txt), Text_String(TXT_CONNECTION_LOST),
                          housep->IniName);
      break;

    case 0:
      Format_Runtime_Text(txt, sizeof(txt), Text_String(TXT_LEFT_GAME),
                          housep->IniName);
      break;

    case -1:
      NullModem.Delete_Connection();
      break;
    default:
      break;
  }

  if (!std::string_view(txt).empty()) {
    Session.Messages.Add_Message(nullptr, 0, txt,
                                 housep->RemapColor == PCOLOR_DIALOG_BLUE
                                     ? PCOLOR_REALLY_BLUE
                                     : housep->RemapColor,
                                 kTpfText, Rule.MessageDelay * kTicksPerMinute);
    Map.Flag_To_Redraw(false);
  }

  /*
  ** Remove this player from the Players vector
  */
  for (int i = 0; i < Session.Players.Count(); i++) {
    if (absl::EqualsIgnoreCase(Session.Players.at(i)->Name, housep->IniName)) {
      delete Session.Players.at(i);
      Session.Players.Delete(Session.Players.at(i));
      break;
    }
  }

  /*
  **	Turn the player's house over to the computer's AI
  */
  housep->IsHuman = false;
  //	housep->Smartness = IQ_MENSA;
  housep->IQ = Rule.MaxIQ;
  port::SafeCopy(housep->IniName, Text_String(TXT_COMPUTER));

  Session.NumPlayers--;

  /*
  **	If we're the last player left, tell the user.
  */
  if (Session.NumPlayers == 1) {
    absl::SNPrintF(txt, sizeof(txt), "%s", Text_String(TXT_JUST_YOU_AND_ME));
    Session.Messages.Add_Message(nullptr, 0, txt,
                                 housep->RemapColor == PCOLOR_DIALOG_BLUE
                                     ? PCOLOR_REALLY_BLUE
                                     : housep->RemapColor,
                                 kTpfText, Rule.MessageDelay * kTicksPerMinute);
    Map.Flag_To_Redraw(false);
  }
}

/***************************************************************************
 * Select_Serial_Dialog -- Serial Communications menu dialog               *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		GAME_MODEM				user wants to play a
 *modem game						* GAME_NULL_MODEM
 *user wants to play a null-modem game				* GAME_NORMAL
 *user hit Cancel
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   04/29/1995 BRR : Created.                                             *
 *=========================================================================*/
GameType Select_Serial_Dialog() {

  /*
  ** Dialog & button dimensions
  */
  const int d_dialog_w = 320;                     // dialog width
  const int d_dialog_h = 188;                     // dialog height
  const int d_dialog_x = (640 - d_dialog_w) / 2;  // dialog x-coord
  const int d_dialog_y = 160;
  //	int d_dialog_y = ((136 * 2 - d_dialog_h) / 2);	// dialog
  // y-coord
  const int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);  // center x-coord

  const int d_txt6_h = 14;  // ht of 6-pt text
  const int d_margin = 14;  // margin width/height

  const int d_dial_w = 180;
  const int d_dial_h = 18;
  const int d_dial_x = d_dialog_cx - (d_dial_w / 2);
  const int d_dial_y = d_dialog_y + d_margin + d_txt6_h + d_margin;

  const int d_answer_w = 180;
  const int d_answer_h = 18;
  const int d_answer_x = d_dialog_cx - (d_answer_w / 2);
  const int d_answer_y = d_dial_y + d_dial_h + 2;

  const int d_nullmodem_w = 180;
  const int d_nullmodem_h = 18;
  const int d_nullmodem_x = d_dialog_cx - (d_nullmodem_w / 2);
  const int d_nullmodem_y = d_answer_y + d_answer_h + 2;

  const int d_settings_w = 180;
  const int d_settings_h = 18;
  const int d_settings_x = d_dialog_cx - (d_settings_w / 2);
  const int d_settings_y = d_nullmodem_y + d_nullmodem_h + 2;

  const int d_cancel_w = 100;
  const int d_cancel_h = 18;
  const int d_cancel_x = d_dialog_cx - (d_cancel_w / 2);
  const int d_cancel_y = d_settings_y + d_settings_h + d_margin;

  /*
  ** Button Enumerations
  */
  constexpr int kButtonDial = 100;
  constexpr int kButtonAnswer = 101;
  constexpr int kButtonNullmodem = 102;
  constexpr int kButtonSettings = 103;
  constexpr int kButtonCancel = 104;
  constexpr int kNumOfButtons = 5;

  /*
  ** Redraw values: in order from "top" to "bottom" layer of the dialog
  */
  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_BUTTONS = 1,
    REDRAW_BACKGROUND = 2,
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;

  /*
  ** Dialog variables
  */
  RedrawType display = REDRAW_ALL;  // redraw level
  bool process = true;              // process while true
  GameType retval = GAME_NORMAL;  // return value

  int selection = 0;
  TextButtonClass* buttons[kNumOfButtons];

  SerialSettingsType* settings = nullptr;
  bool selectsettings = false;
  RemapControlType* scheme = GadgetClass::Get_Color_Scheme();

  /*
  ** Buttons
  */

  TextButtonClass dialbtn(kButtonDial, TXT_DIAL_MODEM, kTpfButton, d_dial_x,
                          d_dial_y, d_dial_w, d_dial_h);
  TextButtonClass answerbtn(kButtonAnswer, TXT_ANSWER_MODEM, kTpfButton,
                            d_answer_x, d_answer_y, d_answer_w, d_answer_h);
  TextButtonClass nullmodembtn(kButtonNullmodem, TXT_NULL_MODEM, kTpfButton,
                               d_nullmodem_x, d_nullmodem_y, d_nullmodem_w,
                               d_nullmodem_h);
  TextButtonClass settingsbtn(kButtonSettings, TXT_SETTINGS, kTpfButton,
                              d_settings_x, d_settings_y, d_settings_w,
                              d_settings_h);
  TextButtonClass cancelbtn(kButtonCancel, TXT_CANCEL, kTpfButton, d_cancel_x,
                            d_cancel_y, d_cancel_w, d_cancel_h);

  /*
  ** Initialize
  */
  Set_Logic_Page(SeenBuff);

  if (Session.SerialDefaults.Port == 0 || Session.SerialDefaults.IRQ == -1 ||
      Session.SerialDefaults.Baud == -1 ||
      NullModemClass::Detect_Port(&Session.SerialDefaults) != PORT_VALID) {
    selectsettings = true;
  }

  /*
  ** Create the list
  */
  GadgetClass* commands = &dialbtn;  // button list
  answerbtn.Add_Tail(*commands);
  nullmodembtn.Add_Tail(*commands);
  settingsbtn.Add_Tail(*commands);
  cancelbtn.Add_Tail(*commands);

  /*
  ** Fill array of button ptrs
  */
  int curbutton = 0;
  buttons[0] = &dialbtn;
  buttons[1] = &answerbtn;
  buttons[2] = &nullmodembtn;
  buttons[3] = &settingsbtn;
  buttons[4] = &cancelbtn;
  base::At(buttons, curbutton)->Turn_On();

  Keyboard->Clear();

  Fancy_Text_Print(TXT_NONE, 0, 0, scheme, kTBlack, TPF_CENTER | kTpfText);

  /*
  ** Main Processing Loop
  */
  display = REDRAW_ALL;
  process = true;
  bool pressed = false;
  while (process) {
    /*
    ** Invoke game callback
    */
    ServiceRealTime();

    /*
    ** Refresh display if needed
    */
    /*
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      display = REDRAW_ALL;
    }

    if (display != REDRAW_NONE) {
      Hide_Mouse();
      if (display >= REDRAW_BACKGROUND) {
        /*
        ** Refresh the backdrop
        */
        Load_Title_Page(true);
        /*
        ** Draw the background
        */
        Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);
        /*
        ** Draw the labels
        */
        Draw_Caption(TXT_SELECT_SERIAL_GAME, d_dialog_x, d_dialog_y,
                     d_dialog_w);
      }
      commands->Draw_All();
      Show_Mouse();
      display = REDRAW_NONE;
    }

    /*
    ** Get user input
    */
    const KeyNumType input = commands->Input();

    /*
    ** Process input
    */
    switch (static_cast<int>(input)) {
      case ButtonKey(kButtonDial):
        selection = kButtonDial;
        pressed = true;
        break;

      case ButtonKey(kButtonAnswer):
        selection = kButtonAnswer;
        pressed = true;
        break;

      case ButtonKey(kButtonNullmodem):
        selection = kButtonNullmodem;
        pressed = true;
        break;

      case ButtonKey(kButtonSettings):
        selection = kButtonSettings;
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
          curbutton = kNumOfButtons - 1;
        }
        base::At(buttons, curbutton)->Turn_On();
        base::At(buttons, curbutton)->Flag_To_Redraw();
        break;

      case KN_DOWN:
        base::At(buttons, curbutton)->Turn_Off();
        base::At(buttons, curbutton)->Flag_To_Redraw();
        curbutton++;
        if (curbutton > kNumOfButtons - 1) {
          curbutton = 0;
        }
        base::At(buttons, curbutton)->Turn_On();
        base::At(buttons, curbutton)->Flag_To_Redraw();
        break;

      case KN_RETURN:
        selection = curbutton + kButtonDial;
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
      curbutton = selection - kButtonDial;
      base::At(buttons, curbutton)->Turn_On();
      base::At(buttons, curbutton)->IsPressed = true;
      base::At(buttons, curbutton)->Draw_Me(true);

      switch (selection) {
        case kButtonDial:

          if (selectsettings) {
            WWMessageBox().Process(TXT_SELECT_SETTINGS);

            /*
            ** Remote-connect
            */
          } else if (Phone_Dialog()) {
            if (Session.PhoneBook.at(Session.CurPhoneIdx)->Settings.Port == 0) {
              settings = &Session.SerialDefaults;
            } else {
              settings = &Session.PhoneBook.at(Session.CurPhoneIdx)->Settings;
            }

            delete SerialPort;

            SerialPort = new WinModemClass;
            if (Init_Null_Modem(settings)) {
              if (settings->CallWaitStringIndex == kCallWaitCustom) {
                DialString = settings->CallWaitString;
              } else {
                DialString = base::At(SessionClass::CallWaitStrings,
                                      settings->CallWaitStringIndex);
              }
              DialString += Session.PhoneBook.at(Session.CurPhoneIdx)->Number;

              if (Dial_Modem(settings, false)) {
                Session.ModemType = MODEM_DIALER;
                if (Com_Scenario_Dialog()) {
                  retval = GAME_MODEM;
                  process = false;
                }
              }

              if (process) {  // restore to default
                NullModemClass::Change_IRQ_Priority(0);
              }
            } else {
              WWMessageBox().Process(TXT_SELECT_SETTINGS);
            }
          }

          if (process) {
            base::At(buttons, curbutton)->IsPressed = false;
            base::At(buttons, curbutton)->Flag_To_Redraw();
          }

          display = REDRAW_ALL;
          break;

        case kButtonAnswer:

          if (selectsettings) {
            WWMessageBox().Process(TXT_SELECT_SETTINGS);
          } else {
            /*
            ** Remote-connect
            */
            settings = &Session.SerialDefaults;
            delete SerialPort;
            SerialPort = new WinModemClass;
            if (Init_Null_Modem(settings)) {
              if (Answer_Modem(settings, false)) {
                Session.ModemType = MODEM_ANSWERER;
                if (Com_Show_Scenario_Dialog()) {
                  retval = GAME_MODEM;
                  process = false;
                }
              }

              if (process) {  // restore to default
                NullModemClass::Change_IRQ_Priority(0);
              }
            } else {
              WWMessageBox().Process(TXT_SELECT_SETTINGS);
            }
          }

          if (process) {
            base::At(buttons, curbutton)->IsPressed = false;
            base::At(buttons, curbutton)->Flag_To_Redraw();
          }

          display = REDRAW_ALL;
          break;

        case kButtonNullmodem:
          /*
          ** Remote-connect unless the settings still need selecting; save
          ** values if we're recording
          */
          if (!selectsettings && Init_Null_Modem(&Session.SerialDefaults)) {
            const int rc = Test_Null_Modem();
            switch (rc) {
              case 1:
                Session.ModemType = MODEM_NULL_HOST;
                if (Com_Scenario_Dialog()) {
                  retval = GAME_NULL_MODEM;
                  process = false;
                }
                break;

              case 2:
                Session.ModemType = MODEM_NULL_JOIN;
                if (Com_Show_Scenario_Dialog()) {
                  retval = GAME_NULL_MODEM;
                  process = false;
                }
                break;

              case 3:
                WWMessageBox().Process(TXT_MODEM_OR_LOOPBACK);
                break;
              default:
                break;
            }

            if (process) {  // restore to default
              NullModemClass::Change_IRQ_Priority(0);
            }
          } else {
            WWMessageBox().Process(TXT_SELECT_SETTINGS);
          }

          if (process) {
            base::At(buttons, curbutton)->IsPressed = false;
            base::At(buttons, curbutton)->Flag_To_Redraw();
          }

          display = REDRAW_ALL;
          break;

        case kButtonSettings:
          if (Com_Settings_Dialog(&Session.SerialDefaults)) {
            Session.Write_MultiPlayer_Settings();

            selectsettings = true;

            if ((Session.SerialDefaults.Port != 0 &&
                 Session.SerialDefaults.IRQ != -1 &&
                 Session.SerialDefaults.Baud != -1) &&
                (NullModemClass::Detect_Port(&Session.SerialDefaults) ==
                 PORT_VALID)) {
              selectsettings = false;
            }
          }

          base::At(buttons, curbutton)->IsPressed = false;
          base::At(buttons, curbutton)->Flag_To_Redraw();
          display = REDRAW_ALL;
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
  } /* end of while */

  return retval;
}

/***********************************************************************************************
 * Advanced_Modem_Settings -- Allows to user to set additional modem settings *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    current settings *
 *                                                                                             *
 * OUTPUT:   modified settings *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 12/16/96 2:29PM ST : Created *
 *=============================================================================================*/
static void Advanced_Modem_Settings(SerialSettingsType* settings) {
  /*........................................................................
  Dialog & button dimensions
  ........................................................................*/
  const int d_dialog_w = config::kIsEnglish ? 340 : 440;  // dialog width

  const int d_dialog_h = 200;                     // dialog height
  const int d_dialog_x = 320 - (d_dialog_w / 2);  // dialog x-coord
  const int d_dialog_y = 200 - (d_dialog_h / 4);  // dialog y-coord

  const int d_compression_w = 50;
  const int d_compression_h = 18;
  const int d_compression_x = d_dialog_x + (d_dialog_w / 2) + 40;
  const int d_compression_y = d_dialog_y + 40;

  const int d_errorcorrection_w = 50;
  const int d_errorcorrection_h = 18;
  const int d_errorcorrection_x = d_dialog_x + (d_dialog_w / 2) + 40;
  const int d_errorcorrection_y = d_dialog_y + 65;

  const int d_hardwareflowcontrol_w = 50;
  const int d_hardwareflowcontrol_h = 18;
  const int d_hardwareflowcontrol_x = d_dialog_x + (d_dialog_w / 2) + 40;
  const int d_hardwareflowcontrol_y = d_dialog_y + 90;

  const int d_default_w = 100;
  const int d_default_h = 18;
  const int d_default_x = d_dialog_x + (d_dialog_w / 2) - (d_default_w / 2);
  const int d_default_y = d_dialog_y + d_dialog_h - 70;

  const int d_ok_w = 100;
  const int d_ok_h = 18;
  const int d_ok_x = d_dialog_x + (d_dialog_w / 2) - (d_ok_w / 2);
  const int d_ok_y = d_dialog_y + d_dialog_h - 40;

  constexpr int kButtonCompression = 100;
  constexpr int kButtonErrorCorrection = 101;
  constexpr int kButtonHardwareFlowControl = 102;
  constexpr int kButtonDefault = 103;
  constexpr int kButtonOk = 104;

  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_BUTTONS = 1,
    REDRAW_BACKGROUND = 2,
    REDRAW_ALL = REDRAW_BACKGROUND,
  };
  using enum RedrawType;

  /*
  ** Yes/No strings
  */
  char compress_text[16];
  char correction_text[16];
  char flowcontrol_text[16];

  RemapControlType* scheme = GadgetClass::Get_Color_Scheme();

  /*
  ** Initialise the button text
  */
  port::SafeCopy(compress_text, settings->Compression ? Text_String(TXT_ON)
                                                      : Text_String(TXT_OFF));
  port::SafeCopy(correction_text, settings->ErrorCorrection
                                      ? Text_String(TXT_ON)
                                      : Text_String(TXT_OFF));
  port::SafeCopy(flowcontrol_text, settings->HardwareFlowControl
                                       ? Text_String(TXT_ON)
                                       : Text_String(TXT_OFF));

  /*
  ** Create the buttons
  */
  TextButtonClass compressionbutton(
      kButtonCompression, compress_text,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
      d_compression_x, d_compression_y, d_compression_w, d_compression_h);

  TextButtonClass errorcorrectionbutton(
      kButtonErrorCorrection, correction_text,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
      d_errorcorrection_x, d_errorcorrection_y, d_errorcorrection_w,
      d_errorcorrection_h);

  TextButtonClass hardwareflowcontrolbutton(
      kButtonHardwareFlowControl, flowcontrol_text,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
      d_hardwareflowcontrol_x, d_hardwareflowcontrol_y, d_hardwareflowcontrol_w,
      d_hardwareflowcontrol_h);

  TextButtonClass defaultbutton(
      kButtonDefault, TXT_DEFAULT,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, d_default_x,
      d_default_y, d_default_w, d_default_h);

  TextButtonClass okbutton(
      kButtonOk, TXT_OK,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, d_ok_x,
      d_ok_y, d_ok_w, d_ok_h);

  /*
  ** Misc. variables.
  */
  RedrawType display = REDRAW_ALL;  // redraw level
  bool process = true;              // process while true

  GadgetClass* commands = &okbutton;  // button list
  defaultbutton.Add_Tail(*commands);
  compressionbutton.Add_Tail(*commands);
  errorcorrectionbutton.Add_Tail(*commands);
  hardwareflowcontrolbutton.Add_Tail(*commands);

  /*
  ** Main process loop
  */
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
    ServiceRealTime();

    /*
    ...................... Refresh display if needed ......................
    */
    if (display != REDRAW_NONE) {
      Hide_Mouse();
      /*
      .................. Redraw backgound & dialog box ...................
      */
      if (display >= REDRAW_BACKGROUND) {
        Load_Title_Page(true);
        CCPalette.Set();

        Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);

        // init font variables

        Fancy_Text_Print(TXT_NONE, 0, 0, scheme, kTBlack, kTpfText);

        /*...............................................................
        Dialog & Field labels
        ...............................................................*/
        Draw_Caption(TXT_MODEM_INITIALISATION, d_dialog_x, d_dialog_y,
                     d_dialog_w);

        Fancy_Text_Print(TXT_DATA_COMPRESSION, d_compression_x - 26,
                         d_compression_y + 2, scheme, kTBlack,
                         kTpfText | TPF_RIGHT);

        Fancy_Text_Print(TXT_ERROR_CORRECTION, d_errorcorrection_x - 26,
                         d_errorcorrection_y + 2, scheme, kTBlack,
                         kTpfText | TPF_RIGHT);

        Fancy_Text_Print(
            TXT_HARDWARE_FLOW_CONTROL, d_hardwareflowcontrol_x - 26,
            d_hardwareflowcontrol_y + 2, scheme, kTBlack, kTpfText | TPF_RIGHT);
      }

      /*
      .......................... Redraw buttons ..........................
      */
      if (display >= REDRAW_BUTTONS) {
        compressionbutton.Flag_To_Redraw();
        errorcorrectionbutton.Flag_To_Redraw();
        hardwareflowcontrolbutton.Flag_To_Redraw();
      }

      Show_Mouse();
      display = REDRAW_NONE;
    }

    /*
    ........................... Get user input ............................
    */
    const KeyNumType input = commands->Input();

    /*
    ---------------------------- Process input ----------------------------
    */
    switch (static_cast<int>(input)) {
      case ButtonKey(kButtonCompression):
        settings->Compression = !settings->Compression;
        port::SafeCopy(compress_text, settings->Compression
                                          ? Text_String(TXT_ON)
                                          : Text_String(TXT_OFF));
        display = std::max(display, REDRAW_BUTTONS);
        break;

      case ButtonKey(kButtonErrorCorrection):
        settings->ErrorCorrection = !settings->ErrorCorrection;
        port::SafeCopy(correction_text, settings->ErrorCorrection
                                            ? Text_String(TXT_ON)
                                            : Text_String(TXT_OFF));
        display = std::max(display, REDRAW_BUTTONS);
        break;

      case ButtonKey(kButtonHardwareFlowControl):
        settings->HardwareFlowControl = !settings->HardwareFlowControl;
        port::SafeCopy(flowcontrol_text, settings->HardwareFlowControl
                                             ? Text_String(TXT_ON)
                                             : Text_String(TXT_OFF));
        display = std::max(display, REDRAW_BUTTONS);
        break;

      case ButtonKey(kButtonDefault):
        settings->Compression = false;
        settings->ErrorCorrection = false;
        settings->HardwareFlowControl = true;

        port::SafeCopy(compress_text, settings->Compression
                                          ? Text_String(TXT_ON)
                                          : Text_String(TXT_OFF));

        port::SafeCopy(correction_text, settings->ErrorCorrection
                                            ? Text_String(TXT_ON)
                                            : Text_String(TXT_OFF));

        port::SafeCopy(flowcontrol_text, settings->HardwareFlowControl
                                             ? Text_String(TXT_ON)
                                             : Text_String(TXT_OFF));

        display = std::max(display, REDRAW_BUTTONS);
        break;

      case ButtonKey(kButtonOk):
        process = false;
        break;
      default:
        break;
    }
  }
}

/***************************************************************************
 * Com_Settings_Dialog -- Lets user select serial port settings            *
 *                                                                         *
 *  ┌──────────────────────────────────────────────────────┐               *
 *  │                    Settings                          │               *
 *  │                                                      │               *
 *  │     Port:____       IRQ:__        Baud:______        │               *
 *  │  ┌────────────┐  ┌────────────┐  ┌────────────┐      │               *
 *  │  │            │  │            │  │            │      │               *
 *  │  │            │  │            │  │            │      │               *
 *  │  │            │  │            │  │            │      │               *
 *  │  │            │  │            │  │            │      │               *
 *  │  └────────────┘  └────────────┘  └────────────┘      │               *
 *  │                                                      │               *
 *  │   Initialization:        [Add]   [Delete]            │               *
 *  │    _____________________________                     │               *
 *  │   ┌────────────────────────────────────────────┐     │               *
 *  │   │                                            │     │               *
 *  │   │                                            │     │               *
 *  │   │                                            │     │               *
 *  │   └────────────────────────────────────────────┘     │               *
 *  │                                                      │               *
 *  │   Call Waiting:                                      │               *
 *  │    _______________                                   │               *
 *  │   ┌─────────────────┐          [Tone Dialing]        │               *
 *  │   │                 │                                │               *
 *  │   │                 │          [Pulse Dialing]       │               *
 *  │   │                 │                                │               *
 *  │   └─────────────────┘                                │               *
 *  │                                                      │               *
 *  │                   [OK]   [Cancel]                    │               *
 *  └──────────────────────────────────────────────────────┘               *
 *                                                                         *
 * INPUT:                                                                  *
 *		settings		ptr to SerialSettingsType structure
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		true = OK, false = Cancel
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   04/29/1995 BRR : Created.                                             *
 *=========================================================================*/
static int Com_Settings_Dialog(SerialSettingsType* settings) {
  /*
  ** Dialog & button dimensions
  */
  const int d_dialog_w = 640;                             // dialog width
  const int d_dialog_h = 400;                             // dialog height
  const int d_dialog_x = (640 - d_dialog_w) / 2;          // dialog x-coord
  const int d_dialog_y = (400 - d_dialog_h) / 2;          // dialog y-coord
  const int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);  // center x-coord

  const int d_txt6_h = 14;  // ht of 6-pt text
  const int d_margin = 10;  // margin width/height

  const int d_portlist_w = 240;  // Port list wider in hires
  const int d_portlist_h = 66;
  const int d_portlist_x = 0x45;
  const int d_portlist_y =
      d_dialog_y + ((d_margin + d_txt6_h) * 2) + d_margin + 20;

  const int d_port_w = d_portlist_w;
  const int d_port_x = 0x45;
  const int d_port_h = 18;
  const int d_port_y = d_portlist_y - d_margin - d_txt6_h;

  const int d_irqlist_y = d_portlist_y;

  const int d_baudlist_w = 160;
  const int d_baudlist_h = 66;
  int d_baudlist_x = d_dialog_x + (d_dialog_w * 5 / 6) - (d_baudlist_w / 2);
  d_baudlist_x -= 32;
  const int d_baudlist_y = d_irqlist_y;

  const int d_baud_w = ((BAUDBUF_MAX - 1) * 12) + 6;
  const int d_baud_h = 18;
  const int d_baud_x = d_baudlist_x + 58;
  const int d_baud_y = d_baudlist_y - d_margin - d_txt6_h;

  const int d_initstrlist_w = ((INITSTRBUF_MAX - 1) * 12) + 16 + 6;
  const int d_initstrlist_h = 42;
  const int d_initstrlist_x = d_dialog_cx - (d_initstrlist_w / 2);
  const int d_initstrlist_y =
      d_portlist_y + d_portlist_h + ((d_margin + d_txt6_h) * 2) + d_margin + 4;

  const int d_initstr_w = ((INITSTRBUF_MAX - 1) * 12) + 6;
  const int d_initstr_h = 18;
  const int d_initstr_x = d_initstrlist_x;
  const int d_initstr_y = d_initstrlist_y - d_margin - d_txt6_h;

  const int d_add_w = 90;
  const int d_add_x =
      d_dialog_cx - (d_add_w / 2) + (config::kIsFrench ? 0 : 30);
  const int d_add_h = 18;
  const int d_add_y = d_initstr_y - d_add_h - 6;

  const int d_delete_w = 90;
  const int d_delete_x = d_dialog_x + (d_dialog_w * 3 / 4) - (d_delete_w / 2) +
                         (config::kIsFrench ? 10 : 0);
  const int d_delete_h = 18;
  const int d_delete_y = d_initstr_y - d_add_h - 6;

  const int d_cwaitstrlist_w = ((CWAITSTRBUF_MAX - 1 + 9) * 12) + 6;
  const int d_cwaitstrlist_h = 54;
  const int d_cwaitstrlist_x = d_initstrlist_x;
  const int d_cwaitstrlist_y =
      d_initstrlist_y + d_initstrlist_h + ((d_margin + d_txt6_h) * 2) + 2;

  const int d_cwaitstr_w = ((CWAITSTRBUF_MAX - 1) * 12) + 6;
  const int d_cwaitstr_h = 18;
  const int d_cwaitstr_x = d_cwaitstrlist_x;
  const int d_cwaitstr_y = d_cwaitstrlist_y - d_margin - d_txt6_h;

  const int d_tone_w = 160;
  const int d_tone_h = 18;
  const int d_tone_x = d_dialog_x + (d_dialog_w * 3 / 4) - (d_tone_w / 2);
  const int d_tone_y = d_cwaitstrlist_y;

  const int d_pulse_w = 160;
  const int d_pulse_h = 18;
  const int d_pulse_x = d_dialog_x + (d_dialog_w * 3 / 4) - (d_pulse_w / 2);
  const int d_pulse_y = d_tone_y + d_tone_h + d_margin;

  const int d_save_w = config::kIsFrench ? 160 : 80;
  const int d_save_h = 18;
  const int d_save_x = d_dialog_x + (d_dialog_w / 5) - (d_save_w / 2);
  const int d_save_y = d_dialog_y + d_dialog_h - d_save_h - d_margin - 8;

  const int d_cancel_w = 100;
  const int d_cancel_h = 18;
  const int d_cancel_x = d_dialog_x + (d_dialog_w * 4 / 5) - (d_cancel_w / 2);
  const int d_cancel_y = d_dialog_y + d_dialog_h - d_cancel_h - d_margin - 8;

  const int d_advanced_w = config::kIsEnglish ? 80 : 100;
  const int d_advanced_h = 18;
  const int d_advanced_x = d_dialog_x + (d_dialog_w / 2) - (d_advanced_w / 2);
  const int d_advanced_y =
      d_dialog_y + d_dialog_h - d_advanced_h - d_margin - 8;

  /*
  ** Button Enumerations
  */
  constexpr int kButtonPort = 100;
  constexpr int kButtonPortlist = 101;
  constexpr int kButtonBaud = 104;
  constexpr int kButtonBaudlist = 105;
  constexpr int kButtonInitstr = 106;
  constexpr int kButtonInitstrlist = 107;
  constexpr int kButtonAdd = 108;
  constexpr int kButtonDelete = 109;
  constexpr int kButtonCwaitstr = 110;
  constexpr int kButtonCwaitstrlist = 111;
  constexpr int kButtonTone = 112;
  constexpr int kButtonPulse = 113;
  constexpr int kButtonSave = 114;
  constexpr int kButtonAdvanced = 115;
  constexpr int kButtonCancel = 116;

  /*
  ** Redraw values: in order from "top" to "bottom" layer of the dialog
  */
  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_BUTTONS = 1,
    REDRAW_BACKGROUND = 2,
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;

  static char custom_port[10 + MODEM_NAME_MAX] = {"CUSTOM - ????"};


  static const char* baudname[5] = {
      "14400", "19200", "28800", "38400", "57600",
  };

  static char modemnames[10][MODEM_NAME_MAX];

  /*
  ** Dialog variables
  */
  RedrawType display = REDRAW_ALL;  // redraw level
  bool process = true;              // process while true
  char* item = nullptr;             // general-purpose string
  size_t temp = 0;                  // general-purpose string
  RemapControlType* scheme = GadgetClass::Get_Color_Scheme();

  char portbuf[PORTBUF_MAX] = {0};          // buffer for port
  char baudbuf[BAUDBUF_MAX] = {0};          // buffer for baud
  char initstrbuf[INITSTRBUF_MAX] = {0};    // buffer for init string
  char cwaitstrbuf[CWAITSTRBUF_MAX] = {0};  // buffer for call waiting string

  int port_index = 1;  // index of currently-selected port (default = com2)
  int port_custom_index = 4;  // index of custom entry in port list
  int baud_index = 1;  // index of currently-selected baud (default = 19200)
  int initstr_index =
      0;  // index of currently-selected modem init (default = "ATZ")
  int cwaitstr_index = kCallWaitCustom;   // index of currently-selected call
                                          // waiting (default = "")
  int rc = 0;                             // -1 = user cancelled, 1 = New
  int pos = 0;
  int len = 0;
  bool firsttime = true;
  SerialSettingsType tempsettings{};
  /*
  ** Buttons
  */

  EditClass port_edt(kButtonPort, portbuf, PORTBUF_MAX, kTpfText, d_port_x,
                     d_port_y, d_port_w, d_port_h, EditClass::kAlphanumeric);

  ListClass portlist(kButtonPortlist, d_portlist_x, d_portlist_y, d_portlist_w,
                     d_portlist_h, kTpfText,
                     MixArchive::RetrieveData("BTN-UP.SHP"),
                     MixArchive::RetrieveData("BTN-DN.SHP"));

  EditClass baud_edt(kButtonBaud, baudbuf, BAUDBUF_MAX, kTpfText, d_baud_x,
                     d_baud_y, d_baud_w, d_baud_h, EditClass::kNumeric);
  ListClass baudlist(kButtonBaudlist, d_baudlist_x, d_baudlist_y, d_baudlist_w,
                     d_baudlist_h, kTpfText,
                     MixArchive::RetrieveData("BTN-UP.SHP"),
                     MixArchive::RetrieveData("BTN-DN.SHP"));
  EditClass initstr_edt(kButtonInitstr, initstrbuf, INITSTRBUF_MAX, kTpfText,
                        d_initstr_x, d_initstr_y, d_initstr_w, d_initstr_h,
                        EditClass::kAlphanumeric);
  ListClass initstrlist(kButtonInitstrlist, d_initstrlist_x, d_initstrlist_y,
                        d_initstrlist_w, d_initstrlist_h, kTpfText,
                        MixArchive::RetrieveData("BTN-UP.SHP"),
                        MixArchive::RetrieveData("BTN-DN.SHP"));
  TextButtonClass addbtn(kButtonAdd, TXT_ADD, kTpfButton, d_add_x, d_add_y,
                         d_add_w, d_add_h);
  TextButtonClass deletebtn(kButtonDelete, TXT_DELETE_BUTTON, kTpfButton,
                            d_delete_x, d_delete_y, d_delete_w, d_delete_h);
  EditClass cwaitstr_edt(kButtonCwaitstr, cwaitstrbuf, CWAITSTRBUF_MAX,
                         kTpfText, d_cwaitstr_x, d_cwaitstr_y, d_cwaitstr_w,
                         d_cwaitstr_h, EditClass::kAlphanumeric);
  ListClass cwaitstrlist(kButtonCwaitstrlist, d_cwaitstrlist_x,
                         d_cwaitstrlist_y, d_cwaitstrlist_w, d_cwaitstrlist_h,
                         kTpfText, MixArchive::RetrieveData("BTN-UP.SHP"),
                         MixArchive::RetrieveData("BTN-DN.SHP"));
  TextButtonClass tonebtn(kButtonTone, TXT_TONE_BUTTON, kTpfButton, d_tone_x,
                          d_tone_y, d_tone_w, d_tone_h);
  TextButtonClass pulsebtn(kButtonPulse, TXT_PULSE_BUTTON, kTpfButton,
                           d_pulse_x, d_pulse_y, d_pulse_w, d_pulse_h);
  TextButtonClass savebtn(kButtonSave, TXT_SAVE_BUTTON, kTpfButton, d_save_x,
                          d_save_y, d_save_w, d_save_h);
  TextButtonClass cancelbtn(kButtonCancel, TXT_CANCEL, kTpfButton, d_cancel_x,
                            d_cancel_y, d_cancel_w, d_cancel_h);
  TextButtonClass advancedbutton(kButtonAdvanced, TXT_ADVANCED, kTpfButton,
                                 d_advanced_x, d_advanced_y, d_advanced_w,
                                 d_advanced_h);
  /*
  ** Various Inits
  */
  tempsettings = *settings;

  if (tempsettings.Port == 0) {
    tempsettings.Port = 0x2f8;
  }

  if (tempsettings.IRQ == -1) {
    tempsettings.IRQ = 3;
  }

  if (tempsettings.Baud == -1) {
    tempsettings.Baud = 19200;
  }

  /*
  ** Set the current indices
  */

  if (tempsettings.Baud == 14400) {
    baud_index = 0;
  } else if (tempsettings.Baud == 19200) {
    baud_index = 1;
  } else if (tempsettings.Baud == 28800) {
    baud_index = 2;
  } else if (tempsettings.Baud == 38400) {
    baud_index = 3;
  } else {
    baud_index = 4;
  }
  absl::SNPrintF(baudbuf, sizeof(baudbuf), "%d", tempsettings.Baud);

  /*
  ** Set up the port list box & edit box
  */

  /*
  ** Loop through the first 10 possible modem entries in the registry. Frankly,
  *its just
  ** tough luck if the user has more than 10 modems attached!
  */
  delete ModemRegistry;

  int modems_found = 0;
  for (int i = 0; i < 10; i++) {
    ModemRegistry = new ModemRegistryEntryClass(i);
    if (ModemRegistry->Get_Modem_Name()) {
      port::SafeCopy(base::At(modemnames, modems_found),
                     ModemRegistry->Get_Modem_Name());
      portlist.Add_Item(base::At(modemnames, modems_found++));
      port_custom_index++;
    }
    delete ModemRegistry;
  }
  ModemRegistry = nullptr;


  portlist.Add_Item(custom_port);

  /*
  ** Work out the current port index
  */
  port_index = -1;
  if (tempsettings.ModemName[0]) {
    for (int i = 0; i < port_custom_index; i++) {
      if (absl::EqualsIgnoreCase(portlist.Get_Item(i),
                                 tempsettings.ModemName)) {
        port_index = i;
        port::SafeCopy(portbuf, tempsettings.ModemName);
        break;
      }
    }
    /*
    ** The modem name specified wasnt in the registry so add it as a custom
    *entry
    */
    if (port_index == -1) {
      temp = std::string_view(custom_port).find('-');
      if (temp != std::string_view::npos) {
        pos = static_cast<int>(temp) + 2;
        len = static_cast<int>(std::string_view(tempsettings.ModemName).size());
        port::SafeCopy(std::span(custom_port).subspan(base::ToSize(pos)),
                       tempsettings.ModemName);
        base::At(custom_port, pos + len) = 0;
        port::SafeCopy(portbuf, tempsettings.ModemName);
        port_index = port_custom_index;
      }
    }
  }

  if (port_index == -1) {
    switch (tempsettings.Port) {
      case 0x3f8:
        port_index = 0;
        port::SafeCopy(portbuf, "COM1");
        break;

      case 0x2f8:
        port_index = 1;
        port::SafeCopy(portbuf, "COM2");
        break;

      case 0x3e8:
        port_index = 2;
        port::SafeCopy(portbuf, "COM3");
        break;

      case 0x2e8:
        port_index = 3;
        port::SafeCopy(portbuf, "COM4");
        break;

      default:
        port_index = port_custom_index;
        absl::SNPrintF(portbuf, sizeof(portbuf), "%x",
                       static_cast<unsigned int>(tempsettings.Port));
        temp = std::string_view(custom_port).find('-');
        if (temp != std::string_view::npos) {
          pos = static_cast<int>(temp) + 2;
          len = static_cast<int>(std::string_view(portbuf).size());
          port::SafeCopy(std::span(custom_port).subspan(base::ToSize(pos)),
                         portbuf);
          base::At(custom_port, pos + len) = 0;
        }
        break;
    }
  }

  // The list copied custom_port when it was added; show what the settings
  // wrote into it since.
  portlist.Set_Item(port_custom_index, custom_port);
  portlist.Set_Selected_Index(port_index);

  /*
  ** Set up the port edit box
  */
  port_edt.Set_Text(portbuf, PORTBUF_MAX);
  /*
  ** Set up the baud rate list box & edit box
  */
  for (auto& i : baudname) {
    baudlist.Add_Item(i);
  }

  baudlist.Set_Selected_Index(baud_index);
  baud_edt.Set_Text(baudbuf, BAUDBUF_MAX);

  initstr_index = tempsettings.InitStringIndex;
  Build_Init_String_Listbox(&initstrlist, &initstr_edt, initstrbuf,
                            &initstr_index);

  /*
  ** Set up the cwait rate list box & edit box
  */
  cwaitstr_index = tempsettings.CallWaitStringIndex;
  for (int i = 0; i < kCallWaitStringsNum; i++) {
    if (i == kCallWaitCustom) {
      std::string item_str = base::At(SessionClass::CallWaitStrings, i);
      const size_t dash_pos = item_str.find('-');
      if (dash_pos != std::string::npos) {
        pos = static_cast<int>(dash_pos) + 2;
        item_str.replace(base::ToSize(pos), std::string::npos, tempsettings.CallWaitString);
        if (i == cwaitstr_index) {
          port::SafeCopy(std::span(cwaitstrbuf).first(CWAITSTRBUF_MAX),
                         std::string_view(item_str).substr(base::ToSize(pos)));
        }
      }
      cwaitstrlist.Add_Item(item_str.c_str());
    } else {
      if (i == cwaitstr_index) {
        port::SafeCopy(std::span(cwaitstrbuf).first(CWAITSTRBUF_MAX),
                       base::At(SessionClass::CallWaitStrings, i));
      }
      cwaitstrlist.Add_Item(base::At(SessionClass::CallWaitStrings, i));
    }
  }

  cwaitstrlist.Set_Selected_Index(cwaitstr_index);
  cwaitstr_edt.Set_Text(cwaitstrbuf, CWAITSTRBUF_MAX);

  /*
  ** Build the button list
  */
  GadgetClass* commands = &cancelbtn;  // button list
  port_edt.Add_Tail(*commands);
  portlist.Add_Tail(*commands);
  baud_edt.Add_Tail(*commands);
  baudlist.Add_Tail(*commands);
  initstr_edt.Add_Tail(*commands);
  initstrlist.Add_Tail(*commands);
  addbtn.Add_Tail(*commands);
  deletebtn.Add_Tail(*commands);
  cwaitstr_edt.Add_Tail(*commands);
  cwaitstrlist.Add_Tail(*commands);
  tonebtn.Add_Tail(*commands);
  pulsebtn.Add_Tail(*commands);
  savebtn.Add_Tail(*commands);
  advancedbutton.Add_Tail(*commands);

  if (tempsettings.DialMethod == DIAL_TOUCH_TONE) {
    tonebtn.Turn_On();
  } else {
    pulsebtn.Turn_On();
  }

  /*
  ** Processing loop
  */
  while (process) {
    /*
    ** Invoke game callback
    */
    ServiceRealTime();

    /*
    ** Dont allow editing of non-custom ports to fix the problem of the cursor
    *appearing
    ** outside the edit box.
    */
    if (port_index == port_custom_index) {
      port_edt.Set_Read_Only(false);
    } else {
      port_edt.Set_Read_Only(true);
    }

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
    ** Refresh display if needed
    */
    if (display != REDRAW_NONE) {
      Hide_Mouse();
      /*
      ** Redraw backgound & dialog box
      */
      if (display >= REDRAW_BACKGROUND) {
        Load_Title_Page(true);
        CCPalette.Set();

        Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);

        // init font variables

        Fancy_Text_Print(TXT_NONE, 0, 0, nullptr, kTBlack,
                         TPF_CENTER | kTpfText);

        /*
        ** Dialog & Field labels
        */
        Draw_Caption(TXT_SETTINGS, d_dialog_x, d_dialog_y, d_dialog_w);

        Fancy_Text_Print(TXT_PORT_COLON, d_port_x - 6, d_port_y + 2, scheme,
                         kTBlack, TPF_RIGHT | kTpfText);
        Fancy_Text_Print(TXT_BAUD_COLON, d_baud_x - 6, d_baud_y + 2, scheme,
                         kTBlack, TPF_RIGHT | kTpfText);

        Fancy_Text_Print(TXT_INIT_STRING, d_initstr_x,
                         d_initstr_y - d_txt6_h - 6, scheme, kTBlack, kTpfText);

        Fancy_Text_Print(TXT_CWAIT_STRING, d_cwaitstr_x,
                         d_cwaitstr_y - d_txt6_h - 6, scheme, kTBlack,
                         kTpfText);
      }

      /*
      ** Redraw buttons
      */
      if (display >= REDRAW_BUTTONS) {
        cancelbtn.Flag_To_Redraw();
        port_edt.Flag_To_Redraw();
        portlist.Flag_To_Redraw();
        baud_edt.Flag_To_Redraw();
        baudlist.Flag_To_Redraw();
        advancedbutton.Flag_To_Redraw();
        initstr_edt.Flag_To_Redraw();
        initstrlist.Flag_To_Redraw();
        addbtn.Flag_To_Redraw();
        deletebtn.Flag_To_Redraw();
        cwaitstr_edt.Flag_To_Redraw();
        cwaitstrlist.Flag_To_Redraw();
        tonebtn.Flag_To_Redraw();
        pulsebtn.Flag_To_Redraw();
        savebtn.Flag_To_Redraw();
      }

      Show_Mouse();
      display = REDRAW_NONE;
    }

    /*
    ** Get user input
    */
    KeyNumType input = commands->Input();

    if (firsttime) {
      //			port_edt.Set_Focus();
      port_edt.Flag_To_Redraw();
      input = commands->Input();
      firsttime = false;
    }

    /*
    ** Process input
    */
    switch (static_cast<int>(input)) {
      case ButtonKey(kButtonAdvanced):
        Advanced_Modem_Settings(&tempsettings);
        display = REDRAW_ALL;
        break;

      case ButtonKey(kButtonPort):
        if (port_index < 4) {
          const char* const current = portlist.Current_Item();
          const auto space = std::string_view(current).find(' ');
          if (space == std::string_view::npos) {
            port::SafeCopy(std::span(portbuf).first(PORTBUF_MAX), current);
          } else {
            pos = static_cast<int>(space);
            port::SafeCopy(std::span(portbuf).first(base::ToSize(pos)),
                           current);
          }
          port_edt.Set_Text(portbuf, PORTBUF_MAX);
          port_edt.Flag_To_Redraw();
        } else {
          std::ranges::transform(port::MutableCString(portbuf), portbuf,
                                 absl::ascii_toupper);
          if (absl::EqualsIgnoreCase(portbuf, "3F8")) {
            port_index = 0;
            portlist.Set_Selected_Index(port_index);
            port::SafeCopy(portbuf, "COM1");
            display = REDRAW_BUTTONS;

          } else if (absl::EqualsIgnoreCase(portbuf, "2F8")) {
            port_index = 1;
            portlist.Set_Selected_Index(port_index);
            port::SafeCopy(portbuf, "COM2");
            display = REDRAW_BUTTONS;

          } else if (absl::EqualsIgnoreCase(portbuf, "3E8")) {
            port_index = 2;
            portlist.Set_Selected_Index(port_index);
            port::SafeCopy(portbuf, "COM3");
            display = REDRAW_BUTTONS;

          } else if (absl::EqualsIgnoreCase(portbuf, "2E8")) {
            port_index = 3;
            portlist.Set_Selected_Index(port_index);
            port::SafeCopy(portbuf, "COM4");
            display = REDRAW_BUTTONS;

          } else if (std::string_view(portbuf).starts_with("COM")) {
            display = REDRAW_BUTTONS;

            switch (portbuf[3] - '0') {
              case 1:
                port_index = 0;
                break;

              case 2:
                port_index = 1;
                break;

              case 3:
                port_index = 2;
                break;

              case 4:
                port_index = 3;
                break;

              default:
                if (portbuf[3] <= '9' && portbuf[3] > '0') {
                  portbuf[4] = 0;
                  port_index = port_custom_index;
                  temp = std::string_view(custom_port).find('-');
                  if (temp != std::string_view::npos) {
                    pos = static_cast<int>(temp) + 2;
                    port::SafeCopy(
                        std::span(custom_port)
                            .subspan(base::ToSize(pos))
                            .first(sizeof(custom_port) - base::ToSize(pos)),
                        portbuf);
                    portlist.Set_Item(port_custom_index, custom_port);
                    display = REDRAW_BUTTONS;
                  }
                  break;
                }
                WWMessageBox().Process(TXT_INVALID_PORT_ADDRESS);
                port_edt.Set_Focus();
                display = REDRAW_ALL;
                break;
            }

            portlist.Set_Selected_Index(port_index);

          } else {
            temp = std::string_view(custom_port).find('-');
            if (temp != std::string_view::npos) {
              pos = static_cast<int>(temp) + 2;
              port::SafeCopy(
                  std::span(custom_port)
                      .subspan(base::ToSize(pos))
                      .first(sizeof(custom_port) - base::ToSize(pos)),
                  portbuf);
              portlist.Set_Item(port_custom_index, custom_port);
              display = REDRAW_BUTTONS;
            }
          }
        }
        break;

      case ButtonKey(kButtonPortlist):
        if (portlist.Current_Index() != port_index) {
          port_index = portlist.Current_Index();
          const char* const current = portlist.Current_Item();
          {
            if (port_index == port_custom_index) {
              /*
              ** This is the custom entry
              */
              const auto sep = std::string_view(current).find('-');
              if (sep != std::string_view::npos) {
                pos = static_cast<int>(sep) + 2;
                if (std::string_view(current).at(base::ToSize(pos)) == '?') {
                  portbuf[0] = 0;
                } else {
                  port::SafeCopy(portbuf, std::string_view(current).substr(
                                              base::ToSize(pos)));
                }
              }
              port_edt.Set_Focus();
            } else {
              /*
              ** Must be a modem name entry so just copy iy
              */
              port::SafeCopy(portbuf, current);
            }
          }
          port_edt.Set_Text(portbuf, PORTBUF_MAX);
        } else if (port_index < port_custom_index) {
          port_edt.Clear_Focus();
        } else {
          port_edt.Set_Focus();
        }
        display = REDRAW_BUTTONS;
        break;

      case ButtonKey(kButtonBaud):
        port::SafeCopy(baudbuf, baudlist.Current_Item());
        baud_edt.Set_Text(baudbuf, BAUDBUF_MAX);
        initstr_edt.Set_Focus();
        initstr_edt.Flag_To_Redraw();
        display = REDRAW_BUTTONS;
        break;

      case ButtonKey(kButtonBaudlist):
        if (baudlist.Current_Index() != baud_index) {
          baud_index = baudlist.Current_Index();
          port::SafeCopy(std::span(baudbuf).first(BAUDBUF_MAX),
                         baudlist.Current_Item());
          baud_edt.Set_Text(baudbuf, BAUDBUF_MAX);
          baud_edt.Clear_Focus();
          display = REDRAW_BUTTONS;
        }
        break;

      case ButtonKey(kButtonInitstrlist):
        if (initstrlist.Current_Index() != initstr_index) {
          initstr_index = initstrlist.Current_Index();
          port::SafeCopy(initstrbuf, initstrlist.Current_Item());
          initstr_edt.Set_Text(initstrbuf, INITSTRBUF_MAX);
        }
        initstr_edt.Set_Focus();
        initstr_edt.Flag_To_Redraw();
        display = REDRAW_BUTTONS;
        break;

      /*
      ** Add a new InitString entry
      */
      case ButtonKey(kButtonAdd): {
        item = new char[INITSTRBUF_MAX]{};
        // The allocation immediately above owns exactly INITSTRBUF_MAX bytes.
        // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
        const std::span<char> item_buffer(item, INITSTRBUF_MAX);

        std::ranges::transform(port::MutableCString(initstrbuf), initstrbuf,
                               absl::ascii_toupper);
        port::SafeCopy(item_buffer, initstrbuf);

        Session.InitStrings.Add(item);
        Build_Init_String_Listbox(&initstrlist, &initstr_edt, initstrbuf,
                                  &initstr_index);
        /*............................................................
        Set the current listbox index to the newly-added item.
        ............................................................*/
        for (int i = 0; i < Session.InitStrings.Count(); i++) {
          if (item == Session.InitStrings.at(i)) {
            initstr_index = i;
            port::SafeCopy(initstrbuf, Session.InitStrings.at(initstr_index));
            initstr_edt.Set_Text(initstrbuf, INITSTRBUF_MAX);
            initstrlist.Set_Selected_Index(initstr_index);
          }
        }
        initstr_edt.Set_Focus();
        initstr_edt.Flag_To_Redraw();
        display = REDRAW_BUTTONS;
        break;
      }

      /*------------------------------------------------------------------
      Delete the current InitString entry
      ------------------------------------------------------------------*/
      case ButtonKey(kButtonDelete):

        if (Session.InitStrings.Count() && initstr_index != -1) {
          Session.InitStrings.Delete(initstr_index);
          Build_Init_String_Listbox(&initstrlist, &initstr_edt, initstrbuf,
                                    &initstr_index);
        }
        break;

      case ButtonKey(kButtonCwaitstr):
        if (cwaitstr_index >= kCallWaitCustom) {
          const char* const current = cwaitstrlist.Current_Item();
          const auto dash = std::string_view(current).find('-');
          if (dash != std::string_view::npos) {
            // Keep the "Custom - " prefix, replace what follows it.
            std::string custom(current, dash + 2);
            custom += cwaitstrbuf;
            cwaitstrlist.Set_Item(cwaitstr_index, custom);
            display = REDRAW_BUTTONS;
          }
        }
        break;

      case ButtonKey(kButtonCwaitstrlist):
        if (cwaitstrlist.Current_Index() != cwaitstr_index) {
          cwaitstr_index = cwaitstrlist.Current_Index();
          const char* const current = cwaitstrlist.Current_Item();
          if (cwaitstr_index < 3) {
            port::SafeCopy(cwaitstrbuf, current);
            cwaitstr_edt.Clear_Focus();
          } else {
            const auto sep = std::string_view(current).find('-');
            if (sep != std::string_view::npos) {
              pos = static_cast<int>(sep) + 2;
              port::SafeCopy(cwaitstrbuf, std::string_view(current).substr(
                                              base::ToSize(pos)));
            }
            cwaitstr_edt.Set_Focus();
          }
          cwaitstr_edt.Set_Text(cwaitstrbuf, CWAITSTRBUF_MAX);
        } else if (cwaitstr_index < 3) {
          cwaitstr_edt.Clear_Focus();
        } else {
          cwaitstr_edt.Set_Focus();
        }
        display = REDRAW_BUTTONS;
        break;

      case ButtonKey(kButtonTone):
        tempsettings.DialMethod = DIAL_TOUCH_TONE;
        tonebtn.Turn_On();
        pulsebtn.Turn_Off();
        break;

      case ButtonKey(kButtonPulse):
        tempsettings.DialMethod = DIAL_PULSE;
        tonebtn.Turn_Off();
        pulsebtn.Turn_On();
        break;

      /*------------------------------------------------------------------
      SAVE: save the com settings
      ------------------------------------------------------------------*/
      case KN_RETURN:
      case ButtonKey(kButtonSave): {
        if (port_index == port_custom_index) {
          port::SafeCopy(tempsettings.ModemName, portbuf);
          tempsettings.Port = 1;
        } else {
          /*
          ** Must be a modem name index
          */
          port::SafeCopy(tempsettings.ModemName, portlist.Current_Item());
          tempsettings.Port = 1;
        }
      }

        tempsettings.Baud =
            tech::ParseInteger<int>(baudbuf).value_or(tempsettings.Baud);

        tempsettings.InitStringIndex = initstr_index;
        tempsettings.CallWaitStringIndex = cwaitstr_index;

        {
          // The list item carries the string the user edited.
          const char* custom = cwaitstrlist.Get_Item(kCallWaitCustom);
          const auto dash = std::string_view(custom).find('-');
          if (dash != std::string_view::npos) {
            pos = static_cast<int>(dash) + 2;
            port::SafeCopy(cwaitstrbuf,
                           std::string_view(custom).substr(base::ToSize(pos)));
          } else {
            cwaitstrbuf[0] = 0;
          }
        }

        port::SafeCopy(tempsettings.CallWaitString, cwaitstrbuf);

        {
          const DetectPortType dpstatus =
              NullModemClass::Detect_Port(&tempsettings);
          if (dpstatus == PORT_VALID) {
            process = false;
            rc = 1;
          } else if (dpstatus == PORT_INVALID) {
            WWMessageBox().Process(TXT_UNABLE_TO_OPEN_PORT);
            firsttime = true;
            display = REDRAW_ALL;
          } else if (dpstatus == PORT_IRQ_INUSE) {
            WWMessageBox().Process(TXT_IRQ_ALREADY_IN_USE);
            firsttime = true;
            display = REDRAW_ALL;
          }
        }
        break;

      /*------------------------------------------------------------------
      CANCEL: send a SIGN_OFF, bail out with error code
      ------------------------------------------------------------------*/
      case KN_ESC:
      case ButtonKey(kButtonCancel):
        process = false;
        rc = 0;
        break;
      default:
        break;
    }

  } /* end of while */

  /*------------------------------------------------------------------------
  Restore screen
  ------------------------------------------------------------------------*/
  Hide_Mouse();
  Load_Title_Page(true);
  Show_Mouse();

  /*------------------------------------------------------------------------
  Save values into the Settings structure
  ------------------------------------------------------------------------*/
  if (rc) {
    *settings = tempsettings;
  }

  return rc;

} /* end of Com_Settings_Dialog */

/***************************************************************************
 * Build_Init_String_Listbox -- [re]builds the initstring listbox          *
 *                                                                         *
 * This routine rebuilds the initstring list box from scratch; it also * updates
 *the contents of the initstring edit field.
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		list		ptr to list box
 ** edit		ptr to edit box
 ** buf		ptr to buffer for initstring
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   06/08/1995 DRD : Created.                                             *
 *=========================================================================*/
static void Build_Init_String_Listbox(ListClass* list, EditClass* edit,
                                      std::span<char> buf, int* index) {
  int curidx = *index;

  /*........................................................................
  Clear the list
  ........................................................................*/
  list->Clear();

  /*
  ** Now sort the init string list by name then number
  */
  if (Session.InitStrings.Count() > 0) {
    std::vector<char*> sorted(base::ToSize(Session.InitStrings.Count()));
    for (int i = 0; i < Session.InitStrings.Count(); ++i) {
      sorted.at(base::ToSize(i)) = Session.InitStrings.at(i);
    }
    std::ranges::sort(sorted, [](const char* left, const char* right) {
      return std::string_view(left).compare(right) < 0;
    });
    for (int i = 0; i < Session.InitStrings.Count(); ++i) {
      Session.InitStrings.at(i) = sorted.at(base::ToSize(i));
    }
  }

  /*........................................................................
  Build the list
  ........................................................................*/
  for (int i = 0; i < Session.InitStrings.Count(); i++) {
    list->Add_Item(Session.InitStrings.at(i));
  }
  list->Flag_To_Redraw();

  /*........................................................................
  Init the current phone book index
  ........................................................................*/
  if (list->Count() == 0 || curidx < -1) {
    curidx = -1;
  } else if (curidx >= list->Count()) {
    curidx = 0;
  }

  /*........................................................................
  Fill in initstring edit buffer
  ........................................................................*/
  if (curidx > -1) {
    port::SafeCopy(std::span(buf).first(INITSTRBUF_MAX),
                   Session.InitStrings.at(curidx));
    edit->Set_Text(buf, INITSTRBUF_MAX);
    list->Set_Selected_Index(curidx);
  }

  *index = curidx;
}

/***********************************************************************************************
 * Com_Scenario_Dialog -- Serial game scenario selection dialog
 **
 *                                                                         						  *
 *                                                                         						  *
 * INPUT: * none.
 **
 *                                                                         						  *
 * OUTPUT: * true = success, false = cancel
 **
 *                                                                         						  *
 * WARNINGS: * none.
 **
 *                                                                         						  *
 * HISTORY: * 02/14/1995 BR : Created. 01/21/97 V.Grippi added check for CS
 *before sending scenario file *
 *=============================================================================================*/
// A single legacy dialog loop; splitting it is a refactor of its own.
// NOLINTNEXTLINE(readability-function-size,google-readability-function-size)
int Com_Scenario_Dialog(bool skirmish) {
  /*........................................................................
  Dialog & button dimensions
  ........................................................................*/
  const int d_dialog_w = 640;                             // dialog width
  const int d_dialog_h = 400;                             // dialog height
  const int d_dialog_x = (640 - d_dialog_w) / 2;          // dialog x-coord
  const int d_dialog_y = (400 - d_dialog_h) / 2;          // dialog y-coord
  const int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);  // center x-coord

  const int d_txt6_h = 12 + 1;  // ht of 6-pt text
  const int d_margin1 = 10;     // margin width/height
  const int d_margin2 = 14;     // margin width/height

  const int d_name_w = 140;
  const int d_name_h = 18;
  const int d_name_x = d_dialog_x + (d_dialog_w / 4) - (d_name_w / 2);
  const int d_name_y = d_dialog_y + d_margin2 + d_txt6_h + 2;

#ifdef OLDWAY
  int d_gdi_w = 80;
  int d_gdi_h = 18;
  int d_gdi_x = d_dialog_cx - d_gdi_w;
  int d_gdi_y = d_name_y;

  int d_nod_w = 80;
  int d_nod_h = 18;
  int d_nod_x = d_dialog_cx;
  int d_nod_y = d_name_y;
#else
  const int d_house_w = 120;
  const int d_house_h = 8 * 10;
  const int d_house_x = d_dialog_cx - (d_house_w / 2);
  const int d_house_y = d_name_y;
#endif

  const int d_color_w = 20;
  const int d_color_h = 18;
  const int d_color_y = d_name_y;
  const int d_color_x = d_dialog_x + (d_dialog_w / 4 * 3) - (d_color_w * 3);

  const int d_playerlist_w = 236;
  const int d_playerlist_h = (6 * 12) + 6;  // 6 rows high
  const int d_playerlist_x = d_dialog_x + d_margin1 + d_margin1 + 10;
  const int d_playerlist_y =
      d_color_y + d_color_h + d_margin2 + 4 /*KO + d_txt6_h*/;

  const int d_scenariolist_w = 324;
  int d_scenariolist_h = (6 * 12) + 6;  // 6 rows high

  if (skirmish) {
    d_scenariolist_h *= 2;
  }

  int d_scenariolist_x = d_dialog_x + d_dialog_w - d_margin1 - d_margin1 -
                         d_scenariolist_w - 10;
  const int d_scenariolist_y = d_color_y + d_color_h + d_margin2 + 4;

  if (skirmish) {
    d_scenariolist_x = d_dialog_x + ((d_dialog_w - d_scenariolist_w) / 2);
  }

  const int d_count_w = 50;
  const int d_count_h = d_txt6_h;
  const int d_count_x = d_playerlist_x + (d_playerlist_w / 2) + 40;  // (fudged)
  int d_count_y = d_playerlist_y + d_playerlist_h + (d_margin1 * 2) - 4;

  if (skirmish) {
    d_count_y = d_scenariolist_y + d_scenariolist_h + d_margin1 - 4;
  }

  const int d_level_w = 50;
  const int d_level_h = d_txt6_h;
  const int d_level_x = d_playerlist_x + (d_playerlist_w / 2) + 40;  // (fudged)
  const int d_level_y = d_count_y + d_count_h;

  const int d_credits_w = 50;
  const int d_credits_h = d_txt6_h;
  const int d_credits_x =
      d_playerlist_x + (d_playerlist_w / 2) + 40;  // (fudged)
  const int d_credits_y = d_level_y + d_level_h;

  const int d_aiplayers_w = 50;
  const int d_aiplayers_h = d_txt6_h;
  const int d_aiplayers_x =
      d_playerlist_x + (d_playerlist_w / 2) + 40;  // (fudged)
  const int d_aiplayers_y = d_credits_y + d_credits_h;

  const int d_options_w = 212;
  const int d_options_h = (5 * 12) + 8;
  const int d_options_x = d_dialog_x + d_dialog_w - 298;
  const int d_options_y = d_scenariolist_y + d_scenariolist_h + d_margin1 - 4;

  const int d_message_w = d_dialog_w - (d_margin1 * 2) - 40;
  const int d_message_h = (8 * d_txt6_h) + 6;  // 4 rows high
  const int d_message_x = d_dialog_x + d_margin1 + 20;
  const int d_message_y = d_options_y + d_options_h + 4;

  const int d_send_w = d_dialog_w - (d_margin1 * 2) - 40;
  const int d_send_h = 18;
  const int d_send_x = d_dialog_x + d_margin1 + 20;
  const int d_send_y = d_message_y + d_message_h;

  const int d_ok_w = 90;
  const int d_ok_h = 18;
  const int d_ok_x = d_dialog_x + (d_dialog_w / 6) - (d_ok_w / 2);
  const int d_ok_y = d_dialog_y + d_dialog_h - d_ok_h - d_margin1 - 12;

  const int d_cancel_w = 90;
  const int d_cancel_h = 18;
  const int d_cancel_x = d_dialog_cx - (d_cancel_w / 2);
  const int d_cancel_y = d_dialog_y + d_dialog_h - d_cancel_h - d_margin1 - 12;

  const int d_load_w = 90;
  const int d_load_h = 18;
  const int d_load_x = d_dialog_x + (d_dialog_w * 5 / 6) - (d_load_w / 2);
  const int d_load_y = d_dialog_y + d_dialog_h - d_load_h - d_margin1 - 12;

  /*........................................................................
  Button Enumerations
  ........................................................................*/
  constexpr int kButtonName = 100;
#ifdef OLDWAY
  constexpr int kButtonGdi = 101;
  constexpr int kButtonNod = 102;
  constexpr int kButtonCredits = 103;
#else
  constexpr int kButtonHouse = 101;
  constexpr int kButtonCredits = 102;
#endif
  constexpr int kButtonAiplayers = kButtonCredits + 1;
  constexpr int kButtonOptions = kButtonAiplayers + 1;
  constexpr int kButtonPlayerlist = kButtonOptions + 1;
  constexpr int kButtonScenariolist = kButtonPlayerlist + 1;
  constexpr int kButtonCount = kButtonScenariolist + 1;
  constexpr int kButtonLevel = kButtonCount + 1;
  constexpr int kButtonOk = kButtonLevel + 1;
  constexpr int kButtonLoad = kButtonOk + 1;
  constexpr int kButtonCancel = kButtonLoad + 1;
  constexpr int kButtonDifficulty = kButtonCancel + 1;

  /*........................................................................
  Redraw values: in order from "top" to "bottom" layer of the dialog
  ........................................................................*/
  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_PARMS = 1,
    REDRAW_MESSAGE = 2,
    REDRAW_COLORS = 3,
    REDRAW_BUTTONS = 4,
    REDRAW_BACKGROUND = 5,
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;

  /*........................................................................
  Dialog variables
  ........................................................................*/
  RedrawType display = REDRAW_ALL;  // redraw level
  bool process = true;              // process while true
  KeyNumType input = KN_NONE;

  const int playertabs[] = {77 * 2};     // tabs for player list box
  const int optiontabs[] = {8};          // tabs for player list box
  char namebuf[MPLAYER_NAME_MAX] = {0};  // buffer for player's name
  bool transmit = false;                 // 1 = re-transmit new game options
  const int cbox_x[] = {d_color_x,
                        d_color_x + d_color_w,
                        d_color_x + (d_color_w * 2),
                        d_color_x + (d_color_w * 3),
                        d_color_x + (d_color_w * 4),
                        d_color_x + (d_color_w * 5),
                        d_color_x + (d_color_w * 6),
                        d_color_x + (d_color_w * 7)};
  bool changed = false;  // 1 = user has changed an option

  int rc = 0;
  bool recsignedoff = false;
  int i = 0;
  uint32_t version = 0;
  int64_t starttime = 0;
  int64_t timingtime = 0;
  int64_t lastmsgtime = 0;
  int64_t lastredrawtime = 0;
  int64_t transmittime = 0;
  int32_t theirresponsetime = 0;
  int packetlen = 0;
  static bool first_time = true;
  bool gameoptions = Session.Type == GAME_SKIRMISH;
  // event ptr
  int64_t msg_timeout = 1200;  // init to 20 seconds

  GameFile loadfile("SAVEGAME.NET");
  bool load_game = false;  // 1 = load a saved game
  NodeNameType* who = nullptr;       // node to add to Players
  char item[MPLAYER_NAME_MAX + 64];  // for filling in lists
  RemapControlType* scheme = GadgetClass::Get_Color_Scheme();
  bool messages_have_focus = true;  // Gadget focus starts on the message system

  Set_Logic_Page(SeenBuff);

  Timer<SystemTickSource> kludge_timer;  // Timer to allow a wait after client
                                         // joins game before game can start
  bool ok_button_added = false;

  /*........................................................................
  Buttons
  ........................................................................*/
  GadgetClass* commands = nullptr;  // button list

  EditClass name_edt(kButtonName, namebuf, MPLAYER_NAME_MAX, kTpfText, d_name_x,
                     d_name_y, d_name_w, d_name_h, EditClass::kAlphanumeric);

#ifdef OLDWAY
  TextButtonClass gdibtn(kButtonGdi, TXT_ALLIES, kTpfButton, d_gdi_x, d_gdi_y,
                         d_gdi_w, d_gdi_h);
  TextButtonClass nodbtn(kButtonNod, TXT_SOVIET, kTpfButton, d_nod_x, d_nod_y,
                         d_nod_w, d_nod_h);
#else
  char housetext[25] = "";
  Fancy_Text_Print("", 0, 0, nullptr, 0, kTpfText);
  DropListClass housebtn(kButtonHouse, housetext, sizeof(housetext), kTpfText,
                         d_house_x, d_house_y, d_house_w, d_house_h,
                         MixArchive::RetrieveData("BTN-UP.SHP"),
                         MixArchive::RetrieveData("BTN-DN.SHP"));
#endif
  ColorListClass playerlist(kButtonPlayerlist, d_playerlist_x, d_playerlist_y,
                            d_playerlist_w, d_playerlist_h, kTpfText,
                            MixArchive::RetrieveData("BTN-UP.SHP"),
                            MixArchive::RetrieveData("BTN-DN.SHP"));
  ListClass scenariolist(kButtonScenariolist, d_scenariolist_x,
                         d_scenariolist_y, d_scenariolist_w, d_scenariolist_h,
                         kTpfText, MixArchive::RetrieveData("BTN-UP.SHP"),
                         MixArchive::RetrieveData("BTN-DN.SHP"));
  GaugeClass countgauge(kButtonCount, d_count_x, d_count_y, d_count_w,
                        d_count_h);

  char staticcountbuff[35];
  StaticButtonClass staticcount(0, "     ", kTpfText, d_count_x + d_count_w + 6,
                                d_count_y);

  GaugeClass levelgauge(kButtonLevel, d_level_x, d_level_y, d_level_w,
                        d_level_h);

  char staticlevelbuff[35];
  StaticButtonClass staticlevel(0, "     ", kTpfText, d_level_x + d_level_w + 6,
                                d_level_y);

  GaugeClass creditsgauge(kButtonCredits, d_credits_x, d_credits_y, d_credits_w,
                          d_credits_h);

  char staticcreditsbuff[35];
  StaticButtonClass staticcredits(0, "         ", kTpfText,
                                  d_credits_x + d_credits_w + 6, d_credits_y);

  GaugeClass aiplayersgauge(kButtonAiplayers, d_aiplayers_x, d_aiplayers_y,
                            d_aiplayers_w, d_aiplayers_h);

  char staticaibuff[35];
  StaticButtonClass staticai(0, "     ", kTpfText,
                             d_aiplayers_x + d_aiplayers_w + 6, d_aiplayers_y);

  CheckListClass optionlist(kButtonOptions, d_options_x, d_options_y,
                            d_options_w, d_options_h, kTpfText,
                            MixArchive::RetrieveData("BTN-UP.SHP"),
                            MixArchive::RetrieveData("BTN-DN.SHP"));
  TextButtonClass okbtn(kButtonOk, TXT_OK, kTpfButton, d_ok_x, d_ok_y, d_ok_w,
                        d_ok_h);
  TextButtonClass loadbtn(kButtonLoad, TXT_LOAD_BUTTON, kTpfButton, d_load_x,
                          d_load_y, d_load_w, d_load_h);
  TextButtonClass cancelbtn(kButtonCancel, TXT_CANCEL, kTpfButton, d_cancel_x,
                            d_cancel_y, d_cancel_w, d_cancel_h);

  SliderClass difficulty(
      kButtonDifficulty, d_name_x,
      optionlist.Y + optionlist.Height + d_margin1 + d_margin1,
      d_dialog_w - ((d_name_x - d_dialog_x) * 2), 16, true);
  if (Rule.IsFineDifficulty) {
    difficulty.Set_Maximum(5);
    difficulty.Set_Value(2);
  } else {
    difficulty.Set_Maximum(3);
    difficulty.Set_Value(1);
  }

  /*
  ------------------------- Build the button list --------------------------
  */
  commands = &name_edt;
  staticcount.Add_Tail(*commands);
  staticcredits.Add_Tail(*commands);
  staticai.Add_Tail(*commands);
  staticlevel.Add_Tail(*commands);
  if (!skirmish) {
    playerlist.Add_Tail(*commands);
  } else {
    difficulty.Add_Tail(*commands);
  }
  scenariolist.Add_Tail(*commands);
  countgauge.Add_Tail(*commands);
  levelgauge.Add_Tail(*commands);
  creditsgauge.Add_Tail(*commands);
  aiplayersgauge.Add_Tail(*commands);
  optionlist.Add_Tail(*commands);
  if (Session.Type == GAME_SKIRMISH) {
    okbtn.Add_Tail(*commands);
  }
  cancelbtn.Add_Tail(*commands);
  if (!skirmish && loadfile.IsAvailable()) {
  } else {
    cancelbtn.X = loadbtn.X;
  }
#ifdef OLDWAY
  gdibtn.Add_Tail(*commands);
  nodbtn.Add_Tail(*commands);
#else
  housebtn.Add_Tail(*commands);
#endif

  /*
  ----------------------------- Various Inits ------------------------------
  */
  /*........................................................................
  Init player name & house
  ........................................................................*/
  Session.ColorIdx = Session.PrefColor;     // init my preferred color
  port::SafeCopy(namebuf, Session.Handle);  // set my name
  name_edt.Set_Text(namebuf, MPLAYER_NAME_MAX);
  name_edt.Set_Color(&ColorRemaps.at(Session.ColorIdx == PCOLOR_DIALOG_BLUE
                                         ? PCOLOR_REALLY_BLUE
                                         : Session.ColorIdx));

#ifdef OLDWAY
  if (Session.House == HOUSE_GOOD) {
    gdibtn.Turn_On();
  } else {
    nodbtn.Turn_On();
  }
#else
  for (HousesType house = HOUSE_USSR; house <= HOUSE_FRANCE; house++) {
    housebtn.Add_Item(
        Text_String(HouseTypeClass::As_Reference(house).Full_Name()));
  }
  housebtn.Set_Selected_Index(static_cast<int>(Session.House) -
                              static_cast<int>(HOUSE_USSR));
  housebtn.Set_Read_Only(true);
#endif

  /*........................................................................
  Init scenario values, only the first time through
  ........................................................................*/
  Special.IsCaptureTheFlag = Rule.IsMPCaptureTheFlag;
  if (first_time) {
    Session.Options.Credits =
        Rule.MPDefaultMoney;                   // init credits & credit buffer
    Session.Options.Bases = Rule.IsMPBasesOn;  // init scenario parameters
    Session.Options.Tiberium = Rule.IsMPTiberiumGrow;
    Session.Options.Goodies = Rule.IsMPCrates;
    Session.Options.AIPlayers = 0;
    Special.IsShadowGrow = Rule.IsMPShadowGrow;
    Session.Options.UnitCount =
        (base::At(SessionClass::CountMax, Session.Options.Bases) +
         base::At(SessionClass::CountMin, Session.Options.Bases)) /
        2;
    first_time = false;
  }

  /*........................................................................
  Init button states
  ........................................................................*/
  playerlist.Set_Tabs(playertabs);
  playerlist.Set_Selected_Style(ColorListClass::SELECT_NORMAL);

  optionlist.Set_Tabs(optiontabs);
  optionlist.Set_Read_Only(false);

  optionlist.Add_Item(Text_String(TXT_BASES));
  optionlist.Add_Item(Text_String(TXT_ORE_SPREADS));
  optionlist.Add_Item(Text_String(TXT_CRATES));
  optionlist.Add_Item(Text_String(TXT_SHADOW_REGROWS));
  if (!skirmish) {
    optionlist.Add_Item(Text_String(TXT_CAPTURE_THE_FLAG));
  }

  optionlist.Check_Item(0, Session.Options.Bases != 0);
  optionlist.Check_Item(1, Session.Options.Tiberium != 0);
  optionlist.Check_Item(2, Session.Options.Goodies != 0);
  optionlist.Check_Item(3, Special.IsShadowGrow);
  if (!skirmish) {
    optionlist.Check_Item(4, Special.IsCaptureTheFlag);
  }

  countgauge.Set_Maximum(
      base::At(SessionClass::CountMax, Session.Options.Bases) -
      base::At(SessionClass::CountMin, Session.Options.Bases));
  countgauge.Set_Value(Session.Options.UnitCount -
                       base::At(SessionClass::CountMin, Session.Options.Bases));

  levelgauge.Set_Maximum(MPLAYER_BUILD_LEVEL_MAX - 1);
  levelgauge.Set_Value(BuildLevel - 1);

  creditsgauge.Set_Maximum(Rule.MPMaxMoney);
  creditsgauge.Set_Value(Session.Options.Credits);

  const int maxp = Rule.MaxPlayers - 2;
  //	int maxp = Rule.MaxPlayers - (skirmish ? 1 : 2);
  aiplayersgauge.Set_Maximum(maxp);

  if (skirmish) {
    Session.Options.AIPlayers = std::clamp(Session.Options.AIPlayers, 1, 7);
  } else {
    Session.Options.AIPlayers = std::min(Session.Options.AIPlayers, 6);
  }

  aiplayersgauge.Set_Value(Session.Options.AIPlayers - (skirmish ? 1 : 0));

  /*........................................................................
  Init other scenario parameters
  ........................................................................*/
  Rule.IsTGrowth = Rule.IsTSpread = Session.Options.Tiberium != 0;
  Special.IsTGrowth = Special.IsTSpread = Rule.IsTGrowth ? 1 : 0;
  transmit = true;

  /*........................................................................
  Clear the Players vector
  ........................................................................*/
  Clear_Vector(&Session.Players);

  /*........................................................................
  Init scenario description list box
  ........................................................................*/
  for (i = 0; i < Session.Scenarios.Count(); i++) {
    int j = 0;
    for (j = 0; base::At(EngMisStr, base::ToSize(j)) != nullptr; j++) {
      if (std::string_view(Session.Scenarios.at(i)->Description()) ==
          base::At(EngMisStr, base::ToSize(j))) {
        // ajw Added Aftermath installed checks (before, it was
        // assumed). Add mission if it's available to us.
        if ((!IsMissionCounterstrike(Session.Scenarios.at(i)->Get_Filename()) ||
             Is_Counterstrike_Installed()) &&
            (!IsMissionAftermath(Session.Scenarios.at(i)->Get_Filename()) ||
             Is_Aftermath_Installed())) {
          scenariolist.Add_Item(base::At(
              EngMisStr, base::ToSize(config::kIsEnglish ? j : j + 1)));
        }
        break;
      }
    }
    if ((base::At(EngMisStr, base::ToSize(j)) == nullptr) &&
        (!Session.Scenarios.at(i)->Get_Official() ||
         ((!IsMissionCounterstrike(Session.Scenarios.at(i)->Get_Filename()) ||
           Is_Counterstrike_Installed()) &&
          (!IsMissionAftermath(Session.Scenarios.at(i)->Get_Filename()) ||
           Is_Aftermath_Installed()))))
    // ajw Added Aftermath installed checks (before, it was
    // assumed). Added officialness check. Add mission if
    // it's available to us.
    {
      scenariolist.Add_Item(Session.Scenarios.at(i)->Description());
    }
  }

  Session.Options.ScenarioIndex = 0;  // 1st scenario is selected

  /*........................................................................
  Init random-number generator, & create a seed to be used for all random
  numbers from here on out
  ........................................................................*/
  Seed = port::RandomSeed();

  /*........................................................................
  Init the message display system
  ........................................................................*/
  if (!skirmish) {
    Session.Messages.Init(
        d_message_x + 1, d_message_y + 1, 8, MAX_MESSAGE_LENGTH, d_txt6_h,
        d_send_x + 1, d_send_y + 1, 1, 20, MAX_MESSAGE_LENGTH - 5, d_message_w);
    Session.Messages.Add_Edit(Session.ColorIdx == PCOLOR_DIALOG_BLUE
                                  ? PCOLOR_REALLY_BLUE
                                  : Session.ColorIdx,
                              kTpfText, nullptr, '_', d_message_w);
  }

  /*........................................................................
  Init version number clipping system
  ........................................................................*/
  VerNum.Init_Clipping();
  Load_Title_Page(true);
  CCPalette.Set();

  if (std::string_view(ModemRXString).size() > 36) {
    ModemRXString[36] = 0;
  }

  if (!std::string_view(ModemRXString).empty()) {
    Session.Messages.Add_Message(nullptr, 0, ModemRXString, PCOLOR_BROWN,
                                 kTpfText, -1);
  }

  ModemRXString[0] = '\0';

  /*
  ---------------------------- Processing loop -----------------------------
  */
  if (!skirmish) {
    NullModem.Reset_Response_Time();  // clear response time
  }
  theirresponsetime = 10000;  // initialize to an invalid value
  timingtime = lastmsgtime = lastredrawtime = TickCount.Value();

  bool retry_setup = true;
  while (retry_setup) {
    retry_setup = false;

    while (process) {

      if ((!skirmish) &&
          (!ok_button_added && gameoptions && kludge_timer.IsFinished())) {
        okbtn.Add_Tail(*commands);
        ok_button_added = true;
        if (loadfile.IsAvailable()) {
          loadbtn.Add_Tail(*commands);
        }
        display = std::max(display, REDRAW_BUTTONS);
      }

      /*
      ** Kludge to make sure we redraw the message input line when it loses
      * focus.
      ** If we dont do this then the cursor doesnt disappear.
      */
      if (!skirmish) {
        if (messages_have_focus) {
          if (name_edt.Has_Focus()) {
            display = std::max(display, REDRAW_MESSAGE);
          }
        } else {
          if (!name_edt.Has_Focus()) {
            display = std::max(display, REDRAW_MESSAGE);
            Session.Messages.Set_Edit_Focus();
          }
        }
      }

      /*
      ........................ Invoke game callback .........................
      */
      ServiceRealTime();

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
      ...................... Refresh display if needed ......................
      */
      if (display != REDRAW_NONE) {
        if (housebtn.IsDropped) {
          housebtn.Collapse();
          display = REDRAW_BACKGROUND;
        }
        Hide_Mouse();

        /*
        .................. Redraw backgound & dialog box ...................
        */
        if (display >= REDRAW_BACKGROUND) {
          Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);

          // init font variables

          Fancy_Text_Print(TXT_NONE, 0, 0, nullptr, kTBlack,
                           TPF_CENTER | kTpfText);

          /*...............................................................
          Dialog & Field labels
          ...............................................................*/
          Fancy_Text_Print(TXT_YOUR_NAME, d_name_x + (d_name_w / 2),
                           d_name_y - d_txt6_h, scheme, kTBlack,
                           TPF_CENTER | kTpfText);
#ifdef OLDWAY
          Fancy_Text_Print(TXT_SIDE_COLON, d_gdi_x + d_gdi_w,
                           d_gdi_y - d_txt6_h, scheme, kTBlack,
                           TPF_CENTER | kTpfText);
#else
          Fancy_Text_Print(TXT_SIDE_COLON, d_house_x + (d_house_w / 2),
                           d_house_y - d_txt6_h, scheme, kTBlack,
                           TPF_CENTER | kTpfText);
#endif
          Fancy_Text_Print(TXT_COLOR_COLON, d_dialog_x + (d_dialog_w / 4 * 3),
                           d_color_y - d_txt6_h, scheme, kTBlack,
                           TPF_CENTER | kTpfText);
          if (!skirmish) {
            Fancy_Text_Print(TXT_PLAYERS, d_playerlist_x + (d_playerlist_w / 2),
                             d_playerlist_y - d_txt6_h, scheme, kTBlack,
                             TPF_CENTER | kTpfText);
          } else {
            Fancy_Text_Print(TXT_EASY, difficulty.X, difficulty.Y - 16, scheme,
                             kTBlack, kTpfText);
            Fancy_Text_Print(TXT_HARD, difficulty.X + difficulty.Width,
                             difficulty.Y - 16, scheme, kTBlack,
                             TPF_RIGHT | kTpfText);
            Fancy_Text_Print(TXT_NORMAL, difficulty.X + (difficulty.Width / 2),
                             difficulty.Y - 16, scheme, kTBlack,
                             TPF_CENTER | kTpfText);
          }
          Fancy_Text_Print(TXT_SCENARIOS,
                           d_scenariolist_x + (d_scenariolist_w / 2),
                           d_scenariolist_y - d_txt6_h, scheme, kTBlack,
                           TPF_CENTER | kTpfText);
          Fancy_Text_Print(TXT_COUNT, d_count_x - 2, d_count_y, scheme, kTBlack,
                           kTpfText | TPF_RIGHT);
          Fancy_Text_Print(TXT_LEVEL, d_level_x - 2, d_level_y, scheme, kTBlack,
                           kTpfText | TPF_RIGHT);
          Fancy_Text_Print(TXT_CREDITS_COLON, d_credits_x - 2, d_credits_y,
                           scheme, kTBlack, kTpfText | TPF_RIGHT);
          Fancy_Text_Print(TXT_AI_PLAYERS_COLON, d_aiplayers_x - 4,
                           d_aiplayers_y, scheme, kTBlack,
                           kTpfText | TPF_RIGHT);
        }

        /*..................................................................
        Draw the color boxes
        ..................................................................*/
        if (display >= REDRAW_COLORS) {
          for (i = 0; i < MAX_MPLAYER_COLORS; i++) {
            LogicPage->Fill_Rect(
                base::At(cbox_x, i) + 1, d_color_y + 1,
                base::At(cbox_x, i) + 1 + d_color_w - 2,
                d_color_y + 1 + d_color_h - 2,
                ColorRemaps.at(static_cast<PlayerColorType>(i)).Box);
            //						(i ==
            // PCOLOR_DIALOG_BLUE) ? ColorRemaps[PCOLOR_REALLY_BLUE].Box :
            // ColorRemaps[static_cast<PlayerColorType>(i)].Box);

            if (static_cast<PlayerColorType>(i) == Session.ColorIdx) {
              Draw_Box(base::At(cbox_x, i), d_color_y, d_color_w, d_color_h,
                       BOXSTYLE_DOWN, false);
            } else {
              Draw_Box(base::At(cbox_x, i), d_color_y, d_color_w, d_color_h,
                       BOXSTYLE_RAISED, false);
            }
          }
        }

        /*..................................................................
        Draw the message system; erase old messages first
        ..................................................................*/
        if (display >= REDRAW_MESSAGE && !skirmish) {
          Draw_Box(d_message_x, d_message_y, d_message_w, d_message_h,
                   BOXSTYLE_BOX, true);
          Draw_Box(d_send_x, d_send_y, d_send_w, d_send_h, BOXSTYLE_BOX, true);
          Session.Messages.Draw();
        }

        //..................................................................
        // Update game parameter labels
        //..................................................................
        if (display >= REDRAW_PARMS) {
          //				LogicPage->Fill_Rect(d_count_x +
          // d_count_w + 2, d_count_y, d_count_x + d_count_w + 35 * 2,
          // d_aiplayers_y + d_aiplayers_h+2, BLACK);

          absl::SNPrintF(staticcountbuff, sizeof(staticcountbuff), "%d",
                         Session.Options.UnitCount);
          staticcount.Set_Text(staticcountbuff);
          staticcount.Draw_Me();
          //				Fancy_Text_Print("%d ", d_count_x +
          // d_count_w + 3 * 2, d_count_y, scheme, BLACK, kTpfText,
          // Session.Options.UnitCount);

          if (BuildLevel <= MPLAYER_BUILD_LEVEL_MAX) {
            absl::SNPrintF(staticlevelbuff, sizeof(staticlevelbuff), "%d ",
                           BuildLevel);
          } else {
            absl::SNPrintF(staticlevelbuff, sizeof(staticlevelbuff), "**");
          }
          staticlevel.Set_Text(staticlevelbuff);
          staticlevel.Draw_Me();
          //				Fancy_Text_Print(txt, d_level_x +
          // d_level_w + 3 * 2, d_level_y, scheme, BLACK, kTpfText);

          absl::SNPrintF(staticcreditsbuff, sizeof(staticcreditsbuff), "%d",
                         Session.Options.Credits);
          staticcredits.Set_Text(staticcreditsbuff);
          staticcredits.Draw_Me();
          //				Fancy_Text_Print("%d", d_credits_x +
          // d_credits_w + 2 * 2, d_credits_y, scheme, BLACK, kTpfText,
          // Session.Options.Credits);

          absl::SNPrintF(staticaibuff, sizeof(staticaibuff), "%d",
                         Session.Options.AIPlayers);
          staticai.Set_Text(staticaibuff);
          staticai.Draw_Me();
          //				Fancy_Text_Print("%d", d_aiplayers_x +
          // d_aiplayers_w + 2*2, d_aiplayers_y, scheme, BLACK,
          // kTpfText, Session.Options.AIPlayers);
        }

        /*
        .......................... Redraw buttons ..........................
        */
        if (display >= REDRAW_BUTTONS) {
          commands->Flag_List_To_Redraw();
          commands->Draw_All();
        }

        Show_Mouse();
        display = REDRAW_NONE;
      }

      /*
      ........................... Get user input ............................
      */
      messages_have_focus = Session.Messages.Has_Edit_Focus();
      const bool droplist_is_dropped = housebtn.IsDropped;
      input = commands->Input();

      /*
      ** Sort out the input focus between the name edit box and the message
      * system
      */
      if ((!skirmish) && messages_have_focus) {
        if (!name_edt.Has_Focus()) {
          Session.Messages.Set_Edit_Focus();
        } else {
          messages_have_focus = false;
          display = REDRAW_MESSAGE;
        }
      }

      /*
      ** Redraw everything if the droplist collapsed
      */
      if (droplist_is_dropped && !housebtn.IsDropped) {
        display = REDRAW_BACKGROUND;
      }

      if ((input & KN_BUTTON) && housebtn.IsDropped) {
        housebtn.Collapse();
        display = REDRAW_BACKGROUND;
      }

      /*
      ---------------------------- Process input ----------------------------
      */
      switch (static_cast<int>(input)) {
        /*------------------------------------------------------------------
        User clicks on a color button
        ------------------------------------------------------------------*/
        case KN_LMOUSE:
          if (Keyboard->MouseQX > cbox_x[0] &&
              Keyboard->MouseQX < cbox_x[MAX_MPLAYER_COLORS - 1] + d_color_w &&
              Keyboard->MouseQY > d_color_y &&
              Keyboard->MouseQY < d_color_y + d_color_h) {
            Session.PrefColor = static_cast<PlayerColorType>(
                (Keyboard->MouseQX - cbox_x[0]) / d_color_w);
            Session.ColorIdx = Session.PrefColor;
            display = std::max(display, REDRAW_COLORS);

            name_edt.Set_Color(&ColorRemaps.at(
                Session.ColorIdx == PCOLOR_DIALOG_BLUE ? PCOLOR_REALLY_BLUE
                                                       : Session.ColorIdx));
            name_edt.Flag_To_Redraw();
            Session.Messages.Set_Edit_Color(
                Session.ColorIdx == PCOLOR_DIALOG_BLUE ? PCOLOR_REALLY_BLUE
                                                       : Session.ColorIdx);
            port::SafeCopy(Session.Handle, namebuf);
            transmit = true;
            changed = true;
            if (housebtn.IsDropped) {
              housebtn.Collapse();
              display = REDRAW_BACKGROUND;
            }
          }
          break;

        /*------------------------------------------------------------------
        User edits the name field; retransmit new game options
        ------------------------------------------------------------------*/
        case ButtonKey(kButtonName):
          if (housebtn.IsDropped) {
            housebtn.Collapse();
            display = REDRAW_BACKGROUND;
          }
          port::SafeCopy(Session.Handle, namebuf);
          transmit = true;
          changed = true;
          break;

#ifdef OLDWAY
        /*------------------------------------------------------------------
        House Buttons: set the player's desired House
        ------------------------------------------------------------------*/
        case ButtonKey(kButtonGdi):
          Session.House = HOUSE_GOOD;
          gdibtn.Turn_On();
          nodbtn.Turn_Off();
          port::SafeCopy(Session.Handle, namebuf);
          transmit = true;
          break;

        case ButtonKey(kButtonNod):
          Session.House = HOUSE_BAD;
          gdibtn.Turn_Off();
          nodbtn.Turn_On();
          port::SafeCopy(Session.Handle, namebuf);
          transmit = true;
          break;
#else
        case ButtonKey(kButtonHouse):
          Session.House = static_cast<HousesType>(housebtn.Current_Index() +
                                                  static_cast<int>(HOUSE_USSR));
          port::SafeCopy(Session.Handle, namebuf);
          display = REDRAW_BACKGROUND;
          transmit = true;
          break;
#endif

          /*------------------------------------------------------------------
          New Scenario selected.
          ------------------------------------------------------------------*/
          // All scenarios now allowable as downloads. ajw
        case ButtonKey(kButtonScenariolist):
          if (housebtn.IsDropped) {
            housebtn.Collapse();
            display = REDRAW_BACKGROUND;
          }
          if (scenariolist.Current_Index() != Session.Options.ScenarioIndex) {
            Session.Options.ScenarioIndex = scenariolist.Current_Index();
            port::SafeCopy(Session.Handle, namebuf);
            transmit = true;
          }
          break;

        /*------------------------------------------------------------------
        User adjusts max # units
        ------------------------------------------------------------------*/
        case ButtonKey(kButtonCount):
          Session.Options.UnitCount =
              countgauge.Get_Value() +
              base::At(SessionClass::CountMin, Session.Options.Bases);
          display = std::max(display, REDRAW_PARMS);
          if (housebtn.IsDropped) {
            housebtn.Collapse();
            display = REDRAW_BACKGROUND;
          }
          transmit = true;
          break;

        /*------------------------------------------------------------------
        User adjusts build level
        ------------------------------------------------------------------*/
        case ButtonKey(kButtonLevel):
          BuildLevel =
              std::min(levelgauge.Get_Value() + 1, MPLAYER_BUILD_LEVEL_MAX);
          display = std::max(display, REDRAW_PARMS);
          if (housebtn.IsDropped) {
            housebtn.Collapse();
            display = REDRAW_BACKGROUND;
          }
          transmit = true;
          break;

        /*------------------------------------------------------------------
        User adjusts max # units
        ------------------------------------------------------------------*/
        case ButtonKey(kButtonCredits):
          Session.Options.Credits = creditsgauge.Get_Value();
          Session.Options.Credits = (Session.Options.Credits + 250) / 500 * 500;
          display = std::max(display, REDRAW_PARMS);
          if (housebtn.IsDropped) {
            housebtn.Collapse();
            display = REDRAW_BACKGROUND;
          }
          transmit = true;
          break;

        //..................................................................
        //	User adjusts # of AI players
        //..................................................................
        case ButtonKey(kButtonAiplayers): {
          Session.Options.AIPlayers = aiplayersgauge.Get_Value();
          int humans = 2;  // Two humans.
          if (skirmish) {
            Session.Options.AIPlayers += 1;  // Always one forced AI player.
            humans = 1;                      // One human.
                                             //						if
            //(Session.Options.AIPlayers == 0) {
            // Session.Options.AIPlayers = 1;
            // aiplayersgauge.Set_Value(0);
            //						}
          }
          if (Session.Options.AIPlayers + humans >=
              Rule.MaxPlayers) {  // if it's pegged, max it out
            Session.Options.AIPlayers = Rule.MaxPlayers - humans;
            aiplayersgauge.Set_Value(Session.Options.AIPlayers -
                                     (skirmish ? 1 : 0));
          }
          transmit = true;
          display = std::max(display, REDRAW_PARMS);

          if (housebtn.IsDropped) {
            housebtn.Collapse();
            display = REDRAW_BACKGROUND;
          }

          break;
        }

        //------------------------------------------------------------------
        // Toggle-able options:
        // If 'Bases' gets toggled, we have to change the range of the
        // UnitCount slider.
        // Also, if Tiberium gets toggled, we have to set the flags
        // in SpecialClass.
        //------------------------------------------------------------------
        case ButtonKey(kButtonOptions):
          if (!skirmish &&
              (Special.IsCaptureTheFlag != 0) != optionlist.Is_Checked(4) &&
              !Special.IsCaptureTheFlag) {
            optionlist.Check_Item(0, true);
          }
          if ((Session.Options.Bases != 0) != optionlist.Is_Checked(0)) {
            Session.Options.Bases = optionlist.Is_Checked(0) ? 1 : 0;
            if (Session.Options.Bases) {
              Session.Options.UnitCount = static_cast<int>(Rescale(
                static_cast<uint32_t>(Session.Options.UnitCount - SessionClass::CountMin[0]),
                static_cast<uint32_t>(SessionClass::CountMax[0] - SessionClass::CountMin[0]),
                static_cast<uint32_t>(SessionClass::CountMax[1] - SessionClass::CountMin[1])));
            } else {
              if (!skirmish) {
                optionlist.Check_Item(4, false);
              }
              Session.Options.UnitCount = static_cast<int>(Rescale(
                static_cast<uint32_t>(Session.Options.UnitCount - SessionClass::CountMin[1]),
                static_cast<uint32_t>(SessionClass::CountMax[1] - SessionClass::CountMin[1]),
                static_cast<uint32_t>(SessionClass::CountMax[0] - SessionClass::CountMin[0])));
            }
            countgauge.Set_Maximum(
                base::At(SessionClass::CountMax, Session.Options.Bases) -
                base::At(SessionClass::CountMin, Session.Options.Bases));
            countgauge.Set_Value(
                Session.Options.UnitCount -
                base::At(SessionClass::CountMin, Session.Options.Bases));
          }
          Session.Options.Tiberium = optionlist.Is_Checked(1) ? 1 : 0;
          Special.IsTGrowth = static_cast<unsigned>(Session.Options.Tiberium);
          Rule.IsTGrowth = Session.Options.Tiberium != 0;
          Special.IsTSpread = static_cast<unsigned>(Session.Options.Tiberium);
          Rule.IsTSpread = Session.Options.Tiberium != 0;

          Session.Options.Goodies = optionlist.Is_Checked(2) ? 1 : 0;
          Special.IsShadowGrow = optionlist.Is_Checked(3);
          if (!skirmish) {
            Special.IsCaptureTheFlag = optionlist.Is_Checked(4);
          }

          transmit = true;
          display = std::max(display, REDRAW_PARMS);
          if (housebtn.IsDropped) {
            housebtn.Collapse();
            display = REDRAW_BACKGROUND;
          }
          break;

        /*------------------------------------------------------------------
        OK: exit loop with true status
        ------------------------------------------------------------------*/
        case ButtonKey(kButtonLoad):
        case ButtonKey(kButtonOk):
          if (housebtn.IsDropped) {
            housebtn.Collapse();
            display = REDRAW_BACKGROUND;
          }
          //
          // make sure we got a game options packet from the other player
          //
          if (gameoptions) {
            rc = 1;
            process = false;

            // force transmitting of game options packet one last time

            transmit = true;
            transmittime = 0;
          } else {
            WWMessageBox().Process(TXT_ONLY_ONE, TXT_OOPS, TXT_NONE);
            display = REDRAW_ALL;
          }
          if (input == ButtonKey(kButtonLoad)) {
            load_game = true;
          }
          break;

        /*------------------------------------------------------------------
        CANCEL: send a SIGN_OFF, bail out with error code
        ------------------------------------------------------------------*/
        case KN_ESC:
        case ButtonKey(kButtonCancel):
          if (housebtn.IsDropped) {
            housebtn.Collapse();
            display = REDRAW_BACKGROUND;
          }
          process = false;
          rc = 0;
          break;

        /*------------------------------------------------------------------
        Default: manage the inter-player messages
        ------------------------------------------------------------------*/
        default:
          if (!skirmish) {
            if (Session.Messages.Manage()) {
              display = std::max(display, REDRAW_MESSAGE);
            }

            /*...............................................................
            Service keyboard input for any message being edited.
            ...............................................................*/
            i = Session.Messages.Input(input);

            /*...............................................................
            If 'Input' returned 1, it means refresh the message display; 2
            means redraw it. Rather than setting 'display', which would redraw
            all msgs, we only need to erase & redraw the edit box here (which
            also erases the cursor).
            ...............................................................*/
            if (i == 1 || i == 2) {
              Hide_Mouse();
              Draw_Box(d_send_x, d_send_y, d_send_w, d_send_h, BOXSTYLE_BOX,
                       true);
              Session.Messages.Draw();
              Show_Mouse();
            } else if (i == 3 || i == 4) {
              /*...............................................................
              If 'input' returned 3, it means send the current message.
              ...............................................................*/
              base::FillBytes(base::ObjectBytes(SendPacket), 0,
                              sizeof(SendPacket));
              SendPacket.Command = SERIAL_MESSAGE;
              port::SafeCopy(SendPacket.Name, namebuf);
              SendPacket.ID = static_cast<unsigned char>(Session.ColorIdx);
              if (i == 3) {
                port::SafeCopy(SendPacket.Message.Message,
                               Session.Messages.Get_Edit_Buf());
              } else {
                port::SafeCopy(SendPacket.Message.Message,
                               Session.Messages.Get_Overflow_Buf());
                Session.Messages.Clear_Overflow_Buf();
              }

              /*..................................................................
              Send the message
              ..................................................................*/
              if (!skirmish) {
                NullModem.Send_Message(base::ObjectBytes(SendPacket),
                                       sizeof(SendPacket), 1);
                NullModem.Service();
              }
              /*..................................................................
              Add the message to our own screen
              ..................................................................*/
              Session.Messages.Add_Message(
                  SendPacket.Name, SendPacket.ID, SendPacket.Message.Message,
                  Session.ColorIdx == PCOLOR_DIALOG_BLUE ? PCOLOR_REALLY_BLUE
                                                         : Session.ColorIdx,
                  kTpfText, -1);
              Session.Messages.Add_Edit(Session.ColorIdx == PCOLOR_DIALOG_BLUE
                                            ? PCOLOR_REALLY_BLUE
                                            : Session.ColorIdx,
                                        kTpfText, nullptr, '_', d_message_w);

              display = std::max(display, REDRAW_MESSAGE);
            } /* end of send message */
          }
          break;
      }

      /*---------------------------------------------------------------------
      Detect editing of the name buffer, transmit new values to players
      ---------------------------------------------------------------------*/
      if (std::string_view(namebuf) != Session.Handle) {
        port::SafeCopy(Session.Handle, namebuf);
        transmit = true;
        changed = true;
      }

      /*---------------------------------------------------------------------
      If our Transmit flag is set, we need to send out a game option packet.
      This message requires an ACK.  The first time through the loop, transmit
      should be set, so we send out our default options; we'll then send
      any changes we make to the defaults.
      ---------------------------------------------------------------------*/
      if (skirmish) {
        transmit = false;
      }

      if (transmit && TickCount.Value() - transmittime > PACKET_RETRANS_TIME) {
        base::FillBytes(base::ObjectBytes(SendPacket), 0, sizeof(SendPacket));
        SendPacket.Command = SERIAL_GAME_OPTIONS;
        port::SafeCopy(SendPacket.Name, namebuf);
        SendPacket.ScenarioInfo.CheatCheck = RuleINI.Get_Unique_ID();
        SendPacket.ScenarioInfo.MinVersion = VersionClass::Min_Version();
        SendPacket.ScenarioInfo.MaxVersion = VersionClass::Max_Version();
        SendPacket.ScenarioInfo.House = Session.House;
        SendPacket.ScenarioInfo.Color = Session.ColorIdx;
        SendPacket.ScenarioInfo.Credits = Session.Options.Credits;
        SendPacket.ScenarioInfo.IsBases =
            static_cast<unsigned int>(Session.Options.Bases);
        SendPacket.ScenarioInfo.IsTiberium =
            static_cast<unsigned int>(Session.Options.Tiberium);
        SendPacket.ScenarioInfo.IsGoodies =
            static_cast<unsigned int>(Session.Options.Goodies);
        SendPacket.ScenarioInfo.AIPlayers =
            static_cast<unsigned char>(Session.Options.AIPlayers);
        SendPacket.ScenarioInfo.BuildLevel =
            static_cast<unsigned char>(BuildLevel);
        SendPacket.ScenarioInfo.UnitCount =
            static_cast<unsigned char>(Session.Options.UnitCount);
        SendPacket.ScenarioInfo.Seed = Seed;
        SendPacket.ScenarioInfo.Special = Special;
        SendPacket.ScenarioInfo.GameSpeed = Options.GameSpeed;
        SendPacket.ID = static_cast<unsigned char>(Session.ModemType);

        /*
        ** Set up the scenario info so the remote player can match the scenario
        * on his machine
        ** or request a download if it doesnt exist
        */
        port::SafeCopy(
            SendPacket.ScenarioInfo.Scenario,
            Session.Scenarios.at(Session.Options.ScenarioIndex)->Description());
        GameFile file(Session.Scenarios.at(Session.Options.ScenarioIndex)
                          ->Get_Filename());

        SendPacket.ScenarioInfo.FileLength =
            static_cast<unsigned int>(file.Size());

        port::SafeCopy(SendPacket.ScenarioInfo.ShortFileName,
                       Session.Scenarios.at(Session.Options.ScenarioIndex)
                           ->Get_Filename());
        port::SafeCopy(
            SendPacket.ScenarioInfo.FileDigest,
            Session.Scenarios.at(Session.Options.ScenarioIndex)->Get_Digest());
        SendPacket.ScenarioInfo.OfficialScenario =
            Session.Scenarios.at(Session.Options.ScenarioIndex)->Get_Official();
        NullModem.Send_Message(base::ObjectBytes(SendPacket),
                               sizeof(SendPacket), 1);

        transmittime = TickCount.Value();
        transmit = false;

        //..................................................................
        // Keep the player list up to date
        //..................................................................
        if (playerlist.Count()) {
#ifdef OLDWAY
          if (Session.House == HOUSE_GOOD) {
            sprintf(item, "%s\t%s", namebuf, Text_String(TXT_ALLIES));
          } else {
            sprintf(item, "%s\t%s", namebuf, Text_String(TXT_SOVIET));
          }
#else   // OLDWAY
          absl::SNPrintF(
              item, sizeof(item), "%s\t%s", namebuf,
              Text_String(
                  HouseTypeClass::As_Reference(Session.House).Full_Name()));
#endif  // OLDWAY
          playerlist.Set_Item(0, item);
          playerlist.Colors.at(0) = &ColorRemaps.at(
              Session.ColorIdx == PCOLOR_DIALOG_BLUE ? PCOLOR_REALLY_BLUE
                                                     : Session.ColorIdx);
          playerlist.Flag_To_Redraw();
        }

        //..................................................................
        // Play a little sound effect
        //..................................................................
        Sound_Effect(VOC_OPTIONS_CHANGED);
      }

      //
      // send a timing packet if enough time has gone by.
      //
      if (!skirmish && TickCount.Value() - timingtime > PACKET_TIMING_TIMEOUT) {
        base::FillBytes(base::ObjectBytes(SendPacket), 0, sizeof(SendPacket));
        SendPacket.Command = SERIAL_TIMING;
        SendPacket.ScenarioInfo.ResponseTime = NullModem.Response_Time();
        SendPacket.ID = static_cast<unsigned char>(Session.ModemType);

        NullModem.Send_Message(base::ObjectBytes(SendPacket),
                               sizeof(SendPacket), 0);
        timingtime = TickCount.Value();
      }

      /*---------------------------------------------------------------------
      Check for an incoming message
      ---------------------------------------------------------------------*/
      if (!skirmish && NullModem.Get_Message(base::ObjectBytes(ReceivePacket),
                                             &packetlen) > 0) {
        lastmsgtime = TickCount.Value();
        msg_timeout = 600;  // reset timeout value to 10 seconds
                            // (only the 1st time through is 20 seconds)

        // are we getting our own packets back??

        if (ReceivePacket.Command >= SERIAL_CONNECT &&
            ReceivePacket.Command < SERIAL_LAST_COMMAND &&
            ReceivePacket.Command != SERIAL_MESSAGE &&
            ReceivePacket.ID == static_cast<unsigned char>(Session.ModemType)) {
          WWMessageBox().Process(TXT_SYSTEM_NOT_RESPONDING);

          // to skip the other system not responding msg
          lastmsgtime = TickCount.Value();

          process = false;
          rc = 0;

          // say we did receive sign off to keep from sending one
          recsignedoff = true;
          break;
        }

        const auto event_type = port::ReadUnaligned<EventClass::EventType>(
            base::ObjectBytes(ReceivePacket));
        if (event_type <= EventClass::FRAMEINFO) {
          if (TickCount.Value() - lastredrawtime > PACKET_REDRAW_TIME) {
            lastredrawtime = TickCount.Value();
            display = std::max(display, REDRAW_MESSAGE);
          }
        } else {
          switch (ReceivePacket.Command) {
            /*..................................................................
            Sign-off: Give the other machine time to receive my ACK, display a
            message, and exit.
            ..................................................................*/
            case SERIAL_SIGN_OFF:
              starttime = TickCount.Value();
              while (TickCount.Value() - starttime < 60) {
                NullModem.Service();
              }
              WWMessageBox().Process(TXT_USER_SIGNED_OFF);

              // to skip the other system not responding msg
              lastmsgtime = TickCount.Value();

              process = false;
              rc = 0;
              recsignedoff = true;
              break;

            /*..................................................................
            Game Options:  Store the other machine's name, color & house;
            If they've picked the same color as myself, re-transmit my settings
            to force him to choose a different color.  (Com_Show_Scenario_Dialog
            is responsible for ensuring the colors are different.)
            ..................................................................*/
            case SERIAL_GAME_OPTIONS:
              gameoptions = true;
              kludge_timer.Set(static_cast<int64_t>(2) * 60);
              port::SafeCopy(TheirName, ReceivePacket.Name);
              TheirColor = ReceivePacket.ScenarioInfo.Color;
              TheirHouse = ReceivePacket.ScenarioInfo.House;
              transmit = true;

              display = std::max(display, REDRAW_MESSAGE);

              //.........................................................
              // "Clip" the other system's version range to our own
              // ........................................................
              version =
                  VerNum.Clip_Version(ReceivePacket.ScenarioInfo.MinVersion,
                                      ReceivePacket.ScenarioInfo.MaxVersion);
              // ........................................................
              // If the greatest-common-version comes back 0, the other
              // system's range is too low for ours
              // ........................................................
              if (version == 0) {
                WWMessageBox().Process(TXT_DESTGAME_OUTDATED);

                // to skip the other system not responding msg
                lastmsgtime = TickCount.Value();

                process = false;
                rc = 0;
              } else if (version == 0xffffffff) {
                // ........................................................
                // If the greatest-common-version comes back 0xffffffff,
                // the other system's range is too high for ours
                // ........................................................
                WWMessageBox().Process(TXT_YOURGAME_OUTDATED);

                // to skip the other system not responding msg
                lastmsgtime = TickCount.Value();

                process = false;
                rc = 0;
              } else {
                if (ReceivePacket.ScenarioInfo.CheatCheck !=
                    RuleINI.Get_Unique_ID()) {
                  WWMessageBox().Process(TXT_MISMATCH);

                  // to skip the other system not responding msg
                  lastmsgtime = TickCount.Value();

                  process = false;
                  rc = 0;

                } else {
                  // ........................................................
                  // Otherwise, 'version' is the highest version we have in
                  // common; look up the protocol that goes with this version.
                  // ........................................................
                  Session.CommProtocol =
                      VersionClass::Version_Protocol(version);
                }
              }
              /*.........................................................
              If this is the first game-options packet we've received,
              init the game & player lists
              .........................................................*/
              if (playerlist.Count() == 0) {
                //......................................................
                // Add two strings to the player list
                //......................................................
                playerlist.Add_Item("", &ColorRemaps.at(Session.ColorIdx));
                playerlist.Add_Item("", &ColorRemaps.at(TheirColor));
              }

              //.........................................................
              // Ensure the player list has the latest, greatest copy of
              // our names & colors.  Do this every time we receive an
              // options packet.
              //.........................................................
#ifdef OLDWAY
              if (Session.House == HOUSE_GOOD) {
                sprintf(item, "%s\t%s", namebuf, Text_String(TXT_ALLIES));
              } else {
                sprintf(item, "%s\t%s", namebuf, Text_String(TXT_SOVIET));
              }
#else   // OLDWAY
              absl::SNPrintF(
                  item, sizeof(item), "%s\t%s", namebuf,
                  Text_String(
                      HouseTypeClass::As_Reference(Session.House).Full_Name()));
#endif  // OLDWAY
              playerlist.Set_Item(0, item);
              playerlist.Colors.at(0) = &ColorRemaps.at(
                  Session.ColorIdx == PCOLOR_DIALOG_BLUE ? PCOLOR_REALLY_BLUE
                                                         : Session.ColorIdx);

#ifdef OLDWAY
              if (TheirHouse == HOUSE_GOOD) {
                sprintf(item, "%s\t%s", TheirName, Text_String(TXT_ALLIES));
              } else {
                sprintf(item, "%s\t%s", TheirName, Text_String(TXT_SOVIET));
              }
#else   // OLDWAY
              absl::SNPrintF(
                  item, sizeof(item), "%s\t%s", TheirName,
                  Text_String(
                      HouseTypeClass::As_Reference(TheirHouse).Full_Name()));
#endif  // OLDWAY
              playerlist.Set_Item(1, item);
              playerlist.Colors.at(1) = &ColorRemaps.at(
                  TheirColor == PCOLOR_DIALOG_BLUE ? PCOLOR_REALLY_BLUE
                                                   : TheirColor);

              playerlist.Flag_To_Redraw();

              //.........................................................
              // Play a little sound effect
              //.........................................................
              Sound_Effect(VOC_OPTIONS_CHANGED);

              break;

            /*..................................................................
            Incoming message: add to our list
            ..................................................................*/
            case SERIAL_MESSAGE:
              Session.Messages.Add_Message(
                  ReceivePacket.Name,
                  static_cast<int>(
                      static_cast<PlayerColorType>(ReceivePacket.ID) ==
                              PCOLOR_DIALOG_BLUE
                          ? PCOLOR_REALLY_BLUE
                          : static_cast<PlayerColorType>(ReceivePacket.ID)),
                  ReceivePacket.Message.Message,
                  static_cast<PlayerColorType>(ReceivePacket.ID) ==
                          PCOLOR_DIALOG_BLUE
                      ? PCOLOR_REALLY_BLUE
                      : static_cast<PlayerColorType>(ReceivePacket.ID),
                  kTpfText, -1);

              Sound_Effect(VOC_INCOMING_MESSAGE);
              display = std::max(display, REDRAW_MESSAGE);

              break;

            //
            // get their response time
            //
            case SERIAL_TIMING:
              theirresponsetime = ReceivePacket.ScenarioInfo.ResponseTime;

              if (!gameoptions) {
                // retransmit of game options packet again
                transmit = true;
              }
              break;

            //
            // print msg waiting for opponent
            //
            case SERIAL_SCORE_SCREEN:
              display = std::max(display, REDRAW_MESSAGE);
              break;

            case SerialCommandType::SERIAL_CONNECT:
            case SerialCommandType::SERIAL_GO:
            case SerialCommandType::SERIAL_LOADGAME:
            case SerialCommandType::SERIAL_LAST_COMMAND:
            case SerialCommandType::SERIAL_REQ_SCENARIO:
            case SerialCommandType::SERIAL_FILE_INFO:
            case SerialCommandType::SERIAL_FILE_CHUNK:
            case SerialCommandType::SERIAL_READY_TO_GO:
            case SerialCommandType::SERIAL_NO_SCENARIO:
            default:
              break;
          }
        }
      }

      // if we haven't received a msg for 10 seconds exit

      if (!skirmish && TickCount.Value() - lastmsgtime > msg_timeout) {
        WWMessageBox().Process(TXT_SYSTEM_NOT_RESPONDING);
        process = false;
        rc = 0;

        // say we did receive sign off to keep from sending one
        recsignedoff = true;
      }

      /*---------------------------------------------------------------------
      Service the connection
      ---------------------------------------------------------------------*/
      if (!skirmish) {
        NullModem.Service();
      }
    }

    /*------------------------------------------------------------------------
    Prepare to load the scenario
    ------------------------------------------------------------------------*/
    if (rc) {
      Session.NumPlayers = skirmish ? 1 : 2;

      Scen.Scenario = Session.Options.ScenarioIndex;
      port::SafeCopy(
          Scen.ScenarioName,
          Session.Scenarios.at(Session.Options.ScenarioIndex)->Get_Filename());

      /*.....................................................................
      Add both players to the Players vector; the local system is always
      index 0.
      .....................................................................*/
      who = new NodeNameType;
      port::SafeCopy(who->Name, namebuf);
      who->Player.House = Session.House;
      who->Player.Color = Session.ColorIdx;
      who->Player.ProcessTime = -1;
      Session.Players.Add(who);

      /*
      **	Fetch the difficulty setting when in skirmish mode.
      */
      if (skirmish) {
        const int diff =
            difficulty.Get_Value() * (Rule.IsFineDifficulty ? 1 : 2);
        switch (diff) {
          case 0:
            Scen.CDifficulty = DIFF_HARD;
            Scen.Difficulty = DIFF_EASY;
            break;

          case 1:
            Scen.CDifficulty = DIFF_HARD;
            Scen.Difficulty = DIFF_NORMAL;
            break;

          case 2:
            Scen.CDifficulty = DIFF_NORMAL;
            Scen.Difficulty = DIFF_NORMAL;
            break;

          case 3:
            Scen.CDifficulty = DIFF_EASY;
            Scen.Difficulty = DIFF_NORMAL;
            break;

          case 4:
            Scen.CDifficulty = DIFF_EASY;
            Scen.Difficulty = DIFF_HARD;
            break;
          default:
            break;
        }
      } else {
        Scen.CDifficulty = DIFF_NORMAL;
        Scen.Difficulty = DIFF_NORMAL;
      }

      if (!skirmish) {
        who = new NodeNameType;

        /* If the names of the players are the same then we MUST force them
         * be be unique. This is necessary to prevent a crash after loading
         * a modem save game.
         */
        if (std::string_view(TheirName) == namebuf) {
          if (std::string_view(TheirName).size() == MPLAYER_NAME_MAX - 1) {
            TheirName[MPLAYER_NAME_MAX - 1] = '\0';
          } else {
            port::SafeAppend(TheirName, "2");
          }
        }

        port::SafeCopy(who->Name, TheirName);
        who->Player.House = TheirHouse;
        who->Player.Color = TheirColor;
        who->Player.ProcessTime = -1;
        Session.Players.Add(who);
      }

      /*.....................................................................
      Send all players a GO packet.
      .....................................................................*/
      base::FillBytes(base::ObjectBytes(SendPacket), 0, sizeof(SendPacket));
      if (load_game) {
        SendPacket.Command = SERIAL_LOADGAME;
      } else {
        SendPacket.Command = SERIAL_GO;
      }

      if (!skirmish) {
        SendPacket.ScenarioInfo.ResponseTime = NullModem.Response_Time();
        if (theirresponsetime != 10000) {
          SendPacket.ScenarioInfo.ResponseTime =
              std::max(SendPacket.ScenarioInfo.ResponseTime, theirresponsetime);
        }
      }

      //
      // calculated one way delay for a packet and overall delay to execute
      // a packet
      //
      if (!skirmish) {
        if (Session.CommProtocol == COMM_PROTOCOL_MULTI_E_COMP) {
          Session.MaxAhead = static_cast<int>(std::max<int64_t>(
              ((SendPacket.ScenarioInfo.ResponseTime / 8) +
               (Session.FrameSendRate - 1)) /
                  Session.FrameSendRate * Session.FrameSendRate,
              Session.FrameSendRate * 2));
        } else {
          Session.MaxAhead = std::max(
              SendPacket.ScenarioInfo.ResponseTime / 8, MODEM_MIN_MAX_AHEAD);
        }
      }
      SendPacket.ID = static_cast<unsigned char>(Session.ModemType);

      if (!skirmish) {
        NullModem.Send_Message(base::ObjectBytes(SendPacket),
                               sizeof(SendPacket), 1);
        starttime = TickCount.Value();
        while ((NullModem.Num_Send() &&
                TickCount.Value() - starttime < PACKET_SENDING_TIMEOUT) ||
               TickCount.Value() - starttime < 60) {

          NullModem.Service();
        }

        /*
        ** Wait for the go response. This will be either a 'GO' reply, a
        ** request for the scenario to be sent or a reply to say that the
        * scenario
        ** cant be played.
        */
        WWDebugString("RA95 - About to wait for 'GO' response.\n");
        do {
          NullModem.Service();

          if (NullModem.Get_Message(base::ObjectBytes(ReceivePacket),
                                    &packetlen) > 0) {
            if (ReceivePacket.Command == SERIAL_READY_TO_GO) {
              if (Session.Scenarios.at(Session.Options.ScenarioIndex)
                      ->Get_Official() &&
                  (!Force_Scenario_Available(Scen.ScenarioName))) {
                Emergency_Exit(EXIT_FAILURE);
              }

              break;
            }

            if (ReceivePacket.Command == SERIAL_NO_SCENARIO) {
              WWMessageBox().Process(TXT_NO_EXPANSION_SCENARIO, TXT_CANCEL);
              /*
              ** We have to recover from this somehow so restart the session.
              */
              process = true;
              display = REDRAW_ALL;
              lastmsgtime = TickCount.Value();
              retry_setup = true;
              break;
            }

            if (ReceivePacket.Command == SERIAL_REQ_SCENARIO) {
              WWDebugString("RA95 - About to call 'Send_Remote_File'.\n");

              if (Session.Scenarios.at(Session.Options.ScenarioIndex)
                      ->Get_Official() &&
                  (!Force_Scenario_Available(Scen.ScenarioName))) {
                Emergency_Exit(EXIT_FAILURE);
              }

              Send_Remote_File(Scen.ScenarioName, 0);

              break;
            }
          }

        } while (!Keyboard->Check() && !retry_setup);

        // clear queue to keep from doing any resends
        NullModem.Init_Send_Queue();
      }

      if (retry_setup) {
        continue;
      }

    } else {
      if ((!recsignedoff) && (!skirmish))
      /*.....................................................................
      Broadcast my sign-off over my network
      .....................................................................*/
      {
        base::FillBytes(base::ObjectBytes(SendPacket), 0, sizeof(SendPacket));
        SendPacket.Command = SERIAL_SIGN_OFF;
        SendPacket.ScenarioInfo.Color = Session.ColorIdx;  // use Color for ID
        SendPacket.ID = static_cast<unsigned char>(Session.ModemType);
        NullModem.Send_Message(base::ObjectBytes(SendPacket),
                               sizeof(SendPacket), 1);

        starttime = TickCount.Value();
        while ((NullModem.Num_Send() &&
                TickCount.Value() - starttime < PACKET_CANCEL_TIMEOUT) ||
               TickCount.Value() - starttime < 60) {

            if ((NullModem.Get_Message(base::ObjectBytes(ReceivePacket),
                                       &packetlen) > 0) &&
                (ReceivePacket.Command == SERIAL_SIGN_OFF &&
                 ReceivePacket.ID ==
                     static_cast<unsigned char>(Session.ModemType)))
            // are we getting our own packets back??

            {
              // exit while
              break;
            }

            NullModem.Service();
        }
      }

      if (!skirmish) {
        Shutdown_Modem();
      }
    }
  }

  /*------------------------------------------------------------------------
  Clear all lists
  ------------------------------------------------------------------------*/
  while (scenariolist.Count()) {
    scenariolist.Remove_Item(scenariolist.Get_Item(0));
  }

  /*------------------------------------------------------------------------
  Clean up the list boxes
  ------------------------------------------------------------------------*/
  playerlist.Clear();

  /*------------------------------------------------------------------------
  Remove the chat edit box
  ------------------------------------------------------------------------*/
  Session.Messages.Remove_Edit();

  /*------------------------------------------------------------------------
  Restore screen
  ------------------------------------------------------------------------*/
  Hide_Mouse();
  Load_Title_Page(true);
  Show_Mouse();

  /*------------------------------------------------------------------------
  Save any changes made to our options
  ------------------------------------------------------------------------*/
  if (changed) {
    Session.Write_MultiPlayer_Settings();
  }

  if (load_game && !skirmish) {
    if (!Load_Game(-1)) {
      WWMessageBox().Process(TXT_ERROR_LOADING_GAME);
      rc = 0;
    }
    Frame++;
  }

  return rc;
}

/***********************************************************************************************
 * Find_Local_Scenario -- finds the file name of the scenario with matching
 *attributes         *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to Scenario description * ptr to Scenario filename to fix up *
 *           length of file for trivial rejection of scenario files * ptr to
 *digest. Digests must match.                                                *
 *                                                                                             *
 *                                                                                             *
 * OUTPUT:   true if scenario is available locally *
 *                                                                                             *
 * WARNINGS: We need to reject files that don't match exactly because scenarios
 *with the same  * description can exist on both machines but have different
 *contents. For example   * there will be lots of scenarios called 'my map' and
 *'crap' and 'aaaaaa'.          *
 *                                                                                             *
 * HISTORY: * 8/23/96 12:36PM ST : Created *
 *=============================================================================================*/
bool Find_Local_Scenario(const char* description, std::span<char> filename,
                         unsigned int length, const char* digest,
                         bool official) {
  // FILE *fp;
  // fp = fopen("findscen.txt","wt");
  // debugprint("looking for local scenario: description = %s, name=%s,
  // length=%d, digest=%s, official=%d\n", description, filename, length,
  // digest, official);

  char digest_buffer[32];
  /*
  ** Scan through the scenario list looking for scenarios with matching
  *descriptions.
  */
  for (int index = 0; index < Session.Scenarios.Count(); index++) {
    // debugprint( "Checking against scenario: %s\n",
    // Session.Scenarios[index]->Description());
    if (std::string_view(Session.Scenarios.at(index)->Description()) ==
        description) {
      // debugprint("found matching description.\n");
      GameFile file(Session.Scenarios.at(index)->Get_Filename());

      /*
      ** Possible rejection on the basis of availability.
      */
      // debugprint("file is available.\n");
      /*
      ** Possible rejection on the basis of size.
      */
      if (file.IsAvailable() && std::cmp_equal(file.Size(), length)) {
        // debugprint("length matches.\n");
        /*
        ** We don't know the digest for 'official' scenarios so assume its
        *correct
        */
        if (!official) {
          // debugprint("!official.\n");
          /*
          ** Possible rejection on the basis of digest
          */
          INIClass ini;
          ini.Load(file);
          ini.Get_String("Digest", "1", "No digest here mate. Nope.",
                         digest_buffer, sizeof(digest_buffer));
        }
        // debugprint("digest = %s, digest_buffer = %s.\n", digest,
        // digest_buffer);
        // But don't know why this happens.
        // Because of autodownload?
        /*
        ** If this is an aftermath scenario then ignore the digest and return
        *success.
        */
        if (IsMissionAftermath(Session.Scenarios.at(index)->Get_Filename())) {
          // debugprint("a 1match!\n");
          port::SafeCopy(
              std::span(filename).first(port::kMaxFname + port::kMaxExt + 1),
              Session.Scenarios.at(index)->Get_Filename());
          return true;
        }

        /*
        ** This must be the same scenario. Copy the name and return true.
        */
        if (official || (std::string_view(digest) == digest_buffer)) {
          // debugprint("a match!\n");
          port::SafeCopy(
              std::span(filename).first(port::kMaxFname + port::kMaxExt + 1),
              Session.Scenarios.at(index)->Get_Filename());
          return true;
        }
      }

      //			else
      //				debugprint("file not available '%s'.\n",
      // Session.Scenarios[index]->Get_Filename());
    }
  }
  // debugprint("failed match.\n");
  /*
  ** Couldnt find the scenario locally. Return failure.
  */
  return false;
}

/***********************************************************************************************
 * Com_Show_Scenario_Dialog -- Serial game scenario selection dialog
 **
 *                                                                         						  *
 * The 'Players' vector is filled in by this routine, when the game starts; this
 ** is for the Assign_Houses routine, which expects this vector to contain all
 ** players' names & houses & colors.  Other than that, the Players vector,
 *Games					  * vector, and Chat vector aren't used
 *at all by this routine.  The Game & Players				  * list
 *boxes are filled in manually in the processing loop.
 **
 *                                                                         						  *
 *    ┌────────────────────────────────────────────────────────────┐ * │ Serial
 *Game                         │                       	  * │ │ * │ Your Name:
 *__________                    │ * │                       House: [GDI] [NOD]
 *│									  * │
 *Desired Color: [ ][ ][ ][ ]                  │
 ** │                                                            │ * │ Opponent:
 *Name                         │                    	 	  * │ Scenario:
 *Description                  │
 ** │                      Credits: xxxx                         │
 ** │                        Bases: ON                           │ * │ Crates:
 *ON                           │                   	 	  * │ Tiberium:
 *ON                           │                   	 	  * │ Ghosts: ON
 *│                   	 	  * │ │                   	 	  * │
 *[Cancel]                           │                    	 	  * │ │
 ** │   ┌────────────────────────────────────────────────────┐   │ * │   │ │   │
 ** │   │                                                    │   │ * │
 *└────────────────────────────────────────────────────┘   │ * │ [Send Message]
 *│                   	 	  *
 *    └────────────────────────────────────────────────────────────┘ *
 *                                                                         						  *
 * INPUT: * none.
 **
 *                                                                         						  *
 * OUTPUT: * true = success, false = cancel
 **
 *                                                                         						  *
 * WARNINGS: * none.
 **
 *                                                                         						  *
 * HISTORY: * 02/14/1995 BR : Created. *
 *=============================================================================================*/
int Com_Show_Scenario_Dialog() {
  /*........................................................................
  Dialog & button dimensions
  ........................................................................*/
  const int d_dialog_w = 640;                             // dialog width
  const int d_dialog_h = 400;                             // dialog height
  const int d_dialog_x = (640 - d_dialog_w) / 2;          // dialog x-coord
  const int d_dialog_y = (400 - d_dialog_h) / 2;          // dialog y-coord
  const int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);  // center x-coord

  const int d_txt6_h = 12 + 1;  // ht of 6-pt text
  const int d_margin1 = 10;     // margin width/height
  const int d_margin2 = 4;      // margin width/height

  const int d_name_w = 140;
  const int d_name_h = 18;
  const int d_name_x = d_dialog_x + (d_dialog_w / 4) - (d_name_w / 2);
  const int d_name_y = d_dialog_y + d_margin1 + d_margin2 + d_txt6_h + 2;

#ifdef OLDWAY
  int d_gdi_w = 80;
  int d_gdi_h = 18;
  int d_gdi_x = d_dialog_cx - d_gdi_w;
  int d_gdi_y = d_name_y;

  int d_nod_w = 80;
  int d_nod_h = 18;
  int d_nod_x = d_dialog_cx;
  int d_nod_y = d_name_y;

#else  // OLDWAY

  const int d_house_w = 120;
  const int d_house_h = 8 * 10;
  const int d_house_x = d_dialog_cx - (d_house_w / 2);
  const int d_house_y = d_name_y;

#endif  // OLDWAY

  const int d_color_w = 20;
  const int d_color_h = 18;
  const int d_color_x = d_dialog_x + (d_dialog_w / 4 * 3) - (d_color_w * 3);
  const int d_color_y = d_name_y;

  const int d_scenario_y = d_name_y + d_name_h + d_margin2;

  const int d_gamelist_w = 320;
  const int d_gamelist_h = (6 * 12) + 6;  // 6 rows high
  const int d_gamelist_x = d_dialog_x + d_margin1 + 20;
  const int d_gamelist_y =
      d_scenario_y + d_txt6_h + d_margin2 + d_txt6_h + d_margin2;

  // BG	int d_playerlist_w = 112 * 2;
  const int d_playerlist_w = 236;
  const int d_playerlist_h = (6 * 12) + 6;  // 6 rows high
  const int d_playerlist_x =
      d_dialog_x + d_dialog_w - d_margin1 - d_margin1 - d_playerlist_w - 10;
  const int d_playerlist_y = d_gamelist_y;

  const int d_count_w = 50;
  const int d_count_h = d_txt6_h;
  const int d_count_x = d_gamelist_x + (d_gamelist_w / 2);
  const int d_count_y =
      d_gamelist_y + d_gamelist_h + (d_margin1 * 2) - d_margin2;

  const int d_level_w = 50;
  const int d_level_h = d_txt6_h;
  const int d_level_x = d_gamelist_x + (d_gamelist_w / 2);
  const int d_level_y = d_count_y + d_count_h;

  const int d_credits_w = 50;
  const int d_credits_h = d_txt6_h;
  const int d_credits_x = d_gamelist_x + (d_gamelist_w / 2);
  const int d_credits_y = d_level_y + d_level_h;

  const int d_aiplayers_w = 50;
  const int d_aiplayers_h = d_txt6_h;
  const int d_aiplayers_x = d_gamelist_x + (d_gamelist_w / 2);
  const int d_aiplayers_y = d_credits_y + d_credits_h;

  const int d_options_w = 224;
  const int d_options_h = (5 * 12) + 8;
  const int d_options_x = d_playerlist_x;
  const int d_options_y =
      d_playerlist_y + d_playerlist_h + d_margin1 - d_margin2;

  const int d_message_w = d_dialog_w - (d_margin1 * 2) - 40;
  const int d_message_h = (7 * d_txt6_h) + 6;  // 7 rows high
  const int d_message_x = d_gamelist_x;        // d_dialog_x + d_margin1 + 20;
  const int d_message_y =
      d_options_y + d_options_h + d_margin2 /*KO + d_margin1*/;

  const int d_send_w = d_message_w;
  const int d_send_h = 18;
  const int d_send_x = d_message_x;
  const int d_send_y = d_message_y + d_message_h;

  const int d_cancel_w = 90;
  const int d_cancel_x = d_dialog_cx - (d_cancel_w / 2);
  const int d_cancel_y = d_send_y + d_send_h /*KO + d_margin2*/;

  /*........................................................................
  Button Enumerations
  ........................................................................*/
  constexpr int kButtonName = 100;
#ifdef OLDWAY
  constexpr int kButtonGdi = 101;
  constexpr int kButtonNod = 102;
  constexpr int kButtonGamelist = 103;
#else   // OLDWAY
  constexpr int kButtonHouse = 101;
  constexpr int kButtonGamelist = 102;
#endif  // OLDWAY
  constexpr int kButtonPlayerlist = kButtonGamelist + 1;
  constexpr int kButtonCancel = kButtonPlayerlist + 1;
  constexpr int kButtonCount = kButtonCancel + 1;
  constexpr int kButtonLevel = kButtonCount + 1;
  constexpr int kButtonCredits = kButtonLevel + 1;
  constexpr int kButtonAiPlayers = kButtonCredits + 1;
  constexpr int kButtonOptions = kButtonAiPlayers + 1;

  /*........................................................................
  Redraw values: in order from "top" to "bottom" layer of the dialog
  ........................................................................*/
  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_PARMS = 1,
    REDRAW_MESSAGE = 2,
    REDRAW_COLORS = 3,
    REDRAW_BUTTONS = 4,
    REDRAW_BACKGROUND = 5,
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;

  /*........................................................................
  Dialog variables
  ........................................................................*/
  RedrawType display = REDRAW_ALL;  // redraw level
  const int cbox_x[] = {d_color_x,
                        d_color_x + d_color_w,
                        d_color_x + (d_color_w * 2),
                        d_color_x + (d_color_w * 3),
                        d_color_x + (d_color_w * 4),
                        d_color_x + (d_color_w * 5),
                        d_color_x + (d_color_w * 6),
                        d_color_x + (d_color_w * 7)};

  char namebuf[MPLAYER_NAME_MAX] = {0};  // buffer for player's name
  // BG	const int playertabs[] = {77};				// tabs for
  // player list box
  const int playertabs[] = {71 * 2};    // tabs for player list box
  const int optiontabs[] = {8};         // tabs for options list box
  bool transmit = false;                // 1 = re-transmit new game options
  bool first = false;                   // 1 = no packets received yet
  bool parms_received = false;          // 1 = game options received
  bool changed = false;                 // 1 = user has changed an option

  int rc = 0;
  int recsignedoff = 0;
  int i = 0;
  uint32_t version = 0;
  char txt[80];
  int64_t starttime = 0;
  int64_t timingtime = 0;
  int64_t lastmsgtime = 0;
  int64_t lastredrawtime = 0;
  int64_t transmittime = 0;
  int packetlen = 0;
  bool oppscorescreen = false;
  // event ptr
  int64_t msg_timeout = 1200;  // init to 20 seconds
  bool load_game = false;            // 1 = load saved game
  NodeNameType* who = nullptr;       // node to add to Players
  char item[MPLAYER_NAME_MAX + 64];  // for filling in lists
  const char* p = nullptr;
  RemapControlType* scheme = GadgetClass::Get_Color_Scheme();
  Session.Options.ScenarioDescription[0] =
      0;  // Flag that we dont know the scenario name yet
  bool messages_have_focus = true;
  bool ready_packet_was_sent = false;

  /*........................................................................
  Buttons
  ........................................................................*/
  GadgetClass* commands = nullptr;  // button list

  EditClass name_edt(kButtonName, namebuf, MPLAYER_NAME_MAX, kTpfText, d_name_x,
                     d_name_y, d_name_w, d_name_h, EditClass::kAlphanumeric);
#ifdef OLDWAY
  TextButtonClass gdibtn(kButtonGdi, TXT_ALLIES, kTpfButton, d_gdi_x, d_gdi_y,
                         d_gdi_w, d_gdi_h);
  TextButtonClass nodbtn(kButtonNod, TXT_SOVIET, kTpfButton, d_nod_x, d_nod_y,
                         d_nod_w, d_nod_h);
#else   // OLDWAY
  char housetext[25] = "";
  Fancy_Text_Print("", 0, 0, nullptr, 0, kTpfText);
  DropListClass housebtn(kButtonHouse, housetext, sizeof(housetext), kTpfText,
                         d_house_x, d_house_y, d_house_w, d_house_h,
                         MixArchive::RetrieveData("BTN-UP.SHP"),
                         MixArchive::RetrieveData("BTN-DN.SHP"));
#endif  // OLDWAY
  TextButtonClass cancelbtn(kButtonCancel, TXT_CANCEL, kTpfButton, d_cancel_x,
                            d_cancel_y, d_cancel_w);
  ListClass gamelist(kButtonGamelist, d_gamelist_x, d_gamelist_y, d_gamelist_w,
                     d_gamelist_h, kTpfText,
                     MixArchive::RetrieveData("BTN-UP.SHP"),
                     MixArchive::RetrieveData("BTN-DN.SHP"));
  ColorListClass playerlist(kButtonPlayerlist, d_playerlist_x, d_playerlist_y,
                            d_playerlist_w, d_playerlist_h, kTpfText,
                            MixArchive::RetrieveData("BTN-UP.SHP"),
                            MixArchive::RetrieveData("BTN-DN.SHP"));

  GaugeClass countgauge(kButtonCount, d_count_x, d_count_y, d_count_w,
                        d_count_h);
  char staticcountbuff[35];
  StaticButtonClass staticcount(0, "     ", kTpfText, d_count_x + d_count_w + 6,
                                d_count_y);

  GaugeClass levelgauge(kButtonLevel, d_level_x, d_level_y, d_level_w,
                        d_level_h);
  char staticlevelbuff[35];
  StaticButtonClass staticlevel(0, "     ", kTpfText, d_level_x + d_level_w + 6,
                                d_level_y);

  GaugeClass creditsgauge(kButtonCredits, d_credits_x, d_credits_y, d_credits_w,
                          d_credits_h);
  char staticcreditsbuff[35];
  StaticButtonClass staticcredits(0, "         ", kTpfText,
                                  d_credits_x + d_credits_w + 6, d_credits_y);

  GaugeClass aiplayersgauge(kButtonAiPlayers, d_aiplayers_x, d_aiplayers_y,
                            d_aiplayers_w, d_aiplayers_h);
  char staticaibuff[35];
  StaticButtonClass staticai(0, "     ", kTpfText,
                             d_aiplayers_x + d_aiplayers_w + 6, d_aiplayers_y);

  CheckListClass optionlist(kButtonOptions, d_options_x, d_options_y,
                            d_options_w, d_options_h, kTpfText,
                            MixArchive::RetrieveData("BTN-UP.SHP"),
                            MixArchive::RetrieveData("BTN-DN.SHP"));

  /*
  ------------------------- Build the button list --------------------------
  */
  commands = &name_edt;
  staticcount.Add_Tail(*commands);
  staticcredits.Add_Tail(*commands);
  staticai.Add_Tail(*commands);
  staticlevel.Add_Tail(*commands);
  cancelbtn.Add_Tail(*commands);
  gamelist.Add_Tail(*commands);
  playerlist.Add_Tail(*commands);
  countgauge.Add_Tail(*commands);
  levelgauge.Add_Tail(*commands);
  creditsgauge.Add_Tail(*commands);
  aiplayersgauge.Add_Tail(*commands);
  optionlist.Add_Tail(*commands);
#ifdef OLDWAY
  gdibtn.Add_Tail(*commands);
  nodbtn.Add_Tail(*commands);
#else   // OLDWAY
  housebtn.Add_Tail(*commands);
#endif  // OLDWAY

  //------------------------------------------------------------------------
  //	Init the button states
  //------------------------------------------------------------------------
  //........................................................................
  // Name & Color
  //........................................................................
  Session.ColorIdx = Session.PrefColor;     // init my preferred color
  port::SafeCopy(namebuf, Session.Handle);  // set my name
  name_edt.Set_Text(namebuf, MPLAYER_NAME_MAX);
  name_edt.Set_Color(&ColorRemaps.at(Session.ColorIdx == PCOLOR_DIALOG_BLUE
                                         ? PCOLOR_REALLY_BLUE
                                         : Session.ColorIdx));

  //........................................................................
  // List boxes
  //........................................................................
  playerlist.Set_Tabs(playertabs);
  playerlist.Set_Selected_Style(ColorListClass::SELECT_NORMAL);

  optionlist.Set_Tabs(optiontabs);
  optionlist.Set_Read_Only(true);

  optionlist.Add_Item(Text_String(TXT_BASES));
  optionlist.Add_Item(Text_String(TXT_ORE_SPREADS));
  optionlist.Add_Item(Text_String(TXT_CRATES));
  optionlist.Add_Item(Text_String(TXT_CAPTURE_THE_FLAG));
  optionlist.Add_Item(Text_String(TXT_SHADOW_REGROWS));

  optionlist.Check_Item(0, Session.Options.Bases != 0);
  optionlist.Check_Item(1, Session.Options.Tiberium != 0);
  optionlist.Check_Item(2, Session.Options.Goodies != 0);
  optionlist.Check_Item(3, Special.IsCaptureTheFlag);
  optionlist.Check_Item(4, Special.IsShadowGrow);

  //........................................................................
  // House buttons
  //........................................................................
#ifdef OLDWAY
  if (Session.House == HOUSE_GOOD) {
    gdibtn.Turn_On();
  } else {
    nodbtn.Turn_On();
  }
#else   // OLDWAY
  for (HousesType house = HOUSE_USSR; house <= HOUSE_FRANCE; house++) {
    housebtn.Add_Item(
        Text_String(HouseTypeClass::As_Reference(house).Full_Name()));
  }
  housebtn.Set_Selected_Index(static_cast<int>(Session.House) -
                              static_cast<int>(HOUSE_USSR));
  housebtn.Set_Read_Only(true);
#endif  // OLDWAY

  //........................................................................
  // Option gauges
  //........................................................................
  countgauge.Use_Thumb(false);
  countgauge.Set_Maximum(
      base::At(SessionClass::CountMax, Session.Options.Bases) -
      base::At(SessionClass::CountMin, Session.Options.Bases));
  countgauge.Set_Value(Session.Options.UnitCount -
                       base::At(SessionClass::CountMin, Session.Options.Bases));

  levelgauge.Use_Thumb(false);
  levelgauge.Set_Maximum(MPLAYER_BUILD_LEVEL_MAX - 1);
  levelgauge.Set_Value(BuildLevel - 1);

  creditsgauge.Use_Thumb(false);
  creditsgauge.Set_Maximum(Rule.MPMaxMoney);
  creditsgauge.Set_Value(Session.Options.Credits);

  aiplayersgauge.Use_Thumb(false);
  aiplayersgauge.Set_Maximum(Rule.MaxPlayers - 2);
  aiplayersgauge.Set_Value(Session.Options.AIPlayers);

  Fancy_Text_Print("", 0, 0, scheme, kTBlack, TPF_CENTER | kTpfText);

  transmit = true;
  first = true;

  /*........................................................................
  Clear the Players vector
  ........................................................................*/
  Clear_Vector(&Session.Players);

  /*........................................................................
  Init the message display system
  ........................................................................*/
  Session.Messages.Init(d_message_x + 1, d_message_y + 1, 7, MAX_MESSAGE_LENGTH,
                        d_txt6_h, d_send_x + 2,
                        d_send_y + 2, 1, 20, MAX_MESSAGE_LENGTH - 5,
                        d_message_w);
  Session.Messages.Add_Edit(Session.ColorIdx == PCOLOR_DIALOG_BLUE
                                ? PCOLOR_REALLY_BLUE
                                : Session.ColorIdx,
                            kTpfText, nullptr, '_', d_message_w);
  Session.WWChat = false;

  /*........................................................................
  Init version number clipping system
  ........................................................................*/
  VerNum.Init_Clipping();
  Load_Title_Page(true);
  CCPalette.Set();

  // TODO(konove): This is ugly and just for printing a message.
  if (std::string_view(ModemRXString).size() > 36) {
    ModemRXString[36] = 0;
  }

  if (!std::string_view(ModemRXString).empty()) {
    Session.Messages.Add_Message(nullptr, 0, ModemRXString, PCOLOR_BROWN,
                                 kTpfText, -1);
  }

  ModemRXString[0] = '\0';

  /*
  ---------------------------- Processing loop -----------------------------
  */
  NullModem.Reset_Response_Time();  // clear response time
  timingtime = lastmsgtime = lastredrawtime = TickCount.Value();

  bool process = true;  // process while true
  while (process) {

    /*
    ** Kludge to make sure we redraw the message input line when it loses focus.
    ** If we dont do this then the cursor doesnt disappear.
    */
    if (messages_have_focus) {
      if (name_edt.Has_Focus()) {
        display = std::max(display, REDRAW_MESSAGE);
      }
    } else {
      if (!name_edt.Has_Focus()) {
        display = std::max(display, REDRAW_MESSAGE);
        Session.Messages.Set_Edit_Focus();
      }
    }

    /*
    ........................ Invoke game callback .........................
    */
    ServiceRealTime();

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
    ...................... Refresh display if needed ......................
    */
    if (display != REDRAW_NONE) {
      if (housebtn.IsDropped) {
        housebtn.Collapse();
        display = REDRAW_BACKGROUND;
      }
      Hide_Mouse();
      /*
      .................. Redraw backgound & dialog box ...................
      */
      if (display >= REDRAW_BACKGROUND) {
        Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);

        /*...............................................................
        Dialog & Field labels
        ...............................................................*/
        Fancy_Text_Print(TXT_CHANNEL_GAMES, d_gamelist_x + (d_gamelist_w / 2),
                         d_gamelist_y - d_txt6_h, scheme, kTBlack,
                         TPF_CENTER | kTpfText);
        Fancy_Text_Print(TXT_PLAYERS, d_playerlist_x + (d_playerlist_w / 2),
                         d_playerlist_y - d_txt6_h, scheme, kTBlack,
                         TPF_CENTER | kTpfText);
        Fancy_Text_Print(TXT_YOUR_NAME, d_name_x + (d_name_w / 2),
                         d_name_y - d_txt6_h, scheme, kTBlack,
                         TPF_CENTER | kTpfText);
#ifdef OLDWAY
        Fancy_Text_Print(TXT_SIDE_COLON, d_gdi_x + d_gdi_w, d_gdi_y - d_txt6_h,
                         scheme, kTBlack, TPF_CENTER | kTpfText);
#else
        Fancy_Text_Print(TXT_SIDE_COLON, d_house_x + (d_house_w / 2),
                         d_house_y - d_txt6_h, scheme, kTBlack,
                         TPF_CENTER | kTpfText);
#endif
        Fancy_Text_Print(TXT_COLOR_COLON, d_dialog_x + (d_dialog_w / 4 * 3),
                         d_color_y - d_txt6_h, scheme, kTBlack,
                         TPF_CENTER | kTpfText);
        Fancy_Text_Print(TXT_COUNT, d_count_x - 4, d_count_y, scheme, kTBlack,
                         kTpfText | TPF_RIGHT);
        Fancy_Text_Print(TXT_LEVEL, d_level_x - 4, d_level_y, scheme, kTBlack,
                         kTpfText | TPF_RIGHT);
        Fancy_Text_Print(TXT_CREDITS_COLON, d_credits_x - 4, d_credits_y,
                         scheme, kTBlack, kTpfText | TPF_RIGHT);
        Fancy_Text_Print(TXT_AI_PLAYERS_COLON, d_aiplayers_x - 4, d_aiplayers_y,
                         scheme, kTBlack, kTpfText | TPF_RIGHT);
      }

      /*..................................................................
      Draw the color boxes
      ..................................................................*/
      if (display >= REDRAW_COLORS) {
        for (i = 0; i < MAX_MPLAYER_COLORS; i++) {
          LogicPage->Fill_Rect(
              base::At(cbox_x, i) + 2, d_color_y + 2,
              base::At(cbox_x, i) + 2 + d_color_w - 4,
              d_color_y + 2 + d_color_h - 4,
              ColorRemaps.at(static_cast<PlayerColorType>(i)).Box);
          //						(i ==
          // PCOLOR_DIALOG_BLUE) ? ColorRemaps[PCOLOR_REALLY_BLUE].Box :
          // ColorRemaps[static_cast<PlayerColorType>(i)].Box);

          if (static_cast<PlayerColorType>(i) == Session.ColorIdx) {
            Draw_Box(base::At(cbox_x, i), d_color_y, d_color_w, d_color_h,
                     BOXSTYLE_DOWN, false);
          } else {
            Draw_Box(base::At(cbox_x, i), d_color_y, d_color_w, d_color_h,
                     BOXSTYLE_RAISED, false);
          }
        }
      }

      /*..................................................................
      Draw the message system; erase old messages first
      ..................................................................*/
      if (display >= REDRAW_MESSAGE) {
        Draw_Box(d_message_x, d_message_y, d_message_w, d_message_h,
                 BOXSTYLE_BOX, true);
        Draw_Box(d_send_x, d_send_y, d_send_w, d_send_h, BOXSTYLE_BOX, true);
        Session.Messages.Draw();

        //..................................................................
        // Redraw the game options
        //..................................................................
        if (display >= REDRAW_PARMS && parms_received) {
          if (oppscorescreen) {
            absl::SNPrintF(txt, sizeof(txt), "%s",
                           Text_String(TXT_WAITING_FOR_OPPONENT));

            Fancy_Text_Print(txt, d_dialog_cx, d_scenario_y, scheme, kTBlack,
                             TPF_CENTER | kTpfText);
          } else {
            /*............................................................
            Scenario description
            ............................................................*/
            // LogicPage->Fill_Rect(d_dialog_x + 16*2, d_scenario_y,
            //	d_dialog_x + d_dialog_w - 16*2, d_scenario_y + d_txt6_h,
            // BLACK);

            p = Text_String(TXT_SCENARIO_COLON);
            if (Session.Options.ScenarioDescription[0]) {
              //							sprintf(txt,"%s
              //%s",p, Session.Options.ScenarioDescription);
              // Fancy_Text_Print (txt, d_dialog_cx, d_scenario_y, scheme,
              // TBLACK, kTpfText | TPF_CENTER);

              // EW - Scenario language translation goes here!!!!!!!! VG
              for (i = 0; base::At(EngMisStr, base::ToSize(i)) != nullptr;
                   i++) {
                if (std::string_view(Session.Options.ScenarioDescription) ==
                    base::At(EngMisStr, base::ToSize(i))) {
                  absl::SNPrintF(
                      txt, sizeof(txt), "%s %s", p,
                      config::kIsEnglish
                          ? Session.Options.ScenarioDescription
                          : base::At(EngMisStr, base::ToSize(i + 1)));
                  break;
                }
              }
              if (base::At(EngMisStr, base::ToSize(i)) == nullptr) {
                absl::SNPrintF(txt, sizeof(txt), "%s %s", p,
                               Session.Options.ScenarioDescription);
              }
              Fancy_Text_Print(txt, d_dialog_cx, d_scenario_y, scheme, kTBlack,
                               kTpfText | TPF_CENTER);

            } else {
              absl::SNPrintF(txt, sizeof(txt), "%s %s", p,
                             Text_String(TXT_NOT_FOUND));

              Fancy_Text_Print(txt, d_dialog_cx, d_scenario_y,
                               &ColorRemaps.at(PCOLOR_RED), kTBlack,
                               kTpfText | TPF_CENTER);
            }

            //.........................................................
            // Unit count, tech level, credits
            //.........................................................
            // LogicPage->Fill_Rect(d_count_x + d_count_w + 2 * 2,
            // d_count_y, 	d_count_x + d_count_w + 35 * 2,
            // d_aiplayers_y
            //+ d_aiplayers_h+2, 	BLACK);

            absl::SNPrintF(staticcountbuff, sizeof(staticcountbuff), "%d",
                           Session.Options.UnitCount);
            staticcount.Set_Text(staticcountbuff);
            staticcount.Draw_Me();
            if (BuildLevel <= MPLAYER_BUILD_LEVEL_MAX) {
              absl::SNPrintF(staticlevelbuff, sizeof(staticlevelbuff), "%d ",
                             BuildLevel);
            } else {
              absl::SNPrintF(staticlevelbuff, sizeof(staticlevelbuff), "**");
            }
            staticlevel.Set_Text(staticlevelbuff);
            staticlevel.Draw_Me();

            absl::SNPrintF(staticcreditsbuff, sizeof(staticcreditsbuff), "%d",
                           Session.Options.Credits);
            staticcredits.Set_Text(staticcreditsbuff);
            staticcredits.Draw_Me();

            absl::SNPrintF(staticaibuff, sizeof(staticaibuff), "%d",
                           Session.Options.AIPlayers);
            staticai.Set_Text(staticaibuff);
            staticai.Draw_Me();
          }
        }
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
    messages_have_focus = Session.Messages.Has_Edit_Focus();
    const bool droplist_is_dropped = housebtn.IsDropped;
    KeyNumType input = commands->Input();

    /*
    ** Sort out the input focus between the name edit box and the message system
    */
    if (messages_have_focus) {
      if (!name_edt.Has_Focus()) {
        Session.Messages.Set_Edit_Focus();
      } else {
        messages_have_focus = false;
        display = std::max(display, REDRAW_MESSAGE);
      }
    }

    /*
    ** Redraw everything if the droplist collapsed
    */
    if (droplist_is_dropped && !housebtn.IsDropped) {
      display = REDRAW_BACKGROUND;
    }

    /*
    ---------------------------- Process input ----------------------------
    */
    switch (static_cast<int>(input)) {
      /*------------------------------------------------------------------
      User clicks on a color button
      ------------------------------------------------------------------*/
      case KN_LMOUSE:
        if (Keyboard->MouseQX > cbox_x[0] &&
            Keyboard->MouseQX < cbox_x[MAX_MPLAYER_COLORS - 1] + d_color_w &&
            Keyboard->MouseQY > d_color_y &&
            Keyboard->MouseQY < d_color_y + d_color_h) {
          /*.........................................................
          Compute my preferred color as the one I clicked on.
          .........................................................*/
          Session.PrefColor = static_cast<PlayerColorType>(
              (Keyboard->MouseQX - cbox_x[0]) / d_color_w);
          changed = true;

          /*.........................................................
          If 'TheirColor' is set to the other player's color, make
          sure we can't pick that color.
          .........................................................*/
          if (parms_received && (Session.PrefColor == TheirColor)) {
            break;
          }

          Session.ColorIdx = Session.PrefColor;

          name_edt.Set_Color(&ColorRemaps.at(
              Session.ColorIdx == PCOLOR_DIALOG_BLUE ? PCOLOR_REALLY_BLUE
                                                     : Session.ColorIdx));
          name_edt.Flag_To_Redraw();
          Session.Messages.Set_Edit_Color(Session.ColorIdx == PCOLOR_DIALOG_BLUE
                                              ? PCOLOR_REALLY_BLUE
                                              : Session.ColorIdx);
          display = std::max(display, REDRAW_COLORS);
          port::SafeCopy(Session.Handle, namebuf);
          transmit = true;
          if (housebtn.IsDropped) {
            housebtn.Collapse();
            display = REDRAW_BACKGROUND;
          }
        } else if ((Get_Mouse_X() >= d_count_x &&
                    Get_Mouse_X() <= d_count_x + d_count_w &&
                    Get_Mouse_Y() >= d_count_y &&
                    Get_Mouse_Y() <= d_aiplayers_y + d_aiplayers_h) ||
                   (Get_Mouse_X() >= d_options_x &&
                    Get_Mouse_X() <= d_options_x + d_options_w &&
                    Get_Mouse_Y() >= d_options_y &&
                    Get_Mouse_Y() <= d_options_y + d_options_h)) {
          Session.Messages.Add_Message(nullptr, 0,
                                       Text_String(TXT_ONLY_HOST_CAN_MODIFY),
                                       PCOLOR_BROWN, kTpfText, 1200);
          Sound_Effect(VOC_SYS_ERROR);
          display = std::max(display, REDRAW_MESSAGE);
          if (housebtn.IsDropped) {
            housebtn.Collapse();
            display = REDRAW_BACKGROUND;
          }
        }

        break;

#ifdef OLDWAY
      /*------------------------------------------------------------------
      House Buttons: set the player's desired House
      ------------------------------------------------------------------*/
      case ButtonKey(kButtonGdi):
        Session.House = HOUSE_GOOD;
        gdibtn.Turn_On();
        nodbtn.Turn_Off();
        port::SafeCopy(Session.Handle, namebuf);
        transmit = true;
        break;

      case ButtonKey(kButtonNod):
        Session.House = HOUSE_BAD;
        gdibtn.Turn_Off();
        nodbtn.Turn_On();
        port::SafeCopy(Session.Handle, namebuf);
        transmit = true;
        break;
#else   // OLDWAY
      case ButtonKey(kButtonHouse):
        Session.House = static_cast<HousesType>(housebtn.Current_Index() +
                                                static_cast<int>(HOUSE_USSR));
        port::SafeCopy(Session.Handle, namebuf);
        transmit = true;
        // display = REDRAW_BACKGROUND;
        break;
#endif  // OLDWAY

      /*------------------------------------------------------------------
      User edits the name value; retransmit
      ------------------------------------------------------------------*/
      case ButtonKey(kButtonName):
        if (housebtn.IsDropped) {
          housebtn.Collapse();
          display = REDRAW_BACKGROUND;
        }
        port::SafeCopy(Session.Handle, namebuf);
        transmit = true;
        changed = true;
        break;

      /*------------------------------------------------------------------
      CANCEL: send a SIGN_OFF, bail out with error code
      ------------------------------------------------------------------*/
      case KN_ESC:
      case ButtonKey(kButtonCancel):
        process = false;
        rc = 0;
        break;

      /*------------------------------------------------------------------
      Default: manage the inter-player messages
      ------------------------------------------------------------------*/
      default:
        if (Session.Messages.Manage()) {
          display = std::max(display, REDRAW_MESSAGE);
        }

        /*...............................................................
        Service keyboard input for any message being edited.
        ...............................................................*/
        i = Session.Messages.Input(input);

        /*...............................................................
        If 'Input' returned 1, it means refresh the message display; 2
        means redraw it. Rather than setting 'display', which would redraw
        all msgs, we only need to erase & redraw the edit box here (which
        also erases the cursor).
        ...............................................................*/
        if (i == 1 || i == 2) {
          Hide_Mouse();
          Draw_Box(d_send_x, d_send_y, d_send_w, d_send_h, BOXSTYLE_BOX, true);
          Session.Messages.Draw();
          Show_Mouse();
        } else if (i == 3 || i == 4) {
          /*...............................................................
          If 'input' returned 3, it means send the current message.
          ...............................................................*/
          base::FillBytes(base::ObjectBytes(SendPacket), 0, sizeof(SendPacket));
          SendPacket.Command = SERIAL_MESSAGE;
          port::SafeCopy(SendPacket.Name, namebuf);
          SendPacket.ID = static_cast<unsigned char>(Session.ColorIdx);
          if (i == 3) {
            port::SafeCopy(SendPacket.Message.Message,
                           Session.Messages.Get_Edit_Buf());
          } else {
            port::SafeCopy(SendPacket.Message.Message,
                           Session.Messages.Get_Overflow_Buf());
            Session.Messages.Clear_Overflow_Buf();
          }

          /*..................................................................
          Send the message
          ..................................................................*/
          NullModem.Send_Message(base::ObjectBytes(SendPacket),
                                 sizeof(SendPacket), 1);
          NullModem.Service();

          /*..................................................................
          Add the message to our own screen
          ..................................................................*/
          Session.Messages.Add_Message(
              SendPacket.Name, SendPacket.ID, SendPacket.Message.Message,
              Session.ColorIdx == PCOLOR_DIALOG_BLUE ? PCOLOR_REALLY_BLUE
                                                     : Session.ColorIdx,
              kTpfText, -1);
          Session.Messages.Add_Edit(Session.ColorIdx == PCOLOR_DIALOG_BLUE
                                        ? PCOLOR_REALLY_BLUE
                                        : Session.ColorIdx,
                                    kTpfText, nullptr, '_', d_message_w);
          display = std::max(display, REDRAW_MESSAGE);
        }
        break;
    }

    /*---------------------------------------------------------------------
    Detect editing of the name buffer, transmit new values to players
    ---------------------------------------------------------------------*/
    if (std::string_view(namebuf) != Session.Handle) {
      port::SafeCopy(Session.Handle, namebuf);
      transmit = true;
      changed = true;
    }

    /*---------------------------------------------------------------------
    If our Transmit flag is set, we need to send out a game option packet
    ---------------------------------------------------------------------*/
    if (transmit && TickCount.Value() - transmittime > PACKET_RETRANS_TIME) {
      base::FillBytes(base::ObjectBytes(SendPacket), 0, sizeof(SendPacket));
      SendPacket.Command = SERIAL_GAME_OPTIONS;
      port::SafeCopy(SendPacket.Name, namebuf);
      SendPacket.ScenarioInfo.CheatCheck = RuleINI.Get_Unique_ID();
      SendPacket.ScenarioInfo.MinVersion = VersionClass::Min_Version();
      SendPacket.ScenarioInfo.MaxVersion = VersionClass::Max_Version();
      SendPacket.ScenarioInfo.House = Session.House;
      SendPacket.ScenarioInfo.Color = Session.ColorIdx;
      SendPacket.ID = static_cast<unsigned char>(Session.ModemType);

      NullModem.Send_Message(base::ObjectBytes(SendPacket), sizeof(SendPacket),
                             1);

      transmittime = TickCount.Value();
      transmit = false;

      //..................................................................
      // Keep the player list up to date
      //..................................................................
      if (playerlist.Count()) {
#ifdef OLDWAY
        if (Session.House == HOUSE_GOOD) {
          sprintf(item, "%s\t%s", namebuf, Text_String(TXT_ALLIES));
        } else {
          sprintf(item, "%s\t%s", namebuf, Text_String(TXT_SOVIET));
        }
#else   // OLDWAY
        absl::SNPrintF(
            item, sizeof(item), "%s\t%s", namebuf,
            Text_String(
                HouseTypeClass::As_Reference(Session.House).Full_Name()));
#endif  // OLDWAY
        playerlist.Set_Item(0, item);
        playerlist.Colors.at(0) = &ColorRemaps.at(
            Session.ColorIdx == PCOLOR_DIALOG_BLUE ? PCOLOR_REALLY_BLUE
                                                   : Session.ColorIdx);
        playerlist.Flag_To_Redraw();
      }

      //..................................................................
      // Play a little sound effect
      //..................................................................
      Sound_Effect(VOC_OPTIONS_CHANGED);
    }

    //
    // send a timing packet if enough time has gone by.
    //
    if (TickCount.Value() - timingtime > PACKET_TIMING_TIMEOUT) {
      base::FillBytes(base::ObjectBytes(SendPacket), 0, sizeof(SendPacket));
      SendPacket.Command = SERIAL_TIMING;
      SendPacket.ScenarioInfo.ResponseTime = NullModem.Response_Time();
      SendPacket.ID = static_cast<unsigned char>(Session.ModemType);

      NullModem.Send_Message(base::ObjectBytes(SendPacket), sizeof(SendPacket),
                             0);
      timingtime = TickCount.Value();
    }

    /*---------------------------------------------------------------------
    Check for an incoming message
    ---------------------------------------------------------------------*/
    if (NullModem.Get_Message(base::ObjectBytes(ReceivePacket), &packetlen) >
        0) {
      lastmsgtime = TickCount.Value();

      msg_timeout = 600;

      // are we getting our own packets back??

      if (ReceivePacket.Command >= SERIAL_CONNECT &&
          ReceivePacket.Command < SERIAL_LAST_COMMAND &&
          ReceivePacket.Command != SERIAL_MESSAGE &&
          ReceivePacket.ID == static_cast<unsigned char>(Session.ModemType)) {
        WWMessageBox().Process(TXT_SYSTEM_NOT_RESPONDING);

        // to skip the other system not responding msg
        rc = 0;

        // say we did receive sign off to keep from sending one
        recsignedoff = 1;
        break;
      }

      const auto event_type = port::ReadUnaligned<EventClass::EventType>(
          base::ObjectBytes(ReceivePacket));
      if (event_type <= EventClass::FRAMEINFO) {
        if (TickCount.Value() - lastredrawtime > PACKET_REDRAW_TIME) {
          lastredrawtime = TickCount.Value();
          oppscorescreen = true;
          display = std::max(display, REDRAW_MESSAGE);
          parms_received = true;
        }
      } else {
        switch (ReceivePacket.Command) {
          /*..................................................................
          Other system signs off:  Give it time to receive my ACK, then show
          a message.
          ..................................................................*/
          case SERIAL_SIGN_OFF:
            starttime = TickCount.Value();
            while (TickCount.Value() - starttime < 60) {
              NullModem.Service();
            }
            WWMessageBox().Process(TXT_USER_SIGNED_OFF);

            // to skip the other system not responding msg
            lastmsgtime = TickCount.Value();

            process = false;
            rc = 0;
            recsignedoff = 1;
            break;

          /*..................................................................
          Game Options: Store all options; check my color & game version.
          ..................................................................*/
          case SERIAL_GAME_OPTIONS:
            oppscorescreen = false;
            display = std::max(display, REDRAW_MESSAGE);
            parms_received = true;

            port::SafeCopy(TheirName, ReceivePacket.Name);
            TheirColor = ReceivePacket.ScenarioInfo.Color;
            TheirHouse = ReceivePacket.ScenarioInfo.House;

            /*...............................................................
            Make sure I don't have the same color as the other guy.
            ...............................................................*/
            if (Session.ColorIdx == TheirColor) {
              // force transmitting of game options packet

              transmit = true;
              transmittime = 0;

              Session.ColorIdx = static_cast<PlayerColorType>(
                  static_cast<int>(TheirColor) + 1);
              if (static_cast<int>(Session.ColorIdx) >= 6) {
                Session.ColorIdx =
                    magic_enum::enum_values<PlayerColorType>().front();
              }
              name_edt.Set_Color(&ColorRemaps.at(
                  Session.ColorIdx == PCOLOR_DIALOG_BLUE ? PCOLOR_REALLY_BLUE
                                                         : Session.ColorIdx));
              name_edt.Flag_To_Redraw();
              display = std::max(display, REDRAW_COLORS);
              if (housebtn.IsDropped) {
                housebtn.Collapse();
                display = REDRAW_BACKGROUND;
              }
            }

            /*...............................................................
            Save scenario settings.
            ...............................................................*/
            Session.Options.Credits = ReceivePacket.ScenarioInfo.Credits;
            Session.Options.Bases = ReceivePacket.ScenarioInfo.IsBases;
            Session.Options.Tiberium = ReceivePacket.ScenarioInfo.IsTiberium;
            Session.Options.Goodies = ReceivePacket.ScenarioInfo.IsGoodies;
            Session.Options.AIPlayers = ReceivePacket.ScenarioInfo.AIPlayers;
            BuildLevel = ReceivePacket.ScenarioInfo.BuildLevel;
            Session.Options.UnitCount = ReceivePacket.ScenarioInfo.UnitCount;
            Seed = ReceivePacket.ScenarioInfo.Seed;
            Special = ReceivePacket.ScenarioInfo.Special;
            Options.GameSpeed = ReceivePacket.ScenarioInfo.GameSpeed;

            if (Session.Options.Tiberium) {
              Special.IsTGrowth = true;
              Rule.IsTGrowth = true;
              Special.IsTSpread = true;
              Rule.IsTSpread = true;
            } else {
              Special.IsTGrowth = false;
              Rule.IsTGrowth = false;
              Special.IsTSpread = false;
              Rule.IsTSpread = false;
            }

            //.........................................................
            // Adjust the gauges
            //.........................................................
            countgauge.Set_Maximum(
                base::At(SessionClass::CountMax, Session.Options.Bases) -
                base::At(SessionClass::CountMin, Session.Options.Bases));
            countgauge.Set_Value(
                Session.Options.UnitCount -
                base::At(SessionClass::CountMin, Session.Options.Bases));
            levelgauge.Set_Value(BuildLevel - 1);
            creditsgauge.Set_Value(Session.Options.Credits);
            aiplayersgauge.Set_Value(Session.Options.AIPlayers);

            //.........................................................
            // Update the options list box
            //.........................................................
            optionlist.Check_Item(0, Session.Options.Bases != 0);
            optionlist.Check_Item(1, Session.Options.Tiberium != 0);
            optionlist.Check_Item(2, Session.Options.Goodies != 0);
            optionlist.Check_Item(3, Special.IsCaptureTheFlag);
            optionlist.Check_Item(4, Special.IsShadowGrow);
            optionlist.Flag_To_Redraw();

            /*
            ** If the scenario name changed then we need to redraw the whole
            *lot.
            */
            if (std::string_view(Session.Options.ScenarioDescription) !=
                ReceivePacket.ScenarioInfo.Scenario) {
              display = std::max(display, REDRAW_BACKGROUND);
            }

            /*...............................................................
            Copy the information about the scenario that the host wants to
            play so ee can request this scenario from the host if we don't
            have it locally.
            ...............................................................*/
            port::SafeCopy(Session.Options.ScenarioDescription,
                           ReceivePacket.ScenarioInfo.Scenario);
            port::SafeCopy(Session.ScenarioFileName,
                           ReceivePacket.ScenarioInfo.ShortFileName);
            port::SafeCopy(Session.ScenarioDigest,
                           ReceivePacket.ScenarioInfo.FileDigest);
            Session.ScenarioIsOfficial =
                ReceivePacket.ScenarioInfo.OfficialScenario;
            Session.ScenarioFileLength = ReceivePacket.ScenarioInfo.FileLength;

            //.........................................................
            // "Clip" the other system's version range to our own
            // ........................................................
            version =
                VerNum.Clip_Version(ReceivePacket.ScenarioInfo.MinVersion,
                                    ReceivePacket.ScenarioInfo.MaxVersion);
            // ........................................................
            // If the greatest-common-version comes back 0, the other
            // system's range is too low for ours
            // ........................................................
            if (version == 0) {
              WWMessageBox().Process(TXT_DESTGAME_OUTDATED);

              // to skip the other system not responding msg
              lastmsgtime = TickCount.Value();

              process = false;
              rc = 0;
            } else if (version == 0xffffffff) {
              // ........................................................
              // If the greatest-common-version comes back 0xffffffff,
              // the other system's range is too high for ours
              // ........................................................
              WWMessageBox().Process(TXT_YOURGAME_OUTDATED);

              // to skip the other system not responding msg
              lastmsgtime = TickCount.Value();

              process = false;
              rc = 0;
            }
            // ........................................................
            // Otherwise, 'version' is the highest version we have in
            // common; look up the protocol that goes with this version.
            // ........................................................
            else {
              if (ReceivePacket.ScenarioInfo.CheatCheck !=
                  RuleINI.Get_Unique_ID()) {
                WWMessageBox().Process(TXT_MISMATCH);

                // to skip the other system not responding msg
                lastmsgtime = TickCount.Value();

                process = false;
                rc = 0;
              } else {
                Session.CommProtocol = VersionClass::Version_Protocol(version);
              }
            }

            /*.........................................................
            If this is the first game-options packet we've received,
            init the game & player lists, then transmit our options
            to him.
            .........................................................*/
            if (first) {
              //......................................................
              // Add a string to the game list, and two to the player
              // list
              //......................................................
              gamelist.Add_Item("");
              playerlist.Add_Item(
                  "", &ColorRemaps.at(Session.ColorIdx == PCOLOR_DIALOG_BLUE
                                          ? PCOLOR_REALLY_BLUE
                                          : Session.ColorIdx));
              playerlist.Add_Item(
                  "", &ColorRemaps.at(TheirColor == PCOLOR_DIALOG_BLUE
                                          ? PCOLOR_REALLY_BLUE
                                          : TheirColor));

              first = false;
              transmit = true;
              transmittime = 0;
            }

            //.........................................................
            // Ensure the game list & player list have the latest,
            // greatest copy of our names & colors.  Do this every time
            // we receive an options packet.
            //.........................................................
            Format_Runtime_Text(item, MPLAYER_NAME_MAX + 64,
                                Text_String(TXT_THATGUYS_GAME), TheirName);
            gamelist.Set_Item(0, item);
#ifdef OLDWAY
            if (Session.House == HOUSE_GOOD) {
              sprintf(item, "%s\t%s", namebuf, Text_String(TXT_ALLIES));
            } else {
              sprintf(item, "%s\t%s", namebuf, Text_String(TXT_SOVIET));
            }
#else  // OLDWAY
            absl::SNPrintF(
                item, sizeof(item), "%s\t%s", namebuf,
                Text_String(
                    HouseTypeClass::As_Reference(Session.House).Full_Name()));

#endif  // OLDWAY
            playerlist.Set_Item(0, item);
            playerlist.Colors.at(0) = &ColorRemaps.at(
                Session.ColorIdx == PCOLOR_DIALOG_BLUE ? PCOLOR_REALLY_BLUE
                                                       : Session.ColorIdx);

#ifdef OLDWAY
            if (TheirHouse == HOUSE_GOOD) {
              sprintf(item, "%s\t%s", TheirName, Text_String(TXT_ALLIES));
            } else {
              sprintf(item, "%s\t%s", TheirName, Text_String(TXT_SOVIET));
            }
#else   // OLDWAY
            absl::SNPrintF(
                item, sizeof(item), "%s\t%s", TheirName,
                Text_String(
                    HouseTypeClass::As_Reference(TheirHouse).Full_Name()));
#endif  // OLDWAY
            playerlist.Set_Item(1, item);
            playerlist.Colors.at(1) = &ColorRemaps.at(
                TheirColor == PCOLOR_DIALOG_BLUE ? PCOLOR_REALLY_BLUE
                                                 : TheirColor);

            gamelist.Flag_To_Redraw();
            playerlist.Flag_To_Redraw();

            //.........................................................
            // Play a little sound effect
            //.........................................................
            Sound_Effect(VOC_OPTIONS_CHANGED);

            break;

          /*..................................................................
          GO: Exit this routine with a success code.
          ..................................................................*/
          case SERIAL_LOADGAME:
            load_game = true;
            [[fallthrough]];
          case SERIAL_GO:

            ready_packet_was_sent = false;

            if (!load_game) {
              /*
              ** Special new kludge for counterstrike.
              **
              ** Find local scenario will fail to match a counterstrike mission
              ** unless the CS CD is in the drive. So....
              **
              ** If Counterstrike is installed and this is an official map and
              ** the file name matches a counterstrike map then tell the host
              ** that I have the scenario so he can continue while we make
              ** sure the local user has the Counterstrike CD in the drive.
              **
              */
              //	This is duplicated for Aftermath scenarios. ajw

              if (Session.ScenarioIsOfficial &&
                  ((Expansion_CS_Present() &&
                    IsMissionCounterstrike(Session.ScenarioFileName)) ||
                   (Expansion_AM_Present() &&
                    IsMissionAftermath(Session.ScenarioFileName)))) {
                GameFile check_file(Session.ScenarioFileName);
                if (!check_file.IsAvailable()) {
                  const int current_drive = SearchPaths::current_cd_drive();
                  const int index = Get_CD_Index(current_drive, 1 * 60);
                  bool needcd = false;
                  if (IsMissionCounterstrike(Session.ScenarioFileName) &&
                      (index != 2 && index != 3)) {
                    RequiredCD = 2;
                    needcd = true;
                  }

                  if (IsMissionAftermath(Session.ScenarioFileName) &&
                      (index != 3)) {
                    RequiredCD = 3;
                    needcd = true;
                  }

                  if (needcd) {
                    WWDebugString("RA95 - Counterstrike CD is not in drive\n");

                    /*
                    ** We should have the scenario but the wrong disk is in.
                    ** Tell the host that I am ready to go anyway.
                    */
                    base::FillBytes(base::ObjectBytes(SendPacket), 0,
                                    sizeof(SendPacket));
                    SendPacket.Command = SERIAL_READY_TO_GO;
                    NullModem.Send_Message(base::ObjectBytes(SendPacket),
                                           sizeof(SendPacket), 1);

                    starttime = TickCount.Value();
                    while (
                        (NullModem.Num_Send() && TickCount.Value() - starttime <
                                                     PACKET_SENDING_TIMEOUT) ||
                        TickCount.Value() - starttime < 60) {
                      NullModem.Service();
                    }
                    ready_packet_was_sent = true;

                    if (!Force_CD_Available(RequiredCD)) {
                      Emergency_Exit(EXIT_FAILURE);
                    }

                    /*
                    ** Update the internal list of scenarios to include the
                    *counterstrike
                    ** list.
                    */
                    Session.Read_Scenario_Descriptions();

                    /*
                    ** Make sure we dont time out because of the disk swap
                    */
                    lastmsgtime = TickCount.Value();
                  }
                }
              }

              /*
              ** If the scenario that the host wants to play doesnt exist
              *locally then we *	need to request that it is sent. If we
              *can identify the scenario locally then *	we need to fix up the
              *file name so we load the right one.
              */
              if (Find_Local_Scenario(
                      Session.Options.ScenarioDescription,
                      Session.ScenarioFileName, Session.ScenarioFileLength,
                      Session.ScenarioDigest, Session.ScenarioIsOfficial)) {
                /*
                ** We have the scenario. Tell the host that I am ready to go.
                */
                if (!ready_packet_was_sent) {
                  base::FillBytes(base::ObjectBytes(SendPacket), 0,
                                  sizeof(SendPacket));
                  SendPacket.Command = SERIAL_READY_TO_GO;
                  NullModem.Send_Message(base::ObjectBytes(SendPacket),
                                         sizeof(SendPacket), 1);
                  starttime = TickCount.Value();

                  while (
                      (NullModem.Num_Send() && TickCount.Value() - starttime <
                                                   PACKET_SENDING_TIMEOUT) ||
                      TickCount.Value() - starttime < 60) {
                    NullModem.Service();
                  }
                }
              } else {
                if (bSpecialAftermathScenario(
                        Session.Options.ScenarioDescription)) {
                  break;
                }
                if (!Get_Scenario_File_From_Host(
                        Session.ScenarioFileName,
                        sizeof(Session.ScenarioFileName), 0)) {
                  rc = 0;
                  break;
                }
                /*
                 ** Make sure we dont time-out because of the download
                 */
                lastmsgtime = TickCount.Value();
              }
            } else {
              /*
              ** Make sure we respond to the host in a load game
              */
              base::FillBytes(base::ObjectBytes(SendPacket), 0,
                              sizeof(SendPacket));
              SendPacket.Command = SERIAL_READY_TO_GO;
              NullModem.Send_Message(base::ObjectBytes(SendPacket),
                                     sizeof(SendPacket), 1);
              starttime = TickCount.Value();

              while ((NullModem.Num_Send() &&
                      TickCount.Value() - starttime < PACKET_SENDING_TIMEOUT) ||
                     TickCount.Value() - starttime < 60) {
                NullModem.Service();
              }
            }

            /*
            ** Fall through here...
            */
            port::SafeCopy(Scen.ScenarioName, Session.ScenarioFileName);
            //
            // calculated one way delay for a packet and overall delay
            // to execute a packet
            //
            if (Session.CommProtocol == COMM_PROTOCOL_MULTI_E_COMP) {
              Session.MaxAhead = static_cast<int>(std::max<int64_t>(
                  ((ReceivePacket.ScenarioInfo.ResponseTime / 8) +
                   (Session.FrameSendRate - 1)) /
                      Session.FrameSendRate * Session.FrameSendRate,
                  Session.FrameSendRate * 2));
            } else {
              Session.MaxAhead = std::max(
                  ReceivePacket.ScenarioInfo.ResponseTime / 8,
                  MODEM_MIN_MAX_AHEAD);
            }

            process = false;
            rc = 1;
            if (ReceivePacket.Command == SERIAL_LOADGAME) {
              load_game = true;
            }
            break;

          /*..................................................................
          Incoming message: add to our list
          ..................................................................*/
          case SERIAL_MESSAGE:
            oppscorescreen = false;

            Session.Messages.Add_Message(
                ReceivePacket.Name,
                static_cast<int>(
                    static_cast<PlayerColorType>(ReceivePacket.ID) ==
                            PCOLOR_DIALOG_BLUE
                        ? PCOLOR_REALLY_BLUE
                        : static_cast<PlayerColorType>(ReceivePacket.ID)),
                ReceivePacket.Message.Message,
                static_cast<PlayerColorType>(ReceivePacket.ID) ==
                        PCOLOR_DIALOG_BLUE
                    ? PCOLOR_REALLY_BLUE
                    : static_cast<PlayerColorType>(ReceivePacket.ID),
                kTpfText, -1);

            Sound_Effect(VOC_INCOMING_MESSAGE);
            display = std::max(display, REDRAW_MESSAGE);
            break;

          //
          // throw away timing packet
          //
          case SERIAL_TIMING:
            oppscorescreen = false;
            break;

          //
          // print msg waiting for opponent
          //
          case SERIAL_SCORE_SCREEN:
            oppscorescreen = true;
            display = std::max(display, REDRAW_MESSAGE);
            parms_received = true;
            break;

          case SerialCommandType::SERIAL_CONNECT:
          case SerialCommandType::SERIAL_LAST_COMMAND:
          case SerialCommandType::SERIAL_REQ_SCENARIO:
          case SerialCommandType::SERIAL_FILE_INFO:
          case SerialCommandType::SERIAL_FILE_CHUNK:
          case SerialCommandType::SERIAL_READY_TO_GO:
          case SerialCommandType::SERIAL_NO_SCENARIO:
          default:
            break;
        }
      }
    }

    // if we haven't received a msg for 10 seconds exit

    if (TickCount.Value() - lastmsgtime > msg_timeout) {
      WWMessageBox().Process(TXT_SYSTEM_NOT_RESPONDING);
      process = false;
      rc = 0;

      // say we did receive sign off to keep from sending one
      recsignedoff = 1;
    }

    /*---------------------------------------------------------------------
    Service the connection
    ---------------------------------------------------------------------*/
    NullModem.Service();
  }

  /*------------------------------------------------------------------------
  Prepare to load the scenario
  ------------------------------------------------------------------------*/
  if (rc) {
    Session.NumPlayers = 2;

    /*.....................................................................
    Add both players to the Players vector; the local system is always
    index 0.
    .....................................................................*/
    who = new NodeNameType;

    /* If the names of the players are the same then we MUST force them
     * to be unique. This is necessary to prevent a crash after loading
     * a modem save game.
     */
    if (std::string_view(TheirName) == namebuf) {
      if (std::string_view(TheirName).size() == MPLAYER_NAME_MAX - 1) {
        namebuf[MPLAYER_NAME_MAX - 1] = '\0';
      } else {
        port::SafeAppend(namebuf, "2");
      }
    }

    port::SafeCopy(who->Name, namebuf);
    who->Player.House = Session.House;
    who->Player.Color = Session.ColorIdx;
    who->Player.ProcessTime = -1;
    Session.Players.Add(who);

    who = new NodeNameType;
    port::SafeCopy(who->Name, TheirName);
    who->Player.House = TheirHouse;
    who->Player.Color = TheirColor;
    who->Player.ProcessTime = -1;
    Session.Players.Add(who);

    starttime = TickCount.Value();
    while ((NullModem.Num_Send() &&
            TickCount.Value() - starttime < PACKET_SENDING_TIMEOUT) ||
           TickCount.Value() - starttime < 60) {

      NullModem.Service();
    }

    // clear queue to keep from doing any resends
    NullModem.Init_Send_Queue();

  } else {
    if (!recsignedoff) {
      /*.....................................................................
      Broadcast my sign-off over my network
      .....................................................................*/
      base::FillBytes(base::ObjectBytes(SendPacket), 0, sizeof(SendPacket));
      SendPacket.Command = SERIAL_SIGN_OFF;
      SendPacket.ScenarioInfo.Color = Session.ColorIdx;  // use Color for ID
      SendPacket.ID = static_cast<unsigned char>(Session.ModemType);
      NullModem.Send_Message(base::ObjectBytes(SendPacket), sizeof(SendPacket),
                             1);

      starttime = TickCount.Value();
      while ((NullModem.Num_Send() &&
              TickCount.Value() - starttime < PACKET_CANCEL_TIMEOUT) ||
             TickCount.Value() - starttime < 60) {

        if ((NullModem.Get_Message(base::ObjectBytes(ReceivePacket),
                                   &packetlen) > 0) &&
            (ReceivePacket.Command == SERIAL_SIGN_OFF &&
             ReceivePacket.ID == static_cast<unsigned char>(Session.ModemType)))
        // are we getting our own packets back??

        {
          // exit while
          break;
        }

        NullModem.Service();
      }
    }

    Shutdown_Modem();
  }

  /*------------------------------------------------------------------------
  Clean up the list boxes
  ------------------------------------------------------------------------*/
  //	while (optionlist.Count()>0) {
  //		item = (char *)optionlist.Get_Item(0);
  //		delete [] item;
  //		optionlist.Remove_Item(item);
  //	}
  gamelist.Clear();
  playerlist.Clear();

  /*------------------------------------------------------------------------
  Remove the chat edit box
  ------------------------------------------------------------------------*/
  Session.Messages.Remove_Edit();

  /*------------------------------------------------------------------------
  Restore screen
  ------------------------------------------------------------------------*/
  Hide_Mouse();
  Load_Title_Page(true);
  Show_Mouse();

  /*------------------------------------------------------------------------
  Save any changes made to our options
  ------------------------------------------------------------------------*/
  if (changed) {
    Session.Write_MultiPlayer_Settings();
  }

  if (load_game) {
    if (!Load_Game(-1)) {
      WWMessageBox().Process(TXT_ERROR_LOADING_GAME);
      rc = 0;
    }
    Frame++;
  }

  return rc;

} /* end of Com_Show_Scenario_Dialog */

/***************************************************************************
 * Phone_Dialog -- Lets user edit phone directory & dial                   *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		true = dial the current phone book entry, false = cancel.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		Serial options must have been read from CC.INI.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   04/29/1995 BRR : Created.                                             *
 *=========================================================================*/
static int Phone_Dialog() {
  /*........................................................................
  Dialog & button dimensions
  ........................................................................*/
  const int d_dialog_w = 560;                             // dialog width
  const int d_dialog_h = 320;                             // dialog height
  const int d_dialog_x = (640 - d_dialog_w) / 2;          // dialog x-coord
  const int d_dialog_y = (400 - d_dialog_h) / 2;          // dialog y-coord
  const int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);  // center x-coord

  const int d_txt6_h = 14;  // ht of 6-pt text
  const int d_margin = 14;  // margin width/height

  const int d_phonelist_w = 496;
  const int d_phonelist_h = 174;
  const int d_phonelist_x = d_dialog_cx - (d_phonelist_w / 2);
  const int d_phonelist_y = d_dialog_y + d_margin + d_txt6_h + 22;

  const int d_add_w = 90;
  const int d_add_h = 18;
  const int d_add_x = d_dialog_cx - (d_add_w / 2) - d_margin - d_add_w;
  const int d_add_y = d_phonelist_y + d_phonelist_h + d_margin;

  const int d_edit_w = 90;
  const int d_edit_h = 18;
  const int d_edit_x = d_dialog_cx - (d_edit_w / 2);
  const int d_edit_y = d_phonelist_y + d_phonelist_h + d_margin;

  const int d_delete_w = 90;
  const int d_delete_h = 18;
  const int d_delete_x = d_dialog_cx + (d_delete_w / 2) + d_margin;
  const int d_delete_y = d_phonelist_y + d_phonelist_h + d_margin;

  const int d_numedit_w = ((PhoneEntryClass::PHONE_MAX_NUM - 1) * 12) + 6;
  const int d_numedit_h = 18;
  const int d_numedit_x = d_dialog_cx - (d_numedit_w / 2);
  const int d_numedit_y = d_add_y + d_add_h + d_margin;

  const int d_dial_w = 90;
  const int d_dial_h = 18;
  const int d_dial_x = d_dialog_cx - (d_numedit_w / 2) - d_margin - d_dial_w;
  const int d_dial_y = d_add_y + d_add_h + d_margin;

  const int d_cancel_w = 90;
  const int d_cancel_h = 18;
  const int d_cancel_x = d_dialog_cx + (d_numedit_w / 2) + d_margin;
  const int d_cancel_y = d_add_y + d_add_h + d_margin;

  /*........................................................................
  Button Enumerations
  ........................................................................*/
  constexpr int kButtonPhonelist = 100;
  constexpr int kButtonAdd = 101;
  constexpr int kButtonEdit = 102;
  constexpr int kButtonDelete = 103;
  constexpr int kButtonDial = 104;
  constexpr int kButtonCancel = 105;
  constexpr int kButtonNumedit = 106;

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
  RedrawType display = REDRAW_ALL;  // redraw level
  bool process = true;              // process while true

  char phone_num[PhoneEntryClass::PHONE_MAX_NUM] = {
      0};  // buffer for editing phone #
  int rc = 0;
  const int tabs[] = {123 * 2, 414};  // tabs for list box
  PhoneEntryClass* p_entry =
      nullptr;               // for creating / editing phonebook entries
  bool changed = false;      // 1 = save changes to INI file
  bool firsttime = false;

  /*........................................................................
  Buttons
  ........................................................................*/

  ListClass phonelist(kButtonPhonelist, d_phonelist_x, d_phonelist_y,
                      d_phonelist_w, d_phonelist_h, kTpfText,
                      MixArchive::RetrieveData("BTN-UP.SHP"),
                      MixArchive::RetrieveData("BTN-DN.SHP"));
  TextButtonClass addbtn(kButtonAdd, TXT_ADD, kTpfButton, d_add_x, d_add_y,
                         d_add_w, d_add_h);
  TextButtonClass editbtn(kButtonEdit, TXT_EDIT, kTpfButton, d_edit_x, d_edit_y,
                          d_edit_w, d_edit_h);
  TextButtonClass deletebtn(kButtonDelete, TXT_DELETE_BUTTON, kTpfButton,
                            d_delete_x, d_delete_y, d_delete_w, d_delete_h);
  TextButtonClass dialbtn(kButtonDial, TXT_DIAL, kTpfButton, d_dial_x, d_dial_y,
                          d_dial_w, d_dial_h);
  TextButtonClass cancelbtn(kButtonCancel, TXT_CANCEL, kTpfButton, d_cancel_x,
                            d_cancel_y, d_cancel_w, d_cancel_h);
  EditClass numedit(kButtonNumedit, phone_num, PhoneEntryClass::PHONE_MAX_NUM,
                    kTpfText, d_numedit_x, d_numedit_y, d_numedit_w,
                    d_numedit_h, EditClass::kAlphanumeric);

  /*
  ------------------------- Build the button list --------------------------
  */
  GadgetClass* commands = &phonelist;  // button list
  addbtn.Add_Tail(*commands);
  editbtn.Add_Tail(*commands);
  deletebtn.Add_Tail(*commands);
  dialbtn.Add_Tail(*commands);
  cancelbtn.Add_Tail(*commands);
  numedit.Add_Tail(*commands);
  dialbtn.Turn_On();

  /*
  ----------------------------- Various Inits ------------------------------
  */
  /*........................................................................
  Fill in the phone directory list box
  ........................................................................*/
  phonelist.Set_Tabs(tabs);
  Build_Phone_Listbox(&phonelist, &numedit, phone_num);

  if (Session.CurPhoneIdx == -1) {
    firsttime = true;
  }

  /*
  ---------------------------- Processing loop -----------------------------
  */
  while (process) {
    /*
    ........................ Invoke game callback .........................
    */
    ServiceRealTime();

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
    ...................... Refresh display if needed ......................
    */
    if (display != REDRAW_NONE) {
      Hide_Mouse();
      /*
      .................. Redraw backgound & dialog box ...................
      */
      if (display >= REDRAW_BACKGROUND) {
        Load_Title_Page(true);
        CCPalette.Set();

        Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);

        // init font variables

        Fancy_Text_Print(TXT_NONE, 0, 0, nullptr, kTBlack,
                         TPF_CENTER | kTpfText);

        /*...............................................................
        Dialog & Field labels
        ...............................................................*/
        Draw_Caption(TXT_PHONE_LIST, d_dialog_x, d_dialog_y, d_dialog_w);
        phonelist.Draw_Me(true);
      }
      /*
      .......................... Redraw buttons ..........................
      */
      if (display >= REDRAW_BUTTONS) {
        addbtn.Flag_To_Redraw();
        editbtn.Flag_To_Redraw();
        deletebtn.Flag_To_Redraw();
        dialbtn.Flag_To_Redraw();
        cancelbtn.Flag_To_Redraw();
        numedit.Flag_To_Redraw();
      }
      Show_Mouse();
      display = REDRAW_NONE;
    }

    /*
    ........................... Get user input ............................
    */
    KeyNumType input = commands->Input();

    if (firsttime) {
      numedit.Set_Focus();
      numedit.Flag_To_Redraw();
      input = commands->Input();
      firsttime = false;
    }

    /*
    ---------------------------- Process input ----------------------------
    */
    switch (static_cast<int>(input)) {
      /*------------------------------------------------------------------
      New phone listing selected.
      ------------------------------------------------------------------*/
      case ButtonKey(kButtonPhonelist):
        /*...............................................................
        Detect a change in the selected item; update CurPhoneIdx, and
        the edit box buffer.
        ...............................................................*/
        if ((Session.CurPhoneIdx != -1) &&
            (phonelist.Current_Index() != Session.CurPhoneIdx)) {
          Session.CurPhoneIdx = phonelist.Current_Index();
          port::SafeCopy(phone_num,
                         Session.PhoneBook.at(Session.CurPhoneIdx)->Number);
          numedit.Set_Text(phone_num, PhoneEntryClass::PHONE_MAX_NUM);
          changed = true;
        }

        break;

      /*------------------------------------------------------------------
      Add a new entry
      ------------------------------------------------------------------*/
      case ButtonKey(kButtonAdd):

        /*...............................................................
        Allocate a new phone book entry
        ...............................................................*/
        p_entry = new PhoneEntryClass();
        p_entry->Name[0] = 0;
        p_entry->Number[0] = 0;
        p_entry->Settings.Port = 0;
        p_entry->Settings.IRQ = -1;
        p_entry->Settings.Baud = -1;
        p_entry->Settings.DialMethod = DIAL_TOUCH_TONE;
        p_entry->Settings.InitStringIndex = 0;
        p_entry->Settings.CallWaitStringIndex = kCallWaitCustom;
        p_entry->Settings.CallWaitString[0] = 0;

        /*...............................................................
        Invoke the entry editor; if user clicks Save, add the new entry
        to the list, and rebuild the list box.
        ...............................................................*/
        if (Edit_Phone_Dialog(p_entry)) {
          Session.PhoneBook.Add(p_entry);
          Build_Phone_Listbox(&phonelist, &numedit, phone_num);
          /*............................................................
          Set the current listbox index to the newly-added item.
          ............................................................*/
          for (int i = 0; i < Session.PhoneBook.Count(); i++) {
            if (p_entry == Session.PhoneBook.at(i)) {
              Session.CurPhoneIdx = i;
              port::SafeCopy(phone_num,
                             Session.PhoneBook.at(Session.CurPhoneIdx)->Number);
              numedit.Set_Text(phone_num, PhoneEntryClass::PHONE_MAX_NUM);
              phonelist.Set_Selected_Index(Session.CurPhoneIdx);
            }
          }
          changed = true;
        } else {
          /*...............................................................
          If the user clicked Cancel, delete the entry & keep looping.
          ...............................................................*/
          delete p_entry;
        }
        display = REDRAW_ALL;
        break;

      /*------------------------------------------------------------------
      Edit the current entry
      ------------------------------------------------------------------*/
      case ButtonKey(kButtonEdit):

        /*...............................................................
        Do nothing if no entry is selected.
        ...............................................................*/
        if (Session.CurPhoneIdx == -1) {
          break;
        }

        /*...............................................................
        Allocate a new entry & copy the currently-selected entry into it
        ...............................................................*/
        p_entry = new PhoneEntryClass();
        *p_entry = *Session.PhoneBook.at(Session.CurPhoneIdx);

        /*...............................................................
        Pass the new entry to the entry editor; if the user selects OK,
        copy the data back into our phone book.  Rebuild the list so
        the changes show up in the list box.
        ...............................................................*/
        if (Edit_Phone_Dialog(p_entry)) {
          *Session.PhoneBook.at(Session.CurPhoneIdx) = *p_entry;
          Build_Phone_Listbox(&phonelist, &numedit, phone_num);
          /*............................................................
          Set the current listbox index to the newly-added item.
          ............................................................*/
          for (int i = 0; i < Session.PhoneBook.Count(); i++) {
            if (Session.PhoneBook.at(Session.CurPhoneIdx) ==
                Session.PhoneBook.at(i)) {
              Session.CurPhoneIdx = i;
              port::SafeCopy(phone_num,
                             Session.PhoneBook.at(Session.CurPhoneIdx)->Number);
              numedit.Set_Text(phone_num, PhoneEntryClass::PHONE_MAX_NUM);
              phonelist.Set_Selected_Index(Session.CurPhoneIdx);
            }
          }
          changed = true;
        }
        delete p_entry;
        display = REDRAW_ALL;
        break;

      /*------------------------------------------------------------------
      Delete the current entry
      ------------------------------------------------------------------*/
      case ButtonKey(kButtonDelete):

        /*...............................................................
        Do nothing if no entry is selected.
        ...............................................................*/
        if (Session.CurPhoneIdx == -1) {
          break;
        }

        /*...............................................................
        Delete the current item & rebuild the phone listbox
        ...............................................................*/
        Session.PhoneBook.Delete(Session.CurPhoneIdx);
        Build_Phone_Listbox(&phonelist, &numedit, phone_num);

        if (Session.CurPhoneIdx == -1) {
          *phone_num = 0;
          numedit.Set_Text(phone_num, PhoneEntryClass::PHONE_MAX_NUM);
        }
        changed = true;
        break;

      /*------------------------------------------------------------------
      Dial the current number
      ------------------------------------------------------------------*/
      case KN_RETURN:
        dialbtn.IsPressed = true;
        dialbtn.Draw_Me(true);
        [[fallthrough]];

      case ButtonKey(kButtonDial):

        /*...............................................................
        If no item is selected, just dial the number in the phone #
        edit box:
        - Create a new phone entry
        - Copy the phone number into it
        - Set settings to defaults
        ...............................................................*/
        if (Session.CurPhoneIdx == -1 ||
            std::string_view(
                Session.PhoneBook.at(Session.CurPhoneIdx)->Number) !=
                phone_num) {
          if (std::string_view(phone_num).empty()) {  // do not dial
            dialbtn.IsPressed = true;
            dialbtn.Flag_To_Redraw();
            break;
          }

          p_entry = new PhoneEntryClass();
          port::SafeCopy(p_entry->Name, "NONAME");
          port::SafeCopy(p_entry->Number, phone_num);
          p_entry->Settings.Port = 0;
          p_entry->Settings.IRQ = -1;
          p_entry->Settings.Baud = -1;
          p_entry->Settings.DialMethod = DIAL_TOUCH_TONE;
          p_entry->Settings.InitStringIndex = 0;
          p_entry->Settings.CallWaitStringIndex = kCallWaitCustom;
          p_entry->Settings.CallWaitString[0] = 0;

          Session.PhoneBook.Add(p_entry);
          Build_Phone_Listbox(&phonelist, &numedit, phone_num);
          /*............................................................
          Set the current listbox index to the newly-added item.
          ............................................................*/
          for (int i = 0; i < Session.PhoneBook.Count(); i++) {
            if (p_entry == Session.PhoneBook.at(i)) {
              Session.CurPhoneIdx = i;
            }
          }
          changed = true;
        }

        process = false;
        rc = 1;
        break;

      /*------------------------------------------------------------------
      CANCEL: bail out
      ------------------------------------------------------------------*/
      case KN_ESC:
      case ButtonKey(kButtonCancel):
        process = false;
        rc = 0;
        break;
      default:
        break;
    }

  } /* end of while */

  /*------------------------------------------------------------------------
  Save any changes we've made to the phone list or settings
  ------------------------------------------------------------------------*/
  if (changed) {
    Session.Write_MultiPlayer_Settings();
  }

  /*------------------------------------------------------------------------
  Clear the list box
  ------------------------------------------------------------------------*/
  phonelist.Clear();

  return rc;

} /* end of Phone_Dialog */

/***************************************************************************
 * Build_Phone_Listbox -- [re]builds the phone entry listbox               *
 *                                                                         *
 * This routine rebuilds the phone list box from scratch; it also updates
 ** the contents of the phone # edit field.
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		list		ptr to list box
 ** edit		ptr to edit box
 ** buf		ptr to buffer for phone #
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   04/29/1995 BRR : Created.                                             *
 *=========================================================================*/
static void Build_Phone_Listbox(ListClass* list, EditClass* edit,
                                std::span<char> buf) {
  char item[80];
  char phonename[21];
  char phonenum[15];

  /*........................................................................
  Clear the list
  ........................................................................*/
  list->Clear();

  /*
  ** Now sort the phone list by name then number
  */
  if (Session.PhoneBook.Count() > 0) {
    std::vector<PhoneEntryClass*> sorted;
    sorted.reserve(base::ToSize(Session.PhoneBook.Count()));
    for (int i = 0; i < Session.PhoneBook.Count(); ++i) {
      sorted.push_back(Session.PhoneBook.at(i));
    }
    std::ranges::sort(
        sorted, [](const PhoneEntryClass* left, const PhoneEntryClass* right) {
          int result = std::string_view(left->Name).compare(right->Name);
          if (result == 0) {
            // Same name, so order by the phone number instead.
            result = std::string_view(left->Number).compare(right->Number);
          }
          return result < 0;
        });
    for (int i = 0; i < Session.PhoneBook.Count(); ++i) {
      Session.PhoneBook.at(i) = sorted.at(base::ToSize(i));
    }
  }

  /*........................................................................
  Build the list
  ........................................................................*/
  for (int i = 0; i < Session.PhoneBook.Count(); i++) {
    if (std::string_view(Session.PhoneBook.at(i)->Name).empty()) {
      port::SafeCopy(phonename, " ");
    } else {
      port::SafeCopy(phonename, Session.PhoneBook.at(i)->Name);
    }

    if (std::string_view(Session.PhoneBook.at(i)->Number).empty()) {
      port::SafeCopy(phonenum, " ");
    } else {
      if (std::string_view(Session.PhoneBook.at(i)->Number).size() < 14) {
        port::SafeCopy(phonenum, Session.PhoneBook.at(i)->Number);
      } else {
        port::SafeCopy(phonenum, Session.PhoneBook.at(i)->Number);
        port::SafeAppend(phonenum, "...");
      }
    }

    if (Session.PhoneBook.at(i)->Settings.Baud != -1) {
      absl::SNPrintF(item, sizeof(item), "%s\t%s\t%d", phonename, phonenum,
                     Session.PhoneBook.at(i)->Settings.Baud);
    } else {
      absl::SNPrintF(item, sizeof(item), "%s\t%s\t[%s]", phonename, phonenum,
                     Text_String(TXT_DEFAULT));
    }
    list->Add_Item(item);
  }
  list->Flag_To_Redraw();

  /*........................................................................
  Init the current phone book index
  ........................................................................*/
  if (list->Count() == 0 || Session.CurPhoneIdx < -1) {
    Session.CurPhoneIdx = -1;
  } else {
    if (Session.CurPhoneIdx >= list->Count()) {
      Session.CurPhoneIdx = 0;
    }
  }

  /*........................................................................
  Fill in phone number edit buffer
  ........................................................................*/
  if (Session.CurPhoneIdx > -1) {
    port::SafeCopy(std::span(buf).first(PhoneEntryClass::PHONE_MAX_NUM),
                   Session.PhoneBook.at(Session.CurPhoneIdx)->Number);
    edit->Set_Text(buf, PhoneEntryClass::PHONE_MAX_NUM);
    list->Set_Selected_Index(Session.CurPhoneIdx);
  }
}

/***************************************************************************
 * Edit_Phone_Dialog -- lets user edit a phone book entry                  *
 *                                                                         *
 * INPUT:                                                                  *
 *		phone		entry to edit
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		true = OK, false = cancel
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   04/29/1995 BRR : Created.                                             *
 *=========================================================================*/
static int Edit_Phone_Dialog(PhoneEntryClass* phone) {
  /*........................................................................
  Dialog & button dimensions
  ........................................................................*/
  const int d_dialog_w = 460;                             // dialog width
  const int d_dialog_h = 220;                             // dialog height
  const int d_dialog_x = (640 - d_dialog_w) / 2;          // dialog x-coord
  const int d_dialog_y = (272 - d_dialog_h) / 2;          // dialog y-coord
  const int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);  // center x-coord

  const int d_margin = 14;  // margin width/height

  const int d_name_w = ((PhoneEntryClass::PHONE_MAX_NAME - 1) * 12) + 6;
  const int d_name_h = 18;
  const int d_name_x = d_dialog_x + ((d_dialog_w - d_name_w) * 3 / 4) - 10;
  const int d_name_y = d_dialog_y + 50;

  const int d_number_w = ((PhoneEntryClass::PHONE_MAX_NUM - 1) * 12) + 6;
  const int d_number_h = 18;
  const int d_number_x = d_dialog_x + ((d_dialog_w - d_number_w) * 3 / 4) - 10;
  const int d_number_y = d_name_y + d_name_h + d_margin;

  const int d_default_w = 260;
  const int d_default_h = 18;
  const int d_default_x = d_dialog_cx - (d_default_w / 2);
  const int d_default_y = d_number_y + d_number_h + d_margin;

  const int d_custom_w = 260;
  const int d_custom_h = 18;
  const int d_custom_x = d_dialog_cx - (d_default_w / 2);
  const int d_custom_y = d_default_y + d_default_h + d_margin;

  const int d_save_w = 110;
  const int d_save_h = 18;
  const int d_save_x = d_dialog_cx - d_margin - d_save_w;
  const int d_save_y = d_dialog_y + d_dialog_h - d_margin - d_save_h - 10;

  const int d_cancel_w = 110;
  const int d_cancel_h = 18;
  const int d_cancel_x = d_dialog_cx + d_margin;
  const int d_cancel_y = d_dialog_y + d_dialog_h - d_margin - d_cancel_h - 10;

  /*........................................................................
  Button Enumerations
  ........................................................................*/
  constexpr int kButtonName = 100;
  constexpr int kButtonNumber = 101;
  constexpr int kButtonDefault = 102;
  constexpr int kButtonCustom = 103;
  constexpr int kButtonSave = 104;
  constexpr int kButtonCancel = 105;

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
  RedrawType display = REDRAW_ALL;  // redraw level

  char namebuf[PhoneEntryClass::PHONE_MAX_NAME] = {
      0};  // buffer for editing name
  char numbuf[PhoneEntryClass::PHONE_MAX_NUM] = {
      0};  // buffer for editing phone #
  int rc = 0;
  SerialSettingsType settings{};
  bool custom = false;
  bool firsttime = true;
  RemapControlType* scheme = GadgetClass::Get_Color_Scheme();

  /*........................................................................
  Buttons
  ........................................................................*/

  EditClass nameedit(kButtonName, namebuf, PhoneEntryClass::PHONE_MAX_NAME,
                     kTpfText, d_name_x, d_name_y, d_name_w, d_name_h,
                     EditClass::kAlphanumeric);
  EditClass numedit(kButtonNumber, numbuf, PhoneEntryClass::PHONE_MAX_NUM,
                    kTpfText, d_number_x, d_number_y, d_number_w, d_number_h,
                    EditClass::kAlphanumeric);
  TextButtonClass defaultbtn(kButtonDefault, TXT_DEFAULT_SETTINGS, kTpfButton,
                             d_default_x, d_default_y, d_default_w,
                             d_default_h);
  TextButtonClass custombtn(kButtonCustom, TXT_CUSTOM_SETTINGS, kTpfButton,
                            d_custom_x, d_custom_y, d_custom_w, d_custom_h);
  TextButtonClass savebtn(kButtonSave, TXT_SAVE_BUTTON, kTpfButton, d_save_x,
                          d_save_y, d_save_w, d_save_h);
  TextButtonClass cancelbtn(kButtonCancel, TXT_CANCEL, kTpfButton, d_cancel_x,
                            d_cancel_y, d_cancel_w, d_cancel_h);

  /*
  ------------------------- Build the button list --------------------------
  */
  GadgetClass* commands = &nameedit;  // button list
  numedit.Add_Tail(*commands);
  defaultbtn.Add_Tail(*commands);
  custombtn.Add_Tail(*commands);
  savebtn.Add_Tail(*commands);
  cancelbtn.Add_Tail(*commands);

  /*
  ----------------------------- Various Inits ------------------------------
  */
  /*........................................................................
  Init the settings; if the phone entry is set to use defaults, init our
  settings to sensible values (in case we invoke the setting editor);
  otherwise, copy the entry's settings.
  ........................................................................*/
  if (phone->Settings.Port == 0 || phone->Settings.IRQ == -1 ||
      phone->Settings.Baud == -1) {
    settings = Session.SerialDefaults;
    defaultbtn.Turn_On();
    custom = false;
  } else {
    settings = phone->Settings;
    custombtn.Turn_On();
    custom = true;
  }

  port::SafeCopy(namebuf, phone->Name);
  nameedit.Set_Text(namebuf, PhoneEntryClass::PHONE_MAX_NAME);

  port::SafeCopy(numbuf, phone->Number);
  numedit.Set_Text(numbuf, PhoneEntryClass::PHONE_MAX_NUM);

  /*
  ---------------------------- Processing loop -----------------------------
  */
  bool process = true;  // process while true
  while (process) {
    /*
    ........................ Invoke game callback .........................
    */
    ServiceRealTime();

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
    ...................... Refresh display if needed ......................
    */
    if (display != REDRAW_NONE) {
      Hide_Mouse();
      /*
      .................. Redraw backgound & dialog box ...................
      */
      if (display >= REDRAW_BACKGROUND) {
        Load_Title_Page(true);
        CCPalette.Set();

        Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);
        Draw_Caption(TXT_PHONE_LISTING, d_dialog_x, d_dialog_y, d_dialog_w);

        /*...............................................................
        Dialog & Field labels
        ...............................................................*/
        Fancy_Text_Print(TXT_NAME_COLON, d_name_x - 10, d_name_y + 2, scheme,
                         kTBlack, TPF_RIGHT | kTpfText);
        Fancy_Text_Print(TXT_NUMBER_COLON, d_number_x - 10, d_number_y + 2,
                         scheme, kTBlack, TPF_RIGHT | kTpfText);
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
    KeyNumType input = commands->Input();

    if (firsttime) {
      nameedit.Set_Focus();
      nameedit.Flag_To_Redraw();
      input = commands->Input();
      firsttime = false;
    }

    /*
    ---------------------------- Process input ----------------------------
    */
    switch (static_cast<int>(input)) {
      case ButtonKey(kButtonName):
        numedit.Set_Focus();
        numedit.Flag_To_Redraw();
        break;

        //			case (kButtonNumber | KN_BUTTON):
        //				nameedit.Clear_Focus();
        //				nameedit.Flag_To_Redraw();
        //				break;

      /*------------------------------------------------------------------
      Use Default Serial Settings
      ------------------------------------------------------------------*/
      case ButtonKey(kButtonDefault):
        custombtn.Turn_Off();
        defaultbtn.Turn_On();
        custom = false;
        break;

      /*------------------------------------------------------------------
      Use Custom Serial Settings
      ------------------------------------------------------------------*/
      case ButtonKey(kButtonCustom):
        if (Com_Settings_Dialog(&settings)) {
          custombtn.Turn_On();
          defaultbtn.Turn_Off();
        }
        custom = true;
        display = REDRAW_ALL;
        break;

      /*------------------------------------------------------------------
      CANCEL: bail out
      ------------------------------------------------------------------*/
      case KN_ESC:
      case ButtonKey(kButtonCancel):
        process = false;
        rc = 0;
        break;

      /*------------------------------------------------------------------
      Save: save changes
      ------------------------------------------------------------------*/
      case KN_RETURN:
      case ButtonKey(kButtonSave):
        process = false;
        rc = 1;
        break;
      default:
        break;
    }

  } /* end of while */

  /*------------------------------------------------------------------------
  If 'Save', save all current settings
  ------------------------------------------------------------------------*/
  if (rc) {
    port::SafeCopy(phone->Name, absl::AsciiStrToUpper(namebuf));

    // if nothing was entered then make if NONAME

    if (!phone->Name[0]) {
      port::SafeCopy(phone->Name, "NONAME");
    }

    port::SafeCopy(phone->Number, absl::AsciiStrToUpper(numbuf));

    if (custom) {
      phone->Settings = settings;
    } else {
      phone->Settings.Port = 0;
      phone->Settings.IRQ = -1;
      phone->Settings.Baud = -1;
      phone->Settings.DialMethod = DIAL_TOUCH_TONE;
      phone->Settings.InitStringIndex = 0;
      phone->Settings.CallWaitStringIndex = kCallWaitCustom;
      phone->Settings.CallWaitString[0] = 0;
    }
  }

  return rc;

} /* end of Edit_Phone_Dialog */

static bool Dial_Modem(SerialSettingsType* settings, bool reconnect) {
  bool connected = false;

  /*
  **	Turn modem servicing off in the callback routine.
  */
  Session.ModemService = false;

  // save for later to reconnect

  DialSettings = settings;

  const auto carrier = NullModemClass::Get_Modem_Status();
  if (reconnect) {
    if (carrier & kCdSet) {
      connected = true;
      Session.ModemService = true;
      return connected;
    }
  } else if (carrier & kCdSet) {
    NullModem.Hangup_Modem();
    Session.ModemService = false;
  }

  NullModemClass::Setup_Modem_Echo(Modem_Echo);

  int modemstatus = NullModem.Detect_Modem(settings, reconnect);
  if (!modemstatus) {
    NullModemClass::Remove_Modem_Echo();
    NullModemClass::Print_EchoBuf();
    NullModem.Reset_EchoBuf();
    /*
    ** If our first attempt to detect the modem failed, and we're at
    ** 14400 or 28800, bump up to the next baud rate & try again.
    */
    switch (settings->Baud) {
      case 14400:
        settings->Baud = 19200;
        Shutdown_Modem();
        Init_Null_Modem(settings);
        NullModemClass::Setup_Modem_Echo(Modem_Echo);
        modemstatus = NullModem.Detect_Modem(settings, reconnect);
        if (!modemstatus) {
          NullModemClass::Remove_Modem_Echo();
          NullModemClass::Print_EchoBuf();
          NullModem.Reset_EchoBuf();
          WWMessageBox().Process(TXT_UNABLE_FIND_MODEM);
          Session.ModemService = true;
          return connected;
        }
        break;

      case 28800:
        settings->Baud = 38400;
        Shutdown_Modem();
        Init_Null_Modem(settings);
        NullModemClass::Setup_Modem_Echo(Modem_Echo);
        modemstatus = NullModem.Detect_Modem(settings, reconnect);
        if (!modemstatus) {
          NullModemClass::Remove_Modem_Echo();
          NullModemClass::Print_EchoBuf();
          NullModem.Reset_EchoBuf();
          WWMessageBox().Process(TXT_UNABLE_FIND_MODEM);
          Session.ModemService = true;
          return connected;
        }
        break;

      default:
        WWMessageBox().Process(TXT_UNABLE_FIND_MODEM);
        Session.ModemService = true;
        return connected;
    }

  } else if (modemstatus == -1) {
    NullModemClass::Remove_Modem_Echo();
    NullModemClass::Print_EchoBuf();
    NullModem.Reset_EchoBuf();
    WWMessageBox().Process(TXT_ERROR_IN_INITSTRING);
    //		WWMessageBox().Process( "Error in the InitString." );
    Session.ModemService = true;
    return connected;
  }


  /*
  ** Completely disable audio. This is required for MWave devices like those
  ** found in the IBM Aptiva.
  */
  ThemeType old_theme = THEME_NONE;
  if (SoundOn) {
    old_theme = Theme.What_Is_Playing();
    Theme.Stop();
    CountDownTimerClass wait;
    ServiceRealTime();
    wait.Set(60, true);
    while (wait.Time()) {
      ServiceRealTime();
    }
    CloseAudio();
    ServiceRealTime();
    wait.Set(60, true);
    while (wait.Time()) {
      ServiceRealTime();
    }
    SoundOn = false;
  }

  const DialStatusType dialstatus =
      NullModem.Dial_Modem(DialString.c_str(), settings->DialMethod, reconnect);

  if (reconnect) {
    /*
    --------------------------- Redraw the display ---------------------------
    */
    HidPage.Clear();
    Map.Flag_To_Redraw(true);
    Map.Render();
  }

  switch (dialstatus) {
    case DIAL_CONNECTED:
      connected = true;
      break;

    case DIAL_NO_CARRIER:
      WWMessageBox().Process(TXT_NO_CARRIER);
      connected = false;
      break;

    case DIAL_BUSY:
      WWMessageBox().Process(TXT_LINE_BUSY);
      connected = false;
      break;

    case DIAL_ERROR:
      WWMessageBox().Process(TXT_NUMBER_INVALID);
      connected = false;
      break;

    case DIAL_NO_DIAL_TONE:
      WWMessageBox().Process(TXT_NO_DIAL_TONE);
      connected = false;
      break;

    case DIAL_CANCELED:
      NullModem.Hangup_Modem();
      Session.ModemService = false;
      WWMessageBox().Process(TXT_DIALING_CANCELED);
      connected = false;
      break;
    default:
      break;
  }

  NullModemClass::Remove_Modem_Echo();
  NullModemClass::Print_EchoBuf();
  NullModem.Reset_EchoBuf();

  /*
  ** Restore audio capability
  */
  SoundOn = OpenAudio(MainWindow, 16, false, 11025 * 2, 0);
  if (SoundOn) {
    Theme.Play_Song(old_theme);
  }

  Session.ModemService = true;
  return connected;

} /* end of Dial_Modem */

static bool Answer_Modem(SerialSettingsType* settings, bool reconnect) {
  bool connected = false;

  /*
  **	Turn modem servicing off in the callback routine.
  */
  Session.ModemService = false;

  // save for later to reconnect

  DialSettings = settings;

  const auto carrier = NullModemClass::Get_Modem_Status();
  if (reconnect) {
    if (carrier & kCdSet) {
      connected = true;
      Session.ModemService = true;
      return connected;
    }
  } else if (carrier & kCdSet) {
    NullModem.Hangup_Modem();
    Session.ModemService = false;
  }

  NullModemClass::Setup_Modem_Echo(Modem_Echo);

  int modemstatus = NullModem.Detect_Modem(settings, reconnect);
  if (!modemstatus) {
    NullModemClass::Remove_Modem_Echo();
    NullModemClass::Print_EchoBuf();
    NullModem.Reset_EchoBuf();
    /*
    ** If our first attempt to detect the modem failed, and we're at
    ** 14400 or 28800, bump up to the next baud rate & try again.
    */
    switch (settings->Baud) {
      case 14400:
        settings->Baud = 19200;
        Shutdown_Modem();
        Init_Null_Modem(settings);
        NullModemClass::Setup_Modem_Echo(Modem_Echo);
        modemstatus = NullModem.Detect_Modem(settings, reconnect);
        if (!modemstatus) {
          NullModemClass::Remove_Modem_Echo();
          NullModemClass::Print_EchoBuf();
          NullModem.Reset_EchoBuf();
          WWMessageBox().Process(TXT_UNABLE_FIND_MODEM);
          Session.ModemService = true;
          return connected;
        }
        break;

      case 28800:
        settings->Baud = 38400;
        Shutdown_Modem();
        Init_Null_Modem(settings);
        NullModemClass::Setup_Modem_Echo(Modem_Echo);
        modemstatus = NullModem.Detect_Modem(settings, reconnect);
        if (!modemstatus) {
          NullModemClass::Remove_Modem_Echo();
          NullModemClass::Print_EchoBuf();
          NullModem.Reset_EchoBuf();
          WWMessageBox().Process(TXT_UNABLE_FIND_MODEM);
          Session.ModemService = true;
          return connected;
        }
        break;

      default:
        WWMessageBox().Process(TXT_UNABLE_FIND_MODEM);
        Session.ModemService = true;
        return connected;
    }
  } else if (modemstatus == -1) {
    NullModemClass::Remove_Modem_Echo();
    NullModemClass::Print_EchoBuf();
    NullModem.Reset_EchoBuf();
    WWMessageBox().Process(TXT_ERROR_IN_INITSTRING);
    Session.ModemService = true;
    return connected;
  }

  /*
  ** Completely disable audio. This is required for some MWave devices like
  *those
  ** found in the IBM Aptiva.
  */
  ThemeType old_theme = THEME_NONE;
  if (SoundOn) {
    old_theme = Theme.What_Is_Playing();
    Theme.Stop();
    CountDownTimerClass wait;
    ServiceRealTime();
    wait.Set(60, true);
    while (wait.Time()) {
      ServiceRealTime();
    }
    CloseAudio();
    ServiceRealTime();
    wait.Set(60, true);
    while (wait.Time()) {
      ServiceRealTime();
    }
    SoundOn = false;
  }

  const DialStatusType dialstatus = NullModem.Answer_Modem(reconnect);

  switch (dialstatus) {
    case DIAL_CONNECTED:
      connected = true;
      break;

    case DIAL_NO_CARRIER:
      WWMessageBox().Process(TXT_NO_CARRIER);
      connected = false;
      break;

      //		case DIAL_BUSY:
      //			WWMessageBox().Process(TXT_LINE_BUSY);
      //			connected = false;
      //			break;

    case DIAL_ERROR:
      WWMessageBox().Process(TXT_NUMBER_INVALID);
      connected = false;
      break;

    case DIAL_CANCELED:
      WWMessageBox().Process(TXT_ANSWERING_CANCELED);
      connected = false;
      break;
    case DialStatusType::DIAL_BUSY:
    case DialStatusType::DIAL_NO_DIAL_TONE:
    default:
      break;
  }

  NullModemClass::Remove_Modem_Echo();
  NullModemClass::Print_EchoBuf();
  NullModem.Reset_EchoBuf();

  /*
  ** Restore audio capability
  */
  SoundOn = OpenAudio(MainWindow, 16, false, 11025 * 2, 0);
  if (SoundOn) {
    Theme.Play_Song(old_theme);
  }

  Session.ModemService = true;
  return connected;

} /* end of Answer_Modem */

static void Modem_Echo(char c) {
  if (NullModem.EchoCount < NullModem.EchoSize - 1) {
    NullModem.EchoBuf.at(base::ToSize(NullModem.EchoCount)) = c;
    NullModem.EchoBuf.at(base::ToSize(NullModem.EchoCount + 1)) = 0;
    NullModem.EchoCount++;
  }

} /* end of Modem_Echo */

void Smart_Print(const std::string_view text) {
  if (Debug_Smart_Print) {
    absl::PrintF("%s", text);
  } else {
    if (Debug_Heap_Dump) {
      absl::PrintF("%s", text);
    }
    if (Debug_Modem_Dump) {
      absl::PrintF("%s", text);
    }
  }
}

void Hex_Dump_Data(std::span<const char> buffer) {
  int length = static_cast<int>(buffer.size());
  int offset = 0;
  char buff[10];
  char ptr[16]{};
  char c = 0;

  while (length >= 16) {
    base::CopyBytes(base::ObjectBytes(ptr),
                    std::as_bytes(buffer.subspan(base::ToSize(offset))),
                    std::min(length, 16));

    Smart_Printf("%05X  ", static_cast<unsigned int>(offset));

    for (int i = 0; i < 16; i++) {
      c = base::At(ptr, i);
      itoh(c, buff);

      if (i % 4 == 0 && i) {
        Smart_Printf("│ ");
      }

      Smart_Printf("%s ", buff);
    }

    Smart_Printf("  ");

    for (const char i : ptr) {
      c = i;

      if (c && (c < 7 || c > 11) && c != 13) {
        Smart_Printf("%c", c);
      } else {
        Smart_Printf(".");
      }
    }

    Smart_Printf("\n");

    offset += 16;
    length -= 16;
  }

  if (length) {
    base::CopyBytes(base::ObjectBytes(ptr),
                    std::as_bytes(buffer.subspan(base::ToSize(offset))),
                    std::min(length, 16));

    Smart_Printf("%05X  ", static_cast<unsigned int>(offset));

    for (int i = 0; i < 16; i++) {
      if (i < length) {
        c = base::At(ptr, i);
        itoh(c, buff);
        if (i % 4 == 0 && i) {
          Smart_Printf("│ ");
        }
        Smart_Printf("%s ", buff);
      } else {
        if (i % 4 == 0 && i) {
          Smart_Printf("  ");
        }
        Smart_Printf("   ");
      }
    }

    Smart_Printf("  ");

    for (int i = 0; i < length; i++) {
      c = base::At(ptr, i);

      if (c && (c < 7 || c > 11) && c != 13) {
        Smart_Printf("%c", c);
      } else {
        Smart_Printf(".");
      }
    }

    Smart_Printf("\n");
  }

} /* end of Hex_Dump_Data */

void itoh(int i, std::span<char> s) {
  constexpr std::string_view digits = "0123456789ABCDEF";
  const auto bits = static_cast<uint32_t>(i);
  base::At(s, 0) = digits.at((bits >> 4) & 0xfU);
  base::At(s, 1) = digits.at(bits & 0xfU);
  base::At(s, 2) = '\0';
}

void Log_Start_Time(const char* string) {
  //	LogDump_Print = true;

  LogLevel = 0;
  base::At(LogLevelTime, LogLevel) = LogLastTime = TickCount.Value();

  Smart_Printf("start tick=%" PRId64 ", %s \n", LogLastTime, string);
}

void Log_End_Time(const char* string) {
  const int64_t currtime = TickCount.Value();
  while (LogLevel >= 0) {
    if (LogLevel < kMaxLogLevel) {
      //
      // put one space for each level as indenting
      //
      int i = 0;
      while (i++ < LogLevel) {
        Smart_Printf(" ");
      }
    } else {
      Smart_Printf("LogLevel %d too large!-! \n", LogLevel);
      LogLevel = kMaxLogLevel - 1;
    }

    const int64_t ticks = currtime - base::At(LogLevelTime, LogLevel--);
    Smart_Printf("end tick=%" PRId64 ", ticks=%" PRId64 ", tsecs=%" PRId64
                 ", %s \n",
                 currtime, ticks, ticks * 10 / 60, string);
  }

  LogDump_Print = false;
}

void Log_Time(const char* string) {
  const int64_t currtime = TickCount.Value();

  if (LogLevel < kMaxLogLevel) {
    //
    // put one space for each level as indenting
    //
    int i = 0;
    while (i++ < LogLevel) {
      Smart_Printf(" ");
    }
  } else {
    Smart_Printf("LogLevel %d too large!-! \n", LogLevel);
    LogLevel = kMaxLogLevel - 1;
  }

  const int64_t ticks = currtime - LogLastTime;

  Smart_Printf("tick=%" PRId64 ", ticks=%" PRId64 ", tsecs=%" PRId64 ", %s \n",
               currtime, ticks, ticks * 10 / 60, string);

  LogLastTime = currtime;
}

void Log_Start_Nest_Time(const char* string) {
  const int64_t currtime = TickCount.Value();

  if (LogLevel < kMaxLogLevel) {
    //
    // put one space for each level as indenting
    //
    int i = 0;
    while (i++ < LogLevel) {
      Smart_Printf(" ");
    }
  } else {
    Smart_Printf("LogLevel %d too large!-! \n", LogLevel);
    LogLevel = kMaxLogLevel - 1;
  }

  const int64_t ticks = currtime - LogLastTime;
  Smart_Printf("start ntick=%" PRId64 ", ticks=%" PRId64 ", tsecs=%" PRId64
               ", %s \n",
               currtime, ticks, ticks * 10 / 60, string);

  if (LogLevel >= kMaxLogLevel - 1) {
    Smart_Printf("Could not start another nesting Maxed at %d,%d!-! \n",
                 LogLevel, kMaxLogLevel - 1);
  } else {
    base::At(LogLevelTime, ++LogLevel) = currtime;
  }

  LogLastTime = currtime;
}

void Log_End_Nest_Time(const char* string) {
  const int64_t currtime = TickCount.Value();

  if (LogLevel <= 0) {
    Smart_Printf("Could not end another nesting Mined at %d,%d!-! \n", LogLevel,
                 0);
    LogLevel = 0;
  }

  if (LogLevel < kMaxLogLevel) {
    //
    // put one space for each level as indenting
    //
    int i = 0;
    while (i++ < LogLevel) {
      Smart_Printf(" ");
    }
  } else {
    Smart_Printf("LogLevel %d too large!-! \n", LogLevel);
    LogLevel = kMaxLogLevel - 1;
  }

  const int64_t ticks = currtime - base::At(LogLevelTime, LogLevel);
  Smart_Printf("end ntick=%" PRId64 ", ticks=%" PRId64 ", secs=%" PRId64
               ", %s \n",
               currtime, ticks, ticks * 10 / 60, string);

  if (LogLevel) {
    LogLevel--;
  }

  LogLastTime = currtime;
}
