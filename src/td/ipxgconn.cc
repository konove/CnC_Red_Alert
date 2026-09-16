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

/* $Header:   F:\projects\c&c\vcs\code\ipxgconn.cpv   1.9   16 Oct 1995 16:51:00
 * JOE_BOSTIC  $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : IPXGCONN.CPP                             *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : December 20, 1994                        *
 *                                                                         *
 *                  Last Update : July 6, 1995 [BRR]                       *
 *-------------------------------------------------------------------------*
 * Functions: * IPXGlobalConnClass::IPXGlobalConnClass -- class constructor *
 *   IPXGlobalConnClass::~IPXGlobalConnClass -- class destructor           *
 *   IPXGlobalConnClass::Send_Packet -- adds a packet to the send queue *
 *   IPXGlobalConnClass::Receive_Packet -- adds packet to the receive queue*
 *   IPXGlobalConnClass::Get_Packet -- gets a packet from the receive queue*
 *   IPXGlobalConnClass::Send -- sends a packet
 ** IPXGlobalConnClass::Service_Receive_Queue -- services recieve queue	*
 *   IPXGlobalConnClass::Set_Bridge -- Sets up connection to cross a bridge*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/***************************************************************************
 * IPXGlobalConnClass::IPXGlobalConnClass -- class constructor             *
 *                                                                         *
 * This routine chains to the parent constructor, but it adjusts the size
 ** of the packet by the added bytes in the GlobalHeaderType structure. * This
 * forces the parent classes to allocate the proper sized PacketBuf	* for
 * outgoing packets, and to set MaxPacketLen to the proper value.
 *	*
 *                                                                         *
 * INPUT:                                                                  *
 *		numsend			desired # of entries for the send queue
 ** numreceive		desired # of entries for the recieve queue
 ** maxlen			max length of an application packet
 ** product_id		unique ID for this product
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
#include "td/ipxgconn.h"

#include <cstdint>
#include <cstring>
#include <utility>

#include "base/buffer.h"
#include "base/numeric.h"
#include "port/aligned_buffer.h"
#include "port/unaligned.h"
#include "td/combuf.h"
#include "td/connect.h"
#include "td/ipx.h"
#include "td/ipxaddr.h"
#include "td/ipxconn.h"

IPXGlobalConnClass::IPXGlobalConnClass(int numsend, int numreceive, int maxlen,
                                       uint16_t product_id)
    : IPXConnClass(numsend, numreceive,
                   maxlen + static_cast<int>(sizeof(GlobalHeaderType) -
                                             sizeof(CommHeaderType)),
                   kGlobalMagicnum,  // magic number for this connection
                   nullptr,          // IPX Address (none)
                   0,                // Connection ID
                   ""),
      ProductID(product_id)  // Connection Name
{}                           /* end of IPXGlobalConnClass */

/***************************************************************************
 * IPXGlobalConnClass::Send_Packet -- adds a packet to the send queue *
 *                                                                         *
 * This routine prefixes the given buffer with a GlobalHeaderType and * queues
 *the resulting packet into the Send Queue.  The packet's * MagicNumber, Code,
 *PacketID, destination Address and ProductID are set 	* here.
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		buf			buffer to send
 ** buflen		length of buffer
 ** address		address to send the packet to (NULL = Broadcast)
 ** ack_req		true = ACK is required for this packet; false = isn't
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
int IPXGlobalConnClass::Send_Packet(void* buf, int buflen,
                                    IPXAddressClass* address, int ack_req) {
  /*------------------------------------------------------------------------
  Store the packet's Magic Number
  ------------------------------------------------------------------------*/
  port::AlignedObject<GlobalHeaderType>(PacketBuf)->Header.MagicNumber =
      MagicNum;

  /*------------------------------------------------------------------------
  If this is a ACK-required packet, sent to a specific system, mark it as
  ACK-required; otherwise, mark as no-ACK-required.
  ------------------------------------------------------------------------*/
  if (ack_req && address != nullptr) {
    port::AlignedObject<GlobalHeaderType>(PacketBuf)->Header.Code =
        static_cast<unsigned char>(PACKET_DATA_ACK);
  } else {
    port::AlignedObject<GlobalHeaderType>(PacketBuf)->Header.Code =
        static_cast<unsigned char>(PACKET_DATA_NOACK);
  }

  /*------------------------------------------------------------------------
  Fill in the packet ID.  This will have very limited meaning; it only
  allows us to determine if an ACK packet we receive later goes with this
  packet; it doesn't let us detect re-sends of other systems' packets.
  ------------------------------------------------------------------------*/
  port::AlignedObject<GlobalHeaderType>(PacketBuf)->Header.PacketID =
      Queue->Send_Total();

  /*------------------------------------------------------------------------
  Set the product ID for this packet.
  ------------------------------------------------------------------------*/
  port::AlignedObject<GlobalHeaderType>(PacketBuf)->ProductID = ProductID;

  /*------------------------------------------------------------------------
  Set this packet's destination address.  If no address is specified, use
  a Broadcast address (which IPXAddressClass's default constructor creates).
  ------------------------------------------------------------------------*/
  if (address != nullptr) {
    port::AlignedObject<GlobalHeaderType>(PacketBuf)->Address = *address;
  } else {
    port::AlignedObject<GlobalHeaderType>(PacketBuf)->Address =
        IPXAddressClass();
  }

  /*------------------------------------------------------------------------
  Copy the application's data
  ------------------------------------------------------------------------*/
  memcpy(PacketBuf + sizeof(GlobalHeaderType), buf, base::ToSize(buflen));

  /*------------------------------------------------------------------------
  Queue it
  ------------------------------------------------------------------------*/
  return Queue->Queue_Send(
      PacketBuf, buflen + static_cast<int>(sizeof(GlobalHeaderType)));

} /* end of Send_Packet */

