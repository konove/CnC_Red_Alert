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

/* $Header:   F:\projects\c&c\vcs\code\ipxmgr.h_v   1.10   16 Oct 1995 16:47:34
 * JOE_BOSTIC  $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : IPXMGR.H                                 *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : December 19, 1994                        *
 *                                                                         *
 *                  Last Update : April 3, 1995   [BR] *
 *                                                                         *
 *-------------------------------------------------------------------------*
 *                                                                         *
 * This is the Connection Manager for IPX network communications.  It * creates,
 *manages, & orchestrates multiple IPX connections, as well as	* the "global"
 *connection ("Global Channel"), which can talk to any 		* system on the
 *net.
 **
 *																									*
 * Use the Global Channel to query systems for their names, ID's, &
 ** IPX addresses.  Then, create a Private Connection with each system * that
 *joins your game, and use the Private Channel to send game packets	* (the
 * private channel will perform somewhat faster, & gives you better	*
 * control than the Global Channel; it can detect retries, and the Global
 ** Channel can't).
 **
 * 																								*
 * HOW THIS CLASS WORKS:
 ** This class has to set up an IPX Event Service Routine in low (DOS) * memory.
 *So, it uses DPMI to allocate & lock a chunk of DOS memory;		* this
 *memory is used for all incoming packet buffers, the outgoing * packet buffer,
 *and the actual code for the event handler.  The real-		* mode handler
 *code & this class share a portion of memory that's mapped	* into a
 * "RealModeDataType" structure.  As packets come in, the handler	* points
 *IPX to the next available packet buffer & restarts listening;		* it
 * sets a flag to tell this class that a packet is present at that
 *	* buffer slot.  This class must read all the packets & determine which *
 * connection they go with (the Global Channel, or one of the Private *
 * Channels).  This parsing is done in the Service routine for this class.
 **
 * 																								*
 * Constructor:	Just inits some variables, checks to see if IPX is there
 ** Destructor:		Complete shutdown; stops IPX listening, frees all memory
 ** Init:				Should only be called once (but can be
 *called more); 		* allocates all memory, creates the Global
 *Channel			* connection, starts IPX listening.  By not
 *placing this 	* step in the constructor, the app can control when
 ** listening actually starts; also, you don't get a bunch	* of allocations
 *just by declaring an IPXManagerClass		* instance.  You have to call
 *Init() for the allocations	* to occur.
 ** Connection utilities: Create & manage Private Connections.  Each
 ** connection has its own IPX address, numerical ID, and		*
 *						character name (presumably the
 *name of the other			* player).
 ** Send/Get_Global_Message: adds a packet to the Global Connection queue,
 ** or reads from the queue.  The caller should check the		*
 *						ProductID value from returned
 *packets to be sure it's		* talking to the right product.
 ** Send/Get_Private_Message: adds a packet to a Private Connection queue,
 ** or reads from the queue
 ** Service:			Checks the Real-Mode-Memory packet array to see
 *if any	* new packets have come in; if they have, it parses them
 ** & distributes them to the right connection queue.  The	* queue's
 *Service routine handles ACK'ing or Resending		* packets.
 **
 *																									*
 *	Here's a memory map of the Real-Mode memory block.  'N' is the number
 ** of packet buffers allocated in low memory:
 **
 *																									*
 *						----------------------------------
 ** |       Shared-memory data       |
 **
 *						|--------------------------------|
 ** |  Real-mode event handler code  |
 **
 *						|--------------------------------|
 ** |  IPX Header & Packet Buffer 0  |
 **
 *						|--------------------------------|
 ** |  IPX Header & Packet Buffer 1  |
 **
 *						|--------------------------------|
 ** |  IPX Header & Packet Buffer 2  |
 **
 *						|--------------------------------|
 ** |             . . .              |
 **
 *						|--------------------------------|
 ** |  IPX Header & Packet Buffer N  |
 **
 *						|--------------------------------|
 ** |    Send Event Control Block    |
 **
 *						|--------------------------------|
 ** |         Send IPX Header        |
 **
 *						|--------------------------------|
 ** |       Send Packet Buffer       |
 **
 *						|--------------------------------|
 ** |        Flags Array [N]         |
 **
 *						----------------------------------
 **
 *																									*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef CNC_RED_ALERT_TD_IPXMGR_H_
#define CNC_RED_ALERT_TD_IPXMGR_H_

#include <cstdint>

/*
********************************* Includes **********************************
*/
#include "td/connmgr.h"
#include "td/ipx.h"
#include "td/ipxaddr.h"
#include "td/ipxconn.h"
#include "td/ipxgconn.h"

