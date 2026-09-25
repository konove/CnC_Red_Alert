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

// Originally written by Bill Randolph and Denzil E. Long, Jr. at Westwood
// Studios, June 1995, with a draw and a page-flip routine for each DOS video
// mode; only the decode into a buffer survives.

#include "winvq/vqa32/movie_drawer.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

#include "engine/base/types.h"
#include "winvq/vqa32/audio_ring.h"
#include "winvq/vqa32/frame_ring.h"
#include "winvq/vqa32/movie_clock.h"
#include "winvq/vqa32/vq_decoder.h"
#include "winvq/vqa32/vqa_format.h"
#include "winvq/vqa32/vqa_player.h"

MovieDrawer::MovieDrawer(FrameRing& ring, const MovieClock& clock,
                         const AudioRing* const audio, const VqaHeader& header,
                         VqaClient& client, const bool skip_late_frames)
    : ring_(&ring),
      clock_(&clock),
      audio_(audio),
      client_(&client),
      skip_late_frames_(skip_late_frames),
      frame_rate_(header.fps),
      image_(static_cast<std::size_t>(header.image_width) *
             header.image_height),
      width_(header.image_width),
      height_(header.image_height),
      block_shape_(BlockShapeFor(header.block_width, header.block_height)),
      // A partial block at the right or bottom edge is not drawn.
      blocks_per_row_(header.image_width / header.block_width),
      block_rows_(header.image_height / header.block_height) {}

// This is where the frame skipping and pacing happen.
DrawStatus MovieDrawer::SelectFrame() {
  Frame* frame = &ring_->draw_frame();

  // The loader has not filled this buffer yet.
  if (!frame->loaded) {
    return DrawStatus::kNoFrame;
  }

  // The frame the clock has reached; a frame is due from its start time on.
  const int64_t due_frame = clock_->Now() * frame_rate_ / kVqaTicksPerSecond;

  // Too early for this frame.
  if (frame->frame_number > due_frame) {
    return DrawStatus::kNotTime;
  }

  // Draw this frame however late it is when skipping is off, or once
  // frame_rate / 5 frames have gone by since the last frame selected, so a
  // slow machine still shows about 5 frames a second. Skipping comes on for
  // good once the sound has run dry, so it does not happen again.
  const bool may_skip =
      skip_late_frames_ || (audio_ != nullptr && audio_->underran());
  if (!may_skip ||
      frame->frame_number - last_selected_frame_ >= frame_rate_ / 5) {
    last_selected_frame_ = frame->frame_number;
    return DrawStatus::kDrawn;
  }

  // Playback is late: drop the frames before the one that is due, up to the
  // first key frame, or as far as the loader has got. A dropped frame's
  // palette still has to take effect, so it is carried to the frame drawn
  // next; a later one replaces it.
  while (true) {
    // The loader has not caught up; continue from here next time.
    if (!frame->loaded) {
      return DrawStatus::kNoFrame;
    }

    // Stop at the frame that is due; key frames are never skipped.
    if (frame->key || frame->frame_number >= due_frame) {
      break;
    }

    if (frame->has_palette) {
      frame->palette.Decompress();
      const std::span<const unsigned char> palette = frame->palette.contents();
      carried_palette_.assign(
          palette.begin(),
          palette.begin() +
              std::min(std::ssize(palette), base::ssize{kMaxPaletteBytes}));
    }

    if (!client_->OnFrameSkipped(frame->frame_number)) {
      return DrawStatus::kStopped;
    }

    ring_->FinishDrawing();
    frame = &ring_->draw_frame();
  }

  last_selected_frame_ = frame->frame_number;
  return DrawStatus::kDrawn;
}

DrawStatus MovieDrawer::DrawNextFrame() {
  if (const DrawStatus status = SelectFrame(); status != DrawStatus::kDrawn) {
    return status;
  }

  // Decompress what the frame is drawn from. The group's later frames find
  // the codebook decompressed already.
  Frame& frame = ring_->draw_frame();
  Codebook& codebook = ring_->codebook(frame.codebook);
  codebook.data.Decompress();
  frame.palette.Decompress();
  frame.pointers.Decompress();

  if (block_shape_.has_value()) {
    DecodeVqFrame(*block_shape_, codebook.data.data(), frame.pointers.data(),
                  image_, blocks_per_row_, block_rows_, width_);
  }

  // Show the frame with its own palette, or else the one carried from a frame
  // dropped before it: this frame's palette buffer holds only a stale palette
  // when the frame carries none.
  VqaFrameView view{.frame_number = frame.frame_number,
                    .width = width_,
                    .height = height_,
                    .pixels = image_,
                    .palette = {}};
  if (frame.has_palette) {
    const std::span<const unsigned char> palette = frame.palette.contents();
    view.palette = palette.first(
        std::min(palette.size(), static_cast<std::size_t>(kMaxPaletteBytes)));
  } else {
    view.palette = carried_palette_;
  }
  last_drawn_frame_ = frame.frame_number;

  const bool keep_going = client_->OnFrame(view);
  carried_palette_.clear();
  ring_->FinishDrawing();
  return keep_going ? DrawStatus::kDrawn : DrawStatus::kStopped;
}
