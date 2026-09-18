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

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                     $Archive:: /Sun/WSPUDP.cpp $*
 *                                                                                             *
 *                      $Author:: Joe_b $*
 *                                                                                             *
 *                     $Modtime:: 8/05/97 6:45p $*
 *                                                                                             *
 *                    $Revision:: 3 $*
 *                                                                                             *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *  WSProto.CPP WinsockInterfaceClass to provide an interface to Winsock
 *protocols             *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Functions: * UDPInterfaceClass::UDPInterfaceClass -- Class constructor. *
 * UDPInterfaceClass::Set_Broadcast_Address -- Sets the address to send
 *broadcast packets to   * UDPInterfaceClass::Open_Socket -- Opens a socket for
 *communications via the UDP protocol    * TMC::Message_Handler -- Message
 *handler function for Winsock related messages               *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/wspudp.h"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <span>
#include <string_view>  // IWYU pragma: keep - used by an DCHECK() below.

#include "absl/log/check.h"
#include "absl/strings/str_format.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "port/socket_bytes.h"
#include "port/unaligned.h"
#include "ra/externs.h"
#include "ra/internet.h"
#include "ra/jshell.h"
#include "ra/wsproto.h"
#include "sdllib/net_select.h"

#ifdef _WIN32
#include <nspapi.h>
#include <svcguid.h>
#include <winsock2.h>
#include <ws2tcpip.h>

using socklen_t = int;
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define SOCKET_ERROR (-1)

#endif

/***********************************************************************************************
 * UDPInterfaceClass::UDPInterfaceClass -- Class constructor. *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/5/97 12:11PM ST : Created *
 *=============================================================================================*/
UDPInterfaceClass::UDPInterfaceClass() = default;

/***********************************************************************************************
 * UDPIC::~UDPInterfaceClass -- UDPInterface class destructor *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 10/9/97 12:17PM ST : Created *
 *=============================================================================================*/
UDPInterfaceClass::~UDPInterfaceClass() {
  while (BroadcastAddresses.Count()) {
    BroadcastAddresses.Delete(0);
  }

  while (LocalAddresses.Count() > 0) {
    LocalAddresses.Delete(0);
  }

  Close();
}

/***********************************************************************************************
 * UDPInterfaceClass::Set_Broadcast_Address -- Sets the address to send
 *broadcast packets to   *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to address in decimal dot format. i.e. xxx.xxx.xxx.xxx *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/5/97 12:12PM ST : Created *
 *=============================================================================================*/
void UDPInterfaceClass::Set_Broadcast_Address(const char* address) {
  const char* ip_addr = address;
  DCHECK(std::string_view(ip_addr).size() <=
         std::string_view("xxx.xxx.xxx.xxx").size());

  BroadcastAddresses.Add(inet_addr(ip_addr));
}

/***********************************************************************************************
 * UDPInterfaceClass::Open_Socket -- Opens a socket for communications via the
 *UDP protocol    *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Socket number to use. Not required for this protocol. *
 *                                                                                             *
 * OUTPUT:   True if socket was opened OK *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/5/97 12:13PM ST : Created *
 *=============================================================================================*/
bool UDPInterfaceClass::Open_Socket(SOCKET /*unused*/) {
  linger ling{};
  struct sockaddr_in addr{};

  /*
  ** If Winsock is not initialised then do it now.
  */
  if ((!WinsockInitialised) && (!Init())) {
    return false;
  }

  /*
  ** Create our UDP socket
  */
  Socket = socket(AF_INET, SOCK_DGRAM, 0);
  if (Socket == INVALID_SOCKET) {
    return false;
  }

  /*
  ** Bind our UDP socket to our UDP port number
  */
  addr.sin_family = AF_INET;
  addr.sin_port = htons(static_cast<uint16_t>(PlanetWestwoodPortNumber));
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(Socket, SocketAddress(addr), sizeof(addr)) == SOCKET_ERROR) {
    Close_Socket();
    return false;
  }

  /*
  ** Look up the local host's name to enumerate its IPv4 addresses.
  */
  char hostname[128];
  gethostname(hostname, 128);
  WWDebugString(hostname);
  addrinfo hints{};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  addrinfo* results = nullptr;
  if (getaddrinfo(hostname, nullptr, &hints, &results) != 0) {
    results = nullptr;
  }

  /*
  ** Clear out any old local addresses from the local address list.
  */
  while (LocalAddresses.Count() > 0) {
    LocalAddresses.Delete(0);
  }

  /*
  ** Add all local IP addresses to the list. This list will be used to discard
  *any packets that
  ** we send to ourselves.
  */
  for (const addrinfo* info = results; info != nullptr; info = info->ai_next) {
    if (info->ai_family != AF_INET || info->ai_addr == nullptr ||
        info->ai_addrlen < sizeof(sockaddr_in)) {
      continue;
    }
    // getaddrinfo owns ai_addr and supplies its readable extent in ai_addrlen.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto* bytes = reinterpret_cast<const std::byte*>(info->ai_addr);
    // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
    const std::span<const std::byte> address_bytes(bytes, info->ai_addrlen);
    const uint32_t address =
        port::ReadUnaligned<sockaddr_in>(address_bytes).sin_addr.s_addr;

    char temp[128];
    absl::SNPrintF(temp, sizeof(temp),
                   "RA95: Found local address: %d.%d.%d.%d\n",
                   static_cast<int>(address & 0xff),
                   static_cast<int>((address & 0xff00) >> 8),
                   static_cast<int>((address & 0xff0000) >> 16),
                   static_cast<int>((address & 0xff000000) >> 24));
    absl::PrintF("%s", temp);

    LocalAddresses.Add(address);
  }
  freeaddrinfo(results);

  /*
  ** Set options for the UDP socket
  */
  ling.l_onoff = 0;   // linger off
  ling.l_linger = 0;  // timeout in seconds (ie close now)
  setsockopt(Socket, SOL_SOCKET, SO_LINGER, SocketBytes(ling), sizeof(ling));

  // enable broadcast
  int yes = 1;
  setsockopt(Socket, SOL_SOCKET, SO_BROADCAST, SocketBytes(yes), sizeof(int));

  WinsockInterfaceClass::Set_Socket_Options();

  return true;
}