/***************************************************************************
 * IPXGlobalConnClass::Receive_Packet -- adds packet to the receive queue
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		buf		buffer to process (already includes
 *GlobalHeaderType)			* buflen	length of buffer to
 *process
 ** address	the address of the sender (the IPX Manager class must
 ** extract this from the IPX Header of the received packet.)	*
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
int IPXGlobalConnClass::Receive_Packet(void* buf, int buflen,
                                       IPXAddressClass* address) {

  /*
  --------------------------- Check the magic # ----------------------------
  */
  if (std::cmp_less(buflen, sizeof(GlobalHeaderType))) {
    return 0;
  }
  auto packet_storage = port::ReadUnaligned<GlobalHeaderType>(buf);
  GlobalHeaderType* packet = &packet_storage;  // ptr to this packet
  if (packet->Header.MagicNumber != MagicNum) {
    return 0;
  }

  /*------------------------------------------------------------------------
  Process the packet based on its Code
  ------------------------------------------------------------------------*/
  switch (packet->Header.Code) {
    /*.....................................................................
    DATA: Save the given address in the message buffer (so Get_Message()
    can extract it later), and queue this message.
    Don't bother checking for a Re-Send; since this queue is receiving data
    from multiple systems, the Total_Receive() value for this queue will
    have nothing to do with the packet's ID.  The application must deal
    with this by being able to handle multiple receipts of the same packet.
    .....................................................................*/
    case static_cast<unsigned char>(PACKET_DATA_ACK):
    case static_cast<unsigned char>(PACKET_DATA_NOACK):
      packet->Address = *address;
      port::WriteUnaligned(buf, packet_storage);
      Queue->Queue_Receive(buf, buflen);
      break;

    /*.....................................................................
    ACK: If this ACK is for any of my packets, mark that packet as
    acknowledged, then throw this packet away.  Otherwise, ignore the ACK
    (if we re-sent before we received the other system's first ACK, this
    ACK will be a leftover)
    .....................................................................*/
    case static_cast<unsigned char>(PACKET_ACK):
      for (int i = 0; i < Queue->Num_Send(); i++) {
        /*
        ..................... Get queue entry ptr .......................
        */
        SendQueueType* send_entry =
            Queue->Get_Send(i);  // ptr to send entry header
        /*
        ............. If ptr is valid, get ptr to its data ..............
        */
        auto* entry_data = port::AlignedObject<GlobalHeaderType>(
            send_entry->Buffer);  // ptr to queue entry data
        /*
        .............. If ACK is for this entry, mark it ................
        */
        if (packet->Header.PacketID == entry_data->Header.PacketID &&
            entry_data->Header.Code ==
                static_cast<unsigned char>(PACKET_DATA_ACK)) {
          send_entry->IsACK = 1;
          break;
        }
      }
      break;

    /*.....................................................................
    Default: ignore the packet
    .....................................................................*/
    default:
      break;

  } /* end of switch */

  return 1;
}

/***************************************************************************
 * IPXGlobalConnClass::Get_Packet -- gets a packet from the receive queue
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		buf			location to store buffer
 ** buflen		filled in with length of 'buf'
 ** address		filled in with sender's address
 ** product_id	filled in with sender's ProductID
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
int IPXGlobalConnClass::Get_Packet(void* buf, int* buflen,
                                   IPXAddressClass* address,
                                   uint16_t* product_id) {

  /*
  ------------------------ Return if nothing to do -------------------------
  */
  if (Queue->Num_Receive() == 0) {
    return 0;
  }

  /*
  ------------------ Get ptr to the next available entry -------------------
  */
  ReceiveQueueType* rec_entry =
      Queue->Get_Receive(0);  // ptr to receive entry header

  /*
  ------------------------ Read it if it's un-read -------------------------
  */
  if (rec_entry != nullptr && rec_entry->IsRead == 0) {
    /*
    ........................... Mark as read ..............................
    */
    rec_entry->IsRead = 1;

    /*
    .......................... Copy data packet ...........................
    */
    auto* packet = port::AlignedObject<GlobalHeaderType>(rec_entry->Buffer);
    const int packetlen =
        rec_entry->BufLen -
        static_cast<int>(sizeof(GlobalHeaderType));  // size of received packet
    if (packetlen > 0) {
      memcpy(buf, rec_entry->Buffer + sizeof(GlobalHeaderType),
             base::ToSize(packetlen));
    }
    *buflen = packetlen;
    *address = packet->Address;
    *product_id = packet->ProductID;

    return 1;
  }

  return 0;
}

