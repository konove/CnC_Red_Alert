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
 *     VQA player library. (32-Bit protected mode)
 *
 * FILE
 *     loader.c
 *
 * DESCRIPTION
 *     Stream loading and pre-processing.
 *
 * PROGRAMMER
 *     Bill Randolph
 *     Denzil E. Long, Jr.
 *
 * DATE
 *     August 21, 1995
 *
 *----------------------------------------------------------------------------
 *
 * PUBLIC
 *     VQA_Open      - Open a VQA file to play.
 *     VQA_Close     - Close an opened VQA file.
 *     VQA_LoadFrame - Load the next video frame from the VQA data stream.
 *     VQA_SeekFrame - Position the movie stream to the specified frame.
 *
 * PRIVATE
 *     AllocBuffers  - Allocates the numerous VQA play buffers
 *     FreeBuffers   - Frees the VQA play buffers
 *     PrimeBuffers  - Pre-Load the internal buffers.
 *     Load_FINF     - Loads the Frame Info Table.
 *     Load_CBF0     - Loads a full, uncompressed codebook
 *     Load_CBFZ     - Loads a full, compressed codebook
 *     Load_CBP0     - Loads a partial uncompressed codebook
 *     Load_CBPZ     - Loads a partial compressed codebook
 *     Load_CPL0     - Loads an uncompressed palette
 *     Load_CPLZ     - Loads a compressed palette
 *     Load_VPT0     - Loads uncompressed pointers
 *     Load_VPTZ     - Loads compressed pointers
 *     Load_VQF      - Loads a VQ Frame chunk
 *     Load_SND0     - Loads an uncompressed sound chunk
 *     Load_SND1     - Loads a compressed sound chunk
 *     Load_AudFrame - Loads blocks from separate audio file, if needed.
 *
 ****************************************************************************/

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <utility>
#include <vector>

#include "base/numeric.h"
#include "sdllib/ww_win.h"
#include "winvq/vqa32/vqafile.h"
#include "winvq/vqa32/vqaplay.h"
#include "winvq/vqa32/vqaplayp.h"
#include "winvq/vqm32/compress.h"
#include "winvq/vqm32/iff.h"
#include "winvq/vqm32/palette.h"
#include "winvq/vqm32/soscomp.h"

/*---------------------------------------------------------------------------
 * PRIVATE DECLARATIONS
 *-------------------------------------------------------------------------*/

static VQAData* AllocBuffers(VQAHeader* header, VQAConfig* config);
static void FreeBuffers(VQAData* vqa, VQAConfig* config, VQAHeader* header);
static long PrimeBuffers(VQAHandle* vqa);
static long Load_VQF(VQAHandle* vqap, int32_t iffsize);
static long Load_FINF(VQAHandle* vqap, int32_t iffsize);
static long Load_CBF0(VQAHandle* vqap, int32_t iffsize);
static long Load_CBFZ(VQAHandle* vqap, int32_t iffsize);
static long Load_CBP0(VQAHandle* vqap, int32_t iffsize);
static long Load_CBPZ(VQAHandle* vqap, int32_t iffsize);
static long Load_CPL0(VQAHandle* vqap, int32_t iffsize);
static long Load_CPLZ(VQAHandle* vqap, int32_t iffsize);
static long Load_VPT0(VQAHandle* vqap, int32_t iffsize);
static long Load_VPTZ(VQAHandle* vqap, int32_t iffsize);
static long Load_SND0(VQAHandle* vqap, int32_t iffsize);
static long Load_SND1(VQAHandle* vqap, int32_t iffsize);
static long Load_SND2(VQAHandle* vqap, int32_t iffsize);

extern "C" {
void __cdecl Force_VM_Page_In(void* buffer, int length);
}

// Returns the payload size of an IFF chunk, which the file stores big-endian.
// VQA chunks are far smaller than 2 GiB, so the size fits int32_t.
static int32_t ChunkSize(const ChunkHeader& chunk) {
  return static_cast<int32_t>(std::byteswap(chunk.size));
}

// Returns a chunk size rounded up to the even boundary IFF chunks are padded
// to. size must not be negative.
static constexpr int32_t PadSize(int32_t size) { return size + (size % 2); }

// Returns whether a size from ChunkSize() is usable. A file size of 2^31 or
// more reads back negative, and INT32_MAX would overflow PadSize(). Every
// chunk size must pass this before any other use.
static constexpr bool IsValidChunkSize(int32_t size) {
  return size >= 0 && size < INT32_MAX;
}

// Returns whether size bytes starting at offset lie inside a buffer of
// capacity bytes. Takes 64-bit values so callers can add offsets without
// overflowing.
static constexpr bool FitsInBuffer(int64_t offset, int64_t size,
                                   int64_t capacity) {
  return offset >= 0 && size >= 0 && offset + size <= capacity;
}

/****************************************************************************
 *
 * NAME
 *     VQA_Open - Open a VQA file to play.
 *
 * SYNOPSIS
 *     Error = VQA_Open(VQA, Name, Config)
 *
 *     long VQA_Open(VQAHandle *, char *, VQAConfig *);
 *
 * FUNCTION
 *     - Open a VQA file for reading.
 *     - Validate that it is an IFF file, of the VQA type.
 *     - Read the VQA header.
 *     - Open a VOC file for playback, if requested.
 *     - Set the Loader's frame rate, if the caller's Config structure's
 *       FrameRate is set to -1
 *     - Set the Drawer's frame rate, if the caller's Config structure's
 *       DrawRate is set to -1
 *
 * INPUTS
 *     VQA    - Pointer to initialized handle. Obtained by VQA_Alloc().
 *     Name   - Pointer to name of VQA file to open.
 *     Config - Pointer to initialized VQA configuration structure.
 *
 * RESULT
 *     Error - 0 if successful, or VQAERR_ error code.
 *
 ****************************************************************************/

long VQA_Open(VQAHandle* vqa, const char* filename, VQAConfig* config) {
  VQAHandle* vqap;
  VQAHeader* header;
  ChunkHeader chunk;
  long done;

  /* Dereference commonly used data members for quicker access. */
  vqap = vqa;
  header = &vqap->header;

  VQAMovieDone = 0;
  /*-------------------------------------------------------------------------
   * VERIFY VALIDITY OF VQA FILE.
   *-----------------------------------------------------------------------*/

  /* Open the file. */
  if (vqap->io->Open(filename)) {
    return VQAERR_OPEN;
  }

  /* Read the file ID & Size */
  if (vqap->io->Read(&chunk, 8)) {
    VQA_Close(vqa);
    return VQAERR_READ;
  }

  /* Verify an IFF FORM */
  if (chunk.id != ID_FORM || chunk.size == 0) {
    VQA_Close(vqa);
    return VQAERR_NOTVQA;
  }

  /* Read in WVQA ID */
  if (vqap->io->Read(&chunk, 4)) {
    VQA_Close(vqa);
    return VQAERR_READ;
  }

  /* Verify VQA */
  if (chunk.id != ID_WVQA) {
    VQA_Close(vqa);
    return VQAERR_NOTVQA;
  }

  /*-------------------------------------------------------------------------
   * INITIALIZE THE PLAYERS CONFIGURATION
   *-----------------------------------------------------------------------*/

  /* Use the clients configuration if they provided one. */
  if (config != nullptr) {
    memcpy(&vqap->config, config, sizeof(VQAConfig));
  } else {
    VQA_DefaultConfig(&vqap->config);
  }

  /* Use the internal configuration structure from now on. */
  config = &vqap->config;

  /*-------------------------------------------------------------------------
   * PROCESS THE PRE-FRAME CHUNKS (VQHD, CAP, FINF, ETC...)
   *-----------------------------------------------------------------------*/
  done = 0;

  while (!done) {
    if (vqap->io->Read(&chunk, 8)) {
      VQA_Close(vqa);
      return VQAERR_READ;
    }

    const int32_t chunk_size = ChunkSize(chunk);

    // A negative size would make the skip below seek backwards.
    if (!IsValidChunkSize(chunk_size)) {
      VQA_Close(vqa);
      return VQAERR_NOTVQA;
    }

    switch (chunk.id) {
      /*---------------------------------------------------------------------
       * READ IN THE VQA HEADER.
       *-------------------------------------------------------------------*/
      case ID_VQHD:
        // A second header would leak the first header's buffers.
        if (std::cmp_not_equal(chunk_size, sizeof(VQAHeader)) ||
            vqap->data != nullptr) {
          VQA_Close(vqa);
          return VQAERR_NOTVQA;
        }

        /* Read the header data. */
        if (vqap->io->Read(header, PadSize(chunk_size))) {
          VQA_Close(vqa);
          return VQAERR_READ;
        }

        // These fields are divisors when sizing buffers and timing playback.
        if (header->BlockWidth == 0 || header->BlockHeight == 0 ||
            header->Groupsize == 0 || header->FPS == 0) {
          VQA_Close(vqa);
          return VQAERR_NOTVQA;
        }

        /*-------------------------------------------------------------------
         * SETUP THE CONFIGURATION FROM THE HEADER.
         *-----------------------------------------------------------------*/
        if (config->ImageWidth == -1) {
          config->ImageWidth = header->ImageWidth;
        }

        if (config->ImageHeight == -1) {
          config->ImageHeight = header->ImageHeight;
        }

        /* If Loaders frame rate is -1 then use the value from the header. */
        if (config->FrameRate == -1) {
          config->FrameRate = header->FPS;
        }

        /* If Drawers frame rate is -1 then use the value from the header,
         * which will result in a "variable" frame rate.
         */
        if (config->DrawRate == -1) {
          config->DrawRate = header->FPS;
        }

        /* Finally, if the DrawRate was set to -1 or 0 (ie MaxRate contained
         * bogus values), set it to the header value.
         */
        if (config->DrawRate == -1 || config->DrawRate == 0) {
          config->DrawRate = header->FPS;
        }

        /* If an alternate audio track is not available then turn it off.
         * This enables the primary audio track to be played.
         */
        if (header->Version > VQAHD_VER1 &&
            !(header->Flags & VQAHDF_ALTAUDIO)) {
          config->OptionFlags &= ~VQAOPTF_ALTAUDIO;
        }

        /*-------------------------------------------------------------------
         * ALLOCATE THE BUFFERS THAT WE NEED TO PLAY THE VQA.
         *-----------------------------------------------------------------*/
        // The audio setup divides by the HMI buffer size.
        if ((header->Flags & VQAHDF_AUDIO) != 0 &&
            (config->OptionFlags & VQAOPTF_AUDIO) != 0 &&
            config->HMIBufSize <= 0) {
          VQA_Close(vqa);
          return VQAERR_AUDIO;
        }

        vqap->data = AllocBuffers(header, config);
        if (vqap->data == nullptr) {
          VQA_Close(vqa);
          return VQAERR_NOMEM;
        }

        break;

      /*---------------------------------------------------------------------
       * READ FRAME INFORMATION
       *-------------------------------------------------------------------*/
      case ID_FINF:
        // The frame table is sized from the header, so it must come first.
        if (vqap->data == nullptr) {
          VQA_Close(vqa);
          return VQAERR_NOTVQA;
        }

        if (Load_FINF(vqap, chunk_size)) {
          VQA_Close(vqa);
          return VQAERR_READ;
        }

        done = 1;
        break;

      default:
        if (vqap->io->Seek(PadSize(chunk_size), SEEK_CUR)) {
          VQA_Close(vqa);
          return VQAERR_SEEK;
        }
        break;
    }
  }

  /*-------------------------------------------------------------------------
   * INITIALIZE THE VIDEO SYSTEM IF WE ARE REQUIRED TO HANDLE THAT.
   *-----------------------------------------------------------------------*/
  vqap->data->VBIBit = config->VBIBit;

  /*-------------------------------------------------------------------------
   * AUDIO TRACK OVERRIDE FROM EXTERNAL FILE (.VOC)
   *-----------------------------------------------------------------------*/

  /* Open VOC file if one is requested. */

  /* If the movie does not contain an audio track make sure we won't try
   * to play one.
   */
  if ((header->Flags & VQAHDF_AUDIO) == 0) {
    config->OptionFlags &= ~VQAOPTF_AUDIO;
  }

  /*-------------------------------------------------------------------------
   * INITIALIZE THE AUDIO PLAYBACK/TIMING SYSTEM.
   *-----------------------------------------------------------------------*/
  if (config->OptionFlags & VQAOPTF_AUDIO) {
    VQAAudio* audio;

    /* Dereference for quick access. */
    audio = &vqap->data->Audio;

    /* Open HMI audio resource for playback. */
    if (VQA_OpenAudio(vqap, MainWindow)) {
      VQA_Close(vqa);
      return VQAERR_AUDIO;
    }

    /* Initialize ADPCM information structure for audio stream. */
    VQA_sosCODECInitStream(&audio->ADPCM_Info);

    if (header->Version == VQAHD_VER1) {
      audio->ADPCM_Info.bit_size = 8;
      audio->ADPCM_Info.uncomp_size = 22050L / header->FPS * header->Frames;
      audio->ADPCM_Info.channels = 1;
    } else {
      audio->ADPCM_Info.bit_size = audio->BitsPerSample;
      audio->ADPCM_Info.uncomp_size = audio->SampleRate / header->FPS *
                                      (audio->BitsPerSample >> 3) *
                                      audio->Channels * header->Frames;

      audio->ADPCM_Info.channels = audio->Channels;
    }

    audio->ADPCM_Info.comp_size =
        audio->ADPCM_Info.uncomp_size /
        static_cast<uint32_t>(audio->ADPCM_Info.bit_size / 4);
  }

  /*-------------------------------------------------------------------------
   * PRIME THE BUFFERS BY PRE-LOADING THEM WITH FRAME DATA.
   *-----------------------------------------------------------------------*/
  if (PrimeBuffers(vqa) != 0) {
    VQA_Close(vqa);
    return VQAERR_READ;
  }

  return 0;
}

