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

/* $Header:   F:\projects\c&c\vcs\code\netdlg.cpv   2.17   16 Oct 1995 16:52:26
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                         						  *
 *                 Project Name : Command & Conquer
 **
 *                                                                         						  *
 *                    File Name : NETDLG.CPP
 **
 *                                                                         						  *
 *                   Programmer : Bill Randolph
 **
 *                                                                         						  *
 *                   Start Date : January 23, 1995
 **
 *                                                                         						  *
 *                  Last Update : July 8, 1995 [BRR]
 **
 *                                                                         						  *
 *---------------------------------------------------------------------------------------------*
 *                                                                         						  *
 * These routines establish & maintain peer-to-peer connections between this
 *system				  * and all others in the game.  Each
 *system finds out the IPX address of the others,			  * and
 *forms a direct connection (IPXConnectionClass) to that system.  Systems are
 ** found out via broadcast queries.  Every system broadcasts its queries, and
 *every				  * system replies to queries it receives.  At
 *the point when the game owner signals				  * 'OK', every
 *system must know about all the other systems in the game.
 **
 *                                                                         						  *
 * How Bridges are handled:
 ** Currently, bridges are handled by specifying the destination IPX address of
 *the				  * "server" (game owner's system) on the
 *command-line.  This address is used to * derive a broadcast address to that
 *destination network, and this system's queries			  * are
 * broadcast over its network & the server's network; replies to the queries
 * come			  * with each system's IPX address attached, so once we
 * have the address, we can form			  * a connection with
 * any system on the bridged net.
 **
 *                                                                         						  *
 * The flaw in this plan is that we can only cross one bridge.  If there are 3
 *nets				  * bridged (A, B, & C), and the server is on
 *net B, and we're on net A, our broadcasts			  * will reach
 *nets A & B, but not C.  The way to circumvent this (if it becomes a problem)
 ** would be to have the server tell us what other systems are in its game, not
 *each				  * individual player's system.  Thus, each
 *system would find out about all the other systems	  * by interacting with
 *the game's owner system (this would be more involved than what
 ** I'm doing here).
 **
 *                                                                         						  *
 * Here's a list of all the different packets sent over the Global Channel:
 **
 *																															  *
 *	NET_QUERY_GAME
 ** (no other data)
 ** NET_ANSWER_GAME
 ** Name:					game owner's name
 ** GameInfo:			game's version & open state
 ** NET_QUERY_PLAYER
 ** Name:					name of game we want players to
 *respond for				  * NET_ANSWER_PLAYER
 ** Name:					player's name
 ** PlayerInfo:			info about player
 ** NET_QUERY_JOIN
 ** Name:					name of player wanting to join
 ** PlayerInfo:			player's requested house & color
 ** NET_CONFIRM_JOIN
 ** PlayerInfo:			approves player's house & color
 ** NET_REJECT_JOIN
 ** (no other data)
 ** NET_GAME_OPTIONS
 ** ScenarioInfo:		info about scenario
 ** NET_SIGN_OFF
 ** Name:					name of player signing off
 ** NET_PING
 ** (no other data)
 ** NET_GO
 ** Delay:            value of one-way response time, in frames               *
 * 																														  *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Clear_Game_List -- Clears the game-name listbox & 'Games' Vector
 ** Clear_Player_List -- Clears the player-name listbox & Vector
 ** Destroy_Connection -- destroys the given connection
 ** Get_Join_Responses -- sends queries for the Join Dialog
 ** Get_NewGame_Responses -- processes packets for New Game dialog *
 *   Init_Network -- initializes network stuff
 ** Net_Join_Dialog -- lets user join an existing game, or start a new one
 ** Net_New_Dialog -- lets user start a new game
 ** Process_Global_Packet -- responds to remote queries
 ** Remote_Connect -- handles connecting this user to others
 ** Request_To_Join -- Sends a JOIN request packet to game owner
 ** Send_Join_Queries -- sends queries for the Join Dialog
 ** Shutdown_Network -- shuts down network stuff
 ** Compute_Name_CRC -- computes CRC from char string * Net_Reconnect_Dialog --
 *Draws/updates the network reconnect dialog                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/netdlg.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>
#include <string_view>
#include <utility>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "port/ex_string.h"
#include "port/random_seed.h"
#include "port/safe_string.h"
#include "port/unaligned.h"
#include "sdllib/drawbuff.h"
#include "sdllib/font.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/timer.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "td/colrlist.h"
#include "td/conquer.h"
#include "td/defines.h"
#include "td/dialog.h"
#include "td/edit.h"
#include "td/event.h"
#include "td/externs.h"
#include "td/gadget.h"
#include "td/gauge.h"
#include "td/globals.h"
#include "td/goptions.h"
#include "td/house.h"
#include "td/init.h"
#include "td/ipx.h"
#include "td/ipxgconn.h"
#include "td/ipxmgr.h"
#include "td/jshell.h"
#include "td/list.h"
#include "td/mapedit.h"
#include "td/mplayer.h"
#include "td/msgbox.h"
#include "td/msglist.h"
#include "td/nodename.h"
#include "td/palette.h"
#include "td/queue.h"
#include "td/special.h"
#include "td/tcpip.h"
#include "td/text.h"
#include "td/textbtn.h"
#include "td/vector.h"
#include "tech/crc.h"
#include "tech/number_parse.h"

#ifdef _WIN32
#include "td/ccdde.h"
#endif

#define SHOW_MONO 0

#ifndef DEMO

/*---------------------------------------------------------------------------
The possible states of the join-game dialog
---------------------------------------------------------------------------*/
enum class JoinStateType {
  JOIN_REJECTED = -1,  // we've been rejected
  JOIN_NOTHING,        // we're not trying to join a game
  JOIN_WAIT_CONFIRM,   // we're asking to join, & waiting for confirmation
  JOIN_CONFIRMED,      // we've been confirmed
  JOIN_GAME_START,     // the game we've joined is starting
};
using enum JoinStateType;

/*---------------------------------------------------------------------------
The possible return codes from Get_Join_Responses()
---------------------------------------------------------------------------*/
enum class JoinEventType {
  EV_NONE,            // nothing happened
  EV_STATE_CHANGE,    // Join dialog is in a new state
  EV_NEW_GAME,        // a new game was detected
  EV_NEW_PLAYER,      // a new player was detected
  EV_PLAYER_SIGNOFF,  // a player has signed off
  EV_GAME_SIGNOFF,    // a gamed owner has signed off
  EV_GAME_OPTIONS,    // a game options packet was received
  EV_MESSAGE,         // a message was received
};
using enum JoinEventType;

// Size of the heap buffers holding the "xxx's Game" entries of the game list:
// a player name plus room for the surrounding text and brackets.
constexpr size_t kGameListItemSize = MPLAYER_NAME_MAX + 9;

/*
******************************** Prototypes *********************************
*/
static int Net_Join_Dialog();
static void Clear_Game_List(ListClass* gamelist);
static void Clear_Player_List(ListClass* playerlist);
static bool Request_To_Join(const char* playername, int join_index,
                            ListClass* playerlist, HousesType house, int color);
static void Send_Join_Queries(int curgame, int gamenow, int playernow);
static JoinEventType Get_Join_Responses(JoinStateType* joinstate,
                                        ListClass* gamelist,
                                        ColorListClass* playerlist,
                                        int join_index);
static int Net_New_Dialog();
static JoinEventType Get_NewGame_Responses(ColorListClass* playerlist);
static int Net_Fake_New_Dialog();
static int Net_Fake_Join_Dialog();

/***********************************************************************************************
 * Init_Network -- initializes network stuff
 **
 *                                                                         						  *
 * INPUT: * none.
 **
 *                                                                         						  *
 * OUTPUT: * true = Initialization OK, false = error
 **
 *                                                                         						  *
 * WARNINGS: * none.
 **
 *                                                                         						  *
 * HISTORY: * 02/14/1995 BR : Created. *
 *=============================================================================================*/
bool Init_Network() {
  NetNumType net;
  NetNodeType node;

  /*------------------------------------------------------------------------
  This call allocates all necessary queue buffers, allocates Real-mode
  memory, and commands IPX to start listening on the Global Channel.
  ------------------------------------------------------------------------*/
  if (!Ipx.Init()) {
    return false;
  }

  /*------------------------------------------------------------------------
  Allocate our "meta-packet" buffer
  ------------------------------------------------------------------------*/
  if (MetaPacket.empty()) {
    MetaPacket.resize(sizeof(EventClass) * MAX_EVENTS);
  }

  /*------------------------------------------------------------------------
  Set up the IPX manager to cross a bridge
  ------------------------------------------------------------------------*/
  if ((!(GameToPlay == GAME_INTERNET)) && IsBridge) {
    BridgeNet.Get_Address(net, node);
    Ipx.Set_Bridge(net);
  }

  return true;

} /* end of Init_Network */

/***********************************************************************************************
 * Shutdown_Network -- shuts down network stuff
 **
 *                                                                         						  *
 * INPUT: * none.
 **
 *                                                                         						  *
 * OUTPUT: * none.
 **
 *                                                                         						  *
 * WARNINGS: * none.
 **
 *                                                                         						  *
 * HISTORY: * 02/14/1995 BR : Created. *
 *=============================================================================================*/
void Shutdown_Network() {
//
// Note: The thought behind this section of code was that if the program
// terminates early, without an EventClass::EXIT event, it still needs to
// tell the other systems that it's gone, so it would send a SIGN_OFF packet.
// BUT, this causes a sync bug if the systems are running slow and this system
// is running ahead of the others; it will send the NET_SIGN_OFF >>before<<
// the other system execute their EventClass::EXIT event, and the other systems
// will kill the connection at some random Frame # & turn my stuff over to
// the computer possibly at different times.
// BRR, 10/29/96
//

  /*------------------------------------------------------------------------
  Delete our "meta-packet"
  ------------------------------------------------------------------------*/
MetaPacket.clear();
MetaPacket.shrink_to_fit();

/*------------------------------------------------------------------------
If I was in a game, I'm not now, so clear the game name
------------------------------------------------------------------------*/
base::At(MPlayerGameName, 0) = 0;
}

/***********************************************************************************************
 * Process_Global_Packet -- responds to remote queries
 **
 *                                                                         						  *
 * The only commands from other systems this routine responds to are
 *NET_QUERY_GAME				  * and NET_QUERY_PLAYER.  The
 *other commands are too context-specific to be able
 ** to handle here, such as joining the game or signing off; but this routine
 *handles			  * the majority of the program's needs.
 **
 *                                                                         						  *
 * INPUT: * packet		ptr to packet to process
 ** address		source address of sender
 **
 *                                                                         						  *
 * OUTPUT: * true = packet was processed, false = wasn't
 **
 *                                                                         						  *
 * WARNINGS: * MPlayerName & MPlayerGameName must have been filled in before
 *this function				  * can be called.
 **
 *                                                                         						  *
 * HISTORY: * 02/15/1995 BR : Created. *
 *=============================================================================================*/
bool Process_Global_Packet(GlobalPacketType* packet, IPXAddressClass* address) {
  GlobalPacketType mypacket;

  /*
  ---------------- Another system asking what game this is -----------------
  */
  if (packet->Command == NET_QUERY_GAME && !NetStealth) {
    /*.....................................................................
    If the game is closed, let every player respond, and let the sender of
    the query sort it all out.  This way, if the game's host exits the game,
    the game still shows up on other players' dialogs.
    If the game is open, only the game owner may respond.
    .....................................................................*/
    if (!std::string_view(MPlayerName).empty() &&
        !std::string_view(MPlayerGameName).empty() &&
        (!NetOpen ||
         (NetOpen && (std::string_view(MPlayerName) == MPlayerGameName)))) {
      base::FillBytes(base::ObjectBytes(*packet), 0, sizeof(GlobalPacketType));

      mypacket.Command = NET_ANSWER_GAME;
      port::SafeCopy(mypacket.Name, MPlayerGameName);
#ifdef PATCH
      if (IsV107) {
        mypacket.GameInfo.Version = 1;
      } else {
        mypacket.GameInfo.Version = 2;
      }
#else
      mypacket.GameInfo.Version = Version_Number();
#endif
      mypacket.GameInfo.IsOpen = NetOpen;

      Ipx.Send_Global_Message(base::ObjectBytes(mypacket),
                              sizeof(GlobalPacketType), 1, address);
    }
    return true;
  }
  /*
      ----------------- Another system asking what player I am -----------------
      */
  if (packet->Command == NET_QUERY_PLAYER &&
      (std::string_view(packet->Name) == MPlayerGameName) &&
      !std::string_view(MPlayerGameName).empty() && !NetStealth) {
    base::FillBytes(base::ObjectBytes(*packet), 0, sizeof(GlobalPacketType));

    mypacket.Command = NET_ANSWER_PLAYER;
    port::SafeCopy(mypacket.Name, MPlayerName);
    mypacket.PlayerInfo.House = MPlayerHouse;
    mypacket.PlayerInfo.Color = static_cast<unsigned int>(MPlayerColorIdx);
    mypacket.PlayerInfo.NameCRC = Compute_Name_CRC(MPlayerGameName);

    Ipx.Send_Global_Message(base::ObjectBytes(mypacket),
                            sizeof(GlobalPacketType), 1, address);
    return true;
  }
  return false;
}

/***********************************************************************************************
 * Destroy_Connection -- destroys the given connection
 **
 *                                                                         						  *
 * Call this routine when a connection goes bad, or another player signs off.
 **
 *                                                                         						  *
 * INPUT: * id			connection ID to destroy
 ** error		0 = user signed off; 1 = connection error; otherwise, no
 *error is shown.		  *
 *                                                                         						  *
 * OUTPUT: * none.
 **
 *                                                                         						  *
 * WARNINGS: * none.
 **
 *                                                                         						  *
 * HISTORY: * 04/22/1995 BR : Created. *
 *=============================================================================================*/
void Destroy_Connection(int id, int error) {
  char txt[80];

  /*------------------------------------------------------------------------
  Create a message to display to the user
  ------------------------------------------------------------------------*/
  base::At(txt, 0) = '\0';
  if (error == 1) {
    Format_Runtime_Text(txt, sizeof(txt), Text_String(TXT_CONNECTION_LOST),
                        Ipx.Connection_Name(id));
  } else if (error == 0) {
    Format_Runtime_Text(txt, sizeof(txt), Text_String(TXT_LEFT_GAME),
                        Ipx.Connection_Name(id));
  }

  if (!std::string_view(txt).empty()) {
    Messages.Add_Message(
        txt,
        base::At(MPlayerTColors, static_cast<int>(MPlayerID_To_ColorIndex(
                                     static_cast<unsigned char>(id)))),
        TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW, 600, 0, 0);
    Map.Flag_To_Redraw(false);
  }

  /*------------------------------------------------------------------------
  Delete the IPX connection, shift the MPlayerID's & MPlayerHouses' back one.
  ------------------------------------------------------------------------*/
  Ipx.Delete_Connection(id);

  for (int i = 0; i < MPlayerCount; i++) {
    if (base::At(MPlayerID, i) == static_cast<unsigned char>(id)) {
      /*..................................................................
      Turn the player's house over to the computer's AI
      ..................................................................*/
      const HousesType house = base::At(MPlayerHouses, i);
      HouseClass* housep = HouseClass::As_Pointer(house);
      housep->IsHuman = false;
      housep->IsStarted = true;

      /*..................................................................
      Move arrays back by one
      ..................................................................*/
      for (int j = i; j < MPlayerCount - 1; j++) {
        base::At(MPlayerID, j) = base::At(MPlayerID, j + 1);
        base::At(MPlayerHouses, j) = base::At(MPlayerHouses, j + 1);
        port::SafeCopy(base::At(MPlayerNames, j),
                       base::At(MPlayerNames, j + 1));
        base::At(TheirProcessTime, j) = base::At(TheirProcessTime, j + 1);
      }
    }
  }

  MPlayerCount--;

  /*------------------------------------------------------------------------
  If we're the last player left, tell the user.
  ------------------------------------------------------------------------*/
  if (MPlayerCount == 1) {
    absl::SNPrintF(txt, sizeof(txt), "%s", Text_String(TXT_JUST_YOU_AND_ME));
    Messages.Add_Message(
        txt,
        base::At(MPlayerTColors, static_cast<int>(MPlayerID_To_ColorIndex(
                                     static_cast<unsigned char>(id)))),
        TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW, 600, 0, 0);
    Map.Flag_To_Redraw(false);
  }

} /* end of Destroy_Connection */

/***********************************************************************************************
 * Remote_Connect -- handles connecting this user to others
 **
 *                                                                         						  *
 * INPUT: * none.
 **
 *                                                                         						  *
 * OUTPUT: * true = connections established; false = not
 **
 *                                                                         						  *
 * WARNINGS: * none.
 **
 *                                                                         						  *
 * HISTORY: * 02/14/1995 BR : Created. *
 *=============================================================================================*/
bool Remote_Connect() {

  /*------------------------------------------------------------------------
  Init network timing parameters; these values should work for both a "real"
  network, and a simulated modem network (ie Kali)
  ------------------------------------------------------------------------*/
  Ipx.Set_Timing(30,    // retry 2 times per second
                 -1,    // ignore max retries
                 600);  // give up after 10 seconds

  /*------------------------------------------------------------------------
  Save the original value of the NetStealth flag, so we can turn stealth
  off for now (during this portion of the dialogs, we must show ourselves)
  ------------------------------------------------------------------------*/
  const bool stealth = NetStealth;  // original state of NetStealth flag
  NetStealth = false;

  /*------------------------------------------------------------------------
  Init my game name to 0-length, since I haven't joined any game yet.
  ------------------------------------------------------------------------*/
  base::At(MPlayerGameName, 0) = 0;

  /*------------------------------------------------------------------------
  The game is now "open" for joining.  Close it as soon as we exit this
  routine.
  ------------------------------------------------------------------------*/
  NetOpen = true;

  /*------------------------------------------------------------------------
  Read the default values from the INI file
  ------------------------------------------------------------------------*/
  Read_MultiPlayer_Settings();

  /*------------------------------------------------------------------------
  Keep looping until something useful happens.
  ------------------------------------------------------------------------*/
  while (true) {
    /*---------------------------------------------------------------------
    Pop up the network Join/New dialog
    ---------------------------------------------------------------------*/
    const int rc = Net_Join_Dialog();

    /*---------------------------------------------------------------------
    -1 = user selected Cancel
    ---------------------------------------------------------------------*/
    if (rc == -1) {
      NetStealth = stealth;
      NetOpen = false;
      return false;
    }
    /*---------------------------------------------------------------------
          0 = user has joined an existing game; save values & return
          ---------------------------------------------------------------------*/
    if (rc == 0) {
      Write_MultiPlayer_Settings();
      NetStealth = stealth;
      NetOpen = false;

      return true;
    }
    /*---------------------------------------------------------------------
          1 = user requests New Network Game
          ---------------------------------------------------------------------*/
    if ((rc == 1) && Net_New_Dialog())
    /*..................................................................
    Pop up the New Network Game dialog; if user selects OK, return
    'true'; otherwise, return to the Join Dialog.
    ..................................................................*/
    {
      Write_MultiPlayer_Settings();
      NetOpen = false;
      NetStealth = stealth;
      NetOpen = false;

      return true;
    }
  }
}

/***********************************************************************************************
 * Remote_Connect -- handles connecting this host to the server in an internet
 *game 			    *
 *                                                                         						    *
 * INPUT: * none.
 **
 *                                                                         						    *
 * OUTPUT: * true = connections established; false = not
 **
 *                                                                         						    *
 * WARNINGS: * none.
 **
 *                                                                         						    *
 * HISTORY: * 02/14/1995 BR : Created. *
 *=============================================================================================*/
bool Server_Remote_Connect() {

  /*------------------------------------------------------------------------
  Init network timing parameters; these values should work for both a "real"
  network, and a simulated modem network (ie Kali)
  ------------------------------------------------------------------------*/
  Ipx.Set_Timing(30,    // retry 2 times per second
                 -1,    // ignore max retries
                 600);  // give up after 10 seconds

  /*------------------------------------------------------------------------
  Save the original value of the NetStealth flag, so we can turn stealth
  off for now (during this portion of the dialogs, we must show ourselves)
  ------------------------------------------------------------------------*/
  const bool stealth = NetStealth;  // original state of NetStealth flag
  NetStealth = false;

  /*------------------------------------------------------------------------
  The game is now "open" for joining.  Close it as soon as we exit this
  routine.
  ------------------------------------------------------------------------*/
  NetOpen = true;

  /*------------------------------------------------------------------------
  Read the default values from the INI file
  ------------------------------------------------------------------------*/
  Read_MultiPlayer_Settings();

  if (!Net_Fake_New_Dialog()) {
    Write_MultiPlayer_Settings();
    return false;
  }

  NetOpen = false;
  NetStealth = stealth;
  Write_MultiPlayer_Settings();
  return true;
}

/***********************************************************************************************
 * Client_Remote_Connect -- handles connecting this client to the server in an
 *internet game   *
 *                                                                         						    *
 * INPUT: * none.
 **
 *                                                                         						    *
 * OUTPUT: * true = connections established; false = not
 **
 *                                                                         						    *
 * WARNINGS: * none.
 **
 *                                                                         						    *
 * HISTORY: * 02/14/1995 ST : Created. *
 *=============================================================================================*/
bool Client_Remote_Connect() {

  /*------------------------------------------------------------------------
  Init network timing parameters; these values should work for both a "real"
  network, and a simulated modem network (ie Kali)
  ------------------------------------------------------------------------*/
  Ipx.Set_Timing(30,    // retry 2 times per second
                 -1,    // ignore max retries
                 600);  // give up after 10 seconds

  /*------------------------------------------------------------------------
  Save the original value of the NetStealth flag, so we can turn stealth
  off for now (during this portion of the dialogs, we must show ourselves)
  ------------------------------------------------------------------------*/
  const bool stealth = NetStealth;  // original state of NetStealth flag
  NetStealth = false;

  /*------------------------------------------------------------------------
  The game is now "open" for joining.  Close it as soon as we exit this
  routine.
  ------------------------------------------------------------------------*/
  NetOpen = true;

  /*------------------------------------------------------------------------
  Read the default values from the INI file
  ------------------------------------------------------------------------*/
  Read_MultiPlayer_Settings();

  /*---------------------------------------------------------------------
  Pop up the network Join/New dialog
  ---------------------------------------------------------------------*/
  const int rc = Net_Fake_Join_Dialog();
  Write_MultiPlayer_Settings();

  NetStealth = stealth;
  NetOpen = false;

  return rc != -1;
}

