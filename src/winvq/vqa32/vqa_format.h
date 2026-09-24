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

#ifndef CNC_RED_ALERT_WINVQ_VQA32_VQA_FORMAT_H_
#define CNC_RED_ALERT_WINVQ_VQA32_VQA_FORMAT_H_

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

#include "base/numeric.h"

// VqaHeader: the payload of the VQHD chunk, read straight off the disk.
//
// The loader rejects a VQHD whose size is not sizeof(VqaHeader), so the packing
// and field order are the file format. The fields are little-endian and read
// raw, which assumes a little-endian host. A version 1 movie's audio fields are
// ignored: its sound is always 22050 Hz 8-bit mono (see AllocateMovie in
// loader.cc).
#pragma pack(push, 1)
struct VqaHeader {
  uint16_t version;       // kVqaVersion1 or kVqaVersion2
  uint16_t flags;         // kVqaHas* bits
  uint16_t frame_count;   // Total number of frames in the movie
  uint16_t image_width;   // Frame width in pixels
  uint16_t image_height;  // Frame height in pixels
  uint8_t block_width;    // Compression block width in pixels, never 0
  uint8_t block_height;   // Compression block height in pixels, never 0
  uint8_t fps;            // Playback frame rate (frames per second), never 0
  // Frames per codebook group, never 0. Each group's codebook arrives in
  // pieces (partial codebooks) during the previous group, which is why seeking
  // starts one group early.
  uint8_t frames_per_group;
  // Number of single-color blocks. Unused by the player.
  uint16_t single_color_blocks;
  uint16_t codebook_entries;  // Sizes the codebook buffer
  // Where the encoder wanted the frames drawn; 0xFFFF (-1) centers on that
  // axis. Unused: the player leaves placing the frames to its client.
  uint16_t draw_x;
  uint16_t draw_y;
  uint16_t max_frame_bytes;  // Size of the largest frame. Unused.
  // The primary audio track, meaningful when flags has kVqaHasAudio.
  uint16_t sample_rate;     // Sample rate in Hz
  uint8_t channels;         // Number of channels
  uint8_t bits_per_sample;  // Sample bit depth
  // The alternate audio track, meaningful when flags has kVqaHasAltAudio.
  uint16_t alt_sample_rate;
  uint8_t alt_channels;
  uint8_t alt_bits_per_sample;
  std::array<uint16_t, 5> reserved;  // Pads the header to 42 bytes
};
#pragma pack(pop)
static_assert(sizeof(VqaHeader) == 42);

// ZapHeader: the start of a SND1 (Westwood ZAP ADPCM) sound chunk's payload.
// Equal sizes mean the sound is stored uncompressed.
struct ZapHeader {
  uint16_t uncompressed_size;  // Bytes of sound after decompression.
  uint16_t compressed_size;    // Bytes of sound in the chunk.
};
static_assert(sizeof(ZapHeader) == 4);

// VqaHeader::version values.
constexpr uint16_t kVqaVersion1 = 1;
constexpr uint16_t kVqaVersion2 = 2;

// VqaHeader::flags bits: the movie has a primary or an alternate audio track.
constexpr uint16_t kVqaHasAudio = base::Bit<uint16_t>(0);
constexpr uint16_t kVqaHasAltAudio = base::Bit<uint16_t>(1);

// Frame information (FINF) entries.
//
// The FINF chunk holds one 32-bit entry per frame, flags on top of an offset:
//
//   Bits   Name    Description
//   31-28  Flags   Bit 31 marks a key frame, bit 30 a frame that carries a
//                  palette, bit 29 an audio synchronization point.
//   27-0   Offset  Where the frame's chunks start, in 16-bit words from the
//                  start of the file. Chunks are padded to even sizes, so
//                  halving the offset loses nothing and reaches 512 MB.
//
// The player does not read the table: it only marks the end of the chunks in
// front of the frames. The DOS player seeked with it.

