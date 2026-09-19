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

// File: Inter-player chat over IPX, serial links and Westwood Online.

#include "ra/chat.h"

#include <memory>
#include <span>
#include <string>

#include "absl/strings/str_format.h"
#include "base/buffer.h"
#include "port/safe_string.h"
#include "ra/config.h"
#include "ra/defines.h"
#include "ra/externs.h"
#include "ra/house.h"
#include "ra/inline.h"
#include "ra/ipx.h"
#include "ra/ipxaddr.h"
#include "ra/ipxgconn.h"
#include "ra/ipxmgr.h"
#include "ra/jshell.h"
#include "ra/mapedit.h"
#include "ra/msglist.h"
#include "ra/netdlg.h"
#include "ra/nullmgr.h"
#include "ra/rawolapi.h"
#include "ra/rules.h"
#include "ra/session.h"
#include "ra/text_ids.h"
#include "ra/vector_dynamic.h"
#include "ra/wolapiob.h"
#include "ra/wolstrng.h"
#include "ra/ww_audio.h"
#include "sdllib/keyboard.h"
#include "tech/fixed.h"

// The key that answers a page from a Westwood Online user outside the
// game.
constexpr KeyNumType kPageRespondKey = KN_RETURN;  // KN_COMMA

// Opens an editable reply addressed to the Westwood Online user who paged us
// from outside the game, or reports that nobody has paged.
//
// Only reachable when config::kWolapiEnabled; the caller gates it with
// `if constexpr` so the whole page-respond path folds away with the toggle.
// The caller has already checked that pWolapi is live. Marked maybe_unused
// because the only call site sits in a discarded `if constexpr` branch when
// the toggle is off, which clang otherwise reports as an unneeded static.
[[maybe_unused]] static void Start_External_Page_Reply() {
  if (*pWolapi->szExternalPager == '\0') {
    Session.Messages.Add_Message(
        nullptr, 0, TXT_WOL_NOTPAGED, PCOLOR_GOLD,
        TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW,
        Rule.MessageDelay * kTicksPerMinute);
    PlaySoundEffect(VOC_SYS_ERROR);
    return;
  }

  // An all-zero address is the flag the send path reads later to mean "this
  // is a reply to whoever paged me from outside the game".
  NetNumType blip;
  NetNodeType blop;
  base::FillBytes(base::ObjectBytes(blip), 0, sizeof(blip));
  base::FillBytes(base::ObjectBytes(blop), 0, sizeof(blop));
  Session.MessageAddress = IPXAddressClass(blip, blop);

  // Tell pWolapi not to reset szExternalPager while the reply is being typed.
  pWolapi->bFreezeExternalPager = true;

  char txt[MAX_MESSAGE_LENGTH + 32] = {};
  // TXT_TO comes from the localized string table, so verify the translation
  // still takes exactly one %s before using it.
  const auto format = absl::ParsedFormat<'s'>::New(Text_String(TXT_TO));
  if (format != nullptr) {
    port::SafeCopy(txt,
                   absl::StrFormat(*format, pWolapi->szExternalPager).c_str());
  }

  Session.Messages.Add_Edit(Session.ColorIdx,
                            TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW,
                            txt, 0, 464);

  Map.Flag_To_Redraw(false);

  Keyboard->Clear();
}

