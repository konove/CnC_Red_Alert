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

/* $Header: /CounterStrike/JSHELL.CPP 2     3/13/97 2:05p Steve_tall $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
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
 ** Conquer_Build_Translucent_Table -- Builds fading table for shadow colors
 *only.            * Fatal -- General purpose fatal error handler. *
 *   Load_Alloc_Data -- Allocates a buffer and loads the file into it. *
 *   Load_Uncompress -- Loads and uncompresses data to a buffer. * Set_Window --
 *Sets the window dimensions to that specified.                               *
 *   Small_Icon -- Create a small icon from a big one. * Translucent_Table_Size
 *-- Determines the size of a translucent table.                     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/jshell.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/strings/str_format.h"
#include "absl/types/span.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/seek_origin.h"
#include "base/types.h"
#include "port/format.h"
#include "port/safe_string.h"
#include "ra/compat.h"
#include "ra/monoc.h"
#include "ra/palette.h"
#include "ra/startup.h"
#include "sdllib/buffer.h"
#include "sdllib/iff.h"
#include "sdllib/misc.h"
#include "sdllib/ww_win.h"
#include "tech/file.h"
#include "tech/game_file.h"
#include "tech/rgb.h"

/***********************************************************************************************
 * Small_Icon -- Create a small icon from a big one. *
 *                                                                                             *
 *    This routine will extract the specified icon from the icon data file and
 *convert that    * icon into a small (3x3) representation. Typical use of this
 *mini-icon is for the radar   * map. *
 *                                                                                             *
 * INPUT:   iconptr  -- Pointer to the icon data file. *
 *                                                                                             *
 *          iconnum  -- The embedded icon number to convert into a small image.
 **
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the small icon imagery. This is exactly 9
 *bytes long.    *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/11/1995 JLB : Created. *
 *=============================================================================================*/
std::span<const unsigned char> Small_Icon(std::span<const std::byte> iconptr,
                                          int iconnum) {
  static unsigned char icon[9];
  const IconsetClass iconset(iconptr);
  const auto map = iconset.Map_Data();
  if (iconnum < 0 || static_cast<size_t>(iconnum) >= map.size()) {
    return {};
  }
  const size_t offset =
      static_cast<size_t>(base::At(map, static_cast<size_t>(iconnum))) * 24 *
      24;
  const auto pixels = iconset.Icon_Data();
  if (offset > pixels.size() || pixels.size() - offset < size_t{24} * 24) {
    return {};
  }
  for (int index = 0; index < 9; ++index) {
    base::At(icon, index) = base::At(
        pixels, offset + static_cast<size_t>((4 + (index % 3 * 8)) +
                                             ((4 + (index / 3 * 8)) * 24)));
  }
  return icon;
}

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
  base::At(base::At(WindowList, window), kWindowWidth) = w;
  base::At(base::At(WindowList, window), kWindowHeight) = h;
  base::At(base::At(WindowList, window), kWindowX) = x;
  base::At(base::At(WindowList, window), kWindowY) = y;
}

/***********************************************************************************************
 * Fatal -- General purpose fatal error handler. *
 *                                                                                             *
 *    This is a very simple general purpose fatal error handler. It goes
 *directly to text      * mode, prints the error, and then aborts with a failure
 *code.                             *
 *                                                                                             *
 * INPUT:   message  -- The text message to display. *
 *                                                                                             *
 *          ...      -- Any optional parameters that are used in formatting the
 *message.       *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   This routine never returns. The game exits immediately. *
 *                                                                                             *
 * HISTORY: * 10/17/1994 JLB : Created. *
 *=============================================================================================*/
void Fatal_Message(const std::string_view message) {
  // Prog_End();
  absl::FPrintF(stderr, "%s", message);
  Mono_Printf("%s", message);
  Emergency_Exit(EXIT_FAILURE);
}