/****************************************************************************
 *
 * NAME
 *     VQA_Close - Close an opened VQA file.
 *
 * SYNOPSIS
 *     VQA_Close(VQA)
 *
 *     void VQA_Close(VQAHandle *);
 *
 * FUNCTION
 *     Close the file that was opened with VQA_Open().
 *
 * INPUTS
 *     VQA - Pointer VQAHandle to close.
 *
 * RESULT
 *     NONE
 *
 ****************************************************************************/

void VQA_Close(VQAHandle* vqa) {
  auto* vqa_handle_p = vqa;
  /* Shutdown audio/timing system. */
  // Audio is open only once VQA_OpenAudio() has run. A failed VQA_Open() can
  // get here earlier, with no data and no audio callback to tear down.
  if (vqa_handle_p->data != nullptr &&
      (vqa_handle_p->data->Audio.Flags & VQAAUDF_DIGIINIT) != 0) {
    VQA_CloseAudio(vqa_handle_p);
  } else if ((vqa_handle_p->config.OptionFlags & VQAOPTF_AUDIO) == 0) {
    VQA_StopTimerInt(vqa_handle_p);
  }

  /* Free memory */
  if (vqa_handle_p->data != nullptr) {
    FreeBuffers(vqa_handle_p->data, &vqa_handle_p->config,
                &vqa_handle_p->header);
  }

  /* Close the VQA file */
  vqa_handle_p->io->Close();

  /* Reset the VQAHandle */
  vqa->Reset();
}

/****************************************************************************
 *
 * NAME
 *     VQA_LoadFrame - Load the next video frame from the VQA data stream.
 *
 * SYNOPSIS
 *     Error = VQA_LoadFrame(VQA)
 *
 *     long VQA_LoadFrame(VQAHandle *);
 *
 * FUNCTION
 *     The codebook is split up such that the last frame of every group gets
 *     a new, complete codebook, ready for the next group.  The first codebook
 *     in the VQA is a full codebook, and goes with the first frame's data.
 *     Partial codebooks are stored per frame after that, and they add up to
 *     a full codebook just before the first frame for the next group is read.
 *
 *     (Currently, this routine can read either the older non-frame-grouped
 *     VQA file format, or the new frame-chunk format.  For the older format,
 *     it's assumed that the last chunk in a frame is the pointer data.)
 *
 *     This routine also does a sort of "cooperative multitasking".  If the
 *     Loader hits a "wait state" where it has to wait on the audio to finish
 *     playing before it can continue to load, it sets a "sleep" flag and
 *     just returns.  The sleep flag is checked on entry to see if it needs
 *     to jump to the proper execution point. This may improve performance on
 *     some platforms, but it also allows the Loader to be called regardless
 *     of the size of the buffers; if the buffers fill up or the audio fails
 *     to play, the Loader won't just get stuck.
 *
 * INPUTS
 *     VQA - Pointer to VQAHandle structure.
 *
 * RESULT
 *     Error - 0 if successful or VQAERR_??? error code.
 *
 ****************************************************************************/

