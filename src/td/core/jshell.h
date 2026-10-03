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

#include "absl/base/attributes.h"
#include "base/types.h"
#include "engine/gfx/palette.h"

class ByteStream;

namespace td {

struct TLucentType {
  unsigned char SourceColor;
  unsigned char DestColor;
  unsigned char Fading;
  unsigned char reserved;
};

void Set_Window(int window, int x, int y, int w, int h);
// Allocates a buffer, reads the file into it, and null-terminates.
// Returns ownership of the buffer. Caller must delete[].
std::span<std::byte> Load_Alloc_Data(ByteStream& file);

// Reads a file into owned byte storage.
std::vector<std::byte> LoadAllocData(ByteStream& file);
// Reads one compressed picture from `file` and uncompresses it into
// `dest_buff`, staging it in `uncomp_buff` (which may be the same buffer).
// The bytes a picture reserves ahead of its data are its palette; the colors
// they hold replace those of `*palette` when `palette` is not nullptr, and are
// skipped otherwise.
// Returns the uncompressed size, 0 on failure.
int32_t Load_Uncompress(ByteStream& file, std::span<uint8_t> uncomp_buff,
                        std::span<uint8_t> dest_buff,
                        engine::gfx::Palette* palette);
std::span<unsigned char> Build_Translucent_Table(
    const engine::gfx::Palette& palette, std::span<const TLucentType> control,
    int count, std::span<unsigned char> buffer ABSL_ATTRIBUTE_LIFETIME_BOUND);
std::span<unsigned char> Conquer_Build_Translucent_Table(
    const engine::gfx::Palette& palette, std::span<const TLucentType> control,
    int count, std::span<unsigned char> buffer ABSL_ATTRIBUTE_LIFETIME_BOUND);

}  // namespace td

#endif  // CNC_RED_ALERT_TD_CORE_JSHELL_H_
