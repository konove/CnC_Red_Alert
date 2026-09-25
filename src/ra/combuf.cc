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

/* $Header: /CounterStrike/COMBUF.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : COMBUF.CPP *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : December 19, 1994                        *
 *                                                                         *
 *                  Last Update : October 23, 1995 [BRR]                   *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   CommBufferClass::CommBufferClass -- class constructor *
 *   CommBufferClass::~CommBufferClass -- class destructor *
 *   CommBufferClass::Init -- initializes this queue                       *
 *   CommBufferClass::Init_Send_Queue -- Clears the send queue             *
 *   CommBufferClass::Queue_Send -- queues a message for sending           *
 *   CommBufferClass::UnQueue_Send -- removes next entry from send queue
 ** CommBufferClass::Get_Send -- gets ptr to queue entry                  *
 *   CommBufferClass::Queue_Receive -- queues a received message
 ** CommBufferClass::UnQueue_Receive -- removes next entry from send queue*
 *   CommBufferClass::Get_Receive -- gets ptr to queue entry               *
 *   CommBufferClass::Add_Delay -- adds a new delay value for response time*
 *   CommBufferClass::Avg_Response_Time -- returns average response time *
 *   CommBufferClass::Max_Response_Time -- returns max response time *
 *   CommBufferClass::Reset_Response_Time -- resets computations
 ** - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "ra/combuf.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

#include "base/buffer.h"
#include "base/numeric.h"

/***************************************************************************
 * CommBufferClass::CommBufferClass -- class constructor *
 *                                                                         *
 * INPUT:                                                                  *
 *		numsend		# queue entries for sending
 ** numreceive	# queue entries for receiving
 ** maxlen		maximum desired packet length, in bytes
 ** extralen		max size of app-specific extra bytes (optional)
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
 *   12/19/1994 BR : Created.                                              *
 *=========================================================================*/
CommBufferClass::CommBufferClass(int numsend, int numreceive, int maxlen,
                                 int extralen)
    : MaxSend(numsend),
      MaxReceive(numreceive),
      MaxPacketSize(maxlen),
      MaxExtraSize(extralen),
      SendQueue(base::ToSize(numsend)),
      SendIndex(base::ToSize(numsend)),
      ReceiveQueue(base::ToSize(numreceive)),
      ReceiveIndex(base::ToSize(numreceive)) {
  //------------------------------------------------------------------------
  //	Init variables
  //------------------------------------------------------------------------

  //------------------------------------------------------------------------
  //	Allocate the queue entries
  //------------------------------------------------------------------------

  //------------------------------------------------------------------------
  //	Allocate queue entry buffers
  //------------------------------------------------------------------------
  for (int i = 0; i < MaxSend; i++) {
    // Byte vectors own the complete packet allocation and carry its capacity.
    SendQueue.at(base::ToSize(i)).Buffer.resize(base::ToSize(maxlen));
    if (MaxExtraSize > 0) {
      SendQueue.at(base::ToSize(i))
          .ExtraBuffer.resize(base::ToSize(MaxExtraSize));
    }
  }

  for (int i = 0; i < MaxReceive; i++) {
    ReceiveQueue.at(base::ToSize(i)).Buffer.resize(base::ToSize(maxlen));
    if (MaxExtraSize > 0) {
      ReceiveQueue.at(base::ToSize(i))
          .ExtraBuffer.resize(base::ToSize(MaxExtraSize));
    }
  }

  Init();

} /* end of CommBufferClass */

/***************************************************************************
 * CommBufferClass::~CommBufferClass -- class destructor *
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
 *   12/19/1994 BR : Created.                                              *
 *=========================================================================*/
CommBufferClass::~CommBufferClass() = default;

/***************************************************************************
 * CommBufferClass::Init -- initializes this queue                         *
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
 *   01/20/1995 BR : Created.                                              *
 *=========================================================================*/