long VQA_LoadFrame(VQAHandle* vqa) {
  VQAData* vqabuf;
  VQALoader* loader;
  VQADrawer* drawer;
  VQAFrameNode* curframe;
  ChunkHeader* chunk;
  int32_t iffsize;
  long frame_loaded = 0;

  /* Dereference commonly used data members for quicker access. */
  VQAHandle* vqa_handle_p = vqa;
  vqabuf = vqa_handle_p->data;
  loader = &vqabuf->Loader;
  drawer = &vqa_handle_p->data->Drawer;
  curframe = loader->CurFrame;
  chunk = &loader->CurChunkHdr;

  iffsize = ChunkSize(*chunk);

  /* We have reached the end of the file if we loaded all the frames. */
  if (std::cmp_greater_equal(loader->CurFrameNum,
                             vqa_handle_p->header.Frames)) {
    return VQAERR_EOF;
  }

  /* If no buffer is available for loading then return. This allows the
   * drawer to service one of the buffers more readily. (We'll wait for one
   * to free up).
   */
  if (curframe->Flags & VQAFRMF_LOADED) {
    loader->WaitsOnDrawer++;
    return VQAERR_NOBUFFER;
  }

  /* If we're not sleeping, initialize */
  if (!(vqabuf->Flags & VQADATF_LSLEEP)) {
    frame_loaded = 0;
    loader->FrameSize = 0;

    /* Initialize the codebook ptr for the frame we're about to load:
     * (This frame's codebook is the last full codebook; we have to init it
     * now, because if we're on the last frame in a group, we'll get a new
     * FullCB pointer.)
     */
    curframe->Codebook = loader->FullCB;
  }

  /*-------------------------------------------------------------------------
   * THE MAIN LOADER LOOP
   *-----------------------------------------------------------------------*/
  while (frame_loaded == 0) {
    /* Read new chunk, only if we're not sleeping */
    if (!(vqabuf->Flags & VQADATF_LSLEEP)) {
      /* Read chunk ID */
      if (vqa_handle_p->io->Read(chunk, 8)) {
        return VQAERR_EOF;
      }

      iffsize = ChunkSize(*chunk);
      if (!IsValidChunkSize(iffsize)) {
        return VQAERR_READ;
      }

      // Saturates so a run of large skipped chunks cannot overflow the stat.
      loader->FrameSize = static_cast<int32_t>(
          std::min<int64_t>(int64_t{loader->FrameSize} + iffsize, INT32_MAX));
    }

    /* Handle each chunk type */
    switch (chunk->id) {
      /* VQ Normal Frame */
      case ID_VQFR:
        if (Load_VQF(vqa_handle_p, iffsize)) {
          return VQAERR_READ;
        }

        frame_loaded = 1;
        break;

      /* VQ Key Frame */
      case ID_VQFK:
        if (Load_VQF(vqa_handle_p, iffsize)) {
          return VQAERR_READ;
        }

        /* Flag this frame as being key. */
        curframe->Flags |= VQAFRMF_KEY;
        frame_loaded = 1;
        break;

      /* Full uncompressed codebook */
      case ID_CBF0:
        if (Load_CBF0(vqa_handle_p, iffsize)) {
          return VQAERR_READ;
        }
        break;

      /* Full compressed codebook */
      case ID_CBFZ:
        if (Load_CBFZ(vqa_handle_p, iffsize)) {
          return VQAERR_READ;
        }
        break;

      /* Partial uncompressed codebook */
      case ID_CBP0:
        if (Load_CBP0(vqa_handle_p, iffsize)) {
          return VQAERR_READ;
        }
        break;

      /* Partial compressed codebook */
      case ID_CBPZ:
        if (Load_CBPZ(vqa_handle_p, iffsize)) {
          return VQAERR_READ;
        }
        break;

      /* Uncompressed palette */
      case ID_CPL0:
        if (Load_CPL0(vqa_handle_p, iffsize)) {
          return VQAERR_READ;
        }

        /* If this is the first occurance of a palette then store it now.
         * This functionality is needed for Monopoly!
         */
        if (drawer->CurPalSize == 0) {
          memcpy(drawer->Palette_24, curframe->Palette,
                 base::ToSize(curframe->PaletteSize));
          drawer->CurPalSize = curframe->PaletteSize;
        }

        /* Flag this frame as having a palette. */
        curframe->Flags |= VQAFRMF_PALETTE;
        break;

      /* Compressed palette */
      case ID_CPLZ:
        if (Load_CPLZ(vqa_handle_p, iffsize)) {
          return VQAERR_READ;
        }

        /* If this is the first occurance of a palette then store it now.
         * This functionality is needed for Monopoly!
         */
        if (drawer->CurPalSize == 0) {
          drawer->CurPalSize =
              LCW_Uncompress(curframe->Palette + curframe->PalOffset,
                             drawer->Palette_24, sizeof(drawer->Palette_24));
        }

        /* Flag this frame as having a palette. */
        curframe->Flags |= VQAFRMF_PALETTE;
        break;

      /* Uncompressed pointer data */
      case ID_VPT0:
        if (Load_VPT0(vqa_handle_p, iffsize)) {
          return VQAERR_READ;
        }

        frame_loaded = 1;
        break;

      /* Compressed pointer data */
      case ID_VPTZ:
      case ID_VPTD:
        if (Load_VPTZ(vqa_handle_p, iffsize)) {
          return VQAERR_READ;
        }

        frame_loaded = 1;
        break;

      /* Pointer data Key (Must draw) */
      case ID_VPTK:
        if (Load_VPTZ(vqa_handle_p, iffsize)) {
          return VQAERR_READ;
        }

        /* Flag this frame as being key. */
        curframe->Flags |= VQAFRMF_KEY;
        frame_loaded = 1;
        break;

        /* Uncompressed audio frame.
         *
         *  - Make sure the sound load buffer (Audio.TempBuf) is empty; if not
         *    go into a sleep state.
         *  - Load the data into TempBuf.
         */
      case ID_SND0:
        if (!(vqa_handle_p->config.OptionFlags & VQAOPTF_ALTAUDIO)) {
          /* Move the last audio frame to the play buffer. */
          if (CopyAudio(vqa_handle_p) == VQAERR_SLEEPING) {
            vqabuf->Flags |= VQADATF_LSLEEP;
            return VQAERR_SLEEPING;
          }
          vqabuf->Flags &= ~VQADATF_LSLEEP;

          /* Load an uncompressed audio frame. */
          if (Load_SND0(vqa_handle_p, iffsize) != 0) {
            return VQAERR_READ;
          }
        } else {
          if (vqa_handle_p->io->Seek(PadSize(iffsize), SEEK_CUR)) {
            return VQAERR_SEEK;
          }
        }
        break;

      case ID_SNA0:
        if (vqa_handle_p->config.OptionFlags & VQAOPTF_ALTAUDIO) {
          /* Move the last audio frame to the play buffer. */
          if (CopyAudio(vqa_handle_p) == VQAERR_SLEEPING) {
            vqabuf->Flags |= VQADATF_LSLEEP;
            return VQAERR_SLEEPING;
          }
          vqabuf->Flags &= ~VQADATF_LSLEEP;

          /* Load an uncompressed audio frame. */
          if (Load_SND0(vqa_handle_p, iffsize) != 0) {
            return VQAERR_READ;
          }
        } else {
          if (vqa_handle_p->io->Seek(PadSize(iffsize), SEEK_CUR)) {
            return VQAERR_SEEK;
          }
        }
        break;

      /* Compressed audio frame.
       *
       *  - Make sure the sound load buffer (Audio.TempBuf) is empty; if not
       *    go into a sleep state.
       *  - Load the data into TempBuf.
       */
      case ID_SND1:
        if (!(vqa_handle_p->config.OptionFlags & VQAOPTF_ALTAUDIO)) {
          /* Move the last audio frame to the play buffer. */
          if (CopyAudio(vqa_handle_p) == VQAERR_SLEEPING) {
            vqabuf->Flags |= VQADATF_LSLEEP;
            return VQAERR_SLEEPING;
          }
          vqabuf->Flags &= ~VQADATF_LSLEEP;

          /* Load a compressed audio frame. */
          if (Load_SND1(vqa_handle_p, iffsize) != 0) {
            return VQAERR_READ;
          }
        } else {
          if (vqa_handle_p->io->Seek(PadSize(iffsize), SEEK_CUR)) {
            return VQAERR_SEEK;
          }
        }
        break;

      case ID_SNA1:
        if (vqa_handle_p->config.OptionFlags & VQAOPTF_ALTAUDIO) {
          /* Move the last audio frame to the play buffer. */
          if (CopyAudio(vqa_handle_p) == VQAERR_SLEEPING) {
            vqabuf->Flags |= VQADATF_LSLEEP;
            return VQAERR_SLEEPING;
          }
          vqabuf->Flags &= ~VQADATF_LSLEEP;

          /* Load a compressed audio frame. */
          if (Load_SND1(vqa_handle_p, iffsize) != 0) {
            return VQAERR_READ;
          }
        } else {
          if (vqa_handle_p->io->Seek(PadSize(iffsize), SEEK_CUR)) {
            return VQAERR_SEEK;
          }
        }
        break;

      /* HMI ADPCM compressed audio frame.
       *
       *  - Make sure the sound load buffer (Audio.TempBuf) is empty; if not
       *    go into a sleep state.
       *  - Load the data into TempBuf.
       */
      case ID_SND2:
        if (!(vqa_handle_p->config.OptionFlags & VQAOPTF_ALTAUDIO)) {
          /* Move the last audio frame to the play buffer. */
          if (CopyAudio(vqa_handle_p) == VQAERR_SLEEPING) {
            vqabuf->Flags |= VQADATF_LSLEEP;
            return VQAERR_SLEEPING;
          }
          vqabuf->Flags &= ~VQADATF_LSLEEP;

          /* Load a compressed audio frame. */
          if (Load_SND2(vqa_handle_p, iffsize) != 0) {
            return VQAERR_READ;
          }
        } else {
          if (vqa_handle_p->io->Seek(PadSize(iffsize), SEEK_CUR)) {
            return VQAERR_SEEK;
          }
        }
        break;

      case ID_SNA2:
        if (vqa_handle_p->config.OptionFlags & VQAOPTF_ALTAUDIO) {
          /* Move the last audio frame to the play buffer. */
          if (CopyAudio(vqa_handle_p) == VQAERR_SLEEPING) {
            vqabuf->Flags |= VQADATF_LSLEEP;
            return VQAERR_SLEEPING;
          }
          vqabuf->Flags &= ~VQADATF_LSLEEP;

          /* Load a compressed audio frame. */
          if (Load_SND2(vqa_handle_p, iffsize) != 0) {
            return VQAERR_READ;
          }
        } else {
          if (vqa_handle_p->io->Seek(PadSize(iffsize), SEEK_CUR)) {
            return VQAERR_SEEK;
          }
        }
        break;

      /* Skip any unknown chunks. */
      default:
        if (vqa_handle_p->io->Seek(PadSize(iffsize), SEEK_CUR)) {
          return VQAERR_SEEK;
        }
        break;
    }
  }

  /* Update maximum frame size stat. */
  if (loader->CurFrameNum > 0 && loader->FrameSize > loader->MaxFrameSize) {
    loader->MaxFrameSize = loader->FrameSize;
  }

  /*-------------------------------------------------------------------------
   * SET UP THE FRAME FOR DRAWING.
   *-----------------------------------------------------------------------*/

  /* Set the frame # */
  curframe->FrameNum = loader->CurFrameNum;
  loader->CurFrameNum++;

  /* Update data for mono output */
  loader->LastFrameNum = loader->CurFrameNum;

  /* Loader is finished with this frame; tell Drawer to draw it */
  curframe->Flags |= VQAFRMF_LOADED;
  loader->CurFrame = curframe->Next;

  return 0;
}

/****************************************************************************
 *
 * NAME
 *     VQA_SeekFrame - Position the movie stream to the specified frame.
 *
 * SYNOPSIS
 *     Frame = VQA_SeekFrame(VQA, Frame, FromWhere)
 *
 *     long VQA_SeekFrame(VQAHandle *, int32_t, long);
 *
 * FUNCTION
 *     This function sets the movie stream to the new frame specified by
 *     the 'offset' parameter. 'FromWhere' is a symbolic constant that is used
 *     to specify from where in the stream offset should be applied.
 *
 * INPUTS
 *     VQA       - Pointer to VQAHandle of movie to seek into.
 *     Frame     - Frame to seek to.
 *     FromWhere - Relative position indicator.
 *
 * RESULT
 *     Frame - New frame position, or a negative VQAERR_ code: VQAERR_EOF
 *             for a frame past the end, VQAERR_SEEK for a negative frame
 *             or a movie without a frame table.
 *
 ****************************************************************************/

