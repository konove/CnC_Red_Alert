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

/* $Header:   F:\projects\c&c\vcs\code\ipxmgr.cpv   1.9   16 Oct 1995 16:48:22
 * JOE_BOSTIC  $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : IPXMGR.CPP                               *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : December 20, 1994                        *
 *                                                                         *
 *                  Last Update : May 4, 1995 [BRR]                        *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions: * IPXManagerClass::IPXManagerClass -- class constructor *
 *   IPXManagerClass::~IPXManagerClass -- class destructor                 *
 *   IPXManagerClass::Init -- initialization routine
 ** IPXManagerClass::Is_IPX -- tells if IPX is installed or not
 ** IPXManagerClass::Set_Timing -- sets timing for all connections
 ** IPXManagerClass::Create_Connection -- creates a new connection        *
 *   IPXManagerClass::Delete_Connection -- deletes a connection            *
 *   IPXManagerClass::Num_Connections -- gets the # of connections
 ** IPXManagerClass::Connection_ID -- gets the given connection's ID      *
 *   IPXManagerClass::Connection_Name -- gets name for given connection    *
 *   IPXManagerClass::Connection_Address -- retrieves connection's address
 ** IPXManagerClass::Connection_Index -- gets given connection's index    *
 *   IPXManagerClass::Send_Global_Message -- sends a Global Message
 ** IPXManagerClass::Get_Global_Message -- polls the Global Message queue
 ** IPXManagerClass::Send_Private_Message -- Sends a Private Message *
 *   IPXManagerClass::Get_Private_Message -- Polls Private Message queue
 ** IPXManagerClass::Service -- main polling routine for IPX Connections
 ** IPXManagerClass::Get_Bad_Connection -- returns bad connection ID      *
 *   IPXManagerClass::Global_Num_Send	-- gets # entries in send queue	   *
 *   IPXManagerClass::Global_Num_Receive -- gets # entries in recv queue   *
 *   IPXManagerClass::Private_Num_Send -- gets # entries in send queue	   *
 *   IPXManagerClass::Private_Num_Receive -- gets # entries in recv queue  *
 *   IPXManagerClass::Set_Bridge -- prepares to cross a bridge             *
 *   IPXManagerClass::Set_Socket -- sets socket ID for all connections *
 *   IPXManagerClass::Response_Time -- Returns largest Avg Response Time   *
 *   IPXManagerClass::Global_Response_Time -- Returns Avg Response Time    *
 *   IPXManagerClass::Reset_Response_Time -- Reset response time
 ** IPXManagerClass::Oldest_Send -- gets ptr to oldest send buf           *
 *   IPXManagerClass::Alloc_RealMode_Mem -- allocates real-mode memory *
 *   IPXManagerClass::Free_RealMode_Mem -- frees real-mode memory
 **
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "td/net/ipxmgr.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <utility>

#include "absl/log/log.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/unaligned.h"
#include "engine/net/combuf.h"
#include "td/core/defines.h"
#include "td/net/connect.h"
#include "td/net/ipx.h"
#include "td/net/ipxaddr.h"
#include "td/net/ipxconn.h"
#include "td/net/ipxgconn.h"
#include "td/net/network.h"
#include "td/net/session.h"
#include "td/net/tcpip.h"

namespace td {

/***************************************************************************
 * IPXManagerClass::IPXManagerClass -- class constructor                   *
 *                                                                         *
 * INPUT:                                                                  *
 *		glb_maxlen		Global Channel maximum packet length
 ** pvt_maxlen		Private Channel maximum packet length
 ** socket			socket ID to use
 ** product_id		a unique numerical ID for this product
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		The socket number is byte-swapped, since IPX requires socket
 *ID's		* to be stored high/low.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
IPXManagerClass::IPXManagerClass(int glb_maxlen, int pvt_maxlen,
                                 int glb_num_packets, int pvt_num_packets,
                                 uint16_t socket, uint16_t product_id)
    // IPX requires socket IDs stored high/low, so the socket is byte-swapped.
    : Glb_MaxPacketLen(glb_maxlen),
      Glb_NumPackets(glb_num_packets),
      Pvt_MaxPacketLen(pvt_maxlen),
      Pvt_NumPackets(pvt_num_packets),
      ProductID(product_id),
      Socket(static_cast<uint16_t>(
          (static_cast<uint32_t>(socket) & 0x00ffU) << 8 |
          (static_cast<uint32_t>(socket) & 0xff00U) >> 8)) {}

/***************************************************************************
 * IPXManagerClass::~IPXManagerClass -- class destructor                   *
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
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
IPXManagerClass::~IPXManagerClass() {

  /*------------------------------------------------------------------------
  Stop all IPX events
  ------------------------------------------------------------------------*/
  Stop_Listening();

  /*------------------------------------------------------------------------
  Free all protected-mode memory
  ------------------------------------------------------------------------*/
  GlobalChannel.reset();
  for (int i = 0; i < NumConnections; i++) {
    base::At(Connection, i).reset();
  }
  NumConnections = 0;

  /*------------------------------------------------------------------------
  Free all real-mode memory
  ------------------------------------------------------------------------*/
  if (RealMemAllocd) {
    Free_RealMode_Mem();
    RealMemAllocd = false;
  }

} /* end of ~IPXManagerClass */