/***********************************************************************************************
 * Net_Join_Dialog -- lets user join an existing game or start a new one
 **
 *                                                                         						  *
 * This dialog displays an edit field for the player's name, and a list of all
 *non-stealth-	  * mode games.  Clicking once on a game name displays a list of
 *who's in that game.  Clicking  * "New" takes the user to the Net_New dialog,
 *where he waits for other users to join his		  * game.  All other
 *input is done through this dialog.
 **
 *                                                                         						  *
 * The dialog has several "states":
 **
 *                                                                         						  *
 * 1) Initially, it waits for the user to fill in his/her name and then to
 *select Join or New; * if New is selected, this dialog is exited.
 **
 *                                                                         						  *
 *	2) If Join is selected, the Join & New buttons are removed, but the
 *Cancel button remains.  * The join request is transmitted to the game's owner,
 *and the message "Waiting for		  * Confirmation" is displayed, until a
 *confirmation or denial is received from the game's	  * owner.  The user may
 *click Cancel at this point to cancel the join request.
 ** (Once Join is selected, the name editing field is disabled, and becomes a
 *display-only	  * field.  If cancel is selected, it reappears as an edit
 *field.) The user can still click  * around & see who's in which games.
 **
 *                                                                         						  *
 *	3) If the join request is denied, the dialog re-initializes to its
 *pre-join state; the		  * Join & New buttons reappear, & the Name
 *field is available again.
 **
 *                                                                         						  *
 * 4) If join confirmation is obtained, the message just changes to "Confirmed.
 *Waiting for	  * Entry Signal." or some such nonsense.  The user can still
 *click around & see who's		  * in which games.
 **
 *                                                                         						  *
 * Any game running in Stealth mode won't show up on this dialog.
 **
 *                                                                         						  *
 *    ┌───────────────────────────────────────────────────┐
 ** │                 Network Games                     │
 ** │                                                   │
 ** │              Your Name: ____________              │
 ** │                  House: [GDI] [NOD]               │
 ** │          Desired Color: [ ][ ][ ][ ]              │
 ** │                                                   │
 ** │            Games                 Players          │
 ** │ ┌──────────────────────┬─┐ ┌──────────────────┬─┐ │
 ** │ │(Bill's Game         )│↑│ │ Peter Parker GDI │↑│ │
 ** │ │ Peter Parker's Game  ├─┤ │ Mary Jane    GDI ├─┤ │
 ** │ │(Magnum PI's Game    )│ │ │ JJ Jameson   NOD │ │ │
 ** │ │                      ├─┤ │                  ├─┤ │
 ** │ │                      │↓│ │                  │↓│ │
 ** │ └──────────────────────┴─┘ └──────────────────┴─┘ │
 ** │           Scenario: Big Long Description          │
 ** │                Starting Credits: xxxx             │
 ** │          Count: ---          Level: ---           │
 ** │          Bases: ON          Crates: ON            │
 ** │       Tiberium: ON      AI Players: ON            │
 ** │                                                   │
 ** │            [Join]  [Cancel]    [New]              │
 ** │  ┌─────────────────────────────────────────────┐  │
 ** │  │                                             │  │
 ** │  │                                             │  │
 ** │  └─────────────────────────────────────────────┘  │
 ** │                  [Send Message]                   │
 ** └───────────────────────────────────────────────────┘
 **
 *                                                                         						  *
 * INPUT: * none.
 **
 *                                                                         						  *
 * OUTPUT: * -1 = cancel, 0 = OK, 1 = New net game requested
 **
 *                                                                         						  *
 * WARNINGS: * none.
 **
 *                                                                         						  *
 * HISTORY: * 02/14/1995 BR : Created. *
 *=============================================================================================*/
static int Net_Join_Dialog() {
  const int factor = SeenBuff.Get_Width() == 320 ? 1 : 2;
  /*........................................................................
  Dialog & button dimensions
  ........................................................................*/
  /* ###Change collision detected! C:\PROJECTS\CODE\NETDLG.CPP... */
  const int d_dialog_w = 287 * factor;                       // dialog width
  const int d_dialog_h = 198 * factor;                       // dialog height
  const int d_dialog_x = ((320 * factor) - d_dialog_w) / 2;  // dialog x-coord
  const int d_dialog_y = ((200 * factor) - d_dialog_h) / 2;  // centered y-coord
  const int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);     // center x-coord

  const int d_txt6_h = (6 * factor) + 1;  // ht of 6-pt text
  const int d_margin1 = 5 * factor;       // large margin
  const int d_margin2 = 2 * factor;       // small margin

  const int d_name_w = 70 * factor;
  const int d_name_h = 9 * factor;
  const int d_name_x = d_dialog_cx - (10 * factor);
  const int d_name_y = d_dialog_y + d_margin1 + d_txt6_h + d_txt6_h;

  const int d_gdi_w = 30 * factor;
  const int d_gdi_h = 9 * factor;
  const int d_gdi_x = d_dialog_cx - (10 * factor);
  const int d_gdi_y = d_name_y + d_name_h + d_margin2;

  const int d_nod_w = 30 * factor;
  const int d_nod_h = 9 * factor;
  const int d_nod_x = d_gdi_x + d_gdi_w;
  const int d_nod_y = d_name_y + d_name_h + d_margin2;

  const int d_color_w = 10 * factor;
  const int d_color_h = 9 * factor;
  const int d_color_y = d_nod_y + d_nod_h + d_margin2;

  const int d_gamelist_w = 160 * factor;
  const int d_gamelist_h = 27 * factor;
  const int d_gamelist_x = d_dialog_x + d_margin1;
  const int d_gamelist_y = d_color_y + d_color_h + d_margin1 + d_txt6_h;

  const int d_playerlist_w = 106 * factor;
  const int d_playerlist_h = 27 * factor;
  const int d_playerlist_x =
      d_dialog_x + d_dialog_w - d_margin1 - d_playerlist_w;
  const int d_playerlist_y = d_color_y + d_color_h + d_margin1 + d_txt6_h;

  const int d_msg1_y = d_gamelist_y + d_gamelist_h + d_margin1;
  const int d_msg2_y = d_msg1_y + d_txt6_h;
  const int d_msg3_y = d_msg2_y + d_txt6_h;
  const int d_msg4_y = d_msg3_y + d_txt6_h;
  const int d_msg5_y = d_msg4_y + d_txt6_h;

  const int d_join_w = 40 * factor;
  const int d_join_h = 9 * factor;
  const int d_join_x = d_dialog_x + (d_dialog_w / 6) - (d_join_w / 2);
  const int d_join_y = d_msg5_y + d_txt6_h + d_margin1;

#if (defined(GERMAN) || defined(FRENCH))
  int d_cancel_w = 50 * factor;
#else
  const int d_cancel_w = 40 * factor;
#endif
  const int d_cancel_h = 9 * factor;
  const int d_cancel_x = d_dialog_cx - (d_cancel_w / 2);
  const int d_cancel_y = d_msg5_y + d_txt6_h + d_margin1;

  const int d_new_w = 40 * factor;
  const int d_new_h = 9 * factor;
  const int d_new_x = d_dialog_x + (d_dialog_w * 5 / 6) - (d_new_w / 2);
  const int d_new_y = d_msg5_y + d_txt6_h + d_margin1;

  const int d_message_w = d_dialog_w - (d_margin1 * 2);
  const int d_message_h = 34 * factor;
  const int d_message_x = d_dialog_x + d_margin1;
  const int d_message_y = d_cancel_y + d_cancel_h + d_margin1;

  const int d_send_w = 80 * factor;
  const int d_send_h = 9 * factor;
  const int d_send_x = d_dialog_cx - (d_send_w / 2);
  const int d_send_y = d_message_y + d_message_h + d_margin2;

  /*........................................................................
  Button Enumerations
  ........................................................................*/
  constexpr int kButtonName = 100;
  constexpr int kButtonGdi = 101;
  constexpr int kButtonNod = 102;
  constexpr int kButtonGamelist = 103;
  constexpr int kButtonPlayerlist = 104;
  constexpr int kButtonJoin = 105;
  constexpr int kButtonCancel = 106;
  constexpr int kButtonNew = 107;
  constexpr int kButtonSend = 108;

  /*........................................................................
  Redraw values: in order from "top" to "bottom" layer of the dialog
  ........................................................................*/
  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_MESSAGE = 1,
    REDRAW_COLORS = 2,
    REDRAW_BUTTONS = 3,
    REDRAW_BACKGROUND = 4,
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;

  /*........................................................................
  Dialog variables
  ........................................................................*/
  RedrawType display = REDRAW_ALL;  // redraw level
  bool process = true;              // process while true
  KeyNumType input = KN_NONE;
  const int cbox_x[] = {d_gdi_x,
                        d_gdi_x + d_color_w,
                        d_gdi_x + (d_color_w * 2),
                        d_gdi_x + (d_color_w * 3),
                        d_gdi_x + (d_color_w * 4),
                        d_gdi_x + (d_color_w * 5)};

  JoinStateType joinstate = JOIN_NOTHING;  // current "state" of this dialog
  char namebuf[MPLAYER_NAME_MAX] = {0};    // buffer for player's name
  const int tabs[] = {77 * factor};        // tabs for player list box
  int game_index = -1;                     // index of currently-selected game
  int join_index = -1;                     // index of game we're joining
  int rc = 0;                              // -1 = user cancelled, 1 = New
  int i = 0;
  int j = 0;  // loop counter
  char txt[80];
  const char* p = nullptr;
  int parms_received = 0;  // 1 = game options received
  int found = 0;

  unsigned char tmp_id[MAX_PLAYERS] =
      {};                    // temp storage for sorting player ID's
  int min_index = 0;         // for sorting player ID's
  unsigned char min_id = 0;  // for sorting player ID's
  unsigned char id = 0;      // connection ID
  char item[kGameListItemSize];
  int64_t starttime = 0;

  NodeNameType* who = nullptr;

  int message_length = 0;
  int sent_so_far = 0;
  uint16_t magic_number = 0;
  uint16_t crc = 0;

  std::span<const std::byte> up_button;
  std::span<const std::byte> down_button;

  if (InMainLoop) {
    up_button = Hires_Retrieve("BTN-UP.SHP");
    down_button = Hires_Retrieve("BTN-DN.SHP");
  } else {
    up_button = Hires_Retrieve("BTN-UP2.SHP");
    down_button = Hires_Retrieve("BTN-DN2.SHP");
  }

  /*........................................................................
  Buttons
  ........................................................................*/
  GadgetClass* commands = nullptr;  // button list

  EditClass name_edt(kButtonName, namebuf, MPLAYER_NAME_MAX,
                     TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, d_name_x,
                     d_name_y, d_name_w, d_name_h, EditClass::ALPHANUMERIC);

  TextButtonClass gdibtn(
      kButtonGdi, TXT_G_D_I,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, d_gdi_x,
      d_gdi_y, d_gdi_w, d_gdi_h);

  TextButtonClass nodbtn(
      kButtonNod, TXT_N_O_D,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, d_nod_x,
      d_nod_y, d_nod_w, d_nod_h);

  ListClass gamelist(
      kButtonGamelist, d_gamelist_x, d_gamelist_y, d_gamelist_w, d_gamelist_h,
      TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, up_button, down_button);

  ColorListClass playerlist(kButtonPlayerlist, d_playerlist_x, d_playerlist_y,
                            d_playerlist_w, d_playerlist_h,
                            TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                            up_button, down_button);

  TextButtonClass joinbtn(
      kButtonJoin, TXT_JOIN,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
#ifdef FRENCH
      d_join_x, d_join_y);
#else
      d_join_x, d_join_y, d_join_w, d_join_h);