// Fills in Session.GPacket with the message just finished in the edit buffer
// and sends it over IPX, either to every connection (broadcast address) or to
// the single address the F-key handler recorded.
//
// rc is the code MessageListClass::Input returned: 3 for a message that fit
// the edit buffer, 4 for one that spilled into the overflow buffer.
static void Send_Network_Chat_Message(const int rc) {
  Session.GPacket.Command = NET_MESSAGE;
  port::SafeCopy(Session.GPacket.Name, Session.Players.at(0)->Name);
  Session.GPacket.Message.Color = Session.ColorIdx;
  Session.GPacket.Message.NameCRC = Compute_Name_CRC(Session.GameName);

  if (rc == 3) {
    port::SafeCopy(Session.GPacket.Message.Buf,
                   Session.Messages.Get_Edit_Buf());
  } else {
    port::SafeCopy(Session.GPacket.Message.Buf,
                   Session.Messages.Get_Overflow_Buf());
    Session.Messages.Clear_Overflow_Buf();
  }

  // If 'F4' was hit, MessageAddress will be a broadcast address; send the
  // message to every player we have a connection with.
  if (Session.MessageAddress.Is_Broadcast()) {
    char* ptr = &Session.GPacket.Message.Buf[0];
    if (std::string_view(ptr).starts_with("SECRET UNITS ON ") &&
        NewUnitsEnabled) {
      *ptr = 'X';  // force it to an odd hack so we know it was broadcast.
      Enable_Secret_Units();
    }
    for (int i = 0; i < Ipx.Num_Connections(); ++i) {
      Ipx.Send_Global_Message(base::ObjectBytes(Session.GPacket),
                              sizeof(GlobalPacketType), 1,
                              Ipx.Connection_Address(Ipx.Connection_ID(i)));
      Ipx.Service();
    }
  } else {
    // Otherwise, MessageAddress contains the exact address to send to.
    // Send to that address only.
    Ipx.Send_Global_Message(base::ObjectBytes(Session.GPacket),
                            sizeof(GlobalPacketType), 1,
                            &Session.MessageAddress);
    Ipx.Service();
  }

  // Store this message in our LastMessage buffer; the computer may send us a
  // version of it later.
  port::SafeCopy(Session.LastMessage, Session.GPacket.Message.Buf);
}

