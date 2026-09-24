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
#include <span>
#include <utility>

#include "absl/log/check.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "winvq/vqa32/unvq.h"
#include "winvq/vqa32/vqa_format.h"
#include "winvq/vqa32/vqa_player.h"
#include "winvq/vqa32/vqa_player_state.h"
#include "winvq/vqm32/compress.h"

static int32_t Select_Frame(VqaPlayerState* vqap);
static void Prepare_Frame(VqaMovie* vqabuf);


static void UnVQ_Nop(std::span<const unsigned char> codebook,
                     std::span<const unsigned char> pointers,
                     std::span<unsigned char> buffer, int blocksperrow,
                     int numrows, int bufwidth);

void ConfigureDrawer(VqaPlayerState* vqap) {
  VqaMovie* vqabuf = vqap->movie.get();
  VqaDrawer* drawer = &vqabuf->drawer;
  VqaHeader* header = &vqap->header;
  VqaConfig* config = &vqap->config;
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
    const bool right =
        origin == kVqaDrawTopRight || origin == kVqaDrawBottomRight;
    const bool bottom =
        origin == kVqaDrawBottomLeft || origin == kVqaDrawBottomRight;

    if (right) {
      drawer->x1 = drawer->image_width - 1 - config->margin_x;
      drawer->x2 = drawer->x1 - header->image_width + 1;
    } else {
      drawer->x1 = config->margin_x;
      drawer->x2 = drawer->x1 + header->image_width - 1;
    }

    if (bottom) {
      drawer->y1 = drawer->image_height - 1 - config->margin_y;
      drawer->y2 = drawer->y1 - header->image_height + 1;
    } else {
      drawer->y1 = config->margin_y;
      drawer->y2 = drawer->y1 + header->image_height - 1;
    }
  }

  // The placement comes from the caller's config, not the file, so an image
  // that does not fit the buffer is a programmer error. Unchecked, decode_frame
  // would write outside the buffer from image_offset.
  DCHECK(std::min(drawer->x1, drawer->x2) >= 0 &&
         std::max(drawer->x1, drawer->x2) < drawer->image_width &&
         std::min(drawer->y1, drawer->y2) >= 0 &&
         std::max(drawer->y1, drawer->y2) < drawer->image_height);

  // Pick the decoder for the movie's block size, and the image size in blocks
  // it walks. A partial block at the right or bottom edge is not drawn.
  drawer->blocks_per_row = header->image_width / header->block_width;
  drawer->block_rows = header->image_height / header->block_height;
  const uint32_t blkdim =
      BlockDimensions(header->block_width, header->block_height);

  // Without kVqaDrawToBuffer, or for a block size no decoder handles, frames
  // decode to nothing rather than through a null pointer.
  vqabuf->decode_frame = UnVQ_Nop;

  if (config->draw_flags & kVqaDrawToBuffer) {
    switch (blkdim) {
      case kBlock4x2:
        vqabuf->decode_frame = UnVQ_4x2;
        break;
      case kBlock4x4:
        vqabuf->decode_frame = UnVQ_4x4;
        break;
      default:
        break;
    }
  }

  // decode_frame fills rightward and downward, so it starts at the image's
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
static int32_t Select_Frame(VqaPlayerState* vqap) {
  VqaConfig* config = &vqap->config;
  VqaMovie* vqabuf = vqap->movie.get();
  VqaDrawer* drawer = &vqabuf->drawer;
  VqaFrame* curframe = drawer->current_frame;

  // The loader has not filled this buffer yet.
  if ((curframe->flags & kFrameLoaded) == 0) {
    return kVqaNoBuffer;
  }

  // Stepping draws each frame as soon as it is loaded, whatever the clock.
  if (config->option_flags & kVqaOptionStep) {
    drawer->last_selected_frame = curframe->frame_number;
    return 0;
  }

  // The frame the clock has reached; a frame is due from its start time on.
  // It counts at draw_rate, not frame_rate, since a Westwood change of June
  // 1995 ("should look for the desired frame to draw, not load, right?").
  // TODO: With draw_rate != frame_rate this compares draw-rate counts against
  // frame numbers, which run at frame_rate, so the skip loop below skips too
  // few frames; and last_time, the draw_rate pacing below, is set only when
  // the skip loop runs, so with kVqaDrawNoSkip the pacing never engages.
  // Neither game sets a draw_rate of its own.
  const int64_t curtime = ReadMovieClock(vqap);
  const int64_t desiredframe = curtime * config->draw_rate / kVqaTicksPerSecond;

  // Too early for this frame? With a draw_rate of its own the drawer waits
  // one draw period since the last frame; otherwise until the frame is due.
  if (config->draw_rate != config->frame_rate) {
    if (curtime - drawer->last_time < kVqaTicksPerSecond / config->draw_rate) {
      return kVqaNotTime;
    }
  } else {
    if (curframe->frame_number > desiredframe) {
      return kVqaNotTime;
    }
  }

  // Once frame_rate / 5 frames have gone by since the last frame selected,
  // draw this one however late it is rather than skip further, so a slow
  // machine still shows about 5 frames a second.
  if (curframe->frame_number - drawer->last_selected_frame >=
      config->frame_rate / 5) {
    drawer->last_selected_frame = curframe->frame_number;
    return 0;
  }

  // Skipping disabled: draw every frame, late or not.
  if (config->draw_flags & kVqaDrawNoSkip) {
    drawer->last_selected_frame = curframe->frame_number;
    return 0;
  }

  // Playback is late: skip the frames before the one that is due, up to the
  // first key frame, or as far as the loader has got. A skipped frame's
  // palette still has to take effect, so it is kept for the frame drawn next.
  while (true) {
    // The loader has not caught up; continue from here next time.
    if ((curframe->flags & kFrameLoaded) == 0) {
      return kVqaNoBuffer;
    }

    // Key frames are never skipped.
    if (curframe->flags & kFrameKey) {
      break;
    }

    if (curframe->frame_number < desiredframe) {
      // Stash the palette in saved_palette, and flag it for DrawNextFrame() to
      // set with the next frame it draws. A later skipped palette replaces it.
      if (curframe->flags & kFrameHasPalette) {
        // Decompressed in place, as Prepare_Frame() would have.
        if (curframe->flags & kFramePaletteCompressed) {
          curframe->palette_bytes = LCW_Uncompress(
              std::span(curframe->palette)
                  .subspan(base::ToSize(curframe->palette_offset)),
              curframe->palette);

          curframe->flags &= ~kFramePaletteCompressed;
        }

        // Stash the palette. A decompressed palette can report up to
        // palette_capacity bytes, more than the 256-color copy holds.
        const int32_t stash_size = std::min(
            curframe->palette_bytes, int32_t{sizeof(drawer->saved_palette)});
        base::CopyBytes(base::ObjectBytes(drawer->saved_palette),
                        std::as_bytes(std::span(curframe->palette)),
                        stash_size);
        drawer->saved_palette_bytes = stash_size;
        drawer->flags |= kDrawerPalettePending;
      }

      // Tell the client a frame went by undrawn.
      if ((config->frame_callback != nullptr) &&
          (config->frame_callback(nullptr, curframe->frame_number) != 0)) {
        return kVqaEndOfMovie;
      }

      // Clearing the flags hands the buffer back to the loader.
      curframe->flags = 0;
      curframe = curframe->next;
      drawer->current_frame = curframe;
    } else {
      break;
    }
  }

  drawer->last_selected_frame = curframe->frame_number;
  drawer->last_time = curtime;

  return 0;
}