/***************************************************************************
 * IPXManagerClass::Init -- initialization routine
 **
 *                                                                         *
 * This routine allocates memory,
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = error
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
int IPXManagerClass::Init() {
  if (!(TheSession().type() == GAME_INTERNET)) {
    /*
    ----------------------- Error if IPX not installed -----------------------
    */
    if (!IPXStatus) {
      return 0;
    }

    /*------------------------------------------------------------------------
    Stop Listening
    ------------------------------------------------------------------------*/
    Stop_Listening();

    /*------------------------------------------------------------------------
    Free Real-mode memory
    ------------------------------------------------------------------------*/
    if (RealMemAllocd) {
      Free_RealMode_Mem();
      RealMemAllocd = false;
    }
  } else {
    /*
    ** Pretend IPX is available for Internet games whether it is or not
    */
    IPXStatus = true;
  }

  /*------------------------------------------------------------------------
  Free protected-mode memory
  ------------------------------------------------------------------------*/
  GlobalChannel.reset();
  for (int i = 0; i < NumConnections; i++) {
    base::At(Connection, i).reset();
  }
  NumConnections = 0;

  if (!(TheSession().type() == GAME_INTERNET)) {
    /*------------------------------------------------------------------------
    Allocate real-mode memory
    ------------------------------------------------------------------------*/
    if (!Alloc_RealMode_Mem()) {
      return 0;
    }
    RealMemAllocd = true;
  }

  /*------------------------------------------------------------------------
  Allocate the Global Channel
  ------------------------------------------------------------------------*/
  GlobalChannel = std::make_unique<IPXGlobalConnClass>(
      Glb_NumPackets, Glb_NumPackets, Glb_MaxPacketLen, ProductID);
  if (!GlobalChannel) {
    return 0;
  }
  GlobalChannel->Init();
  GlobalChannel->Set_Retry_Delta(RetryDelta);
  GlobalChannel->Set_Max_Retries(MaxRetries);
  GlobalChannel->Set_TimeOut(Timeout);

  /*------------------------------------------------------------------------
  Configure the IPX Connections
  ------------------------------------------------------------------------*/
  IPXConnClass::Configure(Socket, ConnectionNum, ListenECB, SendECB,
                          FirstHeaderBuf, SendHeader, FirstDataBuf, SendBuf,
                          Handler, PacketLen);

  /*------------------------------------------------------------------------
  Start Listening
  ------------------------------------------------------------------------*/
  if ((!(TheSession().type() == GAME_INTERNET)) &&
      (!IPXConnClass::Start_Listening())) {
    return 0;
  }

  Listening = true;

  return 1;
}

void IPXManagerClass::Stop_Listening() {
  if (Listening) {
    IPXConnClass::Stop_Listening();
    Listening = false;
  }
}

