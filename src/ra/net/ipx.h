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

/* $Header: /CounterStrike/IPX.H 1     3/03/97 10:24a Joe_bostic $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : IPX.H                                    *
 *                                                                         *
 *                   Programmer : Barry Nance
 ** from Client/Server LAN Programming			* Westwood-ized by Bill
 *Randolph				*
 *                                                                         *
 *                   Start Date : December 14, 1994                        *
 *                                                                         *
 *                  Last Update : December 14, 1994   [BR]                 *
 *                                                                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef CNC_RED_ALERT_RA_NET_IPX_H_
#define CNC_RED_ALERT_RA_NET_IPX_H_

#include <cstdint>

namespace ra {

/*
******************************** Structures *********************************
*/
using NetNumType = unsigned char[4];
using NetNodeType = unsigned char[6];

/*---------------------------------------------------------------------------
This is the IPX Packet structure.  It's followed by the data itself, which
can be up to 546 bytes long.  Annotation of 'IPX' means IPX will set this
field; annotation of 'APP' means the application must set the field.
NOTE: All header fields are ordered high-byte,low-byte.
---------------------------------------------------------------------------*/
struct IPXHEADER {
  uint16_t CheckSum;                     // IPX: Not used; always 0xffff
  uint16_t Length;                       // IPX: Total size, incl header & data
  unsigned char TransportControl;        // IPX: # bridges message crossed
  unsigned char PacketType;              // APP: Set to 4 for IPX (5 for SPX)
  unsigned char DestNetworkNumber[4];    // APP: destination Network Number
  unsigned char DestNetworkNode[6];      // APP: destination Node Address
  uint16_t DestNetworkSocket;            // APP: destination Socket Number
  unsigned char SourceNetworkNumber[4];  // IPX: source Network Number
  unsigned char SourceNetworkNode[6];    // IPX: source Node Address
  uint16_t SourceNetworkSocket;          // IPX: source Socket Number
};
using IPXHeaderType = IPXHEADER;

/*---------------------------------------------------------------------------
This is the IPX Event Control Block.  It serves as a communications area
between IPX and the application for a single IPX operation.  You should set
up a separate ECB for each IPX operation you perform.
---------------------------------------------------------------------------*/
struct ECB {
  void* Link_Address;
  void (*Event_Service_Routine)();  // APP: event handler (NULL=none)
  unsigned char InUse;              // IPX: 0 = event complete
  unsigned char CompletionCode;     // IPX: event's return code
  uint16_t SocketNumber;            // APP: socket to send data through
  uint16_t ConnectionID;            // returned by Listen (???)
  uint16_t RestOfWorkspace;
  unsigned char DriverWorkspace[12];
  unsigned char ImmediateAddress[6];  // returned by Get_Local_Target
  uint16_t PacketCount;
  struct {
    void* Address;
    uint16_t Length;
  } Packet[2];
};
using ECBType = ECB;

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_NET_IPX_H_

/***************************** end of ipx.h ********************************/
