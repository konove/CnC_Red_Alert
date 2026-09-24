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

/****************************************************************************
 *
 *        C O N F I D E N T I A L -- W E S T W O O D  S T U D I O S
 *
 *----------------------------------------------------------------------------
 *
 * PROJECT
 *     VQAPlay32 library. (32-Bit protected mode)
 *
 * FILE
 *     drawer.c
 *
 * DESCRIPTION
 *     Frame drawing and page flip control.
 *
 * PROGRAMMER
 *     Bill Randolph
 *     Denzil E. Long, Jr.
 *
 * DATE
 *     June 26, 1995
 *
 *----------------------------------------------------------------------------
 *
 * PUBLIC
 *     ConfigureDrawer - Configure the drawer routines.
 *
 * PRIVATE
 *     Select_Frame             - Selects frame to draw and preforms frame
 *                                skip.
 *     Prepare_Frame            - Process/Decompress frame information.
 *     DrawFrame_Xmode          - Draws a frame directly to Xmode screen.
 *     DrawFrame_XmodeBuf       - Draws a frame in Xmode format to a buffer.
 *     DrawFrame_XmodeVRAM      - Draws a frame in Xmode with resident
 *                                Codebook.
 *     PageFlip_Xmode           - Page flip Xmode display.
 *     DrawFrame_MCGA           - Draws a frame directly to MCGA screen.
 *     PageFlip_MCGA            - Page flip MCGA display.
 *     DrawFrame_MCGABuf        - Draws a frame in MCGA format to a buffer.
 *     PageFlip_MCGABuf         - Page flip a buffered MCGA display.
 *     DrawFrame_VESA640        - Draws a frame in VESA640 format.
 *     DrawFrame_VESA320_32K    - Draws a frame to VESA320_32K screen.
 *     DrawFrame_VESA320_32KBuf - Draws a frame in VESA320_32K format to a
 *                                buffer.
 *     PageFlip_VESA            - Page flip VESA display.
 *     PageFlip_Nop             - Do nothing page flip.
 *     UnVQ_Nop                 - Do nothing decode_frame.
 *     Mask_Rect                - Sets non-drawable rectangle in image.
 *     Mask_Pointers            - Mask vector pointer that are in the mask
 *                                rectangle.
 *
 ****************************************************************************/

#include <algorithm>
#include <cstdint>
#include <span>

#include "absl/log/check.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "winvq/vqa32/unvq.h"
#include "winvq/vqa32/vqa_format.h"
#include "winvq/vqa32/vqa_player.h"
#include "winvq/vqa32/vqa_player_state.h"
#include "winvq/vqm32/compress.h"

/*---------------------------------------------------------------------------
 * PRIVATE DECLARATIONS
 *-------------------------------------------------------------------------*/
static int32_t Select_Frame(VqaPlayerState* vqap);
static void Prepare_Frame(VqaMovie* vqabuf);


static void UnVQ_Nop(std::span<const unsigned char> codebook,
                     std::span<const unsigned char> pointers,
                     std::span<unsigned char> buffer, int blocksperrow,
                     int numrows, int bufwidth);

/****************************************************************************
 *
 * NAME
 *     ConfigureDrawer - Configure the drawer routines.
 *
 * SYNOPSIS
 *     ConfigureDrawer(VQA)
 *
 *     void ConfigureDrawer(VqaPlayerState *);
 *
 * FUNCTION
 *     Configure the drawing system for the current movie and configuration
 *     options.
 *
 * INPUTS
 *     VQA - Pointer to private VqaPlayerState.
 *
 * RESULT
 *     NONE
 *
 ****************************************************************************/