/***************************************************************************
 * IPXManagerClass::Is_IPX -- tells if IPX is installed or not
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = IPX is installed; 0 = isn't
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
int IPXManagerClass::Is_IPX() const { return IPXStatus; } /* end of Is_IPX */

/***************************************************************************
 * IPXManagerClass::Set_Timing -- sets timing for all connections
 **
 *                                                                         *
 * This will set the timing parameters for all existing connections, and
 ** all connections created from now on.  This allows an application to *
 * measure the Response_Time while running, and adjust timing accordingly.
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		retrydelta	value to set for retry delta
 ** maxretries	value to set for max # retries
 ** timeout		value to set for connection timeout
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
 *   07/02/1995 BR : Created.                                              *
 *=========================================================================*/
void IPXManagerClass::Set_Timing(int32_t retrydelta, int32_t maxretries,
                                 int32_t timeout) {

  RetryDelta = retrydelta;
  MaxRetries = maxretries;
  Timeout = timeout;

  if (GlobalChannel) {
    GlobalChannel->Set_Retry_Delta(RetryDelta);
    GlobalChannel->Set_Max_Retries(MaxRetries);
    GlobalChannel->Set_TimeOut(Timeout);
  }

  for (int i = 0; i < NumConnections; i++) {
    base::At(Connection, i)->Set_Retry_Delta(RetryDelta);
    base::At(Connection, i)->Set_Max_Retries(MaxRetries);
    base::At(Connection, i)->Set_TimeOut(Timeout);
  }

} /* end of Set_Timing */

/***************************************************************************
 * IPXManagerClass::Create_Connection -- creates a new connection          *
 *                                                                         *
 * INPUT:                                                                  *
 *		id				application-specific numerical
 *ID for this connection		* node			ptr to
 *IPXNodeIDType (name & address)
 ** address		IPX address for this connection
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = error
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		Never create a connection with an 'id' of -1.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
bool IPXManagerClass::Create_Connection(int id, char* name,
                                        IPXAddressClass* address) {
  /*
  ----------------------- Error if IPX not installed -----------------------
  */
  if (!IPXStatus) {
    return false;
  }

  /*
  ------------------------- Error if no more room --------------------------
  */
  if (NumConnections == CONNECT_MAX) {
    return false;
  }

  /*
  ------------------------- Create new connection --------------------------
  */
  base::At(Connection, NumConnections) = std::make_unique<IPXConnClass>(
      Pvt_NumPackets, Pvt_NumPackets, Pvt_MaxPacketLen, ProductID, address, id,
      name);
  if (!base::At(Connection, NumConnections)) {
    return false;
  }

  base::At(Connection, NumConnections)->Init();
  base::At(Connection, NumConnections)->Set_Retry_Delta(RetryDelta);
  base::At(Connection, NumConnections)->Set_Max_Retries(MaxRetries);
  base::At(Connection, NumConnections)->Set_TimeOut(Timeout);

  NumConnections++;

  return true;

} /* end of Create_Connection */

/***************************************************************************
 * IPXManagerClass::Delete_Connection -- deletes a connection              *
 *                                                                         *
 * INPUT:                                                                  *
 *		id		ID of connection to delete
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = error
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
bool IPXManagerClass::Delete_Connection(int id) {

  /*
  ----------------------- Error if IPX not installed -----------------------
  */
  if (!IPXStatus) {
    return false;
  }

  /*
  ------------------- Error if no connections to delete --------------------
  */
  if (NumConnections == 0) {
    return false;
  }

  /*
  ---------------------- Loop through all connections ----------------------
  */
  for (int i = 0; i < NumConnections; i++) {
    /*
    ........................ If a match, delete it ........................
    */
    if (base::At(Connection, i)->ID == id) {
      base::At(Connection, i).reset();
      /*
      ................ Move array elements back one index ................
      */
      for (int j = i; j < NumConnections - 1; j++) {
        base::At(Connection, j) = std::move(base::At(Connection, j + 1));
      }
      /*
      ......................... Adjust counters ..........................
      */
      NumConnections--;
      if (CurConnection >= NumConnections) {
        CurConnection = 0;
      }
      return true;
    }
  }

  /*
  ---------------------------- No match; error -----------------------------
  */
  return false;

} /* end of Delete_Connection */

