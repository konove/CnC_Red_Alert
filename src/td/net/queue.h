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

/* $Header:   F:\projects\c&c\vcs\code\queue.h_v   2.16   16 Oct 1995 16:45:50
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : QUEUE.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 12/08/94 *
 *                                                                                             *
 *                  Last Update : December 9, 1994 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * QueueClass<T,size>::Add -- Add object to queue. *
 *   QueueClass<T,size>::First -- Fetches reference to first object in list. *
 *   QueueClass<T,size>::Init -- Initializes queue to empty state. *
 *   QueueClass<T,size>::Next -- Throws out the head of the line. *
 *   QueueClass<T,size>::operator[] -- Fetches reference to sub object in queue.
 ** QueueClass<T,size>::QueueClass -- Default constructor for QueueClass
 *objects.             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TD_NET_QUEUE_H_
#define CNC_RED_ALERT_TD_NET_QUEUE_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "td/core/defines.h"

namespace td {

bool Queue_Target(Target whom, Target target);
bool Queue_Destination(Target whom, Target target);
bool Queue_Mission(Target whom, MissionType mission);
bool Queue_Mission(Target whom, MissionType mission, Target target,
                   Target destination);
bool Queue_Options();
bool Queue_Exit();
void Queue_AI();

// Decode the events packed in `buf` onto DoList. Return the number of events
// read.
int Extract_Uncompressed_Events(std::span<const std::byte> buf, int bufsize);
int Extract_Compressed_Events(std::span<const std::byte> buf, int bufsize);
void Add_CRC(uint32_t* crc, uint32_t val);

}  // namespace td

#endif  // CNC_RED_ALERT_TD_NET_QUEUE_H_