// Packs four characters into a chunk ID. A chunk's ID is read raw from the
// file, so on a little-endian host the first character is the lowest byte.
constexpr uint32_t MakeChunkId(char a, char b, char c, char d) {
  return (static_cast<uint32_t>(static_cast<uint8_t>(d)) << 24) |
         (static_cast<uint32_t>(static_cast<uint8_t>(c)) << 16) |
         (static_cast<uint32_t>(static_cast<uint8_t>(b)) << 8) |
         static_cast<uint32_t>(static_cast<uint8_t>(a));
}

// The IFF chunk a VQA file is: a FORM whose payload starts with its type.
constexpr uint32_t kChunkForm = MakeChunkId('F', 'O', 'R', 'M');

// VQA chunk IDs. A "Z" suffix means the payload is LCW compressed.
//
// The format has more chunks than the player decodes, and it skips them: NAME
// (a name string), VPTR and VPRZ (vector pointers in the Run-Skip-Dump
// compression, the latter LCW compressed on top), SNDZ and SNAZ (LCW
// compressed sound), CAP0 (caption text) and EVA0 (EVA text).
constexpr uint32_t kFormWvqa =
    MakeChunkId('W', 'V', 'Q', 'A');  // The VQA form.
constexpr uint32_t kChunkVqhd = MakeChunkId('V', 'Q', 'H', 'D');  // VqaHeader.
constexpr uint32_t kChunkFinf =
    MakeChunkId('F', 'I', 'N', 'F');  // Frame table.
constexpr uint32_t kChunkVqfr =
    MakeChunkId('V', 'Q', 'F', 'R');  // Frame container.
constexpr uint32_t kChunkVqfk = MakeChunkId('V', 'Q', 'F', 'K');  // Key frame.
constexpr uint32_t kChunkCbf0 =
    MakeChunkId('C', 'B', 'F', '0');  // Full codebook.
constexpr uint32_t kChunkCbfz = MakeChunkId('C', 'B', 'F', 'Z');
constexpr uint32_t kChunkCbp0 =
    MakeChunkId('C', 'B', 'P', '0');  // Partial codebook.
constexpr uint32_t kChunkCbpz = MakeChunkId('C', 'B', 'P', 'Z');
constexpr uint32_t kChunkVpt0 =
    MakeChunkId('V', 'P', 'T', '0');  // Vector pointers.
constexpr uint32_t kChunkVptz = MakeChunkId('V', 'P', 'T', 'Z');
constexpr uint32_t kChunkVptk =
    MakeChunkId('V', 'P', 'T', 'K');  // Delta, key frame.
constexpr uint32_t kChunkVptd = MakeChunkId('V', 'P', 'T', 'D');  // Delta.
constexpr uint32_t kChunkCpl0 =
    MakeChunkId('C', 'P', 'L', '0');  // Color palette.
constexpr uint32_t kChunkCplz = MakeChunkId('C', 'P', 'L', 'Z');

// Sound for the primary track (SND*) and the alternate track (SNA*); the
// loader keeps one track and skips the other's chunks.
constexpr uint32_t kChunkSnd0 =
    MakeChunkId('S', 'N', 'D', '0');  // Uncompressed.
constexpr uint32_t kChunkSnd1 =
    MakeChunkId('S', 'N', 'D', '1');  // Zap compressed.
constexpr uint32_t kChunkSnd2 =
    MakeChunkId('S', 'N', 'D', '2');  // ADPCM compressed.
constexpr uint32_t kChunkSna0 =
    MakeChunkId('S', 'N', 'A', '0');  // Uncompressed.
constexpr uint32_t kChunkSna1 =
    MakeChunkId('S', 'N', 'A', '1');  // Zap compressed.
constexpr uint32_t kChunkSna2 =
    MakeChunkId('S', 'N', 'A', '2');  // ADPCM compressed.

#endif  // CNC_RED_ALERT_WINVQ_VQA32_VQA_FORMAT_H_
