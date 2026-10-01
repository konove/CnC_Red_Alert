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

/* $Header: /CounterStrike/IPXCONN.H 1     3/03/97 10:24a Joe_bostic $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : IPXCONN.H                                *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : December 19, 1994                        *
 *                                                                         *
 *                  Last Update : April 9, 1995   [BR] *
 *                                                                         *
 *-------------------------------------------------------------------------*
 *                                                                         *
 * This is the Connection Class for IPX communications.  It inherits      *
 * a Queue, PacketBuf, timeout variables, the Send_/Receive_/Get_Packet    *
 * functions and the ACK/Retry logic from ConnectionClass.  Its packets    *
 * travel over the DatagramTransport it is given.                          *
 *                                                                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef CNC_RED_ALERT_ENGINE_NET_IPXCONN_H_
#define CNC_RED_ALERT_ENGINE_NET_IPXCONN_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "absl/base/attributes.h"
#include "engine/net/connect.h"
#include "engine/net/ipxaddr.h"

namespace engine::net {

class DatagramTransport;

// One player's private channel: a ConnectionClass that sends to Address
// through the manager's transport.
class IPXConnClass : public ConnectionClass {
 public:
  static constexpr int kConnNameMax = 40;  // max # chars for the name

  // Sends through `transport`, which must outlive the connection. `address`
  // may be null to leave Address the broadcast address. `extralen` reserves
  // that many extra bytes per queue entry, for the global channel.
  IPXConnClass(DatagramTransport& transport ABSL_ATTRIBUTE_LIFETIME_BOUND,
               int numsend, int numreceive, int maxlen, uint16_t magicnum,
               IPXAddressClass* address, int id, const char* name,
               int extralen = 0);
  ~IPXConnClass() override = default;
  IPXConnClass(const IPXConnClass&) = delete;
  IPXConnClass& operator=(const IPXConnClass&) = delete;
  IPXConnClass(IPXConnClass&&) = delete;
  IPXConnClass& operator=(IPXConnClass&&) = delete;

  // The destination address for this connection.
  IPXAddressClass Address;

  // Each connection has a name and a unique numerical ID.
  int ID;
  char Name[kConnNameMax]{};

 protected:
  // Sends to Address; the extra bytes are not sent.
  int Send(std::span<const std::byte> buf, int buflen,
           std::span<const std::byte> extrabuf, int extralen) override;

  // Sends the first `buflen` bytes of `buf` to `address`, or to everyone.
  // Return 0 (false) for a bad length.
  int Send_To(std::span<const std::byte> buf, int buflen,
              const IPXAddressClass& address);
  bool Broadcast(std::span<const std::byte> buf, int buflen);

 private:
  DatagramTransport& transport_;
};

}  // namespace engine::net

#endif  // CNC_RED_ALERT_ENGINE_NET_IPXCONN_H_
