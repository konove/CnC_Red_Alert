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

// Decodes the keyframe shapes (.SHP) of both games: the frames of building,
// unit and interface images, each stored as an LCW-compressed key frame or as
// XOR deltas that rebuild it from one.
//
// Originally KEYFRAME.CPP (Counterstrike's 2KEYFRAM.CPP) by Joe L. Bostic,
// started June 25, 1995.

#include "engine/gfx/keyframe.h"

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
#include "engine/gfx/palette.h"

namespace engine::gfx {
namespace {

// The file header is seven little-endian 16-bit words: the frame count, x, y,
// width, height, largest_frame_size and flags. Width and height give every
// frame's size in pixels; nothing reads x, y or largest_frame_size. An offset
// table of kEntrySize bytes per frame follows it (see ReadFrameEntry()).
constexpr base::ssize kHeaderSize = 14;
constexpr base::ssize kEntrySize = 8;

// A header flag: a palette (Palette::kFileSize bytes) sits in the file that
// the frame offsets do not count, so every offset read from the table has to
// skip it.
constexpr uint16_t kHasPalette = 0x01;

// The bits of an offset-table word that hold a file offset; the top byte holds
// the frame's kKf* flags.
constexpr uint32_t kOffsetMask = 0x00ffffffU;

// The frame flags in an offset word's top byte that decoding tells apart. A
// key frame is a whole image, LCW-compressed; a delta is an XOR delta that
// continues a chain. A frame with neither is a key delta (0x40), the delta that
// starts a chain against a key frame. The original also named 0x08 (a frame
// number) and 0x10 (LCW compression), which decoding does not read.
constexpr uint8_t kKfDelta = 0x20;
constexpr uint8_t kKfKeyFrame = 0x80;

// What the decoder needs from the header.
struct KeyframeHeader {
  int frame_count = 0;
  int width = 0;
  int height = 0;
  // How far every offset in the table has to be moved to skip the palette.
  base::ssize palette_bytes = 0;
};

// Returns the header of `shape_file`, or all zeroes (so no frames) if
// `shape_file` is too short to hold one.
KeyframeHeader ReadHeader(const std::span<const std::byte> shape_file) {
  if (std::ssize(shape_file) < kHeaderSize) {
    return {};
  }
  const auto flags = base::ReadLittleEndian<uint16_t>(shape_file, 12);
  return {.frame_count = base::ReadLittleEndian<uint16_t>(shape_file, 0),
          .width = base::ReadLittleEndian<uint16_t>(shape_file, 6),
          .height = base::ReadLittleEndian<uint16_t>(shape_file, 8),
          .palette_bytes = (flags & kHasPalette) != 0 ? Palette::kFileSize : 0};
}

// One frame's entry in the offset table. `offset_word` packs the frame's kKf*
// flags into the top byte and the offset of its data into kOffsetMask.
// `reference_word` depends on the kind of frame: a key delta's holds the offset
// of the key frame it is a delta against (under kOffsetMask again), and a
// kKfDelta frame's the index of the key delta that starts its chain.
struct FrameEntry {
  uint32_t offset_word = 0;
  uint32_t reference_word = 0;
};

// Returns `frame`'s entry from the offset table, or nullopt if the entry lies
// past the end of `shape_file`.
std::optional<FrameEntry> ReadFrameEntry(
    const std::span<const std::byte> shape_file, const int frame) {
  const auto entry = base::SliceOrEmpty(
      shape_file, kHeaderSize + (base::ssize{frame} * kEntrySize), kEntrySize);
  if (entry.empty()) {
    return std::nullopt;
  }
  return FrameEntry{
      .offset_word = base::ReadLittleEndian<uint32_t>(entry, 0),
      .reference_word = base::ReadLittleEndian<uint32_t>(entry, 4)};
}

// Returns the data from the offset in `word` to the end of `shape_file`, or an
// empty span if that offset is not inside the file. The palette is skipped for
// key frames and deltas alike, as the original did.
std::span<const std::byte> DataAt(const std::span<const std::byte> shape_file,
                                  const KeyframeHeader& header,
                                  const uint32_t word) {
  return base::SliceOrEmpty(
      shape_file, base::ssize{word & kOffsetMask} + header.palette_bytes);
}

}  // namespace

std::span<uint8_t> BuildFrame(const std::span<const std::byte> shape_file,
                              const int frame,
                              const std::span<uint8_t> destination) {
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

  // A key frame stands alone: its data is the whole image, LCW-compressed.
  if ((frame_flags & kKfKeyFrame) != 0) {
    const auto key_frame = DataAt(shape_file, header, entry->offset_word);
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
  // delta names the key delta that starts it (in the low 16 bits of its
  // reference word, as frame numbers are 16 bits), whose entry then gives the
  // key frame.
  const int chain_start =
      (frame_flags & kKfDelta) != 0
          ? static_cast<int>(entry->reference_word & 0xffffU)
          : frame;
  if (chain_start > frame) {
    return {};
  }
  const auto chain_entry = ReadFrameEntry(shape_file, chain_start);
  if (!chain_entry) {
    return {};
  }
  const auto key_frame =
      DataAt(shape_file, header, chain_entry->reference_word);
  if (key_frame.empty()) {
    return {};
  }
  LCW_Uncompress(key_frame, std::as_writable_bytes(output));
  for (int delta_frame = chain_start; delta_frame <= frame; ++delta_frame) {
    const auto delta_entry = ReadFrameEntry(shape_file, delta_frame);
    if (!delta_entry) {
      return {};
    }
    const auto delta = DataAt(shape_file, header, delta_entry->offset_word);
    if (delta.empty()) {
      return {};
    }
    ApplyXorDelta(output, delta);
  }
  return output.subspan(0);
}

int KeyframeFrameCount(const std::span<const std::byte> shape_file) {
  return ReadHeader(shape_file).frame_count;
}
int KeyframeWidth(const std::span<const std::byte> shape_file) {
  return ReadHeader(shape_file).width;
}
int KeyframeHeight(const std::span<const std::byte> shape_file) {
  return ReadHeader(shape_file).height;
}

}  // namespace engine::gfx
