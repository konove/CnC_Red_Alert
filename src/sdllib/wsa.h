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
//   WsaAnimation anim("TITLE.WSA", palette);
//   for (int i = 0; i < anim.frame_count(); ++i) {
//     anim.DrawFrame(view, i);
//   }

#ifndef CNC_RED_ALERT_SDLLIB_WSA_H_
#define CNC_RED_ALERT_SDLLIB_WSA_H_

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "sdllib/gbuffer.h"

// A .WSA animation held in memory, in the manner of std::ifstream: the
// constructor loads the file, is_open() says whether that worked, and a closed
// animation is safe to use, drawing nothing and reporting no frames. The games
// rely on that to play on without an animation whose file is missing.
//
// Frames are XORed straight onto the view, so the view has to keep the previous
// frame intact between calls to DrawFrame().
class WsaAnimation {
 public:
  // A closed animation.
  WsaAnimation() = default;

  // Loads the animation in `file_name`; it stays closed if the file is missing
  // or corrupt. If the file has a palette and `palette` holds at least 768
  // bytes (256 RGB triples), it is read into `palette`.
  explicit WsaAnimation(std::string_view file_name,
                        std::span<uint8_t> palette = {});

  [[nodiscard]] bool is_open() const { return is_open_; }

  // Frees the animation's buffers. Does nothing if it is already closed.
  void Close();

  // Draws frame `frame_number` into `view` at the offset stored in the
  // animation file, applying every delta between the last frame drawn and the
  // requested one. Frames may be requested in any order, but the cost grows
  // with the distance from the previous frame. Returns false if the animation
  // is closed, the frame number is out of range, the view cannot be locked, or
  // the frame does not fit the view. Also returns false if a delta on the way
  // is corrupt; `view` then shows the last frame that could be reached, and a
  // later call carries on from there.
  bool DrawFrame(GraphicViewPortClass& view, int frame_number);

  // The number of frames, 0 if the animation is closed. Negative for an Amiga
  // animation, which sets the high bit of the file's 16-bit frame count.
  [[nodiscard]] int frame_count() const {
    return static_cast<int16_t>(total_frames_);
  }

 private:
  // Does the constructor's work on the open file. On failure the animation is
  // left half set up for Close() to reset.
  bool Load(int file_handle, std::span<uint8_t> palette);

  // Decompresses the delta that produces frame `delta_number` from the frame
  // before it and XORs it onto `dest`, the view's pixels, whose rows are
  // `dest_stride` apart; delta total_frames_ is the loop delta. Returns false,
  // with `dest` untouched, if the offset table is corrupt.
  bool ApplyFrameDelta(int delta_number, std::span<uint8_t> dest,
                       int dest_stride);

  bool is_open_ = false;
  int total_frames_ = 0;
  // The frame the view currently shows. Equal to total_frames_ until the first
  // DrawFrame(), meaning that not even frame 0 has been applied yet.
  int current_frame_ = 0;
  // Where the frame goes on the view. The file's offsets are read as signed,
  // so x_ and y_ may be negative.
  int x_ = 0;
  int y_ = 0;
  int width_ = 0;
  int height_ = 0;
  // False if the file has no frame 0: the animation starts from whatever is
  // already on the view.
  bool has_frame0_ = false;
  // Frame 0 is a delta against a picture the caller has already drawn, so it is
  // XORed onto the view instead of overwriting it.
  bool frame0_is_delta_ = false;
  // False if the file has no loop delta, so playback cannot wrap from the last
  // frame to frame 0 or back and DrawFrame() always takes the direct route.
  bool has_loop_delta_ = false;
  // Scratch space that holds one frame's delta, first compressed at the back
  // and then decompressed at the front.
  std::vector<std::byte> delta_buffer_;
  // The file's offset table and frames 1 and up.
  std::vector<std::byte> file_buffer_;
};

// XOR delta decoders, formerly the assembly in LP_ASM.ASM and now in wsa.cc.
// All of them stop quietly at the first command that is malformed or would
// step outside `target` or `delta`.

// Applies the uncompressed XOR delta in `delta` to `target`, treating `target`
// as one contiguous run of pixels.
void ApplyXorDelta(std::span<uint8_t> target, std::span<const std::byte> delta);

// Applies the uncompressed XOR delta in `delta` to a `width`-pixel-wide image
// whose rows start `stride` bytes apart in `target`; pixels past `width` on
// each row are left alone. The delta is XORed onto `target`, or overwrites it
// if `copy` is set. Does nothing if `width` or `stride` is not positive or
// `stride` is less than `width`.
void ApplyXorDeltaToView(std::span<uint8_t> target,
                         std::span<const std::byte> delta, int width,
                         int stride, bool copy);

#endif  // CNC_RED_ALERT_SDLLIB_WSA_H_
