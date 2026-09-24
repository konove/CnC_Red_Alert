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

// File: the VQA drawer. It picks the loaded frame the clock says is due,
// skipping late frames to catch up, decompresses that frame's data and decodes
// it into the image buffer. ConfigureDrawer() sets it up when playback starts.
//
// Originally written by Bill Randolph and Denzil E. Long, Jr. at Westwood
// Studios, June 1995, with a draw and a page-flip routine for each DOS video
// mode; only the decode into a buffer survives.

#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>

#include "absl/log/check.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/types.h"
#include "winvq/vqa32/frame_ring.h"
#include "winvq/vqa32/vq_decoder.h"
#include "winvq/vqa32/vqa_format.h"
#include "winvq/vqa32/vqa_player.h"
#include "winvq/vqa32/vqa_player_state.h"

void ConfigureDrawer(VqaPlayerState* state) {
  VqaMovie* movie = state->movie.get();
  VqaDrawer* drawer = &movie->drawer;
  VqaHeader* header = &state->header;
  VqaConfig* config = &state->config;
  const uint32_t origin = config->draw_flags & kVqaDrawOriginMask;

  // Place the image in the buffer. Both margins -1 center it; a single -1 is
  // taken as a margin like any other.
  if (config->margin_x == -1 && config->margin_y == -1) {
    drawer->x1 = (drawer->image_width - header->image_width) / 2;
    drawer->y1 = (drawer->image_height - header->image_height) / 2;
    drawer->x2 = drawer->x1 + header->image_width - 1;
    drawer->y2 = drawer->y1 + header->image_height - 1;
  } else {
    // The margins are the gap between the image and the buffer corner the
    // origin names, mirroring the top-left case: a zero gap puts the image
    // flush in that corner. x1,y1 is the image pixel nearest that corner and
    // x2,y2 the opposite pixel, both inclusive.
    const bool anchored_right =
        origin == kVqaDrawTopRight || origin == kVqaDrawBottomRight;
    const bool anchored_bottom =
        origin == kVqaDrawBottomLeft || origin == kVqaDrawBottomRight;

    if (anchored_right) {
      drawer->x1 = drawer->image_width - 1 - config->margin_x;
      drawer->x2 = drawer->x1 - header->image_width + 1;
    } else {
      drawer->x1 = config->margin_x;
      drawer->x2 = drawer->x1 + header->image_width - 1;
    }

    if (anchored_bottom) {
      drawer->y1 = drawer->image_height - 1 - config->margin_y;
      drawer->y2 = drawer->y1 - header->image_height + 1;
    } else {
      drawer->y1 = config->margin_y;
      drawer->y2 = drawer->y1 + header->image_height - 1;
    }
  }

  // The placement comes from the caller's config, not the file, so an image
  // that does not fit the buffer is a programmer error. Unchecked, the decoder
  // would write outside the buffer from image_offset.
  DCHECK(std::min(drawer->x1, drawer->x2) >= 0 &&
         std::max(drawer->x1, drawer->x2) < drawer->image_width &&
         std::min(drawer->y1, drawer->y2) >= 0 &&
         std::max(drawer->y1, drawer->y2) < drawer->image_height);

  // Pick the decoder for the movie's block size, and the image size in blocks
  // it walks. A partial block at the right or bottom edge is not drawn.
  drawer->blocks_per_row = header->image_width / header->block_width;
  drawer->block_rows = header->image_height / header->block_height;

  // Without kVqaDrawToBuffer, or for a block size no decoder handles, frames
  // are not decoded.
  movie->block_shape =
      (config->draw_flags & kVqaDrawToBuffer) != 0
          ? BlockShapeFor(header->block_width, header->block_height)
          : std::nullopt;

  // The decoder fills rightward and downward, so it starts at the image's
  // top-left pixel whichever corner is anchored.
  drawer->image_offset =
      (drawer->image_width * std::min(drawer->y1, drawer->y2)) +
      std::min(drawer->x1, drawer->x2);
}