long VQA_SeekFrame(VQAHandle* vqa, int32_t framenum, long /*fromwhere*/) {
  VQAHandle* vqap;
  VQAData* vqabuf;
  VQALoader* loader;
  VQAHeader* header;
  VQAFrameNode* frame;
  VQAConfig* config;
  int32_t group;
  int32_t i;
  long rc = VQAERR_NONE;
  VQAAudio* audio;
  long audio_on;
  /* Dereference commonly used data members for quick access. */
  vqap = vqa;
  vqabuf = vqap->data;
  loader = &vqabuf->Loader;
  header = &vqap->header;
  config = &vqap->config;

  audio = &vqabuf->Audio;

  /* Stop audio playback. */
  audio_on = audio->Flags & VQAAUDF_ISPLAYING;
  VQA_StopAudio(vqap);

  /* Make sure the requested frame is valid and the frame information
   * array is allocated before continuing.
   */
  if (framenum < 0 || vqabuf->Foff == nullptr) {
    rc = VQAERR_SEEK;
  } else if (std::cmp_greater_equal(framenum, header->Frames)) {
    rc = VQAERR_EOF;
  }

  if (rc == VQAERR_NONE) {
    /* Find and load the most recent palette. */
    if (!(config->OptionFlags & VQAOPTF_PALOFF)) {
      /* Get the current frame. */
      frame = loader->CurFrame;

      for (i = framenum; i >= 0; i--) {
        if (vqabuf->Foff[i] & VQAFINF_PAL) {
          /* Seek to the palette frame. */
          rc = vqap->io->Seek(VQAFRAME_OFFSET(vqabuf->Foff[i]), SEEK_SET);

          /* Fool the loader into thinking this frame is empty. */
          if (!rc) {
            loader->NumPartialCB = 0;
            loader->PartialCBSize = 0;
            loader->FullCB = vqabuf->CBData;
            loader->CurCB = vqabuf->CBData;
            loader->CurFrameNum = 0;
            frame->Flags = 0;

            /* Load the frame with the palette. */
            if (VQA_LoadFrame(vqa) == 0) {
              /* Decompress the palette if neccessary.*/
              if (frame->Flags & VQAFRMF_PALCOMP) {
                frame->PaletteSize =
                    LCW_Uncompress(frame->Palette + frame->PalOffset,
                                   frame->Palette, vqabuf->Max_Pal_Size);
              }

              SetPalette(frame->Palette, frame->PaletteSize, 0);
            }
          } else {
            rc = VQAERR_SEEK;
          }
          break;
        }
      }
    }

    /* Build the codebook for the frame we are seeking to. */
    if (!rc) {
      /* Compute the starting group frame of the requested frame. */
      group = framenum / header->Groupsize;
      group = group * header->Groupsize;

      /* The codebook for the group we want to goto is found in the previous
       * group, with the exception of the very first group.
       */
      if (std::cmp_greater_equal(group, header->Groupsize)) {
        group -= header->Groupsize;
      }

      /* Seek to the start of the group containing the partial codebooks for
       * the target frame.
       */
      if (!vqap->io->Seek(VQAFRAME_OFFSET(vqabuf->Foff[group]), SEEK_SET)) {
        /* Throw away any audio frames that were loaded. */
        if (config->OptionFlags & VQAOPTF_AUDIO && audio->Buffer != nullptr) {
          memset(audio->IsLoaded, 0,
                 base::ToSize(audio->NumAudBlocks) * sizeof(*audio->IsLoaded));
          memset(audio->Buffer, 0, base::ToSize(config->AudioBufSize));

          /* Position the audio buffer to 1/2 second. */
          audio->AudBufPos = audio->SampleRate * audio->Channels *
                             (audio->BitsPerSample >> 3) / 2;

          /* Mark 1/2 second of the audio buffer as loaded. */
          for (i = 0; i < audio->AudBufPos / config->HMIBufSize; i++) {
            audio->IsLoaded[i] = 1;
          }
        }

        /* Force the loader to the desired frame. */
        loader->NumPartialCB = 0;
        loader->PartialCBSize = 0;
        loader->FullCB = vqabuf->CBData;
        loader->CurCB = vqabuf->CBData;
        loader->CurFrameNum = group;

        /* Load frames up to the target frame collecting partial codebooks
         * along the way.
         */
        for (i = 0; i < framenum - group; i++) {
          /* Fool the loader into thinking the frame has been drawn. */
          loader->CurFrame->Flags = 0;

          audio->TempBufLen = 0;

          /* Load the frame. */
          rc = VQA_LoadFrame(vqa);
          if (rc != 0) {
            if (rc != VQAERR_NOBUFFER && rc != VQAERR_SLEEPING) {
              break;
            }
            rc = 0;
          }
        }

        /* If everything is okay, then re-prime the buffers. */
        if (!rc) {
          /* Mark all the frames except the current one as empty. */
          loader->CurFrame->Flags = 0;
          frame = loader->CurFrame->Next;

          while (frame != loader->CurFrame) {
            frame->Flags = 0;
            frame = frame->Next;
          }

          /* Set the drawer to the current frame and the loader
           * to the next.
           */
          vqabuf->Drawer.CurFrame = loader->CurFrame;

          /* Prime the buffers for the new position. */
          rc = PrimeBuffers(vqa);

          /* An end of file is not considered and error. */
          if (rc == 0 || rc == VQAERR_EOF) {
            rc = framenum;
          }
        }
      } else {
        rc = VQAERR_SEEK;
      }
    }
  }

  /* Restart audio playback. */
  if (audio_on) {
    VQA_StartAudio(vqap);
  }

  return rc;
}

/****************************************************************************
 *
 * NAME
 *     AllocBuffers - Allocate VQA play buffers.
 *
 * SYNOPSIS
 *     VQAData = AllocBuffers(Header, Config)
 *
 *     VQAData *AllocBuffers(VQAHeader *, VQAConfig *);
 *
 * FUNCTION
 *     For those structures that contain buffer pointers (codebook nodes,
 *     frame buffer nodes), enough memory is allocated for both the structure
 *     and its associated buffers, then the buffer pointers are pointed to
 *     the appropriate offset from the structure pointer.  This allows us
 *     to perform only one malloc & free for each node.
 *
 *     Buffers allocated:
 *       - vqa
 *       - vqa->CBData (list)
 *       - vqa->FrameData (list)
 *       - vqa->Drawer.ImageBuf
 *       - vqa->Audio.Buffer
 *       - vqa->Audio.IsLoaded
 *       - vqa->Foff
 *
 * INPUTS
 *     Header - Pointer to VQAHeader structure.
 *     Config - Pointer to VQA configuration structure.
 *
 * RESULT
 *     VQAData - Pointer to initialized VQAData structure.
 *
 ****************************************************************************/

