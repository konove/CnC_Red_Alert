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

#ifndef CNC_RED_ALERT_WINVQ_VQA32_UNVQ_H_
#define CNC_RED_ALERT_WINVQ_VQA32_UNVQ_H_

// File: the VQ frame decoders, which rebuild a frame's pixels from its vector
// pointers and the codebook of pixel blocks they index.
//
// Originally written by Denzil E. Long, Jr. at Westwood Studios, February
// 1995.

#include <span>

// Decodes a frame of 4x2-pixel blocks into buffer, a pixel buffer bufwidth
// bytes wide that starts at the image's top-left pixel. pointers holds one
// 16-bit entry per block, blocksperrow x numrows of them, stored as two
// planes: every entry's low byte, then every entry's high byte. An entry
// numbers an 8-byte block in codebook, or, when its high byte is 0x0F, is a
// solid block of the color in its low byte. Sizes that do not fit the spans
// draw nothing; an entry past the end of codebook stops the decode there.
void UnVQ_4x2(std::span<const unsigned char> codebook,
              std::span<const unsigned char> pointers,
              std::span<unsigned char> buffer, int blocksperrow, int numrows,
              int bufwidth);

// As UnVQ_4x2(), for 4x4-pixel blocks: codebook entries are 16 bytes, and a
// high byte of 0xFF marks a solid block.
void UnVQ_4x4(std::span<const unsigned char> codebook,
              std::span<const unsigned char> pointers,
              std::span<unsigned char> buffer, int blocksperrow, int numrows,
              int bufwidth);

#endif  // CNC_RED_ALERT_WINVQ_VQA32_UNVQ_H_