// Moves the drawer on to the frame to draw next and returns 0, or returns
// kVqaNoBuffer (the loader has not caught up), kVqaNotTime (the frame is not
// due yet) or kVqaEndOfMovie (frame_callback asked to stop while frames were
// skipped). When playback runs late it skips the frames the clock has passed,
// except key frames, and keeps a skipped frame's palette for the next frame
// drawn. This is where the frame skipping and pacing happen.
static int32_t SelectFrameToDraw(VqaPlayerState* state) {
  VqaConfig* config = &state->config;
  VqaMovie* movie = state->movie.get();
  VqaDrawer* drawer = &movie->drawer;
  Frame* frame = &movie->ring.draw_frame();

  // The loader has not filled this buffer yet.
  if (!frame->loaded) {
    return kVqaNoBuffer;
  }

  // Stepping draws each frame as soon as it is loaded, whatever the clock.
  if (config->option_flags & kVqaOptionStep) {
    drawer->last_selected_frame = frame->frame_number;
    return 0;
  }

  // The frame the clock has reached; a frame is due from its start time on.
  const int64_t now_ticks = movie->clock.Now();
  const int64_t due_frame = now_ticks * config->frame_rate / kVqaTicksPerSecond;

  // Too early for this frame.
  if (frame->frame_number > due_frame) {
    return kVqaNotTime;
  }

  // Draw this frame however late it is when skipping is disabled, or once
  // frame_rate / 5 frames have gone by since the last frame selected, so a
  // slow machine still shows about 5 frames a second. kVqaDrawNoSkip holds
  // only until the sound first runs dry, so it does not happen again.
  const bool underran = movie->audio != nullptr && movie->audio->underran();
  if (((config->draw_flags & kVqaDrawNoSkip) != 0 && !underran) ||
      frame->frame_number - drawer->last_selected_frame >=
          config->frame_rate / 5) {
    drawer->last_selected_frame = frame->frame_number;
    return 0;
  }

  // Playback is late: skip the frames before the one that is due, up to the
  // first key frame, or as far as the loader has got. A skipped frame's
  // palette still has to take effect, so it is kept for the frame drawn next.
  while (true) {
    // The loader has not caught up; continue from here next time.
    if (!frame->loaded) {
      return kVqaNoBuffer;
    }

    // Stop at the frame that is due; key frames are never skipped.
    if (frame->key || frame->frame_number >= due_frame) {
      break;
    }

    // Stash the palette in saved_palette, and flag it for DrawNextFrame() to
    // set with the next frame it draws. A later skipped palette replaces it.
    if (frame->has_palette) {
      frame->palette.Decompress();

      // A decompressed palette can be up to palette_capacity bytes, more than
      // the 256-color copy holds.
      const auto saved_bytes = static_cast<int32_t>(std::min(
          frame->palette.size(), base::ssize{sizeof(drawer->saved_palette)}));
      base::CopyBytes(base::ObjectBytes(drawer->saved_palette),
                      std::as_bytes(frame->palette.contents()), saved_bytes);
      drawer->saved_palette_bytes = saved_bytes;
      drawer->flags |= kDrawerPalettePending;
    }

    // Tell the client a frame went by undrawn.
    if ((config->frame_callback != nullptr) &&
        (config->frame_callback(nullptr, frame->frame_number) != 0)) {
      return kVqaEndOfMovie;
    }

    movie->ring.FinishDrawing();
    frame = &movie->ring.draw_frame();
  }

  drawer->last_selected_frame = frame->frame_number;

  return 0;
}

// Decompresses what a frame is drawn from: its codebook, palette and vector
// pointers. The group's later frames find the codebook decompressed already.
static void DecompressFrame(Frame& frame, Codebook& codebook) {
  codebook.data.Decompress();
  frame.palette.Decompress();
  frame.pointers.Decompress();
}

int32_t DrawNextFrame(VqaPlayerState* state) {
  const VqaConfig* config = &state->config;
  VqaMovie* movie = state->movie.get();
  VqaDrawer* drawer = &movie->drawer;

  if (const auto result = SelectFrameToDraw(state); result != 0) {
    return result;
  }

  Frame& frame = movie->ring.draw_frame();
  Codebook& codebook = movie->ring.codebook(frame.codebook);
  DecompressFrame(frame, codebook);

  // Without a buffer (kVqaDrawToBuffer clear and none provided) a centered
  // image's offset lies past the empty one. The decoder then gets nothing and
  // draws nothing, and the frame goes on to be released like any other.
  const bool offset_in_buffer =
      drawer->image_offset >= 0 &&
      std::cmp_less_equal(drawer->image_offset, drawer->image_buffer.size());
  const auto image =
      offset_in_buffer
          ? drawer->image_buffer.subspan(base::ToSize(drawer->image_offset))
          : std::span<unsigned char>{};

  const uint32_t slow_palette =
      (config->option_flags & kVqaOptionSlowPalette) != 0 ? 1U : 0U;

  // Set the frame's own palette, or else the one SelectFrameToDraw() saved from
  // a frame it skipped: this frame's palette buffer holds only a stale palette
  // when the frame carries none.
  if (frame.has_palette) {
    QueueVqaPalette(frame.palette.writable_data(),
                    static_cast<int32_t>(frame.palette.size()), slow_palette);
  } else if ((drawer->flags & kDrawerPalettePending) != 0) {
    QueueVqaPalette(drawer->saved_palette, drawer->saved_palette_bytes,
                    slow_palette);
  }
  drawer->flags &= ~kDrawerPalettePending;

  if (movie->block_shape.has_value()) {
    DecodeVqFrame(*movie->block_shape, codebook.data.data(),
                  frame.pointers.data(), image, drawer->blocks_per_row,
                  drawer->block_rows, drawer->image_width);
  }

  // For PlayVqa() to return in walk mode.
  drawer->last_drawn_frame = frame.frame_number;

  // The client shows the frame. The queued palette points into the frame's
  // buffer, so the frame goes back to the loader only after the callback.
  const bool stop = config->frame_callback != nullptr &&
                    config->frame_callback(drawer->image_buffer.data(),
                                           frame.frame_number) != 0;
  movie->ring.FinishDrawing();

  return stop ? kVqaEndOfMovie : 0;
}
