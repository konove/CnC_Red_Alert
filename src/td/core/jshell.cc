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
 * Functions: * Build_Translucent_Table -- Creates a translucent control table.
 *   Conquer_Build_Translucent_Table -- Builds fading table for shadow colors
 *only.            * Load_Alloc_Data -- Allocates a buffer and loads the file
 *into it.                         * Load_Uncompress -- Loads and uncompresses
 *data to a buffer.                               * Set_Window
 *-- Sets the window dimensions to that specified. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/core/jshell.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <utility>
#include <vector>

#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/types.h"
#include "engine/codec/compressed_block.h"
#include "engine/gfx/fading_table.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/text_window.h"
#include "engine/stream/byte_stream.h"
#include "engine/stream/seek_origin.h"
#include "td/core/support.h"

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


/***********************************************************************************************
 * Load_Uncompress -- Loads and uncompresses data to a buffer. *
 *                                                                                             *
 *    This is the C++ counterpart to the Load_Uncompress function. It will load
 *the file       * specified into the graphic buffer indicated and uncompress
 *it.                           *
 *                                                                                             *
 * INPUT:   file     -- The file to load and uncompress. *
 *                                                                                             *
 *          uncomp_buff -- The graphic buffer that initial loading will use. *
 *                                                                                             *
 *          dest_buff   -- The buffer that will hold the uncompressed data. *
 *                                                                                             *
 *          reserved_data  -- This is an optional pointer to a buffer that will
 *hold any       * reserved data the compressed file may contain. This is *
 *                            typically a palette. *
 *                                                                                             *
 * OUTPUT:  Returns with the size of the uncompressed data in the destination
 *buffer.          *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/17/1994 JLB : Created. *
 *=============================================================================================*/