/***************************************************************************
 * IPXManagerClass::Num_Connections -- gets the # of connections
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		# of connections
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   01/25/1995 BR : Created.                                              *
 *=========================================================================*/
int IPXManagerClass::Num_Connections() {

  return NumConnections;

} /* end of Num_Connections */

/***************************************************************************
 * IPXManagerClass::Connection_ID -- gets the given connection's ID        *
 *                                                                         *
 * INPUT:                                                                  *
 *		index			index of connection to retrieve
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		ID for that connection, kConnectionNone if invalid index
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   01/25/1995 BR : Created.                                              *
 *=========================================================================*/
int IPXManagerClass::Connection_ID(int index) {
  if (index >= 0 && index < NumConnections) {
    return base::At(Connection, index)->ID;
  }
  return IPXConnClass::kConnectionNone;
}

/***************************************************************************
 * IPXManagerClass::Connection_Name -- retrieves name for given connection *
 *                                                                         *
 * INPUT:                                                                  *
 *		id		ID of connection to get name of
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		ptr to connection's name, NULL if not found
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   01/19/1995 BR : Created.                                              *
 *=========================================================================*/
char* IPXManagerClass::Connection_Name(int id) {
  for (int i = 0; i < NumConnections; i++) {
    if (base::At(Connection, i)->ID == id) {
      return base::At(Connection, i)->Name;
    }
  }

  return nullptr;

} /* end of Connection_Name */

/***************************************************************************
 * IPXManagerClass::Connection_Address -- retrieves connection's address
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		id		ID of connection to get address for
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		pointer to IXPAddressClass, NULL if not found
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   01/19/1995 BR : Created.                                              *
 *=========================================================================*/
IPXAddressClass* IPXManagerClass::Connection_Address(int id) {
  for (int i = 0; i < NumConnections; i++) {
    if (base::At(Connection, i)->ID == id) {
      return &base::At(Connection, i)->Address;
    }
  }

  return nullptr;

} /* end of Connection_Address */

/***************************************************************************
 * IPXManagerClass::Connection_Index -- gets given connection's index      *
 *                                                                         *
 * INPUT:                                                                  *
 *		ID to retrieve index for
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		index for this connection, kConnectionNone if not found
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   01/25/1995 BR : Created.                                              *
 *=========================================================================*/
int IPXManagerClass::Connection_Index(int id) {
  for (int i = 0; i < NumConnections; i++) {
    if (base::At(Connection, i)->ID == id) {
      return i;
    }
  }

  return IPXConnClass::kConnectionNone;

} /* end of Connection_Index */

/***************************************************************************
 * IPXManagerClass::Send_Global_Message -- sends a Global Message
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		buf			buffer to send
 ** buflen		length of buf
 ** ack_req		1 = ACK required; 0 = no ACK required
 ** address		address to send to; NULL = broadcast
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = error
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   01/25/1995 BR : Created.                                              *
 *=========================================================================*/
int IPXManagerClass::Send_Global_Message(std::span<const std::byte> buf,
                                         int buflen, int ack_req,
                                         IPXAddressClass* address) {
  /*
  ------------ Error if IPX not installed or not Listening -----------------
  */
  if (!IPXStatus || !Listening) {
    return 0;
  }

  const int rc = GlobalChannel->Send_Packet(buf, buflen, address, ack_req);
  if (!rc) {
    DLOG(WARNING) << "IPX global send queue overflow; packet dropped";
  }

  return rc;
}

/***************************************************************************
 * IPXManagerClass::Get_Global_Message -- polls the Global Message queue
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		buf				buffer to store received packet
 ** buflen			length of data placed in 'buf'
 ** address			IPX address of sender
 ** product_id		product ID of sender
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = error
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   01/25/1995 BR : Created.                                              *
 *=========================================================================*/
