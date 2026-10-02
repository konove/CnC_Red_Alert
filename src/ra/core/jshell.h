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

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "absl/base/attributes.h"
#include "absl/log/check.h"
#include "absl/strings/str_format.h"
#include "absl/types/span.h"
#include "base/array.h"
#include "base/numeric.h"
#include "base/strings/format.h"
#include "base/types.h"
#include "ra/assets/compat.h"
#include "ra/assets/palette.h"

class ByteStream;

namespace ra {


// Bit indices must refer to the supplied word span.
inline void Set_Bit(std::span<uint32_t> array, int bit, int value) {
  CHECK_GE(bit, 0);
  CHECK_LT(base::ToSize(bit / 32), array.size());
  if (value) {
    base::At(array, base::ToSize(bit / 32)) |= base::Bit<uint32_t>(bit % 32);
  } else {
    base::At(array, base::ToSize(bit / 32)) &= ~base::Bit<uint32_t>(bit % 32);
  }
}

inline bool Get_Bit(std::span<const uint32_t> array, int bit) {
  CHECK_GE(bit, 0);
  CHECK_LT(base::ToSize(bit / 32), array.size());
  return (base::At(array, base::ToSize(bit / 32)) &
          base::Bit<uint32_t>(bit % 32)) != 0;
}

// Returns -1 if no matching bit exists within the supplied words.
inline int First_True_Bit(std::span<const uint32_t> array) {
  int off = 0;
  for (const uint32_t word : array) {
    const int pos = std::countr_zero(word);
    if (pos < 32) {
      return off + pos;
    }
    off += 32;
  }
  return -1;
}

inline int First_False_Bit(std::span<const uint32_t> array) {
  int off = 0;
  for (const uint32_t word : array) {
    const int pos = std::countr_zero(~word);
    if (pos < 32) {
      return off + pos;
    }
    off += 32;
  }
  return -1;
}

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
std::span<const unsigned char> Small_Icon(std::span<const std::byte> iconptr,
                                          int iconnum);
void Set_Window(int window, int x, int y, int w, int h);
// Reads one compressed picture from `file` and uncompresses it into
// `dest_buff`, staging it in `uncomp_buff` (which may be the same buffer),
// skipping the palette it may carry. Returns the uncompressed size, 0 on
// failure.
int32_t Load_Uncompress(ByteStream& file, std::span<uint8_t> uncomp_buff,
                        std::span<uint8_t> dest_buff);
int32_t Translucent_Table_Size(int count);
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

// Prints `format` with `args`, checked at compile time, to stderr and the
// mono page, then exits with a failure code.
[[noreturn]] void Fatal_Message(std::string_view message);
template <typename... Args>
[[noreturn]] void Fatal(const absl::FormatSpec<Args...>& format,
                        const Args&... args) {
  Fatal_Message(absl::StrFormat(format, args...));
}

// Formats `format`, known only at run time (the string table or a format
// handed in by a caller), with `args` into `buffer`, which holds `size` bytes.
// The result is always null terminated and truncated rather than allowed to
// overflow. Each conversion is checked against its argument; a format that
// does not match `args` is copied unformatted (see base::FormatRuntime).
void Format_Runtime_Text(std::span<char> buffer, size_t size,
                         const char* format,
                         absl::Span<const absl::FormatArg> args = {});
template <typename... Args>
  requires(sizeof...(Args) > 0)
void Format_Runtime_Text(std::span<char> buffer, const size_t size,
                         const char* format, const Args&... args) {
  const auto packed = base::MakeFormatArgs(args...);
  Format_Runtime_Text(buffer, size, format, absl::MakeConstSpan(packed));
}

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_CORE_JSHELL_H_
