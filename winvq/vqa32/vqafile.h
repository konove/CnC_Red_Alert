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

#ifndef VQAFILE_H
#define VQAFILE_H
/****************************************************************************
 *
 *         C O N F I D E N T I A L -- W E S T W O O D  S T U D I O S
 *
 *----------------------------------------------------------------------------
 *
 * PROJECT
 *     VQA player library. (32-Bit protected mode)
 *
 * FILE
 *     vqafile.h
 *
 * DESCRIPTION
 *     VQA file format definitions.
 *
 * PROGRAMMER
 *     Denzil E. Long, Jr.
 *
 * DATE
 *     April 10, 1995
 *
 ****************************************************************************/

#include <array>
#include <cstdint>

#include "vqm32/iff.h"

/*---------------------------------------------------------------------------
 * STRUCTURE DEFINITIONS AND RELATED DEFINES.
 *-------------------------------------------------------------------------*/

// VQAHeader: VQA movie description header (VQHD chunk)
#pragma pack(push, 1)
struct VQAHeader {
  uint16_t Version;          // VQA format version number
  uint16_t Flags;            // Bitfield flags for various options
  uint16_t Frames;           // Total number of frames in the movie
  uint16_t ImageWidth;       // Frame width in pixels
  uint16_t ImageHeight;      // Frame height in pixels
  uint8_t BlockWidth;        // Compression block width in pixels
  uint8_t BlockHeight;       // Compression block height in pixels
  uint8_t FPS;               // Playback frame rate (frames per second)
  uint8_t Groupsize;         // Frame grouping size (frames per codebook)
  uint16_t Num1Colors;       // Number of single-color blocks
  uint16_t CBentries;        // Number of codebook entries
  uint16_t Xpos;             // X position for drawing frames (-1 = center)
  uint16_t Ypos;             // Y position for drawing frames (-1 = center)
  uint16_t MaxFramesize;     // Size of largest frame in bytes
  uint16_t SampleRate;       // Audio sample rate (Hz) for primary stream
  uint8_t Channels;          // Number of audio channels in primary stream
  uint8_t BitsPerSample;     // Audio sample bit depth in primary stream
  uint16_t AltSampleRate;    // Audio sample rate (Hz) for alternate stream
  uint8_t AltChannels;       // Number of audio channels in alternate stream
  uint8_t AltBitsPerSample;  // Audio sample bit depth in alternate stream
  std::array<uint16_t, 5> FutureUse;  // Reserved for future expansion
};
#pragma pack(pop)

/* Version type. */
#define VQAHD_VER1 1
#define VQAHD_VER2 2

/* VQA header flag definitions */
#define VQAHDB_AUDIO 0    /* Audio track present. */
#define VQAHDB_ALTAUDIO 1 /* Alternate audio track present. */
#define VQAHDF_AUDIO (1 << VQAHDB_AUDIO)
#define VQAHDF_ALTAUDIO (1 << VQAHDB_ALTAUDIO)

/* Frame information (FINF) chunk definitions
 *
 * The FINF chunk contains a longword (4 bytes) entry for each
 * frame in the movie. This entry is divided into two parts,
 * flags (4 bits) and offset (28 bits).
 *
 * BITS   NAME     DESCRIPTION
 * -----------------------------------------------------------
 * 31-28  Flags    4 bitwise boolean flags.
 * 27-0   Offset   Offset in WORDS from the start of the file.
 */
#define VQAFINB_KEY 31
#define VQAFINB_PAL 30
#define VQAFINB_SYNC 29
#define VQAFINF_KEY (1L << VQAFINB_KEY)
#define VQAFINF_PAL (1L << VQAFINB_PAL)
#define VQAFINF_SYNC (1L << VQAFINB_SYNC)

/* FINF related defines and macros. */
#define VQAFINF_OFFSET 0x0FFFFFFFL
#define VQAFINF_FLAGS 0xF0000000L
#define VQAFRAME_OFFSET(a) (((a & VQAFINF_OFFSET) << 1))

/* VQ vector pointer codes. */
#define VPC_ONE_SINGLE 0xF000    /* One single color block */
#define VPC_ONE_SEMITRANS 0xE000 /* One semitransparent block */
#define VPC_SHORT_DUMP 0xD000    /* Short dump of single color blocks */
#define VPC_LONG_DUMP 0xC000     /* Long dump of single color blocks */
#define VPC_SHORT_RUN 0xB000     /* Short run of single color blocks */
#define VPC_LONG_RUN 0xA000      /* Long run */

