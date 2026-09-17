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
#include <span>

#include "base/array.h"
#include "sdllib/iff.h"

int32_t LCW_Uncompress(std::span<const std::byte> source,
                       std::span<std::byte> dest) {
  std::size_t written = 0;
  while (written < dest.size() && !source.empty()) {
    const auto opcode = std::to_integer<unsigned char>(source.front());
    source = source.subspan(1);
    std::size_t count = 0;
    std::size_t offset = 0;
    if (!(opcode & 0x80)) {
      if (source.empty()) {
        break;
      }
      count = (opcode >> 4) + 3;
      const std::size_t distance =
          std::to_integer<unsigned char>(source.front()) +
          ((opcode & 0x0f) * 256);
      source = source.subspan(1);
      if (distance == 0 || distance > written) {
        break;
      }
      offset = written - distance;
    } else if (!(opcode & 0x40)) {
      if (opcode == 0x80) {
        break;
      }
      count = std::min<std::size_t>(opcode & 0x3f, dest.size() - written);
      count = std::min(count, source.size());
      // Forward copies preserve the legacy in-place decompression behavior.
      for (std::size_t i = 0; i < count; ++i) {
        base::At(dest, written++) = base::At(source, i);
      }
      source = source.subspan(count);
      continue;
    } else if (opcode == 0xfe) {
      if (source.size() < 3) {
        break;
      }
      count = std::to_integer<std::size_t>(base::At(source, 0)) +
              (std::to_integer<std::size_t>(base::At(source, 1)) << 8);
      const std::byte value = base::At(source, 2);
      source = source.subspan(3);
      count = std::min(count, dest.size() - written);
      std::ranges::fill(dest.subspan(written, count), value);
      written += count;
      continue;
    } else {
      if (opcode == 0xff) {
        if (source.size() < 2) {
          break;
        }
        count = std::to_integer<std::size_t>(base::At(source, 0)) +
                (std::to_integer<std::size_t>(base::At(source, 1)) << 8);
        source = source.subspan(2);
      } else {
        count = (opcode & 0x3f) + 3;
      }
      if (source.size() < 2) {
        break;
      }
      offset = std::to_integer<std::size_t>(base::At(source, 0)) +
               (std::to_integer<std::size_t>(base::At(source, 1)) << 8);
      source = source.subspan(2);
      // A zero-length copy does not access its offset.
      if (count != 0 && offset >= written) {
        break;
      }
    }
    count = std::min(count, dest.size() - written);
    for (std::size_t i = 0; i < count; ++i) {
      base::At(dest, written++) = base::At(dest, offset++);
    }
  }
  return static_cast<int32_t>(written);
}

int32_t LCW_Uncompress(std::span<const unsigned char> source,
                       std::span<unsigned char> dest) {
  return LCW_Uncompress(std::as_bytes(source), std::as_writable_bytes(dest));
}
