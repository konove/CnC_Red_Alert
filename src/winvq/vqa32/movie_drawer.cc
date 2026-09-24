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
#include <utility>

#include "absl/log/check.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/types.h"
#include "winvq/vqa32/audio_ring.h"
#include "winvq/vqa32/frame_ring.h"
#include "winvq/vqa32/movie_clock.h"
#include "winvq/vqa32/vq_decoder.h"
#include "winvq/vqa32/vqa_format.h"
#include "winvq/vqa32/vqa_player.h"

// Places an image_width x image_height image in a buffer_width x
// buffer_height buffer from config's margins and origin flags. Both margins
// -1 center it; a single -1 is taken as a margin like any other.
static ImagePlacement PlaceImage(const VqaConfig& config,
                                 const int buffer_width,
                                 const int buffer_height, const int image_width,
                                 const int image_height) {
  ImagePlacement placement;
  if (config.margin_x == -1 && config.margin_y == -1) {
    placement.x1 = (buffer_width - image_width) / 2;
    placement.y1 = (buffer_height - image_height) / 2;
    placement.x2 = placement.x1 + image_width - 1;
    placement.y2 = placement.y1 + image_height - 1;
  } else {
    // The margins are the gap between the image and the buffer corner the
    // origin names, mirroring the top-left case: a zero gap puts the image
    // flush in that corner.
    const uint32_t origin = config.draw_flags & kVqaDrawOriginMask;
    const bool anchored_right =
        origin == kVqaDrawTopRight || origin == kVqaDrawBottomRight;
    const bool anchored_bottom =
        origin == kVqaDrawBottomLeft || origin == kVqaDrawBottomRight;

    if (anchored_right) {
      placement.x1 = buffer_width - 1 - config.margin_x;
      placement.x2 = placement.x1 - image_width + 1;
    } else {
      placement.x1 = config.margin_x;
      placement.x2 = placement.x1 + image_width - 1;
    }

    if (anchored_bottom) {
      placement.y1 = buffer_height - 1 - config.margin_y;
      placement.y2 = placement.y1 - image_height + 1;
    } else {
      placement.y1 = config.margin_y;
      placement.y2 = placement.y1 + image_height - 1;
    }
  }

  // The placement comes from the caller's config, not the file, so an image
  // that does not fit the buffer is a programmer error. Unchecked, the decoder
  // would write outside the buffer from the offset.
  DCHECK(std::min(placement.x1, placement.x2) >= 0 &&
         std::max(placement.x1, placement.x2) < buffer_width &&
         std::min(placement.y1, placement.y2) >= 0 &&
         std::max(placement.y1, placement.y2) < buffer_height);

  // The decoder fills rightward and downward, so it starts at the image's
  // top-left pixel whichever corner is anchored.
  placement.offset = (buffer_width * std::min(placement.y1, placement.y2)) +
                     std::min(placement.x1, placement.x2);
  return placement;
}

MovieDrawer::MovieDrawer(FrameRing& ring, const MovieClock& clock,
                         const AudioRing* const audio, const VqaHeader& header,
                         const VqaConfig& config)
    : ring_(&ring),
      clock_(&clock),
      audio_(audio),
      config_(&config),
      image_buffer_(config.image_buffer),
      image_width_(config.image_width),
      image_height_(config.image_height),
      // A partial block at the right or bottom edge is not drawn.
      blocks_per_row_(header.image_width / header.block_width),
      block_rows_(header.image_height / header.block_height) {
  const bool decode = (config.draw_flags & kVqaDrawToBuffer) != 0;

  // The caller's buffer; else, when the player decodes, its own the size of
  // the movie; else none, and nothing is decoded.
  if (image_buffer_.empty() && decode) {
    own_image_.resize(static_cast<std::size_t>(header.image_width) *
                      header.image_height);
    image_buffer_ = own_image_;
    image_width_ = header.image_width;
    image_height_ = header.image_height;
  }

  placement_ = PlaceImage(config, image_width_, image_height_,
                          header.image_width, header.image_height);

  // Without kVqaDrawToBuffer, or for a block size no decoder handles, frames
  // are not decoded.
  if (decode) {
    block_shape_ = BlockShapeFor(header.block_width, header.block_height);
  }
}