#endif

  TextButtonClass cancelbtn(
      kButtonCancel, TXT_CANCEL,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
      // #if (GERMAN | FRENCH)
      //		d_cancel_x, d_cancel_y);
      // #else
      d_cancel_x, d_cancel_y, d_cancel_w, d_cancel_h);
  // #endif

  TextButtonClass newbtn(
      kButtonNew, TXT_NEW,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, d_new_x,
      d_new_y, d_new_w, d_new_h);

  TextButtonClass sendbtn(
      kButtonSend, TXT_SEND_MESSAGE,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
      // #if (GERMAN | FRENCH)
      //		d_send_x, d_send_y);
      // #else
      d_send_x, d_send_y, d_send_w, d_send_h);
  // #endif

  playerlist.Set_Tabs(tabs);

  /*
  ----------------------------- Various Inits ------------------------------
  */
  MPlayerColorIdx = MPlayerPrefColor;    // init my preferred color
  port::SafeCopy(namebuf, MPlayerName);  // set my name
  name_edt.Set_Text(namebuf, MPLAYER_NAME_MAX);
  name_edt.Set_Color(base::At(MPlayerTColors, MPlayerColorIdx));

  playerlist.Set_Selected_Style(ColorListClass::SELECT_NONE);

  if (MPlayerHouse == HOUSE_GOOD) {
    gdibtn.Turn_On();
  } else {
    nodbtn.Turn_On();
  }

  Fancy_Text_Print("", 0, 0, kCcGreen, kTBlack,
                   TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

  Messages.Init(d_message_x + 2, d_message_y + 2, 4, MAX_MESSAGE_LENGTH,
                d_txt6_h);

  /*
  --------------------------- Send network query ---------------------------
  */
  Send_Join_Queries(game_index, 1, 0);

  Load_Title_Page(true);
  Set_Palette(Palette);

/*
---------------------------- Init Mono Output ----------------------------
*/
#if (SHOW_MONO)
  Ipx.Configure_Debug(-1, sizeof(GlobalHeaderType), sizeof(NetCommandType),
                      GlobalPacketNames, 11);
  Ipx.Mono_Debug_Print(-1, 1);
#endif
  while (Get_Mouse_State() > 0) {
    Show_Mouse();
  }

  /*
  ---------------------------- Processing loop -----------------------------
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

#if (SHOW_MONO)
    Ipx.Mono_Debug_Print(-1, 0);
#endif
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
        Set_Palette(Palette);

        Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);

        /*...............................................................
        Dialog & Field labels
        ...............................................................*/
        Draw_Caption(TXT_JOIN_NETWORK_GAME, d_dialog_x, d_dialog_y, d_dialog_w);

        Fancy_Text_Print(
            TXT_YOUR_NAME, d_name_x - 5, d_name_y + 1, kCcGreen, kTBlack,
            TPF_RIGHT | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        Fancy_Text_Print(
            TXT_SIDE_COLON, d_gdi_x - 5, d_gdi_y + 1, kCcGreen, kTBlack,
            TPF_RIGHT | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        Fancy_Text_Print(
            TXT_COLOR_COLON, base::At(cbox_x, 0) - 5, d_color_y + 1, kCcGreen,
            kTBlack,
            TPF_RIGHT | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        Fancy_Text_Print(
            TXT_GAMES, d_gamelist_x + (d_gamelist_w / 2),
            d_gamelist_y - d_txt6_h, kCcGreen, kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        Fancy_Text_Print(
            TXT_PLAYERS, d_playerlist_x + (d_playerlist_w / 2),
            d_playerlist_y - d_txt6_h, kCcGreen, kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        /*...............................................................
        Join-state-specific labels:
        ...............................................................*/
        if (joinstate > JOIN_NOTHING) {
          Fancy_Text_Print(namebuf, d_name_x, d_name_y + 1, kCcGreen, kTBlack,
                           TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
          if (MPlayerHouse == HOUSE_GOOD) {
            Fancy_Text_Print(TXT_G_D_I, d_gdi_x, d_gdi_y + 1, kCcGreen, kTBlack,
                             TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
          } else {
            Fancy_Text_Print(TXT_N_O_D, d_gdi_x, d_gdi_y + 1, kCcGreen, kTBlack,
                             TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
          }
        }

        /*
        .................... Rebuild the button list ....................
        */
        cancelbtn.Zap();
        gamelist.Zap();
        playerlist.Zap();
        gdibtn.Zap();
        nodbtn.Zap();
        name_edt.Zap();
        joinbtn.Zap();
        newbtn.Zap();
        sendbtn.Zap();

        commands = &cancelbtn;
        gamelist.Add_Tail(*commands);
        playerlist.Add_Tail(*commands);
        /*...............................................................
        Only add the name edit field, the House, Join & New buttons if
        we're doing nothing, or we've just been rejected.
        ...............................................................*/
        if (joinstate <= JOIN_NOTHING) {
          gdibtn.Add_Tail(*commands);
          nodbtn.Add_Tail(*commands);
          name_edt.Add_Tail(*commands);
          joinbtn.Add_Tail(*commands);
          newbtn.Add_Tail(*commands);
        }
        if (joinstate == JOIN_CONFIRMED) {
          sendbtn.Add_Tail(*commands);
        }
      }
      /*
      .......................... Redraw buttons ..........................
      */
      if (display >= REDRAW_BUTTONS) {
        commands->Draw_All();
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
              static_cast<unsigned char>(base::At(MPlayerGColors, i)));

          if (i == MPlayerColorIdx) {
            Draw_Box(base::At(cbox_x, i), d_color_y, d_color_w, d_color_h,
                     BOXSTYLE_GREEN_DOWN, false);
          } else {
            Draw_Box(base::At(cbox_x, i), d_color_y, d_color_w, d_color_h,
                     BOXSTYLE_GREEN_RAISED, false);
          }
        }
      }

      /*..................................................................
      Draw the message:
      - Erase an old message first
      - If we're in a game, print the game options (if they've been
        received)
      - If we've been rejected from a game, print that message
      ..................................................................*/
      if (display >= REDRAW_MESSAGE) {
        Draw_Box(d_message_x, d_message_y, d_message_w, d_message_h,
                 BOXSTYLE_GREEN_BORDER, true);
        Messages.Draw();

        LogicPage->Fill_Rect(d_dialog_x + 2, d_msg1_y,
                             d_dialog_x + d_dialog_w - 4, d_msg5_y + d_txt6_h,
                             kBlack);

        if (joinstate == JOIN_CONFIRMED && parms_received) {
          /*............................................................
          Scenario title
          ............................................................*/
          p = Text_String(TXT_SCENARIO_COLON);
          if (ScenarioIdx != -1) {
            absl::SNPrintF(txt, sizeof(txt), "%s %s", p,
                           MPlayerScenarios[ScenarioIdx]);

            Fancy_Text_Print(
                txt, d_dialog_cx, d_msg1_y, kCcGreen, kTBlack,
                TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW | TPF_CENTER);
          } else {
            absl::SNPrintF(txt, sizeof(txt), "%s %s", p,
                           Text_String(TXT_NOT_FOUND));

            Fancy_Text_Print(
                txt, d_dialog_cx, d_msg1_y, kCcNodColor, kTBlack,
                TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW | TPF_CENTER);
          }

          /*............................................................
          # of credits
          ............................................................*/
          p = Text_String(TXT_START_CREDITS_COLON);
          absl::SNPrintF(txt, sizeof(txt), "%s %d", p, MPlayerCredits);
          Fancy_Text_Print(
              txt, d_dialog_cx, d_msg2_y, kCcGreen, kTBlack,
              TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW | TPF_CENTER);

          /*............................................................
          Count & Level values
          ............................................................*/
          p = Text_String(TXT_COUNT);
          absl::SNPrintF(txt, sizeof(txt), "%s %d", p, MPlayerUnitCount);
          Fancy_Text_Print(
              txt, d_dialog_x + (d_dialog_w / 4) - String_Pixel_Width(p),
              d_msg3_y, kCcGreen, kTBlack,
              TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

          p = Text_String(TXT_LEVEL);
          if (BuildLevel <= MPLAYER_BUILD_LEVEL_MAX) {
            absl::SNPrintF(txt, sizeof(txt), "%s %d", p, BuildLevel);
          } else {
            absl::SNPrintF(txt, sizeof(txt), "%s **", p);
          }
          Fancy_Text_Print(txt,
                           d_dialog_x + d_dialog_w - (d_dialog_w / 4) -
                               String_Pixel_Width(p),
                           d_msg3_y, kCcGreen, kTBlack,
                           TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

          /*............................................................
          Bases
          ............................................................*/
          p = Text_String(TXT_BASES_COLON);
          if (MPlayerBases) {
            absl::SNPrintF(txt, sizeof(txt), "%s %s", p, Text_String(TXT_ON));
          } else {
            absl::SNPrintF(txt, sizeof(txt), "%s %s", p, Text_String(TXT_OFF));
          }
          Fancy_Text_Print(
              txt, d_dialog_x + (d_dialog_w / 4) - String_Pixel_Width(p),
              d_msg4_y, kCcGreen, kTBlack,
              TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

          /*............................................................
          Tiberium
          ............................................................*/
          p = Text_String(TXT_TIBERIUM_COLON);
          if (MPlayerTiberium) {
            absl::SNPrintF(txt, sizeof(txt), "%s %s", p, Text_String(TXT_ON));
          } else {
            absl::SNPrintF(txt, sizeof(txt), "%s %s", p, Text_String(TXT_OFF));
          }

          Fancy_Text_Print(
              txt, d_dialog_x + (d_dialog_w / 4) - String_Pixel_Width(p),
              d_msg5_y, kCcGreen, kTBlack,
              TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

          /*............................................................
          Goody boxes
          ............................................................*/
          p = Text_String(TXT_CRATES_COLON);
          if (MPlayerGoodies) {
            absl::SNPrintF(txt, sizeof(txt), "%s %s", p, Text_String(TXT_ON));
          } else {
            absl::SNPrintF(txt, sizeof(txt), "%s %s", p, Text_String(TXT_OFF));
          }

          Fancy_Text_Print(txt,
                           d_dialog_x + d_dialog_w - (d_dialog_w / 4) -
                               String_Pixel_Width(p),
                           d_msg4_y, kCcGreen, kTBlack,
                           TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

          /*............................................................
          Computer AI players
          ............................................................*/
          if (Special.IsCaptureTheFlag) {
            p = Text_String(TXT_CAPTURE_THE_FLAG_COLON);
            absl::SNPrintF(txt, sizeof(txt), "%s %s", p, Text_String(TXT_ON));
          } else {
            p = Text_String(TXT_AI_PLAYERS_COLON);
            if (MPlayerGhosts) {
              absl::SNPrintF(txt, sizeof(txt), "%s %s", p, Text_String(TXT_ON));
            } else {
              absl::SNPrintF(txt, sizeof(txt), "%s %s", p,
                             Text_String(TXT_OFF));
            }
          }
          Fancy_Text_Print(txt,
                           d_dialog_x + d_dialog_w - (d_dialog_w / 4) -
                               String_Pixel_Width(p),
                           d_msg5_y, kCcGreen, kTBlack,
                           TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        } else {
          /*...............................................................
          Rejection notice
          ...............................................................*/
          if (joinstate == JOIN_REJECTED) {
            Fancy_Text_Print(
                TXT_REQUEST_DENIED, d_dialog_cx, d_msg3_y, kCcGreen, kTBlack,
                TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
          }
        }
      }

      Show_Mouse();
      display = REDRAW_NONE;
    }

    /*
    ........................... Get user input ............................
    */
    input = commands->Input();

    /*
    ---------------------------- Process input ----------------------------
    */
    switch (static_cast<int>(input)) {
      /*------------------------------------------------------------------
      User clicks on a color button:
      - If we've joined a game, don't allow a new color selection
      - otherwise, select that color
      ------------------------------------------------------------------*/
      case KN_LMOUSE:
        if (joinstate > JOIN_NOTHING) {
          break;
        }
        if (ActiveKeyboard->MouseQX > base::At(cbox_x, 0) &&
            ActiveKeyboard->MouseQX <
                base::At(cbox_x, MAX_MPLAYER_COLORS - 1) + d_color_w &&
            ActiveKeyboard->MouseQY > d_color_y &&
            ActiveKeyboard->MouseQY < d_color_y + d_color_h) {
          MPlayerPrefColor =
              (ActiveKeyboard->MouseQX - base::At(cbox_x, 0)) / d_color_w;
          MPlayerColorIdx = MPlayerPrefColor;

          name_edt.Set_Color(base::At(MPlayerTColors, MPlayerColorIdx));
          name_edt.Flag_To_Redraw();

          display = REDRAW_COLORS;
        }
        break;

      /*------------------------------------------------------------------
      User clicks on the game list:
      - If we've joined a game, don't allow the selected item to change;
      otherwise:
      - Clear the player list
      - Send an immediate player query
      ------------------------------------------------------------------*/
      case ButtonKey(kButtonGamelist):
        if (joinstate == JOIN_CONFIRMED) {
          gamelist.Set_Selected_Index(game_index);
        } else {
          if (gamelist.Current_Index() != game_index) {
            Clear_Player_List(&playerlist);
            game_index = gamelist.Current_Index();
            Send_Join_Queries(game_index, 0, 1);
          }
        }
        break;

      /*------------------------------------------------------------------
      House Buttons: set the player's desired House
      ------------------------------------------------------------------*/
      case ButtonKey(kButtonGdi):
        MPlayerHouse = HOUSE_GOOD;
        gdibtn.Turn_On();
        nodbtn.Turn_Off();
        break;

      case ButtonKey(kButtonNod):
        MPlayerHouse = HOUSE_BAD;
        gdibtn.Turn_Off();
        nodbtn.Turn_On();
        break;

      /*------------------------------------------------------------------
      JOIN: send a join request packet & switch to waiting-for-confirmation
      mode.  (Request_To_Join fills in MPlayerName with my namebuf.)
      ------------------------------------------------------------------*/
      case ButtonKey(kButtonJoin):
        name_edt.Clear_Focus();
        name_edt.Flag_To_Redraw();

        join_index = gamelist.Current_Index();
        parms_received = 0;
        if (Request_To_Join(namebuf, join_index, &playerlist, MPlayerHouse,
                            MPlayerColorIdx)) {
          joinstate = JOIN_WAIT_CONFIRM;
        } else {
          display = REDRAW_ALL;
        }
        break;

      /*------------------------------------------------------------------
      CANCEL: send a SIGN_OFF
      - If we're part of a game, stay in this dialog; otherwise, exit
      ------------------------------------------------------------------*/
      case KN_ESC:
        if (Messages.Get_Edit_Buf() != nullptr) {
          Messages.Input(input);
          display = REDRAW_MESSAGE;
          break;
        }
        [[fallthrough]];
      case ButtonKey(kButtonCancel):
        base::FillBytes(base::ObjectBytes(GPacket), 0,
                        sizeof(GlobalPacketType));

        GPacket.Command = NET_SIGN_OFF;
        port::SafeCopy(GPacket.Name, MPlayerName);

        /*...............................................................
        If we're joined to a game, make extra sure the other players in
        that game know I'm exiting; send my SIGN_OFF as an ack-required
        packet.  Do not send this packet to myself (index 0).
        ...............................................................*/
        if (joinstate == JOIN_CONFIRMED) {
          //
          // Remove myself from the player list box
          //
          if (playerlist.Count()) {  // added: BRR 6/14/96
            playerlist.Remove_Item(0);
            playerlist.Flag_To_Redraw();
          }

          //
          // Remove myself from the Players list
          //
          if (Players.Count()) {  // added: BRR 6/14/96
            who = Players[0];
            Players.Delete(0);
            delete who;
          }

          for (i = 0; i < Players.Count(); i++) {
            Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                    sizeof(GlobalPacketType), 1,
                                    &Players[i]->Address);
            Ipx.Service();
          }
        }

        /*...............................................................
        Now broadcast my SIGN_OFF so other players looking at this game
        know I'm leaving.
        ...............................................................*/
        Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                sizeof(GlobalPacketType), 0, nullptr);
        Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                sizeof(GlobalPacketType), 0, nullptr);

        if (IsBridge) {
          Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                  sizeof(GlobalPacketType), 0, &BridgeNet);
          Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                  sizeof(GlobalPacketType), 0, &BridgeNet);
        }

        while (Ipx.Global_Num_Send() > 0 && Ipx.Service() != 0) {
        }

        if (joinstate != JOIN_CONFIRMED) {
          process = false;
          rc = -1;
        } else {
          base::At(MPlayerGameName, 0) = 0;
          joinstate = JOIN_NOTHING;
          display = REDRAW_ALL;
        }
        break;

      /*------------------------------------------------------------------
      NEW: bail out with return code 1
      ------------------------------------------------------------------*/
      case ButtonKey(kButtonNew):
        /*
        .................. Force user to enter a name ...................
        */
        if (std::string_view(namebuf).empty()) {
          CCMessageBox().Process(TXT_NAME_ERROR);
          display = REDRAW_ALL;
          break;
        }
        /*
        ..................... Ensure name is unique .....................
        */
        found = 0;
        for (i = 0; i < Games.Count(); i++) {
          if (!port::CompareIgnoreCase(Games[i]->Name, namebuf)) {
            found = 1;
            CCMessageBox().Process(TXT_GAMENAME_MUSTBE_UNIQUE);
            display = REDRAW_ALL;
            break;
          }
        }
        if (found) {
          break;
        }
        /*
        .................... Save player & game name ....................
        */
        port::SafeCopy(MPlayerName, namebuf);
        port::SafeCopy(MPlayerGameName, namebuf);

        name_edt.Clear_Focus();
        name_edt.Flag_To_Redraw();

        rc = 1;
        process = false;
        break;

      /*------------------------------------------------------------------
      Default: manage the inter-player messages
      ------------------------------------------------------------------*/
      default:
        /*...............................................................
        F4/SEND/'M' = edit a message
        ...............................................................*/
        if (Messages.Get_Edit_Buf() == nullptr) {
          if ((input == KN_M && joinstate == JOIN_CONFIRMED) ||
              input == ButtonKey(kButtonSend) || input == KN_F4) {
            base::FillBytes(base::ObjectBytes(txt), 0, 80);

            port::SafeCopy(txt, Text_String(TXT_TO_ALL));  // "To All:"

            Messages.Add_Edit(base::At(MPlayerTColors, MPlayerColorIdx),
                              TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW,
                              txt, d_message_w - (70 * factor));

            if (joinstate <= JOIN_NOTHING) {
              name_edt.Clear_Focus();
              name_edt.Flag_To_Redraw();
            }

            display = REDRAW_MESSAGE;

            break;
          }
        } else

          /*...............................................................
          If we're already editing a message and the user clicks on
          'Send', translate our input to a Return so Messages.Input() will
          work properly.
          ...............................................................*/
          if (input == ButtonKey(kButtonSend)) {
            input = KN_RETURN;
          }

        /*...............................................................
        Manage the message system (get rid of old messages)
        ...............................................................*/
        if (Messages.Manage()) {
          display = REDRAW_MESSAGE;
        }

        /*...............................................................
        Service keyboard input for any message being edited.
        ...............................................................*/
        i = Messages.Input(input);

        /*...............................................................
        If 'Input' returned 1, it means refresh the message display.
        ...............................................................*/
        if (i == 1) {
          Messages.Draw();
        } else {
          /*...............................................................
          If 'Input' returned 2, it means redraw the message display.
          ...............................................................*/
          if (i == 2) {
            display = REDRAW_MESSAGE;
          } else {
            /*...............................................................
            If 'input' returned 3, it means send the current message.
            ...............................................................*/
            if (i == 3) {

              sent_so_far = 0;
              magic_number = MESSAGE_HEAD_MAGIC_NUMBER;
              message_length = static_cast<int>(
                  std::string_view(Messages.Get_Edit_Buf()).size());
              crc = static_cast<uint16_t>(
                  CrcEngine::Compute(Messages.Get_Edit_Buf()) & 0xffff);

              while (sent_so_far < message_length) {
                GPacket.Command = NET_MESSAGE;
                port::SafeCopy(GPacket.Name, namebuf);
                port::SafeCopy(std::span(GPacket.Message.Buf)
                                   .first(COMPAT_MESSAGE_LENGTH - 4),
                               std::string_view(Messages.Get_Edit_Buf())
                                   .substr(base::ToSize(sent_so_far)));

                /*
                ** Steve I's stuff for splitting message on word boundries
                */
                int32_t actual_message_size = COMPAT_MESSAGE_LENGTH - 5;

                /* Start at the end of the message and find a space with 10
                 * chars. */
                const auto the_string = std::span(GPacket.Message.Buf);
                while (COMPAT_MESSAGE_LENGTH - 5 - actual_message_size < 10 &&
                       the_string[base::ToSize(actual_message_size)] != ' ') {
                  --actual_message_size;
                }
                if (the_string[base::ToSize(actual_message_size)] == ' ') {
                  /* Now delete the extra characters after the space (they musnt
                   * print) */
                  for (int k = 0;
                       k < COMPAT_MESSAGE_LENGTH - 5 - actual_message_size;
                       k++) {
                    the_string[base::ToSize(k + actual_message_size)] =
                        static_cast<char>(0xff);
                  }
                } else {
                  actual_message_size = COMPAT_MESSAGE_LENGTH - 5;
                }

                base::At(GPacket.Message.Buf, COMPAT_MESSAGE_LENGTH - 5) = 0;
                port::WriteUnaligned(base::ObjectBytes(GPacket.Message.Buf)
                                         .subspan(COMPAT_MESSAGE_LENGTH - 4),
                                     magic_number);
                port::WriteUnaligned(base::ObjectBytes(GPacket.Message.Buf)
                                         .subspan(COMPAT_MESSAGE_LENGTH - 2),
                                     crc);
                GPacket.Message.ID = static_cast<unsigned char>(
                    Build_MPlayerID(MPlayerColorIdx, MPlayerHouse));
                GPacket.Message.NameCRC = Compute_Name_CRC(MPlayerGameName);

                /*..................................................................
                Send the message to every player in our player list.  The local
                system will also receive this message, since it's in the Player
                list.
                ..................................................................*/
                if (joinstate == JOIN_CONFIRMED) {
                  for (i = 1; i < Players.Count(); i++) {
                    Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                            sizeof(GlobalPacketType), 1,
                                            &Players[i]->Address);
                    Ipx.Service();
                  }

                  Format_Runtime_Text(txt, sizeof(txt),
                                      Text_String(TXT_FROM), MPlayerName,
                                      GPacket.Message.Buf);
                  Messages.Add_Message(
                      txt, base::At(MPlayerTColors, MPlayerColorIdx),
                      TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW, 600,
                      magic_number, crc);
                } else {
                  for (i = 0; i < Players.Count(); i++) {
                    Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                            sizeof(GlobalPacketType), 1,
                                            &Players[i]->Address);
                    Ipx.Service();
                  }
                }
                magic_number++;
                sent_so_far += static_cast<int>(actual_message_size);  // COMPAT_MESSAGE_LENGTH-5;
              }
            }
          }
        }
        break;
    }

    /*---------------------------------------------------------------------
    Resend our query packets
    ---------------------------------------------------------------------*/
    Send_Join_Queries(game_index, 0, 0);

    /*---------------------------------------------------------------------
    Process incoming packets
    ---------------------------------------------------------------------*/
    const JoinEventType event =
        Get_Join_Responses(&joinstate, &gamelist, &playerlist, join_index);
    /*.....................................................................
    If we've changed state, redraw everything; if we're starting the game,
    break out of the loop.  If we've just joined, send out a player query
    so I'll get added to the list instantly.
    .....................................................................*/
    if (event == EV_STATE_CHANGE) {
      display = REDRAW_ALL;
      if (joinstate == JOIN_GAME_START) {
        rc = 0;
        process = false;
      } else {
        /*..................................................................
        If we're newly-confirmed, immediately send out a player query
        ..................................................................*/
        if (joinstate == JOIN_CONFIRMED) {
          Clear_Player_List(&playerlist);

          if (MPlayerHouse == HOUSE_GOOD) {
            absl::SNPrintF(item, sizeof(item), "%s\t%s", MPlayerName,
                           Text_String(TXT_G_D_I));
          } else {
            absl::SNPrintF(item, sizeof(item), "%s\t%s", MPlayerName,
                           Text_String(TXT_N_O_D));
          }
          playerlist.Add_Item(item, static_cast<char>(base::At(
                                        MPlayerTColors, MPlayerColorIdx)));

          who = new NodeNameType;
          port::SafeCopy(who->Name, MPlayerName);
          who->Address = IPXAddressClass();
          who->Player.House = MPlayerHouse;
          who->Player.Color = static_cast<unsigned char>(MPlayerColorIdx);
          Players.Add(who);

          Send_Join_Queries(game_index, 0, 1);
        } else {
          /*..................................................................
          If we've been rejected, clear any messages we may have been typing.
          ..................................................................*/
          if (joinstate == JOIN_REJECTED) {
            //
            // Remove myself from the player list box
            //
            if (playerlist.Count()) {  // added: BRR 6/14/96
              playerlist.Remove_Item(0);
              playerlist.Flag_To_Redraw();
            }

            //
            // Remove myself from the Players list
            //
            if (Players.Count()) {
              who = Players[0];
              Players.Delete(0);
              delete who;
            }

            Messages.Init(d_message_x + 2, d_message_y + 2, 4,
                          MAX_MESSAGE_LENGTH, d_txt6_h);
          }
        }
      }
    } else

      /*.....................................................................
      If a new game is detected, and it's the first game on our list,
      automatically send out a player query for that game.
      .....................................................................*/
      if (event == EV_NEW_GAME && gamelist.Count() == 1) {
        gamelist.Set_Selected_Index(0);
        game_index = gamelist.Current_Index();
        Send_Join_Queries(game_index, 0, 1);
      } else

        /*.....................................................................
        If the game options have changed, print them.
        .....................................................................*/
        if (event == EV_GAME_OPTIONS) {
          parms_received = 1;
          display = REDRAW_MESSAGE;
        } else

          /*.....................................................................
          Draw an incoming message
          .....................................................................*/
          if (event == EV_MESSAGE) {
            display = REDRAW_MESSAGE;
          } else

            /*.....................................................................
            A game before the one I've selected is gone, so we have a new index
            now. 'game_index' must be kept set to the currently-selected list
            item, so we send out queries for the currently-selected game.  It's
            therefore imperative that we detect any changes to the game list. If
            we're joined in a game, we must decrement our game_index to keep it
            aligned with the game we're joined to.
            .....................................................................*/
            if (event == EV_GAME_SIGNOFF) {
              if (joinstate == JOIN_CONFIRMED) {
                game_index--;
                join_index--;
                gamelist.Set_Selected_Index(join_index);
              } else {
                gamelist.Flag_To_Redraw();
                Clear_Player_List(&playerlist);
                game_index = gamelist.Current_Index();
                Send_Join_Queries(game_index, 0, 1);
              }
            }

    /*---------------------------------------------------------------------
    Service the Ipx connections
    ---------------------------------------------------------------------*/
    Ipx.Service();

    /*---------------------------------------------------------------------
    Clean out the Game List; if an old entry is found:
    - Remove it
    - Clear the player list
    - Send queries for the new selected game, if there is one
    ---------------------------------------------------------------------*/
    for (i = 0; i < Games.Count(); i++) {
      if (TickCount.Time() - Games[i]->Game.LastTime > 400) {
        Games.Delete(Games[i]);
        gamelist.Remove_Item(i);
        if (i <= game_index) {
          gamelist.Flag_To_Redraw();
          Clear_Player_List(&playerlist);
          game_index = gamelist.Current_Index();
          Send_Join_Queries(game_index, 0, 1);
        }
      }
    }

    /*---------------------------------------------------------------------
    Service the sounds & score; GameActive must be false at this point,
    so Call_Back() doesn't intercept global messages from me!
    ---------------------------------------------------------------------*/
    Call_Back();
  }

  /*------------------------------------------------------------------------
  Establish connections with all other players.
  ------------------------------------------------------------------------*/
  if (rc == 0) {
    /*.....................................................................
    If the other guys are playing a scenario I don't have (sniff), I can't
    play.  Try to bail gracefully.
    .....................................................................*/
    if (ScenarioIdx == -1) {
      CCMessageBox().Process(TXT_UNABLE_PLAY_WAAUGH);

      //
      // Remove myself from the player list box
      //
      if (playerlist.Count()) {  // added: BRR 6/14/96
        playerlist.Remove_Item(0);
        playerlist.Flag_To_Redraw();
      }

      //
      // Remove myself from the Players list
      //
      if (Players.Count()) {  // added: BRR 6/14/96
        who = Players[0];
        Players.Delete(0);
        delete who;
      }

      base::FillBytes(base::ObjectBytes(GPacket), 0, sizeof(GlobalPacketType));

      GPacket.Command = NET_SIGN_OFF;
      port::SafeCopy(GPacket.Name, MPlayerName);

      for (i = 0; i < Players.Count(); i++) {
        Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                sizeof(GlobalPacketType), 1,
                                &Players[i]->Address);
        Ipx.Service();
      }

      Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                              sizeof(GlobalPacketType), 0, nullptr);
      Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                              sizeof(GlobalPacketType), 0, nullptr);

      if (IsBridge) {
        Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                sizeof(GlobalPacketType), 0, &BridgeNet);
        Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                sizeof(GlobalPacketType), 0, &BridgeNet);
      }

      while (Ipx.Global_Num_Send() > 0 && Ipx.Service() != 0) {
      }

      rc = -1;

    } else {
      /*..................................................................
      Set the number of players in this game, and my ID
      ..................................................................*/
      MPlayerCount = static_cast<int>(Players.Count());
      MPlayerLocalID = static_cast<unsigned char>(
          Build_MPlayerID(MPlayerColorIdx, MPlayerHouse));

      /*..................................................................
      Get the scenario number
      ..................................................................*/
      Scenario = MPlayerFilenum[ScenarioIdx];

      /*..................................................................
      Form connections with all other players.  Form the IPX Connection ID
      from the player's Color and House.  This will let us extract any
      player's color & house at any time.  Fill in 'tmp_id' while we're
      doing this.
      ..................................................................*/
      for (i = 0; i < Players.Count(); i++) {
        /*...............................................................
        Only create the connection if it's not myself!
        ...............................................................*/
        if (std::string_view(MPlayerName) != Players[i]->Name) {
          id = static_cast<unsigned char>(Build_MPlayerID(
              Players[i]->Player.Color, Players[i]->Player.House));

          base::At(tmp_id, i) = id;

          Ipx.Create_Connection(id, Players[i]->Name, &Players[i]->Address);
        } else {
          base::At(tmp_id, i) = MPlayerLocalID;
        }
      }

      /*..................................................................
      Store every player's ID in the MPlayerID[] array.  This array will
      determine the order of event execution, so the ID's must be stored
      in the same order on all systems.
      ..................................................................*/
      for (i = 0; i < MPlayerCount; i++) {
        min_index = 0;
        min_id = 0xff;
        for (j = 0; j < MPlayerCount; j++) {
          if (base::At(tmp_id, j) < min_id) {
            min_id = base::At(tmp_id, j);
            min_index = j;
          }
        }
        base::At(MPlayerID, i) = base::At(tmp_id, min_index);
        base::At(tmp_id, min_index) = 0xff;
      }
      /*..................................................................
      Fill in the array of player names, including my own.
      ..................................................................*/
      for (i = 0; i < MPlayerCount; i++) {
        if (base::At(MPlayerID, i) == MPlayerLocalID) {
          port::SafeCopy(base::At(MPlayerNames, i), MPlayerName);
        } else {
          port::SafeCopy(base::At(MPlayerNames, i),
                         Ipx.Connection_Name(base::At(MPlayerID, i)));
        }
      }
    }
    /*---------------------------------------------------------------------
    Wait a while, polling the IPX service routines, to give our ACK
    a chance to get to the other system.  If he doesn't get our ACK, he'll
    be waiting the whole time we load MIX files.
    ---------------------------------------------------------------------*/
    i = std::max<int>(static_cast<int>(Ipx.Global_Response_Time()) * 2, 60);
    starttime = TickCount.Time();
    while (TickCount.Time() - starttime < static_cast<int64_t>(i)) {
      Ipx.Service();
    }
  }

  /*------------------------------------------------------------------------
  Init network timing values, using previous response times as a measure
  of what our retry delta & timeout should be.
  ------------------------------------------------------------------------*/
  Ipx.Set_Timing(Ipx.Global_Response_Time() + 2, -1,
                 Ipx.Global_Response_Time() * 4);

  /*------------------------------------------------------------------------
  Clear all lists
  ------------------------------------------------------------------------*/
  Clear_Game_List(&gamelist);
  Clear_Player_List(&playerlist);

  /*------------------------------------------------------------------------
  Restore screen
  ------------------------------------------------------------------------*/
  Hide_Mouse();
  Load_Title_Page(true);
  Show_Mouse();

  return rc;
}

/***************************************************************************
 * Clear_Game_List -- Clears the game-name listbox & 'Games' Vector        *
 *                                                                         *
 * Assumes each entry in 'Games' & the list box have been allocated *
 * separately.
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		gamelist		ptr to list box
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none
 **
 *                                                                         *
 * HISTORY:                                                                *
 *=========================================================================*/
static void Clear_Game_List(ListClass* gamelist) {

  /*------------------------------------------------------------------------
  Clear the list box
  ------------------------------------------------------------------------*/
  gamelist->Clear();

  /*------------------------------------------------------------------------
  Clear the 'Games' Vector
  ------------------------------------------------------------------------*/
  for (int i = 0; i < Games.Count(); i++) {
    delete Games[i];
  }

  Games.Clear();

} /* end of Clear_Game_List */

/***************************************************************************
 * Clear_Player_List -- Clears the player-name listbox & Vector *
 *                                                                         *
 * Assumes each entry in 'Players' & the list box have been allocated *
 * separately.
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		playerlist		ptr to list box
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none
 **
 *                                                                         *
 * HISTORY:                                                                *
 *=========================================================================*/
static void Clear_Player_List(ListClass* playerlist) {

  /*------------------------------------------------------------------------
  Clear the list box
  ------------------------------------------------------------------------*/
  playerlist->Clear();

  /*------------------------------------------------------------------------
  Clear the 'Players' Vector
  ------------------------------------------------------------------------*/
  for (int i = 0; i < Players.Count(); i++) {
    delete Players[i];
  }

  Players.Clear();

} /* end of Clear_Player_List */

/***************************************************************************
 * Request_To_Join -- Sends a JOIN request packet to game owner            *
 *                                                                         *
 * Regardless of the return code, the Join Dialog will need to be redrawn
 ** after calling this routine.
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		playername		player's name
 ** join_index		index of game we're joining
 ** playerlist		listbox containing other players' names
 ** house				requested house
 ** color				requested color
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = Packet sent, 0 = wasn't
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *=========================================================================*/
static bool Request_To_Join(const char* playername, int join_index,
                            ListClass* /*playerlist*/, HousesType house,
                            int color) {

  /*
  --------------------------- Validate join_index --------------------------
  */
  if (Games.Count() == 0 || join_index > Games.Count() || join_index < 0) {
    CCMessageBox().Process(TXT_NOTHING_TO_JOIN);
    return false;
  }

  /*
  ----------------------- Force user to enter a name -----------------------
  */
  if (std::string_view(playername).empty()) {
    CCMessageBox().Process(TXT_NAME_ERROR);
    return false;
  }

  /*
  ------------------------- The game must be open --------------------------
  */
  if (!Games[join_index]->Game.IsOpen) {
    CCMessageBox().Process(TXT_GAME_IS_CLOSED);
    return false;
  }

  /*
  ------------------------ Make sure name is unique ------------------------
  */
  for (int i = 0; i < Players.Count(); i++) {
    if (!port::CompareIgnoreCase(playername, Players[i]->Name)) {
      CCMessageBox().Process(TXT_NAME_MUSTBE_UNIQUE);
      return false;
    }
  }

  /*
  ----------------------------- Check version #'s --------------------------
  */
  int v = 0;
#ifdef PATCH
  if (IsV107) {
    v = 1;
  } else {
    v = 2;
  }
#else
  v = Version_Number();
#endif
  if (Games[join_index]->Game.Version > v) {
    CCMessageBox().Process(TXT_YOURGAME_OUTDATED);
    return false;
  }
  if (Games[join_index]->Game.Version < v) {
    CCMessageBox().Process(TXT_DESTGAME_OUTDATED);
    return false;
  }

  /*
  ----------------------------- Save game name -----------------------------
  */
  port::SafeCopy(MPlayerName, playername);

  /*
  ----------------------- Send packet to game's owner ----------------------
  */
  base::FillBytes(base::ObjectBytes(GPacket), 0, sizeof(GlobalPacketType));

  GPacket.Command = NET_QUERY_JOIN;
  port::SafeCopy(GPacket.Name, MPlayerName);
  GPacket.PlayerInfo.House = house;
  GPacket.PlayerInfo.Color = static_cast<unsigned int>(color);

  Ipx.Send_Global_Message(base::ObjectBytes(GPacket), sizeof(GlobalPacketType),
                          1, &Games[join_index]->Address);

  return true;
}

