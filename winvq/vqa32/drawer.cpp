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

#include <cstdio>
#include <cstring>
#include "vqa32/unvq.h"
#include "vqa32/vqafile.h"
#include "vqa32/vqaplay.h"
#include "vqa32/vqaplayp.h"
#include "vqm32/compress.h"

/*---------------------------------------------------------------------------
 * PRIVATE DECLARATIONS
 *-------------------------------------------------------------------------*/
static long Select_Frame(VQAHandleP *vqap);
static void Prepare_Frame(VQAData *vqabuf);

static long DrawFrame_Buffer(VQAHandle *vqa);

static void __cdecl UnVQ_Nop(unsigned char *codebook, unsigned char *pointers,
                             unsigned char *buffer, unsigned long blocksperrow,
                             unsigned long numrows, unsigned long bufwidth);

/****************************************************************************
 *
 * NAME
 *     VQA_GetPalette - Get the palette used in the movie.
 *
 * SYNOPSIS
 *     Palette = VQA_GetPalette(VQA)
 *
 *     char *VQA_GetPalette(VQAHandle *);
 *
 * FUNCTION
 *     Retrieve the address of the current palette used in the movie. If there
 *     isn't a palette available then a NULL value will be returned.
 *
 * INPUTS
 *     VQA - Pointer to VQAHandle to get palette for.
 *
 * RESULT
 *     Palette - Pointer to palette or nullptr if no palette available.
 *
 ****************************************************************************/

unsigned char *VQA_GetPalette(VQAHandle *vqa) {
  VQADrawer *drawer;
  unsigned char *palette = nullptr;

  /* Dereference commonly used data members for quick access. */
  drawer = &((VQAHandleP *)vqa)->VQABuf->Drawer;

  if (drawer->CurPalSize > 0) {
    palette = drawer->Palette_24;
  }

  return (palette);
}

/****************************************************************************
 *
 * NAME
 *     VQA_GetPaletteSize - Get the size of the palette used in the movie.
 *
 * SYNOPSIS
 *     PalSize = VQA_GetPaletteSize(VQA)
 *
 *     long VQA_GetPaletteSize(VQAHandle *);
 *
 * FUNCTION
 *     Retrieve the size of the current palette used in the movie. If there
 *     isn't a palette available then a zero size will be returned.
 *
 * INPUTS
 *     VQA - Pointer to VQAHandle to get palette for.
 *
 * RESULT
 *     PalSize - Size in bytes of the current palette.
 *
 ****************************************************************************/

long VQA_GetPaletteSize(VQAHandle *vqa) {
  VQADrawer *drawer;

  /* Dereference commonly used data members for quick access. */
  drawer = &((VQAHandleP *)vqa)->VQABuf->Drawer;

  return (drawer->CurPalSize);
}

/****************************************************************************
 *
 * NAME
 *     VQA_Set_DrawBuffer - Set the buffer to draw the images to.
 *
 * SYNOPSIS
 *     VQA_Set_DrawBuffer(VQA, Buffer, Width, Height, XPos, YPos)
 *
 *     void VQA_Set_DrawBuffer(VQAHandle *, unsigned char *,
 *                             unsigned long, unsigned long, unsigned long,
 *                             unsigned long);
 *
 * FUNCTION
 *     Set the draw buffer to the buffer provided by the client.
 *
 * INPUTS
 *     VQA    - Pointer to VQAHandle to set buffer for.
 *     Buffer - Pointer to new image buffer.
 *     Width  - Width of the buffer in pixels.
 *     Height - Height of the buffer in pixels.
 *     XPos   - X pixel position in buffer to draw image.
 *     YPos   - Y pixel position in buffer to draw image.
 *
 * RESULT
 *     NONE
 *
 ****************************************************************************/

