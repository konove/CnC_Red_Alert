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
#include <optional>
#include <span>

#include "base/buffer.h"
#include "base/numeric.h"
#include "base/types.h"
#include "base/unaligned.h"
#include "engine/codec/lcw_uncompress.h"
#include "engine/codec/xor_delta.h"
#include "ra/defines.h"
#include "ra/keyframe.h"

namespace {

// The file header is seven little-endian 16-bit words: the frame count, x, y,
// width, height, largest_frame_size and flags. Width and height give every
// frame's size in pixels; nothing reads x, y or largest_frame_size. An offset
// table of kEntrySize bytes per frame follows it (see ReadFrameEntry()).
constexpr base::ssize kHeaderSize = 14;
constexpr base::ssize kEntrySize = 8;

// A header flag: a palette of kPaletteSize bytes sits in the file that the
// frame offsets do not count, so every offset read from the table has to skip
// it.
constexpr uint16_t kHasPalette = 0x01;
constexpr base::ssize kPaletteSize = 768;

// The bits of an offset-table word that hold a file offset; the top byte holds
// the frame's kKf* flags.
constexpr uint32_t kOffsetMask = 0x00ffffffU;

// The header fields the decoder uses.
struct KeyframeHeader {
  int frame_count = 0;
  int width = 0;
  int height = 0;
  uint16_t flags = 0;
};

// Returns the header of `shape_file`, or all zeroes (so no frames) if
// `shape_file` is too short to hold one.
KeyframeHeader ReadHeader(std::span<const std::byte> shape_file) {
  if (std::ssize(shape_file) < kHeaderSize) {
    return {};
  }
  return {.frame_count = base::ReadLittleEndian<uint16_t>(shape_file, 0),
          .width = base::ReadLittleEndian<uint16_t>(shape_file, 6),
          .height = base::ReadLittleEndian<uint16_t>(shape_file, 8),
          .flags = base::ReadLittleEndian<uint16_t>(shape_file, 12)};
}

// One frame's entry in the offset table. `offset_word` packs the frame's kKf*
// flags into the top byte and the offset of its data into kOffsetMask.
// `reference_word` depends on the kind of frame: a kKfKeyDelta frame's holds
// the offset of the key frame it is a delta against (under kOffsetMask again),
// and a kKfDelta frame's the index of the kKfKeyDelta frame that starts its
// chain.
struct FrameEntry {
  uint32_t offset_word = 0;
  uint32_t reference_word = 0;
};

// Returns `frame`'s entry from the offset table, or nullopt if the entry lies
// past the end of `shape_file`.
std::optional<FrameEntry> ReadFrameEntry(std::span<const std::byte> shape_file,
                                         int frame) {
  const auto entry = base::SliceOrEmpty(
      shape_file, kHeaderSize + (base::ssize{frame} * kEntrySize), kEntrySize);
  if (entry.empty()) {
    return std::nullopt;
  }
  return FrameEntry{
      .offset_word = base::ReadLittleEndian<uint32_t>(entry, 0),
      .reference_word = base::ReadLittleEndian<uint32_t>(entry, 4)};
}

// Returns the data from the offset in `word` (plus `palette_bytes`) to the end
// of `shape_file`, or an empty span if that offset is not inside the file.
std::span<const std::byte> DataAt(std::span<const std::byte> shape_file,
                                  uint32_t word, base::ssize palette_bytes) {
  return base::SliceOrEmpty(shape_file,
                            base::ssize{word & kOffsetMask} + palette_bytes);
}

}  // namespace

std::span<uint8_t> BuildFrame(std::span<const std::byte> shape_file, int frame,
                              std::span<uint8_t> destination) {
  const KeyframeHeader header = ReadHeader(shape_file);
  const base::ssize pixel_count = base::ssize{header.width} * header.height;
  if (frame < 0 || frame >= header.frame_count || pixel_count == 0 ||
      pixel_count > std::ssize(destination)) {
    return {};
  }
  const auto entry = ReadFrameEntry(shape_file, frame);
  if (!entry) {
    return {};
  }
  const auto output = destination.first(base::ToSize(pixel_count));
  const auto frame_flags = static_cast<uint8_t>(entry->offset_word >> 24);
  const base::ssize palette_bytes =
      (header.flags & kHasPalette) != 0 ? kPaletteSize : 0;

  // A key frame stands alone: its data is the whole image, LCW-compressed.
  if ((frame_flags & kKfKeyFrame) != 0) {
    const auto key_frame =
        DataAt(shape_file, entry->offset_word, palette_bytes);
    if (key_frame.empty()) {
      return {};
    }
    LCW_Uncompress(key_frame, std::as_writable_bytes(output));
    // A copy rather than `output` itself: with other paths returning {}, the
    // named span cannot be elided, which -Wnrvo reports.
    return output.subspan(0);
  }

  // Anything else is rebuilt from the key frame by XORing on the deltas from
  // the start of its chain to `frame`. A key delta starts the chain itself; a
  // delta names the key delta that starts it, whose entry then gives the key
  // frame.
  int chain_start = frame;
  auto chain_entry = entry;
  if ((frame_flags & kKfDelta) != 0) {
    // Frame numbers are 16 bits; the word's high half is not part of one.
    chain_start = static_cast<int>(entry->reference_word & 0xffffU);
    if (chain_start > frame) {
      return {};
    }
    chain_entry = ReadFrameEntry(shape_file, chain_start);
    if (!chain_entry) {
      return {};
    }
  }
  const auto key_frame =
      DataAt(shape_file, chain_entry->reference_word, palette_bytes);
  if (key_frame.empty()) {
    return {};
  }
  LCW_Uncompress(key_frame, std::as_writable_bytes(output));
  for (int delta_frame = chain_start; delta_frame <= frame; ++delta_frame) {
    const auto delta_entry = ReadFrameEntry(shape_file, delta_frame);
    if (!delta_entry) {
      return {};
    }
    // The palette is skipped here too, as the original did for delta data.
    const auto delta =
        DataAt(shape_file, delta_entry->offset_word, palette_bytes);
    if (delta.empty()) {
      return {};
    }
    ApplyXorDelta(output, delta);
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
