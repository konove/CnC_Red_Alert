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
 *                     $Archive:: /Sun/WSPIPX.cpp $*
 *                                                                                             *
 *                      $Author:: Joe_b $*
 *                                                                                             *
 *                     $Modtime:: 8/20/97 10:54a $*
 *                                                                                             *
 *                    $Revision:: 6 $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 *                                                                                             *
 * IPXInterfaceClass::IPXInterfaceClass -- Class constructor *
 * IPXInterfaceClass::Get_Network_Card_Address -- Get the ID of the installed
 *net card         * IPXInterfaceClass::Open_Socket -- Opens an IPX socket for
 *reading & writing                 * IPXInterfaceClass::Message_Handler --
 *Handler for windows messages relating to IPX          *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/wspipx.h"

#include <cstring>

#ifdef _WIN32
/*
** This file normally resides with the SDK. However, since it needs fixing up
*before watcom will
** compile it, it has been incorporated into the project.
*/
#include "wsnwlink.h"
#else

#endif

/***********************************************************************************************
 * IPXInterfaceClass::IPXInterfaceClass -- Class constructor *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/4/97 11:41AM ST : Created *
 *=============================================================================================*/
IPXInterfaceClass::IPXInterfaceClass() {
  /*
  ** Set the net and node addressed to their default values.
  */
  memset(BroadcastNet, 0xff, sizeof(BroadcastNet));
  memset(BroadcastNode, 0xff, sizeof(BroadcastNode));
  memset(MyNode, 0xff, sizeof(MyNode));
}

/***********************************************************************************************
 * IPXInterfaceClass::Get_Network_Card_Address -- Get the ID of the installed
 *net card         *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    card number to retrieve ID for * ptr to addr to return ID in *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/1/97 3:04PM ST : Created *
 *=============================================================================================*/
bool IPXInterfaceClass::Get_Network_Card_Address(int /*card_number*/,
                                                 SOCKADDR_IPX* /*addr*/) {
  return false;
}

/***********************************************************************************************
 * IPXInterfaceClass::Open_Socket -- Opens an IPX socket for reading & writing *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    SOCKET number to open. This is usually VIRGIN_SOCKET *
 *                                                                                             *
 * OUTPUT:   true if socket was opened without error *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/4/97 5:54PM ST : Created *
 *=============================================================================================*/
bool IPXInterfaceClass::Open_Socket(SOCKET /*socketnum*/) {
  return false;
}

/***********************************************************************************************
 * IPXInterfaceClass::Message_Handler -- Handler for windows messages relating
 *to IPX          *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Usual windoze message handler stuff *
 *                                                                                             *
 * OUTPUT:   0 if message was handled *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 8/4/97 5:55PM ST : Created *
 *=============================================================================================*/