void CommBufferClass::Init() {

  //------------------------------------------------------------------------
  //	Init data members
  //------------------------------------------------------------------------
  SendTotal = 0L;
  ReceiveTotal = 0L;

  DelaySum = 0L;
  NumDelay = 0;
  MeanDelay = 0L;
  MaxDelay = 0L;

  SendCount = 0;

  ReceiveCount = 0;

  //------------------------------------------------------------------------
  //	Init the queue entries
  //------------------------------------------------------------------------
  for (int i = 0; i < MaxSend; i++) {
    SendQueue.at(base::ToSize(i)).IsActive = 0;
    SendQueue.at(base::ToSize(i)).IsACK = 0;
    SendQueue.at(base::ToSize(i)).FirstTime = 0L;
    SendQueue.at(base::ToSize(i)).LastTime = 0L;
    SendQueue.at(base::ToSize(i)).SendCount = 0;
    SendQueue.at(base::ToSize(i)).BufLen = 0;
    SendQueue.at(base::ToSize(i)).ExtraLen = 0;

    SendIndex.at(base::ToSize(i)) = 0;
  }

  for (int i = 0; i < MaxReceive; i++) {
    ReceiveQueue.at(base::ToSize(i)).IsActive = 0;
    ReceiveQueue.at(base::ToSize(i)).IsRead = 0;
    ReceiveQueue.at(base::ToSize(i)).IsACK = 0;
    ReceiveQueue.at(base::ToSize(i)).BufLen = 0;
    ReceiveQueue.at(base::ToSize(i)).ExtraLen = 0;

    ReceiveIndex.at(base::ToSize(i)) = 0;
  }

} /* end of Init */

/***************************************************************************
 * CommBufferClass::Init_Send_Queue -- Clears the send queue               *
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
 *   10/23/1995 BRR : Created.                                             *
 *=========================================================================*/
void CommBufferClass::Init_Send_Queue() {

  //------------------------------------------------------------------------
  //	Init data members
  //------------------------------------------------------------------------
  SendCount = 0;

  //------------------------------------------------------------------------
  //	Init the queue entries
  //------------------------------------------------------------------------
  for (int i = 0; i < MaxSend; i++) {
    SendQueue.at(base::ToSize(i)).IsActive = 0;
    SendQueue.at(base::ToSize(i)).IsACK = 0;
    SendQueue.at(base::ToSize(i)).FirstTime = 0L;
    SendQueue.at(base::ToSize(i)).LastTime = 0L;
    SendQueue.at(base::ToSize(i)).SendCount = 0;
    SendQueue.at(base::ToSize(i)).BufLen = 0;
    SendQueue.at(base::ToSize(i)).ExtraLen = 0;

    SendIndex.at(base::ToSize(i)) = 0;
  }

} /* end of Init_Send_Queue */

/***************************************************************************
 * CommBufferClass::Queue_Send -- queues a message for sending             *
 *                                                                         *
 * INPUT:                                                                  *
 *		buf			buffer containing the message
 ** buflen		length of 'buf'
 ** extrabuf		buffer containing extra data (optional)
 ** extralen		length of extra data (optional)
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = no room in the queue
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
int CommBufferClass::Queue_Send(std::span<const std::byte> buf, int buflen,
                                std::span<const std::byte> extrabuf,
                                int extralen) {
  //------------------------------------------------------------------------
  //	Error if no room in the queue
  //------------------------------------------------------------------------
  if (SendCount == MaxSend || buflen < 0 || buflen > MaxPacketSize ||
      base::ToSize(buflen) > buf.size()) {
    return 0;
  }

  //------------------------------------------------------------------------
  //	Find an empty slot
  //------------------------------------------------------------------------
  int index = -1;
  for (int i = 0; i < MaxSend; i++) {
    if (SendQueue.at(base::ToSize(i)).IsActive == 0) {
      index = i;
      break;
    }
  }
  if (index == -1) {
    return 0;
  }

  //------------------------------------------------------------------------
  //	Set entry flags
  //------------------------------------------------------------------------
  SendQueue.at(base::ToSize(index)).IsActive = 1;  // entry is now active
  SendQueue.at(base::ToSize(index)).IsACK = 0;     // entry hasn't been ACK'd
  SendQueue.at(base::ToSize(index)).FirstTime =
      0L;  // filled in by Manager when sent
  SendQueue.at(base::ToSize(index)).LastTime =
      0L;  // filled in by Manager when sent
  SendQueue.at(base::ToSize(index)).SendCount =
      0;  // filled in by Manager when sent
  SendQueue.at(base::ToSize(index)).BufLen = buflen;  // save buffer size

  //------------------------------------------------------------------------
  //	Copy the packet data
  //------------------------------------------------------------------------
  base::CopyBytes(SendQueue.at(base::ToSize(index)).Buffer, buf,
                  base::ToSize(buflen));

  //------------------------------------------------------------------------
  //	Fill in the extra data, if there is any
  //------------------------------------------------------------------------
  if (!extrabuf.empty() && extralen > 0 && extralen <= MaxExtraSize &&
      base::ToSize(extralen) <= extrabuf.size()) {
    base::CopyBytes(SendQueue.at(base::ToSize(index)).ExtraBuffer, extrabuf,
                    base::ToSize(extralen));
    SendQueue.at(base::ToSize(index)).ExtraLen = extralen;
  } else {
    SendQueue.at(base::ToSize(index)).ExtraLen = 0;
  }

  //------------------------------------------------------------------------
  //	Save this entry's index
  //------------------------------------------------------------------------
  SendIndex.at(base::ToSize(SendCount)) = index;

  //------------------------------------------------------------------------
  //	Increment counters & entry ptr
  //------------------------------------------------------------------------
  SendCount++;
  SendTotal++;

  return 1;

} /* end of Queue_Send */

