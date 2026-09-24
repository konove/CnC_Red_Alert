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

#ifndef CNC_RED_ALERT_WINVQ_VQA32_VQ_DECODER_H_
#define CNC_RED_ALERT_WINVQ_VQA32_VQ_DECODER_H_

// File: the VQ frame decoders, which rebuild a frame's pixels from its vector
// pointers and the codebook of pixel blocks they index.
//
// Originally written by Denzil E. Long, Jr. at Westwood Studios, February
// 1995.

#include <optional>
#include <span>

// The pixel blocks a movie's frames are made of; only these have a decoder.
enum class BlockShape {
  k4x2,  // 4x2-pixel blocks, 8 bytes each in the codebook.
  k4x4,  // 4x4-pixel blocks, 16 bytes each.
};

// Returns the shape of width x height blocks, or nullopt for a size with no
// decoder.
std::optional<BlockShape> BlockShapeFor(int width, int height);

// Decodes a frame of blocks of the given shape into buffer, a pixel buffer
// stride bytes wide that starts at the image's top-left pixel. pointers holds
// one 16-bit entry per block, blocks_per_row x block_rows of them, stored as
// two planes: every entry's low byte, then every entry's high byte. An entry
// numbers a block in codebook, or, when its high byte is 0x0F (4x2) or 0xFF
// (4x4), is a solid block of the color in its low byte. Sizes that do not fit
// the spans draw nothing; an entry past the end of codebook stops the decode
// there.
void DecodeVqFrame(BlockShape shape, std::span<const unsigned char> codebook,
                   std::span<const unsigned char> pointers,
                   std::span<unsigned char> buffer, int blocks_per_row,
                   int block_rows, int stride);

#endif  // CNC_RED_ALERT_WINVQ_VQA32_VQ_DECODER_H_
