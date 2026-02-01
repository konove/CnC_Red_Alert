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

#ifndef CNC_RED_ALERT_WINVQ_VQA32_UNVQ_H_
#define CNC_RED_ALERT_WINVQ_VQA32_UNVQ_H_

/****************************************************************************
 *
 *         C O N F I D E N T I A L -- W E S T W O O D  S T U D I O S
 *
 *----------------------------------------------------------------------------
 *
 * PROJECT
 *     VQAPlay32 library. (32-Bit protected mode)
 *
 * FILE
 *     unvq.h
 *
 * DESCRIPTION
 *     VQ frame decompress definitions.
 *
 * PROGRAMMER
 *     Denzil E. Long, Jr.
 *
 * DATE
 *     Feburary 8, 1995
 *
 ****************************************************************************/

void UnVQ_4x2(const unsigned char* codebook, const unsigned char* pointers,
              unsigned char* buffer, unsigned long blocksperrow,
              unsigned long numrows, unsigned long bufwidth);

void UnVQ_4x4(const unsigned char* codebook, const unsigned char* pointers,
              unsigned char* buffer, unsigned long blocksperrow,
              unsigned long numrows, unsigned long bufwidth);

#endif  // CNC_RED_ALERT_WINVQ_VQA32_UNVQ_H_