int IPXManagerClass::Get_Global_Message(std::span<std::byte> buf, int* buflen,
                                        IPXAddressClass* address,
                                        uint16_t* product_id) {
  /*
  ------------ Error if IPX not installed or not Listening -----------------
  */
  if (!IPXStatus || !Listening) {
    return 0;
  }

  return GlobalChannel->Get_Packet(buf, buflen, address, product_id);
}

/***************************************************************************
 * IPXManagerClass::Send_Private_Message -- Sends a Private Message
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		buf			buffer to send
 ** buflen		length of 'buf'
 ** conn_id		connection ID to send to (kConnectionNone = all)
 ** ack_req		1 = ACK required; 0 = no ACK required
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = error
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   01/25/1995 BR : Created.                                              *
 *=========================================================================*/
int IPXManagerClass::Send_Private_Message(std::span<const std::byte> buf,
                                          int buflen, int ack_req,
                                          int conn_id) {
  int i = 0;            // loop counter
  int connect_idx = 0;  // index of channel to send to, if specified

  /*
  ------------ Error if IPX not installed or not Listening -----------------
  */
  if (!IPXStatus || !Listening || NumConnections == 0) {
    return 0;
  }

  /*------------------------------------------------------------------------
  Send the message to all connections
  ------------------------------------------------------------------------*/
  if (conn_id == IPXConnClass::kConnectionNone) {
    /*
    ** If this is an internet game and no ack is reqired then we only need to
    *send *	 the packet to the VSS and it will forward it to all the other
    *players.
    */

      /*.....................................................................
      Check for room in all connections
      .....................................................................*/
      for (i = 0; i < NumConnections; i++) {
          if (base::At(Connection, i)->Queue->Num_Send() ==
              base::At(Connection, i)->Queue->Max_Send()) {
            DLOG(WARNING) << "IPX send queue full for connection " << i
                          << "; broadcast dropped";
            return 0;
          }
      }

      /*.....................................................................
      Send packet to all connections
      .....................................................................*/

      for (i = 0; i < NumConnections; i++) {
          base::At(Connection, i)->Send_Packet(buf, buflen, ack_req);
      }

    return 1;
  }
  /*------------------------------------------------------------------------
      Send the message to the specified connection
      ------------------------------------------------------------------------*/
  connect_idx = Connection_Index(conn_id);
  if (connect_idx == IPXConnClass::kConnectionNone) {
    DLOG(WARNING) << "IPX send to unknown connection " << conn_id;
    return 0;
  }

  /*.....................................................................
  Check for room in the connection
  .....................................................................*/
  if (base::At(Connection, connect_idx)->Queue->Num_Send() ==
      base::At(Connection, connect_idx)->Queue->Max_Send()) {
    DLOG(WARNING) << "IPX send queue full for connection " << connect_idx
                  << "; packet dropped";
    return 0;
  }

  /*.....................................................................
  Send the packet to that connection
  .....................................................................*/
  base::At(Connection, connect_idx)->Send_Packet(buf, buflen, ack_req);
  return 1;
}

/***************************************************************************
 * IPXManagerClass::Get_Private_Message -- Polls the Private Message queue
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		buf			buffer to store incoming packet
 ** buflen		length of data placed in 'buf'
 ** conn_id		filled in with connection ID of sender
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = error
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   01/25/1995 BR : Created.                                              *
 *=========================================================================*/