int32_t Load_Uncompress(ByteStream& file, std::span<uint8_t> uncomp_buff,
                        std::span<uint8_t> dest_buff,
                        engine::gfx::Palette* const palette) {
  const auto decode = [&] -> int32_t {
    uint16_t stored_size = 0;
    CompressedBlockHeader header{};
    if (!file.ReadObject(stored_size) || !file.ReadObject(header) ||
        stored_size < sizeof(header)) {
      return 0;
    }
    std::size_t size = stored_size - sizeof(header);
    if (header.skip_bytes < 0 || base::ToSize(header.skip_bytes) > size) {
      return 0;
    }
    if (header.skip_bytes != 0) {
      size -= base::ToSize(header.skip_bytes);
      if (palette != nullptr) {
        std::array<uint8_t, engine::gfx::Palette::kFileSize> vga{};
        if (header.skip_bytes > engine::gfx::Palette::kFileSize ||
            file.Read(std::span(vga), header.skip_bytes) != header.skip_bytes) {
          return 0;
        }
        // A short palette replaces only the colors it holds whole.
        const engine::gfx::Palette loaded = engine::gfx::Palette::FromVga(vga);
        std::ranges::copy(
            loaded.colors().first(base::ToSize(header.skip_bytes / 3)),
            palette->colors().begin());
      } else {
        file.Seek(header.skip_bytes, SeekOrigin::kCurrent);
      }
      header.skip_bytes = 0;
    }
    auto source = uncomp_buff;
    const auto dest = dest_buff;
    const auto packet_size = size + sizeof(header);
    if (packet_size > source.size()) {
      return 0;
    }
    if (source.data() == dest.data()) {
      source = source.last(packet_size);
    } else {
      source = source.first(packet_size);
    }
    base::CopyBytes(std::as_writable_bytes(source), base::ObjectBytes(header),
                    sizeof(header));
    if (file.Read(source.subspan(sizeof(header))) !=
        static_cast<base::ssize>(size)) {
      return 0;
    }
    return static_cast<int32_t>(
        UncompressBlock(std::as_bytes(source), std::as_writable_bytes(dest)));
  };
  return decode();
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

namespace {

// A translucent table is a 256-byte map from source color to fading table,
// followed by one 256-entry fading table per translucent color.
constexpr base::ssize TranslucentTableSize(int count) {
  return 256 * (base::ssize{count} + 1);
}

}  // namespace

/***********************************************************************************************
 * Build_Translucent_Table -- Creates a translucent control table. *
 *                                                                                             *
 *    The table created by this routine is used by Draw_Shape (GHOST) to *
 *    achieve a translucent affect. The original color of the shape will * show
 *through. This differs from the fading effect, since that * affect only alters
 *the background color toward a single destination                      * color.
 **
 *                                                                                             *
 * INPUT:   palette  -- Pointer to the control palette. *
 *                                                                                             *
 *          control  -- Pointer to array of structures that control how * the
 *translucent table will be built.                                   *
 *                                                                                             *
 *          count    -- The number of entries in the control array. *
 *                                                                                             *
 *          buffer   -- Pointer to buffer to place the translucent table. * If
 *NULL is passed in, then the buffer will be                          *
 *                      allocated. *
 *                                                                                             *
 * OUTPUT:  Returns with pointer to the translucent table. *
 *                                                                                             *
 * WARNINGS:   This routine is exceedingly slow. Use sparingly. *
 *                                                                                             *
 * HISTORY: * 04/02/1994 JLB : Created. *
 *=============================================================================================*/
std::span<unsigned char> Build_Translucent_Table(
    const engine::gfx::Palette& palette, std::span<const TLucentType> control,
    int count, std::span<unsigned char> buffer) {
  if (count <= 0 || std::cmp_greater(count, control.size())) {
    return buffer;
  }
  const auto size = base::ToSize(TranslucentTableSize(count));
  if (buffer.empty()) {
    // This legacy allocation is returned with its exact size to the caller.
    // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
    buffer = std::span<unsigned char>(new unsigned char[size], size);
  }
  if (buffer.size() < size) {
    return {};
  }
  std::ranges::fill(buffer.first(256), static_cast<unsigned char>(255));
  for (int index = 0; index < count; ++index) {
    const auto& item = base::At(control, base::ToSize(index));
    base::At(buffer, item.SourceColor) = static_cast<unsigned char>(index);
    Build_Fading_Table(palette,
                       buffer.subspan(base::ToSize(index + 1) * 256, 256),
                       item.DestColor, item.Fading);
  }
  return buffer;
}

/***********************************************************************************************
 * Conquer_Build_Translucent_Table -- Builds fading table for shadow colors
 *only.              *
 *                                                                                             *
 *    This routine will build a translucent (fading) table to remap colors into
 *the shadow     * color region of the palette. Shadow colors are not affected
 *by this translucent table.   * This means that a shape can be overlapped any
 *number of times and the imagery will       * remain deterministic (and
 *constant).                                                     *
 *                                                                                             *
 * INPUT:   palette  -- Pointer to the palette to base the translucent process
 *on.             *
 *                                                                                             *
 *          control  -- Pointer to special control structure that specifies the
 ** target color, and percentage of fade.                                  *
 *                                                                                             *
 *          count    -- The number of colors to be remapped (entries in the
 *control array).    *
 *                                                                                             *
 *          buffer   -- Pointer to the staging buffer that will hold the
 *translucent table     * data. If this parameter is NULL, then an appropriate
 *sized table       * will be allocated. *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the translucent table data. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/27/1994 JLB : Created. *
 *=============================================================================================*/
std::span<unsigned char> Conquer_Build_Translucent_Table(
    const engine::gfx::Palette& palette, std::span<const TLucentType> control,
    int count, std::span<unsigned char> buffer) {
  if (count <= 0 || std::cmp_greater(count, control.size())) {
    return buffer;
  }
  const auto size = base::ToSize(TranslucentTableSize(count));
  if (buffer.empty()) {
    // This legacy allocation is returned with its exact size to the caller.
    // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
    buffer = std::span<unsigned char>(new unsigned char[size], size);
  }
  if (buffer.size() < size) {
    return {};
  }
  std::ranges::fill(buffer.first(256), static_cast<unsigned char>(255));
  for (int index = 0; index < count; ++index) {
    const auto& item = base::At(control, base::ToSize(index));
    base::At(buffer, item.SourceColor) = static_cast<unsigned char>(index);
    Conquer_Build_Fading_Table(
        palette, buffer.subspan(base::ToSize(index + 1) * 256, 256),
        item.DestColor, item.Fading);
  }
  return buffer;
}

}  // namespace td
