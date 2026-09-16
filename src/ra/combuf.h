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

/* $Header: /CounterStrike/COMBUF.H 1     3/03/97 10:24a Joe_bostic $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : COMBUF.H *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : December 19, 1994                        *
 *                                                                         *
 *                  Last Update : April 1, 1995   [BR] *
 *                                                                         *
 *-------------------------------------------------------------------------*
 *                                                                         *
 * This class's job is to store outgoing messages & incoming messages, * and
 *serves as a storage area for various flags for ACK & Retry logic.	*
 *                                                                         *
 * This class stores buffers in a non-sequenced order; it allows freeing
 ** any entry, so the buffers can be kept clear, even if packets come in
 ** out of order.
 **
 *                                                                         *
 * The class also contains routines to maintain a cumulative response time
 ** for this queue.  It's up to the caller to call Add_Delay() whenever * it
 * detects that an outgoing message has been ACK'd; this class adds
 *	* that delay into a computed average delay over the last few message *
 * delays.
 **
 *                                                                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef CNC_RED_ALERT_RA_COMBUF_H_
#define CNC_RED_ALERT_RA_COMBUF_H_

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "absl/base/attributes.h"

/*
********************************** Defines **********************************
*/
/*---------------------------------------------------------------------------
This is one output queue entry
---------------------------------------------------------------------------*/
struct SendQueueType {
  unsigned int IsActive : 1 = 0;  // 1 = this entry is ready to be processed
  unsigned int IsACK : 1 = 0;     // 1 = ACK received for this packet
  int64_t FirstTime = 0;          // time this packet was first sent
  int64_t LastTime = 0;           // time this packet was last sent
  int SendCount = 0;              // # of times this packet has been sent
  int BufLen = 0;                 // size of the packet stored in this entry
  std::vector<std::byte> Buffer;  // the data packet
  int ExtraLen = 0;               // size of extra data
  std::vector<std::byte> ExtraBuffer;  // extra data buffer
};

/*---------------------------------------------------------------------------
This is one input queue entry
---------------------------------------------------------------------------*/
struct ReceiveQueueType {
  unsigned int IsActive : 1 = 0;  // 1 = this entry is ready to be processed
  unsigned int IsRead : 1 = 0;    // 1 = caller has read this entry
  unsigned int IsACK : 1 = 0;     // 1 = ACK sent for this packet
  int BufLen = 0;                 // size of the packet stored in this entry
  std::vector<std::byte> Buffer;  // the data packet
  int ExtraLen = 0;               // size of extra data
  std::vector<std::byte> ExtraBuffer;  // extra data buffer
};

/*
***************************** Class Declaration *****************************
*/
class CommBufferClass {
  /*
  ---------------------------- Public Interface ----------------------------
  */
 public:
  /*
  ....................... Constructor/Destructor ........................
  */
  CommBufferClass(int numsend, int numreceive, int maxlen, int extralen = 0);
  virtual ~CommBufferClass();
  CommBufferClass(const CommBufferClass&) = delete;
  CommBufferClass& operator=(const CommBufferClass&) = delete;
  CommBufferClass(CommBufferClass&&) = delete;
  CommBufferClass& operator=(CommBufferClass&&) = delete;
  void Init();
  void Init_Send_Queue();

  /*
  ......................... Send Queue routines .........................
  */
  int Queue_Send(std::span<const std::byte> buf, int buflen,
                 std::span<const std::byte> extrabuf = {}, int extralen = 0);
  int UnQueue_Send(std::span<std::byte> buf, int* buflen, int index,
                   std::span<std::byte> extrabuf = {}, int* extralen = nullptr);
  // # entries in queue
  [[nodiscard]] int Num_Send() const { return SendCount; }
  // max # send queue entries
  [[nodiscard]] int Max_Send() const { return MaxSend; }
  SendQueueType* Get_Send(int index)
      ABSL_ATTRIBUTE_LIFETIME_BOUND;  // random access to queue
  [[nodiscard]] uint32_t Send_Total() const { return SendTotal; }

  /*
  ....................... Receive Queue routines ........................
  */
  int Queue_Receive(std::span<const std::byte> buf, int buflen,
                    std::span<const std::byte> extrabuf = {}, int extralen = 0);
  int UnQueue_Receive(std::span<std::byte> buf, int* buflen, int index,
                      std::span<std::byte> extrabuf = {},
                      int* extralen = nullptr);
  // # entries in queue
  [[nodiscard]] int Num_Receive() const { return ReceiveCount; }
  // max # recv queue entries
  [[nodiscard]] int Max_Receive() const { return MaxReceive; }
  ReceiveQueueType* Get_Receive(int index)
      ABSL_ATTRIBUTE_LIFETIME_BOUND;  // random access to queue
  [[nodiscard]] uint32_t Receive_Total() const { return ReceiveTotal; }

  /*
  ....................... Response time routines ........................
  */
  void Add_Delay(int64_t delay);                    // accumulates response time
  [[nodiscard]] int32_t Avg_Response_Time() const;  // gets mean response time
  [[nodiscard]] int32_t Max_Response_Time() const;  // gets max response time
  void Reset_Response_Time();                       // resets computations

  /*
  ........................ Debug output routines ........................
  */
  void Configure_Debug(int type_offset, int type_size, const char** names,
                       int namestart, int namecount);
  static void Mono_Debug_Print(int refresh = 0);
  static void Mono_Debug_Print2(int refresh = 0);

  /*
  --------------------------- Private Interface ----------------------------
  */
 private:
  /*
  .......................... Limiting variables .........................
  */
  int MaxSend;        // max # send queue entries
  int MaxReceive;     // max # receive queue entries
  int MaxPacketSize;  // max size of a packet, in bytes
  int MaxExtraSize;   // max size of extra bytes

  /*
  ....................... Response time variables .......................
  */
  int64_t DelaySum = 0;   // sum of last 4 delay times
  int NumDelay = 0;       // current # delay times summed
  int64_t MeanDelay = 0;  // current average delay time
  int64_t MaxDelay = 0;   // max delay ever for this queue

  /*
  ........................ Send Queue variables .........................
  */
  std::vector<SendQueueType> SendQueue;  // incoming packets
  int SendCount = 0;         // # packets in the queue
  uint32_t SendTotal = 0;    // total # added to send queue
  std::vector<int> SendIndex;  // array of Send entry indices

  /*
  ....................... Receive Queue variables .......................
  */
  std::vector<ReceiveQueueType> ReceiveQueue;  // outgoing packets
  int ReceiveCount = 0;            // # packets in the queue
  uint32_t ReceiveTotal = 0;       // total # added to receive queue
  std::vector<int> ReceiveIndex;   // array of Receive entry indices

  /*
  ......................... Debugging Variables .........................
  */
  int DebugOffset = 0;  // offset into app's packet for ID
  int DebugSize = 0;   // size of app's ID
  const char** DebugNames = nullptr;  // ptr to array of app-specific names
  int DebugNameStart = 0;  // number of 1st ID
  int DebugNameCount = 0;  // # of names in array
};

#endif  // CNC_RED_ALERT_RA_COMBUF_H_

/**************************** end of combuf.h ******************************/
