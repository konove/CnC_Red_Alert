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

/* $Header: /CounterStrike/LCW.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : LCW.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 06/30/96 *
 *                                                                                             *
 *                  Last Update : June 30, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TECH_LCW_H_
#define CNC_RED_ALERT_TECH_LCW_H_

#include <cstddef>
#include <span>

// Decodes one LCW stream from `source` into `dest` without reading or writing
// outside either span. Returns the number of bytes written, or -1 if the
// stream is malformed: an operation runs past either span, a back-reference
// points outside the bytes written so far, or the end marker is missing.
int LcwUncompBounded(std::span<const std::byte> source,
                     std::span<std::byte> dest);

// Returns the largest stream LCW_Comp produces for `length` input bytes: the
// input stored as literal runs of at most 63 bytes, each behind one opcode,
// plus the end marker.
constexpr int LcwWorstCaseSize(int length) {
  return length + ((length + 62) / 63) + 1;
}

// Compresses in into out as one LCW stream ending in the 0x80 marker.
// Returns the stream size, or -1 if out cannot hold
// LcwWorstCaseSize(in.size()).
int LCW_Comp(std::span<const std::byte> in, std::span<std::byte> out);

#endif  // CNC_RED_ALERT_TECH_LCW_H_