/***********************************************************************************************
 * Send_Join_Queries -- sends queries for the Join Dialog
 **
 *                                                                         						  *
 * This routine [re]sends the queries related to the Join Dialog:
 **
 * - NET_QUERY_GAME
 **
 * - NET_QUERY_PLAYER for the game currently selected (if there is one)
 **
 *																															  *
 * The queries are "staggered" in time so they aren't all sent at once;
 *otherwise, we'd		  * be inundated with reply packets & we'd miss
 *some (even though the replies will require		  * ACK's).
 **
 *                                                                         						  *
 * INPUT: * curgame		index of currently-selected game; -1 = none
 ** gamenow		if 1, will immediately send the game query
 ** playernow	if 1, will immediately send the player query for
 *currently-selected game	  *
 *                                                                         						  *
 * OUTPUT: * none.
 **
 *                                                                         						  *
 * WARNINGS: * none.
 **
 *                                                                         						  *
 * HISTORY: * 02/14/1995 BR : Created. * 04/15/1995 BRR : Created. *
 *=============================================================================================*/
static void Send_Join_Queries(int curgame, int gamenow, int playernow) {
  static int lasttime1 = 0;  // time since last Game query sent out
  static int lasttime2 = 0;  // time since last Player query sent out

  /*------------------------------------------------------------------------
  Send the game-name query if the time has expired, or we're told to do
  it right now
  ------------------------------------------------------------------------*/
  if (TickCount.Time() - lasttime1 > 120 || gamenow) {
    lasttime1 = static_cast<int>(TickCount.Time());

    base::FillBytes(base::ObjectBytes(GPacket), 0, sizeof(GlobalPacketType));

    GPacket.Command = NET_QUERY_GAME;

    Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                            sizeof(GlobalPacketType), 0, nullptr);

    /*.....................................................................
    If the user specified a remote server address, broadcast over that
    network, too.
    .....................................................................*/
    if (IsBridge) {
      Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                              sizeof(GlobalPacketType), 0, &BridgeNet);
    }
  }

  /*------------------------------------------------------------------------
  Send the player query for the game currently clicked on, if the time has
  expired and there is a currently-selected game, or we're told to do it
  right now
  ------------------------------------------------------------------------*/
  if (curgame != -1 && curgame < Games.Count() &&
      (TickCount.Time() - lasttime2 > 35 || playernow)) {
    lasttime2 = static_cast<int>(TickCount.Time());

    base::FillBytes(base::ObjectBytes(GPacket), 0, sizeof(GlobalPacketType));

    GPacket.Command = NET_QUERY_PLAYER;
    port::SafeCopy(GPacket.Name, Games[curgame]->Name);

    Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                            sizeof(GlobalPacketType), 0, nullptr);

    /*.....................................................................
    If the user specified a remote server address, broadcast over that
    network, too.
    .....................................................................*/
    if (IsBridge) {
      Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                              sizeof(GlobalPacketType), 0, &BridgeNet);
    }
  }

} /* end of Send_Join_Queries */

/***********************************************************************************************
 * Get_Join_Responses -- sends queries for the Join Dialog
 **
 *                                                                         						  *
 * This routine polls the Global Channel to see if there are any incoming
 *packets;				  * if so, it processes them.  This
 *routine can change the state of the Join Dialog, or			  * the
 *contents of the list boxes, based on what the packet is.
 **
 *                                                                         						  *
 * The list boxes are passed in as pointers; they can't be made globals, because
 *they			  * can't be constructed, because they require shape
 *pointers to the arrow buttons, and			  * the mix files won't
 *have been initialized when the global variables' constructors are * called.
 **
 *                                                                         						  *
 * This routine sets the globals
 ** MPlayerHouse			(from NET_CONFIRM_JOIN)
 ** MPlayerColorIdx		(from NET_CONFIRM_JOIN)
 ** MPlayerBases			(from NET_GAME_OPTIONS)
 ** MPlayerTiberium		(from NET_GAME_OPTIONS)
 ** MPlayerGoodies			(from NET_GAME_OPTIONS)
 ** MPlayerGhosts			(from NET_GAME_OPTIONS)
 ** ScenarioIdx				(from NET_GAME_OPTIONS; -1 = scenario
 *not found)						  *
 *                                                                         						  *
 * INPUT: * joinstate		current state of Join Dialog
 ** gamelist			list box containing game names
 ** playerlist		list box containing player names for the
 *currently-selected game			  * join_index		index of
 *the game we've joined or are asking to join
 **
 *                                                                         						  *
 * OUTPUT: * Event that occurred
 **
 *                                                                         						  *
 * WARNINGS: * none.
 **
 *                                                                         						  *
 * HISTORY: * 02/14/1995 BR : Created. * 04/15/1995 BRR : Created. *
 *=============================================================================================*/
static JoinEventType Get_Join_Responses(JoinStateType* joinstate,
                                        ListClass* gamelist,
                                        ColorListClass* playerlist,
                                        int join_index) {
  char item[kGameListItemSize];  // general-purpose string
  NodeNameType* who = nullptr;   // node to add to Games or Players
  int i = 0;
  int found = 0;
  JoinEventType retcode = EV_NONE;
  char txt[80];

  /*------------------------------------------------------------------------
  If there is no incoming packet, just return
  ------------------------------------------------------------------------*/
  const int rc = Ipx.Get_Global_Message(base::ObjectBytes(GPacket), &GPacketlen,
                                        &GAddress, &GProductID);
  if (!rc || GProductID != IPXGlobalConnClass::kCommandAndConquer) {
    return EV_NONE;
  }

  /*------------------------------------------------------------------------
  If we're joined in a game, handle the packet in a standard way; otherwise,
  don't answer standard queries.
  ------------------------------------------------------------------------*/
  if (*joinstate == JOIN_CONFIRMED &&
      Process_Global_Packet(&GPacket, &GAddress)) {
    return EV_NONE;
  }

  /*------------------------------------------------------------------------
  NET_ANSWER_GAME:  Another system is answering our GAME query, so add that
  system to our list box if it's new.
  ------------------------------------------------------------------------*/
  if (GPacket.Command == NET_ANSWER_GAME) {
    /*.....................................................................
    See if this name is unique
    .....................................................................*/
    retcode = EV_NONE;
    found = 0;
    for (i = 0; i < Games.Count(); i++) {
      if ((std::string_view(Games[i]->Name) == GPacket.Name)) {
        found = 1;
        /*...............................................................
        If name was found, update the node's time stamp & IsOpen flag.
        ...............................................................*/
        Games[i]->Game.LastTime = TickCount.Time();
        if (Games[i]->Game.IsOpen != GPacket.GameInfo.IsOpen) {
          if (GPacket.GameInfo.IsOpen) {
            Format_Runtime_Text(item, kGameListItemSize,
                                Text_String(TXT_THATGUYS_GAME), GPacket.Name);
          } else {
            Format_Runtime_Text(item, kGameListItemSize,
                                Text_String(TXT_THATGUYS_GAME_BRACKET),
                                GPacket.Name);
          }
          gamelist->Set_Item(i, item);
          Games[i]->Game.IsOpen = GPacket.GameInfo.IsOpen;
          gamelist->Flag_To_Redraw();
          /*............................................................
          If this game has gone from closed to open, copy the responder's
          address into our Game slot, since the guy responding to this
          must be game owner.
          ............................................................*/
          if (Games[i]->Game.IsOpen) {
            Games[i]->Address = GAddress;
          }
        }
        break;
      }
    }
    /*.....................................................................
    name not found (or addresses are different); add it to 'Games'
    .....................................................................*/
    if (found == 0) {
      /*..................................................................
      Create a new node structure, fill it in, add it to 'Games'
      ..................................................................*/
      who = new NodeNameType;
      port::SafeCopy(who->Name, GPacket.Name);
      who->Address = GAddress;
      who->Game.Version = GPacket.GameInfo.Version;
      who->Game.IsOpen = GPacket.GameInfo.IsOpen;
      who->Game.LastTime = TickCount.Time();
      Games.Add(who);

      /*..................................................................
      Create a string for "xxx's Game", leaving room for brackets around
      the string if it's a closed game
      ..................................................................*/
      if (GPacket.GameInfo.IsOpen) {
        Format_Runtime_Text(item, kGameListItemSize,
                            Text_String(TXT_THATGUYS_GAME), GPacket.Name);
      } else {
        Format_Runtime_Text(item, kGameListItemSize,
                            Text_String(TXT_THATGUYS_GAME_BRACKET),
                            GPacket.Name);
      }
      gamelist->Add_Item(item);

      retcode = EV_NEW_GAME;
    }
  }

  /*------------------------------------------------------------------------
  NET_ANSWER_PLAYER: Another system is answering our PLAYER query, so add it
  to our player list box & the Player Vector if it's new
  ------------------------------------------------------------------------*/
  else if (GPacket.Command == NET_ANSWER_PLAYER) {
    /*.....................................................................
    See if this name is unique
    .....................................................................*/
    retcode = EV_NONE;
    found = 0;
    for (i = 0; i < Players.Count(); i++) {
      /*..................................................................
      If the address is already present, re-copy their name, color &
      house into the existing entry, in case they've changed it without
      our knowledge; set the 'found' flag so we won't create a new entry.
      ..................................................................*/
      if (Players[i]->Address == GAddress) {
        port::SafeCopy(Players[i]->Name, GPacket.Name);
        Players[i]->Player.House = GPacket.PlayerInfo.House;
        Players[i]->Player.Color =
            static_cast<unsigned char>(GPacket.PlayerInfo.Color);
        playerlist->Colors[i] = static_cast<char>(
            base::At(MPlayerTColors, GPacket.PlayerInfo.Color));
        found = 1;
        break;
      }
    }
    /*.....................................................................
    Don't add this player if he's not part of the game that's selected.
    .....................................................................*/
    i = gamelist->Current_Index();
    if (Games.Count() &&
        GPacket.PlayerInfo.NameCRC != Compute_Name_CRC(Games[i]->Name)) {
      found = 1;
    }

    /*
    ** Dont add this player if its really me! (hack, hack)
    */
    if ((std::string_view(GPacket.Name) == MPlayerName)) {
      found = 1;
    }

    /*.....................................................................
    name not found (or address didn't match); add to player list box & Vector
    .....................................................................*/
    if (found == 0) {
      /*..................................................................
      Create & add a node to the Vector
      ..................................................................*/
      who = new NodeNameType;
      port::SafeCopy(who->Name, GPacket.Name);
      who->Address = GAddress;
      who->Player.House = GPacket.PlayerInfo.House;
      who->Player.Color = static_cast<unsigned char>(GPacket.PlayerInfo.Color);
      Players.Add(who);

      /*..................................................................
      Create & add a string to the list box
      ..................................................................*/
      if (GPacket.PlayerInfo.House == HOUSE_GOOD) {
        absl::SNPrintF(item, sizeof(item), "%s\t%s", GPacket.Name,
                       Text_String(TXT_G_D_I));
      } else {
        absl::SNPrintF(item, sizeof(item), "%s\t%s", GPacket.Name,
                       Text_String(TXT_N_O_D));
      }
      playerlist->Add_Item(
          item, static_cast<char>(base::At(MPlayerTColors, who->Player.Color)));

      retcode = EV_NEW_PLAYER;
    }
  }

  /*------------------------------------------------------------------------
  NET_CONFIRM_JOIN: The game owner has confirmed our JOIN query; mark us as
  being confirmed, and start answering queries from other systems
  ------------------------------------------------------------------------*/
  else if (GPacket.Command == NET_CONFIRM_JOIN) {
    if (*joinstate != JOIN_CONFIRMED) {
      port::SafeCopy(MPlayerGameName, GPacket.Name);
      MPlayerHouse = GPacket.PlayerInfo.House;
      MPlayerColorIdx = static_cast<int>(GPacket.PlayerInfo.Color);

      *joinstate = JOIN_CONFIRMED;
      retcode = EV_STATE_CHANGE;
    }
  }

  /*------------------------------------------------------------------------
  NET_REJECT_JOIN: The game owner has turned down our JOIN query; restore
  the dialog state to its first pop-up state.  Broadcast a sign-off to
  tell all other systems that I'm no longer a part of any game; this way,
  I'll be properly removed from their dialogs.
  ------------------------------------------------------------------------*/
  else if (GPacket.Command == NET_REJECT_JOIN) {
    if (*joinstate != JOIN_REJECTED) {
      base::FillBytes(base::ObjectBytes(GPacket), 0, sizeof(GlobalPacketType));

      GPacket.Command = NET_SIGN_OFF;
      port::SafeCopy(GPacket.Name, MPlayerName);

      Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                              sizeof(GlobalPacketType), 0, nullptr);
      Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                              sizeof(GlobalPacketType), 0, nullptr);

      if (IsBridge) {
        Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                sizeof(GlobalPacketType), 0, &BridgeNet);
        Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                sizeof(GlobalPacketType), 0, &BridgeNet);
      }

      while (Ipx.Global_Num_Send() > 0 && Ipx.Service() != 0) {
      }

      base::At(MPlayerGameName, 0) = 0;

      *joinstate = JOIN_REJECTED;
      retcode = EV_STATE_CHANGE;
    }
  }

  /*------------------------------------------------------------------------
  NET_GAME_OPTIONS: The game owner has changed the game options & is sending
  us the new values.
  ------------------------------------------------------------------------*/
  else if (GPacket.Command == NET_GAME_OPTIONS) {
    if (*joinstate == JOIN_CONFIRMED) {
      MPlayerCredits = static_cast<int>(GPacket.ScenarioInfo.Credits);
      MPlayerBases = GPacket.ScenarioInfo.IsBases;
      MPlayerTiberium = GPacket.ScenarioInfo.IsTiberium;
      MPlayerGoodies = GPacket.ScenarioInfo.IsGoodies;
      MPlayerGhosts = GPacket.ScenarioInfo.IsGhosties;
      BuildLevel = GPacket.ScenarioInfo.BuildLevel;
      MPlayerUnitCount = GPacket.ScenarioInfo.UnitCount;
      Seed = GPacket.ScenarioInfo.Seed;
      Special = GPacket.ScenarioInfo.Special;
      Options.GameSpeed = GPacket.ScenarioInfo.GameSpeed;

      if (MPlayerTiberium) {
        Special.IsTGrowth = 1;
        Special.IsTSpread = 1;
      } else {
        Special.IsTGrowth = 0;
        Special.IsTSpread = 0;
      }

      if (Winsock.Get_Connected()) {
        ScenarioIdx = GPacket.ScenarioInfo.Scenario;
      } else {
        ScenarioIdx = -1;
        for (i = 0; i < MPlayerFilenum.Count(); i++) {
          if (std::cmp_equal(GPacket.ScenarioInfo.Scenario,
                             MPlayerFilenum[i])) {
            ScenarioIdx = i;
          }
        }
      }

      retcode = EV_GAME_OPTIONS;
    }
  }

  /*------------------------------------------------------------------------
  NET_SIGN_OFF: Another system is signing off: search for that system in
  both the game list & player list, & remove it if found
  ------------------------------------------------------------------------*/
  else if (GPacket.Command == NET_SIGN_OFF) {
    /*.....................................................................
    Remove this name from the list of games
    .....................................................................*/
    for (i = 0; i < Games.Count(); i++) {
      if ((std::string_view(Games[i]->Name) == GPacket.Name) &&
          Games[i]->Address == GAddress) {
        /*...............................................................
        If the system signing off is the currently-selected list
        item, clear the player list since that game is no longer
        forming.
        ...............................................................*/
        if (i == gamelist->Current_Index()) {
          Clear_Player_List(playerlist);
        }

        /*...............................................................
        If the system signing off was the owner of our game, mark
        ourselves as rejected
        ...............................................................*/
        if (*joinstate > JOIN_NOTHING && i == join_index) {
          *joinstate = JOIN_REJECTED;
          retcode = EV_STATE_CHANGE;
        }

        /*
        ....................... Set my return code ......................
        */
        if (retcode == EV_NONE) {
          if (i <= gamelist->Current_Index()) {
            retcode = EV_GAME_SIGNOFF;
          } else {
            retcode = EV_PLAYER_SIGNOFF;
          }
        }

        /*
        ................. Remove game name from game list ...............
        */
        Games.Delete(Games[i]);
        gamelist->Remove_Item(i);
        gamelist->Flag_To_Redraw();
      }
    }
    /*.....................................................................
    Remove this name from the list of players
    .....................................................................*/
    for (i = 0; i < Players.Count(); i++) {
      /*
      ..................... Name found; remove it .....................
      */
      if (Players[i]->Address == GAddress) {
        playerlist->Remove_Item(i);
        Players.Delete(Players[i]);
        playerlist->Flag_To_Redraw();

        if (retcode == EV_NONE) {
          retcode = EV_PLAYER_SIGNOFF;
        }
      }
    }
  }

  /*------------------------------------------------------------------------
  NET_GO: The game's owner is signalling us to start playing.
  ------------------------------------------------------------------------*/
  else if (GPacket.Command == NET_GO) {
    if (*joinstate == JOIN_CONFIRMED) {
      MPlayerMaxAhead = GPacket.ResponseTime.OneWay;
      *joinstate = JOIN_GAME_START;
      retcode = EV_STATE_CHANGE;
      CCDebugString("C&C95 - Received the 'GO' packet\n");
    }
  }

  /*------------------------------------------------------------------------
  NET_MESSAGE: Someone is sending us a message
  ------------------------------------------------------------------------*/
  else if (GPacket.Command == NET_MESSAGE) {
    Format_Runtime_Text(txt, sizeof(txt), Text_String(TXT_FROM), GPacket.Name,
                        GPacket.Message.Buf);
    const auto magic_number =
        port::ReadUnaligned<uint16_t>(base::ObjectBytes(GPacket.Message.Buf)
                                          .subspan(COMPAT_MESSAGE_LENGTH - 4));
    const auto crc =
        port::ReadUnaligned<uint16_t>(base::ObjectBytes(GPacket.Message.Buf)
                                          .subspan(COMPAT_MESSAGE_LENGTH - 2));
    const int color =
        static_cast<int>(MPlayerID_To_ColorIndex(GPacket.Message.ID));
    Messages.Add_Message(txt, base::At(MPlayerTColors, color),
                         TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW, 1200,
                         magic_number, crc);
    retcode = EV_MESSAGE;
  }

  /*------------------------------------------------------------------------
  Default case: nothing happened.  (This case will be hit every time I
  receive my own NET_QUERY_GAME or NET_QUERY_PLAYER packets.)  It also covers
  NET_PING: someone pinging me to get a response time measure (will only
  happen after I've joined a game); the IPX Manager will handle sending an
  ACK, and updating the response time measurements.
  ------------------------------------------------------------------------*/
  else {
    retcode = EV_NONE;
  }

  return retcode;
}

/***********************************************************************************************
 * Net_New_Dialog -- lets user start a new game
 **
 *                                                                         						  *
 * This dialog shows a list of who's requesting to join this game, and lets
 ** the game initiator selectively approve each user.
 **
 *                                                                         						  *
 *    ┌────────────────────────────────────────────┐
 ** │              New Network Game              │
 ** │                                            │
 ** │     Players               Scenario         │
 ** │ ┌─────────────┬─┐   ┌──────────────────┬─┐ │
 ** │ │ Boffo       │↑│   │ Hell's Kitchen   │↑│ │
 ** │ │ Bozo        ├─┤   │ Heaven's Gate    ├─┤ │
 ** │ │ Bonzo       │ │   │      ...         │ │ │
 ** │ │             ├─┤   │                  ├─┤ │
 ** │ │             │↓│   │                  │↓│ │
 ** │ └─────────────┴─┘   └──────────────────┴─┘ │
 ** │     [Reject]             Count:--- ##      │
 ** │                          Level:--- ##      │
 ** │                                            │
 ** │               Credits: _____               │ * │       [  Bases   ]   [
 *Crates   ]        │ * │       [ Tiberium ]   [ AI Players ]        │
 ** │                                            │
 ** │              [OK]    [Cancel]              │
 ** │  ┌─────────────────────────────────────┐   │
 ** │  │                                     │   │
 ** │  │                                     │   │
 ** │  └─────────────────────────────────────┘   │
 ** │               [Send Message]               │
 ** └────────────────────────────────────────────┘
 **
 *                                                                         						  *
 * INPUT: * none.
 **
 *                                                                         						  *
 * OUTPUT: * true = success, false = cancel
 **
 *                                                                         						  *
 * WARNINGS: * MPlayerName & MPlayerGameName must contain this player's name.
 **
 *                                                                         						  *
 * HISTORY: * 02/14/1995 BR : Created. *
 *=============================================================================================*/
static int Net_New_Dialog() {
  /* ###Change collision detected! C:\PROJECTS\CODE\NETDLG.CPP... */
  const int factor = SeenBuff.Get_Width() == 320 ? 1 : 2;
  /*........................................................................
  Dialog & button dimensions
  ........................................................................*/
  // D_DIALOG_W = 281;
  // // dialog width
  const int d_dialog_w = 287 * factor;                       // dialog width
  const int d_dialog_h = 177 * factor;                       // dialog height
  const int d_dialog_x = ((320 * factor) - d_dialog_w) / 2;  // dialog x-coord
  const int d_dialog_y = ((200 * factor) - d_dialog_h) / 2;  // centered y-coord
  const int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);     // center x-coord

  const int d_txt6_h = (6 * factor) + 1;  // ht of 6-pt text
  const int d_margin1 = 5 * factor;       // margin width/height
  const int d_margin2 = 2 * factor;       // margin width/height

  // d_playerlist_w = 100;
  const int d_playerlist_w = 106 * factor;
  const int d_playerlist_h = 27 * factor;
  const int d_playerlist_x = d_dialog_x + d_margin1;
  const int d_playerlist_y = d_dialog_y + d_margin1 + (d_txt6_h * 3);

  const int d_scenariolist_w = 162 * factor;
  const int d_scenariolist_h = 27 * factor;
  const int d_scenariolist_x =
      d_dialog_x + d_dialog_w - d_margin1 - d_scenariolist_w;
  const int d_scenariolist_y = d_dialog_y + d_margin1 + (d_txt6_h * 3);

#if (defined(GERMAN) || defined(FRENCH))
  int d_reject_w = 55 * factor;
#else
  const int d_reject_w = 45 * factor;
