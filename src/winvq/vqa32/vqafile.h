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

#include "base/types.h"
#include "winvq/vqm32/iff.h"

// VqaHeader: the payload of the VQHD chunk, read straight off the disk.
//
// The loader rejects a VQHD whose size is not sizeof(VqaHeader), so the packing
// and field order are the file format. The fields are little-endian and read
// raw, which assumes a little-endian host. A version 1 movie's audio fields are
// ignored: its sound is always 22050 Hz 8-bit mono (see AllocBuffers in
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
  // axis. Unused by the player, which takes the position from VQAConfig.
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

// VqaHeader::version values.
constexpr uint16_t kVqaVersion1 = 1;
constexpr uint16_t kVqaVersion2 = 2;

// VqaHeader::flags bits.
constexpr uint32_t kVqaHasAudio = 1U << 0;     // A primary audio track.
constexpr uint32_t kVqaHasAltAudio = 1U << 1;  // An alternate audio track.

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
// Seeking uses both: it replays the nearest palette frame at or before the
// target, then starts reading at the codebook group before it. The player
// reads no other flag.
constexpr uint32_t kFrameInfoHasPalette = uint32_t{1} << 30;
constexpr uint32_t kFrameInfoOffsetMask = 0x0FFFFFFFU;

// Returns the byte offset in the file of the frame a FINF entry describes.
constexpr base::ssize FrameByteOffset(uint32_t frame_info) {
  return base::ssize{frame_info & kFrameInfoOffsetMask} * 2;
}

// VQA chunk IDs. MakeId packs the four characters in file order, so these
// compare equal to an ID read raw from the disk. A "Z" suffix means the payload
// is LCW compressed.
//
// The format has more chunks than the player decodes, and it skips them: NAME
// (a name string), VPTR and VPRZ (vector pointers in the Run-Skip-Dump
// compression, the latter LCW compressed on top), SNDZ and SNAZ (LCW
// compressed sound), CAP0 (caption text) and EVA0 (EVA text).
constexpr int32_t kFormWvqa = MakeId('W', 'V', 'Q', 'A');   // The VQA form.
constexpr int32_t kChunkVqhd = MakeId('V', 'Q', 'H', 'D');  // VqaHeader.
constexpr int32_t kChunkFinf = MakeId('F', 'I', 'N', 'F');  // Frame table.
constexpr int32_t kChunkVqfr = MakeId('V', 'Q', 'F', 'R');  // Frame container.
constexpr int32_t kChunkVqfk = MakeId('V', 'Q', 'F', 'K');  // Key frame.
constexpr int32_t kChunkCbf0 = MakeId('C', 'B', 'F', '0');  // Full codebook.
constexpr int32_t kChunkCbfz = MakeId('C', 'B', 'F', 'Z');
constexpr int32_t kChunkCbp0 = MakeId('C', 'B', 'P', '0');  // Partial codebook.
constexpr int32_t kChunkCbpz = MakeId('C', 'B', 'P', 'Z');
constexpr int32_t kChunkVpt0 = MakeId('V', 'P', 'T', '0');  // Vector pointers.
constexpr int32_t kChunkVptz = MakeId('V', 'P', 'T', 'Z');
constexpr int32_t kChunkVptk = MakeId('V', 'P', 'T', 'K');  // Delta, key frame.
constexpr int32_t kChunkVptd = MakeId('V', 'P', 'T', 'D');  // Delta.
constexpr int32_t kChunkCpl0 = MakeId('C', 'P', 'L', '0');  // Color palette.
constexpr int32_t kChunkCplz = MakeId('C', 'P', 'L', 'Z');

// Sound for the primary track (SND*) and the alternate track (SNA*); the
// loader keeps one track and skips the other's chunks.
constexpr int32_t kChunkSnd0 = MakeId('S', 'N', 'D', '0');  // Uncompressed.
constexpr int32_t kChunkSnd1 = MakeId('S', 'N', 'D', '1');  // Zap compressed.
constexpr int32_t kChunkSnd2 = MakeId('S', 'N', 'D', '2');  // ADPCM compressed.
constexpr int32_t kChunkSna0 = MakeId('S', 'N', 'A', '0');  // Uncompressed.
constexpr int32_t kChunkSna1 = MakeId('S', 'N', 'A', '1');  // Zap compressed.
constexpr int32_t kChunkSna2 = MakeId('S', 'N', 'A', '2');  // ADPCM compressed.

#endif  // CNC_RED_ALERT_WINVQ_VQA32_VQAFILE_H_