int IPXManagerClass::Get_Private_Message(std::span<std::byte> buf, int* buflen,
                                         int* conn_id) {
  int i = 0;
  int rc = 0;
  int c_id = 0;

  /*
  ------------ Error if IPX not installed or not Listening -----------------
  */
  if (!IPXStatus || !Listening || NumConnections == 0) {
    return 0;
  }

  /*------------------------------------------------------------------------
  Safety check: ensure CurConnection is in range.
  ------------------------------------------------------------------------*/
  if (CurConnection >= NumConnections) {
    CurConnection = 0;
  }

  /*------------------------------------------------------------------------
  Scan all connections for a received packet, starting with 'CurConnection'
  ------------------------------------------------------------------------*/
  for (i = 0; i < NumConnections; i++) {
    /*.....................................................................
    Check this connection for a packet
    .....................................................................*/
    rc = base::At(Connection, CurConnection)->Get_Packet(buf, buflen);
    c_id = base::At(Connection, CurConnection)->ID;

    /*.....................................................................
    Increment CurConnection to the next connection index
    .....................................................................*/
    CurConnection++;
    if (CurConnection >= NumConnections) {
      CurConnection = 0;
    }

    /*.....................................................................
    If we got a packet, return the connection ID
    .....................................................................*/
    if (rc) {
      *conn_id = c_id;
      return 1;
    }
  }

  return 0;
}

/***************************************************************************
 * IPXManagerClass::Service -- main polling routine for IPX Connections *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = error
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   01/25/1995 BR : Created.                                              *
 *=========================================================================*/
int IPXManagerClass::Service() {
  int rc = 1;
  int i = 0;
  CommHeaderType packet_storage{};
  CommHeaderType* packet = &packet_storage;
  int packetlen = 0;
  IPXAddressClass address;

  unsigned char temp_receive_buffer[1024];
  int recv_length = 0;

  // Cursors into temp_receive_buffer above.
  IPXHeaderType* cur_header_buf = nullptr;
  std::span<std::byte> cur_data_buf;

  if (TheNetwork().winsock().Get_Connected()) {
    while ((recv_length = TheNetwork().winsock().Read(
                base::ObjectBytes(temp_receive_buffer), 1024)) != 0) {
      cur_header_buf = nullptr;
      cur_data_buf = base::ObjectBytes(temp_receive_buffer);

      /*.....................................................................
      Compute the length of the packet (byte-swap the length in the IPX hdr)
      .....................................................................*/
      packetlen = recv_length;

      /*.....................................................................
      Extract the sender's address from the IPX header
      .....................................................................*/
      address.Set_Address(cur_header_buf);

      /*.....................................................................
      Examine the Magic Number of the received packet to determine if this
      packet goes into the Global Queue, or into one of the Private Queues
      .....................................................................*/
      if (std::cmp_less(packetlen, sizeof(CommHeaderType)) ||
          base::ToSize(packetlen) > cur_data_buf.size()) {
        continue;
      }
      cur_data_buf = cur_data_buf.first(base::ToSize(packetlen));
      packet_storage = base::ReadUnaligned<CommHeaderType>(cur_data_buf);
      if (packet->MagicNumber == GlobalChannel->Magic_Num()) {
        /*..................................................................
        Put the packet in the Global Queue
        ..................................................................*/
        if (!GlobalChannel->Receive_Packet(cur_data_buf, packetlen, &address)) {
          DLOG(WARNING) << "IPX global receive queue overflow; packet lost";
        }
      } else {
        if (packet->MagicNumber == ProductID) {
          /*..................................................................
          Find the Private Queue that this packet is for
          ..................................................................*/
          for (i = 0; i < NumConnections; i++) {
            if (base::At(Connection, i)->Address == address) {
              if (!base::At(Connection, i)
                       ->Receive_Packet(cur_data_buf, packetlen)) {
                DLOG(WARNING) << "IPX receive queue overflow for connection "
                              << i << "; packet lost";
              }
              break;
            }
          }
        }
      }
    }
  }

  /*------------------------------------------------------------------------
  Service all connections.  If a connection reports that it's gone "bad",
  report an error to the caller.  If it's the Global Channel, un-queue the
  send entry that's holding things up.  This will keep the Global Channel
  from being clogged by one un-ACK'd outgoing packet.
  ------------------------------------------------------------------------*/
  if (GlobalChannel && (!GlobalChannel->Service())) {
    GlobalChannel->Queue->UnQueue_Send({}, {}, 0);
    rc = 0;
  }

  for (i = 0; i < NumConnections; i++) {
    if (!base::At(Connection, i)->Service()) {
        rc = 0;
        BadConnection = base::At(Connection, i)->ID;
    }
  }

  if (rc) {
    BadConnection = IPXConnClass::kConnectionNone;
  }

  return rc;

} /* end of Service */