/***********************************************************************************************
 * UDPIC::Broadcast -- Send data via the Winsock socket *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to buffer containing data to send * length of data to send *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 3/20/96 3:00PM ST : Created *
 *=============================================================================================*/
void UDPInterfaceClass::Broadcast(std::span<const std::byte> buffer,
                                  int buffer_len) {
  for (int i = 0; i < BroadcastAddresses.Count(); i++) {
    /*
    ** Create a temporary holding area for the packet.
    */
    auto* packet = new WinsockBufferType;

    /*
    ** Copy the packet into the holding buffer.
    */
    base::CopyBytes(base::ObjectBytes(packet->Buffer), buffer,
                    base::ToSize(buffer_len));
    packet->BufferLen = buffer_len;

    /*
    ** Indicate that this packet should be broadcast.
    */
    packet->IsBroadcast = true;

    /*
    ** Set up the send address for this packet.
    */
    base::FillBytes(base::ObjectBytes(packet->Address), 0,
                    sizeof(packet->Address));
    base::CopyBytes(base::ObjectBytes(packet->Address).subspan(4),
                    base::ObjectBytes(BroadcastAddresses.at(i)), 4);

    /*
    ** Add it to our out list.
    */
    if (!OutBuffers.Add(packet)) {
      delete packet;
      continue;
    }

    // enable write events
    Socket_Check_Write(Socket, true);

    /*
    ** Make sure the message loop gets called.
    */
    Keyboard->Check();
  }
}

// like below, but less windows-y
void UDPInterfaceClass::Event_Handler(int /*socket*/, SocketEvent event) {
  struct sockaddr_in addr{};
  WinsockBufferType* packet = nullptr;

  switch (event) {
    case SOCKEV_READ: {
      /*
      ** Call the recvfrom function to get the outstanding packet.
      */
      socklen_t addr_len = sizeof(addr);
      const int rc = static_cast<int>(
          recvfrom(Socket, SocketBytes(ReceiveBuffer), sizeof(ReceiveBuffer), 0,
                   SocketAddress(addr), &addr_len));
      if (rc == SOCKET_ERROR) {
        Clear_Socket_Error(Socket);
        return;
      }

      /*
      ** rc is the number of bytes received
      */
      if (rc) {
        /*
        ** Make sure this packet didn't come from us. If it did then throw it
        *away.
        */
        for (int i = 0; i < LocalAddresses.Count(); i++) {
          if (LocalAddresses.at(i) == addr.sin_addr.s_addr) {
            return;
          }
        }

        /*
        ** Create a new buffer and store this packet in it.
        */
        packet = new WinsockBufferType;
        packet->BufferLen = rc;
        base::CopyBytes(base::ObjectBytes(packet->Buffer),
                        base::ObjectBytes(ReceiveBuffer), base::ToSize(rc));
        base::FillBytes(base::ObjectBytes(packet->Address), 0,
                        sizeof(packet->Address));
        base::CopyBytes(base::ObjectBytes(packet->Address).subspan(4),
                        base::ObjectBytes(addr.sin_addr.s_addr), 4);
        if (!InBuffers.Add(packet)) {
          delete packet;
        }
      }
      break;
    }
    case SOCKEV_WRITE: {
      /*
      ** If there are no packets waiting to be sent then bail.
      */
      if (OutBuffers.Count() == 0) {
        Socket_Check_Write(Socket,
                           false);  // don't need to be notified any more
        return;
      }
      const int packetnum = 0;

      /*
      ** Get a pointer to the packet.
      */
      packet = OutBuffers.at(packetnum);

      /*
      ** Set up the address structure of the outgoing packet
      */
      addr.sin_family = AF_INET;
      addr.sin_port = htons(static_cast<uint16_t>(PlanetWestwoodPortNumber));
      base::CopyBytes(base::ObjectBytes(addr.sin_addr.s_addr),
                      base::ObjectBytes(packet->Address).subspan(4), 4);

      /*
      ** Send it.
      ** If we get a WSAWOULDBLOCK error it means that Winsock is unable to
      *accept the packet
      ** at this time. In this case, we clear the socket error and just exit.
      *Winsock will
      ** send us another WRITE message when it is ready to receive more data.
      */
      const int rc = static_cast<int>(sendto(
          Socket, SocketBytes(packet->Buffer), base::ToSize(packet->BufferLen),
          0, SocketAddress(addr), sizeof(addr)));

      if (rc == -1) {
        if (Get_Last_Error() == EWOULDBLOCK) {
          Clear_Socket_Error(Socket);
          return;
        }
      }

      /*
      ** Delete the sent packet.
      */
      OutBuffers.Delete(packetnum);
      delete packet;

      break;
    }
    case SocketEvent::SOCKEV_ERROR:
    default:
      break;
  }
}