// Decompresses the drawer's current frame: its codebook, palette and vector
// pointers. Each is LCW compressed at the end of its buffer and decompressed in
// place towards the start, and only once: the flags record what is still
// compressed, and a codebook serves every frame of its group.
static void Prepare_Frame(VqaMovie* vqabuf) {
  VqaDrawer* drawer = &vqabuf->drawer;
  VqaFrame* curframe = drawer->current_frame;
  VqaCodebook* codebook = curframe->codebook;

  if (codebook->flags & kCodebookCompressed) {
    LCW_Uncompress(std::span(codebook->buffer)
                       .subspan(base::ToSize(codebook->compressed_offset)),
                   codebook->buffer);

    // The group's later frames use it as it is.
    codebook->flags &= ~kCodebookCompressed;
  }

  if (curframe->flags & kFramePaletteCompressed) {
    curframe->palette_bytes =
        LCW_Uncompress(std::span(curframe->palette)
                           .subspan(base::ToSize(curframe->palette_offset)),
                       curframe->palette);

    curframe->flags &= ~kFramePaletteCompressed;
  }

  if (curframe->flags & kFramePointersCompressed) {
    LCW_Uncompress(std::span(curframe->pointers)
                       .subspan(base::ToSize(curframe->pointers_offset)),
                   curframe->pointers);

    curframe->flags &= ~kFramePointersCompressed;
  }
}

