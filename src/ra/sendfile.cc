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

/*************************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S **
 *************************************************************************************
 *                                                                                   *
 *                 Project Name : Command & Conquer - Red Alert *
 *                                                                                   *
 *                    File Name : SENDFILE.CPP *
 *                                                                                   *
 *                   Programmer : Steve Tall *
 *                                                                                   *
 *                   Start Date : Audust 20th, 1996 *
 *                                                                                   *
 *                  Last Update : August 20th, 1996 [ST] *
 *                                                                                   *
 *-----------------------------------------------------------------------------------*
 * Overview: *
 *                                                                                   *
 *  Functions for scenario file transfer between machines *
 *                                                                                   *
 *-----------------------------------------------------------------------------------*
 * Functions: *
 *                                                                         				*
 *                                                                         				*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - */

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>

#include "base/array.h"
#include "base/buffer.h"
#include "port/safe_string.h"
#include "ra/conquer.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/externs.h"
#include "ra/gadget.h"
#include "ra/gauge.h"
#include "ra/globals.h"
#include "ra/init.h"
#include "ra/inline.h"
#include "ra/ipxaddr.h"
#include "ra/ipxmgr.h"
#include "ra/jshell.h"
#include "ra/mission_id.h"
#include "ra/nullmgr.h"
#include "ra/palette.h"
#include "ra/session.h"
#include "ra/textbtn.h"
#include "sdllib/file_access.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "tech/disk_file.h"
#include "tech/ftimer.h"
#include "tech/game_file.h"

static bool Receive_Remote_File(const char* file_name, int file_length,
                                int gametype);

#define RESPONSE_TIMEOUT (int64_t{60} * 60)

#include "ra/config.h"
#include "ra/wolapiob.h"
#include "sdllib/timer.h"

namespace {

//	A scenario transfer can take a while, and the chat server drops a client
//	that stops answering. Keep pumping it while we wait.
void PumpWolapi() {
  if constexpr (config::kWolapiEnabled) {
    if (Session.Type == GAME_INTERNET && pWolapi != nullptr &&
        Get_Time_Ms() > pWolapi->dwTimeNextWolapiPump) {
      pWolapi->pChat->PumpMessages();
      pWolapi->dwTimeNextWolapiPump = Get_Time_Ms() + WOLAPIPUMPWAIT;
    }
  }
}

}  // namespace

/***********************************************************************************************
 * Get_Scenario_File_From_Host -- Initiates download of scenario file from game
 *host           *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to buffer to copy file name into * game type - 0 for modem/null
 *modem, 1 otherwise                                   *
 *                                                                                             *
 * OUTPUT:   true if file successfully downloaded *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/22/96 3:06PM ST : Created *
 *=============================================================================================*/
bool Get_Scenario_File_From_Host(std::span<char> return_name, size_t dest_size,
                                 int gametype) {
  // WWDebugString ("RA95 - In Get_Scenario_From_Host\n");

  int file_length = 0;

  SerialPacketType send_packet;
  SerialPacketType receive_packet;
  GlobalPacketType net_send_packet;
  GlobalPacketType net_receive_packet;
  int packet_len = 0;
  uint16_t product_id = 0;

  IPXAddressClass sender_address;

  Timer<SystemTickSource>
      response_timer;  // timeout timer for waiting for responses

  /*
  ** Send the scenario request using guaranteed delivery.
  */
  if (!gametype) {
    base::FillBytes(base::ObjectBytes(send_packet), 0, sizeof(send_packet));
    send_packet.Command = SERIAL_REQ_SCENARIO;
    NullModem.Send_Message(base::ObjectBytes(send_packet), sizeof(send_packet),
                           1);
  } else {
    base::FillBytes(base::ObjectBytes(net_send_packet), 0,
                    sizeof(net_send_packet));
    net_send_packet.Command = NET_REQ_SCENARIO;
    Ipx.Send_Global_Message(base::ObjectBytes(net_send_packet),
                            sizeof(net_send_packet), 1, &Session.HostAddress);
  }

  // WWDebugString ("RA95 - Waiting for response from host\n");

  /*
  ** Wait for host to respond with a file info packet
  */
  response_timer.Set(RESPONSE_TIMEOUT);
  if (!gametype) {
    do {
      NullModem.Service();

      if ((NullModem.Get_Message(base::ObjectBytes(receive_packet),
                                 &packet_len) > 0) &&
          (receive_packet.Command == SERIAL_FILE_INFO)) {
        port::SafeCopy(
            return_name.first(std::min(return_name.size(), dest_size)),
            receive_packet.ScenarioInfo.ShortFileName);
        file_length = static_cast<int>(receive_packet.ScenarioInfo.FileLength);
        break;
      }

    } while (response_timer.HasTimeLeft());
  } else {
    do {
      Ipx.Service();
      int receive_packet_length = sizeof(net_receive_packet);
      if (Ipx.Get_Global_Message(base::ObjectBytes(net_receive_packet),
                                 &receive_packet_length, &sender_address,
                                 &product_id) &&
          (net_receive_packet.Command == NET_FILE_INFO &&
           sender_address == Session.HostAddress))
      // WWDebugString ("RA95 - Got packet from host\n");
      {
        port::SafeCopy(
            return_name.first(std::min(return_name.size(), dest_size)),
            net_receive_packet.ScenarioInfo.ShortFileName);
        file_length =
            static_cast<int>(net_receive_packet.ScenarioInfo.FileLength);
        // WWDebugString ("RA95 - Got file info packet from host\n");
        break;
      }

      PumpWolapi();
    } while (response_timer.HasTimeLeft());
  }

  // char rt[80];
  // sprintf (rt, "RA95 - response_timer = %d\n", response_timer );
  // WWDebugString (rt);

  /*
  ** If we timed out then something horrible has happened to the other player so
  *just
  ** return failure.
  */
  if (response_timer.IsFinished()) {
    return false;
  }

  //	debugprint( "about to download '%s'\n", return_name );

  /*
  ** Receive the file from the host
  */
  return Receive_Remote_File(return_name.data(), file_length, gametype);
}