// This is where the frame skipping and pacing happen.
DrawStatus MovieDrawer::SelectFrame() {
  Frame* frame = &ring_->draw_frame();

  // The loader has not filled this buffer yet.
  if (!frame->loaded) {
    return DrawStatus::kNoFrame;
  }

  // Stepping draws each frame as soon as it is loaded, whatever the clock.
  if ((config_->option_flags & kVqaOptionStep) != 0) {
    last_selected_frame_ = frame->frame_number;
    return DrawStatus::kDrawn;
  }

  // The frame the clock has reached; a frame is due from its start time on.
  const int64_t due_frame =
      clock_->Now() * config_->frame_rate / kVqaTicksPerSecond;

  // Too early for this frame.
  if (frame->frame_number > due_frame) {
    return DrawStatus::kNotTime;
  }

  // Draw this frame however late it is when skipping is disabled, or once
  // frame_rate / 5 frames have gone by since the last frame selected, so a
  // slow machine still shows about 5 frames a second. kVqaDrawNoSkip holds
  // only until the sound first runs dry, so it does not happen again.
  const bool underran = audio_ != nullptr && audio_->underran();
  if (((config_->draw_flags & kVqaDrawNoSkip) != 0 && !underran) ||
      frame->frame_number - last_selected_frame_ >= config_->frame_rate / 5) {
    last_selected_frame_ = frame->frame_number;
    return DrawStatus::kDrawn;
  }

  // Playback is late: skip the frames before the one that is due, up to the
  // first key frame, or as far as the loader has got. A skipped frame's
  // palette still has to take effect, so it is kept for the frame drawn next.
  while (true) {
    // The loader has not caught up; continue from here next time.
    if (!frame->loaded) {
      return DrawStatus::kNoFrame;
    }

    // Stop at the frame that is due; key frames are never skipped.
    if (frame->key || frame->frame_number >= due_frame) {
      break;
    }

    // A later skipped palette replaces an earlier one.
    if (frame->has_palette) {
      frame->palette.Decompress();

      // A decompressed palette can be larger than the 256-color copy.
      const auto saved_bytes = static_cast<int32_t>(
          std::min(frame->palette.size(), base::ssize{kMaxPaletteBytes}));
      base::CopyBytes(base::ObjectBytes(saved_palette_),
                      std::as_bytes(frame->palette.contents()), saved_bytes);
      saved_palette_bytes_ = saved_bytes;
      palette_pending_ = true;
    }

    // Tell the client a frame went by undrawn.
    if (config_->frame_callback != nullptr &&
        config_->frame_callback(nullptr, frame->frame_number) != 0) {
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

  // Without a buffer (kVqaDrawToBuffer clear and none provided) a centered
  // image's offset lies past the empty one. The decoder then gets nothing and
  // draws nothing, and the frame goes on to be released like any other.
  const bool offset_in_buffer =
      placement_.offset >= 0 &&
      std::cmp_less_equal(placement_.offset, image_buffer_.size());
  const auto image =
      offset_in_buffer ? image_buffer_.subspan(base::ToSize(placement_.offset))
                       : std::span<unsigned char>{};

  const uint32_t slow_palette =
      (config_->option_flags & kVqaOptionSlowPalette) != 0 ? 1U : 0U;

  // Set the frame's own palette, or else the one SelectFrame() saved from a
  // frame it skipped: this frame's palette buffer holds only a stale palette
  // when the frame carries none.
  if (frame.has_palette) {
    QueueVqaPalette(frame.palette.writable_data(),
                    static_cast<int32_t>(frame.palette.size()), slow_palette);
  } else if (palette_pending_) {
    QueueVqaPalette(saved_palette_, saved_palette_bytes_, slow_palette);
  }
  palette_pending_ = false;

  if (block_shape_.has_value()) {
    DecodeVqFrame(*block_shape_, codebook.data.data(), frame.pointers.data(),
                  image, blocks_per_row_, block_rows_, image_width_);
  }

  last_drawn_frame_ = frame.frame_number;

  // The client shows the frame. The queued palette points into the frame's
  // buffer, so the frame goes back to the loader only after the callback.
  const bool stop =
      config_->frame_callback != nullptr &&
      config_->frame_callback(image_buffer_.data(), frame.frame_number) != 0;
  ring_->FinishDrawing();
  return stop ? DrawStatus::kStopped : DrawStatus::kDrawn;
}
