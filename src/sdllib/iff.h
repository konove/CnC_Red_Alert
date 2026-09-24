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

// Westwood's compressed data block, the format of the .CPS pictures: a
// CompHeaderType, a reserved area the header's Skip counts (a CPS keeps its
// palette there), then the data itself.

#ifndef CNC_RED_ALERT_SDLLIB_IFF_H_
#define CNC_RED_ALERT_SDLLIB_IFF_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "base/types.h"

// The pixel layout a picture is loaded into. Only byte per pixel is used.
enum class PicturePlaneType {
  BM_AMIGA = 0,  // Bit plane format (8K per bitplane).
  BM_MCGA = 1,   // Byte per pixel format (64K).

  BM_DEFAULT = BM_MCGA  // Default picture format.
};
using enum PicturePlaneType;

// The compression method, as stored in CompHeaderType::Method. The values are
// the file format's. Only NOCOMPRESS and LCW are decoded.
enum class CompressionType {
  NOCOMPRESS,  // No compression (raw data).
  LZW12,       // LZW 12 bit codes.
  LZW14,       // LZW 14 bit codes.
  HORIZONTAL,  // Run length encoding (RLE).
  LCW          // Westwood proprietary compression.
};
using enum CompressionType;

// The header every compressed block starts with, little-endian and unpadded
// as on disk. A block stored in a file is preceded by two more bytes giving
// the size of the rest of it: this header, the skipped area and the data.
#pragma pack(push, 1)
struct CompHeaderType {
  char Method;    // Compression method (CompressionType).
  char pad;       // Reserved pad byte (always 0).
  uint32_t Size;  // Size of the uncompressed data.
  int16_t Skip;   // Bytes between this header and the data.
};
#pragma pack(pop)

// Decodes the block in `src`, which starts with its CompHeaderType, into the
// front of `dst`. Returns the number of bytes written: the header's Size for
// uncompressed data, what the stream produced for LCW. Returns 0 for any other
// method, a negative Skip, a `src` too short for the header and the skipped
// area, or a `dst` too short for Size.
base::ssize Uncompress_Data(std::span<const std::byte> src,
                            std::span<std::byte> dst);

#endif  // CNC_RED_ALERT_SDLLIB_IFF_H_
