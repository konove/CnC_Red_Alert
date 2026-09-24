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

#ifndef CNC_RED_ALERT_WINVQ_VQA32_VQAFILE_H_
#define CNC_RED_ALERT_WINVQ_VQA32_VQAFILE_H_

// File: the on-disk layout of a VQA movie - the VQHD header, the FINF frame
// table's bit fields, and the chunk IDs inside the WVQA form.
//
// A VQA file is an IFF FORM of type WVQA. The loader reads the VQHD header and
// the FINF frame table up front, then walks the per-frame chunks (codebooks,
// vector pointers, palettes, sound) as playback proceeds.
//
// Originally written by Denzil E. Long, Jr. at Westwood Studios, April 1995.

#include <array>
#include <cstdint>

#include "winvq/vqm32/iff.h"

// VQAHeader: the payload of the VQHD chunk, read straight off the disk.
//
// The loader rejects a VQHD whose size is not sizeof(VQAHeader), so the packing
// and field order are the file format. The fields are little-endian and read
// raw, which assumes a little-endian host. A version 1 movie's audio fields are
// ignored: its sound is always 22050 Hz 8-bit mono (see AllocBuffers in
// loader.cc).
#pragma pack(push, 1)
struct VQAHeader {
  uint16_t Version;      // VQAHD_VER1 or VQAHD_VER2
  uint16_t Flags;        // VQAHDF_* bits
  uint16_t Frames;       // Total number of frames in the movie
  uint16_t ImageWidth;   // Frame width in pixels
  uint16_t ImageHeight;  // Frame height in pixels
  uint8_t BlockWidth;    // Compression block width in pixels, never 0
  uint8_t BlockHeight;   // Compression block height in pixels, never 0
  uint8_t FPS;           // Playback frame rate (frames per second), never 0
  // Frames per codebook group, never 0. Each group's codebook arrives in
  // pieces (partial codebooks) during the previous group, which is why seeking
  // starts one group early.
  uint8_t Groupsize;
  // Number of single-color blocks. Unused by the player.
  uint16_t Num1Colors;
  uint16_t CBentries;  // Number of codebook entries, sizes the codebook buffer
  // Where the encoder wanted the frames drawn; 0xFFFF (-1) centers on that
  // axis. Unused by the player, which takes the position from VQAConfig.
  uint16_t Xpos;
  uint16_t Ypos;
  uint16_t MaxFramesize;  // Size of the largest frame in bytes. Unused.
  // The primary audio track, meaningful when Flags has VQAHDF_AUDIO.
  uint16_t SampleRate;    // Sample rate in Hz
  uint8_t Channels;       // Number of channels
  uint8_t BitsPerSample;  // Sample bit depth
  // The alternate audio track, meaningful when Flags has VQAHDF_ALTAUDIO.
  uint16_t AltSampleRate;
  uint8_t AltChannels;
  uint8_t AltBitsPerSample;
  std::array<uint16_t, 5> FutureUse;  // Reserved, pads the header to 42 bytes
};
#pragma pack(pop)

// VQAHeader::Version values.
#define VQAHD_VER1 1
#define VQAHD_VER2 2

// VQAHeader::Flags bit numbers (VQAHDB_*) and masks (VQAHDF_*).
#define VQAHDB_AUDIO 0     // The movie has a primary audio track.
#define VQAHDB_ALTAUDIO 1  // The movie has an alternate audio track.
#define VQAHDF_AUDIO (1U << VQAHDB_AUDIO)
#define VQAHDF_ALTAUDIO (1U << VQAHDB_ALTAUDIO)

// Frame information (FINF) entries.
//
// The FINF chunk holds one 32-bit entry per frame, flags on top of an offset:
//
//   Bits   Name    Description
//   31-28  Flags   Up to 4 boolean flags; VQAFINB_* names the three in use.
//   27-0   Offset  Where the frame's chunks start, in 16-bit words from the
//                  start of the file. Chunks are padded to even sizes, so
//                  halving the offset loses nothing and reaches 512 MB.
//
// Seeking uses both: it replays the nearest palette frame at or before the
// target, then starts reading at the codebook group before it.
#define VQAFINB_KEY 31   // Key frame.
#define VQAFINB_PAL 30   // The frame carries a palette.
#define VQAFINB_SYNC 29  // Audio synchronization point.
#define VQAFINF_KEY (1L << VQAFINB_KEY)
#define VQAFINF_PAL (uint32_t{1} << VQAFINB_PAL)
#define VQAFINF_SYNC (1L << VQAFINB_SYNC)

// Masks for the two halves of a FINF entry, and its byte offset in the file.
#define VQAFINF_OFFSET 0x0FFFFFFFU
#define VQAFINF_FLAGS 0xF0000000L
#define VQAFRAME_OFFSET(a) (((a) & VQAFINF_OFFSET) * 2)

