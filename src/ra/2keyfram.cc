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

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "base/buffer.h"
#include "base/unaligned.h"
#include "engine/codec/lcw_uncompress.h"
#include "engine/codec/xor_delta.h"
#include "ra/defines.h"
#include "ra/keyframe.h"

// The file header: seven little-endian 16-bit words, of which width and height
// give every frame's size in pixels. Nothing reads x, y or largest_frame_size.
// An offset table of one 8-byte entry per frame follows it (see FrameEntry()).
struct KeyFrameHeaderType {
  uint16_t frames;
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
// Returns the header of `data`, or all zeroes (so no frames) if `data` is too
// short to hold one.
KeyFrameHeaderType Header(std::span<const std::byte> data) {
  KeyFrameHeaderType header{};
  if (data.size() >= sizeof(header)) {
    base::CopyBytes(base::ObjectBytes(header), data, sizeof(header));
  }
  return header;
}

// Reads `frame`'s entry from the offset table. Its first word packs the frame's
// kKf* flags into the top byte and the offset of its data into the low 24 bits.
// The second word depends on the kind of frame: a kKfKeyDelta frame's holds the
// offset of the key frame it is a delta against (in the low 24 bits again), and
// a kKfDelta frame's the index of the kKfKeyDelta frame that starts its chain.
// Returns false, leaving both outputs alone, if the entry lies past the end of
// `data`.
bool FrameEntry(std::span<const std::byte> data, uint16_t frame,
                uint32_t& offset, uint32_t& reference) {
  const auto entry = base::SliceOrEmpty(
      data, sizeof(KeyFrameHeaderType) + (size_t{frame} * 8), 8);
  if (entry.empty()) {
    return false;
  }
  offset = base::ReadLittleEndian<uint32_t>(entry, 0);
  reference = base::ReadLittleEndian<uint32_t>(entry, 4);
  return true;
}
}  // namespace

std::span<uint8_t> Build_Frame(std::span<const std::byte> data, uint16_t frame,
                               std::span<uint8_t> destination) {
  const auto header = Header(data);
  const auto pixels = size_t{header.width} * header.height;
  if (frame >= header.frames || pixels == 0 || pixels > destination.size()) {
    return {};
  }
  const auto output = destination.first(pixels);
  uint32_t offset = 0;
  uint32_t reference = 0;
  if (!FrameEntry(data, frame, offset, reference)) {
    return {};
  }
  const auto flags = static_cast<uint8_t>(offset >> 24);
  const size_t palette_bytes =
      (static_cast<uint16_t>(header.flags) & 1U) != 0 ? 768 : 0;

  // A key frame stands alone: its data is the whole image, LCW-compressed.
  if ((flags & kKfKeyFrame) != 0) {
    const auto start =
        static_cast<size_t>(offset & 0x00ffffffU) + palette_bytes;
    if (start >= data.size()) {
      return {};
    }
    LCW_Uncompress(data.subspan(start), std::as_writable_bytes(output));
    return output.subspan(0);
  }

  // Anything else is rebuilt from the key frame by XORing on the deltas from
  // the start of its chain to `frame`. A key delta starts the chain itself; a
  // delta names the key delta that starts it, whose entry then gives the key
  // frame.
  uint16_t first_delta = frame;
  if ((flags & kKfDelta) != 0) {
    first_delta = static_cast<uint16_t>(reference);
    if (first_delta > frame ||
        !FrameEntry(data, first_delta, offset, reference)) {
      return {};
    }
  }
  const auto key_offset =
      static_cast<size_t>(reference & 0x00ffffffU) + palette_bytes;
  if (key_offset >= data.size()) {
    return {};
  }
  LCW_Uncompress(data.subspan(key_offset), std::as_writable_bytes(output));
  for (uint32_t current = first_delta; current <= frame; ++current) {
    if (!FrameEntry(data, static_cast<uint16_t>(current), offset, reference)) {
      return {};
    }
    // The palette is skipped here too, as the original did for delta data.
    const auto delta_offset =
        static_cast<size_t>(offset & 0x00ffffffU) + palette_bytes;
    if (delta_offset >= data.size()) {
      return {};
    }
    ApplyXorDelta(output, data.subspan(delta_offset));
  }
  return output.subspan(0);
}

std::span<uint8_t> Build_Frame(std::span<const std::byte> data,
                               uint16_t frame) {
  static std::vector<uint8_t> staging(kShapeBufferSize);
  return Build_Frame(data, frame, staging);
}

uint16_t Get_Build_Frame_Count(std::span<const std::byte> data) {
  return Header(data).frames;
}
uint16_t Get_Build_Frame_X(std::span<const std::byte> data) {
  return Header(data).x;
}
uint16_t Get_Build_Frame_Y(std::span<const std::byte> data) {
  return Header(data).y;
}
uint16_t Get_Build_Frame_Width(std::span<const std::byte> data) {
  return Header(data).width;
}
uint16_t Get_Build_Frame_Height(std::span<const std::byte> data) {
  return Header(data).height;
}