/***********************************************************************************************
 * Receive_Remote_File -- Handles incoming file download packets from the game
 *host            *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    file name to save as * length of file to expect * game type - 0 for
 *modem/null modem, 1 otherwise                                   *
 *                                                                                             *
 * OUTPUT:   true if file downloaded was completed *
 *                                                                                             *
 * WARNINGS: This fuction can modify the file name passed in *
 *                                                                                             *
 * HISTORY: * 8/22/96 3:07PM ST : Created *
 *=============================================================================================*/
bool Receive_Remote_File(const char* file_name, int file_length, int gametype) {
  // WWDebugString ("RA95 - In Receive_Remote_File\n");
  uint16_t product_id = 0;
  IPXAddressClass sender_address;

  /*
  ** Dialog & button dimensions
  */
  const int d_dialog_w = 400;                             // dialog width
  const int d_dialog_h = 180;                             // dialog height
  const int d_dialog_x = (640 - d_dialog_w) / 2;          // dialog x-coord
  const int d_dialog_y = (400 - d_dialog_h) / 2;          // centered y-coord
  const int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);  // center x-coord

  const int d_cancel_w = config::kIsEnglish ? 80 : 100;
  const int d_cancel_h = 18;
  const int d_cancel_x = d_dialog_cx - (d_cancel_w / 2);
  const int d_cancel_y = d_dialog_y + d_dialog_h - 40;

  const int d_progress_w = 200;
  const int d_progress_h = 20;
  const int d_progress_x = (SeenBuff.Get_Width() / 2) - (d_progress_w / 2);
  const int d_progress_y = d_dialog_y + 90;

  int width = 0;
  int height = 0;

  // Format_Window_String inserts line breaks in place, so format a copy
  // rather than the shared string table.
  std::string info_string(Text_String(TXT_RECEIVING_SCENARIO));

  Fancy_Text_Print(TXT_NONE, 0, 0, GadgetClass::Get_Color_Scheme(), kTBlack,
                   TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

  Format_Window_String(std::span(info_string), SeenBuff.Get_Height(), width,
                       height);

  /*
  ** Button Enumerations
  */
  constexpr int kButtonCancel = 100;
  constexpr int kButtonProgress = 101;

  /*
  ** Buttons
  */
  // TextButtonClass *buttons;
  // // button list

  TextButtonClass cancelbtn(
      kButtonCancel, TXT_CANCEL,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
      // The German and French captions outgrow the button, so those builds
      // let it size itself to the text.
      d_cancel_x, d_cancel_y, config::kIsEnglish ? d_cancel_w : -1,
      config::kIsEnglish ? d_cancel_h : -1);

  GaugeClass progress_meter(kButtonProgress, d_progress_x, d_progress_y,
                            d_progress_w, d_progress_h);

  Fancy_Text_Print(TXT_NONE, 0, 0, GadgetClass::Get_Color_Scheme(), kTBlack,
                   TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_PROGRESS = 1,
    REDRAW_BUTTONS = 2,
    REDRAW_BACKGROUND = 3,
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;

  bool process = true;
  RedrawType display = REDRAW_ALL;  // redraw level
  bool return_code = false;
  int update_time = 0;

  RemoteFileTransferType receive_packet;

  int last_received_block = -1;  // No blocks received yet
  int total_length = 0;
  int packet_len = 0;

  /*
  ** If the file name is already in use, use the temp file name
  */
  GameFile test_file(file_name);

  std::string save_file_name;
  if (test_file.IsAvailable()) {
    save_file_name = "DOWNLOAD.TMP";
  } else {
    save_file_name = std::string(file_name);
  }

  DiskFile save_file(save_file_name);

  /*
  ** If the file already exists then delete it and re-create it.
  */
  if (save_file.IsAvailable()) {
    save_file.Delete();
  }

  /*
  ** Open the file for write
  */
  save_file.Open(FileAccess::kWrite);

  GadgetClass* commands = &cancelbtn;  // button list
  commands->Add_Tail(progress_meter);

  progress_meter.Set_Maximum(100);  // Max is 100%
  progress_meter.Set_Value(0);      // Current is 0%

  /*
  ** Wait for all the blocks to arrive
  */

  do {
    /*
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      display = REDRAW_ALL;
    }

    PumpWolapi();

    if (display != REDRAW_NONE) {
      if (display >= REDRAW_BACKGROUND) {
        Hide_Mouse();
        /*
        ** Redraw backgound & dialog box
        */
        Load_Title_Page(true);
        Set_Palette(CCPalette);

        Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);

        /*
        ** Dialog & Field labels
        */
        Draw_Caption(TXT_NONE, d_dialog_x, d_dialog_y, d_dialog_w);

        Fancy_Text_Print(info_string.c_str(), d_dialog_cx - (width / 2),
                         d_dialog_y + 50, GadgetClass::Get_Color_Scheme(),
                         kTBlack,
                         TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        Show_Mouse();
      }

      if (display >= REDRAW_BUTTONS) {
        commands->Draw_All();
      }

      if (display >= REDRAW_PROGRESS) {
        progress_meter.Draw_Me(true);
      }

      display = REDRAW_NONE;
    }

    if (!gametype) {
      NullModem.Service();

      if ((NullModem.Get_Message(base::ObjectBytes(receive_packet),
                                 &packet_len) > 0) &&
          (receive_packet.Command == SERIAL_FILE_CHUNK) &&
          (receive_packet.BlockNumber == last_received_block + 1)) {
        save_file.Write(receive_packet.RawData, receive_packet.BlockLength);
        total_length += receive_packet.BlockLength;
        last_received_block++;

        update_time++;
        if (update_time > 7) {
          progress_meter.Set_Value(total_length * 100 / file_length);
          display = REDRAW_PROGRESS;
          update_time = 0;
        }

        if (total_length >= file_length) {
          process = false;
          return_code = true;
          progress_meter.Set_Value(100);
          progress_meter.Draw_Me(true);
        }
      }

    } else {
      Ipx.Service();

      int receive_packet_len = sizeof(receive_packet);
      if (Ipx.Get_Global_Message(base::ObjectBytes(receive_packet),
                                 &receive_packet_len, &sender_address,
                                 &product_id) &&
          (receive_packet.Command == SERIAL_FILE_CHUNK &&
           sender_address == Session.HostAddress) &&
          (receive_packet.BlockNumber == last_received_block + 1))

      {
        save_file.Write(receive_packet.RawData, receive_packet.BlockLength);
        total_length += receive_packet.BlockLength;
        last_received_block++;

        update_time++;
        if (update_time > 7) {
          progress_meter.Set_Value(total_length * 100 / file_length);
          display = REDRAW_PROGRESS;
          update_time = 0;
        }

        if (total_length >= file_length) {
          process = false;
          return_code = true;
          progress_meter.Set_Value(100);
          progress_meter.Draw_Me(true);
        }
      }
    }

    if (process) {
      const KeyNumType input = cancelbtn.Input();

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
          return_code = false;
          break;
        default:
          break;
      }
    }

  } while (process);

  save_file.Close();

  /*
  ** Update the internal list of scenarios to include the downloaded one so we
  *know about it
  **  for the next game.
  */
  Session.Read_Scenario_Descriptions();

  return return_code;
}

