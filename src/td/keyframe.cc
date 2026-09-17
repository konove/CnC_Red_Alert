/*
**	Command & Conquer(tm)
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

/* $Header:   F:\projects\c&c\vcs\code\keyframe.cpv   2.14   16 Oct 1995
 * 16:48:54   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : KEYFRAME.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 06/25/95 *
 *                                                                                             *
 *                  Last Update : June 25, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Get_Build_Frame_Count -- Fetches the number of frames in data
 *block.                      * Get_Build_Frame_Width -- Fetches the width of
 *the shape image.                            * Get_Build_Frame_Height --
 *Fetches the height of the shape image.                          *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/keyframe.h"

#include <cstddef>
#include <cstdint>
#include <span>

#include "base/buffer.h"
#include "port/unaligned.h"
#include "sdllib/iff.h"
#include "sdllib/wsa.h"
#include "td/defines.h"
#include "tech/2keyfbuf.h"

struct KeyFrameHeaderType {
  uint16_t frames;
  uint16_t x;
  uint16_t y;
  uint16_t width;
  uint16_t height;
  uint16_t largest_frame_size;
  int16_t flags;
};

// The uncompressed-shape cache was never finished: nothing sets
// UseBigShapeBuffer, so Build_Frame always decodes into the caller's buffer.
// tech/2keyfbuf.cc still reads these three to choose its draw path, so the
// definitions stay.
char* BigShapeBufferStart = nullptr;
std::span<char> BigShapeBufferBytes;
char* TheaterShapeBufferStart = nullptr;
std::span<char> TheaterShapeBufferBytes;
bool UseBigShapeBuffer = false;
// Set by the type classes around Build_Frame for theater-specific shapes; only
// the uncompressed-shape cache ever read it.
bool IsTheaterShape = false;

// Only the uncompressed-shape cache recorded a frame length, so this stays 0.
static int Length;

int Get_Last_Frame_Length() { return Length; }

namespace {
KeyFrameHeaderType Header(std::span<const std::byte> data) {
  KeyFrameHeaderType header{};
  if (data.size() >= sizeof(header)) {
    base::CopyBytes(base::ObjectBytes(header), data, sizeof(header));
  }
  return header;
}

bool FrameEntry(std::span<const std::byte> data, uint16_t frame,
                uint32_t& offset, uint32_t& reference) {
  const auto entry = sizeof(KeyFrameHeaderType) + (size_t{frame} * 8);
  if (entry > data.size() || data.size() - entry < 8) {
    return false;
  }
  offset = port::ReadUnaligned<uint32_t>(data.subspan(entry));
  reference = port::ReadUnaligned<uint32_t>(data.subspan(entry + 4));
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

  if ((flags & kKfKeyFrame) != 0) {
    const auto start =
        static_cast<size_t>(offset & 0x00ffffffU) + palette_bytes;
    if (start >= data.size()) {
      return {};
    }
    LCW_Uncompress(data.subspan(start), std::as_writable_bytes(output));
    return output.subspan(0);
  }

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
    // Palette bytes were also added to the delta base in the original format.
    const auto delta_offset =
        static_cast<size_t>(offset & 0x00ffffffU) + palette_bytes;
    if (delta_offset >= data.size()) {
      return {};
    }
    Apply_XOR_Delta(output, data.subspan(delta_offset));
  }
  return output.subspan(0);
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

bool Get_Build_Frame_Palette(std::span<const std::byte> data,
                             std::span<uint8_t> palette) {
  const auto header = Header(data);
  const auto start =
      (size_t{header.frames} * 8) + 16 + sizeof(KeyFrameHeaderType);
  if ((static_cast<uint16_t>(header.flags) & 1U) == 0 || palette.size() < 768 ||
      start > data.size() || data.size() - start < 768) {
    return false;
  }
  base::CopyBytes(std::as_writable_bytes(palette), data.subspan(start), 768);
  return true;
}