static VQAData* AllocBuffers(VQAHeader* header, VQAConfig* config) {
  /* Check the configuration for valid values. */
  if (config->NumCBBufs == 0 || config->NumFrameBufs == 0) {
    return nullptr;
  }

  /* Allocate the master structure using unique_ptr for RAII. */
  auto vqa_ptr = std::make_unique<VQAData>();
  VQAData* vqa = vqa_ptr.get();

  /*-------------------------------------------------------------------------
   * INITIALIZE THE VQA DATA STRUCTURES.
   *
   * The Max buffer sizes are computed with 1K of padding, and'd with 0xFFFC
   * to make the size divisible by 4, to ensure DWORD alignment.
   *-----------------------------------------------------------------------*/
  vqa->MemUsed = sizeof(VQAData);
  vqa->Drawer.LastTime = -VQA_TIMETICKS;

  /* Set maximum codebook size. */
  vqa->Max_CB_Size =
      ((header->CBentries * header->BlockWidth * header->BlockHeight) + 250) &
      0xFFFC;

  /* Set maximum palette size. */
  vqa->Max_Pal_Size = (768 + 1024) & 0xFFFC;

  /* Set maximum vector pointers size. */
  vqa->Max_Ptr_Size = (((header->ImageWidth / header->BlockWidth) *
                        (header->ImageHeight / header->BlockHeight) *
                        int{sizeof(int16_t)}) +
                       1024) &
                      0xFFFC;

  /* Set the frame number of the frame containing the last codebook. */
  vqa->Loader.LastCBFrame =
      ((header->Frames - 1) / header->Groupsize) * header->Groupsize;

  /*-------------------------------------------------------------------------
   * ALLOCATE THE CODEBOOK BUFFERS.
   *-----------------------------------------------------------------------*/
  vqa->CBNodes.reserve(base::ToSize(config->NumCBBufs));

  for (int32_t i = 0; i < config->NumCBBufs; i++) {
    /* Allocate a codebook node using unique_ptr. */
    auto cbnode = std::make_unique<VQACBNode>();

    /* Allocate the buffer storage. */
    cbnode->BufferStorage.resize(base::ToSize(vqa->Max_CB_Size));
    cbnode->Buffer = cbnode->BufferStorage.data();

    /* Keep count of the memory usage. */
    vqa->MemUsed += int32_t{sizeof(VQACBNode)} + vqa->Max_CB_Size;

    vqa->CBNodes.push_back(std::move(cbnode));
  }

  /* Set up the circular linked list */
  for (size_t i = 0; i < vqa->CBNodes.size(); i++) {
    size_t next_idx = (i + 1) % vqa->CBNodes.size();
    vqa->CBNodes[i]->Next = vqa->CBNodes[next_idx].get();
  }

  /* Install the Codebook list */
  vqa->CBData = vqa->CBNodes[0].get();
  vqa->Loader.CurCB = vqa->CBData;
  vqa->Loader.FullCB = vqa->CBData;

  /*-------------------------------------------------------------------------
   * ALLOCATE THE FRAME BUFFERS.
   *-----------------------------------------------------------------------*/
  vqa->FrameNodes.reserve(base::ToSize(config->NumFrameBufs));

  for (int32_t i = 0; i < config->NumFrameBufs; i++) {
    /* Allocate a frame node using unique_ptr. */
    auto framenode = std::make_unique<VQAFrameNode>();

    /* Allocate the buffer storage. */
    framenode->PointersStorage.resize(base::ToSize(vqa->Max_Ptr_Size));
    framenode->PaletteStorage.resize(base::ToSize(vqa->Max_Pal_Size));
    framenode->Pointers = framenode->PointersStorage.data();
    framenode->Palette = framenode->PaletteStorage.data();

    framenode->Codebook = vqa->CBData;

    /* Keep count of the memory usage. */
    vqa->MemUsed +=
        int32_t{sizeof(VQAFrameNode)} + vqa->Max_Ptr_Size + vqa->Max_Pal_Size;

    vqa->FrameNodes.push_back(std::move(framenode));
  }

  /* Set up the circular linked list */
  for (size_t i = 0; i < vqa->FrameNodes.size(); i++) {
    size_t next_idx = (i + 1) % vqa->FrameNodes.size();
    vqa->FrameNodes[i]->Next = vqa->FrameNodes[next_idx].get();
  }

  /* Install the Frame Buffer list */
  vqa->FrameData = vqa->FrameNodes[0].get();
  vqa->Loader.CurFrame = vqa->FrameData;
  vqa->Drawer.CurFrame = vqa->FrameData;
  vqa->Flipper.CurFrame = vqa->FrameData;

  /*-------------------------------------------------------------------------
   * ALLOCATE THE IMAGE BUFFERS IF ONE IS NOT ALREADY PROVIDED.
   *-----------------------------------------------------------------------*/
  if (config->ImageBuf == nullptr) {
    /* Allocate our own buffer. */
    if ((config->DrawFlags & VQACFGF_BUFFER) != 0) {
      vqa->ImageBufStorage.resize(static_cast<std::size_t>(header->ImageWidth) *
                                  header->ImageHeight);
      vqa->Drawer.ImageBuf = vqa->ImageBufStorage.data();

      /* Plugin image buffer information. */
      vqa->Drawer.ImageWidth = header->ImageWidth;
      vqa->Drawer.ImageHeight = header->ImageHeight;
      vqa->MemUsed += header->ImageWidth * header->ImageHeight;
    } else {
      vqa->Drawer.ImageWidth = config->ImageWidth;
      vqa->Drawer.ImageHeight = config->ImageHeight;
    }
  } else {
    /* Use caller provided buffer */
    vqa->Drawer.ImageBuf = config->ImageBuf;
    vqa->Drawer.ImageWidth = config->ImageWidth;
    vqa->Drawer.ImageHeight = config->ImageHeight;
  }

  /*-------------------------------------------------------------------------
   * ALLOCATE AND INITIALIZE AUDIO BUFFERS AND STRUCTURES.
   *-----------------------------------------------------------------------*/
  if ((header->Flags & VQAHDF_AUDIO) != 0 &&
      (config->OptionFlags & VQAOPTF_AUDIO) != 0) {
    /* Dereference audio structure for quick access. */
    VQAAudio* audio = &vqa->Audio;

    /* Version 1 VQA's only supported 22050 8 bit mono audio. */
    if (header->Version < VQAHD_VER2) {
      audio->SampleRate = 22050U;
      audio->Channels = 1;
      audio->BitsPerSample = 8;
      audio->BytesPerSec = 22050;
    } else {
      if (config->OptionFlags & VQAOPTF_ALTAUDIO &&
          header->Flags & VQAHDF_ALTAUDIO) {
        audio->SampleRate = header->AltSampleRate;
        audio->Channels = header->AltChannels;
        audio->BitsPerSample = header->AltBitsPerSample;
      } else {
        audio->SampleRate = header->SampleRate;
        audio->Channels = header->Channels;
        audio->BitsPerSample = header->BitsPerSample;
      }

      audio->BytesPerSec =
          audio->SampleRate * audio->Channels * (audio->BitsPerSample >> 3);
    }

    /* The default audio buffer size should be large enough to hold
     * 1.5 seconds of data.
     */
    if (config->AudioBufSize == -1) {
      /* Compute the number of HMI buffers that will completly fit into
       * 1.5 seconds of audio data.
       */
      auto i =
          (audio->BytesPerSec + (audio->BytesPerSec / 2)) / config->HMIBufSize;
      config->AudioBufSize = config->HMIBufSize * i;
    }

    /* Do not allocate anything if the audio buffer is zero length. */
    if (config->AudioBufSize > 0) {
      /* Allocate an audio buffer if the user did not provide one.
       * Otherwise, use the user supplied buffer.
       */
      if (config->AudioBuf == nullptr) {
        audio->BufferStorage.resize(base::ToSize(config->AudioBufSize));
        audio->Buffer = audio->BufferStorage.data();

        /* Add audio buffer size to memory usage. */
        vqa->MemUsed += config->AudioBufSize;
      } else {
        audio->Buffer = config->AudioBuf;
      }

      /* Allocate IsLoaded flags */
      audio->NumAudBlocks = config->AudioBufSize / config->HMIBufSize;
      audio->IsLoadedStorage.resize(base::ToSize(audio->NumAudBlocks), 0);
      audio->IsLoaded = audio->IsLoadedStorage.data();

      /* Add IsLoaded flags array to memory usage. */
      vqa->MemUsed += audio->NumAudBlocks * int32_t{sizeof(*audio->IsLoaded)};

      /* Allocate temporary staging buffer for the audio frames. */
      audio->TempBufSize = (audio->BytesPerSec / header->FPS * 2) + 100;
      audio->TempBufStorage.resize(base::ToSize(audio->TempBufSize));
      audio->TempBuf = audio->TempBufStorage.data();

      /* Add temporary buffer size to memory usage. */
      vqa->MemUsed += audio->TempBufSize;
    }
  }

  /*-------------------------------------------------------------------------
   * ALLOCATE THE FRAME INFORMATION TABLE.
   *-----------------------------------------------------------------------*/
  vqa->FoffStorage.resize(header->Frames);
  vqa->Foff = vqa->FoffStorage.data();

  /* Keep a running total of memory usage. */
  vqa->MemUsed += header->Frames * int32_t{sizeof(*vqa->Foff)};

  /* Release ownership - caller is responsible for the pointer now.
   * VQAHandle::VQABuf will own this pointer. */
  return vqa_ptr.release();
}

/****************************************************************************
 *
 * NAME
 *     FreeBuffers - Free VQA play buffers.
 *
 * SYNOPSIS
 *     FreeBuffers(VQAData, Config, Header)
 *
 *     void FreeBuffers(VQAData *, VQAConfig *, VQAHeader *);
 *
 * FUNCTION
 *      Free the buffers allocated by AllocBuffers().
 *
 * INPUTS
 *      VQAData - Pointer to VQAData structure.
 *      Config  - Pointer to configuration structure.
 *      Header  - Pointer to movie header structure.
 *
 * RESULT
 *      NONE
 *
 ****************************************************************************/

static void FreeBuffers(VQAData* vqa, VQAConfig* /*config*/,
                        VQAHeader* /*header*/) {
  /* With RAII, all we need to do is delete the VQAData structure.
   * The vectors and unique_ptrs inside will automatically clean up:
   * - FoffStorage (vector<uint32_t>)
   * - Audio.BufferStorage, IsLoadedStorage, TempBufStorage (vectors)
   * - ImageBufStorage (vector<unsigned char>)
   * - FrameNodes (vector<unique_ptr<VQAFrameNode>>)
   * - CBNodes (vector<unique_ptr<VQACBNode>>)
   * Each node's BufferStorage/PointersStorage/PaletteStorage vectors
   * are also automatically cleaned up.
   */
  delete vqa;
}

/****************************************************************************
 *
 * NAME
 *     PrimeBuffers - Pre-Load the internal buffers.
 *
 * SYNOPSIS
 *     Error = PrimeBuffers(VQA)
 *
 *     long = PrimeBuffers(VQAHandle *);
 *
 * FUNCTION
 *     Pre-load the internal buffers in order to give the player some slack
 *     in the playback of large frames.
 *
 * INPUTS
 *     VQA - Pointer to VQAHandle structure.
 *
 * RESULT
 *     Error - 0 if successful, or VQAERR_??? error code.
 *
 ****************************************************************************/

long PrimeBuffers(VQAHandle* vqa) {
  VQAData* vqabuf;
  VQAConfig* config;
  long rc;
  long i;

  /* Dereference commonly used data members for quick access. */
  vqabuf = vqa->data;
  config = &vqa->config;

  /* Pre-load the buffers */
  for (i = 0; i < config->NumFrameBufs; i++) {
    rc = VQA_LoadFrame(vqa);
    if (rc == 0) {
      vqabuf->LoadedFrames++;
    } else if (rc == VQAERR_EOF && std::cmp_greater_equal(
                                       vqabuf->Loader.CurFrameNum,
                                       vqa->header.Frames)) {
      // A movie with fewer frames than buffers ends while priming. Only an
      // end of file before the last frame (a truncated movie) is an error.
      break;
    } else if (rc != VQAERR_NOBUFFER && rc != VQAERR_SLEEPING) {
      return rc;
    }
  }

  return 0;
}

/****************************************************************************
 *
 * NAME
 *     Load_VQF - Loads a VQ Frame chunk.
 *
 * SYNOPSIS
 *     Error = Load_VQF(VQA, Iffsize)
 *
 *     long Load_VQF(VQAHandle *, int32_t);
 *
 * FUNCTION
 *     The VQ Frame Chunk contains a set of other chunks (codebooks,
 *     palettes, pointers).  This routine reads the frame's chunk size,
 *     then loops until it's read that many bytes.
 *
 * INPUTS
 *     VQA     - Pointer to private VQA handle.
 *     Iffsize - Size of IFF chunk.
 *
 * RESULT
 *     Error - 0 if successful or VQAERR_??? error code.
 *
 ****************************************************************************/

