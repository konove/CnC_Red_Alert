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

// Decodes Red Alert's keyframe shapes (.SHP): the frames of building, unit
// and interface images, each stored as an LCW-compressed key frame or as XOR
// deltas that rebuild it from one.
//
// Originally KEYFRAME.CPP (Counterstrike's 2KEYFRAM.CPP) by Joe L. Bostic,
// started June 25, 1995.

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "base/buffer.h"
#include "base/numeric.h"
#include "base/types.h"
#include "base/unaligned.h"
#include "engine/codec/lcw_uncompress.h"
#include "engine/codec/xor_delta.h"
#include "ra/defines.h"
#include "ra/keyframe.h"

// The file header: seven little-endian 16-bit words, of which width and height
// give every frame's size in pixels. Nothing reads x, y or largest_frame_size.
// An offset table of one 8-byte entry per frame follows it (see
// ReadFrameEntry()).
struct KeyframeHeader {
  uint16_t frame_count;
  uint16_t x;
  uint16_t y;
  uint16_t width;
  uint16_t height;
  uint16_t largest_frame_size;
  // Bit 0: a 768-byte palette sits in the file that the frame offsets do not
  // count, so every offset read from the table has to skip it.
  uint16_t flags;
};

namespace {
// Returns the header of `shape_file`, or all zeroes (so no frames) if
// `shape_file` is too short to hold one.
KeyframeHeader ReadHeader(std::span<const std::byte> shape_file) {
  KeyframeHeader header{};
  if (shape_file.size() >= sizeof(header)) {
    base::CopyBytes(base::ObjectBytes(header), shape_file, sizeof(header));
  }
  return header;
}

// Reads `frame`'s entry from the offset table. Its first word packs the frame's
// kKf* flags into the top byte and the offset of its data into the low 24 bits.
// The second word depends on the kind of frame: a kKfKeyDelta frame's holds the
// offset of the key frame it is a delta against (in the low 24 bits again), and
// a kKfDelta frame's the index of the kKfKeyDelta frame that starts its chain.
// Returns false, leaving both outputs alone, if the entry lies past the end of
// `shape_file`.
bool ReadFrameEntry(std::span<const std::byte> shape_file, int frame,
                    uint32_t& offset_word, uint32_t& reference_word) {
  const auto entry = base::SliceOrEmpty(
      shape_file,
      base::ssize{sizeof(KeyframeHeader)} + (base::ssize{frame} * 8), 8);
  if (entry.empty()) {
    return false;
  }
  offset_word = base::ReadLittleEndian<uint32_t>(entry, 0);
  reference_word = base::ReadLittleEndian<uint32_t>(entry, 4);
  return true;
}
}  // namespace

std::span<uint8_t> BuildFrame(std::span<const std::byte> shape_file, int frame,
                              std::span<uint8_t> destination) {
  const auto header = ReadHeader(shape_file);
  const base::ssize pixel_count = base::ssize{header.width} * header.height;
  if (frame < 0 || frame >= int{header.frame_count} || pixel_count == 0 ||
      pixel_count > std::ssize(destination)) {
    return {};
  }
  const auto output = destination.first(base::ToSize(pixel_count));
  uint32_t offset_word = 0;
  uint32_t reference_word = 0;
  if (!ReadFrameEntry(shape_file, frame, offset_word, reference_word)) {
    return {};
  }
  const auto frame_flags = static_cast<uint8_t>(offset_word >> 24);
  const base::ssize palette_bytes = (header.flags & 1U) != 0 ? 768 : 0;

  // A key frame stands alone: its data is the whole image, LCW-compressed.
  if ((frame_flags & kKfKeyFrame) != 0) {
    const base::ssize data_offset =
        base::ssize{offset_word & 0x00ffffffU} + palette_bytes;
    if (data_offset >= std::ssize(shape_file)) {
      return {};
    }
    LCW_Uncompress(shape_file.subspan(base::ToSize(data_offset)),
                   std::as_writable_bytes(output));
    return output.subspan(0);
  }

  // Anything else is rebuilt from the key frame by XORing on the deltas from
  // the start of its chain to `frame`. A key delta starts the chain itself; a
  // delta names the key delta that starts it, whose entry then gives the key
  // frame.
  int chain_start = frame;
  if ((frame_flags & kKfDelta) != 0) {
    // Frame numbers are 16 bits; the word's high half is not part of one.
    chain_start = static_cast<int>(reference_word & 0xffffU);
    if (chain_start > frame ||
        !ReadFrameEntry(shape_file, chain_start, offset_word, reference_word)) {
      return {};
    }
  }
  const base::ssize key_frame_offset =
      base::ssize{reference_word & 0x00ffffffU} + palette_bytes;
  if (key_frame_offset >= std::ssize(shape_file)) {
    return {};
  }
  LCW_Uncompress(shape_file.subspan(base::ToSize(key_frame_offset)),
                 std::as_writable_bytes(output));
  for (int delta_frame = chain_start; delta_frame <= frame; ++delta_frame) {
    if (!ReadFrameEntry(shape_file, delta_frame, offset_word, reference_word)) {
      return {};
    }
    // The palette is skipped here too, as the original did for delta data.
    const base::ssize delta_offset =
        base::ssize{offset_word & 0x00ffffffU} + palette_bytes;
    if (delta_offset >= std::ssize(shape_file)) {
      return {};
    }
    ApplyXorDelta(output, shape_file.subspan(base::ToSize(delta_offset)));
  }
  return output.subspan(0);
}

std::span<uint8_t> BuildFrame(std::span<const std::byte> shape_file,
                              int frame) {
  static std::array<uint8_t, kShapeBufferSize> staging{};
  return BuildFrame(shape_file, frame, staging);
}

int KeyframeFrameCount(std::span<const std::byte> shape_file) {
  return ReadHeader(shape_file).frame_count;
}
int KeyframeWidth(std::span<const std::byte> shape_file) {
  return ReadHeader(shape_file).width;
}
int KeyframeHeight(std::span<const std::byte> shape_file) {
  return ReadHeader(shape_file).height;
}
