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

/* $Header: /CounterStrike/IPXMGR.H 1     3/03/97 10:24a Joe_bostic $ */
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

#ifndef CNC_RED_ALERT_ENGINE_NET_IPXMGR_H_
#define CNC_RED_ALERT_ENGINE_NET_IPXMGR_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

#include "absl/base/attributes.h"
#include "engine/net/connmgr.h"
#include "engine/net/global_conn.h"
#include "engine/net/ipxaddr.h"
#include "engine/net/ipxconn.h"

namespace engine::net {

class DatagramTransport;

// Virgin Interactive Entertainment's registered socket ID.
inline constexpr uint16_t kVirginSocket = 0x8813;

class IPXManagerClass : public ConnManClass {
 public:
  // The most private connections any game asks for.
  static constexpr int kMaxConnections = 7;

  // Builds a game's global channel over `transport`: `num_packets` queue
  // entries each way, packets of up to `maxlen` bytes, tagged with
  // `product_id`.
  using GlobalChannelFactory = std::unique_ptr<GlobalConnClass> (*)(
      DatagramTransport& transport, int num_packets, int maxlen,
      uint16_t product_id);

  // Called by Service() in an Internet game for a private, no-ACK packet
  // from an address no connection has; `payload` follows the packet's
  // CommHeaderType. Red Alert uses it to follow a player whose address
  // changed.
  using UnknownSenderHandler = void (*)(IPXManagerClass& manager,
                                        std::span<const std::byte> payload,
                                        const IPXAddressClass& from);

  // `transport` must outlive the manager. `available` is false when the
  // transport cannot work outside an Internet game (Init(false) then fails).
  // Allows at most `max_connections` (capped at kMaxConnections) private
  // connections. Opens nothing: Init() does.
  IPXManagerClass(
      DatagramTransport& transport ABSL_ATTRIBUTE_LIFETIME_BOUND,
      bool available,
      GlobalChannelFactory make_global_channel ABSL_ATTRIBUTE_LIFETIME_BOUND,
      int max_connections, int glb_maxlen, int pvt_maxlen, int glb_num_packets,
      int pvt_num_packets, uint16_t socket, uint16_t product_id);
  ~IPXManagerClass() override;
  IPXManagerClass(const IPXManagerClass&) = delete;
  IPXManagerClass& operator=(const IPXManagerClass&) = delete;
  IPXManagerClass(IPXManagerClass&&) = delete;
  IPXManagerClass& operator=(IPXManagerClass&&) = delete;

  // Drops every connection, builds a fresh global channel and starts
  // listening (in an Internet game the transport is already connected, so
  // nothing is opened). Returns 1 on success, 0 if IPX is unavailable or the
  // transport cannot listen.
  int Init(bool internet);

  // Stops listening for packets, if the manager is.
  void Stop_Listening();
  [[nodiscard]] bool Is_IPX() const;
  void Set_Timing(int32_t retrydelta, int32_t maxretries,
                  int32_t timeout) override;

  // See UnknownSenderHandler; null (the default) ignores such packets.
  void Set_Unknown_Sender_Handler(UnknownSenderHandler handler) {
    UnknownSender = handler;
  }

  /*.....................................................................
  These routines control creation of the "Connections" (data queues) for
  each remote system.
  .....................................................................*/
  int Create_Connection(int id, char* name, IPXAddressClass* address);
  int Delete_Connection(int id);
  int Num_Connections() override;
  int Connection_ID(int index) override;
  // Both point into the connection, which lives until Delete_Connection.
  char* Connection_Name(int id) ABSL_ATTRIBUTE_LIFETIME_BOUND;
  IPXAddressClass* Connection_Address(int id) ABSL_ATTRIBUTE_LIFETIME_BOUND;
  int Connection_Index(int id) override;
  void Set_Connection_Parms(int index, int id, char* name);

  /*.....................................................................
  This is how the application sends & receives messages.
  .....................................................................*/
  int Send_Global_Message(std::span<const std::byte> buf, int buflen,
                          int ack_req = 0, IPXAddressClass* address = nullptr);
  int Get_Global_Message(std::span<std::byte> buf, int* buflen,
                         IPXAddressClass* address, uint16_t* product_id);
  int Send_Private_Message(std::span<const std::byte> buf, int buflen,
                           int ack_req = 1,
                           int conn_id = kConnectionNone) override;
  int Get_Private_Message(std::span<std::byte> buf, int* buflen,
                          int* conn_id) override;

  /*.....................................................................
  The main polling routine; should be called as often as possible.
  .....................................................................*/
  int Service() override;

  /*.....................................................................
  This routine reports which connection has an error on it.
  .....................................................................*/
  [[nodiscard]] int Get_Bad_Connection() const;

  /*.....................................................................
  Queue utility routines.  The application can determine how many
  messages are in the send/receive queues.
  .....................................................................*/
  int Global_Num_Send() override;
  int Global_Num_Receive() override;
  int Private_Num_Send(int id = kConnectionNone) override;
  int Private_Num_Receive(int id = kConnectionNone) override;

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
  This routine returns the oldest non-ACK'd buffer I've sent, or an empty
  span when there is none.
  .....................................................................*/
  std::span<const std::byte> Oldest_Send();

 private:
  DatagramTransport& Transport;
  GlobalChannelFactory MakeGlobalChannel;
  UnknownSenderHandler UnknownSender = nullptr;

  bool IPXStatus;          // false = no IPX outside an Internet game
  bool Listening = false;  // true = Listening is on
  bool Internet = false;   // the game Init() set up is an Internet game

  /*.....................................................................
  Packet sizes and queue lengths for the channels.
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

  // Our socket ID for sending/receiving, high byte first.
  uint16_t Socket;

  /*.....................................................................
  Array of connection queues
  .....................................................................*/
  int MaxConnections;  // the most connections allowed, <= kMaxConnections
  std::unique_ptr<IPXConnClass> Connection[kMaxConnections];
  int NumConnections = 0;  // # connection objects in use
  std::unique_ptr<GlobalConnClass> GlobalChannel;  // the Global Channel

  /*.....................................................................
  Current queue for polling for received packets
  .....................................................................*/
  int CurConnection = 0;

  /*.....................................................................
  Timing parameters for all connections
  .....................................................................*/
  int32_t RetryDelta = 2;   // ticks between retries
  int32_t MaxRetries = -1;  // -1 means no limit
  int32_t Timeout = 60;     // -1 means no limit; 60 ticks is one second

  /*.....................................................................
  Various Statistics
  .....................................................................*/
  int BadConnection = kConnectionNone;
};

}  // namespace engine::net

#endif  // CNC_RED_ALERT_ENGINE_NET_IPXMGR_H_

/*************************** end of ipxmgr.h *******************************/