static long Load_VQF(VQAHandle* vqap, int32_t frame_iffsize) {
  VQAData* vqabuf;
  VQAFrameNode* curframe;
  ChunkHeader* chunk;
  int32_t iffsize;
  int32_t framesize;
  int64_t bytes_loaded = 0;  // 64-bit: sums sizes up to 2^31 each.
  VQADrawer* drawer;

  /* Dereference commonly used data members for quicker access. */
  vqabuf = vqap->data;
  curframe = vqabuf->Loader.CurFrame;
  framesize = PadSize(frame_iffsize);
  drawer = &vqap->data->Drawer;
  chunk = &vqabuf->Loader.CurChunkHdr;

  /*-------------------------------------------------------------------------
   * FRAME LOADING LOOP.
   *-----------------------------------------------------------------------*/
  while (bytes_loaded < framesize) {
    /* Read chunk ID */
    if (vqap->io->Read(chunk, 8)) {
      return VQAERR_EOF;
    }

    iffsize = ChunkSize(*chunk);
    if (!IsValidChunkSize(iffsize)) {
      return VQAERR_READ;
    }

    bytes_loaded += 8;
    bytes_loaded += PadSize(iffsize);

    /* Handle each chunk type */
    switch (chunk->id) {
      /* Full uncompressed codebook */
      case ID_CBF0:
        if (Load_CBF0(vqap, iffsize)) {
          return VQAERR_READ;
        }
        break;

      /* Full compressed codebook */
      case ID_CBFZ:
        if (Load_CBFZ(vqap, iffsize)) {
          return VQAERR_READ;
        }
        break;

      /* Partial uncompressed codebook */
      case ID_CBP0:
        if (Load_CBP0(vqap, iffsize)) {
          return VQAERR_READ;
        }
        break;

      /* Partial compressed codebook */
      case ID_CBPZ:
        if (Load_CBPZ(vqap, iffsize)) {
          return VQAERR_READ;
        }
        break;

      /* Uncompressed palette */
      case ID_CPL0:
        if (Load_CPL0(vqap, iffsize)) {
          return VQAERR_READ;
        }

        /* If this is the first occurance of a palette then store it now.
         * This functionality is needed for Monopoly!
         */
        if (drawer->CurPalSize == 0) {
          memcpy(drawer->Palette_24, curframe->Palette,
                 base::ToSize(curframe->PaletteSize));
          drawer->CurPalSize = curframe->PaletteSize;
        }

        /* Flag this frame as having a palette. */
        curframe->Flags |= VQAFRMF_PALETTE;
        break;

      /* Compressed palette */
      case ID_CPLZ:
        if (Load_CPLZ(vqap, iffsize)) {
          return VQAERR_READ;
        }

        /* If this is the first occurance of a palette then store it now.
         * This functionality is needed for Monopoly!
         */
        if (drawer->CurPalSize == 0) {
          drawer->CurPalSize =
              LCW_Uncompress(curframe->Palette + curframe->PalOffset,
                             drawer->Palette_24, sizeof(drawer->Palette_24));
        }

        /* Flag this frame as having a palette. */
        curframe->Flags |= VQAFRMF_PALETTE;
        break;

      /* Uncompressed pointer data */
      case ID_VPT0:
        if (Load_VPT0(vqap, iffsize)) {
          return VQAERR_READ;
        }
        break;

      /* Compressed pointer data */
      case ID_VPTZ:
      case ID_VPTD:
        if (Load_VPTZ(vqap, iffsize)) {
          return VQAERR_READ;
        }
        break;

      /* Compressed pointer data */
      case ID_VPTK:
        if (Load_VPTZ(vqap, iffsize)) {
          return VQAERR_READ;
        }

        /* Flag this frame as being key. */
        curframe->Flags |= VQAFRMF_KEY;
        break;

      /* An unknown chunk in the video frame is an error. */
      default:
        return VQAERR_READ;
    }
  }

  return 0;
}

/****************************************************************************
 *
 * NAME
 *     Load_FINF - Load Frame Info chunk.
 *
 * SYNOPSIS
 *     Error = Load_FINF(VQA, Iffsize)
 *
 *     long Load_FINF(VQAHandle *, int32_t);
 *
 * FUNCTION
 *     Load FINF chunk if buffer available, otherwise skip it.
 *
 * INPUTS
 *     VQA     - Pointer to private VQA handle.
 *     Iffsize - Size of IFF chunk.
 *
 * RESULT
 *     Error - 0 if successful or VQAERR_??? error code.
 *
 ****************************************************************************/

static long Load_FINF(VQAHandle* vqap, int32_t iffsize) {
  VQAData* vqabuf = vqap->data;

  // The table has one 4-byte entry per frame in the header. Copying no more
  // than that and skipping the rest keeps an oversized chunk from writing
  // past it; entries a short chunk leaves out stay zero.
  const auto table_bytes =
      static_cast<int64_t>(vqabuf->FoffStorage.size() * sizeof(uint32_t));
  const int64_t copy_bytes = std::min<int64_t>(iffsize, table_bytes);
  if (copy_bytes > 0 &&
      vqap->io->Read(vqabuf->FoffStorage.data(), copy_bytes)) {
    return VQAERR_READ;
  }

  const int64_t skip_bytes = PadSize(iffsize) - copy_bytes;
  if (skip_bytes > 0 && vqap->io->Seek(skip_bytes, SEEK_CUR)) {
    return VQAERR_SEEK;
  }

  return 0;
}

/****************************************************************************
 *
 * NAME
 *     Load_CBF0 - Load full uncompressed codebook.
 *
 * SYNOPSIS
 *     Error = Load_CBF0(VQA, Iffsize)
 *
 *     long Load_CBF0(VQAHandle *, int32_t);
 *
 * FUNCTION
 *
 * INPUTS
 *     VQA     - Pointer to private VQA handle.
 *     Iffsize - Size of IFF chunk.
 *
 * RESULT
 *     Error - 0 if successful or VQAERR_??? error code.
 *
 ****************************************************************************/

static long Load_CBF0(VQAHandle* vqap, int32_t iffsize) {
  VQALoader* loader;
  VQACBNode* curcb;

  /* Dereference commonly used data members for quicker access. */
  loader = &vqap->data->Loader;
  curcb = loader->CurCB;

  if (!FitsInBuffer(0, PadSize(iffsize), vqap->data->Max_CB_Size)) {
    return VQAERR_READ;
  }

  /* Read into the start of the buffer */
  if (vqap->io->Read(curcb->Buffer, PadSize(iffsize))) {
    return VQAERR_READ;
  }

  /* Reset the partial codebook counter. */
  loader->NumPartialCB = 0;

  /* Flag this codebook as uncompressed. */
  curcb->Flags &= ~VQACBF_CBCOMP;
  curcb->CBOffset = 0;

  /* Clock pointers to next CB Buffer. */
  loader->FullCB = curcb;
  loader->FullCB->Flags &= ~VQACBF_DOWNLOADED;
  loader->CurCB = curcb->Next;

  return 0;
}

/****************************************************************************
 *
 * NAME
 *     Load_CBFZ - Load full compressed codebook.
 *
 * SYNOPSIS
 *     Error = Load_CBFZ(VQA, Iffsize)
 *
 *     long Load_CBFZ(VQAHandle *, int32_t);
 *
 * FUNCTION
 *
 * INPUTS
 *     VQA     - Pointer to private VQA handle.
 *     Iffsize - Size of IFF chunk.
 *
 * RESULT
 *     Error - 0 if successful or VQAERR_??? error code.
 *
 ****************************************************************************/

static long Load_CBFZ(VQAHandle* vqap, int32_t iffsize) {
  VQALoader* loader;
  VQACBNode* curcb;
  void* buffer;
  int32_t padsize;
  int32_t lcwoffset;

  /* Dereference commonly used data members for quicker access. */
  loader = &vqap->data->Loader;
  curcb = loader->CurCB;
  padsize = PadSize(iffsize);

  /* Load the codebook into the end of the buffer. */
  lcwoffset = vqap->data->Max_CB_Size - padsize;

  // A chunk larger than the buffer would start before it.
  if (lcwoffset < 0) {
    return VQAERR_READ;
  }

  buffer = curcb->Buffer + lcwoffset;

  if (vqap->io->Read(buffer, padsize)) {
    return VQAERR_READ;
  }

  /* Reset the partial codebook counter. */
  loader->NumPartialCB = 0;

  /* Flag this codebook as compressed */
  curcb->Flags |= VQACBF_CBCOMP;
  curcb->CBOffset = lcwoffset;

  /* Clock pointers to next CB Buffer */
  loader->FullCB = curcb;
  loader->FullCB->Flags &= ~VQACBF_DOWNLOADED;
  loader->CurCB = curcb->Next;

  return 0;
}

/****************************************************************************
 *
 * NAME
 *     Load_CBP0 - Load partial uncompressed codebook.
 *
 * SYNOPSIS
 *     Error = Load_CBP0(VQA, Iffsize)
 *
 *     long Load_CBP0(VQAHandle *, int32_t);
 *
 * FUNCTION
 *
 * INPUTS
 *     VQA     - Pointer to private VQA handle.
 *     Iffsize - Size of IFF chunk.
 *
 * RESULT
 *     Error - 0 if successful or VQA_??? error code.
 *
 ****************************************************************************/

static long Load_CBP0(VQAHandle* vqap, int32_t iffsize) {
  VQAData* vqabuf;
  VQALoader* loader;
  VQACBNode* curcb;
  void* buffer;

  /* Dereference commonly used data members for quicker access. */
  vqabuf = vqap->data;
  loader = &vqabuf->Loader;
  curcb = loader->CurCB;

  /*-------------------------------------------------------------------------
   * ASSEMBLY PARTIAL CODEBOOKS.
   *-----------------------------------------------------------------------*/

  if (!FitsInBuffer(loader->PartialCBSize, PadSize(iffsize),
                    vqabuf->Max_CB_Size)) {
    return VQAERR_READ;
  }

  /* Read the partial codebook into the next position in the buffer. */
  buffer = curcb->Buffer + loader->PartialCBSize;

  if (vqap->io->Read(buffer, PadSize(iffsize))) {
    return VQAERR_READ;
  }

  /* Accumulate the partial codebook values. */
  loader->PartialCBSize += iffsize;
  loader->NumPartialCB++;

  /*-------------------------------------------------------------------------
   * PROCESS FULL CODEBOOK.
   *-----------------------------------------------------------------------*/
  if (std::cmp_equal(loader->NumPartialCB, vqap->header.Groupsize)) {
    /* Reset the codebook accumulator values */
    loader->NumPartialCB = 0;
    loader->PartialCBSize = 0;

    /* Flag this codebook as uncompressed */
    curcb->Flags &= ~VQACBF_CBCOMP;
    curcb->CBOffset = 0;

    /* Go to the next codebook buffer */
    loader->FullCB = curcb;
    loader->FullCB->Flags &= ~VQACBF_DOWNLOADED;
    loader->CurCB = curcb->Next;
  }

  return 0;
}

