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

#ifndef _SOS_COMPRESS
#define _SOS_COMPRESS

#include <cstdint>

/* compression types */
enum {
  _ADPCM_TYPE_1,
};

/* define compression structure */
struct SosCompressInfo {
  std::uint8_t* source;
  std::uint8_t* dest;

  std::uint32_t comp_size;
  std::uint32_t uncomp_size;

  std::int16_t bit_size;
  std::int16_t channels;

  // --- Channel 1 State ---
  std::uint32_t sample_index;
  std::int32_t predicted;
  std::int32_t difference;
  std::int16_t code_buf;
  std::int16_t code;
  std::int16_t step_index;
  std::int16_t index;

  // Channel 2 Data
  std::uint32_t sample_index2;
  std::int32_t predicted2;
  std::int32_t difference2;
  std::int16_t code_buf2;
  std::int16_t code2;
  std::int16_t step_index2;
  std::int16_t index2;
};

/* compressed file type header */
typedef struct _tagCOMPRESS_HEADER {
  unsigned long dwType;              // type of compression
  unsigned long dwCompressedSize;    // compressed file size
  unsigned long dwUnCompressedSize;  // uncompressed file size
  unsigned long dwSourceBitSize;     // original bit size
  char szName[16];                   // file type, for error checking
} _SOS_COMPRESS_HEADER;

/* Prototypes */

#ifdef __cplusplus
extern "C" {
#endif

void __cdecl VQA_sosCODECInitStream(SosCompressInfo*);
unsigned long __cdecl VQA_sosCODECCompressData(SosCompressInfo*, unsigned long);
bool __cdecl DecompressVqaSosData(SosCompressInfo*, unsigned long);

#ifdef __cplusplus
}
#endif

#endif