void ConfigureDrawer(VqaPlayerState* vqap) {
  /* Dereference commonly used data members for quicker access. */
  VqaMovie* vqabuf = vqap->movie.get();
  VqaDrawer* drawer = &vqabuf->drawer;
  VqaHeader* header = &vqap->header;
  VqaConfig* config = &vqap->config;
  const uint32_t origin = config->draw_flags & kVqaDrawOriginMask;

  /*-------------------------------------------------------------------------
   * SET THE DRAW POSITION OF THE MOVIE.
   *
   * X1 = -1 -- Center image of the X axis, otherwise use X1 value.
   * Y1 = -1 -- Center image of the Y axis, otherwise use Y1 value.
   *-----------------------------------------------------------------------*/
  if (config->margin_x == -1 && config->margin_y == -1) {
    drawer->x1 = (drawer->image_width - header->image_width) / 2;
    drawer->y1 = (drawer->image_height - header->image_height) / 2;
    drawer->x2 = drawer->x1 + header->image_width - 1;
    drawer->y2 = drawer->y1 + header->image_height - 1;
  } else {
    // config->X1/Y1 is the gap between the image and the buffer corner the
    // origin names, mirroring the top-left case: a zero gap puts the image
    // flush in that corner. X1,Y1 is the image pixel nearest that corner and
    // X2,Y2 the opposite pixel, both inclusive.
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

  /*-------------------------------------------------------------------------
   * INITIALIZE THE UNVQ ROUTINE FOR THE SPECIFIED VIDEO MODE AND BLOCK SIZE.
   *-----------------------------------------------------------------------*/

  /* Pre-compute commonly used values for speed. */
  drawer->blocks_per_row = header->image_width / header->block_width;
  drawer->block_rows = header->image_height / header->block_height;
  const uint32_t blkdim =
      BlockDimensions(header->block_width, header->block_height);

  /* Initialize draw routine vectors to a NOP routine in order to prevent
   * a crash.
   */
  vqabuf->decode_frame = UnVQ_Nop;

  /* If the client specifies buffering then go ahead an set the unvq
   * vector. All of the buffered modes use the same unvq routines.
   */
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

  // Pre-compute the draw offset for speed. decode_frame fills rightward and
  // downward, so it starts at the image's top-left pixel whichever corner is
  // anchored.
  drawer->image_offset =
      (drawer->image_width * std::min(drawer->y1, drawer->y2)) +
      std::min(drawer->x1, drawer->x2);
}

/****************************************************************************
 *
 * NAME
 *     Select_Frame - Selects frame to draw and preforms frame skip.
 *
 * SYNOPSIS
 *     Error = Select_Frame(VQA)
 *
 *     long Select_Frame(VqaPlayerState *);
 *
 * FUNCTION
 *     Select a frame to draw. This is were the frame skipping/delay is
 *     performed.
 *
 * INPUTS
 *     VQA - Pointer to private VqaPlayerState.
 *
 * RESULT
 *     Error - 0 if successful, or VQAERR_??? error code.
 *
 ****************************************************************************/

static int32_t Select_Frame(VqaPlayerState* vqap) {
  /* Dereference commonly used data members for quicker access. */
  VqaConfig* config = &vqap->config;
  VqaMovie* vqabuf = vqap->movie.get();
  VqaDrawer* drawer = &vqabuf->drawer;
  VqaFrame* curframe = drawer->current_frame;

  /* Make sure the current frame is drawable. If the frame is not ready
   * then we must wait for the loader to catch up.
   */
  if ((curframe->flags & kFrameLoaded) == 0) {
    return kVqaNoBuffer;
  }

  /* If single stepping then return with the next frame.*/
  if (config->option_flags & kVqaOptionStep) {
    drawer->last_selected_frame = curframe->frame_number;
    return 0;
  }

  /* Find the frame # we should play (rounded to nearest frame): */
  const int64_t curtime = ReadMovieClock(vqap);
  // MEG MOD 06.22.95 - Should look for the desired frame to draw, not load,
  // right?
  const int64_t desiredframe = curtime * config->draw_rate / kVqaTicksPerSecond;

  /* Handle the cases where the player is going so fast that it's not time
   * to draw this frame yet.
   *
   * - If the Drawer is using a slower frame rate than the Loader, use a
   *   delta-time-based wait; otherwise, use the frame number as the wait.
   */
  if (config->draw_rate != config->frame_rate) {
    if (curtime - drawer->last_time < kVqaTicksPerSecond / config->draw_rate) {
      return kVqaNotTime;
    }
  } else {
    if (curframe->frame_number > desiredframe) {
      return kVqaNotTime;
    }
  }

  /* Make sure we draw at least 5 frames per second */
  if (curframe->frame_number - drawer->last_selected_frame >=
      config->frame_rate / 5) {
    drawer->last_selected_frame = curframe->frame_number;
    return 0;
  }

  /* If frame skipping is disabled then draw every frame. */
  if (config->draw_flags & kVqaDrawNoSkip) {
    drawer->last_selected_frame = curframe->frame_number;
    return 0;
  }

  /* Handle the case where the player is going too slow, so we have to skip
   * some frames:
   *
   * - If this is a Key Frame, draw it
   * - If this frame's # is less than what we're supposed to draw, skip it
   *   (Because the 1st 'desiredframe' will be 0, frame_number MUST be typecast
   *   to signed WORD for the comparison; otherwise, the comparison uses
   *   UWORDs, and the first frame is always skipped.)
   * - If this is a palette-set frame, set the palette before skipping it
   * - Loop until we get the frame we need, or there's no frames available
   */
  while (true) {
    /* No frame available; return */
    if ((curframe->flags & kFrameLoaded) == 0) {
      return kVqaNoBuffer;
    }

    /* Force drawing of a Key Frame */
    if (curframe->flags & kFrameKey) {
      break;
    }

    /* Skip the frame */
    if (curframe->frame_number < desiredframe) {
      /* Handle a palette in a skipped frame:
       *
       * - Stash the palette in Drawer.saved_palette
       * - Set the Drawer.Flags kDrawerPalettePending bit, to tell the page-flip
       *   routines that this palette must be set
       */
      if (curframe->flags & kFrameHasPalette) {
        /* Un-LCW if needed */
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

      /* Invoke callback with nullptr screen ptr */
      if ((config->frame_callback != nullptr) &&
          (config->frame_callback(nullptr, curframe->frame_number) != 0)) {
        return kVqaEndOfMovie;
      }

      /* Skip the frame */
      curframe->flags = 0L;
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

/****************************************************************************
 *
 * NAME
 *     Prepare_Frame - Process/Decompress frame information.
 *
 * SYNOPSIS
 *     Prepare_Frame(VqaMovie)
 *
 *     void Prepare_Frame(VqaMovie *);
 *
 * FUNCTION
 *     Decompress and preprocess the various frame elements (codebook,
 *     pointers, palette, etc...)
 *
 * INPUTS
 *     VqaMovie - Pointer to VqaMovie structure.
 *
 * RESULT
 *     NONE
 *
 ****************************************************************************/

static void Prepare_Frame(VqaMovie* vqabuf) {
  /* Dereference commonly used data members for quicker access. */
  VqaDrawer* drawer = &vqabuf->drawer;
  VqaFrame* curframe = drawer->current_frame;
  VqaCodebook* codebook = curframe->codebook;

  /* Decompress the codebook, if needed */
  if (codebook->flags & kCodebookCompressed) {
    /* Decompress the codebook. */
    LCW_Uncompress(std::span(codebook->buffer)
                       .subspan(base::ToSize(codebook->compressed_offset)),
                   codebook->buffer);

    /* Mark as uncompressed for the next time we use it */
    codebook->flags &= ~kCodebookCompressed;
  }

  /* Decompress the palette, if needed */
  if (curframe->flags & kFramePaletteCompressed) {
    curframe->palette_bytes =
        LCW_Uncompress(std::span(curframe->palette)
                           .subspan(base::ToSize(curframe->palette_offset)),
                       curframe->palette);

    /* Mark as uncompressed */
    curframe->flags &= ~kFramePaletteCompressed;
  }

  /* Decompress the pointer data, if needed */
  if (curframe->flags & kFramePointersCompressed) {
    LCW_Uncompress(std::span(curframe->pointers)
                       .subspan(base::ToSize(curframe->pointers_offset)),
                   curframe->pointers);

    /* Mark as uncompressed */
    curframe->flags &= ~kFramePointersCompressed;
  }
}

int32_t DrawNextFrame(VqaPlayerState* vqa) {
  auto* vqa_handle_p = vqa;
  /* Dereference data members for quicker access. */
  const VqaConfig* config = &vqa_handle_p->config;
  VqaMovie* vqabuf = vqa_handle_p->movie.get();
  VqaDrawer* drawer = &vqabuf->drawer;

  /* Check our "sleep" state */
  if (!(vqabuf->flags & kMovieDrawerAsleep)) {
    /* Find the frame to draw */
    if (const auto result = Select_Frame(vqa_handle_p); result != 0) {
      return result;
    }

    /* Uncompress the frame data */
    Prepare_Frame(vqabuf);
  }

  /* Wait for Update_Enabled to be set low */
  if (vqabuf->flags & kMovieAwaitingRelease) {
    vqabuf->flags |= kMovieDrawerAsleep;
    return kVqaSleeping;
  }

  vqabuf->flags &= ~kMovieDrawerAsleep;

  /* Dereference current frame for quicker access. */
  VqaFrame* curframe = drawer->current_frame;

  if (drawer->image_offset < 0 ||
      base::ToSize(drawer->image_offset) > drawer->image_buffer.size()) {
    return kVqaNoBuffer;
  }
  const auto buff =
      drawer->image_buffer.subspan(base::ToSize(drawer->image_offset));

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

  /* Un-VQ the image */
  vqabuf->decode_frame(curframe->codebook->buffer, curframe->pointers, buff,
                       drawer->blocks_per_row, drawer->block_rows,
                       drawer->image_width);

  /* Remember the last frame drawn, for status reporting. */
  drawer->last_drawn_frame = curframe->frame_number;

  /* Tell the flipper which frame to use */
  vqabuf->flipper.drawn_frame = curframe;

  /* Set the page-avail flag for the flipper */
  vqabuf->flags |= kMovieAwaitingRelease;

  /* Invoke user's callback routine */
  if ((config->frame_callback != nullptr) &&
      (config->frame_callback(drawer->image_buffer.data(),
                              curframe->frame_number) != 0)) {
    return kVqaEndOfMovie;
  }

  /* Move to the next frame */
  drawer->current_frame = curframe->next;

  return 0;
}

/****************************************************************************
 *
 * NAME
 *     UnVQ_Nop - Do nothing decode_frame.
 *
 * SYNOPSIS
 *     UnVQ_Nop(Codebook, Pointers, Buffer, BPR, Rows, BufWidth)
 *
 *     void UnVQ_Nop(unsigned char *, unsigned char *, unsigned char *,
 *                   int, int, int);
 * FUNCTION
 *
 * INPUTS
 *     Codebook - Not used. (Prototype placeholder)
 *     Pointers - Not used. (Prototype placeholder)
 *     Buffer   - Not used. (Prototype placeholder)
 *     BPR      - Not used. (Prototype placeholder)
 *     Rows     - Not used. (Prototype placeholder)
 *     BufWidth - Not used. (Prototype placeholder)
 *
 * RESULT
 *     NONE
 *
 ****************************************************************************/

static void UnVQ_Nop(std::span<const unsigned char> /*codebook*/,
                     std::span<const unsigned char> /*pointers*/,
                     std::span<unsigned char> /*buffer*/, int /*blocksperrow*/,
                     int /*numrows*/, int /*bufwidth*/) {}
