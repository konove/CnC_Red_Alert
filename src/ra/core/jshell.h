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

/* $Header: /CounterStrike/JSHELL.H 1     3/03/97 10:24a Joe_bostic $ */
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

#ifndef CNC_RED_ALERT_RA_CORE_JSHELL_H_
#define CNC_RED_ALERT_RA_CORE_JSHELL_H_

#include <cstdint>
#include <span>
#include <vector>

#include "absl/base/attributes.h"
#include "base/types.h"
#include "ra/assets/compat.h"
#include "ra/assets/palette.h"

namespace ra {

struct TLucentType {
  unsigned char SourceColor;
  unsigned char DestColor;
  unsigned char Fading;
  unsigned char reserved;
};

// Loads the compressed picture `filename` into `destbuf`, staging it in
// `scratchbuf`, and skips the palette it may carry. Returns the uncompressed
// size in units of 8000 bytes, 0 on failure.
int Load_Picture(const char* filename, std::span<uint8_t> scratchbuf,
                 std::span<uint8_t> destbuf);
std::span<unsigned char> Conquer_Build_Fading_Table(
    const PaletteClass& palette,
    std::span<unsigned char> dest ABSL_ATTRIBUTE_LIFETIME_BOUND, int color,
    int frac);
void Set_Window(int window, int x, int y, int w, int h);
std::span<unsigned char> Build_Translucent_Table(
    const PaletteClass& palette, std::span<const TLucentType> control,
    int count, std::span<unsigned char> buffer ABSL_ATTRIBUTE_LIFETIME_BOUND);
std::span<unsigned char> Conquer_Build_Translucent_Table(
    const PaletteClass& palette, std::span<const TLucentType> control,
    int count, std::span<unsigned char> buffer ABSL_ATTRIBUTE_LIFETIME_BOUND);
std::span<unsigned char> Make_Fading_Table(const PaletteClass& palette,
                                           std::span<unsigned char> dest
                                               ABSL_ATTRIBUTE_LIFETIME_BOUND,
                                           int color, int frac);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_CORE_JSHELL_H_