/*
********************************** Defines **********************************
*/
/*---------------------------------------------------------------------------
This is Virgin Interactive Entertainment's registered socket ID.
---------------------------------------------------------------------------*/
#define VIRGIN_SOCKET 0x8813

/*---------------------------------------------------------------------------
This is the maximum number of IPX connections supported.  Just change this
value to support more.
---------------------------------------------------------------------------*/
#define CONNECT_MAX 6

/*---------------------------------------------------------------------------
These routines report the location & length of the real-mode routine, as
it's stored in protected-mode memory.
---------------------------------------------------------------------------*/
extern "C" {
void* Get_RM_IPX_Address();
int32_t Get_RM_IPX_Size();
}

/*
***************************** Class Declaration *****************************
*/
class IPXManagerClass : public ConnManClass {
  /*
  ---------------------------- Public Interface ----------------------------
  */
 public:
  /*.....................................................................
  Constructor/destructor.
  .....................................................................*/
  IPXManagerClass(int glb_maxlen, int pvt_maxlen, int glb_num_packets,
                  int pvt_num_packets, uint16_t socket, uint16_t product_id);
  ~IPXManagerClass() override;  // stop listening
  IPXManagerClass(const IPXManagerClass&) = delete;
  IPXManagerClass& operator=(const IPXManagerClass&) = delete;
  IPXManagerClass(IPXManagerClass&&) = delete;
  IPXManagerClass& operator=(IPXManagerClass&&) = delete;

  /*.....................................................................
  Initialization routines.
  .....................................................................*/
  int Init();
  int Is_IPX();
  void Set_Timing(int32_t retrydelta, int32_t maxretries,
                  int32_t timeout) override;
  void Set_Bridge(NetNumType bridge);

  /*.....................................................................
  These routines control creation of the "Connections" (data queues) for
  each remote system.
  .....................................................................*/
  bool Create_Connection(int id, char* name, IPXAddressClass* address);
  bool Delete_Connection(int id);
  int Num_Connections() override;
  int Connection_ID(int index) override;
  char* Connection_Name(int id);
  IPXAddressClass* Connection_Address(int id);
  int Connection_Index(int id) override;

  /*.....................................................................
  This is how the application sends & receives messages.
  .....................................................................*/
  int Send_Global_Message(void* buf, int buflen, int ack_req = 0,
                          IPXAddressClass* address = nullptr);
  int Get_Global_Message(void* buf, int* buflen, IPXAddressClass* address,
                         uint16_t* product_id);

  int Send_Private_Message(void* buf, int buflen, int ack_req = 1,
                           int conn_id = CONNECTION_NONE) override;
  int Get_Private_Message(void* buf, int* buflen, int* conn_id) override;

  /*.....................................................................
  The main polling routine; should be called as often as possible.
  .....................................................................*/
  int Service() override;

  /*.....................................................................
  This routine reports which connection has an error on it.
  .....................................................................*/
  int Get_Bad_Connection();

  /*.....................................................................
  Queue utility routines.  The application can determine how many
  messages are in the send/receive queues.
  .....................................................................*/
  int Global_Num_Send() override;
  int Global_Num_Receive() override;
  int Private_Num_Send(int id = CONNECTION_NONE) override;
  int Private_Num_Receive(int id = CONNECTION_NONE) override;

  /*.....................................................................
  This routine changes the socket ID assigned the IPX Manager when it
  was constructed.  Do not call this function after calling Init()!
  The Socket ID should be known by both ends of the communications before
  any packets are sent.
  .....................................................................*/
  void Set_Socket(uint16_t socket);

  /*.....................................................................
  Routines to return the largest average queue response time, and to
  reset the response time for all queues.
  .....................................................................*/
  int32_t Response_Time() override;
  int32_t Global_Response_Time();
  void Reset_Response_Time() override;

  /*.....................................................................
  This routine returns a pointer to the oldest non-ACK'd buffer I've sent.
  .....................................................................*/
  void* Oldest_Send();

  /*.....................................................................
  Debug routines
  .....................................................................*/
  void Configure_Debug(int index, int offset, int size, const char** names,
                       int maxnames) override;
  void Mono_Debug_Print(int index, int refresh = 0) override;

  /*
  --------------------------- Private Interface ----------------------------
  */
 private:
  /*.....................................................................
  These routines allocate & free the DOS Real-mode memory block.
  .....................................................................*/
  static int Alloc_RealMode_Mem();
  static int Free_RealMode_Mem();

  /*.....................................................................
  Misc variables
  .....................................................................*/
  unsigned int IPXStatus : 1;      // 0 = no IPX, 1 = IPX found
  unsigned int Listening : 1;      // 1 = Listening is on
  unsigned int RealMemAllocd : 1;  // 1 = Real-mode memory has been alloc'd

