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

/* $Header: /CounterStrike/IPXCONN.CPP 1     3/03/97 10:24a Joe_bostic $ */
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
 * Functions:                                                              *
 *   IPXConnClass::IPXConnClass -- class constructor                       *
 *   IPXConnClass::Send -- sends a packet; invoked by ConnectionClass      *
 *   IPXConnClass::Send_To -- sends the packet to the given address        *
 *   IPXConnClass::Broadcast -- broadcasts the given packet                *
 *                                                                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#include "engine/net/ipxconn.h"

#include <cstddef>
#include <cstdint>
#include <span>

#include "base/strings/safe_string.h"
#include "engine/net/connect.h"
#include "engine/net/datagram_transport.h"
#include "engine/net/ipxaddr.h"

namespace engine::net {

IPXConnClass::IPXConnClass(DatagramTransport& transport, int numsend,
                           int numreceive, int maxlen, uint16_t magicnum,
                           IPXAddressClass* address, int id, const char* name,
                           int extralen)
    : ConnectionClass(numsend, numreceive, maxlen, magicnum,
                      2,   // retry delta
                      -1,  // max retries
                      60,  // timeout
                      extralen),
      ID(id),
      transport_(transport) {
  if (address != nullptr) {
    Address = *address;
  }
  base::SafeCopy(Name, name);
}

int IPXConnClass::Send(std::span<const std::byte> buf, const int buflen,
                       std::span<const std::byte> /*extrabuf*/,
                       int /*extralen*/) {
  return Send_To(buf, buflen, Address);
}

int IPXConnClass::Send_To(std::span<const std::byte> buf, const int buflen,
                          const IPXAddressClass& address) {
  if (buflen < 0 || static_cast<std::size_t>(buflen) > buf.size()) {
    return 0;
  }
  transport_.Write_To(buf.first(static_cast<std::size_t>(buflen)), address);
  return 1;
}

bool IPXConnClass::Broadcast(std::span<const std::byte> buf, const int buflen) {
  if (buflen < 0 || static_cast<std::size_t>(buflen) > buf.size()) {
    return false;
  }
  transport_.Broadcast(buf.first(static_cast<std::size_t>(buflen)));
  return true;
}

}  // namespace engine::net