/***************************************************************************
 * CommBufferClass::UnQueue_Send -- removes next entry from send queue *
 *                                                                         *
 * Frees the given entry; the index given by the caller is the "active" * index
 * value (ie the "nth" active entry), not the actual index in the
 *	* array.
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		buf			buffer to store entry's data in; if
 *nullptr, it's discarded	* buflen		filled in with length of
 * entry retrieved						* index "index"
 * of entry to un-queue
 ** extrabuf		buffer for extra data (optional)
 ** extralen		ptr to length of extra data (optional)
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = no entry to retrieve
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
int CommBufferClass::UnQueue_Send(std::span<std::byte> buf, int* buflen,
                                  int index, std::span<std::byte> extrabuf,
                                  int* extralen) {
  //------------------------------------------------------------------------
  //	Error if no entry to retrieve
  //------------------------------------------------------------------------
  if (index < 0 || index >= SendCount ||
      SendQueue.at(base::ToSize(SendIndex.at(base::ToSize(index)))).IsActive ==
          0) {
    return 0;
  }
  if ((!buf.empty() &&
       (buflen == nullptr ||
        base::ToSize(
            SendQueue.at(base::ToSize(SendIndex.at(base::ToSize(index))))
                .BufLen) > buf.size())) ||
      (!extrabuf.empty() && extralen != nullptr &&
       base::ToSize(
           SendQueue.at(base::ToSize(SendIndex.at(base::ToSize(index))))
               .ExtraLen) > extrabuf.size())) {
    return 0;
  }

  //------------------------------------------------------------------------
  //	Copy the data from the entry
  //------------------------------------------------------------------------
  if (!buf.empty()) {
    base::CopyBytes(
        buf,
        SendQueue.at(base::ToSize(SendIndex.at(base::ToSize(index)))).Buffer,
        base::ToSize(
            SendQueue.at(base::ToSize(SendIndex.at(base::ToSize(index))))
                .BufLen));
    *buflen =
        SendQueue.at(base::ToSize(SendIndex.at(base::ToSize(index)))).BufLen;
  }

  //------------------------------------------------------------------------
  //	Copy the extra data
  //------------------------------------------------------------------------
  if (!extrabuf.empty() && extralen != nullptr) {
    base::CopyBytes(
        extrabuf,
        SendQueue.at(base::ToSize(SendIndex.at(base::ToSize(index))))
            .ExtraBuffer,
        base::ToSize(
            SendQueue.at(base::ToSize(SendIndex.at(base::ToSize(index))))
                .ExtraLen));
    *extralen =
        SendQueue.at(base::ToSize(SendIndex.at(base::ToSize(index)))).ExtraLen;
  }

  //------------------------------------------------------------------------
  //	Set entry flags
  //------------------------------------------------------------------------
  SendQueue.at(base::ToSize(SendIndex.at(base::ToSize(index)))).IsActive = 0;
  SendQueue.at(base::ToSize(SendIndex.at(base::ToSize(index)))).IsACK = 0;
  SendQueue.at(base::ToSize(SendIndex.at(base::ToSize(index)))).FirstTime = 0L;
  SendQueue.at(base::ToSize(SendIndex.at(base::ToSize(index)))).LastTime = 0L;
  SendQueue.at(base::ToSize(SendIndex.at(base::ToSize(index)))).SendCount = 0;
  SendQueue.at(base::ToSize(SendIndex.at(base::ToSize(index)))).BufLen = 0;
  SendQueue.at(base::ToSize(SendIndex.at(base::ToSize(index)))).ExtraLen = 0;

  //------------------------------------------------------------------------
  //	Move Indices back one
  //------------------------------------------------------------------------
  for (int i = index; i < SendCount - 1; i++) {
    SendIndex.at(base::ToSize(i)) = SendIndex.at(base::ToSize(i + 1));
  }
  SendIndex.at(base::ToSize(SendCount - 1)) = 0;
  SendCount--;

  return 1;

} /* end of UnQueue_Send */

