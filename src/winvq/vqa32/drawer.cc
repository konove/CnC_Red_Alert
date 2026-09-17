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
 *     VQA_Configure_Drawer - Configure the drawer routines.
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
 *     DrawFrame_Buffer         - Draw a frame to a buffer.
 *     PageFlip_Nop             - Do nothing page flip.
 *     UnVQ_Nop                 - Do nothing UnVQ.
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
#include "winvq/vqa32/vqafile.h"
#include "winvq/vqa32/vqaplay.h"
#include "winvq/vqa32/vqaplayp.h"
#include "winvq/vqm32/compress.h"

/*---------------------------------------------------------------------------
 * PRIVATE DECLARATIONS
 *-------------------------------------------------------------------------*/
static int32_t Select_Frame(VQAHandle* vqap);
static void Prepare_Frame(VQAData* vqabuf);

static int32_t DrawFrame_Buffer(VQAHandle* vqa);

static void __cdecl UnVQ_Nop(std::span<const unsigned char> codebook,
                             std::span<const unsigned char> pointers,
                             std::span<unsigned char> buffer, int blocksperrow,
                             int numrows, int bufwidth);

/****************************************************************************
 *
 * NAME
 *     VQA_Configure_Drawer - Configure the drawer routines.
 *
 * SYNOPSIS
 *     VQA_Configure_Drawer(VQA)
 *
 *     void VQA_Configure_Drawer(VQAHandle *);
 *
 * FUNCTION
 *     Configure the drawing system for the current movie and configuration
 *     options.
 *
 * INPUTS
 *     VQA - Pointer to private VQAHandle.
 *
 * RESULT
 *     NONE
 *
 ****************************************************************************/

void VQA_Configure_Drawer(VQAHandle* vqap) {

  /* Dereference commonly used data members for quicker access. */
  VQAData* vqabuf = vqap->data;
  VQADrawer* drawer = &vqabuf->Drawer;
  VQAHeader* header = &vqap->header;
  VQAConfig* config = &vqap->config;
  const uint32_t origin = config->DrawFlags & VQACFGF_ORIGIN;

  /*-------------------------------------------------------------------------
   * SET THE DRAW POSITION OF THE MOVIE.
   *
   * X1 = -1 -- Center image of the X axis, otherwise use X1 value.
   * Y1 = -1 -- Center image of the Y axis, otherwise use Y1 value.
   *-----------------------------------------------------------------------*/
  if (config->X1 == -1 && config->Y1 == -1) {
    drawer->X1 = (drawer->ImageWidth - header->ImageWidth) / 2;
    drawer->Y1 = (drawer->ImageHeight - header->ImageHeight) / 2;
    drawer->X2 = drawer->X1 + header->ImageWidth - 1;
    drawer->Y2 = drawer->Y1 + header->ImageHeight - 1;
  } else {
    // config->X1/Y1 is the gap between the image and the buffer corner the
    // origin names, mirroring the top-left case: a zero gap puts the image
    // flush in that corner. X1,Y1 is the image pixel nearest that corner and
    // X2,Y2 the opposite pixel, both inclusive.
    const bool right =
        origin == VQACFGF_TOPRIGHT || origin == VQACFGF_BOTRIGHT;
    const bool bottom =
        origin == VQACFGF_BOTLEFT || origin == VQACFGF_BOTRIGHT;

    if (right) {
      drawer->X1 = drawer->ImageWidth - 1 - config->X1;
      drawer->X2 = drawer->X1 - header->ImageWidth + 1;
    } else {
      drawer->X1 = config->X1;
      drawer->X2 = drawer->X1 + header->ImageWidth - 1;
    }

    if (bottom) {
      drawer->Y1 = drawer->ImageHeight - 1 - config->Y1;
      drawer->Y2 = drawer->Y1 - header->ImageHeight + 1;
    } else {
      drawer->Y1 = config->Y1;
      drawer->Y2 = drawer->Y1 + header->ImageHeight - 1;
    }
  }

  // The placement comes from the caller's config, not the file, so an image
  // that does not fit the buffer is a programmer error. Unchecked, UnVQ would
  // write outside the buffer from ScreenOffset.
  DCHECK(std::min(drawer->X1, drawer->X2) >= 0 &&
         std::max(drawer->X1, drawer->X2) < drawer->ImageWidth &&
         std::min(drawer->Y1, drawer->Y2) >= 0 &&
         std::max(drawer->Y1, drawer->Y2) < drawer->ImageHeight);

  /*-------------------------------------------------------------------------
   * INITIALIZE THE UNVQ ROUTINE FOR THE SPECIFIED VIDEO MODE AND BLOCK SIZE.
   *-----------------------------------------------------------------------*/

  /* Pre-compute commonly used values for speed. */
  drawer->BlocksPerRow = header->ImageWidth / header->BlockWidth;
  drawer->NumRows = header->ImageHeight / header->BlockHeight;
  drawer->NumBlocks = drawer->BlocksPerRow * drawer->NumRows;
  const uint32_t blkdim = BLOCK_DIM(header->BlockWidth, header->BlockHeight);

  /* Initialize draw routine vectors to a NOP routine in order to prevent
   * a crash.
   */
  vqabuf->UnVQ = UnVQ_Nop;

  /* If the client specifies buffering then go ahead an set the unvq
   * vector. All of the buffered modes use the same unvq routines.
   */
  if (config->DrawFlags & VQACFGF_BUFFER) {
    switch (blkdim) {
      case BLOCK_4X2:
        vqabuf->UnVQ = UnVQ_4x2;
        break;
      case BLOCK_4X4:
        vqabuf->UnVQ = UnVQ_4x4;
        break;
      default:
        break;
    }
  }

  /* Initialize the draw vectors for the specified video mode. */
  /* Purely buffered (Video refresh is up to the client. */
  {
    vqabuf->Draw_Frame = DrawFrame_Buffer;

    // Pre-compute the draw offset for speed. UnVQ fills rightward and
    // downward, so it starts at the image's top-left pixel whichever corner
    // is anchored.
    drawer->ScreenOffset =
        (drawer->ImageWidth * std::min(drawer->Y1, drawer->Y2)) +
        std::min(drawer->X1, drawer->X2);
  }
}