/****************************************************************************
 *
 * NAME
 *     Load_CBPZ - Load partial compressed codebook.
 *
 * SYNOPSIS
 *     Error = Load_CBPZ(VQA, Iffsize)
 *
 *     long Load_CBPZ(VQAHandle *, int32_t);
 *
 * FUNCTION
 *
 * INPUTS
 *     VQA     - Pointer to private VQA handle.
 *     Iffsize - Size of IFF chunk.
 *
 * RESULT
 *     Error - 0 if successful or VQAERR_??? error code.
 *
 ****************************************************************************/

static long Load_CBPZ(VQAHandle* vqap, int32_t iffsize) {
  VQAData* vqabuf;
  VQALoader* loader;
  VQACBNode* curcb;
  void* buffer;
  int32_t padsize;

  /* Dereference commonly used data members for quicker access */
  vqabuf = vqap->data;
  loader = &vqabuf->Loader;
  curcb = loader->CurCB;
  padsize = PadSize(iffsize);

  /* Attempt to compute the LCW offset into the codebook buffer by
   * multiplying the size of this chunk by the # frames/group, and adding
   * a small fudge factor on, then subtracting that from the CB buffer size.
   */
  if (loader->PartialCBSize == 0) {
    // 64-bit because a large chunk times the group size overflows int32_t.
    // A negative estimate would place the codebook before the buffer.
    const int64_t cboffset =
        int64_t{vqabuf->Max_CB_Size} -
        ((int64_t{padsize} * vqap->header.Groupsize) + 100);
    if (cboffset < 0) {
      return VQAERR_READ;
    }
    curcb->CBOffset = static_cast<int32_t>(cboffset);
  }

  // The estimate assumes every part of the group is the size of the first
  // one, so a larger later part can still run off the end.
  if (!FitsInBuffer(int64_t{curcb->CBOffset} + loader->PartialCBSize, padsize,
                    vqabuf->Max_CB_Size)) {
    return VQAERR_READ;
  }

  /*-------------------------------------------------------------------------
   * ASSEMBLE PARTIAL CODEBOOKS.
   *-----------------------------------------------------------------------*/

  /* Read the partial codebook into the next position in the buffer. */
  buffer = curcb->Buffer + curcb->CBOffset + loader->PartialCBSize;

  if (vqap->io->Read(buffer, padsize)) {
    return VQAERR_READ;
  }

  /* Accumulate partial codebook values */
  loader->PartialCBSize += iffsize;
  loader->NumPartialCB++;

  /*-------------------------------------------------------------------------
   * PROCESS FULL CODEBOOK.
   *-----------------------------------------------------------------------*/
  if (std::cmp_equal(loader->NumPartialCB, vqap->header.Groupsize)) {
    /* Reset the codebook accumulator values. */
    loader->NumPartialCB = 0;
    loader->PartialCBSize = 0;

    /* Flag this codebook as compressed. */
    curcb->Flags |= VQACBF_CBCOMP;

    /* Go to the next codebook buffer */
    loader->FullCB = curcb;
    loader->FullCB->Flags &= ~VQACBF_DOWNLOADED;
    loader->CurCB = curcb->Next;
  }

  return 0;
}

/****************************************************************************
 *
 * NAME
 *     Load_CPL0 - Load an uncompressed palette.
 *
 * SYNOPSIS
 *     Error = Load_CPL0(VQA, Iffsize)
 *
 *     long Load_CPL0(VQAHandle *, int32_t);
 *
 * FUNCTION
 *
 * INPUTS
 *     VQA     - Pointer to private VQA handle.
 *     Iffsize - Size of IFF chunk.
 *
 * RESULT
 *     Error - 0 if successful or VQAERR_??? error code.
 *
 ****************************************************************************/

static long Load_CPL0(VQAHandle* vqap, int32_t iffsize) {
  VQAFrameNode* curframe;

  /* Dereference commonly used data members for quicker access. */
  curframe = vqap->data->Loader.CurFrame;

  // The loader copies a frame's palette into the drawer's 256-color palette,
  // so a larger one is malformed and would overrun that copy.
  if (!FitsInBuffer(0, PadSize(iffsize),
                    int64_t{sizeof(VQADrawer::Palette_24)})) {
    return VQAERR_READ;
  }

  /* Read the palette into the palette buffer */
  if (vqap->io->Read(curframe->Palette, PadSize(iffsize))) {
    return VQAERR_READ;
  }

  /* Flag the palette as uncompressed. */
  curframe->Flags &= ~VQAFRMF_PALCOMP;
  curframe->PalOffset = 0;
  curframe->PaletteSize = iffsize;

  return 0;
}

/****************************************************************************
 *
 * NAME
 *     Load_CPLZ - Load compressed palette.
 *
 * SYNOPSIS
 *     Error = Load_CPLZ(VQA, Iffsize)
 *
 *     long Load_CPLZ(VQAHandle *, int32_t);
 *
 * FUNCTION
 *
 * INPUTS
 *     VQA     - Pointer to private VQA handle.
 *     Iffsize - Size of IFF chunk.
 *
 * RESULT
 *     Error - 0 if successful or VQAERR_??? error code.
 *
 ****************************************************************************/

static long Load_CPLZ(VQAHandle* vqap, int32_t iffsize) {
  VQAFrameNode* curframe;
  void* buffer;
  int32_t padsize;
  int32_t lcwoffset;

  /* Dereference commonly used data members for quicker access. */
  curframe = vqap->data->Loader.CurFrame;
  padsize = PadSize(iffsize);

  /* Read the palette into the end of the palette buffer. */
  lcwoffset = vqap->data->Max_Pal_Size - padsize;

  // A chunk larger than the buffer would start before it.
  if (lcwoffset < 0) {
    return VQAERR_READ;
  }

  buffer = curframe->Palette + lcwoffset;

  if (vqap->io->Read(buffer, padsize)) {
    return VQAERR_READ;
  }

  /* Flag this palette as compressed. */
  curframe->Flags |= VQAFRMF_PALCOMP;
  curframe->PalOffset = lcwoffset;
  curframe->PaletteSize = iffsize;

  return 0;
}

/****************************************************************************
 *
 * NAME
 *     Load_VPT0 - Load uncompressed pointers.
 *
 * SYNOPSIS
 *     Error = Load_VPT0(VQA, Iffsize)
 *
 *     long Load_VPT0(VQAHandle *, int32_t);
 *
 * FUNCTION
 *
 * INPUTS
 *     VQA     - Pointer to private VQA handle.
 *     Iffsize - Size of IFF chunk.
 *
 * RESULT
 *     Error - 0 if successful or VQAERR_??? error code.
 *
 ****************************************************************************/

static long Load_VPT0(VQAHandle* vqap, int32_t iffsize) {
  VQAFrameNode* curframe;

  /* Dereference commonly used data members for quicker access. */
  curframe = vqap->data->Loader.CurFrame;

  if (!FitsInBuffer(0, PadSize(iffsize), vqap->data->Max_Ptr_Size)) {
    return VQAERR_READ;
  }

  /* Read the pointers into start of the pointer buffer. */
  if (vqap->io->Read(curframe->Pointers, PadSize(iffsize))) {
    return VQAERR_READ;
  }

  /* Flag this frame as uncompressed */
  curframe->Flags &= ~VQAFRMF_PTRCOMP;
  curframe->PtrOffset = 0;

  return 0;
}

/****************************************************************************
 *
 * NAME
 *     Load_VPTZ - Load compressed pointers.
 *
 * SYNOPSIS
 *     Error = Load_VPTZ(VQA, Iffsize)
 *
 *     long Load_VPTZ(VQAHandle *, int32_t);
 *
 * FUNCTION
 *
 * INPUTS
 *     VQA     - Pointer to private VQA handle.
 *     Iffsize - Size of IFF chunk.
 *
 * RESULT
 *     Error - 0 if successful or VQAERR_??? error code.
 *
 ****************************************************************************/

static long Load_VPTZ(VQAHandle* vqap, int32_t iffsize) {
  VQAFrameNode* curframe;
  void* buffer;
  int32_t padsize;
  int32_t lcwoffset;

  /* Dereference commonly used data members for quicker access. */
  curframe = vqap->data->Loader.CurFrame;
  padsize = PadSize(iffsize);
  lcwoffset = vqap->data->Max_Ptr_Size - padsize;

  // A chunk larger than the buffer would start before it.
  if (lcwoffset < 0) {
    return VQAERR_READ;
  }

  /* Read the pointers into end of the pointer buffer. */
  buffer = curframe->Pointers + lcwoffset;

  if (vqap->io->Read(buffer, padsize)) {
    return VQAERR_READ;
  }

  /* Flag this frame as compressed. */
  curframe->Flags |= VQAFRMF_PTRCOMP;
  curframe->PtrOffset = lcwoffset;

  return 0;
}

/****************************************************************************
 *
 * NAME
 *     Load_SND0 - Load uncompressed sound chunk.
 *
 * SYNOPSIS
 *     Error = Load_SND0(VQA, Iffsize)
 *
 *     long Load_SND0(VQAHandle *, int32_t);
 *
 * FUNCTION
 *     This routine normally loads the chunk into the TempBuf, unless the
 *     chunk is larger than the temp buffer size, in which case it puts it
 *     directly into the audio buffer itself.  This assumes that the only
 *     such chunk will be the first audio chunk!
 *
 * INPUTS
 *     VQA     - Pointer to private VQA handle.
 *     Iffsize - Size of IFF chunk.
 *
 * RESULT
 *     Error - 0 if successful or VQAERR_??? error code.
 *
 ****************************************************************************/

