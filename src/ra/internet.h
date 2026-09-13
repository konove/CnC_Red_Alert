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
 *                     $Archive:: /Sun/Internet.h $*
 *                                                                                             *
 *                      $Author:: Joe_b $*
 *                                                                                             *
 *                     $Modtime:: 8/05/97 6:45p $*
 *                                                                                             *
 *                    $Revision:: 7 $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_INTERNET_H_
#define CNC_RED_ALERT_RA_INTERNET_H_

#include <cstdint>

#define IP_ADDRESS_MAX 40

extern long PlanetWestwoodPortNumber;  // Port number to send to
extern bool
    PlanetWestwoodIsHost;  // True if this player controls the game options
extern uint32_t PlanetWestwoodGameID;  // Game ID
extern uint32_t PlanetWestwoodStartTime;
extern bool
    GameStatisticsPacketSent;  // True once the game statistics have been sent
extern bool
    ConnectionLost;  // True once the connection to the other player dropped

#endif  // CNC_RED_ALERT_RA_INTERNET_H_