/***************************************************************************
 * IPXGlobalConnClass::Send -- sends a packet
 **
 *                                                                         *
 * This routine gets invoked by NonSequencedConn, when it's processing * the
 *Send & Receive Queues.  The buffer provided will already have the	*
 * GlobalHeaderType header embedded in it.
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		buf		buffer to send
 ** buflen	length of buffer
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
int IPXGlobalConnClass::Send(void* buf, int buflen) {
  int rc = 0;

  /*------------------------------------------------------------------------
  Extract the packet's embedded IPX address
  ------------------------------------------------------------------------*/
  auto header = port::ReadUnaligned<GlobalHeaderType>(buf);
  IPXAddressClass* addr = &header.Address;

  /*------------------------------------------------------------------------
  If it's a broadcast address, broadcast it
  ------------------------------------------------------------------------*/
  if (addr->Is_Broadcast()) {
    return Broadcast(buf, buflen);
  }
  /*------------------------------------------------------------------------
      Otherwise, send it
      ------------------------------------------------------------------------*/
  if (IsBridge && !memcmp(addr, BridgeNet, 4)) {
    rc = Send_To(buf, buflen, addr, BridgeNode);
  } else {
    rc = Send_To(buf, buflen, addr, {});
  }
  return rc;

} /* end of Send */

/***************************************************************************
 * IPXGlobalConnClass::Service_Receive_Queue -- services the recieve queue
 **
 *                                                                         *
 * This routine is necessary because the Global Connection has to ACK * a packet
 *differently from other types of connections; its Send routine	* assumes that
 *the destination address is embedded within the outgoing		*
 * packet, so we have to create our ACK Packet using the GlobalHeaderType,
 ** not the CommHeaderType.
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
int IPXGlobalConnClass::Service_Receive_Queue() {
  GlobalHeaderType ackpacket;    // ACK packet to send

  /*------------------------------------------------------------------------
  Get a pointer to the next received entry
  ------------------------------------------------------------------------*/
  ReceiveQueueType* rec_entry =
      Queue->Get_Receive(0);  // ptr to receive entry header
  if (rec_entry == nullptr) {
    return 1;
  }

  /*------------------------------------------------------------------------
  If this packet doesn't require an ACK, mark it as ACK'd.
  ------------------------------------------------------------------------*/
  auto* packet_hdr = port::AlignedObject<GlobalHeaderType>(
      rec_entry->Buffer);  // packet header
  if (packet_hdr->Header.Code ==
      static_cast<unsigned char>(PACKET_DATA_NOACK)) {
    rec_entry->IsACK = 1;
  }

  /*------------------------------------------------------------------------
  If this packet hasn't been ACK'd, send an ACK:
  - Fill in the MagicNum & the Code
  - Set the PacketID to the same ID that the sending system used, so the
    sending system knows which packet the ACK is for
  ------------------------------------------------------------------------*/
  if (rec_entry->IsACK == 0) {
    ackpacket.Header.MagicNumber = MagicNum;
    ackpacket.Header.Code = static_cast<unsigned char>(PACKET_ACK);
    ackpacket.Header.PacketID = packet_hdr->Header.PacketID;
    ackpacket.Address = packet_hdr->Address;
    ackpacket.ProductID = ProductID;

    Send(&ackpacket, sizeof(GlobalHeaderType));

    rec_entry->IsACK = 1;
  }

  /*------------------------------------------------------------------------
  If this packet has been read by the application, and has been ACK'd, and
  there is another packet in the queue behind this one, it means the other
  system got the ACK we sent for this packet; remove this packet from the
  queue.
  ------------------------------------------------------------------------*/
  if (rec_entry != nullptr && rec_entry->IsRead && rec_entry->IsACK &&
      Queue->Num_Receive() > 1) {
    Queue->UnQueue_Receive(nullptr, nullptr, 0);
  }

  return 1;

} /* end of Service_Receive_Queue */

/***************************************************************************
 * Set_Bridge -- Sets up connection to cross a bridge                      *
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
void IPXGlobalConnClass::Set_Bridge(const NetNumType& bridge) {
  if (Configured) {
    base::CopyBytes(base::ObjectBytes(BridgeNet), base::ObjectBytes(bridge), 4);
    base::FillBytes(base::ObjectBytes(BridgeNode), 0xff, 6);

    if (IPX_Get_Local_Target(BridgeNet, BridgeNode, Socket, BridgeNode) == 0) {
      IsBridge = 1;
    } else {
      IsBridge = 0;
    }
  }
}
