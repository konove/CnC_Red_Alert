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

/* $Header:   F:\projects\c&c\vcs\code\jshell.h_v   2.16   16 Oct 1995 16:45:06
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : JSHELL.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 03/13/95 *
 *                                                                                             *
 *                  Last Update : March 13, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TD_CORE_JSHELL_H_
#define CNC_RED_ALERT_TD_CORE_JSHELL_H_

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "engine/gfx/cps_image.h"
#include "engine/gfx/pixel_buffer.h"

class ByteStream;

namespace td {

void Set_Window(int window, int x, int y, int w, int h);
// Allocates a buffer, reads the file into it, and null-terminates.
// Returns ownership of the buffer. Caller must delete[].
std::span<std::byte> Load_Alloc_Data(ByteStream& file);

// Reads a file into owned byte storage.
std::vector<std::byte> LoadAllocData(ByteStream& file);

// Copies `picture`'s pixels to the top of `page`, which must hold at least
// that many: the system-memory page holds a whole picture.
void CopyPictureToPage(const engine::gfx::CpsImage& picture, PixelBuffer& page);

}  // namespace td

#endif  // CNC_RED_ALERT_TD_CORE_JSHELL_H_
