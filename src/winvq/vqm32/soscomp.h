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

// AdpcmStream: an IMA ADPCM stream being decoded. The VQA loader sets
// source, dest and the format for each sound chunk; the predictor and step
// index carry the decoder's state from one chunk to the next.
struct AdpcmStream {
  std::span<const uint8_t> source;
  std::span<uint8_t> dest;

  int16_t bits_per_sample;
  int16_t channels;

  // The decoder state: the last sample, and the index of the quantizer step
  // the next code is scaled by.
  int32_t predicted;
  int16_t step_index;
};

// Starts a stream: zeroes the predictor and the step index.
void ResetAdpcmStream(AdpcmStream* stream);
// Decodes IMA ADPCM from stream->source into output_bytes of stream->dest,
// continuing from the state the last call left, and advances source and dest
// past what it used. Only 16-bit mono is supported. Returns false, decoding
// nothing, for another format or when either span is too short.
bool DecodeAdpcmSound(AdpcmStream* stream, int32_t output_bytes);

#endif  // CNC_RED_ALERT_WINVQ_VQM32_SOSCOMP_H_
