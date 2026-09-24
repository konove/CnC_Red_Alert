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

/****************************************************************************
 *
 *  File              : soscomp.h
 *  Date Created      : 6/1/94
 *  Description       :
 *
 *  Programmer(s)     : Nick Skrepetos
 *  Last Modification : 10/1/94 - 11:37:9 AM
 *  Additional Notes  : Modified by Denzil E. Long, Jr.
 *
 *****************************************************************************
 *            Copyright (c) 1994,  HMI, Inc.  All Rights Reserved            *
 ****************************************************************************/

#ifndef CNC_RED_ALERT_WINVQ_VQM32_SOSCOMP_H_
#define CNC_RED_ALERT_WINVQ_VQM32_SOSCOMP_H_

#include <cstdint>
#include <span>

// AdpcmStream: the state of an IMA ADPCM decoder, which runs on from one
// sound chunk to the next: the last sample, and the index of the quantizer
// step the next code is scaled by. A value-initialized stream is at its start.
struct AdpcmStream {
  int32_t predicted;
  int16_t step_index;
};

// Decodes IMA ADPCM from source to fill dest with samples of the given format,
// continuing from the state the last call left in stream. source and dest may
// overlap, with source at the end. Only 16-bit mono is supported. Returns
// false, decoding nothing, for another format or when source is too short.
bool DecodeAdpcmSound(AdpcmStream* stream, int channels, int bits_per_sample,
                      std::span<const uint8_t> source, std::span<uint8_t> dest);

#endif  // CNC_RED_ALERT_WINVQ_VQM32_SOSCOMP_H_