void Format_Runtime_Text(std::span<char> buffer, const size_t size,
                         const char* format,
                         const absl::Span<const absl::FormatArg> args) {
  port::SafeCopy(buffer.first(size), port::FormatRuntime(format, args).c_str());
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
int32_t Load_Uncompress(File& file, BufferClass& uncomp_buff,
                        BufferClass& dest_buff,
                        std::span<unsigned char> reserved_data) {
  const bool opened = !file.IsOpen();
  if (opened && !file.Open()) {
    return 0;
  }
  const auto decode = [&] -> int32_t {
    uint16_t stored_size = 0;
    CompHeaderType header;
    if (!file.ReadObject(stored_size) || !file.ReadObject(header) ||
        stored_size < sizeof(header)) {
      return 0;
    }
    std::size_t size = stored_size - sizeof(header);
    if (header.Skip < 0 || std::cmp_greater(header.Skip, size)) {
      return 0;
    }
    if (header.Skip != 0) {
      size -= base::ToSize(header.Skip);
      if (!reserved_data.empty()) {
        if (std::cmp_greater(header.Skip, reserved_data.size()) ||
            file.Read(reserved_data.first(base::ToSize(header.Skip))) !=
                header.Skip) {
          return 0;
        }
      } else {
        file.Seek(header.Skip, SeekOrigin::kCurrent);
      }
      header.Skip = 0;
    }
    auto source = uncomp_buff.Get_Bytes();
    const auto dest = dest_buff.Get_Bytes();
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
    return static_cast<int32_t>(Uncompress_Data(source, dest));
  };
  const int32_t result = decode();
  if (opened) {
    file.Close();
  }
  return result;
}

int Load_Picture(const char* filename, BufferClass& scratchbuf,
                 BufferClass& destbuf, std::span<unsigned char> palette,
                 PicturePlaneType /*unused*/) {
  GameFile fc(filename);
  return Load_Uncompress(fc, scratchbuf, destbuf, palette) / 8000;
}

/***********************************************************************************************
 * Load_Alloc_Data -- Allocates a buffer and loads the file into it. *
 *                                                                                             *
 *    This is the C++ replacement for the Load_Alloc_Data function. It will
 *allocate the       * memory big enough to hold the file and then read the file
 *into it.                       *
 *                                                                                             *
 * INPUT:   file  -- The file to read. *
 *                                                                                             *
 *          mem   -- The memory system to use for allocation. *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the allocated and filled memory block. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 10/17/1994 JLB : Created. *
 *=============================================================================================*/
std::span<std::byte> Load_Alloc_Data(File& file) {
  const auto size = base::ToSize(file.Size());
  // The returned view carries the exact allocation extent; legacy callers
  // retain ownership. The extra NUL also supports files read as C strings.
  // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
  const std::span<char> storage(new char[size + 1], size + 1);
  file.Read(std::as_writable_bytes(storage.first(size)));
  base::At(storage, size) = '\0';
  return std::as_writable_bytes(storage.first(size));
}

// Modern RAII version that returns owned data as a vector.
std::vector<std::byte> LoadAllocData(File& file) {
  const base::ssize size = file.Size();
  std::vector<std::byte> data(base::ToSize(size));
  file.Read(std::span(data));
  return data;
}

/***********************************************************************************************
 * Translucent_Table_Size -- Determines the size of a translucent table. *
 *                                                                                             *
 *    Use this routine to determine how big the translucent table needs * to be
 *given the specified number of colors. This value is typically * used when
 *allocating the buffer for the translucent table. *
 *                                                                                             *
 * INPUT:   count -- The number of colors that are translucent. *
 *                                                                                             *
 * OUTPUT:  Returns the size of the translucent table. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 04/02/1994 JLB : Created. *
 *=============================================================================================*/
int32_t Translucent_Table_Size(int count) { return 256 + (256 * count); }

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
    const PaletteClass& palette, std::span<const TLucentType> control,
    int count, std::span<unsigned char> buffer) {
  if (count <= 0 || std::cmp_greater(count, control.size())) {
    return buffer;
  }
  const auto size = base::ToSize(Translucent_Table_Size(count));
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
    const PaletteClass& palette, std::span<const TLucentType> control,
    int count, std::span<unsigned char> buffer) {
  if (count <= 0 || std::cmp_greater(count, control.size())) {
    return buffer;
  }
  const auto size = base::ToSize(Translucent_Table_Size(count));
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

std::span<unsigned char> Make_Fading_Table(const PaletteClass& palette,
                                           std::span<unsigned char> dest,
                                           int color, int frac) {
  if (dest.size() >= PaletteClass::COLOR_COUNT) {
    auto ptr = dest.begin();

    /*
    **	Find an appropriate remap color index for every color in the palette.
    **	There are certain exceptions to this, but they are trapped within the
    **	loop.
    */
    for (int index = 0; index < PaletteClass::COLOR_COUNT; index++) {
      /*
      **	Find the color that, ideally, the working color should be
      *remapped *	to in the special remap range.
      */
      RGBClass trycolor = palette.at(index);
      trycolor.Adjust(frac, palette.at(color));  // Try to match this color.

      /*
      **	Search through the remap range to find the color that should be
      *remapped *	to. This special range is used for shadows or other
      *effects that are *	not compounded if additively applied.
      */
      *ptr++ = static_cast<unsigned char>(palette.Closest_Color(trycolor));
    }
  }
  return dest;
}

std::span<unsigned char> Conquer_Build_Fading_Table(
    const PaletteClass& palette, std::span<unsigned char> dest, int color,
    int frac) {
  if (dest.size() >= PaletteClass::COLOR_COUNT) {
    auto ptr = dest.begin();
    //		HSVClass desthsv = palette[color];

    /*
    **	Find an appropriate remap color index for every color in the palette.
    **	There are certain exceptions to this, but they are trapped within the
    **	loop.
    */
    for (int index = 0; index < PaletteClass::COLOR_COUNT; index++) {
      /*
      **	If this color should not be remapped, then it will be stored as
      *a remap *	to itself. This is effectively no remap.
      */
      if (index > PaletteClass::COLOR_COUNT - 16 || index == 0) {
        *ptr++ = static_cast<unsigned char>(index);
      } else {
        /*
        **	Find the color that, ideally, the working color should be
        *remapped *	to in the special remap range.
        */
        RGBClass trycolor = palette.at(index);
        trycolor.Adjust(frac, palette.at(color));  // Try to match this color.

        /*
        **	Search through the remap range to find the color that should be
        *remapped *	to. This special range is used for shadows or other
        *effects that are *	not compounded if additively applied.
        */
        int best = -1;
        int bvalue = 0;
        for (int id = PaletteClass::COLOR_COUNT - 16;
             id < PaletteClass::COLOR_COUNT - 1; id++) {
          const int diff = palette.at(id).Difference(trycolor);
          if (best == -1 || diff < bvalue) {
            best = id;
            bvalue = diff;
          }
        }
        *ptr++ = static_cast<unsigned char>(best);
      }
    }
  }
  return dest;
}
