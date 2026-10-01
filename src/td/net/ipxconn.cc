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

/* $Header:   F:\projects\c&c\vcs\code\ipxconn.cpv   1.9   16 Oct 1995 16:50:52
 * JOE_BOSTIC  $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : IPXCONN.CPP                              *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : December 20, 1994                        *
 *                                                                         *
 *                  Last Update : April 9, 1995 [BRR]
 **
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions: * IPXConnClass::IPXConnClass -- class constructor
 ** IPXConnClass::~IPXConnClass -- class destructor * IPXConnClass::Init --
 *hardware-specific initialization routine			*
 *   IPXConnClass::Configure -- One-time initialization routine
 ** IPXConnClass::Start_Listening -- commands IPX to listen               *
 *   IPXConnClass::Stop_Listening -- commands IPX to stop  listen          *
 *   IPXConnClass::Send -- sends a packet; invoked by SequencedConnection
 ** IPXConnClass::Open_Socket -- opens communications socket
 ** IPXConnClass::Close_Socket -- closes the socket
 ** IPXConnClass::Send_To -- sends the packet to the given address
 ** IPXConnClass::Broadcast -- broadcasts the given packet
 **
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "td/net/ipxconn.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include "base/buffer.h"
#include "base/strings/safe_string.h"
#include "td/net/ipx.h"
#include "td/net/ipxaddr.h"
#include "td/net/network.h"
#include "td/net/noseqcon.h"
#include "td/net/tcpip.h"

/*
********************************* Globals ***********************************
*/
uint16_t IPXConnClass::Socket;
int IPXConnClass::ConnectionNum;
ECBType* IPXConnClass::ListenECB;
IPXHeaderType* IPXConnClass::ListenHeader;
char* IPXConnClass::ListenBuf;
ECBType* IPXConnClass::SendECB;
IPXHeaderType* IPXConnClass::SendHeader;
char* IPXConnClass::SendBuf;
int32_t IPXConnClass::Handler;
int IPXConnClass::Configured = 0;
int IPXConnClass::SocketOpen = 0;
int IPXConnClass::Listening = 0;
int IPXConnClass::PacketLen;

/***************************************************************************
 * IPXConnClass::IPXConnClass -- class constructor *
 *                                                                         *
 * INPUT:                                                                  *
 *		numsend			desired # of entries for the send queue
 ** numreceive		desired # of entries for the recieve queue
 ** maxlen			max length of an application packet
 ** magicnum			the packet "magic number" for this connection
 ** address			address of destination (NULL = no address)
 ** id					connection's unique numerical ID
 ** name				connection's name
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
IPXConnClass::IPXConnClass(int numsend, int numreceive, int maxlen,
                           uint16_t magicnum, IPXAddressClass* address, int id,
                           const char* name)
    : NonSequencedConnClass(numsend, numreceive, maxlen, magicnum,
                            2,   // retry delta
                            -1,  // max retries
                            60)  // timeout
{
  NetNumType net;
  NetNodeType node;

  /*------------------------------------------------------------------------
  Save the values passed in
  ------------------------------------------------------------------------*/
  if (address) {
    Address = *address;
  }
  ID = id;
  base::SafeCopy(Name, name);

  if (!TheNetwork().winsock().Get_Connected()) {
    /*------------------------------------------------------------------------
    If our Address field is an actual address (ie NULL wasn't passed to the
    constructor), pre-compute the ImmediateAddress value for the SendECB.
    This allows pre-computing of the ImmediateAddress for all connections
    created after Configure() is called.
    ------------------------------------------------------------------------*/
    if (!Address.Is_Broadcast() && Configured == 1) {
      Address.Get_Address(net, node);
      /*.....................................................................
      Use the destination node address as the ImmediateAddress, and just hope
      there's no network bridge in the path.
      .....................................................................*/
      base::CopyBytes(base::ObjectBytes(ImmediateAddress),
                      base::ObjectBytes(node), 6);

      Immed_Set = 1;
    } else {
      base::FillBytes(base::ObjectBytes(ImmediateAddress), 0, 6);
      Immed_Set = 0;
    }
  }

} /* end of IPXConnClass */

/***************************************************************************
 * IPXConnClass::Init -- hardware-specific initialization routine
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
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
void IPXConnClass::Init() {
  /*------------------------------------------------------------------------
  Invoke the parent's Init routine
  ------------------------------------------------------------------------*/
  NonSequencedConnClass::Init();
}

/***************************************************************************
 * IPXConnClass::Configure -- One-time initialization routine
 **
 *                                                                         *
 * This routine sets up static members that are shared by all IPX
 ** connections (ie those variables used by the Send/Listen/Broadcast
 ** routines).
 **
 *																									*
 * INPUT:                                                                  *
 *		socket				socket ID for sending &
 *receiving						* conn_num
 *local IPX Connection Number (0 = not logged in)		* listen_ecb
 *ptr to ECBType for listening
 ** send_ecb				ptr to ECBType for sending
 ** listen_header		ptr to IPXHeaderType for listening
 ** send_header			ptr to IPXHeaderType for sending
 ** listen_buf			ptr to buffer for listening
 ** send_buf				ptr to buffer for sending
 ** handler_rm_ptr		REAL-MODE pointer to event service routine
 ** (high word = segment, low word = offset)				*
 *		maxpacketlen		max packet size to listen for
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		- All pointers must be protected-mode pointers, but must point
 *to		* DOS real-mode memory (except the Handler segment/offset)
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
void IPXConnClass::Configure(uint16_t socket, int conn_num, ECBType* listen_ecb,
                             ECBType* send_ecb, IPXHeaderType* listen_header,
                             IPXHeaderType* send_header, char* listen_buf,
                             char* send_buf, int32_t handler_rm_ptr,
                             int maxpacketlen) {
  /*------------------------------------------------------------------------
  Save the values passed in
  ------------------------------------------------------------------------*/
  Socket = socket;
  ConnectionNum = conn_num;
  ListenECB = listen_ecb;
  SendECB = send_ecb;
  ListenHeader = listen_header;
  SendHeader = send_header;
  ListenBuf = listen_buf;
  SendBuf = send_buf;
  Handler = handler_rm_ptr;
  PacketLen = maxpacketlen;

  Configured = 1;

} /* end of Configure */