static long Load_SND0(VQAHandle* vqap, int32_t iffsize) {
  VQAData* vqabuf;
  VQAAudio* audio;
  VQAConfig* config;
  int32_t padsize;
  int32_t i;

  /* Dereference commonly used data members for quicker access. */
  vqabuf = vqap->data;
  audio = &vqabuf->Audio;
  config = &vqap->config;
  padsize = PadSize(iffsize);

  /* If sound is disabled, or if we're playing from a VOC file, or if
   * there's no Audio Buffer, just skip the chunk.
   */
  if ((config->OptionFlags & VQAOPTF_AUDIO) == 0 || audio->Buffer == nullptr) {
    if (vqap->io->Seek(padsize, SEEK_CUR)) {
      return VQAERR_SEEK;
    }
    return 0;
  }

  /* Read large startup chunk directly into AudioBuf */
  if (padsize > audio->TempBufSize && audio->AudBufPos == 0) {
    if (padsize > config->AudioBufSize) {
      return VQAERR_READ;
    }

    if (vqap->io->Read(audio->Buffer, padsize)) {
      return VQAERR_READ;
    }

    audio->AudBufPos += iffsize;

    /* Flag the audio frame flags as loaded for the initial audio frame. */
    for (i = 0; i < iffsize / config->HMIBufSize; i++) {
      audio->IsLoaded[i] = 1;
    }

    return 0;
  }
  // Only the first audio chunk may exceed TempBuf; it is handled above.
  if (padsize > audio->TempBufSize) {
    return VQAERR_READ;
  }

  /*  Read data into TempBuf */
  if (vqap->io->Read(audio->TempBuf, padsize)) {
    return VQAERR_READ;
  }

  /* Set the TempBufLen */
  audio->TempBufLen = iffsize;

  return 0;
}

/****************************************************************************
 *
 * NAME
 *     Load_SND1 - Load compressed sound chunk.
 *
 * SYNOPSIS
 *     Error = Load_SND1(VQA, Iffsize)
 *
 *     long Load_SND1(VQAHandle *, int32_t);
 *
 * FUNCTION
 *     This routine normally loads the chunk into the TempBuf, unless the
 *     chunk is larger than the temp buffer size, in which case it puts it
 *     directly into the audio buffer itself.  This assumes that the only
 *     such chunk will be the first audio chunk!
 *
 * INPUTS
 *     VQA     - Pointer to private VQA handle.
 *     Iffsize - Size of IFF chunk.
 *
 * RESULT
 *     Error - 0 if successful or VQAERR_??? error code.
 *
 ****************************************************************************/

static long Load_SND1(VQAHandle* vqap, int32_t iffsize) {
  VQAData* vqabuf;
  VQAAudio* audio;
  VQAConfig* config;
  unsigned char* loadbuf;
  int32_t padsize;
  ZAPHeader zap;
  int32_t i;

  /* Dereference commonly used data members for quicker access. */
  vqabuf = vqap->data;
  audio = &vqabuf->Audio;
  config = &vqap->config;
  padsize = PadSize(iffsize);

  /* If sound is disabled, or if we're playing from a VOC file, or if
   * there's no Audio Buffer, just skip the chunk
   */
  if ((config->OptionFlags & VQAOPTF_AUDIO) == 0 || audio->Buffer == nullptr) {
    if (vqap->io->Seek(padsize, SEEK_CUR)) {
      return VQAERR_SEEK;
    }
    return 0;
  }

  // The ZAP header is part of the chunk; a shorter chunk would leave a
  // negative payload size.
  if (iffsize < int32_t{sizeof(ZAPHeader)}) {
    return VQAERR_READ;
  }

  /* Read the ZAP audio frame header. */
  if (vqap->io->Read(&zap, sizeof(ZAPHeader))) {
    return VQAERR_READ;
  }

  /* Adjust chunk size */
  padsize -= int32_t{sizeof(ZAPHeader)};

  /* Read large startup chunk directly into AudioBuf */
  if (std::cmp_greater(zap.UnCompSize, audio->TempBufSize) &&
      audio->AudBufPos == 0) {
    if (padsize > config->AudioBufSize ||
        std::cmp_greater(zap.UnCompSize, config->AudioBufSize)) {
      return VQAERR_READ;
    }

    /* Load RAW uncompressed data. */
    if (zap.UnCompSize == zap.CompSize) {
      if (vqap->io->Read(audio->Buffer, padsize)) {
        return VQAERR_READ;
      }
    } else {
      /* Load compressed data into the end of the buffer. */
      loadbuf = audio->Buffer + config->AudioBufSize - padsize;

      if (vqap->io->Read(loadbuf, padsize)) {
        return VQAERR_READ;
      }

      /* Uncompress the audio frame. */
      AudioUnzap(loadbuf, audio->Buffer, zap.UnCompSize);
    }

    /* Set buffer positions & flags */
    audio->AudBufPos += zap.UnCompSize;

    for (i = 0; i < zap.UnCompSize / config->HMIBufSize; i++) {
      audio->IsLoaded[i] = 1;
    }

    return 0;
  }

  // Only the first audio chunk may exceed TempBuf; it is handled above.
  if (padsize > audio->TempBufSize ||
      std::cmp_greater(zap.UnCompSize, audio->TempBufSize)) {
    return VQAERR_READ;
  }

  /* Load an audio frame. */
  if (zap.UnCompSize == zap.CompSize) {
    /* If the frame is uncompressed the load it in directly. */
    if (vqap->io->Read(audio->TempBuf, padsize)) {
      return VQAERR_READ;
    }
  } else {
    /* Load the audio frame into the end of the buffer. */
    loadbuf = audio->TempBuf + audio->TempBufSize - padsize;

    if (vqap->io->Read(loadbuf, padsize)) {
      return VQAERR_READ;
    }

    /* Uncompress the audio frame. */
    AudioUnzap(loadbuf, audio->TempBuf, zap.UnCompSize);
  }

  /* Set the TempBufLen */
  audio->TempBufLen = zap.UnCompSize;

  return 0;
}

/****************************************************************************
 *
 * NAME
 *     Load_SND2 - Load ADPCM compressed sound chunk.
 *
 * SYNOPSIS
 *     Error = Load_SND2(VQA, Iffsize)
 *
 *     long Load_SND2(VQAHandle *, int32_t);
 *
 * FUNCTION
 *     This routine normally loads the chunk into the TempBuf, unless the
 *     chunk is larger than the temp buffer size, in which case it puts it
 *     directly into the audio buffer itself.  This assumes that the only
 *     such chunk will be the first audio chunk!
 *
 * INPUTS
 *     VQA     - Pointer to private VQA handle.
 *     Iffsize - Size of IFF chunk.
 *
 * RESULT
 *     Error - 0 if successful or VQAERR_??? error code.
 *
 ****************************************************************************/

static long Load_SND2(VQAHandle* vqap, int32_t iffsize) {
  VQAData* vqabuf;
  VQAAudio* audio;
  VQAConfig* config;
  unsigned char* loadbuf;
  int32_t padsize;
  int32_t uncomp_size;
  int32_t i;

  /* Dereference commonly used data members for quicker access. */
  vqabuf = vqap->data;
  audio = &vqabuf->Audio;
  config = &vqap->config;
  padsize = PadSize(iffsize);

  /* If sound is disabled, or if we're playing from a VOC file, or if
   * there's no Audio Buffer, just skip the chunk
   */
  if ((config->OptionFlags & VQAOPTF_AUDIO) == 0 || audio->Buffer == nullptr) {
    if (vqap->io->Seek(padsize, SEEK_CUR)) {
      return VQAERR_SEEK;
    }
    return 0;
  }

  // 64-bit so an oversized chunk cannot overflow before the bounds checks.
  const int64_t uncomp_bytes =
      int64_t{iffsize} * (audio->BitsPerSample / 4);
  if (uncomp_bytes > std::max(config->AudioBufSize, audio->TempBufSize)) {
    return VQAERR_READ;
  }
  uncomp_size = static_cast<int32_t>(uncomp_bytes);

  /* Read large startup chunk directly into AudioBuf */
  if (uncomp_size > audio->TempBufSize && audio->AudBufPos == 0) {
    if (padsize > config->AudioBufSize || uncomp_size > config->AudioBufSize) {
      return VQAERR_READ;
    }

    /* Load compressed data into the end of the buffer. */
    loadbuf = audio->Buffer + config->AudioBufSize - padsize;

    if (vqap->io->Read(loadbuf, padsize)) {
      return VQAERR_READ;
    }

    /* Uncompress the audio frame. */
    audio->ADPCM_Info.source = loadbuf;
    audio->ADPCM_Info.dest = audio->Buffer;
    DecompressVqaSosData(&audio->ADPCM_Info, uncomp_size);

    /* Set buffer positions & flags */
    audio->AudBufPos += uncomp_size;

    for (i = 0; i < uncomp_size / config->HMIBufSize; i++) {
      audio->IsLoaded[i] = 1;
    }

    return 0;
  }

  // Only the first audio chunk may exceed TempBuf; it is handled above.
  if (padsize > audio->TempBufSize || uncomp_size > audio->TempBufSize) {
    return VQAERR_READ;
  }

  /* Load an audio frame. */
  loadbuf = audio->TempBuf + audio->TempBufSize - padsize;

  if (vqap->io->Read(loadbuf, padsize)) {
    return VQAERR_READ;
  }

  /* Uncompress the audio frame. */
  audio->ADPCM_Info.source = loadbuf;
  audio->ADPCM_Info.dest = audio->TempBuf;
  DecompressVqaSosData(&audio->ADPCM_Info, uncomp_size);

  /* Set the TempBufLen */
  audio->TempBufLen = uncomp_size;

  return 0;
}