// Vector pointer codes of the Run-Skip-Dump (RSD) pointer compression. The
// player does not decode RSD (it skips the VPTR and VPRZ chunks below), so
// these, the long run codes and the length limits only document the format.
#define VPC_ONE_SINGLE 0xF000     // One single-color block.
#define VPC_ONE_SEMITRANS 0xE000  // One semitransparent block.
#define VPC_SHORT_DUMP 0xD000     // Short dump of single-color blocks.
#define VPC_LONG_DUMP 0xC000      // Long dump of single-color blocks.
#define VPC_SHORT_RUN 0xB000      // Short run of single-color blocks.
#define VPC_LONG_RUN 0xA000       // Long run.

// Long run codes.
#define LRC_SEMITRANS 0xC000  // Long run of semitransparent blocks.
#define LRC_SINGLE 0x8000     // Long run of single-color blocks.

// Run and dump length limits of the RSD compression, in blocks. 15 and 4095
// are the largest lengths 4 and 12 bits can hold.
#define MIN_SHORT_RUN_LENGTH 2
#define MAX_SHORT_RUN_LENGTH 15
#define MIN_LONG_RUN_LENGTH 2
#define MAX_LONG_RUN_LENGTH 4095
#define MIN_SHORT_DUMP_LENGTH 3
#define MAX_SHORT_DUMP_LENGTH 15
#define MIN_LONG_DUMP_LENGTH 2
#define MAX_LONG_DUMP_LENGTH 4095

// The top bit of a 16-bit word. Unused.
#define WORD_HI_BIT 0x8000

// VQA chunk IDs. MakeId packs the four characters in file order, so these
// compare equal to an ID read raw from the disk. A "Z" suffix means the payload
// is LCW compressed. The player skips the chunks it has no case for (NAME,
// VPTR, VPRZ, SNDZ, SNAZ, CAP0, EVA0).
#define ID_WVQA MakeId('W', 'V', 'Q', 'A')  // Westwood VQ Animation form.
#define ID_VQHD MakeId('V', 'Q', 'H', 'D')  // VQ header (VQAHeader).
#define ID_NAME MakeId('N', 'A', 'M', 'E')  // Name string.
#define ID_FINF MakeId('F', 'I', 'N', 'F')  // Frame information table.
#define ID_VQFR MakeId('V', 'Q', 'F', 'R')  // VQ frame container.
#define ID_VQFK MakeId('V', 'Q', 'F', 'K')  // VQ key frame container.
#define ID_CBF0 MakeId('C', 'B', 'F', '0')  // Full codebook.
#define ID_CBFZ MakeId('C', 'B', 'F', 'Z')  // Full codebook (compressed).
#define ID_CBP0 MakeId('C', 'B', 'P', '0')  // Partial codebook.
#define ID_CBPZ MakeId('C', 'B', 'P', 'Z')  // Partial codebook (compressed).
#define ID_VPT0 MakeId('V', 'P', 'T', '0')  // Vector pointers.
#define ID_VPTZ MakeId('V', 'P', 'T', 'Z')  // Vector pointers (compressed).
#define ID_VPTK \
  MakeId('V', 'P', 'T', 'K')  // Vector pointers (delta key frame).
#define ID_VPTD MakeId('V', 'P', 'T', 'D')  // Vector pointers (delta).
#define ID_VPTR MakeId('V', 'P', 'T', 'R')  // Vector pointers (RSD compressed).
#define ID_VPRZ MakeId('V', 'P', 'R', 'Z')  // Vector pointers (RSD, then LCW).
#define ID_CPL0 MakeId('C', 'P', 'L', '0')  // Color palette.
#define ID_CPLZ MakeId('C', 'P', 'L', 'Z')  // Color palette (compressed).

// Sound for the primary track (SND*) and the alternate track (SNA*); the
// loader keeps one track and skips the other's chunks.
#define ID_SND0 MakeId('S', 'N', 'D', '0')  // Sound (uncompressed).
#define ID_SND1 MakeId('S', 'N', 'D', '1')  // Sound (Zap compressed).
#define ID_SND2 MakeId('S', 'N', 'D', '2')  // Sound (ADPCM compressed).
#define ID_SNDZ MakeId('S', 'N', 'D', 'Z')  // Sound (LCW compressed).
#define ID_SNA0 MakeId('S', 'N', 'A', '0')  // Sound (uncompressed).
#define ID_SNA1 MakeId('S', 'N', 'A', '1')  // Sound (Zap compressed).
#define ID_SNA2 MakeId('S', 'N', 'A', '2')  // Sound (ADPCM compressed).
#define ID_SNAZ MakeId('S', 'N', 'A', 'Z')  // Sound (LCW compressed).

#define ID_CAP0 MakeId('C', 'A', 'P', '0')  // Caption text.
#define ID_EVA0 MakeId('E', 'V', 'A', '0')  // EVA text.

#endif  // CNC_RED_ALERT_WINVQ_VQA32_VQAFILE_H_