#endif
  const int d_reject_h = 9 * factor;
  const int d_reject_x =
      d_playerlist_x + (d_playerlist_w / 2) - (d_reject_w / 2);
  const int d_reject_y = d_playerlist_y + d_playerlist_h + d_margin2;

  const int d_count_w = 25 * factor;
  const int d_count_h = d_txt6_h;
  const int d_count_x = d_scenariolist_x + (d_scenariolist_w / 2);
  const int d_count_y = d_scenariolist_y + d_scenariolist_h + d_margin2;

  const int d_level_w = 25 * factor;
  const int d_level_h = d_txt6_h;
  const int d_level_x = d_scenariolist_x + (d_scenariolist_w / 2);
  const int d_level_y = d_count_y + d_count_h;

  const int d_credits_w = ((CREDITSBUF_MAX - 1) * 7 * factor) + (4 * factor);
  // int d_credits_w = ((CREDITSBUF_MAX - 1) * 6*factor) + 3*factor;
  const int d_credits_h = 9 * factor;
  const int d_credits_x = d_dialog_cx + (2 * factor);
  const int d_credits_y = d_level_y + d_level_h + d_margin1;

#if (defined(GERMAN) || defined(FRENCH))
  int d_bases_w = 120 * factor;  // bga:100;
#else
  const int d_bases_w = 100 * factor;
#endif
  const int d_bases_h = 9 * factor;
  const int d_bases_x = d_dialog_cx - d_bases_w - d_margin2;
  const int d_bases_y = d_credits_y + d_credits_h + d_margin2;

#if (defined(GERMAN) || defined(FRENCH))
  int d_tiberium_w = 120 * factor;
#else
  const int d_tiberium_w = 100 * factor;
#endif
  const int d_tiberium_h = 9 * factor;
  const int d_tiberium_x = d_dialog_cx - d_bases_w - d_margin2;
  const int d_tiberium_y = d_bases_y + d_bases_h + d_margin2;

#if (defined(GERMAN) || defined(FRENCH))
  int d_goodies_w = 120 * factor;
#else
  const int d_goodies_w = 100 * factor;
#endif
  const int d_goodies_h = 9 * factor;
  const int d_goodies_x = d_dialog_cx + d_margin2;
  const int d_goodies_y = d_credits_y + d_credits_h + d_margin2;

#if (defined(GERMAN) || defined(FRENCH))
  int d_ghosts_w = 120 * factor;
#else
  const int d_ghosts_w = 100 * factor;
#endif
  const int d_ghosts_h = 9 * factor;
  const int d_ghosts_x = d_dialog_cx + d_margin2;
  const int d_ghosts_y = d_goodies_y + d_goodies_h + d_margin2;

  const int d_ok_w = 45 * factor;
  const int d_ok_h = 9 * factor;
  const int d_ok_x = d_dialog_cx - d_margin2 - (d_bases_w / 2) - (d_ok_w / 2);
  const int d_ok_y = d_ghosts_y + d_ghosts_h + d_margin1;

#if (defined(GERMAN) || defined(FRENCH))
  int d_cancel_w = 50 * factor;
#else
  const int d_cancel_w = 45 * factor;
#endif
  const int d_cancel_h = 9 * factor;
  const int d_cancel_x =
      d_dialog_cx + d_margin2 + (d_goodies_w / 2) - (d_cancel_w / 2);
  const int d_cancel_y = d_ghosts_y + d_ghosts_h + d_margin1;

  const int d_message_w = d_dialog_w - (d_margin1 * 2);
  const int d_message_h = 34 * factor;
  const int d_message_x = d_dialog_x + d_margin1;
  const int d_message_y = d_cancel_y + d_cancel_h + d_margin1;

  const int d_send_w = 80 * factor;
  const int d_send_h = 9 * factor;
  const int d_send_x = d_dialog_cx - (d_send_w / 2);
  const int d_send_y = d_message_y + d_message_h + d_margin2;

  /*........................................................................
  Button Enumerations
  ........................................................................*/
  constexpr int kButtonPlayerlist = 100;
  constexpr int kButtonScenariolist = 101;
  constexpr int kButtonReject = 102;
  constexpr int kButtonCount = 103;
  constexpr int kButtonLevel = 104;
  constexpr int kButtonCredits = 105;
  constexpr int kButtonBases = 106;
  constexpr int kButtonTiberium = 107;
  constexpr int kButtonGoodies = 108;
  constexpr int kButtonGhosts = 109;
  constexpr int kButtonOk = 110;
  constexpr int kButtonCancel = 111;
  constexpr int kButtonSend = 112;

  /*........................................................................
  Redraw values: in order from "top" to "bottom" layer of the dialog
  ........................................................................*/
  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_UNIT_COUNT = 1,
    REDRAW_MESSAGE = 2,
    REDRAW_BUTTONS = 3,
    REDRAW_BACKGROUND = 4,
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;

  /*........................................................................
  Dialog variables
  ........................................................................*/
  RedrawType display = REDRAW_ALL;  // redraw level
  bool process = true;              // process while true
  KeyNumType input = KN_NONE;

  char credbuf[CREDITSBUF_MAX];  // for credit edit box

  int64_t ok_timer = 0;  // for timing OK button
  int index = 0;         // index for rejecting a player
  int rc = 0;
  int i = 0;
  int j = 0;
  char item[kGameListItemSize];
  const int tabs[] = {77 * factor};  // tabs for player list box

  int64_t ping_timer = 0;  // for sending Ping packets

  unsigned char tmp_id[MAX_PLAYERS] =
      {};                    // temp storage for sorting player ID's
  int min_index = 0;         // for sorting player ID's
  unsigned char min_id = 0;  // for sorting player ID's
  unsigned char id = 0;      // connection ID
  char txt[80];
  static int first_time = 1;  // 1 = 1st time this dialog is run

  int message_length = 0;
  int sent_so_far = 0;
  uint16_t magic_number = 0;
  uint16_t crc = 0;

  /*........................................................................
  Buttons
  ........................................................................*/

  std::span<const std::byte> up_button;
  std::span<const std::byte> down_button;

  if (InMainLoop) {
    up_button = Hires_Retrieve("BTN-UP.SHP");
    down_button = Hires_Retrieve("BTN-DN.SHP");
  } else {
    up_button = Hires_Retrieve("BTN-UP2.SHP");
    down_button = Hires_Retrieve("BTN-DN2.SHP");
  }

  ColorListClass playerlist(kButtonPlayerlist, d_playerlist_x, d_playerlist_y,
                            d_playerlist_w, d_playerlist_h,
                            TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                            up_button, down_button);

  ListClass scenariolist(kButtonScenariolist, d_scenariolist_x,
                         d_scenariolist_y, d_scenariolist_w, d_scenariolist_h,
                         TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                         up_button, down_button);

  EditClass credit_edt(kButtonCredits, credbuf, CREDITSBUF_MAX,
                       TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                       d_credits_x, d_credits_y, d_credits_w, d_credits_h,
                       EditClass::ALPHANUMERIC);

  TextButtonClass rejectbtn(
      kButtonReject, TXT_REJECT,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
      // #if (GERMAN | FRENCH)
      //		d_reject_x, d_reject_y);
      // #else
      d_reject_x, d_reject_y, d_reject_w, d_reject_h);
  // #endif

  GaugeClass countgauge(kButtonCount, d_count_x, d_count_y, d_count_w,
                        d_count_h);

  GaugeClass levelgauge(kButtonLevel, d_level_x, d_level_y, d_level_w,
                        d_level_h);

  TextButtonClass basesbtn(
      kButtonBases, TXT_BASES_OFF,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, d_bases_x,
      d_bases_y, d_bases_w, d_bases_h);

  TextButtonClass tiberiumbtn(
      kButtonTiberium, TXT_TIBERIUM_OFF,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, d_tiberium_x,
      d_tiberium_y, d_tiberium_w, d_tiberium_h);

  TextButtonClass goodiesbtn(
      kButtonGoodies, TXT_CRATES_OFF,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, d_goodies_x,
      d_goodies_y, d_goodies_w, d_goodies_h);

  TextButtonClass ghostsbtn(
      kButtonGhosts, TXT_AI_PLAYERS_OFF,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, d_ghosts_x,
      d_ghosts_y, d_ghosts_w, d_ghosts_h);

  TextButtonClass okbtn(
      kButtonOk, TXT_OK,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, d_ok_x,
      d_ok_y, d_ok_w, d_ok_h);

  TextButtonClass cancelbtn(
      kButtonCancel, TXT_CANCEL,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
      // #if (GERMAN | FRENCH)
      //		d_cancel_x, d_cancel_y);
      // #else
      d_cancel_x, d_cancel_y, d_cancel_w, d_cancel_h);
  // #endif

  TextButtonClass sendbtn(
      kButtonSend, TXT_SEND_MESSAGE,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
      // #if (GERMAN | FRENCH)
      //		d_send_x, d_send_y);
      // #else
      d_send_x, d_send_y, d_send_w, d_send_h);
  // #endif

  /*
  ------------------------- Build the button list --------------------------
  */
  GadgetClass* commands = &playerlist;  // button list
  scenariolist.Add_Tail(*commands);
  credit_edt.Add_Tail(*commands);
  rejectbtn.Add_Tail(*commands);
  countgauge.Add_Tail(*commands);
  levelgauge.Add_Tail(*commands);
  basesbtn.Add_Tail(*commands);
  tiberiumbtn.Add_Tail(*commands);
  goodiesbtn.Add_Tail(*commands);
  ghostsbtn.Add_Tail(*commands);
  okbtn.Add_Tail(*commands);
  cancelbtn.Add_Tail(*commands);
  sendbtn.Add_Tail(*commands);

  playerlist.Set_Tabs(tabs);

  /*
  ----------------------------- Various Inits ------------------------------
  */
  /*........................................................................
  Init dialog values, only the first time through
  ........................................................................*/
  if (first_time) {
    MPlayerCredits = 3000;  // init credits & credit buffer
    MPlayerBases = 1;       // init scenario parameters
    MPlayerTiberium = 0;
    MPlayerGoodies = 0;
    MPlayerGhosts = 0;
    Special.IsCaptureTheFlag = 0;
    MPlayerUnitCount = (base::At(MPlayerCountMax, MPlayerBases) +
                        base::At(MPlayerCountMin, MPlayerBases)) /
                       2;
    first_time = 0;
  }

  /*........................................................................
  Init button states
  ........................................................................*/
  if (MPlayerBases) {
    basesbtn.Turn_On();
    basesbtn.Set_Text(TXT_BASES_ON);
  }
  if (MPlayerTiberium) {
    tiberiumbtn.Turn_On();
    tiberiumbtn.Set_Text(TXT_TIBERIUM_ON);
  }
  if (MPlayerGoodies) {
    goodiesbtn.Turn_On();
    goodiesbtn.Set_Text(TXT_CRATES_ON);
  }
  if (MPlayerGhosts) {
    ghostsbtn.Turn_On();
    ghostsbtn.Set_Text(TXT_AI_PLAYERS_ON);
  }
  if (Special.IsCaptureTheFlag) {
    MPlayerGhosts = 0;
    ghostsbtn.Turn_On();
    ghostsbtn.Set_Text(TXT_CAPTURE_THE_FLAG);
  }

  absl::SNPrintF(credbuf, sizeof(credbuf), "%d", MPlayerCredits);
  credit_edt.Set_Text(credbuf, CREDITSBUF_MAX);
  int old_cred = MPlayerCredits;  // old value in credits buffer

  levelgauge.Set_Maximum(MPLAYER_BUILD_LEVEL_MAX - 1);
  levelgauge.Set_Value(BuildLevel - 1);

  countgauge.Set_Maximum(base::At(MPlayerCountMax, MPlayerBases) -
                         base::At(MPlayerCountMin, MPlayerBases));
  countgauge.Set_Value(MPlayerUnitCount -
                       base::At(MPlayerCountMin, MPlayerBases));

  /*........................................................................
  Init other scenario parameters
  ........................................................................*/
  Special.IsTGrowth = static_cast<unsigned>(MPlayerTiberium);
  Special.IsTSpread = static_cast<unsigned>(MPlayerTiberium);
  int transmit = 0;  // 1 = re-transmit new game options

  /*........................................................................
  Init scenario description list box
  ........................................................................*/
  for (i = 0; i < MPlayerScenarios.Count(); i++) {
    scenariolist.Add_Item(strupr(MPlayerScenarios[i]));
  }
  ScenarioIdx = 0;  // 1st scenario is selected

  /*........................................................................
  Init player color-used flags
  ........................................................................*/
  for (i = 0; i < MAX_MPLAYER_COLORS; i++) {
    base::At(ColorUsed, i) = 0;  // init all colors to available
  }
  base::At(ColorUsed, MPlayerColorIdx) = 1;  // set my color to used
  playerlist.Set_Selected_Style(ColorListClass::SELECT_BAR, kCcGreenShadow);

  /*........................................................................
  Init random-number generator, & create a seed to be used for all random
  numbers from here on out
  ........................................................................*/
  Seed = port::RandomSeed();

  /*........................................................................
  Init the message display system
  ........................................................................*/
  Messages.Init(d_message_x + (2 * factor), d_message_y + (2 * factor), 4,
                MAX_MESSAGE_LENGTH, d_txt6_h);

  /*------------------------------------------------------------------------
  Add myself to the list.  Note that since I'm not in the Players Vector,
  the Vector & listbox are now 1 out of sync.
  ------------------------------------------------------------------------*/
  if (MPlayerHouse == HOUSE_GOOD) {
    absl::SNPrintF(item, sizeof(item), "%s\t%s", MPlayerName,
                   Text_String(TXT_G_D_I));
  } else {
    absl::SNPrintF(item, sizeof(item), "%s\t%s", MPlayerName,
                   Text_String(TXT_N_O_D));
  }
  playerlist.Add_Item(
      item, static_cast<char>(base::At(MPlayerTColors, MPlayerColorIdx)));

  Load_Title_Page(true);
  Set_Palette(Palette);
  while (Get_Mouse_State() > 0) {
    Show_Mouse();
  }

  /*
  ---------------------------- Processing loop -----------------------------
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

#if (SHOW_MONO)
    Ipx.Mono_Debug_Print(-1, 0);
#endif
    /*
    ...................... Refresh display if needed ......................
    */
    if (display == REDRAW_UNIT_COUNT) {
      /*
      ** Wipe the background behind the unit count then reprint it
      */
      LogicPage->Fill_Rect(d_count_x + d_count_w + (2 * factor), d_count_y,
                           d_count_x + d_count_w + (2 * factor) + 20,
                           d_count_y + 12, 0);
      absl::SNPrintF(txt, sizeof(txt), "%d", MPlayerUnitCount);
      Fancy_Text_Print(txt, d_count_x + d_count_w + (2 * factor), d_count_y,
                       kCcGreen, kTBlack,
                       TPF_NOSHADOW | TPF_6PT_GRAD | TPF_USE_GRAD_PAL);
      display = REDRAW_NONE;
    }

    if (display != REDRAW_NONE) {
      Hide_Mouse();
      /*
      .................. Redraw backgound & dialog box ...................
      */
      if (display >= REDRAW_BACKGROUND) {
        /*
        ** Reload and draw the title page
        */
        Load_Title_Page(true);
        Set_Palette(Palette);

        Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);

        /*...............................................................
        Dialog & Field labels
        ...............................................................*/
        Draw_Caption(TXT_NETGAME_SETUP, d_dialog_x, d_dialog_y, d_dialog_w);

        Fancy_Text_Print(
            TXT_PLAYERS, d_playerlist_x + (d_playerlist_w / 2),
            d_playerlist_y - d_txt6_h, kCcGreen, kTBlack,
            TPF_NOSHADOW | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_CENTER);

        Fancy_Text_Print(
            TXT_SCENARIOS, d_scenariolist_x + (d_scenariolist_w / 2),
            d_scenariolist_y - d_txt6_h, kCcGreen, kTBlack,
            TPF_NOSHADOW | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_CENTER);

        Fancy_Text_Print(
            TXT_COUNT, d_count_x - (2 * factor), d_count_y, kCcGreen, kTBlack,
            TPF_NOSHADOW | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_RIGHT);

        absl::SNPrintF(txt, sizeof(txt), "%d", MPlayerUnitCount);
        Fancy_Text_Print(txt, d_count_x + d_count_w + (2 * factor), d_count_y,
                         kCcGreen, kTBlack,
                         TPF_NOSHADOW | TPF_6PT_GRAD | TPF_USE_GRAD_PAL);

        Fancy_Text_Print(
            TXT_LEVEL, d_level_x - (2 * factor), d_level_y, kCcGreen, kTBlack,
            TPF_NOSHADOW | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_RIGHT);

        if (BuildLevel <= MPLAYER_BUILD_LEVEL_MAX) {
          absl::SNPrintF(txt, sizeof(txt), "%d", BuildLevel);
        } else {
          absl::SNPrintF(txt, sizeof(txt), "**");
        }
        Fancy_Text_Print(txt, d_level_x + d_level_w + (2 * factor), d_level_y,
                         kCcGreen, kTBlack,
                         TPF_NOSHADOW | TPF_6PT_GRAD | TPF_USE_GRAD_PAL);

        Fancy_Text_Print(
            TXT_START_CREDITS_COLON, d_credits_x - (5 * factor),
            d_credits_y + (1 * factor), kCcGreen, kTBlack,
            TPF_NOSHADOW | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_RIGHT);
      }

      /*
      .......................... Redraw buttons ..........................
      */
      if (display >= REDRAW_BUTTONS) {
        commands->Draw_All();
      }

      /*..................................................................
      Draw the messages:
      - Erase an old message first
      - If we're in a game, print the game options (if they've been
        received)
      - If we've been rejected from a game, print that message
      ..................................................................*/
      if (display >= REDRAW_MESSAGE) {
        Draw_Box(d_message_x, d_message_y, d_message_w, d_message_h,
                 BOXSTYLE_GREEN_BORDER, true);
        Messages.Draw();
      }

      Show_Mouse();
      display = REDRAW_NONE;
    }

    /*
    ........................... Get user input ............................
    */
    input = commands->Input();

    /*
    ---------------------------- Process input ----------------------------
    */
    switch (static_cast<int>(input)) {
      /*------------------------------------------------------------------
      New Scenario selected.
      ------------------------------------------------------------------*/
      case ButtonKey(kButtonScenariolist):
        if (scenariolist.Current_Index() != ScenarioIdx) {
          ScenarioIdx = scenariolist.Current_Index();
          MPlayerCredits = tech::ParseInteger<int>(credbuf).value_or(0);
          transmit = 1;
        }
        break;

      /*------------------------------------------------------------------
      Reject the currently-selected player (don't allow rejecting myself,
      who will be the first entry in the list)
      ------------------------------------------------------------------*/
      case ButtonKey(kButtonReject):
        index = playerlist.Current_Index();
        if (index == 0) {
          CCMessageBox().Process(TXT_CANT_REJECT_SELF, TXT_OOPS);
          display = REDRAW_ALL;
          break;
        }
        if (index < 0 || index >= playerlist.Count()) {
          CCMessageBox().Process(TXT_SELECT_PLAYER_REJECT, TXT_OOPS);
          display = REDRAW_ALL;
          break;
        }
        base::FillBytes(base::ObjectBytes(GPacket), 0,
                        sizeof(GlobalPacketType));

        GPacket.Command = NET_REJECT_JOIN;

        Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                sizeof(GlobalPacketType), 1,
                                &Players[index - 1]->Address);
        break;

      /*------------------------------------------------------------------
      User adjusts max # units
      ------------------------------------------------------------------*/
      case ButtonKey(kButtonCount):
        MPlayerUnitCount =
            countgauge.Get_Value() + base::At(MPlayerCountMin, MPlayerBases);

        Hide_Mouse();
        LogicPage->Fill_Rect(d_count_x + d_count_w + (2 * factor), d_count_y,
                             d_count_x + d_count_w + (14 * factor),
                             d_count_y + (6 * factor), kBlack);

        absl::SNPrintF(txt, sizeof(txt), "%d", MPlayerUnitCount);
        Fancy_Text_Print(txt, d_count_x + d_count_w + (2 * factor), d_count_y,
                         kCcGreen, kTBlack,
                         TPF_NOSHADOW | TPF_6PT_GRAD | TPF_USE_GRAD_PAL);
        Show_Mouse();

        transmit = 1;
        break;

      /*------------------------------------------------------------------
      User adjusts build level
      ------------------------------------------------------------------*/
      case ButtonKey(kButtonLevel):
        BuildLevel = std::min(levelgauge.Get_Value() + 1,
                                            MPLAYER_BUILD_LEVEL_MAX);

        Hide_Mouse();
        LogicPage->Fill_Rect(d_level_x + d_level_w + (2 * factor), d_level_y,
                             d_level_x + d_level_w + (14 * factor),
                             d_level_y + (6 * factor), kBlack);

        if (BuildLevel <= MPLAYER_BUILD_LEVEL_MAX) {
          absl::SNPrintF(txt, sizeof(txt), "%d", BuildLevel);
        } else {
          absl::SNPrintF(txt, sizeof(txt), "**");
        }
        Fancy_Text_Print(txt, d_level_x + d_level_w + (2 * factor), d_level_y,
                         kCcGreen, kTBlack,
                         TPF_NOSHADOW | TPF_6PT_GRAD | TPF_USE_GRAD_PAL);
        Show_Mouse();

        transmit = 1;
        break;

      /*------------------------------------------------------------------
      User edits the credits value; retransmit new game options
      ------------------------------------------------------------------*/
      case ButtonKey(kButtonCredits):
        MPlayerCredits = tech::ParseInteger<int>(credbuf).value_or(0);
        transmit = 1;
        break;

      /*------------------------------------------------------------------
      Toggle bases:
      - Clear scenario list & rebuild it with new names
      - toggle bases button, change its text
      - adjust the MPlayerUnitCount to reflect the new allowed range,
        using the current gauge setting
      - Change the unit count gauge limit & value
      ------------------------------------------------------------------*/
      case ButtonKey(kButtonBases):
        if (MPlayerBases) {
          MPlayerBases = 0;
          basesbtn.Turn_Off();
          basesbtn.Set_Text(TXT_BASES_OFF);
          MPlayerUnitCount =
              Fixed_To_Cardinal(
                  base::At(MPlayerCountMax, 0) - base::At(MPlayerCountMin, 0),
                  Cardinal_To_Fixed(
                      base::At(MPlayerCountMax, 1) -
                          base::At(MPlayerCountMin, 1),
                      MPlayerUnitCount - base::At(MPlayerCountMin, 1))) +
              base::At(MPlayerCountMin, 0);
        } else {
          MPlayerBases = 1;
          basesbtn.Turn_On();
          basesbtn.Set_Text(TXT_BASES_ON);
          MPlayerUnitCount =
              Fixed_To_Cardinal(
                  base::At(MPlayerCountMax, 1) - base::At(MPlayerCountMin, 1),
                  Cardinal_To_Fixed(
                      base::At(MPlayerCountMax, 0) -
                          base::At(MPlayerCountMin, 0),
                      MPlayerUnitCount - base::At(MPlayerCountMin, 0))) +
              base::At(MPlayerCountMin, 1);
        }
        MPlayerCredits = tech::ParseInteger<int>(credbuf).value_or(0);
        countgauge.Set_Maximum(base::At(MPlayerCountMax, MPlayerBases) -
                               base::At(MPlayerCountMin, MPlayerBases));
        countgauge.Set_Value(MPlayerUnitCount -
                             base::At(MPlayerCountMin, MPlayerBases));
        transmit = 1;
        countgauge.Flag_To_Redraw();
        display = REDRAW_UNIT_COUNT;
        break;

      /*------------------------------------------------------------------
      Toggle tiberium
      ------------------------------------------------------------------*/
      case ButtonKey(kButtonTiberium):
        if (MPlayerTiberium) {
          MPlayerTiberium = 0;
          Special.IsTGrowth = 0;
          Special.IsTSpread = 0;
          tiberiumbtn.Turn_Off();
          tiberiumbtn.Set_Text(TXT_TIBERIUM_OFF);
        } else {
          MPlayerTiberium = 1;
          Special.IsTGrowth = 1;
          Special.IsTSpread = 1;
          tiberiumbtn.Turn_On();
          tiberiumbtn.Set_Text(TXT_TIBERIUM_ON);
        }
        MPlayerCredits = tech::ParseInteger<int>(credbuf).value_or(0);
        transmit = 1;
        break;

      /*------------------------------------------------------------------
      Toggle goodies
      ------------------------------------------------------------------*/
      case ButtonKey(kButtonGoodies):
        if (MPlayerGoodies) {
          MPlayerGoodies = 0;
          goodiesbtn.Turn_Off();
          goodiesbtn.Set_Text(TXT_CRATES_OFF);
        } else {
          MPlayerGoodies = 1;
          goodiesbtn.Turn_On();
          goodiesbtn.Set_Text(TXT_CRATES_ON);
        }
        MPlayerCredits = tech::ParseInteger<int>(credbuf).value_or(0);
        transmit = 1;
        break;

      /*------------------------------------------------------------------
      Toggle ghosts/capture-the-flag
      ------------------------------------------------------------------*/
      case ButtonKey(kButtonGhosts):
        if (!MPlayerGhosts &&
            !Special.IsCaptureTheFlag) {  // ghosts OFF => ghosts ON
          MPlayerGhosts = 1;
          Special.IsCaptureTheFlag = 0;
          ghostsbtn.Turn_On();
          ghostsbtn.Set_Text(TXT_AI_PLAYERS_ON);
        } else {
          if (MPlayerGhosts) {  // ghosts ON => capture-flag
            MPlayerGhosts = 0;
            Special.IsCaptureTheFlag = 1;
            ghostsbtn.Turn_On();
            ghostsbtn.Set_Text(TXT_CAPTURE_THE_FLAG);
          } else {
            if (Special.IsCaptureTheFlag) {  // capture-flag => AI OFF
              MPlayerGhosts = 0;
              Special.IsCaptureTheFlag = 0;
              ghostsbtn.Turn_Off();
              ghostsbtn.Set_Text(TXT_AI_PLAYERS_OFF);
            }
          }
        }

        MPlayerCredits = tech::ParseInteger<int>(credbuf).value_or(0);
        transmit = 1;
        break;

      /*------------------------------------------------------------------
      OK: exit loop with true status
      ------------------------------------------------------------------*/
      case ButtonKey(kButtonOk):
        /*...............................................................
        If a new player has joined in the last second, don't allow
        an OK; force a wait longer than 1 second (to give all players
        a chance to know about this new guy)
        ...............................................................*/
        i = std::max<int>(static_cast<int>(Ipx.Global_Response_Time()) * 2, 60);
        while (TickCount.Time() - ok_timer < i) {
          Ipx.Service();
        }

        /*...............................................................
        If there are at least 2 players, go ahead & play; error otherwise
        ...............................................................*/
        if (MPlayerSolo || Players.Count() > 0) {
          rc = 1;
          process = false;
        } else {
          CCMessageBox().Process(TXT_ONLY_ONE, TXT_OOPS, TXT_NONE);
          display = REDRAW_ALL;
        }
        break;

      /*------------------------------------------------------------------
      CANCEL: send a SIGN_OFF, bail out with error code
      ------------------------------------------------------------------*/
      case KN_ESC:
        if (Messages.Get_Edit_Buf() != nullptr) {
          Messages.Input(input);
          display = std::max(display, REDRAW_MESSAGE);
          break;
        }
        [[fallthrough]];
      case ButtonKey(kButtonCancel):
        base::FillBytes(base::ObjectBytes(GPacket), 0,
                        sizeof(GlobalPacketType));

        GPacket.Command = NET_SIGN_OFF;
        port::SafeCopy(GPacket.Name, MPlayerName);

        /*...............................................................
        Broadcast my sign-off over my network
        ...............................................................*/
        Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                sizeof(GlobalPacketType), 0, nullptr);
        Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                sizeof(GlobalPacketType), 0, nullptr);
        while (Ipx.Global_Num_Send() > 0 && Ipx.Service() != 0) {
        }

        /*...............................................................
        Broadcast my sign-off over a bridged network if there is one
        ...............................................................*/
        if (IsBridge) {
          Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                  sizeof(GlobalPacketType), 0, &BridgeNet);
          Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                  sizeof(GlobalPacketType), 0, &BridgeNet);
        }
        while (Ipx.Global_Num_Send() > 0 && Ipx.Service() != 0) {
        }

        /*...............................................................
        And now, just be absolutely sure, send my sign-off to each
        player in my game.  (If there's a bridge between us, the other
        player will have specified my address, so he can cross the
        bridge; but I may not have specified a bridge address, so the
        only way I have of crossing the bridge is to send a packet
        directly to him.)
        ...............................................................*/
        for (i = 0; i < Players.Count(); i++) {
          Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                  sizeof(GlobalPacketType), 1,
                                  &Players[i]->Address);
          Ipx.Service();
        }
        while (Ipx.Global_Num_Send() > 0 && Ipx.Service() != 0) {
        }
        base::At(MPlayerGameName, 0) = 0;
        process = false;
        rc = 0;
        break;

      /*------------------------------------------------------------------
      Default: manage the inter-player messages
      ------------------------------------------------------------------*/
      default:
        /*...............................................................
        F4/SEND/'M' = send a message
        ...............................................................*/
        if (Messages.Get_Edit_Buf() == nullptr) {
          if (input == KN_M || input == ButtonKey(kButtonSend) ||
              input == KN_F4) {
            base::FillBytes(base::ObjectBytes(txt), 0, 80);

            port::SafeCopy(txt, Text_String(TXT_TO_ALL));  // "To All:"

            Messages.Add_Edit(base::At(MPlayerTColors, MPlayerColorIdx),
                              TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW,
                              txt, d_message_w - (70 * factor));

            credit_edt.Clear_Focus();
            credit_edt.Flag_To_Redraw();
            display = std::max(display, REDRAW_MESSAGE);

            break;
          }
        } else {
          /*...............................................................
          If we're already editing a message and the user clicks on
          'Send', translate our input to a Return so Messages.Input() will
          work properly.
          ...............................................................*/
          if (input == ButtonKey(kButtonSend)) {
            input = KN_RETURN;
          }
        }

        /*...............................................................
        Manage the message system (get rid of old messages)
        ...............................................................*/
        if (Messages.Manage()) {
          display = std::max(display, REDRAW_MESSAGE);
        }

        /*...............................................................
        Re-draw the messages & service keyboard input for any message
        being edited.
        ...............................................................*/
        i = Messages.Input(input);

        /*...............................................................
        If 'Input' returned 1, it means refresh the message display.
        ...............................................................*/
        if (i == 1) {
          Messages.Draw();
        }

        /*...............................................................
        If 'Input' returned 2, it means redraw the message display.
        ...............................................................*/
        else if (i == 2) {
          display = std::max(display, REDRAW_MESSAGE);
        }

        /*...............................................................
        If 'input' returned 3, it means send the current message.
        ...............................................................*/
        else if (i == 3) {

          sent_so_far = 0;
          magic_number = MESSAGE_HEAD_MAGIC_NUMBER;
          message_length = static_cast<int>(
              std::string_view(Messages.Get_Edit_Buf()).size());
          crc = static_cast<uint16_t>(
              CrcEngine::Compute(Messages.Get_Edit_Buf()) & 0xffff);
          while (sent_so_far < message_length) {
            base::FillBytes(base::ObjectBytes(GPacket), 0,
                            sizeof(GlobalPacketType));
            GPacket.Command = NET_MESSAGE;
            port::SafeCopy(GPacket.Name, MPlayerName);
            port::SafeCopy(
                std::span(GPacket.Message.Buf).first(COMPAT_MESSAGE_LENGTH - 4),
                std::string_view(Messages.Get_Edit_Buf())
                    .substr(base::ToSize(sent_so_far)));

            /*
            ** Steve I's stuff for splitting message on word boundries
            */
            int32_t actual_message_size = COMPAT_MESSAGE_LENGTH - 5;

            /* Start at the end of the message and find a space with 10 chars.
             */
            const auto the_string = std::span(GPacket.Message.Buf);
            while (COMPAT_MESSAGE_LENGTH - 5 - actual_message_size < 10 &&
                   the_string[base::ToSize(actual_message_size)] != ' ') {
              --actual_message_size;
            }
            if (the_string[base::ToSize(actual_message_size)] == ' ') {
              /* Now delete the extra characters after the space (they musnt
               * print) */
              for (int k = 0;
                   k < COMPAT_MESSAGE_LENGTH - 5 - actual_message_size; k++) {
                the_string[base::ToSize(k + actual_message_size)] =
                    static_cast<char>(0xff);
              }
            } else {
              actual_message_size = COMPAT_MESSAGE_LENGTH - 5;
            }

            base::At(GPacket.Message.Buf, COMPAT_MESSAGE_LENGTH - 5) = 0;
            port::WriteUnaligned(base::ObjectBytes(GPacket.Message.Buf)
                                     .subspan(COMPAT_MESSAGE_LENGTH - 4),
                                 magic_number);
            port::WriteUnaligned(base::ObjectBytes(GPacket.Message.Buf)
                                     .subspan(COMPAT_MESSAGE_LENGTH - 2),
                                 crc);
            GPacket.Message.ID = static_cast<unsigned char>(
                Build_MPlayerID(MPlayerColorIdx, MPlayerHouse));
            GPacket.Message.NameCRC = Compute_Name_CRC(MPlayerGameName);

            /*..................................................................
            Send the message to every player in our player list.
            ..................................................................*/
            for (i = 0; i < Players.Count(); i++) {
              Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                      sizeof(GlobalPacketType), 1,
                                      &Players[i]->Address);
              Ipx.Service();
            }
            /*..................................................................
            Add the message to our own list, since we're not in the player list
            on this dialog.
            ..................................................................*/
            Format_Runtime_Text(txt, sizeof(txt), Text_String(TXT_FROM),
                                MPlayerName, GPacket.Message.Buf);
            Messages.Add_Message(
                txt, base::At(MPlayerTColors, MPlayerColorIdx),
                TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW, 1200,
                magic_number, crc);

            magic_number++;
            sent_so_far =
                sent_so_far + actual_message_size;  // COMPAT_MESSAGE_LENGTH-5;
          }
          display = std::max(display, REDRAW_MESSAGE);
        }
    }

    /*---------------------------------------------------------------------
    Detect editing of the credits buffer, transmit new values to players
    ---------------------------------------------------------------------*/
    if (tech::ParseInteger<int>(credbuf).value_or(0) != old_cred) {
      old_cred = Bound(tech::ParseInteger<int>(credbuf).value_or(0), 0, 9999);
      MPlayerCredits = old_cred;
      transmit = 1;
      absl::SNPrintF(credbuf, sizeof(credbuf), "%d", MPlayerCredits);
      credit_edt.Set_Text(credbuf, CREDITSBUF_MAX);
    }

    /*---------------------------------------------------------------------
    Process incoming packets
    ---------------------------------------------------------------------*/
    const JoinEventType whahoppa = Get_NewGame_Responses(&playerlist);
    if (whahoppa == EV_NEW_PLAYER) {
      ok_timer = TickCount.Time();
      transmit = 1;
    } else {
      if (whahoppa == EV_MESSAGE) {
        display = std::max(display, REDRAW_MESSAGE);
      }
    }

    /*---------------------------------------------------------------------
    If our Transmit flag is set, we need to send out a game option packet
    ---------------------------------------------------------------------*/
    if (transmit) {
      for (i = 0; i < Players.Count(); i++) {
        base::FillBytes(base::ObjectBytes(GPacket), 0,
                        sizeof(GlobalPacketType));

        GPacket.Command = NET_GAME_OPTIONS;
        GPacket.ScenarioInfo.Scenario =
            static_cast<unsigned char>(MPlayerFilenum[ScenarioIdx]);
        GPacket.ScenarioInfo.Credits =
            static_cast<unsigned int>(MPlayerCredits);
        GPacket.ScenarioInfo.IsBases =
            static_cast<unsigned int>(MPlayerBases);
        GPacket.ScenarioInfo.IsTiberium =
            static_cast<unsigned int>(MPlayerTiberium);
        GPacket.ScenarioInfo.IsGoodies =
            static_cast<unsigned int>(MPlayerGoodies);
        GPacket.ScenarioInfo.IsGhosties =
            static_cast<unsigned int>(MPlayerGhosts);
        GPacket.ScenarioInfo.BuildLevel =
            static_cast<unsigned char>(BuildLevel);
        GPacket.ScenarioInfo.UnitCount =
            static_cast<unsigned char>(MPlayerUnitCount);
        GPacket.ScenarioInfo.Seed = Seed;
        GPacket.ScenarioInfo.Special = Special;
        GPacket.ScenarioInfo.GameSpeed = Options.GameSpeed;

        Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                sizeof(GlobalPacketType), 1,
                                &Players[i]->Address);
      }
      transmit = 0;
    }

    /*---------------------------------------------------------------------
    Ping every player in my game, to force the Global Channel to measure
    the connection response time.
    ---------------------------------------------------------------------*/
    if (TickCount.Time() - ping_timer > 15) {
      base::FillBytes(base::ObjectBytes(GPacket), 0, sizeof(GlobalPacketType));
      GPacket.Command = NET_PING;
      for (i = 0; i < Players.Count(); i++) {
        Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                sizeof(GlobalPacketType), 1,
                                &Players[i]->Address);
      }
      ping_timer = TickCount.Time();
    }

    /*---------------------------------------------------------------------
    Service the Ipx connections
    ---------------------------------------------------------------------*/
    Ipx.Service();

    /*---------------------------------------------------------------------
    Service the sounds & score; GameActive must be false at this point,
    so Call_Back() doesn't intercept global messages from me!
    ---------------------------------------------------------------------*/
    Call_Back();

  } /* end of while */

  /*------------------------------------------------------------------------
  Establish connections with all other players.
  ------------------------------------------------------------------------*/
  if (rc) {
    /*.....................................................................
    Set the number of players in this game, and my ID
    .....................................................................*/
    MPlayerCount = static_cast<int>(Players.Count()) + 1;
    MPlayerLocalID = static_cast<unsigned char>(
        Build_MPlayerID(MPlayerColorIdx, MPlayerHouse));

    /*.....................................................................
    Get the scenario filename
    .....................................................................*/
    Scenario = MPlayerFilenum[ScenarioIdx];

    /*.....................................................................
    Compute frame delay value for packet transmissions:
    - Divide global channel's response time by 8 (2 to convert to 1-way
      value, 4 more to convert from ticks to frames)
    .....................................................................*/
    MPlayerMaxAhead = std::max<int>(static_cast<int>(Ipx.Global_Response_Time()) / 8, 2);

    /*.....................................................................
    Send all players the NET_GO packet.  Wait until all ACK's have been
    received.
    .....................................................................*/
    base::FillBytes(base::ObjectBytes(GPacket), 0, sizeof(GlobalPacketType));
    GPacket.Command = NET_GO;
    GPacket.ResponseTime.OneWay = MPlayerMaxAhead;
    for (i = 0; i < Players.Count(); i++) {
      Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                              sizeof(GlobalPacketType), 1,
                              &Players[i]->Address);
      /*..................................................................
      Wait for all the ACK's to come in.
      ..................................................................*/
      while (Ipx.Global_Num_Send() > 0) {
        Ipx.Service();
      }
    }

    /*.....................................................................
    Form connections with all other players.  Form the IPX Connection ID
    from the player's Color (high byte) and House (low byte).  This
    will let us extract any player's color & house at any time.
    Fill in 'tmp_id' while we're doing this.
    .....................................................................*/
    for (i = 0; i < Players.Count(); i++) {
      id = static_cast<unsigned char>(
          Build_MPlayerID(Players[i]->Player.Color, Players[i]->Player.House));

      base::At(tmp_id, i) = id;

      Ipx.Create_Connection(id, Players[i]->Name, &Players[i]->Address);
    }
    base::At(tmp_id, i) = MPlayerLocalID;

    /*.....................................................................
    Store every player's ID in the MPlayerID[] array.  This array will
    determine the order of event execution, so the ID's must be stored
    in the same order on all systems.
    .....................................................................*/
    for (i = 0; i < MPlayerCount; i++) {
      min_index = 0;
      min_id = 0xff;
      for (j = 0; j < MPlayerCount; j++) {
        if (base::At(tmp_id, j) < min_id) {
          min_id = base::At(tmp_id, j);
          min_index = j;
        }
      }
      base::At(MPlayerID, i) = base::At(tmp_id, min_index);
      base::At(tmp_id, min_index) = 0xff;
    }
    /*.....................................................................
    Fill in the array of player names, including my own.
    .....................................................................*/
    for (i = 0; i < MPlayerCount; i++) {
      if (base::At(MPlayerID, i) == MPlayerLocalID) {
        port::SafeCopy(base::At(MPlayerNames, i), MPlayerName);
      } else {
        port::SafeCopy(base::At(MPlayerNames, i),
                       Ipx.Connection_Name(base::At(MPlayerID, i)));
      }
    }
  }

  /*------------------------------------------------------------------------
  Init network timing values, using previous response times as a measure
  of what our retry delta & timeout should be.
  ------------------------------------------------------------------------*/
  Ipx.Set_Timing(Ipx.Global_Response_Time() + 2, -1,
                 Ipx.Global_Response_Time() * 4);

  /*------------------------------------------------------------------------
  Clear all lists
  ------------------------------------------------------------------------*/
  while (scenariolist.Count()) {
    scenariolist.Remove_Item(scenariolist.Get_Item(0));
  }
  Clear_Player_List(&playerlist);

  /*------------------------------------------------------------------------
  Restore screen
  ------------------------------------------------------------------------*/
  Hide_Mouse();
  Load_Title_Page(true);
  Show_Mouse();

  return rc;
}

