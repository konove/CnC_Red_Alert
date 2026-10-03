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

/* $Header:   F:\projects\c&c\vcs\code\jshell.cpv   2.18   16 Oct 1995 16:51:12
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : JSHELL.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 2, 1994 *
 *                                                                                             *
 *                  Last Update : May 11, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:
 *   Load_Alloc_Data -- Allocates a buffer and loads the file into it.
 *   Set_Window -- Sets the window dimensions to that specified.
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/core/jshell.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

#include "absl/log/check.h"
#include "base/array.h"
#include "base/numeric.h"
#include "engine/gfx/cps_image.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/text_window.h"
#include "engine/stream/byte_stream.h"

namespace td {

/***********************************************************************************************
 * Set_Window -- Sets the window dimensions to that specified. *
 *                                                                                             *
 *    Use this routine to set the windows dimensions to the coordinates and
 *dimensions         * specified. *
 *                                                                                             *
 * INPUT:   x     -- Window X pixel position. *
 *                                                                                             *
 *          y     -- Window Y pixel position. *
 *                                                                                             *
 *          w     -- Window width in pixels. *
 *                                                                                             *
 *          h     -- Window height in pixels. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   The X and width values are truncated to an even 8 pixel boundary.
 *This is       * the same as stripping off the lower 3 bits. *
 *                                                                                             *
 * HISTORY: * 01/15/1995 JLB : Created. *
 *=============================================================================================*/
void Set_Window(int window, int x, int y, int w, int h) {
  base::At(base::At(WindowList, window), kWindowWidth) = w / 8;
  base::At(base::At(WindowList, window), kWindowHeight) = h;
  base::At(base::At(WindowList, window), kWindowX) = x / 8;
  base::At(base::At(WindowList, window), kWindowY) = y;
}

void CopyPictureToPage(const engine::gfx::CpsImage& picture,
                       PixelBuffer& page) {
  const std::span<uint8_t> bytes = page.bytes();
  CHECK_LE(picture.pixels.size(), bytes.size());
  std::ranges::copy(picture.pixels, bytes.begin());
}

std::vector<std::byte> LoadAllocData(ByteStream& file) {
  std::vector<std::byte> data(base::ToSize(file.Size()));
  file.Read(std::span(data));
  return data;
}

std::span<std::byte> Load_Alloc_Data(ByteStream& file) {
  const auto size = base::ToSize(file.Size());
  // The returned view carries the exact allocation extent; legacy callers
  // retain ownership. The extra NUL also supports files read as C strings.
  // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
  const std::span<char> storage(new char[size + 1], size + 1);
  file.Read(std::as_writable_bytes(storage.first(size)));
  base::At(storage, size) = '\0';
  return std::as_writable_bytes(storage.first(size));
}

}  // namespace td
