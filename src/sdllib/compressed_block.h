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
// CompressedBlockHeader, a reserved area the header's skip_bytes counts (a CPS
// keeps its palette there), then the data itself.

#ifndef CNC_RED_ALERT_SDLLIB_COMPRESSED_BLOCK_H_
#define CNC_RED_ALERT_SDLLIB_COMPRESSED_BLOCK_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "engine/base/types.h"

// The compression method, as stored in CompressedBlockHeader::method. The
// values are the file format's. Only NOCOMPRESS and LCW are decoded.
enum class CompressionMethod {
  NOCOMPRESS,  // No compression (raw data).
  LZW12,       // LZW 12 bit codes.
  LZW14,       // LZW 14 bit codes.
  HORIZONTAL,  // Run length encoding (RLE).
  LCW          // Westwood proprietary compression.
};
using enum CompressionMethod;

// The header every compressed block starts with, little-endian and unpadded
// as on disk. A block stored in a file is preceded by two more bytes giving
// the size of the rest of it: this header, the skipped area and the data.
#pragma pack(push, 1)
struct CompressedBlockHeader {
  char method;                  // Compression method (CompressionMethod).
  char pad;                     // Reserved pad byte (always 0).
  uint32_t uncompressed_bytes;  // Size of the uncompressed data.
  int16_t skip_bytes;           // Bytes between this header and the data.
};
#pragma pack(pop)

// Decodes `block`, which starts with its CompressedBlockHeader, into the
// front of `dest`. Returns the number of bytes written: uncompressed_bytes for
// uncompressed data, what the stream produced for LCW. Returns 0 for any other
// method, a negative skip_bytes, a `block` too short for the header and the
// skipped area, or a `dest` too short for uncompressed_bytes.
base::ssize UncompressBlock(std::span<const std::byte> block,
                            std::span<std::byte> dest);

#endif  // CNC_RED_ALERT_SDLLIB_COMPRESSED_BLOCK_H_