/***************************************************************************
 * Get_NewGame_Responses -- processes packets for New Game dialog          *
 *                                                                         *
 * This routine can modify the contents of the given list box, as well * as the
 *contents of the Players Vector global.
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		playerlist		list of players in this game
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		EV_NONE = nothing happened
 ** EV_NEW_PLAYER = a new player has joined; false otherwise
 ** EV_MESSAGE = a message was received
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   04/18/1995 BRR : Created.                                             *
 *=========================================================================*/
static JoinEventType Get_NewGame_Responses(ColorListClass* playerlist) {
  char item[kGameListItemSize];  // general-purpose string
  JoinEventType retval = EV_NONE;
  char txt[80];

  /*------------------------------------------------------------------------
  If there is no incoming packet, just return
  ------------------------------------------------------------------------*/
  const int rc = Ipx.Get_Global_Message(base::ObjectBytes(GPacket), &GPacketlen,
                                        &GAddress, &GProductID);
  if (!rc || GProductID != IPXGlobalConnClass::kCommandAndConquer) {
    return EV_NONE;
  }

  /*------------------------------------------------------------------------
  Try to handle the packet in a standard way
  ------------------------------------------------------------------------*/
  if (Process_Global_Packet(&GPacket, &GAddress)) {
    return EV_NONE;
  }

  /*------------------------------------------------------------------------
  NET_QUERY_JOIN:
  ------------------------------------------------------------------------*/
  if (GPacket.Command == NET_QUERY_JOIN) {
    /*.....................................................................
    See if this name is unique:
    - If the name matches, but the address is different, reject this player
    - If the name & address match, this packet must be a re-send of a
      prevous request; in this case, do nothing.  The other player must have
      received my CONFIRM_JOIN packet (since it was sent with an ACK
      required), so we can ignore this resend.
    .....................................................................*/
    int found = 0;
    int resend = 0;
    for (int i = 0; i < Players.Count(); i++) {
      if ((std::string_view(Players[i]->Name) == GPacket.Name)) {
        if (Players[i]->Address != GAddress) {
          found = 1;
        } else {
          resend = 1;
        }
        break;
      }
    }
    if ((std::string_view(MPlayerName) == GPacket.Name)) {
      found = 1;
    }

    /*.....................................................................
    Reject if name is a duplicate, or if there are too many players:
    .....................................................................*/
    if (found || (Players.Count() >= MPlayerMax - 1 && !resend)) {
      base::FillBytes(base::ObjectBytes(GPacket), 0, sizeof(GlobalPacketType));

      GPacket.Command = NET_REJECT_JOIN;

      Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                              sizeof(GlobalPacketType), 1, &GAddress);
    }

    /*.....................................................................
    If this packet is NOT a resend, accept the player.  Grant him the
    requested color if possible.
    .....................................................................*/
    else if (!resend) {
      /*..................................................................
      Add node to the Vector list
      ..................................................................*/
      auto* who = new NodeNameType;  // node to add to Players Vector
      port::SafeCopy(who->Name, GPacket.Name);
      who->Address = GAddress;
      who->Player.House = GPacket.PlayerInfo.House;
      Players.Add(who);

      /*..................................................................
      Set player's color; if requested color isn't used, give it to him;
      otherwise, give him the 1st available color.  Mark the color we
      give him as used.
      ..................................................................*/
      if (GPacket.PlayerInfo.Color < MAX_MPLAYER_COLORS &&
          base::At(ColorUsed, GPacket.PlayerInfo.Color) == 0) {
        who->Player.Color =
            static_cast<unsigned char>(GPacket.PlayerInfo.Color);
      } else {
        for (int i = 0; i < MAX_MPLAYER_COLORS; i++) {
          if (base::At(ColorUsed, i) == 0) {
            who->Player.Color = static_cast<unsigned char>(i);
            break;
          }
        }
      }
      base::At(ColorUsed, who->Player.Color) = 1;

      /*..................................................................
      Add player name to the list box
      ..................................................................*/
      if (GPacket.PlayerInfo.House == HOUSE_GOOD) {
        absl::SNPrintF(item, sizeof(item), "%s\t%s", GPacket.Name,
                       Text_String(TXT_G_D_I));
      } else {
        absl::SNPrintF(item, sizeof(item), "%s\t%s", GPacket.Name,
                       Text_String(TXT_N_O_D));
      }
      playerlist->Add_Item(
          item, static_cast<char>(base::At(MPlayerTColors, who->Player.Color)));

      /*..................................................................
      Send a confirmation packet
      ..................................................................*/
      base::FillBytes(base::ObjectBytes(GPacket), 0, sizeof(GlobalPacketType));

      GPacket.Command = NET_CONFIRM_JOIN;
      port::SafeCopy(GPacket.Name, MPlayerName);
      GPacket.PlayerInfo.House = who->Player.House;
      GPacket.PlayerInfo.Color = who->Player.Color;

      Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                              sizeof(GlobalPacketType), 1, &GAddress);

      retval = EV_NEW_PLAYER;
    }
  }

  /*------------------------------------------------------------------------
  NET_SIGN_OFF: Another system is signing off: search for that system in
  the player list, & remove it if found
  ------------------------------------------------------------------------*/
  else if (GPacket.Command == NET_SIGN_OFF) {
    for (int i = 0; i < Players.Count(); i++) {
      /*
      ....................... Name found; remove it ......................
      */
      if ((std::string_view(Players[i]->Name) == GPacket.Name) &&
          Players[i]->Address == GAddress) {
        /*...............................................................
        Remove from the list box
        ...............................................................*/
        playerlist->Remove_Item(i + 1);
        playerlist->Flag_To_Redraw();
        /*...............................................................
        Mark his color as available
        ...............................................................*/
        base::At(ColorUsed, Players[i]->Player.Color) = 0;
        /*...............................................................
        Delete from the Vector list
        ...............................................................*/
        Players.Delete(Players[i]);
        break;
      }
    }
  }

  /*------------------------------------------------------------------------
  NET_MESSAGE: Someone is sending us a message
  ------------------------------------------------------------------------*/
  else if (GPacket.Command == NET_MESSAGE) {
    Format_Runtime_Text(txt, sizeof(txt), Text_String(TXT_FROM), GPacket.Name,
                        GPacket.Message.Buf);
    const auto magic_number =
        port::ReadUnaligned<uint16_t>(base::ObjectBytes(GPacket.Message.Buf)
                                          .subspan(COMPAT_MESSAGE_LENGTH - 4));
    const auto crc =
        port::ReadUnaligned<uint16_t>(base::ObjectBytes(GPacket.Message.Buf)
                                          .subspan(COMPAT_MESSAGE_LENGTH - 2));
    const int color =
        static_cast<int>(MPlayerID_To_ColorIndex(GPacket.Message.ID));
    Messages.Add_Message(txt, base::At(MPlayerTColors, color),
                         TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW, 1200,
                         magic_number, crc);
    retval = EV_MESSAGE;
  }

  return retval;
}

