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
 *                     $Archive:: /Sun/WSPUDP.h $*
 *                                                                                             *
 *                      $Author:: Joe_b $*
 *                                                                                             *
 *                     $Modtime:: 8/05/97 6:45p $*
 *                                                                                             *
 *                    $Revision:: 3 $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_WSPUDP_H_
#define CNC_RED_ALERT_RA_WSPUDP_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "ra/vector_dynamic.h"
#include "ra/wsproto.h"
#include "sdllib/net_select.h"

/*
** Class to allow access to UDP specific portions of the Winsock interface.
**
*/
class UDPInterfaceClass : public WinsockInterfaceClass {
 public:
  UDPInterfaceClass();
  ~UDPInterfaceClass() override;
  UDPInterfaceClass(const UDPInterfaceClass&) = delete;
  UDPInterfaceClass& operator=(const UDPInterfaceClass&) = delete;
  UDPInterfaceClass(UDPInterfaceClass&&) = delete;
  UDPInterfaceClass& operator=(UDPInterfaceClass&&) = delete;
  void Event_Handler(int /*unused*/, SocketEvent /*event*/ /*unused*/) override;
  bool Open_Socket(SOCKET socketnum) override;
  void Set_Broadcast_Address(const char* address) override;
  void Broadcast(std::span<const std::byte> buffer, int buffer_len) override;

  ProtocolEnum Get_Protocol() override { return PROTOCOL_UDP; }

  int Protocol_Event_Message() override { return WM_UDPASYNCEVENT; }

 private:
  /*
  ** Address to use when broadcasting a packet.
  */
  DynamicVectorClass<uint32_t> BroadcastAddresses;

  /*
  ** List of local addresses.
  */
  DynamicVectorClass<uint32_t> LocalAddresses;
};

bool Get_Broadcast_Addresses();

#endif  // CNC_RED_ALERT_RA_WSPUDP_H_