void Message_Input(KeyNumType& input) {
  char txt[MAX_MESSAGE_LENGTH + 32];

  // Check keyboard input for a request to send a message.
  // The 'to' argument for Add_Edit is prefixed to the message buffer; the
  // message buffer is big enough for the 'to' field plus MAX_MESSAGE_LENGTH.
  // To send the message, calling Get_Edit_Buf retrieves the buffer minus the
  // 'to' portion.  At the other end, the buffer allocated to display the
  // message must be MAX_MESSAGE_LENGTH plus the size of "From: xxx (house)".

  // The page-respond key is not one of the per-player F-keys, so it gets its
  // own gate ahead of them. Compiled and type-checked either way, reachable
  // only when Westwood Online is on.
  if constexpr (config::kWolapiEnabled) {
    if (input == kPageRespondKey && Session.Type == GAME_INTERNET &&
        !Session.Messages.Is_Edit() && pWolapi != nullptr &&
        !pWolapi->bConnectionDown) {
      Start_External_Page_Reply();
    }
  }

  if (Session.Type != GAME_NORMAL && Session.Type != GAME_SKIRMISH &&
      input >= KN_F1 && input < KN_F1 + Session.MaxPlayers &&
      !Session.Messages.Is_Edit()) {
    base::FillBytes(base::ObjectBytes(txt), 0, 40);

    // For a serial game, send a message on F1 or F4; set 'txt' to the
    // "Message:" string & add an editable message to the list.
    if (Session.Type == GAME_NULL_MODEM || Session.Type == GAME_MODEM) {
      if (input == KN_F1 || input == KN_F1 + Session.MaxPlayers - 1) {
        port::SafeCopy(txt, Text_String(TXT_MESSAGE));  // "Message:"

        Session.Messages.Add_Edit(
            Session.ColorIdx, TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW,
            txt, 0, 464);

        Map.Flag_To_Redraw(false);
      }
    } else if ((Session.Type == GAME_IPX || Session.Type == GAME_INTERNET) &&
               !Session.Messages.Is_Edit()) {
      // For a network game:
      // F1-F7 = "To <name> (house):" (only allowed if we're not in
      // ObiWan mode) F8 = "To All:"
      if (input == KN_F1 + Session.MaxPlayers - 1) {
        Session.MessageAddress = IPXAddressClass();    // set to broadcast
        port::SafeCopy(txt, Text_String(TXT_TO_ALL));  // "To All:"

        Session.Messages.Add_Edit(
            Session.ColorIdx, TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW,
            txt, 0, 464);

        Map.Flag_To_Redraw(false);

      } else if (input - KN_F1 < Ipx.Num_Connections() && !Session.ObiWan) {
        const int id = Ipx.Connection_ID(input - KN_F1);
        Session.MessageAddress = *Ipx.Connection_Address(id);
        // TXT_TO comes from the localized string table, so verify the
        // translation still takes exactly one %s before using it.
        const auto format = absl::ParsedFormat<'s'>::New(Text_String(TXT_TO));
        if (format != nullptr) {
          port::SafeCopy(
              txt, absl::StrFormat(*format, Ipx.Connection_Name(id)).c_str());
        }

        Session.Messages.Add_Edit(
            Session.ColorIdx, TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW,
            txt, 0, 464);

        Map.Flag_To_Redraw(false);
      }
    }
  }

  // Process message-system input; send the message out if RETURN is hit.
  const KeyNumType copy_input = input;
  const int rc = Session.Messages.Input(input);

  // If a single character has been added to an edit buffer, update the
  // display.
  if (rc == 1 && Session.Type != GAME_NORMAL) {
    Map.Flag_To_Redraw(false);
  }

  // If backspace was hit, redraw the map.  If the edit message was removed,
  // the map must be force-drawn, since it won't be able to compute the
  // cells to redraw; otherwise, let the map compute the cells to redraw,
  // by not force-drawing it, but just setting the IsToRedraw bit.
  if (rc == 2 && Session.Type != GAME_NORMAL) {
    if (copy_input == KN_ESC) {
      Map.Flag_To_Redraw(true);
      if constexpr (config::kWolapiEnabled) {
        if (pWolapi) {
          // Just in case user was responding to a page from outside the
          // game, and we had frozen the "szExternalPager".
          pWolapi->bFreezeExternalPager = false;
        }
      }
    } else {
      Map.Flag_To_Redraw(false);
    }
    Map.IsDisplayToRedraw = true;
  }

  // Send a message
  if ((rc == 3 || rc == 4) && Session.Type != GAME_NORMAL &&
      Session.Type != GAME_SKIRMISH) {
    // Serial game: fill in a SerialPacketType & send it.
    // (Note: The size of the SerialPacketType.Command must be the same as
    // the EventClass.Type!)
    if (Session.Type == GAME_NULL_MODEM || Session.Type == GAME_MODEM) {
      SerialPacketType packet_storage{
          .Command = SERIAL_MESSAGE, .Name = {}, .ID = 0, .ScenarioInfo = {}};
      auto* serial_packet = &packet_storage;

      port::SafeCopy(serial_packet->Name, Session.Players.at(0)->Name);
      serial_packet->ID = static_cast<unsigned char>(Session.ColorIdx);

      if (rc == 3) {
        port::SafeCopy(serial_packet->Message.Message,
                       Session.Messages.Get_Edit_Buf());
      } else {
        port::SafeCopy(serial_packet->Message.Message,
                       Session.Messages.Get_Overflow_Buf());
        Session.Messages.Clear_Overflow_Buf();
      }

      // Send the message, and store this message in our LastMessage
      // buffer; the computer may send us a version of it later.
      NullModem.Send_Message(base::ObjectBytes(packet_storage),
                             sizeof(SerialPacketType), 1);

      // A chat message is how the secret units get switched on for everyone
      // at once: both ends recognize the phrase and enable them locally, so
      // the setting stays in step without a new packet type.
      const char* ptr = &serial_packet->Message.Message[0];
      if (std::string_view(ptr).starts_with("SECRET UNITS ON ") &&
          NewUnitsEnabled) {
        Enable_Secret_Units();
      }
      port::SafeCopy(Session.LastMessage, serial_packet->Message.Message);
    } else if (Session.Type == GAME_IPX || Session.Type == GAME_INTERNET) {
      if constexpr (config::kWolapiEnabled) {
        // An all-zero address is the flag Start_External_Page_Reply set,
        // meaning "this is a reply to whoever paged me from outside the
        // game". No F-key ever produces that address.
        NetNumType blip;
        NetNodeType blop;
        Session.MessageAddress.Get_Address(blip, blop);
        const bool reply_to_external_page =
            blip[0] + blip[1] + blip[2] + blip[3] + blop[0] + blop[1] +
                blop[2] + blop[3] + blop[4] + blop[5] ==
            0;
        if (reply_to_external_page) {
          // (As connection may have gone down.)
          if (pWolapi && !pWolapi->bConnectionDown) {
            // The HRESULT carries nothing here: asked not to wait for a
            // result, Page returns 0 whether or not the request went out.
            static_cast<void>(pWolapi->Page(pWolapi->szExternalPager,
                                            Session.Messages.Get_Edit_Buf(),
                                            false));
            pWolapi->bFreezeExternalPager = false;
          }
        } else {
          Send_Network_Chat_Message(rc);
        }
      } else {
        Send_Network_Chat_Message(rc);
      }
    }

    // Tell the map to completely update itself, since a message is now
    // missing.
    Map.Flag_To_Redraw(true);
  }
}