/****************************************************************************
 *
 * NAME
 *     Select_Frame - Selects frame to draw and preforms frame skip.
 *
 * SYNOPSIS
 *     Error = Select_Frame(VQA)
 *
 *     long Select_Frame(VQAHandle *);
 *
 * FUNCTION
 *     Select a frame to draw. This is were the frame skipping/delay is
 *     performed.
 *
 * INPUTS
 *     VQA - Pointer to private VQAHandle.
 *
 * RESULT
 *     Error - 0 if successful, or VQAERR_??? error code.
 *
 ****************************************************************************/

static int32_t Select_Frame(VQAHandle* vqap) {

  /* Dereference commonly used data members for quicker access. */
  VQAConfig* config = &vqap->config;
  VQAData* vqabuf = vqap->data;
  VQADrawer* drawer = &vqabuf->Drawer;
  VQAFrameNode* curframe = drawer->CurFrame;

  /* Make sure the current frame is drawable. If the frame is not ready
   * then we must wait for the loader to catch up.
   */
  if ((curframe->Flags & VQAFRMF_LOADED) == 0) {
    drawer->WaitsOnLoader++;
    return VQAERR_NOBUFFER;
  }

  /* If single stepping then return with the next frame.*/
  if (config->OptionFlags & VQAOPTF_STEP) {
    drawer->LastFrame = curframe->FrameNum;
    return 0;
  }

  /* Find the frame # we should play (rounded to nearest frame): */
  const int64_t curtime = VQA_GetTime(vqap);
  //	desiredframe = ((curtime * config->FrameRate) / VQA_TIMETICKS);
  // MEG MOD 06.22.95 - Should look for the desired frame to draw, not load,
  // right?
  const int64_t desiredframe = curtime * config->DrawRate / VQA_TIMETICKS;

  /* Handle the cases where the player is going so fast that it's not time
   * to draw this frame yet.
   *
   * - If the Drawer is using a slower frame rate than the Loader, use a
   *   delta-time-based wait; otherwise, use the frame number as the wait.
   */
  if (config->DrawRate != config->FrameRate) {
    if (curtime - drawer->LastTime < VQA_TIMETICKS / config->DrawRate) {
      return VQAERR_NOT_TIME;
    }
  } else {
    if (curframe->FrameNum > desiredframe) {
      return VQAERR_NOT_TIME;
    }
  }

  /* Make sure we draw at least 5 frames per second */
  if (curframe->FrameNum - drawer->LastFrame >= config->FrameRate / 5) {
    drawer->LastFrame = curframe->FrameNum;
    return 0;
  }

  /* If frame skipping is disabled then draw every frame. */
  if (config->DrawFlags & VQACFGF_NOSKIP) {
    drawer->LastFrame = curframe->FrameNum;
    return 0;
  }

  /* Handle the case where the player is going too slow, so we have to skip
   * some frames:
   *
   * - If this is a Key Frame, draw it
   * - If this frame's # is less than what we're supposed to draw, skip it
   *   (Because the 1st 'desiredframe' will be 0, FrameNum MUST be typecast
   *   to signed WORD for the comparison; otherwise, the comparison uses
   *   UWORDs, and the first frame is always skipped.)
   * - If this is a palette-set frame, set the palette before skipping it
   * - Loop until we get the frame we need, or there's no frames available
   */
  while (true) {
    /* No frame available; return */
    if ((curframe->Flags & VQAFRMF_LOADED) == 0) {
      return VQAERR_NOBUFFER;
    }

    /* Force drawing of a Key Frame */
    if (curframe->Flags & VQAFRMF_KEY) {
      break;
    }

    /* Skip the frame */
    if (curframe->FrameNum < desiredframe) {
      /* Handle a palette in a skipped frame:
       *
       * - Stash the palette in Drawer.Palette_24
       * - Set the Drawer.Flags VQADRWF_SETPAL bit, to tell the page-flip
       *   routines that this palette must be set
       */
      if (curframe->Flags & VQAFRMF_PALETTE) {
        /* Un-LCW if needed */
        if (curframe->Flags & VQAFRMF_PALCOMP) {
          curframe->PaletteSize =
              LCW_Uncompress(std::span(curframe->PaletteStorage)
                                 .subspan(base::ToSize(curframe->PalOffset)),
                             curframe->PaletteStorage);

          curframe->Flags &= ~VQAFRMF_PALCOMP;
        }

        // Stash the palette. A decompressed palette can report up to
        // Max_Pal_Size bytes, more than the 256-color copy holds.
        const int32_t stash_size = std::min(
            curframe->PaletteSize, int32_t{sizeof(drawer->Palette_24)});
        base::CopyBytes(base::ObjectBytes(drawer->Palette_24),
                        std::as_bytes(std::span(curframe->PaletteStorage)),
                        stash_size);
        drawer->CurPalSize = stash_size;
        drawer->Flags |= VQADRWF_SETPAL;
      }

      /* Invoke callback with nullptr screen ptr */
      if ((config->DrawerCallback != nullptr) &&
          (config->DrawerCallback(nullptr, curframe->FrameNum) != 0)) {
        return VQAERR_EOF;
      }

      /* Skip the frame */
      curframe->Flags = 0L;
      curframe = curframe->Next;
      drawer->CurFrame = curframe;
      drawer->NumSkipped++;
    } else {
      break;
    }
  }

  drawer->LastFrame = curframe->FrameNum;
  drawer->LastTime = curtime;

  return 0;
}