/***********************************************************************************************
 * Send_Remote_File -- Sends a file to game clients *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    File name *
 *                                                                                             *
 * OUTPUT:   true if file transfer was successfully completed * game type - 0
 *for modem/null modem, 1 otherwise                                   *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/22/96 3:09PM ST : Created *
 *=============================================================================================*/
bool Send_Remote_File(const char* file_name, int gametype) {
  // WWDebugString ("RA95 - In Send_Remote_File\n");

  /*
  ** Dialog & button dimensions
  */
  const int factor = SeenBuff.Get_Width() == 320 ? 1 : 2;

  const int d_dialog_w = 240 * factor;                       // dialog width
  const int d_dialog_h = 90 * factor;                        // dialog height
  const int d_dialog_x = ((320 * factor) - d_dialog_w) / 2;  // dialog x-coord
  const int d_dialog_y = ((200 * factor) - d_dialog_h) / 2;  // centered y-coord
  const int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);     // center x-coord

  const int d_cancel_w = (config::kIsEnglish ? 40 : 50) * factor;
  const int d_cancel_h = 9 * factor;
  const int d_cancel_x = d_dialog_cx - (d_cancel_w / 2);
  const int d_cancel_y = d_dialog_y + d_dialog_h - (20 * factor);

  const int d_progress_w = 100 * factor;
  const int d_progress_h = 10 * factor;
  const int d_progress_x = (SeenBuff.Get_Width() / 2) - (d_progress_w / 2);
  const int d_progress_y = d_dialog_y + (45 * factor);

  int width = 0;
  int height = 0;

  // Format_Window_String inserts line breaks in place, so format a copy
  // rather than the shared string table.
  std::string info_string(Text_String(TXT_SENDING_SCENARIO));

  Timer<SystemTickSource>
      response_timer;  // timeout timer for waiting for responses

  Fancy_Text_Print(TXT_NONE, 0, 0, GadgetClass::Get_Color_Scheme(), kTBlack,
                   TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

  Format_Window_String(std::span(info_string), SeenBuff.Get_Height(), width,
                       height);

  /*
  ** Button Enumerations
  */
  constexpr int kButtonCancel = 100;
  constexpr int kButtonProgress = 101;

  /*
  ** Buttons
  */
  // TextButtonClass *buttons;
  // // button list

  TextButtonClass cancelbtn(
      kButtonCancel, TXT_CANCEL,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
      // The German and French captions outgrow the button, so those builds
      // let it size itself to the text.
      d_cancel_x, d_cancel_y, config::kIsEnglish ? d_cancel_w : -1,
      config::kIsEnglish ? d_cancel_h : -1);

  GaugeClass progress_meter(kButtonProgress, d_progress_x, d_progress_y,
                            d_progress_w, d_progress_h);

  Fancy_Text_Print(TXT_NONE, 0, 0, GadgetClass::Get_Color_Scheme(), kTBlack,
                   TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_PROGRESS = 1,
    REDRAW_BUTTONS = 2,
    REDRAW_BACKGROUND = 3,
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;

  bool process = true;
  RedrawType display = REDRAW_ALL;  // redraw level
  bool return_code = false;
  int update_time = 0;


  RemoteFileTransferType send_packet;
  SerialPacketType file_info;
  GlobalPacketType net_file_info;

  GameFile send_file(file_name);

  if (!send_file.IsAvailable()) {
    // WWDebugString ("RA95 - Error - could not find file to send to client\n");
    //		debugprint("RA95 - Error - could not find file to send to
    // client\n");
    return false;
  }
  int file_length = static_cast<int>(send_file.Size());

  response_timer.Set(RESPONSE_TIMEOUT);

  /*
  ** Send the file info to the remote machine(s)
  */
  if (!gametype) {
    file_info.Command = SERIAL_FILE_INFO;
    port::SafeCopy(file_info.ScenarioInfo.ShortFileName, file_name);
    //	If we're sending an official map, always send it to 'download.tmp'.
    if (IsMissionCounterstrike(file_name) || IsMissionAftermath(file_name)) {
      port::SafeCopy(file_info.ScenarioInfo.ShortFileName, "DOWNLOAD.TMP");
    }
    file_info.ScenarioInfo.FileLength = static_cast<unsigned>(file_length);
    NullModem.Send_Message(base::ObjectBytes(file_info), sizeof(file_info), 1);
    while (NullModem.Num_Send() > 0 && response_timer.HasTimeLeft()) {
      NullModem.Service();
    }
  } else {
    net_file_info.Command = NET_FILE_INFO;
    port::SafeCopy(net_file_info.ScenarioInfo.ShortFileName, file_name);
    //		debugprint( "Uploading '%s'\n", file_name );
    //	If we're sending an official map, always send it to 'download.tmp'.
    if (IsMissionCounterstrike(file_name) || IsMissionAftermath(file_name)) {
      port::SafeCopy(net_file_info.ScenarioInfo.ShortFileName, "DOWNLOAD.TMP");
    }
    //		debugprint( "ShortFileName is '%s'\n",
    // net_file_info.ScenarioInfo.ShortFileName );
    net_file_info.ScenarioInfo.FileLength = static_cast<unsigned>(file_length);

    for (int i = 0; i < Session.RequestCount; i++) {
      Ipx.Send_Global_Message(
          base::ObjectBytes(net_file_info), sizeof(GlobalPacketType), 1,
          &Session.Players.at(base::At(Session.ScenarioRequests, i))->Address);
    }

    while (Ipx.Global_Num_Send() > 0 && response_timer.HasTimeLeft()) {
      Ipx.Service();
    }
  }

  const int max_chunk_size = MAX_SEND_FILE_PACKET_SIZE;
  const int total_blocks = (file_length + max_chunk_size - 1) / max_chunk_size;

  send_file.Open(FileAccess::kRead);

  GadgetClass* commands = &cancelbtn;  // button list
  commands->Add_Tail(progress_meter);

  progress_meter.Set_Maximum(100);  // Max is 100%
  progress_meter.Set_Value(0);      // Current is 0%

  int block_number = 0;

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

    PumpWolapi();

    if (display != REDRAW_NONE) {
      if (display >= REDRAW_BACKGROUND) {
        Hide_Mouse();
        /*
        ** Redraw backgound & dialog box
        */
        Load_Title_Page(true);
        Set_Palette(CCPalette);

        Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);

        /*
        ** Dialog & Field labels
        */
        Draw_Caption(TXT_NONE, d_dialog_x, d_dialog_y, d_dialog_w);

        Fancy_Text_Print(info_string.c_str(), d_dialog_cx - (width / 2),
                         d_dialog_y + (25 * factor),
                         GadgetClass::Get_Color_Scheme(), kTBlack,
                         TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        Show_Mouse();
      }

      if (display >= REDRAW_BUTTONS) {
        commands->Draw_All();
      }

      if (display >= REDRAW_PROGRESS) {
        progress_meter.Draw_Me(true);
      }

      display = REDRAW_NONE;
    }

    if (!gametype) {
      NullModem.Service();

      if (block_number < total_blocks) {
        if (NullModem.Num_Send() < 2) {
          send_packet.Command = SERIAL_FILE_CHUNK;
          send_packet.BlockNumber = static_cast<uint16_t>(block_number);
          send_packet.BlockLength =
              static_cast<uint16_t>(std::min(file_length, max_chunk_size));

          file_length -= send_packet.BlockLength;

          if (send_file.Read(send_packet.RawData, send_packet.BlockLength) ==
              send_packet.BlockLength) {
            NullModem.Send_Message(base::ObjectBytes(send_packet),
                                   sizeof(send_packet), 1);
          }

          block_number++;

          update_time++;
          if (update_time > 7) {
            progress_meter.Set_Value(block_number * 100 / total_blocks);
            display = REDRAW_PROGRESS;
            update_time = 0;
          }
        }
      } else {
        if (NullModem.Num_Send() == 0) {
          process = false;
          return_code = true;
          progress_meter.Set_Value(100);
          progress_meter.Draw_Me(true);
        }
      }

    } else {
      Ipx.Service();

      if (block_number < total_blocks) {
        if (Ipx.Global_Num_Send() == 0) {
          send_packet.Command = SERIAL_FILE_CHUNK;
          send_packet.BlockNumber = static_cast<uint16_t>(block_number);
          send_packet.BlockLength =
              static_cast<uint16_t>(std::min(file_length, max_chunk_size));

          file_length -= send_packet.BlockLength;

          if (send_file.Read(send_packet.RawData, send_packet.BlockLength) ==
              send_packet.BlockLength) {
            for (int i = 0; i < Session.RequestCount; i++) {
              Ipx.Send_Global_Message(
                  base::ObjectBytes(send_packet), sizeof(send_packet), 1,
                  &Session.Players.at(base::At(Session.ScenarioRequests, i))
                       ->Address);
            }
          }

          block_number++;

          update_time++;
          if (update_time > 7) {
            progress_meter.Set_Value(block_number * 100 / total_blocks);
            display = REDRAW_PROGRESS;
            update_time = 0;
          }
        }
      } else {
        if (Ipx.Global_Num_Send() == 0) {
          process = false;
          return_code = true;
          progress_meter.Set_Value(100);
          progress_meter.Draw_Me(true);
        }
      }
    }

    if (process) {
      const KeyNumType input = cancelbtn.Input();

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
          return_code = false;
          break;
        default:
          break;
      }
    }
  }

  return return_code;
}