  /*.....................................................................
  Packet Sizes, used for allocating real-mode memory
  .....................................................................*/
  int Glb_MaxPacketLen;  // Global Channel maximum packet size
  int Glb_NumPackets;    // # Global send/receive packets
  int Pvt_MaxPacketLen;  // Private Channel maximum packet size
  int Pvt_NumPackets;    // # Private send/receive packets

  /*.....................................................................
  The ProductID is used in the Global Channel's packet header, and it's
  used for the Private Channels' Magic Number.
  .....................................................................*/
  uint16_t ProductID;  // product ID

  /*.....................................................................
  The Socket ID, and local Novell Connection Number
  .....................................................................*/
  uint16_t Socket;        // Our socket ID for sending/receiving
  int ConnectionNum;      // local connection #, 0=not logged in

  /*.....................................................................
  Array of connection queues
  .....................................................................*/
  IPXConnClass* Connection[CONNECT_MAX]{};  // array of connection object ptrs
  int NumConnections;                     // # connection objects in use
  IPXGlobalConnClass* GlobalChannel;      // the Global Channel

  /*.....................................................................
  Current queue for polling for received packets
  .....................................................................*/
  int CurConnection;

  /*.....................................................................
  Timing parameters for all connections
  .....................................................................*/
  int32_t RetryDelta;
  int32_t MaxRetries;  // -1 means no limit
  int32_t Timeout;     // -1 means no limit

  /*---------------------------------------------------------------------
  Real-mode memory pointers and such
  ---------------------------------------------------------------------*/
  /*.....................................................................
  This is a structure that mirrors data in real-mode memory:
  .....................................................................*/
  typedef struct {
    int16_t Marker1;                // the byte ID marker
    ECBType ListenECB;              // the Listening ECB
    int16_t NumBufs;                // # of buffers we're giving to the handler
    char* BufferFlags;              // array of buffer-avail flags
    int16_t PacketSize;             // size of packet including IPX header
    IPXHeaderType* FirstPacketBuf;  // ptr to 1st packet buffer
    int16_t CurIndex;               // handler's current packet index
    IPXHeaderType* CurPacketBuf;    // handler's current packet buf
    int16_t FuncOffset;             // contains offset of code
    char Semaphore;                 // prevents re-entrancy
    int16_t ReEntrantCount;         // times we've been called re-entrantly
    int16_t StackPtr;               // real-mode stack pointer
    int16_t StackSeg;               // real-mode stack segment
    int16_t StackPtr_int;           // internal stack pointer
    int16_t StackSeg_int;           // internal stack segment
    int16_t StackCheck;             // stack check value (0x1234)
    int16_t Stack[256];             // actual stack space
    int16_t StackSpace;             // label for top of stack
    int16_t Marker2;                // the byte ID marker
  } RealModeDataType;

  /*.....................................................................
  The number & size of packet buffers in low memory
  .....................................................................*/
  int PacketLen = 0;  // size of packet without IPX header

  /*.....................................................................
  This is a real-mode pointer to the address of the real-mode assembly
  entry point.
  .....................................................................*/
  int32_t Handler = 0;

  /*.....................................................................
  Event Control Block for listening; contained within the real-mode
  assembly routine's data area
  .....................................................................*/
  ECBType* ListenECB = nullptr;  // ECB for listening

  /*.....................................................................
  ptr to the 1st header & data buffers in the packet buffer array
  .....................................................................*/
  IPXHeaderType* FirstHeaderBuf = nullptr;  // array of packet headers & buffers
  char* FirstDataBuf = nullptr;   // 1st data buffer area

  /*.....................................................................
  Current packet index & ptrs for parsing packets. Only the legacy DOS IPX
  path uses these; the Winsock path in Service() walks its own receive buffer
  with locals, so that nothing survives the call pointing into that frame.
  .....................................................................*/
#ifdef NOT_FOR_WIN95
  IPXHeaderType* CurHeaderBuf;  // Current packet ptr, for reading
  char* CurDataBuf;             // Current actual data ptr
#endif

  /*.....................................................................
  ECB, header, & buffer for sending
  .....................................................................*/
  ECBType* SendECB = nullptr;  // ECB for sending
  IPXHeaderType* SendHeader = nullptr;  // Header for sending
  char* SendBuf = nullptr;    // buffer for sending

  /*.....................................................................
  Various Statistics
  .....................................................................*/
  int SendOverflows;
  int ReceiveOverflows;
  int BadConnection;
};

#endif  // CNC_RED_ALERT_TD_IPXMGR_H_
/*************************** end of ipxmgr.h *******************************/
