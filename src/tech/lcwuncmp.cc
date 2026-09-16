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

/* $Header: /CounterStrike/LCWUNCMP.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***************************************************************************
 **    C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *               Project Name : WESTWOOD LIBRARY (PSX)                     *
 *                                                                         *
 *                 File Name : LCWUNCMP.CPP                                *
 *                                                                         *
 *                Programmer : Ian M. Leslie                               *
 *                                                                         *
 *                Start Date : May 17, 1995                                *
 *                                                                         *
 *               Last Update : May 17, 1995    [IML]                       *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "sdllib/iff.h"

extern "C" {

// Decodes into at most length bytes and returns the decoded prefix. Invalid
// back-references stop decoding. Source must contain complete commands: this
// legacy API has no compressed size with which to check source bounds.
int32_t __cdecl LCW_Uncompress(const void* source, void* dest, int32_t length) {
  if (length <= 0) {
    return 0;
  }

  const auto* source_ptr = static_cast<const unsigned char*>(source);
  auto* output = static_cast<unsigned char*>(dest);
  int32_t written = 0;
  while (written < length) {
    const unsigned char opcode = *source_ptr++;
    int count = 0;
    int offset = 0;
    if (!(opcode & 0x80)) {
      count = (opcode >> 4) + 3;
      const int distance = *source_ptr++ + ((opcode & 0x0f) * 256);
      if (distance == 0 || distance > written) {
        return written;
      }
      offset = written - distance;
    } else if (!(opcode & 0x40)) {
      if (opcode == 0x80) {
        return written;
      }
      count = std::min<int>(opcode & 0x3f, length - written);
      // Forward copies preserve the legacy in-place decompression behavior.
      for (int i = 0; i < count; ++i) {
        output[written++] = *source_ptr++;
      }
      continue;
    } else if (opcode == 0xfe) {
      count = source_ptr[0] + (source_ptr[1] << 8);
      const unsigned char value = source_ptr[2];
      source_ptr += 3;
      count = std::min<int>(count, length - written);
      std::memset(output + written, value, static_cast<std::size_t>(count));
      written += count;
      continue;
    } else {
      if (opcode == 0xff) {
        count = source_ptr[0] + (source_ptr[1] << 8);
        source_ptr += 2;
      } else {
        count = (opcode & 0x3f) + 3;
      }
      offset = source_ptr[0] + (source_ptr[1] << 8);
      source_ptr += 2;
      // A zero-length copy does not access its offset.
      if (count != 0 && offset >= written) {
        return written;
      }
    }

    count = std::min<int>(count, length - written);
    for (int i = 0; i < count; ++i) {
      output[written++] = output[offset++];
    }
  }
  return written;
}
}