/***************************************************************************
 * Compute_Name_CRC -- computes CRC from char string                       *
 *                                                                         *
 * INPUT:                                                                  *
 *		name		string to create CRC for
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		CRC
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   06/29/1995 BRR : Created.                                             *
 *=========================================================================*/
uint32_t Compute_Name_CRC(const char* name) {
  char buf[80];
  uint32_t crc = 0L;

  port::SafeCopy(buf, name);
  strupr(buf);

  for (int i = 0; std::cmp_less(i, std::string_view(buf).size()); i++) {
    Add_CRC(&crc, static_cast<uint32_t>(base::At(buf, i)));
  }

  return crc;
}

/***************************************************************************
 * Net_Reconnect_Dialog -- Draws/updates the network reconnect dialog      *
 *                                                                         *
 * INPUT:                                                                  *
 *		reconn			1 = reconnect, 0 = waiting for
 *first-time connection	* fresh				1 = draw from scratch, 0
 *= only update time counter	* oldest_index	IPX connection index of oldest
 *connection 				* (only used for reconnection)
 ** timeval			value to print in the countdown field
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
 *   07/08/1995 BRR : Created.                                             *
 *=========================================================================*/
void Net_Reconnect_Dialog(bool reconn, bool fresh, int oldest_index,
                          int timeval) {
  static int x;
  static int y;
  static int w;
  static int h;
  char buf1[40] = {0};
  char buf2[40] = {0};

  const int factor = SeenBuff.Get_Width() == 320 ? 1 : 2;

  const int d_txt6_h = (6 * factor) + 1;
  const int d_margin = 5 * factor;

  /*------------------------------------------------------------------------
  Draw the dialog from scratch
  ------------------------------------------------------------------------*/
  if (fresh) {
    Fancy_Text_Print(
        "", 0, 0, kCcGreen, kTBlack,
        TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
    if (reconn) {
      const int id = Ipx.Connection_ID(oldest_index);
      Format_Runtime_Text(buf1, sizeof(buf1),
                          Text_String(TXT_RECONNECTING_TO),
                          Ipx.Connection_Name(id));
    } else {
      absl::SNPrintF(buf1, sizeof(buf1), "%s",
                     Text_String(TXT_WAITING_FOR_CONNECTIONS));
    }
    Format_Runtime_Text(buf2, sizeof(buf2), Text_String(TXT_TIME_ALLOWED),
                        timeval + 1);
    const char* buf3 = Text_String(TXT_PRESS_ESC);

    w = std::max<int>(String_Pixel_Width(buf1), String_Pixel_Width(buf2));
    w = std::max<int>(String_Pixel_Width(buf3), w);
    w += d_margin * 4;
    h = (d_txt6_h * 3) + (d_margin * 6);
    x = (160 * factor) - (w / 2);
    y = (100 * factor) - (h / 2);

    Hide_Mouse();
    Set_Logic_Page(SeenBuff);
    Dialog_Box(x, y, w, h);

    Fancy_Text_Print(
        buf1, 160 * factor, y + (d_margin * 2), kCcGreen, kBlack,
        TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

    Fancy_Text_Print(
        buf2, 160 * factor, y + (d_margin * 2) + d_txt6_h + d_margin, kCcGreen,
        kBlack, TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

    Fancy_Text_Print(
        buf3, 160 * factor, y + (d_margin * 2) + ((d_txt6_h + d_margin) * 2),
        kCcGreen, kBlack,
        TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

    Show_Mouse();

  } else {
    /*------------------------------------------------------------------------
    Just update the timeout value on the dialog
    ------------------------------------------------------------------------*/
    Hide_Mouse();
    Set_Logic_Page(SeenBuff);

    Format_Runtime_Text(buf2, sizeof(buf2), Text_String(TXT_TIME_ALLOWED),
                        timeval + 1);
    const int pixwidth = String_Pixel_Width(buf2);
    LogicPage->Fill_Rect((160 * factor) - (pixwidth / 2) - 12,
                         y + (d_margin * 2) + d_txt6_h + d_margin,
                         (160 * factor) + (pixwidth / 2) + 12,
                         y + (d_margin * 2) + (d_txt6_h * 2) + d_margin,
                         kTBlack);
    Fancy_Text_Print(
        buf2, 160 * factor, y + (d_margin * 2) + d_txt6_h + d_margin, kCcGreen,
        kBlack, TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

    Show_Mouse();
  }
}

/***********************************************************************************************
 * Wait_For_Focus -- Wait for game to be in focus before proceeding *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 1/6/97 3:23PM ST : Created *
 *=============================================================================================*/
static void Wait_For_Focus() {
  CountDownTimerClass focus_timer;
  focus_timer.Set(int64_t{5} * 60);
  /*
  ** Process the message loop until we are in focus.
  */
  if (!GameInFocus) {
    CCDebugString("C&C95 - Waiting for game to come into focus.");
    do {
      CCDebugString(".");
      Keyboard::Check();
      if (!focus_timer.Time()) {
        focus_timer.Set(int64_t{5} * 60);
      }

    } while (!GameInFocus);
    CCDebugString("\n");
    AllSurfaces.SurfacesRestored = false;
  }
}

/***********************************************************************************************
 * Net_Fake_New_Dialog -- Just like Net_New_Dialog but without the Dialog. For
 *internet play   *
 *                                                                                             *
 *  This 'dialog' does all the non-dialog game set up stuff that is done in the
 *normal         * network game set up dialog. The only visible button is
 *'cancel'                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   true if successfully connected *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 5/24/96 10:34AM ST : Created *
 *=============================================================================================*/
static int Net_Fake_New_Dialog() {
  const int factor = SeenBuff.Get_Width() == 320 ? 1 : 2;

  const int d_dialog_w = 120 * factor;                       // dialog width
  const int d_dialog_h = 80 * factor;                        // dialog height
  const int d_dialog_x = ((320 * factor) - d_dialog_w) / 2;  // dialog x-coord
  const int d_dialog_y = ((200 * factor) - d_dialog_h) / 2;  // centered y-coord
  const int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);     // center x-coord

  // d_playerlist_w = 100;
  const int d_playerlist_w = 106 * factor;
  const int d_playerlist_h = 27 * factor;
  // int d_playerlist_x = 10 * factor;	//off screen
  const int d_playerlist_x = 500 * factor;  // 10 * factor;	//off screen
  const int d_playerlist_y = d_dialog_y + 20;

#if (defined(GERMAN) || defined(FRENCH))
  int d_cancel_w = 50 * factor;
#else
  const int d_cancel_w = 45 * factor;
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

  bool player_joined = false;
  CountDownTimerClass join_timer;

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

  /*........................................................................
  Button Enumerations
  ........................................................................*/
  constexpr int kButtonCancel = 100;
  constexpr int kButtonPlayerlist = 101;

  /*........................................................................
  Redraw values: in order from "top" to "bottom" layer of the dialog
  ........................................................................*/
  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_MESSAGE = 1,
    REDRAW_BUTTONS = 2,
    REDRAW_BACKGROUND = 3,
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;

  /*........................................................................
  Dialog variables
  ........................................................................*/
  RedrawType display = REDRAW_ALL;  // redraw level
  bool process = true;              // process while true
  KeyNumType input = KN_NONE;

  char credbuf[CREDITSBUF_MAX];  // for credit edit box

  int64_t ok_timer = 0;  // for timing OK button
  int rc = 0;
  int i = 0;
  int j = 0;
  char item[kGameListItemSize];
  const int tabs[] = {77 * factor};  // tabs for player list box

  int64_t ping_timer = 0;  // for sending Ping packets

  unsigned char tmp_id[MAX_PLAYERS] =
      {};                    // temp storage for sorting player ID's
  int min_index = 0;         // for sorting player ID's
  unsigned char min_id = 0;  // for sorting player ID's
  unsigned char id = 0;      // connection ID
  JoinEventType whahoppa = EV_NONE;  // event generated by received packets

  std::span<const std::byte> up_button;
  std::span<const std::byte> down_button;

  if (InMainLoop) {
    up_button = Hires_Retrieve("BTN-UP.SHP");
    down_button = Hires_Retrieve("BTN-DN.SHP");
  } else {
    up_button = Hires_Retrieve("BTN-UP2.SHP");
    down_button = Hires_Retrieve("BTN-DN2.SHP");
  }

  /*........................................................................
  Buttons
  ........................................................................*/

  ColorListClass playerlist(kButtonPlayerlist, d_playerlist_x, d_playerlist_y,
                            d_playerlist_w, d_playerlist_h,
                            TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                            up_button, down_button);

  TextButtonClass cancelbtn(
      kButtonCancel, TXT_CANCEL,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
      // #if (GERMAN | FRENCH)
      //		d_cancel_x, d_cancel_y);
      // #else
      d_cancel_x, d_cancel_y, d_cancel_w, d_cancel_h);
  // #endif

  CCDebugString("C&C95 - In new game dialog - initialising lists.\n");
  /*
  ------------------------- Build the button list --------------------------
  */
  GadgetClass* commands = &playerlist;  // button list
  cancelbtn.Add_Tail(*commands);

  playerlist.Set_Tabs(tabs);

  /*
  ----------------------------- Various Inits ------------------------------
  */

  absl::SNPrintF(credbuf, sizeof(credbuf), "%d", MPlayerCredits);

  /*........................................................................
  Init other scenario parameters
  ........................................................................*/
  Special.IsTGrowth = static_cast<unsigned>(MPlayerTiberium);
  Special.IsTSpread = static_cast<unsigned>(MPlayerTiberium);
  int transmit = 0;  // 1 = re-transmit new game options

  /*........................................................................
  Init player color-used flags
  ........................................................................*/
  for (i = 0; i < MAX_MPLAYER_COLORS; i++) {
    base::At(ColorUsed, i) = 0;  // init all colors to available
  }
  base::At(ColorUsed, MPlayerColorIdx) = 1;  // set my color to used
  playerlist.Set_Selected_Style(ColorListClass::SELECT_BAR, kCcGreenShadow);

  /*........................................................................
  Init random-number generator, & create a seed to be used for all random
  numbers from here on out
  ........................................................................*/
  Seed = port::RandomSeed();

  /*------------------------------------------------------------------------
  Add myself to the list.  Note that since I'm not in the Players Vector,
  the Vector & listbox are now 1 out of sync.
  ------------------------------------------------------------------------*/
  if (MPlayerHouse == HOUSE_GOOD) {
    absl::SNPrintF(item, sizeof(item), "%s\t%s", MPlayerName,
                   Text_String(TXT_G_D_I));
  } else {
    absl::SNPrintF(item, sizeof(item), "%s\t%s", MPlayerName,
                   Text_String(TXT_N_O_D));
  }
  playerlist.Add_Item(
      item, static_cast<char>(base::At(MPlayerTColors, MPlayerColorIdx)));

  Wait_For_Focus();

  CCDebugString("C&C95 - About to uncompress title page.\n");
  Load_Title_Page(true);
  CCDebugString("C&C95 - About to set the palette.\n");
  Set_Palette(Palette);
  CCDebugString("C&C95 - Palette was set OK.\n");

  if (LogicPage != &SeenBuff && LogicPage != &HidPage) {
    CCDebugString("C&C95 - Logic page invalid");
    Set_Logic_Page(SeenBuff);
  }

  char a_buffer[128];
  absl::SNPrintF(a_buffer, sizeof(a_buffer), "Number of players:%d",
                 static_cast<int>(Players.Count()));
  CCDebugString(a_buffer);

#ifdef VIRTUAL_SUBNET_SERVER
  /*
  ** Send a bogus packet to wake up the VSS
  */
  memset(&GPacket, 0, sizeof(GlobalPacketType));

  GPacket.Command = (NetCommandType)50;  // Invalid command
  port::SafeCopy(GPacket.Name, MPlayerName);
  Ipx.Send_Global_Message(base::ObjectBytes(GPacket), sizeof(GlobalPacketType),
                          0, NULL);
#endif  // VIRTUAL_SUBNET_SERVER

  CCDebugString("C&C95 - About to reveal mouse\n");
  while (Get_Mouse_State() > 0) {
    Show_Mouse();
  }

  /*
  ---------------------------- Processing loop -----------------------------
  */
  CCDebugString("C&C95 - Entering join dialogue loop\n");
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
    ...................... Refresh display if needed ......................
    */
    if (display != REDRAW_NONE) {
      Hide_Mouse();
      /*
      .................. Redraw backgound & dialog box ...................
      */
      if (display >= REDRAW_BACKGROUND) {
        Load_Title_Page(true);
        Set_Palette(Palette);

        Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);

        /*...............................................................
        Dialog & Field labels
        ...............................................................*/
        Draw_Caption(TXT_NONE, d_dialog_x, d_dialog_y, d_dialog_w);

        Fancy_Text_Print(buffer, d_dialog_cx - (width / 2),
                         d_dialog_y + (25 * factor), kCcGreen, kTBlack,
                         TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
      }

      /*
      .......................... Redraw buttons ..........................
      */
      if (display >= REDRAW_BUTTONS) {
        commands->Draw_All();
      }

      Show_Mouse();
      display = REDRAW_NONE;
    }

    /*
    ........................... Get user input ............................
    */
    input = commands->Input();

    /*
    ---------------------------- Process input ----------------------------
    */
    switch (static_cast<int>(input)) {
      /*------------------------------------------------------------------
      CANCEL: send a SIGN_OFF, bail out with error code
      ------------------------------------------------------------------*/
      case KN_ESC:
      case ButtonKey(kButtonCancel):
        base::FillBytes(base::ObjectBytes(GPacket), 0,
                        sizeof(GlobalPacketType));

        GPacket.Command = NET_SIGN_OFF;
        port::SafeCopy(GPacket.Name, MPlayerName);

        /*...............................................................
        Broadcast my sign-off over my network
        ...............................................................*/
        Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                sizeof(GlobalPacketType), 0, nullptr);
        Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                sizeof(GlobalPacketType), 0, nullptr);
        while (Ipx.Global_Num_Send() > 0 && Ipx.Service() != 0) {
        }

        /*...............................................................
        Broadcast my sign-off over a bridged network if there is one
        ...............................................................*/
        while (Ipx.Global_Num_Send() > 0 && Ipx.Service() != 0) {
        }

        /*...............................................................
        And now, just be absolutely sure, send my sign-off to each
        player in my game.  (If there's a bridge between us, the other
        player will have specified my address, so he can cross the
        bridge; but I may not have specified a bridge address, so the
        only way I have of crossing the bridge is to send a packet
        directly to him.)
        ...............................................................*/
        for (i = 0; i < Players.Count(); i++) {
          Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                  sizeof(GlobalPacketType), 1,
                                  &Players[i]->Address);
          Ipx.Service();
        }
        while (Ipx.Global_Num_Send() > 0 && Ipx.Service() != 0) {
        }
        base::At(MPlayerGameName, 0) = 0;
        process = false;
        rc = 0;
#ifdef _WIN32
        Send_Data_To_DDE_Server("Hello", strlen("Hello"),
                                DDEServerClass::DDE_CONNECTION_FAILED);
#endif
        GameStatisticsPacketSent = false;
        Spawn_WChat(false);
        break;

      /*------------------------------------------------------------------
      default: exit loop with true status
      ------------------------------------------------------------------*/
      default:
#ifdef VIRTUAL_SUBNET_SERVER
        if (Players.Count() == InternetMaxPlayers - 1) {
#else   // VIRTUAL_SUBNET_SERVER
        if (Players.Count() > 0) {
#endif  // VIRTUAL_SUBNET_SERVER

          // char ddkks[128];
          // sprintf (ddkks, "C&C95 - Players.Count() = %d\n", Players.Count());
          // CCDebugString (ddkks);

          /*
          ** Wait for several secs after receiving request to join before
          *sending
          ** start game packet
          */
          if (!player_joined) {
            player_joined = true;
            join_timer.Set(int64_t{3} * 60, true);
            break;
          }
          if (join_timer.Time()) {
            break;
          }

          CCDebugString("C&C95 - Join timer expired\n");

          /*...............................................................
          If a new player has joined in the last second, don't allow
          an OK; force a wait longer than 2 seconds (to give all players
          a chance to know about this new guy)
          ...............................................................*/
          i = std::max<int>(static_cast<int>(Ipx.Global_Response_Time()) * 2, 120);
          while (TickCount.Time() - ok_timer < i) {
            Ipx.Service();
          }

          /*...............................................................
          If there are at least 2 players, go ahead & play; error otherwise
          ...............................................................*/
          if (MPlayerSolo || Players.Count() > 0) {
            rc = 1;
            process = false;
          } else {
            CCMessageBox().Process(TXT_ONLY_ONE, TXT_OOPS, TXT_NONE);
            display = REDRAW_ALL;
          }
        }
        break;
    }

    /*---------------------------------------------------------------------
    Process incoming packets
    ---------------------------------------------------------------------*/
    whahoppa = Get_NewGame_Responses(&playerlist);
    if (whahoppa == EV_NEW_PLAYER) {
      ok_timer = TickCount.Time();
      transmit = 1;
    } else {
      if (whahoppa == EV_MESSAGE) {
        display = REDRAW_MESSAGE;
      }
    }

    /*---------------------------------------------------------------------
    If our Transmit flag is set, we need to send out a game option packet
    ---------------------------------------------------------------------*/
    if (transmit) {
      for (i = 0; i < Players.Count(); i++) {
        base::FillBytes(base::ObjectBytes(GPacket), 0,
                        sizeof(GlobalPacketType));

        GPacket.Command = NET_GAME_OPTIONS;
        GPacket.ScenarioInfo.Scenario = static_cast<unsigned char>(
            ScenarioIdx);  // MPlayerFilenum[ScenarioIdx];
        GPacket.ScenarioInfo.Credits =
            static_cast<unsigned int>(MPlayerCredits);
        GPacket.ScenarioInfo.IsBases =
            static_cast<unsigned int>(MPlayerBases);
        GPacket.ScenarioInfo.IsTiberium =
            static_cast<unsigned int>(MPlayerTiberium);
        GPacket.ScenarioInfo.IsGoodies =
            static_cast<unsigned int>(MPlayerGoodies);
        GPacket.ScenarioInfo.IsGhosties =
            static_cast<unsigned int>(MPlayerGhosts);
        GPacket.ScenarioInfo.BuildLevel =
            static_cast<unsigned char>(BuildLevel);
        GPacket.ScenarioInfo.UnitCount =
            static_cast<unsigned char>(MPlayerUnitCount);
        GPacket.ScenarioInfo.Seed = Seed;
        GPacket.ScenarioInfo.Special = Special;
        GPacket.ScenarioInfo.GameSpeed = Options.GameSpeed;

        Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                sizeof(GlobalPacketType), 1,
                                &Players[i]->Address);
      }
      transmit = 0;
    }

    /*---------------------------------------------------------------------
    Ping every player in my game, to force the Global Channel to measure
    the connection response time.
    ---------------------------------------------------------------------*/
    if (TickCount.Time() - ping_timer > 15) {
      base::FillBytes(base::ObjectBytes(GPacket), 0, sizeof(GlobalPacketType));
      GPacket.Command = NET_PING;
      for (i = 0; i < Players.Count(); i++) {
        Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                sizeof(GlobalPacketType), 1,
                                &Players[i]->Address);
      }
      ping_timer = TickCount.Time();
    }

    /*---------------------------------------------------------------------
    Service the Ipx connections
    ---------------------------------------------------------------------*/
    Ipx.Service();

    /*---------------------------------------------------------------------
    Service the sounds & score; GameActive must be false at this point,
    so Call_Back() doesn't intercept global messages from me!
    ---------------------------------------------------------------------*/
    Call_Back();

  } /* end of while */

  CCDebugString("C&C95 - Exited process loop\n");

  /*------------------------------------------------------------------------
  Establish connections with all other players.
  ------------------------------------------------------------------------*/
  if (rc) {
    /*.....................................................................
    Set the number of players in this game, and my ID
    .....................................................................*/
    MPlayerCount = static_cast<int>(Players.Count()) + 1;
    MPlayerLocalID = static_cast<unsigned char>(
        Build_MPlayerID(MPlayerColorIdx, MPlayerHouse));

    /*.....................................................................
    Get the scenario filename
    .....................................................................*/
    Scenario = ScenarioIdx;  // PlayerFilenum[ScenarioIdx]; We are passed actual
                             // number now from wchat not index from
    // Scenario = MPlayerFilenum[ScenarioIdx];

    /*.....................................................................
    Compute frame delay value for packet transmissions:
    - Divide global channel's response time by 8 (2 to convert to 1-way
      value, 4 more to convert from ticks to frames)
    .....................................................................*/
    MPlayerMaxAhead = std::max<int>(static_cast<int>(Ipx.Global_Response_Time()) / 8, 2);

    /*.....................................................................
    Send all players the NET_GO packet.  Wait until all ACK's have been
    received.
    .....................................................................*/
    CCDebugString("C&C95 - Sending the 'GO' packet\n");
    base::FillBytes(base::ObjectBytes(GPacket), 0, sizeof(GlobalPacketType));
    GPacket.Command = NET_GO;
    GPacket.ResponseTime.OneWay = MPlayerMaxAhead;
    for (i = 0; i < Players.Count(); i++) {
      char flopbuf[128];
      absl::SNPrintF(flopbuf, sizeof(flopbuf),
                     "Sending 'GO' packet to address %d\n",
                     port::ReadUnaligned<uint16_t>(
                         base::ObjectBytes(Players[i]->Address)));
      CCDebugString(flopbuf);

      Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                              sizeof(GlobalPacketType), 1,
                              &Players[i]->Address);
      /*..................................................................
      Wait for all the ACK's to come in.
      ..................................................................*/
      while (Ipx.Global_Num_Send() > 0) {
        Ipx.Service();
      }
    }

    /*.....................................................................
    Form connections with all other players.  Form the IPX Connection ID
    from the player's Color (high byte) and House (low byte).  This
    will let us extract any player's color & house at any time.
    Fill in 'tmp_id' while we're doing this.
    .....................................................................*/
    for (i = 0; i < Players.Count(); i++) {
      id = static_cast<unsigned char>(
          Build_MPlayerID(Players[i]->Player.Color, Players[i]->Player.House));

      base::At(tmp_id, i) = id;

      Ipx.Create_Connection(id, Players[i]->Name, &Players[i]->Address);
    }