/****************************************************************************
 *
 * NAME
 *     Prepare_Frame - Process/Decompress frame information.
 *
 * SYNOPSIS
 *     Prepare_Frame(VQAData)
 *
 *     void Prepare_Frame(VQAData *);
 *
 * FUNCTION
 *     Decompress and preprocess the various frame elements (codebook,
 *     pointers, palette, etc...)
 *
 * INPUTS
 *     VQAData - Pointer to VQAData structure.
 *
 * RESULT
 *     NONE
 *
 ****************************************************************************/

static void Prepare_Frame(VQAData* vqabuf) {

  /* Dereference commonly used data members for quicker access. */
  VQADrawer* drawer = &vqabuf->Drawer;
  VQAFrameNode* curframe = drawer->CurFrame;
  VQACBNode* codebook = curframe->Codebook;

  /* Decompress the codebook, if needed */
  if (codebook->Flags & VQACBF_CBCOMP) {
    /* Decompress the codebook. */
    LCW_Uncompress(std::span(codebook->BufferStorage)
                       .subspan(base::ToSize(codebook->CBOffset)),
                   codebook->BufferStorage);

    /* Mark as uncompressed for the next time we use it */
    codebook->Flags &= ~VQACBF_CBCOMP;
  }

  /* Decompress the palette, if needed */
  if (curframe->Flags & VQAFRMF_PALCOMP) {
    curframe->PaletteSize =
        LCW_Uncompress(std::span(curframe->PaletteStorage)
                           .subspan(base::ToSize(curframe->PalOffset)),
                       curframe->PaletteStorage);

    /* Mark as uncompressed */
    curframe->Flags &= ~VQAFRMF_PALCOMP;
  }

  /* Decompress the pointer data, if needed */
  if (curframe->Flags & VQAFRMF_PTRCOMP) {
    LCW_Uncompress(std::span(curframe->PointersStorage)
                       .subspan(base::ToSize(curframe->PtrOffset)),
                   curframe->PointersStorage);

    /* Mark as uncompressed */
    curframe->Flags &= ~VQAFRMF_PTRCOMP;
  }
}

