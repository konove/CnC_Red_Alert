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

/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Part of the FILEIO Library               *
 *                                                                         *
 *                    File Name : IFF.H                                    *
 *                                                                         *
 *                   Programmer : Scott K. Bowen                           *
 *                                                                         *
 *                   Start Date : April 20, 1994                           *
 *                                                                         *
 *                  Last Update : April 20, 1994   [SKB]                   *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef CNC_RED_ALERT_SDLLIB_IFF_H_
#define CNC_RED_ALERT_SDLLIB_IFF_H_

#include <cstddef>
#include <cstdint>
#include <span>

enum class PicturePlaneType {
  BM_AMIGA = 0,  // Bit plane format (8K per bitplane).
  BM_MCGA = 1,   // Byte per pixel format (64K).

  BM_DEFAULT = BM_MCGA  // Default picture format.
};
using enum PicturePlaneType;

/*
**	This is the compression type code.  This value is used in the compressed
**	file header to indicate the method of compression used.  Note that the
**	LZW method may not be supported.
*/
enum class CompressionType {
  NOCOMPRESS,  // No compression (raw data).
  LZW12,       // LZW 12 bit codes.
  LZW14,       // LZW 14 bit codes.
  HORIZONTAL,  // Run length encoding (RLE).
  LCW          // Westwood proprietary compression.
};
using enum CompressionType;

/*
**	Compressed blocks of data must start with this header structure.
**	Note that disk based compressed files have an additional two
**	leading bytes that indicate the size of the entire file.
*/

#pragma pack(push, 1)
typedef struct {
  char Method;    // Compression method (CompressionType).
  char pad;       // Reserved pad byte (always 0).
  uint32_t Size;  // Size of the uncompressed data.
  int16_t Skip;   // Number of bytes to skip before data.
} CompHeaderType;
#pragma pack(pop)

size_t Uncompress_Data(std::span<const unsigned char> src,
                       std::span<unsigned char> dst);

/*========================= Assembly Functions ============================*/

// Decodes a bounded LCW stream, returning the number of output bytes.
int32_t LCW_Uncompress(std::span<const std::byte> source,
                       std::span<std::byte> dest);
int32_t LCW_Uncompress(std::span<const unsigned char> source,
                       std::span<unsigned char> dest);

/*=========================================================================*/

#endif  // CNC_RED_ALERT_SDLLIB_IFF_H_