/***************************************************************************
 * CommBufferClass::Get_Send -- gets ptr to queue entry                    *
 *                                                                         *
 * This routine gets a pointer to the indicated queue entry.  The index * value
 *is relative to the next-accessible queue entry; 0 = get the * next available
 *queue entry, 1 = get the one behind that, etc. *
 *                                                                         *
 * INPUT:                                                                  *
 *		index		index of entry to get (0 = 1st available)
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		ptr to entry
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/21/1994 BR : Created.                                              *
 *=========================================================================*/
SendQueueType* CommBufferClass::Get_Send(int index) {
  if (index < 0 || index >= SendCount ||
      SendQueue.at(base::ToSize(SendIndex.at(base::ToSize(index)))).IsActive ==
          0) {
    return nullptr;
  }
  return &SendQueue.at(base::ToSize(SendIndex.at(base::ToSize(index))));

} /* end of Get_Send */

/***************************************************************************
 * CommBufferClass::Queue_Receive -- queues a received message
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		buf			buffer containing the message
 ** buflen		length of 'buf'
 ** extrabuf		buffer containing extra data (optional)
 ** extralen		length of extra data (optional)
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = no room in the queue
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
int CommBufferClass::Queue_Receive(std::span<const std::byte> buf, int buflen,
                                   std::span<const std::byte> extrabuf,
                                   int extralen) {
  //------------------------------------------------------------------------
  //	Error if no room in the queue
  //------------------------------------------------------------------------
  if (ReceiveCount == MaxReceive || buflen < 0 || buflen > MaxPacketSize ||
      base::ToSize(buflen) > buf.size()) {
    return 0;
  }

  //------------------------------------------------------------------------
  //	Find an empty slot
  //------------------------------------------------------------------------
  int index = -1;
  for (int i = 0; i < MaxReceive; i++) {
    if (ReceiveQueue.at(base::ToSize(i)).IsActive == 0) {
      index = i;
      break;
    }
  }
  if (index == -1) {
    return 0;
  }

  //------------------------------------------------------------------------
  //	Set entry flags
  //------------------------------------------------------------------------
  ReceiveQueue.at(base::ToSize(index)).IsActive = 1;
  ReceiveQueue.at(base::ToSize(index)).IsRead = 0;
  ReceiveQueue.at(base::ToSize(index)).IsACK = 0;
  ReceiveQueue.at(base::ToSize(index)).BufLen = buflen;

  //------------------------------------------------------------------------
  //	Copy the packet data
  //------------------------------------------------------------------------
  base::CopyBytes(ReceiveQueue.at(base::ToSize(index)).Buffer, buf,
                  base::ToSize(buflen));

  //------------------------------------------------------------------------
  //	Fill in the extra data, if there is any
  //------------------------------------------------------------------------
  if (!extrabuf.empty() && extralen > 0 && extralen <= MaxExtraSize &&
      base::ToSize(extralen) <= extrabuf.size()) {
    base::CopyBytes(ReceiveQueue.at(base::ToSize(index)).ExtraBuffer, extrabuf,
                    base::ToSize(extralen));
    ReceiveQueue.at(base::ToSize(index)).ExtraLen = extralen;
  } else {
    ReceiveQueue.at(base::ToSize(index)).ExtraLen = 0;
  }

  //------------------------------------------------------------------------
  //	Save this entry's index
  //------------------------------------------------------------------------
  ReceiveIndex.at(base::ToSize(ReceiveCount)) = index;

  //------------------------------------------------------------------------
  //	Increment counters & entry ptr
  //------------------------------------------------------------------------
  ReceiveCount++;
  ReceiveTotal++;

  return 1;

} /* end of Queue_Receive */