/***************************************************************************
 * IPXManagerClass::Get_Bad_Connection -- returns bad connection ID        *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		ID of bad connection; kConnectionNone if none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   05/04/1995 BRR : Created.                                             *
 *=========================================================================*/
int IPXManagerClass::Get_Bad_Connection() const {
  return BadConnection;

} /* end of Get_Bad_Connection */

/***************************************************************************
 * IPXManagerClass::Global_Num_Send	-- reports # entries in send queue
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		# entries in the Global Send Queue
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   01/25/1995 BR : Created.                                              *
 *=========================================================================*/
int IPXManagerClass::Global_Num_Send() {
  /*
  ------------ Error if IPX not installed or not Listening -----------------
  */
  if (!IPXStatus || !Listening) {
    return 0;
  }

  return GlobalChannel->Queue->Num_Send();

} /* end of Global_Num_Send */

/***************************************************************************
 * IPXManagerClass::Global_Num_Receive -- reports # entries in recv queue
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		# entries in the Global Receive Queue
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   01/25/1995 BR : Created.                                              *
 *=========================================================================*/
int IPXManagerClass::Global_Num_Receive() {
  /*
  ------------ Error if IPX not installed or not Listening -----------------
  */
  if (!IPXStatus || !Listening) {
    return 0;
  }

  return GlobalChannel->Queue->Num_Receive();

} /* end of Global_Num_Receive */

/***************************************************************************
 * IPXManagerClass::Private_Num_Send -- reports # entries in send queue *
 *                                                                         *
 * INPUT:                                                                  *
 *		# entries in the Private Send Queue
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = error
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   01/25/1995 BR : Created.                                              *
 *=========================================================================*/
int IPXManagerClass::Private_Num_Send(int id) {
  int i = 0;

  /*
  ------------ Error if IPX not installed or not Listening -----------------
  */
  if (!IPXStatus || !Listening || NumConnections == 0) {
    return 0;
  }

  /*------------------------------------------------------------------------
  If connection ID specified, return that connection's # of packets
  ------------------------------------------------------------------------*/
  if (id != IPXConnClass::kConnectionNone) {
    i = Connection_Index(id);
    if (i != IPXConnClass::kConnectionNone) {
      return base::At(Connection, i)->Queue->Num_Send();
    }
    return 0;
  }
  /*------------------------------------------------------------------------
      Otherwise, return the max # of all connections
      ------------------------------------------------------------------------*/
  int maxnum = 0;
  for (i = 0; i < NumConnections; i++) {
    maxnum = std::max(base::At(Connection, i)->Queue->Num_Send(), maxnum);
  }
  return maxnum;
}

/***************************************************************************
 * IPXManagerClass::Private_Num_Receive -- reports # entries in recv queue
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		id		ID of connection to query
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		# entries in the Private Receive Queue
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   01/25/1995 BR : Created.                                              *
 *=========================================================================*/
int IPXManagerClass::Private_Num_Receive(int id) {
  int i = 0;

  /*
  ------------ Error if IPX not installed or not Listening -----------------
  */
  if (!IPXStatus || !Listening || NumConnections == 0) {
    return 0;
  }

  /*------------------------------------------------------------------------
  If connection ID specified, return that connection's # of packets
  ------------------------------------------------------------------------*/
  if (id != IPXConnClass::kConnectionNone) {
    i = Connection_Index(id);
    if (i != IPXConnClass::kConnectionNone) {
      return base::At(Connection, i)->Queue->Num_Receive();
    }
    return 0;
  }
  /*------------------------------------------------------------------------
      Otherwise, return the max # of all connections
      ------------------------------------------------------------------------*/
  int maxnum = 0;
  for (i = 0; i < NumConnections; i++) {
    maxnum = std::max(base::At(Connection, i)->Queue->Num_Receive(), maxnum);
  }
  return maxnum;
}