void IPX_Call_Back() {
  Ipx.Service();

  // Read packets only if the game is "closed", so we don't steal global
  // messages from the connection dialogs.
  if ((!Session.NetOpen) &&
      Ipx.Get_Global_Message(base::ObjectBytes(Session.GPacket),
                             &Session.GPacketlen, &Session.GAddress,
                             &Session.GProductID) &&
      (Session.GProductID == IPXGlobalConnClass::kCommandAndConquer0))

  {
    // If this is another player signing off, remove the connection &
    // mark that player's house as non-human, so the computer will take
    // it over.
    if (Session.GPacket.Command == NET_SIGN_OFF) {
      for (int i = 0; i < Ipx.Num_Connections(); i++) {
        const int id = Ipx.Connection_ID(i);

        if (Session.GAddress == *Ipx.Connection_Address(id)) {
          Destroy_Connection(id, 0);
        }
      }
    } else {
      // Process a message from another user.
      if (Session.GPacket.Command == NET_MESSAGE) {
        bool msg_ok = false;

        // If NetProtect is set, make sure this message came from within
        // this game.
        if (!Session.NetProtect) {
          msg_ok = true;
        } else {
          msg_ok = Session.GPacket.Message.NameCRC ==
                   Compute_Name_CRC(Session.GameName);
        }

        if (msg_ok) {
          if (!Session.Messages.Concat_Message(
                  Session.GPacket.Name,
                  static_cast<int>(Session.GPacket.Message.Color),
                  Session.GPacket.Message.Buf,
                  Rule.MessageDelay * kTicksPerMinute)) {
            if (NewUnitsEnabled && std::string_view(Session.GPacket.Message.Buf)
                                       .starts_with("XECRET UNITS ON ")) {
              Session.GPacket.Message.Buf[0] = 'S';
              Enable_Secret_Units();
            }
            Session.Messages.Add_Message(
                Session.GPacket.Name,
                static_cast<int>(Session.GPacket.Message.Color),
                Session.GPacket.Message.Buf, Session.GPacket.Message.Color,
                TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW,
                Rule.MessageDelay * kTicksPerMinute);

            PlaySoundEffect(VOC_INCOMING_MESSAGE);
          }

          // Tell the map to do a partial update (just to force the
          // messages to redraw).
          Map.Flag_To_Redraw(true);

          // Save this message in our last-message buffer
          port::SafeCopy(Session.LastMessage, Session.GPacket.Message.Buf);
        }
      } else {
        Process_Global_Packet(&Session.GPacket, &Session.GAddress);
      }
    }
  }
}

// The body -- set SecretUnitsEnabled, drop the phase tank and the helicarrier
// to tech level 10, refresh every building's buildables -- was disabled before
// release. The chat trigger is still recognized so the message is relayed the
// same way on every machine.
void Enable_Secret_Units() {}
