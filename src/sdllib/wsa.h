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

// WSA animation playback: opening a .WSA file, stepping it to an arbitrary
// frame, and the XOR delta decoder the frames are built from. Part of the
// Westwood WSA 32-bit library (WSA.H, Scott K. Bowen, May 1994).
//
// A WSA file stores every frame as an LCW-compressed XOR delta against the
// frame before it; frame 0 is a delta against black, or against a picture the
// caller has already drawn. XOR is its own inverse, so the same delta steps
// forward or backward, and an optional extra delta after the last frame loops
// back to frame 0.
//
// Example:
//   void* anim = OpenAnimation("TITLE.WSA", WSA_OPEN_FROM_MEM, palette);
//   for (int i = 0; i < AnimationFrameCount(anim); ++i) {
//     DrawAnimationFrame(anim, view, i);
//   }
//   CloseAnimation(anim);

#ifndef CNC_RED_ALERT_SDLLIB_WSA_H_
#define CNC_RED_ALERT_SDLLIB_WSA_H_

#include <cstddef>

#include <cstdint>
#include <span>

#include "base/attributes.h"
#include "base/flags.h"
#include "sdllib/gbuffer.h"

// Flags for OpenAnimation(). The zero-valued names are the defaults and exist
// only to make call sites readable; testing for them with `&` is always false.
enum class CNC_FLAG_ENUM WsaOpenFlags {
  // Try to load the entire animation into memory.
  WSA_OPEN_FROM_MEM = 0x0000,
  // First animate to an internal buffer, then copy to the page or viewport.
  WSA_OPEN_INDIRECT = 0x0000,
  // Keep the file open and read each frame's delta from disk as needed.
  WSA_OPEN_FROM_DISK = 0x0001,
  // Animate directly to the page or viewport.
  WSA_OPEN_DIRECT = 0x0002,

  // These two were added for the 32-bit library to say where the deltas land
  // rather than how. Indirect is best if the destination is the visible page
  // and the animation is not played linearly, or if the destination is modified
  // between frames: a direct animation XORs onto whatever is on the destination
  // and so needs the previous frame still intact there.
  WSA_OPEN_TO_PAGE = WSA_OPEN_DIRECT,
  WSA_OPEN_TO_BUFFER = WSA_OPEN_INDIRECT,
};
using enum WsaOpenFlags;
template <>
inline constexpr bool base::kIsFlagEnum<WsaOpenFlags> = true;

// Opens the animation in `file_name` and returns a handle to pass to
// DrawAnimationFrame(), or nullptr if the file is missing, corrupt or too large
// for memory. If the file has a palette and `palette` holds at least 768 bytes
// (256 RGB triples), it is read into `palette`. Release the handle with
// CloseAnimation().
void* OpenAnimation(const char* file_name, WsaOpenFlags open_flags,
                    std::span<uint8_t> palette = {});

// Closes the animation's file, if it is being played from disk, and frees
// `handle`. Does nothing if `handle` is nullptr.
void CloseAnimation(void* handle);

// Draws frame `frame_number` of the animation into `view` at the offset stored
// in the animation file, applying every delta between the last frame drawn and
// the requested one. Frames may be requested in any order, but the cost grows
// with the distance from the previous frame. Returns false if `handle` is
// nullptr, the frame number is out of range, the view cannot be locked, or the
// frame does not fit the view. Also returns false if a delta on the way cannot
// be loaded; `view` then shows the last frame that could be reached, and a
// later call carries on from there.
bool DrawAnimationFrame(void* handle, GraphicViewPortClass& view,
                        int frame_number);

// Returns the number of frames in the animation, or 0 if `handle` is nullptr.
int AnimationFrameCount(void* handle);

// XOR delta decoders, formerly the assembly in LP_ASM.ASM and now in wsa.cc.
// All of them stop quietly at the first command that is malformed or would
// step outside `target` or `delta`.

// Applies the uncompressed XOR delta in `delta` to `target`, treating `target`
// as one contiguous run of pixels.
void ApplyXorDelta(std::span<uint8_t> target, std::span<const std::byte> delta);
void ApplyXorDelta(std::span<uint8_t> target, std::span<const uint8_t> delta);

// Applies the uncompressed XOR delta in `delta` to a `width`-pixel-wide image
// whose rows start `stride` bytes apart in `target`; pixels past `width` on
// each row are left alone. The delta is XORed onto `target`, or overwrites it
// if `copy` is set. Does nothing if `width` or `stride` is not positive or
// `stride` is less than `width`.
void ApplyXorDeltaToView(std::span<uint8_t> target,
                         std::span<const uint8_t> delta, int width, int stride,
                         bool copy);

#endif  // CNC_RED_ALERT_SDLLIB_WSA_H_