void VQA_Set_DrawBuffer(VQAHandle *vqa, unsigned char *buffer,
                        unsigned long width, unsigned long height, long xpos,
                        long ypos) {
  VQAHeader *header;
  VQADrawer *drawer;
  VQAConfig *config;
  long origin;

  /* Dereference commonly used data members for quick access. */
  header = &((VQAHandleP *)vqa)->Header;
  drawer = &((VQAHandleP *)vqa)->VQABuf->Drawer;
  config = &((VQAHandleP *)vqa)->Config;
  origin = (config->DrawFlags & VQACFGF_ORIGIN);

  /* Set the drawer buffer information. */
  drawer->ImageBuf = buffer;
  drawer->ImageWidth = width;
  drawer->ImageHeight = height;

  /*-------------------------------------------------------------------------
   * SET THE DRAW POSITION OF THE MOVIE.
   *
   * X1 = -1 -- Center image of the X axis, otherwise use X1 value.
   * Y1 = -1 -- Center image of the Y axis, otherwise use Y1 value.
   *-----------------------------------------------------------------------*/
  if ((xpos == -1) && (ypos == -1)) {
    drawer->X1 = ((width - header->ImageWidth) / 2);
    drawer->Y1 = ((height - header->ImageHeight) / 2);
    drawer->X2 = ((drawer->X1 + header->ImageWidth) - 1);
    drawer->Y2 = ((drawer->Y1 + header->ImageHeight) - 1);
  } else {
    switch (origin) {
      default:
      case VQACFGF_TOPLEFT:
        drawer->X1 = xpos;
        drawer->Y1 = ypos;
        drawer->X2 = ((drawer->X1 + header->ImageWidth) - 1);
        drawer->Y2 = ((drawer->Y1 + header->ImageHeight) - 1);
        break;

      case VQACFGF_BOTLEFT:
        drawer->X1 = xpos;
        drawer->Y1 = (height - ypos);
        drawer->X2 = ((drawer->X1 + header->ImageWidth) - 1);
        drawer->Y2 = ((drawer->Y2 - header->ImageHeight) - 1);
        break;

      case VQACFGF_BOTRIGHT:
        drawer->X1 = (width - xpos);
        drawer->Y1 = (height - ypos);
        drawer->X2 = (drawer->X1 - header->ImageWidth);
        drawer->Y2 = (drawer->Y1 - header->ImageHeight);
        break;
    }
  }

  /* Pre-compute the draw offset for speed. */
  drawer->ScreenOffset = ((width * drawer->Y1) + drawer->X1);
}