/***************************************************************************
 * IPXManagerClass::Set_Socket -- sets socket ID for all connections
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		socket	new socket ID to use
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		Do not call this function after communications have started; you
 ** must call it before calling Init().
 ** The socket number is byte-swapped, since IPX requires socket ID's * to be
 *stored high/low.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   01/25/1995 BR : Created.                                              *
 *=========================================================================*/
void IPXManagerClass::Set_Socket(uint16_t socket) {
  Socket = static_cast<uint16_t>((static_cast<uint32_t>(socket) & 0x00ff) << 8 |
                                 (static_cast<uint32_t>(socket) & 0xff00) >> 8);

} /* end of Set_Socket */

/***************************************************************************
 * IPXManagerClass::Response_Time -- Returns largest Avg Response Time     *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		largest avg response time
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   05/04/1995 BRR : Created.                                             *
 *=========================================================================*/
int32_t IPXManagerClass::Response_Time() {
  int32_t resp = 0;
  int32_t maxresp = 0;
  int i = 0;
  for (i = 0; i < NumConnections; i++) {
    resp = base::At(Connection, i)->Queue->Avg_Response_Time();
    maxresp = std::max(resp, maxresp);
  }

  return maxresp;
}

/***************************************************************************
 * IPXManagerClass::Global_Response_Time -- Returns Avg Response Time      *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		avg global channel response time
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   05/04/1995 BRR : Created.                                             *
 *=========================================================================*/
int32_t IPXManagerClass::Global_Response_Time() {
  if (GlobalChannel) {
    return GlobalChannel->Queue->Avg_Response_Time();
  }
  return 0;
}

/***************************************************************************
 * IPXManagerClass::Reset_Response_Time -- Reset response time
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
 *   05/04/1995 BRR : Created.                                             *
 *=========================================================================*/
void IPXManagerClass::Reset_Response_Time() {
  for (int i = 0; i < NumConnections; i++) {
    base::At(Connection, i)->Queue->Reset_Response_Time();
  }

  if (GlobalChannel) {
    GlobalChannel->Queue->Reset_Response_Time();
  }

} /* end of Reset_Response_Time */

/***************************************************************************
 * IPXManagerClass::Oldest_Send -- gets ptr to oldest send buf             *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		buf ptr
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   05/04/1995 BRR : Created.                                             *
 *=========================================================================*/
std::span<const std::byte> IPXManagerClass::Oldest_Send() {
  std::array<engine::net::CommBufferClass*, CONNECT_MAX> queues{};
  for (int i = 0; i < NumConnections; i++) {
    queues.at(base::ToSize(i)) = base::At(Connection, i)->Queue;
  }
  const engine::net::SendQueueType* oldest =
      ConnectionClass::OldestUnackedSend(queues);
  return oldest != nullptr
             ? std::span(oldest->Buffer).first(base::ToSize(oldest->BufLen))
             : std::span<const std::byte>{};

} /* end of Oldest_Send */

/***************************************************************************
 * IPXManagerClass::Set_Bridge -- prepares to cross a bridge               *
 *                                                                         *
 * This routine is designed to prevent the connection from having to
 ** call Get_Local_Target, except the minimum number of times, since that
 ** routine is buggy & goes away for long periods sometimes.
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		bridge		network number of the destination bridge
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
 *   07/06/1995 BRR : Created.                                             *
 *=========================================================================*/
void IPXManagerClass::Set_Bridge(const NetNumType& bridge) {
  if (GlobalChannel) {
    GlobalChannel->Set_Bridge(bridge);
  }
}

/***************************************************************************
 * IPXManagerClass::Alloc_RealMode_Mem -- allocates real-mode memory
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = error
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
int IPXManagerClass::Alloc_RealMode_Mem() {
  return 1;
}

/***************************************************************************
 * IPXManagerClass::Free_RealMode_Mem -- frees real-mode memory
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = error
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
int IPXManagerClass::Free_RealMode_Mem() {
  return 1;
} /* end of Free_RealMode_Mem */

}  // namespace td