/***************************************************************************
 * CommBufferClass::UnQueue_Receive -- removes next entry from send queue
 **
 *                                                                         *
 * Frees the given entry; the index given by the caller is the "active" * index
 * value (ie the "nth" active entry), not the actual index in the
 *	* array.
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		buf			buffer to store entry's data in; if
 *nullptr, it's discarded	* buflen		filled in with length of
 * entry retrieved						* index index of
 * entry to un-queue
 ** extrabuf		buffer for extra data (optional)
 ** extralen		ptr to length of extra data (optional)
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		1 = OK, 0 = no entry to retrieve
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/20/1994 BR : Created.                                              *
 *=========================================================================*/
int CommBufferClass::UnQueue_Receive(std::span<std::byte> buf, int* buflen,
                                     int index, std::span<std::byte> extrabuf,
                                     int* extralen) {
  //------------------------------------------------------------------------
  //	Error if no entry to retrieve
  //------------------------------------------------------------------------
  if (index < 0 || index >= ReceiveCount ||
      ReceiveQueue.at(base::ToSize(ReceiveIndex.at(base::ToSize(index))))
              .IsActive == 0) {
    return 0;
  }
  if ((!buf.empty() &&
       (buflen == nullptr ||
        base::ToSize(
            ReceiveQueue.at(base::ToSize(ReceiveIndex.at(base::ToSize(index))))
                .BufLen) > buf.size())) ||
      (!extrabuf.empty() && extralen != nullptr &&
       base::ToSize(
           ReceiveQueue.at(base::ToSize(ReceiveIndex.at(base::ToSize(index))))
               .ExtraLen) > extrabuf.size())) {
    return 0;
  }

  //------------------------------------------------------------------------
  //	Copy the data from the entry
  //------------------------------------------------------------------------
  if (!buf.empty()) {
    base::CopyBytes(
        buf,
        ReceiveQueue.at(base::ToSize(ReceiveIndex.at(base::ToSize(index))))
            .Buffer,
        base::ToSize(
            ReceiveQueue.at(base::ToSize(ReceiveIndex.at(base::ToSize(index))))
                .BufLen));
    *buflen =
        ReceiveQueue.at(base::ToSize(ReceiveIndex.at(base::ToSize(index))))
            .BufLen;
  }

  //------------------------------------------------------------------------
  //	Copy the extra data
  //------------------------------------------------------------------------
  if (!extrabuf.empty() && extralen != nullptr) {
    base::CopyBytes(
        extrabuf,
        ReceiveQueue.at(base::ToSize(ReceiveIndex.at(base::ToSize(index))))
            .ExtraBuffer,
        base::ToSize(
            ReceiveQueue.at(base::ToSize(ReceiveIndex.at(base::ToSize(index))))
                .ExtraLen));
    *extralen =
        ReceiveQueue.at(base::ToSize(ReceiveIndex.at(base::ToSize(index))))
            .ExtraLen;
  }

  //------------------------------------------------------------------------
  //	Set entry flags
  //------------------------------------------------------------------------
  ReceiveQueue.at(base::ToSize(ReceiveIndex.at(base::ToSize(index)))).IsActive =
      0;
  ReceiveQueue.at(base::ToSize(ReceiveIndex.at(base::ToSize(index)))).IsRead =
      0;
  ReceiveQueue.at(base::ToSize(ReceiveIndex.at(base::ToSize(index)))).IsACK = 0;
  ReceiveQueue.at(base::ToSize(ReceiveIndex.at(base::ToSize(index)))).BufLen =
      0;
  ReceiveQueue.at(base::ToSize(ReceiveIndex.at(base::ToSize(index)))).ExtraLen =
      0;

  //------------------------------------------------------------------------
  //	Move Indices back one
  //------------------------------------------------------------------------
  for (int i = index; i < ReceiveCount - 1; i++) {
    ReceiveIndex.at(base::ToSize(i)) = ReceiveIndex.at(base::ToSize(i + 1));
  }
  ReceiveIndex.at(base::ToSize(ReceiveCount - 1)) = 0;
  ReceiveCount--;

  return 1;

} /* end of UnQueue_Receive */