/****************************************************************************
 *
 * NAME
 *     VQA_Configure_Drawer - Configure the drawer routines.
 *
 * SYNOPSIS
 *     VQA_Configure_Drawer(VQA)
 *
 *     void VQA_Configure_Drawer(VQAHandleP *);
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

void VQA_Configure_Drawer(VQAHandleP *vqap) {
  VQAData *vqabuf;
  VQAConfig *config;
  VQAHeader *header;
  VQADrawer *drawer;
  long origin;
  long blkdim;

  /* Dereference commonly used data members for quicker access. */
  vqabuf = vqap->VQABuf;
  drawer = &vqabuf->Drawer;
  header = &vqap->Header;
  config = &vqap->Config;
  origin = (config->DrawFlags & VQACFGF_ORIGIN);

  /*-------------------------------------------------------------------------
   * SET THE DRAW POSITION OF THE MOVIE.
   *
   * X1 = -1 -- Center image of the X axis, otherwise use X1 value.
   * Y1 = -1 -- Center image of the Y axis, otherwise use Y1 value.
   *-----------------------------------------------------------------------*/
  if ((config->X1 == -1) && (config->Y1 == -1)) {
    drawer->X1 = ((drawer->ImageWidth - header->ImageWidth) / 2);
    drawer->Y1 = ((drawer->ImageHeight - header->ImageHeight) / 2);
    drawer->X2 = ((drawer->X1 + header->ImageWidth) - 1);
    drawer->Y2 = ((drawer->Y1 + header->ImageHeight) - 1);
  } else {
    switch (origin) {
      default:
      case VQACFGF_TOPLEFT:
        drawer->X1 = config->X1;
        drawer->Y1 = config->Y1;
        drawer->X2 = ((drawer->X1 + header->ImageWidth) - 1);
        drawer->Y2 = ((drawer->Y1 + header->ImageHeight) - 1);
        break;

      case VQACFGF_BOTLEFT:
        drawer->X1 = config->X1;
        drawer->Y1 = (drawer->ImageHeight - config->Y1);
        drawer->X2 = ((drawer->X1 + header->ImageWidth) - 1);
        drawer->Y2 = ((drawer->Y2 - header->ImageHeight) - 1);
        break;

      case VQACFGF_BOTRIGHT:
        drawer->X1 = (drawer->ImageWidth - config->X1);
        drawer->Y1 = (drawer->ImageHeight - config->Y1);
        drawer->X2 = (drawer->X1 - header->ImageWidth);
        drawer->Y2 = (drawer->Y1 - header->ImageHeight);
        break;
    }
  }

  /*-------------------------------------------------------------------------
   * INITIALIZE THE UNVQ ROUTINE FOR THE SPECIFIED VIDEO MODE AND BLOCK SIZE.
   *-----------------------------------------------------------------------*/

  /* Pre-compute commonly used values for speed. */
  drawer->BlocksPerRow = header->ImageWidth / header->BlockWidth;
  drawer->NumRows = header->ImageHeight / header->BlockHeight;
  drawer->NumBlocks = drawer->BlocksPerRow * drawer->NumRows;
  blkdim = BLOCK_DIM(header->BlockWidth, header->BlockHeight);

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
  switch (config->Vmode) {
    /* Purely buffered (Video refresh is up to the client. */
    default:
      vqabuf->Draw_Frame = DrawFrame_Buffer;

      /* Pre-compute the draw offset for speed. */
      drawer->ScreenOffset = ((drawer->ImageWidth * drawer->Y1) + drawer->X1);
      break;
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
 *     long Select_Frame(VQAHandleP *);
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

static long Select_Frame(VQAHandleP *vqap) {
  VQAData *vqabuf;
  VQADrawer *drawer;
  VQAConfig *config;
  VQAFrameNode *curframe;
  long desiredframe;
  // MEG 11.29.95 - changed from long to unsigned long
  unsigned long curtime;

  /* Dereference commonly used data members for quicker access. */
  config = &vqap->Config;
  vqabuf = vqap->VQABuf;
  drawer = &vqabuf->Drawer;
  curframe = drawer->CurFrame;

  /* Make sure the current frame is drawable. If the frame is not ready
   * then we must wait for the loader to catch up.
   */
  if ((curframe->Flags & VQAFRMF_LOADED) == 0) {
    drawer->WaitsOnLoader++;
    return (VQAERR_NOBUFFER);
  }

  /* If single stepping then return with the next frame.*/
  if (config->OptionFlags & VQAOPTF_STEP) {
    drawer->LastFrame = curframe->FrameNum;
    return (0);
  }

  /* Find the frame # we should play (rounded to nearest frame): */
  curtime = VQA_GetTime(vqap);
  //	desiredframe = ((curtime * config->FrameRate) / VQA_TIMETICKS);
  // MEG MOD 06.22.95 - Should look for the desired frame to draw, not load,
  // right?
  desiredframe = ((curtime * config->DrawRate) / VQA_TIMETICKS);

  /* Handle the cases where the player is going so fast that it's not time
   * to draw this frame yet.
   *
   * - If the Drawer is using a slower frame rate than the Loader, use a
   *   delta-time-based wait; otherwise, use the frame number as the wait.
   */
  if (config->DrawRate != config->FrameRate) {
    if (curtime - drawer->LastTime < (VQA_TIMETICKS / config->DrawRate)) {
      return (VQAERR_NOT_TIME);
    }
  } else {
    if (curframe->FrameNum > desiredframe) {
      return (VQAERR_NOT_TIME);
    }
  }

  /* Make sure we draw at least 5 frames per second */
  if ((curframe->FrameNum - drawer->LastFrame) >= (config->FrameRate / 5)) {
    drawer->LastFrame = curframe->FrameNum;
    return (0);
  }

  /* If frame skipping is disabled then draw every frame. */
  if (config->DrawFlags & VQACFGF_NOSKIP) {
    drawer->LastFrame = curframe->FrameNum;
    return (0);
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
  while (1) {
    /* No frame available; return */
    if ((curframe->Flags & VQAFRMF_LOADED) == 0) {
      return (VQAERR_NOBUFFER);
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
              LCW_Uncompress((char *)curframe->Palette + curframe->PalOffset,
                             (char *)curframe->Palette, vqabuf->Max_Pal_Size);

          curframe->Flags &= ~VQAFRMF_PALCOMP;
        }

        /* Stash the palette */
        memcpy(drawer->Palette_24, curframe->Palette, curframe->PaletteSize);
        drawer->CurPalSize = curframe->PaletteSize;
        drawer->Flags |= VQADRWF_SETPAL;
      }

      /* Invoke callback with nullptr screen ptr */
      if (config->DrawerCallback != nullptr) {
        if ((config->DrawerCallback(nullptr, curframe->FrameNum)) != 0) {
          return (VQAERR_EOF);
        }
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

  return (0);
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

static void Prepare_Frame(VQAData *vqabuf) {
  VQADrawer *drawer;
  VQAFrameNode *curframe;
  VQACBNode *codebook;

  /* Dereference commonly used data members for quicker access. */
  drawer = &vqabuf->Drawer;
  curframe = drawer->CurFrame;
  codebook = curframe->Codebook;

  /* Decompress the codebook, if needed */
  if (codebook->Flags & VQACBF_CBCOMP) {
    /* Decompress the codebook. */
    LCW_Uncompress((char *)codebook->Buffer + codebook->CBOffset,
                   (char *)codebook->Buffer, vqabuf->Max_CB_Size);

    /* Mark as uncompressed for the next time we use it */
    codebook->Flags &= (~VQACBF_CBCOMP);
  }

  /* Decompress the palette, if needed */
  if (curframe->Flags & VQAFRMF_PALCOMP) {
    curframe->PaletteSize =
        LCW_Uncompress((char *)curframe->Palette + curframe->PalOffset,
                       (char *)curframe->Palette, vqabuf->Max_Pal_Size);

    /* Mark as uncompressed */
    curframe->Flags &= ~VQAFRMF_PALCOMP;
  }

  /* Decompress the pointer data, if needed */
  if (curframe->Flags & VQAFRMF_PTRCOMP) {
    LCW_Uncompress((char *)curframe->Pointers + curframe->PtrOffset,
                   (char *)curframe->Pointers, vqabuf->Max_Ptr_Size);

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

extern void __cdecl Set_Palette(void *palette);
extern void Flag_To_Set_Palette(unsigned char *palette, long numbytes,
                                unsigned long slowpal);
static long DrawFrame_Buffer(VQAHandle *vqa) {
  VQAData *vqabuf;
  VQADrawer *drawer;
  VQAFrameNode *curframe;
  VQAConfig *config;
  long rc;
  unsigned char *pal;
  long palsize;
  long slowpal;
  unsigned char *buff;

  /* Dereference data members for quicker access. */
  config = &((VQAHandleP *)vqa)->Config;
  vqabuf = ((VQAHandleP *)vqa)->VQABuf;
  drawer = &vqabuf->Drawer;

  /* Check our "sleep" state */
  if (!(vqabuf->Flags & VQADATF_DSLEEP)) {
    /* Find the frame to draw */
    if ((rc = Select_Frame((VQAHandleP *)vqa)) != 0) {
      return (rc);
    }

    /* Uncompress the frame data */
    Prepare_Frame(vqabuf);
  }

  /* Wait for Update_Enabled to be set low */
  if (vqabuf->Flags & VQADATF_UPDATE) {
    vqabuf->Flags |= VQADATF_DSLEEP;
    return (VQAERR_SLEEPING);
  }

  if (vqabuf->Flags & VQADATF_DSLEEP) {
    drawer->WaitsOnFlipper++;
    vqabuf->Flags &= (~VQADATF_DSLEEP);
  }

  /* Dereference current frame for quicker access. */
  curframe = drawer->CurFrame;

  buff = (unsigned char *)(drawer->ImageBuf + drawer->ScreenOffset);

  pal = curframe->Palette;
  palsize = curframe->PaletteSize;
  slowpal = (config->OptionFlags & VQAOPTF_SLOWPAL) ? 1 : 0;

  /* Set the palette if neccessary */
  if ((curframe->Flags & VQAFRMF_PALETTE) || (drawer->Flags & VQADRWF_SETPAL)) {
    Flag_To_Set_Palette(pal, palsize, slowpal);
    curframe->Flags &= ~VQAFRMF_PALETTE;
    drawer->Flags &= ~VQADRWF_SETPAL;
  }

  /* Un-VQ the image */
  vqabuf->UnVQ(curframe->Codebook->Buffer, curframe->Pointers, buff,
               drawer->BlocksPerRow, drawer->NumRows, drawer->ImageWidth);

  /* Update data for mono output */
  drawer->LastFrameNum = curframe->FrameNum;

  /* Tell the flipper which frame to use */
  vqabuf->Flipper.CurFrame = curframe;

  /* Set the page-avail flag for the flipper */
  vqabuf->Flags |= VQADATF_UPDATE;

  /* Invoke user's callback routine */
  if (config->DrawerCallback != nullptr) {
    if ((config->DrawerCallback(drawer->ImageBuf, curframe->FrameNum)) != 0) {
      return (VQAERR_EOF);
    }
  }

  /* Move to the next frame */
  drawer->CurFrame = curframe->Next;

  return (0);
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
 *                   unsigned long, unsigned long, unsigned long);
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

static void UnVQ_Nop(unsigned char * /*codebook*/, unsigned char * /*pointers*/,
                     unsigned char * /*buffer*/, unsigned long /*blocksperrow*/,
                     unsigned long /*numrows*/, unsigned long /*bufwidth*/) {}