/***************************************************************************
 * IPXConnClass::Start_Listening -- commands IPX to listen                 *
 *                                                                         *
 * This routine may be used to start listening in polled mode (if the * ECB's
 *Event_Service_Routine is NULL), or in interrupt mode; it's * up to the caller
 *to fill the ECB in.  If in polled mode, Listening		* must be
 *restarted every time a packet comes in.
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
 *		- The ListenECB must have been properly filled in by the IPX
 *Manager.*
 *		- Configure must be called before calling this routine.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/16/1994 BR : Created.                                              *
 *=========================================================================*/
bool IPXConnClass::Start_Listening() {

  if (TheNetwork().winsock().Get_Connected()) {
    return true;
  }

  // There is no IPX driver: only the Winsock path can listen.
  Open_Socket(Socket);
  return false;

} /* end of Start_Listening */

/***************************************************************************
 * IPXConnClass::Stop_Listening -- commands IPX to stop  listen            *
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
 *		- This routine MUST NOT be called if IPX is not listening
 *already!	*
 *                                                                         *
 * HISTORY:                                                                *
 *   12/16/1994 BR : Created.                                              *
 *=========================================================================*/
bool IPXConnClass::Stop_Listening() {
  /*------------------------------------------------------------------------
  Don't do anything unless we're already Listening.
  ------------------------------------------------------------------------*/
  if (Listening == 0) {
    return false;
  }

  if (TheNetwork().winsock().Get_Connected()) {
    Listening = 0;
    return true;
  }
  Close_Socket(Socket);

  Listening = 0;

  /*------------------------------------------------------------------------
  All done.
  ------------------------------------------------------------------------*/
  return true;

} /* end of Stop_Listening */

/***************************************************************************
 * IPXConnClass::Send -- sends a packet; invoked by SequencedConnection *
 *                                                                         *
 * INPUT:                                                                  *
 *		socket		desired socket ID number
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
 *   12/16/1994 BR : Created.                                              *
 *=========================================================================*/
int IPXConnClass::Send(std::span<const std::byte> buf, int buflen) {
  /*------------------------------------------------------------------------
  Invoke our own Send_To routine, filling in our Address as the destination.
  ------------------------------------------------------------------------*/
  if (Immed_Set) {
    return Send_To(buf, buflen, &Address, ImmediateAddress);
  }
  return Send_To(buf, buflen, &Address, {});

} /* end of Send */

/***************************************************************************
 * IPXConnClass::Open_Socket -- opens communications socket
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		socket		desired socket ID number
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
 *   12/16/1994 BR : Created.                                              *
 *=========================================================================*/
int IPXConnClass::Open_Socket(uint16_t /*socket*/) {
  SocketOpen = 1;
  return 1;
}

/***************************************************************************
 * IPXConnClass::Close_Socket -- closes the socket
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		socket		desired socket ID number
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		none.
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		Calling this routine when the sockets aren't open may crash!
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/16/1994 BR : Created.                                              *
 *=========================================================================*/
void IPXConnClass::Close_Socket(uint16_t /*socket*/) {
  SocketOpen = 0;
} /* end of Close_Socket */

/***************************************************************************
 * IPXConnClass::Send_To -- sends the packet to the given address
 **
 *                                                                         *
 * The "ImmediateAddress" field of the SendECB must be filled in with the
 ** address of a bridge, or the node address of the destination if there
 ** is no bridge.  The NETX call to find this address will always crash * if
 * NETX isn't loaded (ConnectionNum is 0), so this case is trapped &
 *	* prevented.
 ** Also, if the address of this IPX connection is known when the
 ** constructor is called, and Configure has been called, Get_Local_Target * is
 *called to precompute the ImmediateAddress; this case is detected & 	* if the
 *value is already computed, it's just memcpy'd over.
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		buf			buffer to send
 ** buflen		length of buffer
 ** address		Address to send to
 ** immed			ImmediateAddress value, NULL if none
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
 *   12/16/1994 BR : Created.                                              *
 *=========================================================================*/
int IPXConnClass::Send_To(std::span<const std::byte> buf, int buflen,
                          IPXAddressClass* /*address*/,
                          std::span<const unsigned char> /*immed*/) {
  if (TheNetwork().winsock().Get_Connected()) {
    TheNetwork().winsock().Write(buf, buflen);

    return 1;
  }

  return 0;  // There is no IPX driver to send through.
}

/***************************************************************************
 * IPXConnClass::Broadcast -- broadcasts the given packet
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		socket		desired socket ID number
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
 *   12/16/1994 BR : Created.                                              *
 *=========================================================================*/
int IPXConnClass::Broadcast(std::span<const std::byte> buf, int buflen) {
  if (TheNetwork().winsock().Get_Connected()) {
    TheNetwork().winsock().Write(buf, buflen);
    return 1;
  }
  return 0;  // There is no IPX driver to send through.
}