int32_t DrawNextFrame(VqaPlayerState* vqa) {
  auto* vqa_handle_p = vqa;
  const VqaConfig* config = &vqa_handle_p->config;
  VqaMovie* vqabuf = vqa_handle_p->movie.get();
  VqaDrawer* drawer = &vqabuf->drawer;

  // A drawer asleep has its frame selected and decompressed already, and is
  // only waiting for the last frame drawn to be released.
  if (!(vqabuf->flags & kMovieDrawerAsleep)) {
    if (const auto result = Select_Frame(vqa_handle_p); result != 0) {
      return result;
    }

    Prepare_Frame(vqabuf);
  }

  // One drawn frame at a time: wait for ReleaseDrawnFrame() to hand back the
  // last one before drawing over the image buffer again.
  if (vqabuf->flags & kMovieAwaitingRelease) {
    vqabuf->flags |= kMovieDrawerAsleep;
    return kVqaSleeping;
  }

  vqabuf->flags &= ~kMovieDrawerAsleep;

  VqaFrame* curframe = drawer->current_frame;

  // Without a buffer (kVqaDrawToBuffer clear and none provided) a centered
  // image's offset lies past the empty one. decode_frame then gets nothing and
  // draws nothing, and the frame goes on to be released like any other.
  const bool offset_in_buffer =
      drawer->image_offset >= 0 &&
      std::cmp_less_equal(drawer->image_offset, drawer->image_buffer.size());
  const auto buff =
      offset_in_buffer
          ? drawer->image_buffer.subspan(base::ToSize(drawer->image_offset))
          : std::span<unsigned char>{};

  const uint32_t slowpal =
      (config->option_flags & kVqaOptionSlowPalette) != 0 ? 1U : 0U;

  // Set the frame's own palette, or else the one Select_Frame() saved from a
  // frame it skipped: this frame's palette buffer holds only a stale palette
  // when the frame carries none.
  if ((curframe->flags & kFrameHasPalette) != 0) {
    QueueVqaPalette(curframe->palette, curframe->palette_bytes, slowpal);
  } else if ((drawer->flags & kDrawerPalettePending) != 0) {
    QueueVqaPalette(drawer->saved_palette, drawer->saved_palette_bytes,
                    slowpal);
  }
  curframe->flags &= ~kFrameHasPalette;
  drawer->flags &= ~kDrawerPalettePending;

  // Decode the image.
  vqabuf->decode_frame(curframe->codebook->buffer, curframe->pointers, buff,
                       drawer->blocks_per_row, drawer->block_rows,
                       drawer->image_width);

  // For PlayVqa() to return in walk mode.
  drawer->last_drawn_frame = curframe->frame_number;

  // ReleaseDrawnFrame() frees this buffer once the frame has been shown.
  vqabuf->flipper.drawn_frame = curframe;
  vqabuf->flags |= kMovieAwaitingRelease;

  // The client shows the frame.
  if ((config->frame_callback != nullptr) &&
      (config->frame_callback(drawer->image_buffer.data(),
                              curframe->frame_number) != 0)) {
    return kVqaEndOfMovie;
  }

  drawer->current_frame = curframe->next;

  return 0;
}

// The decode_frame of a movie that draws nothing: there is no buffer to draw
// into, or no decoder for its block size.
static void UnVQ_Nop(std::span<const unsigned char> /*codebook*/,
                     std::span<const unsigned char> /*pointers*/,
                     std::span<unsigned char> /*buffer*/, int /*blocksperrow*/,
                     int /*numrows*/, int /*bufwidth*/) {}