/***************************************************************************
 * CommBufferClass::Get_Receive -- gets ptr to queue entry                 *
 *                                                                         *
 * This routine gets a pointer to the indicated queue entry.  The index * value
 *is relative to the next-accessible queue entry; 0 = get the * next available
 *queue entry, 1 = get the one behind that, etc. *
 *                                                                         *
 * INPUT:                                                                  *
 *		index		index of entry to get (0 = 1st available)
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		ptr to entry
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   12/21/1994 BR : Created.                                              *
 *=========================================================================*/
ReceiveQueueType* CommBufferClass::Get_Receive(int index) {
  if (index < 0 || index >= ReceiveCount ||
      ReceiveQueue.at(base::ToSize(ReceiveIndex.at(base::ToSize(index))))
              .IsActive == 0) {
    return nullptr;
  }
  return &ReceiveQueue.at(base::ToSize(ReceiveIndex.at(base::ToSize(index))));

} /* end of Get_Receive */

/***************************************************************************
 * CommBufferClass::Add_Delay -- adds a new delay value for response time  *
 *                                                                         *
 * This routine updates the average response time for this queue.  The *
 * computation is based on the average of the last 'n' delay values given,
 ** It computes a running total of the last n delay values, then divides * that
 *by n to compute the average.
 **
 *																									*
 * When the number of values given exceeds the max, the mean is subtracted
 ** off the total, then the new value is added in.  Thus, any single delay
 ** value will have an effect on the total that approaches 0 over time, and
 ** the new delay value contributes to 1/n of the mean.
 **
 *                                                                         *
 * INPUT:                                                                  *
 *		delay			value to add into the response time
 *computation				*
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
 *   01/19/1995 BR : Created.                                              *
 *=========================================================================*/
void CommBufferClass::Add_Delay(int64_t delay) {
  int roundoff = 0;

  if (NumDelay == 256) {
    DelaySum -= MeanDelay;
    DelaySum += delay;
    if ((DelaySum % 256) > 127) {
      roundoff = 1;
    }
    MeanDelay = (DelaySum / 256) + roundoff;
  } else {
    NumDelay++;
    DelaySum += delay;
    MeanDelay = DelaySum / NumDelay;
  }

  MaxDelay = std::max(delay, MaxDelay);
} /* end of Add_Delay */

/***************************************************************************
 * CommBufferClass::Avg_Response_Time -- returns average response time *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		latest computed average response time
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   01/19/1995 BR : Created.                                              *
 *=========================================================================*/
int32_t CommBufferClass::Avg_Response_Time() const {
  return static_cast<int32_t>(MeanDelay);

} /* end of Avg_Response_Time */

/***************************************************************************
 * CommBufferClass::Max_Response_Time -- returns max response time *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		latest computed average response time
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   01/19/1995 BR : Created.                                              *
 *=========================================================================*/
int32_t CommBufferClass::Max_Response_Time() const {
  return static_cast<int32_t>(MaxDelay);

} /* end of Max_Response_Time */

/***************************************************************************
 * CommBufferClass::Reset_Response_Time -- resets computations
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
 *   01/19/1995 BR : Created.                                              *
 *=========================================================================*/
void CommBufferClass::Reset_Response_Time() {
  DelaySum = 0L;
  NumDelay = 0;
  MeanDelay = 0L;
  MaxDelay = 0L;

} /* end of Reset_Response_Time */

/************************** end of combuf.cpp ******************************/