/* Long run codes. */
#define LRC_SEMITRANS 0xC000 /* Long run of semitransparent blocks. */
#define LRC_SINGLE 0x8000    /* Long run of single color blocks. */

/* Defines used for Run-Skip-Dump compression. */
#define MIN_SHORT_RUN_LENGTH 2
#define MAX_SHORT_RUN_LENGTH 15
#define MIN_LONG_RUN_LENGTH 2
#define MAX_LONG_RUN_LENGTH 4095
#define MIN_SHORT_DUMP_LENGTH 3
#define MAX_SHORT_DUMP_LENGTH 15
#define MIN_LONG_DUMP_LENGTH 2
#define MAX_LONG_DUMP_LENGTH 4095

#define WORD_HI_BIT 0x8000

/*---------------------------------------------------------------------------
 * VQA FILE CHUNK ID DEFINITIONS.
 *-------------------------------------------------------------------------*/

#define ID_WVQA MAKE_ID('W', 'V', 'Q', 'A') /* Westwood VQ Animation form. */
#define ID_VQHD MAKE_ID('V', 'Q', 'H', 'D') /* VQ header. */
#define ID_NAME MAKE_ID('N', 'A', 'M', 'E') /* Name string. */
#define ID_FINF MAKE_ID('F', 'I', 'N', 'F') /* Frame information. */
#define ID_VQFR MAKE_ID('V', 'Q', 'F', 'R') /* VQ frame container. */
#define ID_VQFK MAKE_ID('V', 'Q', 'F', 'K') /* VQ key frame container. */
#define ID_CBF0 MAKE_ID('C', 'B', 'F', '0') /* Full codebook. */
#define ID_CBFZ MAKE_ID('C', 'B', 'F', 'Z') /* Full codebook (compressed). */
#define ID_CBP0 MAKE_ID('C', 'B', 'P', '0') /* Partial codebook. */
#define ID_CBPZ                                                               \
  MAKE_ID('C', 'B', 'P', 'Z')               /* Partial codebook (compressed). \
                                             */
#define ID_VPT0 MAKE_ID('V', 'P', 'T', '0') /* Vector pointers. */
#define ID_VPTZ                                                              \
  MAKE_ID('V', 'P', 'T', 'Z')               /* Vector pointers (compressed). \
                                             */
#define ID_VPTK MAKE_ID('V', 'P', 'T', 'K') /* Vector pointers (Delta Key). */
#define ID_VPTD MAKE_ID('V', 'P', 'T', 'D') /* Vector pointers (Delta). */
#define ID_VPTR MAKE_ID('V', 'P', 'T', 'R') /* Pointers RSD compressed. */
#define ID_VPRZ                                                              \
  MAKE_ID('V', 'P', 'R', 'Z')               /* Pointers RSD, lcw compressed. \
                                             */
#define ID_CPL0 MAKE_ID('C', 'P', 'L', '0') /* Color palette. */
#define ID_CPLZ MAKE_ID('C', 'P', 'L', 'Z') /* Color palette (compressed). */
#define ID_SND0 MAKE_ID('S', 'N', 'D', '0') /* Sound */
#define ID_SND1 MAKE_ID('S', 'N', 'D', '1') /* Sound (Zap compressed). */
#define ID_SND2 MAKE_ID('S', 'N', 'D', '2') /* Sound (ADPCM compressed). */
#define ID_SNDZ MAKE_ID('S', 'N', 'D', 'Z') /* Sound (LCW compression). */

#define ID_SNA0 MAKE_ID('S', 'N', 'A', '0') /* Sound */
#define ID_SNA1 MAKE_ID('S', 'N', 'A', '1') /* Sound (Zap compressed). */
#define ID_SNA2 MAKE_ID('S', 'N', 'A', '2') /* Sound (ADPCM compressed). */
#define ID_SNAZ MAKE_ID('S', 'N', 'A', 'Z') /* Sound (LCW compression). */

#define ID_CAP0 MAKE_ID('C', 'A', 'P', '0') /* Caption text */
#define ID_EVA0 MAKE_ID('E', 'V', 'A', '0') /* EVA text */

#endif /* VQAFILE_H */
