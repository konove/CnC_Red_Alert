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

/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : IPX95PP                                  *
 *                                                                         *
 *                   Programmer : Steve Tall                               *
 *                                                                         *
 *                   Start Date : January 22nd, 1996                       *
 *                                                                         *
 *                  Last Update : January 22nd, 1996   [ST]                *
 *                                                                         *
 *-------------------------------------------------------------------------*
 *                                                                         *
 *                                                                         *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *                                                                         *
 *                                                                         *
 *                                                                         *
 *                                                                         *
 *                                                                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef CNC_RED_ALERT_RA_IPX95_H_
#define CNC_RED_ALERT_RA_IPX95_H_

/*
** Types for function pointers
*/
using IPXInitialiseType = int (*)();
using IPXGetOutstandingBuffer95Type = int (*)(unsigned char*);
using IPXShutDown95Type = void (*)();
using IPXSendPacket95Type = int (*)(unsigned char*, unsigned char*, int,
                                    unsigned char*, unsigned char*);
using IPXBroadcastPacket95Type = int (*)(unsigned char*, int);
using IPXStartListening95Type = int (*)();
using IPXOpenSocket95Type = int (*)(int);
using IPXCloseSocket95Type = void (*)(int);
using IPXGetConnectionNumber95Type = int (*)();
using IPXGetLocalTarget95 = int (*)(unsigned char*, unsigned char*,
                                    unsigned short, unsigned char*);

/*
** Function pointers
*/
// extern "C"{
extern IPXInitialiseType IPX_Initialise;
extern IPXGetOutstandingBuffer95Type IPX_Get_Outstanding_Buffer95;
extern IPXShutDown95Type IPX_Shut_Down95;
extern IPXSendPacket95Type IPX_Send_Packet95;
extern IPXBroadcastPacket95Type IPX_Broadcast_Packet95;
extern IPXStartListening95Type IPX_Start_Listening95;
extern IPXOpenSocket95Type IPX_Open_Socket95;
extern IPXCloseSocket95Type IPX_Close_Socket95;
extern IPXGetConnectionNumber95Type IPX_Get_Connection_Number95;
extern IPXGetLocalTarget95 IPX_Get_Local_Target95;
//}

/*
** Functions
*/
bool Load_IPX_Dll();
void Unload_IPX_Dll();

extern bool WindowsNT;

#endif  // CNC_RED_ALERT_RA_IPX95_H_