/****************************************************************************
 *
 * NAME
 *     DrawFrame_Buffer - Draw a frame to a buffer.
 *
 * SYNOPSIS
 *     Error = DrawFrame_Buffer(VQA)
 *
 *     long DrawFrame_Buffere(VQAHandle *);
 *
 * FUNCTION
 *
 * INPUTS
 *     VQA - Pointer to VQA handle.
 *
 * RESULT
 *     Error - 0 if successful, otherwise VQAERR_??? error code.
 *
 ****************************************************************************/

extern void __cdecl Set_Palette(void* palette);
static int32_t DrawFrame_Buffer(VQAHandle* vqa) {

  auto* vqa_handle_p = vqa;
  /* Dereference data members for quicker access. */
  const VQAConfig* config = &vqa_handle_p->config;
  VQAData* vqabuf = vqa_handle_p->data;
  VQADrawer* drawer = &vqabuf->Drawer;

  /* Check our "sleep" state */
  if (!(vqabuf->Flags & VQADATF_DSLEEP)) {
    /* Find the frame to draw */
    if (const auto result = Select_Frame(vqa_handle_p); result != 0) {
      return result;
    }

    /* Uncompress the frame data */
    Prepare_Frame(vqabuf);
  }

  /* Wait for Update_Enabled to be set low */
  if (vqabuf->Flags & VQADATF_UPDATE) {
    vqabuf->Flags |= VQADATF_DSLEEP;
    return VQAERR_SLEEPING;
  }

  if (vqabuf->Flags & VQADATF_DSLEEP) {
    drawer->WaitsOnFlipper++;
    vqabuf->Flags &= ~VQADATF_DSLEEP;
  }

  /* Dereference current frame for quicker access. */
  VQAFrameNode* curframe = drawer->CurFrame;

  if (drawer->ScreenOffset < 0 ||
      base::ToSize(drawer->ScreenOffset) > drawer->ImageBuf.size()) {
    return VQAERR_NOBUFFER;
  }
  const auto buff =
      drawer->ImageBuf.subspan(base::ToSize(drawer->ScreenOffset));

  const auto pal = std::span(curframe->PaletteStorage);
  const int32_t palsize = curframe->PaletteSize;
  const uint32_t slowpal =
      (config->OptionFlags & VQAOPTF_SLOWPAL) != 0 ? 1U : 0U;

  /* Set the palette if necessary */
  if (curframe->Flags & VQAFRMF_PALETTE || drawer->Flags & VQADRWF_SETPAL) {
    Flag_To_Set_Palette(pal, palsize, slowpal);
    curframe->Flags &= ~VQAFRMF_PALETTE;
    drawer->Flags &= ~VQADRWF_SETPAL;
  }

  /* Un-VQ the image */
  vqabuf->UnVQ(curframe->Codebook->BufferStorage, curframe->PointersStorage,
               buff, drawer->BlocksPerRow, drawer->NumRows, drawer->ImageWidth);

  /* Remember the last frame drawn, for status reporting. */
  drawer->LastFrameNum = curframe->FrameNum;

  /* Tell the flipper which frame to use */
  vqabuf->Flipper.CurFrame = curframe;

  /* Set the page-avail flag for the flipper */
  vqabuf->Flags |= VQADATF_UPDATE;

  /* Invoke user's callback routine */
  if ((config->DrawerCallback != nullptr) &&
      (config->DrawerCallback(drawer->ImageBuf.data(), curframe->FrameNum) !=
       0)) {
    return VQAERR_EOF;
  }

  /* Move to the next frame */
  drawer->CurFrame = curframe->Next;

  return 0;
}

/****************************************************************************
 *
 * NAME
 *     UnVQ_Nop - Do nothing UnVQ.
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
