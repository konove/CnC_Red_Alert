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

/* $Header:   F:\projects\c&c\vcs\code\connect.cpv   1.9   16 Oct 1995 16:48:56
 * JOE_BOSTIC  $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : CONNECT.CPP                              *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : December 20, 1994                        *
 *                                                                         *
 *                  Last Update : May 31, 1995 [BRR]
 **
 *-------------------------------------------------------------------------*
 * Functions: * ConnectionClass::ConnectionClass -- class constructor *
 *   ConnectionClass::~ConnectionClass -- class destructor                 *
 *   ConnectionClass::Service -- main polling routine; services packets *
 *   ConnectionClass::Time -- gets current time
 ** ConnectionClass::Command_Name -- returns name for a packet command *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "td/connect.h"

#include <chrono>
#include <cstdint>
#include <span>

#include "base/enum_array.h"
#include "base/numeric.h"
#include "port/aligned_buffer.h"
#include "td/combuf.h"

/*
********************************* Globals ***********************************
*/
base::EnumArray<ConnectionClass::ConnectionEnum, const char*,
                static_cast<int>(ConnectionClass::PACKET_COUNT)>
    ConnectionClass::Commands = {"ADATA", "NDATA", "ACK"};

/***************************************************************************
 * ConnectionClass::ConnectionClass -- class constructor                   *
 *                                                                         *
 * If either max_retries or timeout is -1, that parameter is ignored in *
 * timeout computations.  If both are -1, the connection will just keep *
 * retrying forever.
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		numsend			desired # of entries for the send queue
 ** numreceive		desired # of entries for the recieve queue
 ** maxlen			max length of an application packet
 ** magicnum			the packet "magic number" for this connection
 ** retry_delta		the time to wait between sends
 ** max_retries		the max # of retries allowed for a packet
 **
 *							(-1 means retry forever,
 *based on this parameter)		* timeout			the max
 *amount of time before we give up on a packet	*
 *							(-1 means retry forever,
 *based on this parameter)		*
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
ConnectionClass::ConnectionClass(int maxlen, uint16_t magicnum,
                                 int32_t retry_delta, int32_t max_retries,
                                 int32_t timeout)
    : MaxPacketLen(maxlen + static_cast<int>(sizeof(CommHeaderType))),
      PacketBuf(new char[base::ToSize(MaxPacketLen)]),
      MagicNum(magicnum),
      RetryDelta(retry_delta),
      MaxRetries(max_retries),
      Timeout(timeout) {
  /*------------------------------------------------------------------------
  Compute our maximum packet length
  ------------------------------------------------------------------------*/

  /*------------------------------------------------------------------------
  Assign the magic number
  ------------------------------------------------------------------------*/

  /*------------------------------------------------------------------------
  Initialize the retry time.  This is the time that t2 - t1 must be greater
  than before a retry will occur.
  ------------------------------------------------------------------------*/

  /*------------------------------------------------------------------------
  Set the maximum allowable retries.
  ------------------------------------------------------------------------*/

  /*------------------------------------------------------------------------
  Set the timeout for this connection.
  ------------------------------------------------------------------------*/

  /*------------------------------------------------------------------------
  Allocate the packet staging buffer.  This will be used to
  ------------------------------------------------------------------------*/
  // new char[] provides alignment for the packet headers stored at its base.

} /* end of ConnectionClass */

/***************************************************************************
 * ConnectionClass::~ConnectionClass -- class destructor                   *
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
ConnectionClass::~ConnectionClass() {
  /*------------------------------------------------------------------------
  Free memory.
  ------------------------------------------------------------------------*/
  delete[] PacketBuf;

} /* end of ~ConnectionClass */

/***************************************************************************
 * ConnectionClass::Service -- main polling routine; services packets *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = error (connection is broken!)
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
int ConnectionClass::Service() {
  /*------------------------------------------------------------------------
  Service the Send Queue.  This [re]sends packets in the Send Queue which
  haven't been ACK'd yet, and if their retry timeout has expired, and
  updates the FirstTime, LastTime & SendCount values in the Queue entry.
  Entries that have been ACK'd should be removed.
  ------------------------------------------------------------------------*/
  //	if (!Service_Send_Queue())
  //		return(0);

  /*------------------------------------------------------------------------
  Service the Receive Queue.  This sends ACKs for packets that haven't
  been ACK'd yet.  Entries that the app has read, and have been ACK'd,
  should be removed.
  ------------------------------------------------------------------------*/
  //	if (!Service_Receive_Queue())
  //		return(0);

  //	return(1);

  if (Service_Send_Queue() && Service_Receive_Queue()) {
    return 1;
  }
  return 0;

} /* end of Service */

/***************************************************************************
 * ConnectionClass::Time -- gets current time
 **
 *                                                                         *
 * INPUT:                                                                  *
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
int64_t ConnectionClass::Time() {
  const auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now().time_since_epoch())
                        .count();
  return msec / 100 * 6;
}

SendQueueType* ConnectionClass::OldestUnackedSend(
    std::span<CommBufferClass* const> queues) {
  // "Found" is tracked through the result pointer rather than a latest-time
  // sentinel: ticks count from boot and outgrow any fixed 32-bit bound.
  SendQueueType* oldest = nullptr;
  for (CommBufferClass* queue : queues) {
    if (queue == nullptr) {
      continue;
    }
    for (int i = 0; i < queue->Num_Send(); i++) {
      SendQueueType* entry = queue->Get_Send(i);
      if (entry == nullptr) {
        continue;
      }
      const CommHeaderType* packet =
          port::AlignedObject<CommHeaderType>(entry->Buffer);
      if (packet->Code == static_cast<unsigned char>(PACKET_DATA_ACK) &&
          entry->IsACK == 0) {
        if (oldest == nullptr || entry->FirstTime < oldest->FirstTime) {
          oldest = entry;
        }
        break;
      }
    }
  }
  return oldest;
}

/***************************************************************************
 * ConnectionClass::Command_Name -- returns name for given packet command  *
 *                                                                         *
 * INPUT:                                                                  *
 *		command		packet Command value to get name for
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		ptr to command name, NULL if invalid
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   05/31/1995 BRR : Created.                                             *
 *=========================================================================*/
const char* ConnectionClass::Command_Name(int command) {
  if (command >= 0 && command < static_cast<int>(PACKET_COUNT)) {
    return Commands[static_cast<ConnectionEnum>(command)];
  }
  return nullptr;
}