#ifdef VIRTUAL_SUBNET_SERVER
    CCDebugString("C&C95 - Creating connection to the VSS\n");
    /*
    ** Create an additional connection to the VSS
    */
    if (UseVirtualSubnetServer) {
      IPXAddressClass vss_global_address;
      NetNodeType vss_node;
      NetNumType vss_net;
      memset(vss_net, 1, sizeof(vss_net));
      memset(vss_node, 0, sizeof(vss_node));
      vss_global_address.Set_Address(vss_net, vss_node);
      Ipx.Create_Connection(VSS_ID, "VSS", &vss_global_address);
    }
#endif  // VIRTUAL_SUBNET_SERVER
    base::At(tmp_id, i) = MPlayerLocalID;

    /*.....................................................................
    Store every player's ID in the MPlayerID[] array.  This array will
    determine the order of event execution, so the ID's must be stored
    in the same order on all systems.
    .....................................................................*/
    for (i = 0; i < MPlayerCount; i++) {
      min_index = 0;
      min_id = 0xff;
      for (j = 0; j < MPlayerCount; j++) {
        if (base::At(tmp_id, j) < min_id) {
          min_id = base::At(tmp_id, j);
          min_index = j;
        }
      }
      base::At(MPlayerID, i) = base::At(tmp_id, min_index);
      base::At(tmp_id, min_index) = 0xff;
    }
    /*.....................................................................
    Fill in the array of player names, including my own.
    .....................................................................*/
    for (i = 0; i < MPlayerCount; i++) {
      if (base::At(MPlayerID, i) == MPlayerLocalID) {
        port::SafeCopy(base::At(MPlayerNames, i), MPlayerName);
      } else {
        port::SafeCopy(base::At(MPlayerNames, i),
                       Ipx.Connection_Name(base::At(MPlayerID, i)));
      }
    }
  }

  /*------------------------------------------------------------------------
  Init network timing values, using previous response times as a measure
  of what our retry delta & timeout should be.
  ------------------------------------------------------------------------*/
  Ipx.Set_Timing(Ipx.Global_Response_Time() + 2, -1,
                 Ipx.Global_Response_Time() * 4);

  /*------------------------------------------------------------------------
  Clear all lists
  ------------------------------------------------------------------------*/
  Clear_Player_List(&playerlist);

  /*------------------------------------------------------------------------
  Restore screen
  ------------------------------------------------------------------------*/
  Hide_Mouse();
  Load_Title_Page(true);
  Show_Mouse();

  if (rc) {
    Wait_For_Focus();
  }

  return rc;
}

/***********************************************************************************************
 * Net_Fake_Join_Dialog -- Like Net_Join_Dialog but with no dialogs. For
 *Internet Play.        *
 *                                                                                             *
 *  This 'dialog' does all the non-dialog game set up stuff that is done in the
 *normal         * network game set up dialog. The only visible button is
 *'cancel'                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   0 = good, -1 = bad *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 5/24/96 11:07AM ST : Created *
 *=============================================================================================*/

static int Net_Fake_Join_Dialog() {
  const int factor = SeenBuff.Get_Width() == 320 ? 1 : 2;
  /*........................................................................
  Dialog & button dimensions
  ........................................................................*/
  /* ###Change collision detected! C:\PROJECTS\CODE\NETDLG.CPP... */
  const int d_dialog_w = 120 * factor;                       // dialog width
  const int d_dialog_h = 80 * factor;                        // dialog height
  const int d_dialog_x = ((320 * factor) - d_dialog_w) / 2;  // dialog x-coord
  const int d_dialog_y = ((200 * factor) - d_dialog_h) / 2;  // centered y-coord
  const int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);     // center x-coord

  const int d_gamelist_w = 160 * factor;
  const int d_gamelist_h = 27 * factor;
  const int d_gamelist_x = 500 * factor;  // Off screen
  const int d_gamelist_y = d_dialog_y + 20;

  const int d_playerlist_w = 106 * factor;
  const int d_playerlist_h = 27 * factor;
  const int d_playerlist_x = 500 * factor;  // Off screen
  const int d_playerlist_y = d_gamelist_y + 20;

#if (defined(GERMAN) || defined(FRENCH))
  int d_cancel_w = 50 * factor;
#else
  const int d_cancel_w = 45 * factor;
#endif
  const int d_cancel_h = 9 * factor;
  const int d_cancel_x = d_dialog_cx - (d_cancel_w / 2);
  const int d_cancel_y = d_dialog_y + d_dialog_h - (20 * factor);

  bool ready_to_go = false;

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

  /*........................................................................
  Button Enumerations
  ........................................................................*/
  constexpr int kButtonCancel = 100;
  constexpr int kButtonGamelist = 101;
  constexpr int kButtonPlayerlist = 102;

  /*........................................................................
  Redraw values: in order from "top" to "bottom" layer of the dialog
  ........................................................................*/
  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_MESSAGE = 1,
    REDRAW_COLORS = 2,
    REDRAW_BUTTONS = 3,
    REDRAW_BACKGROUND = 4,
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;

  /*........................................................................
  Dialog variables
  ........................................................................*/
  RedrawType display = REDRAW_ALL;  // redraw level
  bool process = true;              // process while true
  KeyNumType input = KN_NONE;

  JoinStateType joinstate = JOIN_NOTHING;  // current "state" of this dialog
  int game_index = -1;                     // index of currently-selected game
  int join_index = -1;                     // index of game we're joining
  int rc = 0;                              // -1 = user cancelled, 1 = New
  JoinEventType event = EV_NONE;           // event from incoming packet
  int i = 0;
  int j = 0;  // loop counter

  unsigned char tmp_id[MAX_PLAYERS] =
      {};                    // temp storage for sorting player ID's
  int min_index = 0;         // for sorting player ID's
  unsigned char min_id = 0;  // for sorting player ID's
  unsigned char id = 0;      // connection ID
  char item[kGameListItemSize];
  int64_t starttime = 0;

  NodeNameType* who = nullptr;

  std::span<const std::byte> up_button;
  std::span<const std::byte> down_button;

  if (InMainLoop) {
    up_button = Hires_Retrieve("BTN-UP.SHP");
    down_button = Hires_Retrieve("BTN-DN.SHP");
  } else {
    up_button = Hires_Retrieve("BTN-UP2.SHP");
    down_button = Hires_Retrieve("BTN-DN2.SHP");
  }

  /*........................................................................
  Buttons
  ........................................................................*/
  GadgetClass* commands = nullptr;  // button list

  ColorListClass playerlist(kButtonPlayerlist, d_playerlist_x, d_playerlist_y,
                            d_playerlist_w, d_playerlist_h,
                            TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                            up_button, down_button);

  ListClass gamelist(
      kButtonGamelist, d_gamelist_x, d_gamelist_y, d_gamelist_w, d_gamelist_h,
      TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, up_button, down_button);

  TextButtonClass cancelbtn(
      kButtonCancel, TXT_CANCEL,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
      // #if (GERMAN | FRENCH)
      //		d_cancel_x, d_cancel_y);
      // #else
      d_cancel_x, d_cancel_y, d_cancel_w, d_cancel_h);
  // #endif

  /*
  ----------------------------- Various Inits ------------------------------
  */
  // MPlayerColorIdx = MPlayerPrefColor;			// init my
  // preferred color

  playerlist.Set_Selected_Style(ColorListClass::SELECT_NONE);

  Fancy_Text_Print("", 0, 0, kCcGreen, kTBlack,
                   TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

  /*
  --------------------------- Send network query ---------------------------
  */
  CCDebugString("C&C95 - About to call Send_Join_Queries.\n");
  Send_Join_Queries(game_index, 1, 0);

  Wait_For_Focus();

  CCDebugString("C&C95 - About to uncompress title page.\n");
  Load_Title_Page(true);
  CCDebugString("C&C95 - About to set the palette.\n");
  Set_Palette(Palette);
  CCDebugString("C&C95 - Palette was set OK.\n");

  if (LogicPage != &SeenBuff && LogicPage != &HidPage) {
    CCDebugString("C&C95 - Logic page invalid\n");
    Set_Logic_Page(SeenBuff);
  }

  char a_buffer[128];
  absl::SNPrintF(a_buffer, sizeof(a_buffer), "C&C95 - Number of players:%d\n",
                 static_cast<int>(Players.Count()));
  CCDebugString(a_buffer);

  /*
  ---------------------------- Init Mono Output ----------------------------
  */
  CCDebugString("C&C95 - About to reveal mouse\n");
  while (Get_Mouse_State() > 0) {
    Show_Mouse();
  }

  /*
  ---------------------------- Processing loop -----------------------------
  */
  CCDebugString("C&C95 - Entering join dialogue loop\n");
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
    ...................... Refresh display if needed ......................
    */
    if (display != REDRAW_NONE) {
      Hide_Mouse();
      /*
      .................. Redraw backgound & dialog box ...................
      */
      if (display >= REDRAW_BACKGROUND) {
        Load_Title_Page(true);
        Set_Palette(Palette);

        Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);

        /*...............................................................
        Dialog & Field labels
        ...............................................................*/
        Draw_Caption(TXT_NONE, d_dialog_x, d_dialog_y, d_dialog_w);

        Fancy_Text_Print(buffer, d_dialog_cx - (width / 2),
                         d_dialog_y + (25 * factor), kCcGreen, kTBlack,
                         TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        /*
        .................... Rebuild the button list ....................
        */
        cancelbtn.Zap();
        gamelist.Zap();
        playerlist.Zap();

        commands = &cancelbtn;
        gamelist.Add_Tail(*commands);
        playerlist.Add_Tail(*commands);
      }
      /*
      .......................... Redraw buttons ..........................
      */
      if (display >= REDRAW_BUTTONS) {
        commands->Draw_All();
      }

      Show_Mouse();
      display = REDRAW_NONE;
    }

    /*
    ........................... Get user input ............................
    */
    input = commands->Input();

    /*
    ---------------------------- Process input ----------------------------
    */
    switch (static_cast<int>(input)) {
      /*------------------------------------------------------------------
      CANCEL: send a SIGN_OFF
      - If we're part of a game, stay in this dialog; otherwise, exit
      ------------------------------------------------------------------*/
      case KN_ESC:
      case ButtonKey(kButtonCancel):
        base::FillBytes(base::ObjectBytes(GPacket), 0,
                        sizeof(GlobalPacketType));

        GPacket.Command = NET_SIGN_OFF;
        port::SafeCopy(GPacket.Name, MPlayerName);

        /*...............................................................
        If we're joined to a game, make extra sure the other players in
        that game know I'm exiting; send my SIGN_OFF as an ack-required
        packet.  Do not send this packet to myself (index 0).
        ...............................................................*/
        if (joinstate == JOIN_CONFIRMED) {
          //
          // Remove myself from the player list box
          //
          playerlist.Remove_Item(0);
          playerlist.Flag_To_Redraw();

          //
          // Remove myself from the Players list
          //
          who = Players[0];
          Players.Delete(0);
          delete who;

          for (i = 0; i < Players.Count(); i++) {
            Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                    sizeof(GlobalPacketType), 1,
                                    &Players[i]->Address);
            Ipx.Service();
          }
        }

        /*...............................................................
        Now broadcast my SIGN_OFF so other players looking at this game
        know I'm leaving.
        ...............................................................*/
        Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                sizeof(GlobalPacketType), 0, nullptr);
        Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                sizeof(GlobalPacketType), 0, nullptr);

        if (IsBridge) {
          Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                  sizeof(GlobalPacketType), 0, &BridgeNet);
          Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                  sizeof(GlobalPacketType), 0, &BridgeNet);
        }

        while (Ipx.Global_Num_Send() > 0 && Ipx.Service() != 0) {
        }

#ifdef _WIN32
        Send_Data_To_DDE_Server("Hello", strlen("Hello"),
                                DDEServerClass::DDE_CONNECTION_FAILED);
#endif
        GameStatisticsPacketSent = false;
        Spawn_WChat(false);
        process = false;
        rc = -1;
        break;

      /*------------------------------------------------------------------
      JOIN: send a join request packet & switch to waiting-for-confirmation
      mode.  (Request_To_Join fills in MPlayerName with my namebuf.)
      ------------------------------------------------------------------*/
      default:
        if (joinstate == JOIN_NOTHING && Games.Count() != 0) {
          gamelist.Set_Selected_Index(0);
          join_index = gamelist.Current_Index();
          if (Request_To_Join(MPlayerName, join_index, &playerlist,
                              MPlayerHouse, MPlayerColorIdx)) {
            joinstate = JOIN_WAIT_CONFIRM;
          } else {
            display = REDRAW_ALL;
          }
        }
        break;
    }

    /*---------------------------------------------------------------------
    Resend our query packets
    ---------------------------------------------------------------------*/
    Send_Join_Queries(game_index, 0, 0);

    /*---------------------------------------------------------------------
    Process incoming packets
    ---------------------------------------------------------------------*/
    event = Get_Join_Responses(&joinstate, &gamelist, &playerlist, join_index);
    /*.....................................................................
    If we've changed state, redraw everything; if we're starting the game,
    break out of the loop.  If we've just joined, send out a player query
    so I'll get added to the list instantly.
    .....................................................................*/
    if (event == EV_STATE_CHANGE) {
      display = REDRAW_ALL;
      if (joinstate == JOIN_GAME_START) {
        CCDebugString("C&C95 - Received 'GO' packet\n");

        ready_to_go = true;
      } else {
        /*..................................................................
        If we're newly-confirmed, immediately send out a player query
        ..................................................................*/
        if (joinstate == JOIN_CONFIRMED) {
          Clear_Player_List(&playerlist);

          if (MPlayerHouse == HOUSE_GOOD) {
            absl::SNPrintF(item, sizeof(item), "%s\t%s", MPlayerName,
                           Text_String(TXT_G_D_I));
          } else {
            absl::SNPrintF(item, sizeof(item), "%s\t%s", MPlayerName,
                           Text_String(TXT_N_O_D));
          }
          playerlist.Add_Item(item, static_cast<char>(base::At(
                                        MPlayerTColors, MPlayerColorIdx)));

          who = new NodeNameType;
          port::SafeCopy(who->Name, MPlayerName);
          who->Address = IPXAddressClass();
          who->Player.House = MPlayerHouse;
          who->Player.Color = static_cast<unsigned char>(MPlayerColorIdx);
          Players.Add(who);

          Send_Join_Queries(game_index, 0, 1);
        } else {
          /*..................................................................
          If we've been rejected, clear any messages we may have been typing.
          ..................................................................*/
          if (joinstate == JOIN_REJECTED) {
            //
            // Remove myself from the player list box
            //
            playerlist.Remove_Item(0);
            playerlist.Flag_To_Redraw();

            //
            // Remove myself from the Players list
            //
            if (Players.Count()) {
              who = Players[0];
              Players.Delete(0);
              delete who;
            }
          }
        }
      }
    } else

      /*.....................................................................
      If a new game is detected, and it's the first game on our list,
      automatically send out a player query for that game.
      .....................................................................*/
      if (event == EV_NEW_GAME && gamelist.Count() == 1) {
        gamelist.Set_Selected_Index(0);
        game_index = gamelist.Current_Index();
        Send_Join_Queries(game_index, 0, 1);
      } else

        /*.....................................................................
        If the game options have changed, print them; likewise draw an
        incoming message.
        .....................................................................*/
        if (event == EV_GAME_OPTIONS || event == EV_MESSAGE) {
          display = REDRAW_MESSAGE;
        } else

          /*.....................................................................
          A game before the one I've selected is gone, so we have a new index
          now. 'game_index' must be kept set to the currently-selected list
          item, so we send out queries for the currently-selected game.  It's
          therefore imperative that we detect any changes to the game list. If
          we're joined in a game, we must decrement our game_index to keep it
          aligned with the game we're joined to.
          .....................................................................*/
          if (event == EV_GAME_SIGNOFF) {
            if (joinstate == JOIN_CONFIRMED) {
              game_index--;
              join_index--;
              gamelist.Set_Selected_Index(join_index);
            } else {
              gamelist.Flag_To_Redraw();
              Clear_Player_List(&playerlist);
              game_index = gamelist.Current_Index();
              Send_Join_Queries(game_index, 0, 1);
            }
          }

    /*---------------------------------------------------------------------
    Service the Ipx connections
    ---------------------------------------------------------------------*/
    Ipx.Service();

    /*---------------------------------------------------------------------
    Clean out the Game List; if an old entry is found:
    - Remove it
    - Clear the player list
    - Send queries for the new selected game, if there is one
    ---------------------------------------------------------------------*/
    for (i = 0; i < Games.Count(); i++) {
      if (TickCount.Time() - Games[i]->Game.LastTime > 400) {
        Games.Delete(Games[i]);
        gamelist.Remove_Item(i);
        if (i <= game_index) {
          gamelist.Flag_To_Redraw();
          Clear_Player_List(&playerlist);
          game_index = gamelist.Current_Index();
          Send_Join_Queries(game_index, 0, 1);
        }
      }
    }

    /*
    ** If we were flagged to start the game and we recognise both players then
    *quit the loop
    */

    // char ddkks[128];
    // sprintf (ddkks, "C&C95 - Players.Count() = %d\n", Players.Count());
    // CCDebugString (ddkks);
    if (ready_to_go) {  // && Players.Count() == InternetMaxPlayers){
      rc = 0;
      process = false;
    }

    /*---------------------------------------------------------------------
    Service the sounds & score; GameActive must be false at this point,
    so Call_Back() doesn't intercept global messages from me!
    ---------------------------------------------------------------------*/
    Call_Back();
  }

  /*------------------------------------------------------------------------
  Establish connections with all other players.
  ------------------------------------------------------------------------*/
  if (rc == 0) {
    /*.....................................................................
    If the other guys are playing a scenario I don't have (sniff), I can't
    play.  Try to bail gracefully.
    .....................................................................*/
    if (ScenarioIdx == -1) {
      CCMessageBox().Process(TXT_UNABLE_PLAY_WAAUGH);

      //
      // Remove myself from the player list box
      //
      playerlist.Remove_Item(0);
      playerlist.Flag_To_Redraw();

      //
      // Remove myself from the Players list
      //
      who = Players[0];
      Players.Delete(0);
      delete who;

      base::FillBytes(base::ObjectBytes(GPacket), 0, sizeof(GlobalPacketType));

      GPacket.Command = NET_SIGN_OFF;
      port::SafeCopy(GPacket.Name, MPlayerName);

      for (i = 0; i < Players.Count(); i++) {
        Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                sizeof(GlobalPacketType), 1,
                                &Players[i]->Address);
        Ipx.Service();
      }

      Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                              sizeof(GlobalPacketType), 0, nullptr);
      Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                              sizeof(GlobalPacketType), 0, nullptr);

      if (IsBridge) {
        Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                sizeof(GlobalPacketType), 0, &BridgeNet);
        Ipx.Send_Global_Message(base::ObjectBytes(GPacket),
                                sizeof(GlobalPacketType), 0, &BridgeNet);
      }

      while (Ipx.Global_Num_Send() > 0 && Ipx.Service() != 0) {
      }

      rc = -1;

    } else {
      /*..................................................................
      Set the number of players in this game, and my ID
      ..................................................................*/
      MPlayerCount = static_cast<int>(Players.Count());
      MPlayerLocalID = static_cast<unsigned char>(
          Build_MPlayerID(MPlayerColorIdx, MPlayerHouse));

      /*..................................................................
      Get the scenario number
      ..................................................................*/
      Scenario = ScenarioIdx;  // PlayerFilenum[ScenarioIdx]; We are passed
                               // actual number now from wchat not index from

      /*..................................................................
      Form connections with all other players.  Form the IPX Connection ID
      from the player's Color and House.  This will let us extract any
      player's color & house at any time.  Fill in 'tmp_id' while we're
      doing this.
      ..................................................................*/
      for (i = 0; i < Players.Count(); i++) {
        /*...............................................................
        Only create the connection if it's not myself!
        ...............................................................*/
        if (std::string_view(MPlayerName) != Players[i]->Name) {
          id = static_cast<unsigned char>(Build_MPlayerID(
              Players[i]->Player.Color, Players[i]->Player.House));

          base::At(tmp_id, i) = id;

          Ipx.Create_Connection(id, Players[i]->Name, &Players[i]->Address);
        } else {
          base::At(tmp_id, i) = MPlayerLocalID;
        }
      }

#ifdef VIRTUAL_SUBNET_SERVER
      /*
      ** Create an additional connection to the VSS
      */
      if (UseVirtualSubnetServer) {
        IPXAddressClass vss_global_address;
        NetNodeType vss_node;
        NetNumType vss_net;
        memset(vss_net, 1, sizeof(vss_net));
        memset(vss_node, 0, sizeof(vss_node));
        vss_global_address.Set_Address(vss_net, vss_node);
        Ipx.Create_Connection(VSS_ID, "VSS", &vss_global_address);
      }
#endif  // VIRTUAL_SUBNET_SERVER

      /*..................................................................
      Store every player's ID in the MPlayerID[] array.  This array will
      determine the order of event execution, so the ID's must be stored
      in the same order on all systems.
      ..................................................................*/
      for (i = 0; i < MPlayerCount; i++) {
        min_index = 0;
        min_id = 0xff;
        for (j = 0; j < MPlayerCount; j++) {
          if (base::At(tmp_id, j) < min_id) {
            min_id = base::At(tmp_id, j);
            min_index = j;
          }
        }
        base::At(MPlayerID, i) = base::At(tmp_id, min_index);
        base::At(tmp_id, min_index) = 0xff;
      }
      /*..................................................................
      Fill in the array of player names, including my own.
      ..................................................................*/
      for (i = 0; i < MPlayerCount; i++) {
        if (base::At(MPlayerID, i) == MPlayerLocalID) {
          port::SafeCopy(base::At(MPlayerNames, i), MPlayerName);
        } else {
          port::SafeCopy(base::At(MPlayerNames, i),
                         Ipx.Connection_Name(base::At(MPlayerID, i)));
        }
      }
    }
    /*---------------------------------------------------------------------
    Wait a while, polling the IPX service routines, to give our ACK
    a chance to get to the other system.  If he doesn't get our ACK, he'll
    be waiting the whole time we load MIX files.
    ---------------------------------------------------------------------*/
    i = std::max<int>(static_cast<int>(Ipx.Global_Response_Time()) * 2, 120);
    starttime = TickCount.Time();
    while (TickCount.Time() - starttime < static_cast<int64_t>(i)) {
      Ipx.Service();
    }
  }

  /*------------------------------------------------------------------------
  Init network timing values, using previous response times as a measure
  of what our retry delta & timeout should be.
  ------------------------------------------------------------------------*/
  Ipx.Set_Timing(Ipx.Global_Response_Time() + 2, -1,
                 Ipx.Global_Response_Time() * 4);

  /*------------------------------------------------------------------------
  Clear all lists
  ------------------------------------------------------------------------*/
  Clear_Game_List(&gamelist);
  Clear_Player_List(&playerlist);

  /*------------------------------------------------------------------------
  Restore screen
  ------------------------------------------------------------------------*/
  Hide_Mouse();
  Load_Title_Page(true);
  Show_Mouse();

  if (rc != -1) {
    Wait_For_Focus();
  }

  return rc;
}

#endif
