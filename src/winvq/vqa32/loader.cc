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
 *     OpenVqa      - Open a VQA file to play.
 *     CloseVqa     - Close an opened VQA file.
 *     LoadNextFrame - Load the next video frame from the VQA data stream.
 *     SeekVqaFrame - Position the movie stream to the specified frame.
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
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "base/buffer.h"
#include "base/numeric.h"
#include "base/seek_origin.h"
#include "winvq/vqa32/vqa_format.h"
#include "winvq/vqa32/vqaio.h"
#include "winvq/vqa32/vqaplay.h"
#include "winvq/vqa32/vqaplayp.h"
#include "winvq/vqm32/compress.h"
#include "winvq/vqm32/iff.h"
#include "winvq/vqm32/palette.h"
#include "winvq/vqm32/soscomp.h"

/*---------------------------------------------------------------------------
 * PRIVATE DECLARATIONS
 *-------------------------------------------------------------------------*/

static std::unique_ptr<VqaMovie> AllocBuffers(const VqaHeader* header,
                                              VqaConfig* config);
static int32_t PrimeBuffers(VqaPlayerState* vqa);
static int32_t Load_VQF(VqaPlayerState* vqap, int32_t iffsize);
static int32_t Load_FINF(const VqaPlayerState* vqap, int32_t iffsize);
static int32_t Load_CBF0(const VqaPlayerState* vqap, int32_t iffsize);
static int32_t Load_CBFZ(const VqaPlayerState* vqap, int32_t iffsize);
static int32_t Load_CBP0(const VqaPlayerState* vqap, int32_t iffsize);
static int32_t Load_CBPZ(const VqaPlayerState* vqap, int32_t iffsize);
static int32_t Load_CPL0(const VqaPlayerState* vqap, int32_t iffsize);
static int32_t Load_CPLZ(const VqaPlayerState* vqap, int32_t iffsize);
static int32_t Load_VPT0(const VqaPlayerState* vqap, int32_t iffsize);
static int32_t Load_VPTZ(const VqaPlayerState* vqap, int32_t iffsize);
static int32_t Load_SND0(VqaPlayerState* vqap, int32_t iffsize);
static int32_t Load_SND1(VqaPlayerState* vqap, int32_t iffsize);
static int32_t Load_SND2(VqaPlayerState* vqap, int32_t iffsize);

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
 *     OpenVqa - Open a VQA file to play.
 *
 * SYNOPSIS
 *     Error = OpenVqa(VQA, Name, Config)
 *
 *     long OpenVqa(VqaPlayerState *, char *, VqaConfig *);
 *
 * FUNCTION
 *     - Open a VQA file for reading.
 *     - Validate that it is an IFF file, of the VQA type.
 *     - Read the VQA header.
 *     - Open a VOC file for playback, if requested.
 *     - Set the Loader's frame rate, if the caller's Config structure's
 *       frame_rate is set to -1
 *     - Set the Drawer's frame rate, if the caller's Config structure's
 *       draw_rate is set to -1
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

int32_t OpenVqa(VqaPlayerState* vqa, std::string_view filename,
                VqaConfig* config) {
  ChunkHeader chunk{};

  /* Dereference commonly used data members for quicker access. */
  VqaPlayerState* vqap = vqa;
  VqaHeader* header = &vqap->header;

  vqa_movie_loaded = false;
  /*-------------------------------------------------------------------------
   * VERIFY VALIDITY OF VQA FILE.
   *-----------------------------------------------------------------------*/

  /* Open the file. */
  if (!vqap->io->Open(filename)) {
    return kVqaErrorOpen;
  }

  /* Read the file ID & Size */
  if (!vqap->io->ReadObject(chunk)) {
    CloseVqa(vqa);
    return kVqaErrorRead;
  }

  /* Verify an IFF FORM */
  if (chunk.id != ID_FORM || chunk.size == 0) {
    CloseVqa(vqa);
    return kVqaErrorNotVqa;
  }

  /* Read in WVQA ID */
  if (!vqap->io->ReadObject(chunk.id)) {
    CloseVqa(vqa);
    return kVqaErrorRead;
  }

  /* Verify VQA */
  if (chunk.id != kFormWvqa) {
    CloseVqa(vqa);
    return kVqaErrorNotVqa;
  }

  /*-------------------------------------------------------------------------
   * INITIALIZE THE PLAYERS CONFIGURATION
   *-----------------------------------------------------------------------*/

  /* Use the clients configuration if they provided one. */
  if (config != nullptr) {
    vqap->config = *config;
  } else {
    SetVqaConfigDefaults(&vqap->config);
  }

  /* Use the internal configuration structure from now on. */
  config = &vqap->config;

  /*-------------------------------------------------------------------------
   * PROCESS THE PRE-FRAME CHUNKS (VQHD, CAP, FINF, ETC...)
   *-----------------------------------------------------------------------*/
  int32_t done = 0;

  while (!done) {
    if (!vqap->io->ReadObject(chunk)) {
      CloseVqa(vqa);
      return kVqaErrorRead;
    }

    const int32_t chunk_size = ChunkSize(chunk);

    // A negative size would make the skip below seek backwards.
    if (!IsValidChunkSize(chunk_size)) {
      CloseVqa(vqa);
      return kVqaErrorNotVqa;
    }

    switch (chunk.id) {
      /*---------------------------------------------------------------------
       * READ IN THE VQA HEADER.
       *-------------------------------------------------------------------*/
      case kChunkVqhd:
        // A second header would leak the first header's buffers.
        if (std::cmp_not_equal(chunk_size, sizeof(VqaHeader)) ||
            vqap->movie != nullptr) {
          CloseVqa(vqa);
          return kVqaErrorNotVqa;
        }

        /* Read the header data, and skip the pad byte of an odd chunk. */
        if (!vqap->io->ReadObject(*header) ||
            !vqap->io->Seek(PadSize(chunk_size) - chunk_size,
                            SeekOrigin::kCurrent)) {
          CloseVqa(vqa);
          return kVqaErrorRead;
        }

        // These fields are divisors when sizing buffers and timing playback.
        if (header->block_width == 0 || header->block_height == 0 ||
            header->frames_per_group == 0 || header->fps == 0) {
          CloseVqa(vqa);
          return kVqaErrorNotVqa;
        }

        /*-------------------------------------------------------------------
         * SETUP THE CONFIGURATION FROM THE HEADER.
         *-----------------------------------------------------------------*/
        if (config->image_width == -1) {
          config->image_width = header->image_width;
        }

        if (config->image_height == -1) {
          config->image_height = header->image_height;
        }

        /* If Loaders frame rate is -1 then use the value from the header. */
        if (config->frame_rate == -1) {
          config->frame_rate = header->fps;
        }

        /* If Drawers frame rate is -1 then use the value from the header,
         * which will result in a "variable" frame rate.
         */
        if (config->draw_rate == -1) {
          config->draw_rate = header->fps;
        }

        /* Finally, if the draw_rate was set to -1 or 0 (ie MaxRate contained
         * bogus values), set it to the header value.
         */
        if (config->draw_rate == -1 || config->draw_rate == 0) {
          config->draw_rate = header->fps;
        }

        /* If an alternate audio track is not available then turn it off.
         * This enables the primary audio track to be played.
         */
        if (header->version > kVqaVersion1 &&
            (header->flags & kVqaHasAltAudio) == 0) {
          config->option_flags &= ~kVqaOptionAltAudio;
        }

        /*-------------------------------------------------------------------
         * ALLOCATE THE BUFFERS THAT WE NEED TO PLAY THE VQA.
         *-----------------------------------------------------------------*/
        // The audio setup divides by the HMI buffer size.
        if ((header->flags & kVqaHasAudio) != 0 &&
            (config->option_flags & kVqaOptionAudio) != 0 &&
            config->audio_block_bytes <= 0) {
          CloseVqa(vqa);
          return kVqaErrorAudio;
        }

        vqap->movie = AllocBuffers(header, config);
        if (vqap->movie == nullptr) {
          CloseVqa(vqa);
          return kVqaErrorNoMemory;
        }

        break;

      /*---------------------------------------------------------------------
       * READ FRAME INFORMATION
       *-------------------------------------------------------------------*/
      case kChunkFinf:
        // The frame table is sized from the header, so it must come first.
        if (vqap->movie == nullptr) {
          CloseVqa(vqa);
          return kVqaErrorNotVqa;
        }

        if (Load_FINF(vqap, chunk_size)) {
          CloseVqa(vqa);
          return kVqaErrorRead;
        }

        done = 1;
        break;

      default:
        if (!vqap->io->Seek(PadSize(chunk_size), SeekOrigin::kCurrent)) {
          CloseVqa(vqa);
          return kVqaErrorSeek;
        }
        break;
    }
  }

  /*-------------------------------------------------------------------------
   * INITIALIZE THE VIDEO SYSTEM IF WE ARE REQUIRED TO HANDLE THAT.
   *-----------------------------------------------------------------------*/
  vqap->movie->VBIBit = config->VBIBit;

  /*-------------------------------------------------------------------------
   * AUDIO TRACK OVERRIDE FROM EXTERNAL FILE (.VOC)
   *-----------------------------------------------------------------------*/

  /* Open VOC file if one is requested. */

  /* If the movie does not contain an audio track make sure we won't try
   * to play one.
   */
  if ((header->flags & kVqaHasAudio) == 0) {
    config->option_flags &= ~kVqaOptionAudio;
  }

  /*-------------------------------------------------------------------------
   * INITIALIZE THE AUDIO PLAYBACK/TIMING SYSTEM.
   *-----------------------------------------------------------------------*/
  if (config->option_flags & kVqaOptionAudio) {
    /* Dereference for quick access. */
    VqaAudio* audio = &vqap->movie->audio;

    /* Open HMI audio resource for playback. */
    if (OpenMovieAudio(vqap)) {
      CloseVqa(vqa);
      return kVqaErrorAudio;
    }

    /* Initialize ADPCM information structure for audio stream. */
    VQA_sosCODECInitStream(&audio->adpcm);

    if (header->version == kVqaVersion1) {
      audio->adpcm.bit_size = 8;
      audio->adpcm.uncomp_size = 22050L / header->fps * header->frame_count;
      audio->adpcm.channels = 1;
    } else {
      audio->adpcm.bit_size = static_cast<int16_t>(audio->bits_per_sample);
      audio->adpcm.uncomp_size = static_cast<uint32_t>(
          audio->sample_rate / header->fps * (audio->bits_per_sample / 8) *
          audio->channels * header->frame_count);

      audio->adpcm.channels = static_cast<int16_t>(audio->channels);
    }

    audio->adpcm.comp_size = audio->adpcm.uncomp_size /
                             static_cast<uint32_t>(audio->adpcm.bit_size / 4);
  }

  /*-------------------------------------------------------------------------
   * PRIME THE BUFFERS BY PRE-LOADING THEM WITH FRAME DATA.
   *-----------------------------------------------------------------------*/
  if (PrimeBuffers(vqa) != 0) {
    CloseVqa(vqa);
    return kVqaErrorRead;
  }

  return 0;
}

/****************************************************************************
 *
 * NAME
 *     CloseVqa - Close an opened VQA file.
 *
 * SYNOPSIS
 *     CloseVqa(VQA)
 *
 *     void CloseVqa(VqaPlayerState *);
 *
 * FUNCTION
 *     Close the file that was opened with OpenVqa().
 *
 * INPUTS
 *     VQA - Pointer VqaPlayerState to close.
 *
 * RESULT
 *     NONE
 *
 ****************************************************************************/

void CloseVqa(VqaPlayerState* vqa) {
  auto* vqa_handle_p = vqa;
  /* Shutdown audio/timing system. */
  // Audio is open only once OpenMovieAudio() has run. A failed OpenVqa() can
  // get here earlier, with no data and no audio callback to tear down.
  if (vqa_handle_p->movie != nullptr &&
      (vqa_handle_p->movie->audio.flags & VQAAUDF_DIGIINIT) != 0) {
    CloseMovieAudio(vqa_handle_p);
  } else if ((vqa_handle_p->config.option_flags & kVqaOptionAudio) == 0) {
    VQA_StopTimerInt(vqa_handle_p);
  }

  /* Close the VQA file */
  vqa_handle_p->io->Close();

  // Also frees the play buffers.
  vqa->Reset();
}

/****************************************************************************
 *
 * NAME
 *     LoadNextFrame - Load the next video frame from the VQA data stream.
 *
 * SYNOPSIS
 *     Error = LoadNextFrame(VQA)
 *
 *     long LoadNextFrame(VqaPlayerState *);
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
 *     VQA - Pointer to VqaPlayerState structure.
 *
 * RESULT
 *     Error - 0 if successful or VQAERR_??? error code.
 *
 ****************************************************************************/

int32_t LoadNextFrame(VqaPlayerState* vqa) {
  int32_t frame_loaded = 0;

  /* Dereference commonly used data members for quicker access. */
  VqaPlayerState* vqa_handle_p = vqa;
  VqaMovie* vqabuf = vqa_handle_p->movie.get();
  VqaLoader* loader = &vqabuf->loader;
  VqaDrawer* drawer = &vqa_handle_p->movie->drawer;
  VqaFrame* curframe = loader->current_frame;
  ChunkHeader* chunk = &loader->chunk_header;

  int32_t iffsize = ChunkSize(*chunk);

  /* We have reached the end of the file if we loaded all the frames. */
  if (std::cmp_greater_equal(loader->next_frame_number,
                             vqa_handle_p->header.frame_count)) {
    return kVqaEndOfMovie;
  }

  /* If no buffer is available for loading then return. This allows the
   * drawer to service one of the buffers more readily. (We'll wait for one
   * to free up).
   */
  if (curframe->flags & kFrameLoaded) {
    loader->WaitsOnDrawer++;
    return kVqaNoBuffer;
  }

  /* If we're not sleeping, initialize */
  if (!(vqabuf->flags & kMovieLoaderAsleep)) {
    frame_loaded = 0;
    loader->frame_bytes = 0;

    /* Initialize the codebook ptr for the frame we're about to load:
     * (This frame's codebook is the last full codebook; we have to init it
     * now, because if we're on the last frame in a group, we'll get a new
     * full_codebook pointer.)
     */
    curframe->codebook = loader->full_codebook;
  }

  /*-------------------------------------------------------------------------
   * THE MAIN LOADER LOOP
   *-----------------------------------------------------------------------*/
  while (frame_loaded == 0) {
    /* Read new chunk, only if we're not sleeping */
    if (!(vqabuf->flags & kMovieLoaderAsleep)) {
      /* Read chunk ID */
      if (!vqa_handle_p->io->ReadObject(*chunk)) {
        return kVqaEndOfMovie;
      }

      iffsize = ChunkSize(*chunk);
      if (!IsValidChunkSize(iffsize)) {
        return kVqaErrorRead;
      }

      // Saturates so a run of large skipped chunks cannot overflow the stat.
      loader->frame_bytes = static_cast<int32_t>(
          std::min<int64_t>(int64_t{loader->frame_bytes} + iffsize, INT32_MAX));
    }

    /* Handle each chunk type */
    switch (chunk->id) {
      /* VQ Normal Frame */
      case kChunkVqfr:
        if (Load_VQF(vqa_handle_p, iffsize)) {
          return kVqaErrorRead;
        }

        frame_loaded = 1;
        break;

      /* VQ Key Frame */
      case kChunkVqfk:
        if (Load_VQF(vqa_handle_p, iffsize)) {
          return kVqaErrorRead;
        }

        /* Flag this frame as being key. */
        curframe->flags |= kFrameKey;
        frame_loaded = 1;
        break;

      /* Full uncompressed codebook */
      case kChunkCbf0:
        if (Load_CBF0(vqa_handle_p, iffsize)) {
          return kVqaErrorRead;
        }
        break;

      /* Full compressed codebook */
      case kChunkCbfz:
        if (Load_CBFZ(vqa_handle_p, iffsize)) {
          return kVqaErrorRead;
        }
        break;

      /* Partial uncompressed codebook */
      case kChunkCbp0:
        if (Load_CBP0(vqa_handle_p, iffsize)) {
          return kVqaErrorRead;
        }
        break;

      /* Partial compressed codebook */
      case kChunkCbpz:
        if (Load_CBPZ(vqa_handle_p, iffsize)) {
          return kVqaErrorRead;
        }
        break;

      /* Uncompressed palette */
      case kChunkCpl0:
        if (Load_CPL0(vqa_handle_p, iffsize)) {
          return kVqaErrorRead;
        }

        /* If this is the first occurance of a palette then store it now.
         * This functionality is needed for Monopoly!
         */
        if (drawer->saved_palette_bytes == 0) {
          base::CopyBytes(base::ObjectBytes(drawer->saved_palette),
                          std::as_bytes(std::span(curframe->palette)),
                          curframe->palette_bytes);
          drawer->saved_palette_bytes = curframe->palette_bytes;
        }

        /* Flag this frame as having a palette. */
        curframe->flags |= kFrameHasPalette;
        break;

      /* Compressed palette */
      case kChunkCplz:
        if (Load_CPLZ(vqa_handle_p, iffsize)) {
          return kVqaErrorRead;
        }

        /* If this is the first occurance of a palette then store it now.
         * This functionality is needed for Monopoly!
         */
        if (drawer->saved_palette_bytes == 0) {
          drawer->saved_palette_bytes = LCW_Uncompress(
              std::span(curframe->palette)
                  .subspan(base::ToSize(curframe->palette_offset)),
              drawer->saved_palette);
        }

        /* Flag this frame as having a palette. */
        curframe->flags |= kFrameHasPalette;
        break;

      /* Uncompressed pointer data */
      case kChunkVpt0:
        if (Load_VPT0(vqa_handle_p, iffsize)) {
          return kVqaErrorRead;
        }

        frame_loaded = 1;
        break;

      /* Compressed pointer data */
      case kChunkVptz:
      case kChunkVptd:
        if (Load_VPTZ(vqa_handle_p, iffsize)) {
          return kVqaErrorRead;
        }

        frame_loaded = 1;
        break;

      /* Pointer data Key (Must draw) */
      case kChunkVptk:
        if (Load_VPTZ(vqa_handle_p, iffsize)) {
          return kVqaErrorRead;
        }

        /* Flag this frame as being key. */
        curframe->flags |= kFrameKey;
        frame_loaded = 1;
        break;

        /* Uncompressed audio frame.
         *
         *  - Make sure the sound load buffer (Audio.TempBuf) is empty; if not
         *    go into a sleep state.
         *  - Load the data into TempBuf.
         */
      case kChunkSnd0:
        if (!(vqa_handle_p->config.option_flags & kVqaOptionAltAudio)) {
          /* Move the last audio frame to the play buffer. */
          if (CopyStagedAudio(vqa_handle_p) == kVqaSleeping) {
            vqabuf->flags |= kMovieLoaderAsleep;
            return kVqaSleeping;
          }
          vqabuf->flags &= ~kMovieLoaderAsleep;

          /* Load an uncompressed audio frame. */
          if (Load_SND0(vqa_handle_p, iffsize) != 0) {
            return kVqaErrorRead;
          }
        } else {
          if (!vqa_handle_p->io->Seek(PadSize(iffsize), SeekOrigin::kCurrent)) {
            return kVqaErrorSeek;
          }
        }
        break;

      case kChunkSna0:
        if (vqa_handle_p->config.option_flags & kVqaOptionAltAudio) {
          /* Move the last audio frame to the play buffer. */
          if (CopyStagedAudio(vqa_handle_p) == kVqaSleeping) {
            vqabuf->flags |= kMovieLoaderAsleep;
            return kVqaSleeping;
          }
          vqabuf->flags &= ~kMovieLoaderAsleep;

          /* Load an uncompressed audio frame. */
          if (Load_SND0(vqa_handle_p, iffsize) != 0) {
            return kVqaErrorRead;
          }
        } else {
          if (!vqa_handle_p->io->Seek(PadSize(iffsize), SeekOrigin::kCurrent)) {
            return kVqaErrorSeek;
          }
        }
        break;

      /* Compressed audio frame.
       *
       *  - Make sure the sound load buffer (Audio.TempBuf) is empty; if not
       *    go into a sleep state.
       *  - Load the data into TempBuf.
       */
      case kChunkSnd1:
        if (!(vqa_handle_p->config.option_flags & kVqaOptionAltAudio)) {
          /* Move the last audio frame to the play buffer. */
          if (CopyStagedAudio(vqa_handle_p) == kVqaSleeping) {
            vqabuf->flags |= kMovieLoaderAsleep;
            return kVqaSleeping;
          }
          vqabuf->flags &= ~kMovieLoaderAsleep;

          /* Load a compressed audio frame. */
          if (Load_SND1(vqa_handle_p, iffsize) != 0) {
            return kVqaErrorRead;
          }
        } else {
          if (!vqa_handle_p->io->Seek(PadSize(iffsize), SeekOrigin::kCurrent)) {
            return kVqaErrorSeek;
          }
        }
        break;

      case kChunkSna1:
        if (vqa_handle_p->config.option_flags & kVqaOptionAltAudio) {
          /* Move the last audio frame to the play buffer. */
          if (CopyStagedAudio(vqa_handle_p) == kVqaSleeping) {
            vqabuf->flags |= kMovieLoaderAsleep;
            return kVqaSleeping;
          }
          vqabuf->flags &= ~kMovieLoaderAsleep;

          /* Load a compressed audio frame. */
          if (Load_SND1(vqa_handle_p, iffsize) != 0) {
            return kVqaErrorRead;
          }
        } else {
          if (!vqa_handle_p->io->Seek(PadSize(iffsize), SeekOrigin::kCurrent)) {
            return kVqaErrorSeek;
          }
        }
        break;

      /* HMI ADPCM compressed audio frame.
       *
       *  - Make sure the sound load buffer (Audio.TempBuf) is empty; if not
       *    go into a sleep state.
       *  - Load the data into TempBuf.
       */
      case kChunkSnd2:
        if (!(vqa_handle_p->config.option_flags & kVqaOptionAltAudio)) {
          /* Move the last audio frame to the play buffer. */
          if (CopyStagedAudio(vqa_handle_p) == kVqaSleeping) {
            vqabuf->flags |= kMovieLoaderAsleep;
            return kVqaSleeping;
          }
          vqabuf->flags &= ~kMovieLoaderAsleep;

          /* Load a compressed audio frame. */
          if (Load_SND2(vqa_handle_p, iffsize) != 0) {
            return kVqaErrorRead;
          }
        } else {
          if (!vqa_handle_p->io->Seek(PadSize(iffsize), SeekOrigin::kCurrent)) {
            return kVqaErrorSeek;
          }
        }
        break;

      case kChunkSna2:
        if (vqa_handle_p->config.option_flags & kVqaOptionAltAudio) {
          /* Move the last audio frame to the play buffer. */
          if (CopyStagedAudio(vqa_handle_p) == kVqaSleeping) {
            vqabuf->flags |= kMovieLoaderAsleep;
            return kVqaSleeping;
          }
          vqabuf->flags &= ~kMovieLoaderAsleep;

          /* Load a compressed audio frame. */
          if (Load_SND2(vqa_handle_p, iffsize) != 0) {
            return kVqaErrorRead;
          }
        } else {
          if (!vqa_handle_p->io->Seek(PadSize(iffsize), SeekOrigin::kCurrent)) {
            return kVqaErrorSeek;
          }
        }
        break;

      /* Skip any unknown chunks. */
      default:
        if (!vqa_handle_p->io->Seek(PadSize(iffsize), SeekOrigin::kCurrent)) {
          return kVqaErrorSeek;
        }
        break;
    }
  }

  /* Update maximum frame size stat. */
  if (loader->next_frame_number > 0 &&
      loader->frame_bytes > loader->max_frame_bytes) {
    loader->max_frame_bytes = loader->frame_bytes;
  }

  /*-------------------------------------------------------------------------
   * SET UP THE FRAME FOR DRAWING.
   *-----------------------------------------------------------------------*/

  /* Set the frame # */
  curframe->frame_number = loader->next_frame_number;
  loader->next_frame_number++;

  /* Remember the last frame loaded, for status reporting. */
  loader->LastFrameNum = loader->next_frame_number;

  /* Loader is finished with this frame; tell Drawer to draw it */
  curframe->flags |= kFrameLoaded;
  loader->current_frame = curframe->next;

  return 0;
}

/****************************************************************************
 *
 * NAME
 *     SeekVqaFrame - Position the movie stream to the specified frame.
 *
 * SYNOPSIS
 *     Frame = SeekVqaFrame(VQA, Frame, FromWhere)
 *
 *     long SeekVqaFrame(VqaPlayerState *, int32_t, long);
 *
 * FUNCTION
 *     This function sets the movie stream to the new frame specified by
 *     the 'offset' parameter. 'FromWhere' is a symbolic constant that is used
 *     to specify from where in the stream offset should be applied.
 *
 * INPUTS
 *     VQA       - Pointer to VqaPlayerState of movie to seek into.
 *     Frame     - Frame to seek to.
 *     FromWhere - Relative position indicator.
 *
 * RESULT
 *     Frame - New frame position, or a negative VQAERR_ code: kVqaEndOfMovie
 *             for a frame past the end, kVqaErrorSeek for a negative frame
 *             or a movie without a frame table.
 *
 ****************************************************************************/

int32_t SeekVqaFrame(VqaPlayerState* vqa, int32_t framenum,
                     int32_t /*fromwhere*/) {
  VqaFrame* frame = nullptr;
  int32_t rc = kVqaOk;
  /* Dereference commonly used data members for quick access. */
  VqaPlayerState* vqap = vqa;
  VqaMovie* vqabuf = vqap->movie.get();
  VqaLoader* loader = &vqabuf->loader;
  VqaHeader* header = &vqap->header;
  VqaConfig* config = &vqap->config;

  VqaAudio* audio = &vqabuf->audio;

  /* Stop audio playback. */
  const int32_t audio_on = audio->flags & kAudioPlaying;
  StopMovieAudio(vqap);

  /* Make sure the requested frame is valid and the frame information
   * array is allocated before continuing.
   */
  if (framenum < 0 || vqabuf->Foff == nullptr) {
    rc = kVqaErrorSeek;
  } else if (std::cmp_greater_equal(framenum, header->frame_count)) {
    rc = kVqaEndOfMovie;
  }

  if (rc == kVqaOk) {
    /* Find and load the most recent palette. */
    if (!(config->option_flags & kVqaOptionPaletteOff)) {
      /* Get the current frame. */
      frame = loader->current_frame;

      for (int32_t i = framenum; i >= 0; i--) {
        if (FrameHasPalette(vqabuf->frame_offsets.at(base::ToSize(i)))) {
          /* Seek to the palette frame. */
          rc = vqap->io->Seek(
                   FrameByteOffset(vqabuf->frame_offsets.at(base::ToSize(i))),
                   SeekOrigin::kBegin)
                   ? kVqaOk
                   : kVqaErrorSeek;

          /* Fool the loader into thinking this frame is empty. */
          if (!rc) {
            loader->partial_count = 0;
            loader->partial_bytes = 0;
            loader->full_codebook = vqabuf->CBData;
            loader->partial_codebook = vqabuf->CBData;
            loader->next_frame_number = 0;
            frame->flags = 0;

            /* Load the frame with the palette. */
            if (LoadNextFrame(vqa) == 0) {
              /* Decompress the palette if neccessary.*/
              if (frame->flags & kFramePaletteCompressed) {
                frame->palette_bytes = LCW_Uncompress(
                    std::span(frame->palette)
                        .subspan(base::ToSize(frame->palette_offset)),
                    frame->palette);
              }

              SetPalette(frame->palette, frame->palette_bytes, 0);
            }
          } else {
            rc = kVqaErrorSeek;
          }
          break;
        }
      }
    }

    /* Build the codebook for the frame we are seeking to. */
    if (!rc) {
      /* Compute the starting group frame of the requested frame. */
      int32_t group = framenum / header->frames_per_group;
      group = group * header->frames_per_group;

      /* The codebook for the group we want to goto is found in the previous
       * group, with the exception of the very first group.
       */
      if (std::cmp_greater_equal(group, header->frames_per_group)) {
        group -= header->frames_per_group;
      }

      /* Seek to the start of the group containing the partial codebooks for
       * the target frame.
       */
      if (vqap->io->Seek(
              FrameByteOffset(vqabuf->frame_offsets.at(base::ToSize(group))),
              SeekOrigin::kBegin)) {
        /* Throw away any audio frames that were loaded. */
        if (config->option_flags & kVqaOptionAudio && !audio->ring.empty()) {
          std::ranges::fill(audio->block_loaded, 0);
          std::ranges::fill(audio->ring, 0);

          /* Position the audio buffer to 1/2 second. */
          audio->write_offset = audio->sample_rate * audio->channels *
                                (audio->bits_per_sample / 8) / 2;

          /* Mark 1/2 second of the audio buffer as loaded. */
          for (int32_t i = 0;
               i < audio->write_offset / config->audio_block_bytes; i++) {
            audio->block_loaded.at(base::ToSize(i)) = 1;
          }
        }

        /* Force the loader to the desired frame. */
        loader->partial_count = 0;
        loader->partial_bytes = 0;
        loader->full_codebook = vqabuf->CBData;
        loader->partial_codebook = vqabuf->CBData;
        loader->next_frame_number = group;

        /* Load frames up to the target frame collecting partial codebooks
         * along the way.
         */
        for (int32_t i = 0; i < framenum - group; i++) {
          /* Fool the loader into thinking the frame has been drawn. */
          loader->current_frame->flags = 0;

          audio->staged_bytes = 0;

          /* Load the frame. */
          rc = LoadNextFrame(vqa);
          if (rc != 0) {
            if (rc != kVqaNoBuffer && rc != kVqaSleeping) {
              break;
            }
            rc = 0;
          }
        }

        /* If everything is okay, then re-prime the buffers. */
        if (!rc) {
          /* Mark all the frames except the current one as empty. */
          loader->current_frame->flags = 0;
          frame = loader->current_frame->next;

          while (frame != loader->current_frame) {
            frame->flags = 0;
            frame = frame->next;
          }

          /* Set the drawer to the current frame and the loader
           * to the next.
           */
          vqabuf->drawer.current_frame = loader->current_frame;

          /* Prime the buffers for the new position. */
          rc = PrimeBuffers(vqa);

          /* An end of file is not considered and error. */
          if (rc == 0 || rc == kVqaEndOfMovie) {
            rc = framenum;
          }
        }
      } else {
        rc = kVqaErrorSeek;
      }
    }
  }

  /* Restart audio playback. */
  if (audio_on) {
    StartMovieAudio(vqap);
  }

  return rc;
}

/****************************************************************************
 *
 * NAME
 *     AllocBuffers - Allocate VQA play buffers.
 *
 * SYNOPSIS
 *     VqaMovie = AllocBuffers(Header, Config)
 *
 *     VqaMovie *AllocBuffers(VqaHeader *, VqaConfig *);
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
 *       - vqa->Drawer.image_buffer
 *       - vqa->Audio.Buffer
 *       - vqa->Audio.IsLoaded
 *       - vqa->Foff
 *
 * INPUTS
 *     Header - Pointer to VqaHeader structure.
 *     Config - Pointer to VQA configuration structure.
 *
 * RESULT
 *     VqaMovie - Pointer to initialized VqaMovie structure.
 *
 ****************************************************************************/

static std::unique_ptr<VqaMovie> AllocBuffers(const VqaHeader* header,
                                              VqaConfig* config) {
  /* Check the configuration for valid values. */
  if (config->codebook_buffer_count == 0 || config->frame_buffer_count == 0) {
    return nullptr;
  }

  /* Allocate the master structure using unique_ptr for RAII. */
  auto vqa_ptr = std::make_unique<VqaMovie>();
  VqaMovie* vqa = vqa_ptr.get();

  /*-------------------------------------------------------------------------
   * INITIALIZE THE VQA DATA STRUCTURES.
   *
   * The Max buffer sizes are computed with 1K of padding, and'd with 0xFFFC
   * to make the size divisible by 4, to ensure DWORD alignment.
   *-----------------------------------------------------------------------*/
  vqa->allocated_bytes = sizeof(VqaMovie);
  vqa->drawer.last_time = -kVqaTicksPerSecond;

  /* Set maximum codebook size. */
  // The sizes are rounded down to a multiple of four.
  vqa->codebook_capacity =
      ((header->codebook_entries * header->block_width * header->block_height) +
       250) /
      4 * 4;

  /* Set maximum palette size. */
  vqa->palette_capacity = (768 + 1024) / 4 * 4;

  /* Set maximum vector pointers size. */
  vqa->pointers_capacity =
      (((header->image_width / header->block_width) *
        (header->image_height / header->block_height) * int{sizeof(int16_t)}) +
       1024) /
      4 * 4;

  /* Set the frame number of the frame containing the last codebook. */
  vqa->loader.LastCBFrame =
      ((header->frame_count - 1) / header->frames_per_group) *
      header->frames_per_group;

  /*-------------------------------------------------------------------------
   * ALLOCATE THE CODEBOOK BUFFERS.
   *-----------------------------------------------------------------------*/
  vqa->codebooks.reserve(base::ToSize(config->codebook_buffer_count));

  for (int32_t i = 0; i < config->codebook_buffer_count; i++) {
    /* Allocate a codebook node using unique_ptr. */
    auto cbnode = std::make_unique<VqaCodebook>();

    /* Allocate the buffer storage. */
    cbnode->buffer.resize(base::ToSize(vqa->codebook_capacity));
    cbnode->Buffer = cbnode->buffer.data();

    /* Keep count of the memory usage. */
    vqa->allocated_bytes +=
        int32_t{sizeof(VqaCodebook)} + vqa->codebook_capacity;

    vqa->codebooks.push_back(std::move(cbnode));
  }

  /* Set up the circular linked list */
  for (size_t i = 0; i < vqa->codebooks.size(); i++) {
    const size_t next_idx = (i + 1) % vqa->codebooks.size();
    vqa->codebooks.at(i)->next = vqa->codebooks.at(next_idx).get();
  }

  /* Install the Codebook list */
  vqa->CBData = vqa->codebooks.at(0).get();
  vqa->loader.partial_codebook = vqa->CBData;
  vqa->loader.full_codebook = vqa->CBData;

  /*-------------------------------------------------------------------------
   * ALLOCATE THE FRAME BUFFERS.
   *-----------------------------------------------------------------------*/
  vqa->frames.reserve(base::ToSize(config->frame_buffer_count));

  for (int32_t i = 0; i < config->frame_buffer_count; i++) {
    /* Allocate a frame node using unique_ptr. */
    auto framenode = std::make_unique<VqaFrame>();

    /* Allocate the buffer storage. */
    framenode->pointers.resize(base::ToSize(vqa->pointers_capacity));
    framenode->palette.resize(base::ToSize(vqa->palette_capacity));
    framenode->Pointers = framenode->pointers.data();
    framenode->Palette = framenode->palette.data();

    framenode->codebook = vqa->CBData;

    /* Keep count of the memory usage. */
    vqa->allocated_bytes += int32_t{sizeof(VqaFrame)} + vqa->pointers_capacity +
                            vqa->palette_capacity;

    vqa->frames.push_back(std::move(framenode));
  }

  /* Set up the circular linked list */
  for (size_t i = 0; i < vqa->frames.size(); i++) {
    const size_t next_idx = (i + 1) % vqa->frames.size();
    vqa->frames.at(i)->next = vqa->frames.at(next_idx).get();
  }

  /* Install the Frame Buffer list */
  vqa->FrameData = vqa->frames.at(0).get();
  vqa->loader.current_frame = vqa->FrameData;
  vqa->drawer.current_frame = vqa->FrameData;
  vqa->flipper.drawn_frame = vqa->FrameData;

  /*-------------------------------------------------------------------------
   * ALLOCATE THE IMAGE BUFFERS IF ONE IS NOT ALREADY PROVIDED.
   *-----------------------------------------------------------------------*/
  if (config->image_buffer.empty()) {
    /* Allocate our own buffer. */
    if ((config->draw_flags & kVqaDrawToBuffer) != 0) {
      vqa->image_storage.resize(static_cast<std::size_t>(header->image_width) *
                                header->image_height);
      vqa->drawer.image_buffer = vqa->image_storage;

      /* Plugin image buffer information. */
      vqa->drawer.image_width = header->image_width;
      vqa->drawer.image_height = header->image_height;
      vqa->allocated_bytes += header->image_width * header->image_height;
    } else {
      vqa->drawer.image_width = config->image_width;
      vqa->drawer.image_height = config->image_height;
    }
  } else {
    /* Use caller provided buffer */
    vqa->drawer.image_buffer = config->image_buffer;
    vqa->drawer.image_width = config->image_width;
    vqa->drawer.image_height = config->image_height;
  }

  /*-------------------------------------------------------------------------
   * ALLOCATE AND INITIALIZE AUDIO BUFFERS AND STRUCTURES.
   *-----------------------------------------------------------------------*/
  if ((header->flags & kVqaHasAudio) != 0 &&
      (config->option_flags & kVqaOptionAudio) != 0) {
    /* Dereference audio structure for quick access. */
    VqaAudio* audio = &vqa->audio;

    /* Version 1 VQA's only supported 22050 8 bit mono audio. */
    if (header->version < kVqaVersion2) {
      audio->sample_rate = 22050;
      audio->channels = 1;
      audio->bits_per_sample = 8;
      audio->bytes_per_second = 22050;
    } else {
      if (config->option_flags & kVqaOptionAltAudio &&
          (header->flags & kVqaHasAltAudio) != 0) {
        audio->sample_rate = header->alt_sample_rate;
        audio->channels = header->alt_channels;
        audio->bits_per_sample = header->alt_bits_per_sample;
      } else {
        audio->sample_rate = header->sample_rate;
        audio->channels = header->channels;
        audio->bits_per_sample = header->bits_per_sample;
      }

      audio->bytes_per_second =
          audio->sample_rate * audio->channels * (audio->bits_per_sample / 8);
    }

    /* The default audio buffer size should be large enough to hold
     * 1.5 seconds of data.
     */
    if (config->audio_buffer_bytes == -1) {
      /* Compute the number of HMI buffers that will completly fit into
       * 1.5 seconds of audio data.
       */
      const auto i = (audio->bytes_per_second + (audio->bytes_per_second / 2)) /
                     config->audio_block_bytes;
      config->audio_buffer_bytes = config->audio_block_bytes * i;
    }

    /* Do not allocate anything if the audio buffer is zero length. */
    if (config->audio_buffer_bytes > 0) {
      /* Allocate an audio buffer if the user did not provide one.
       * Otherwise, use the user supplied buffer.
       */
      if (config->audio_buffer.empty()) {
        audio->ring_storage.resize(base::ToSize(config->audio_buffer_bytes));
        audio->ring = audio->ring_storage;

        /* Add audio buffer size to memory usage. */
        vqa->allocated_bytes += config->audio_buffer_bytes;
      } else {
        audio->ring = config->audio_buffer;
      }

      /* Allocate IsLoaded flags */
      audio->block_count =
          config->audio_buffer_bytes / config->audio_block_bytes;
      audio->block_loaded.resize(base::ToSize(audio->block_count), 0);
      audio->IsLoaded = audio->block_loaded.data();

      /* Add IsLoaded flags array to memory usage. */
      vqa->allocated_bytes +=
          audio->block_count * int32_t{sizeof(*audio->IsLoaded)};

      /* Allocate temporary staging buffer for the audio frames. */
      audio->staging_capacity =
          (audio->bytes_per_second / header->fps * 2) + 100;
      audio->staging.resize(base::ToSize(audio->staging_capacity));
      audio->TempBuf = audio->staging.data();

      /* Add temporary buffer size to memory usage. */
      vqa->allocated_bytes += audio->staging_capacity;
    }
  }

  /*-------------------------------------------------------------------------
   * ALLOCATE THE FRAME INFORMATION TABLE.
   *-----------------------------------------------------------------------*/
  vqa->frame_offsets.resize(header->frame_count);
  vqa->Foff = vqa->frame_offsets.data();

  /* Keep a running total of memory usage. */
  vqa->allocated_bytes += header->frame_count * int32_t{sizeof(*vqa->Foff)};

  return vqa_ptr;
}

/****************************************************************************
 *
 * NAME
 *     PrimeBuffers - Pre-Load the internal buffers.
 *
 * SYNOPSIS
 *     Error = PrimeBuffers(VQA)
 *
 *     long = PrimeBuffers(VqaPlayerState *);
 *
 * FUNCTION
 *     Pre-load the internal buffers in order to give the player some slack
 *     in the playback of large frames.
 *
 * INPUTS
 *     VQA - Pointer to VqaPlayerState structure.
 *
 * RESULT
 *     Error - 0 if successful, or VQAERR_??? error code.
 *
 ****************************************************************************/

int32_t PrimeBuffers(VqaPlayerState* vqa) {
  /* Dereference commonly used data members for quick access. */
  VqaMovie* vqabuf = vqa->movie.get();
  VqaConfig* config = &vqa->config;

  /* Pre-load the buffers */
  for (int32_t i = 0; i < config->frame_buffer_count; i++) {
    const int32_t rc = LoadNextFrame(vqa);
    if (rc == 0) {
      vqabuf->loaded_frames++;
    } else if (rc == kVqaEndOfMovie &&
               std::cmp_greater_equal(vqabuf->loader.next_frame_number,
                                      vqa->header.frame_count)) {
      // A movie with fewer frames than buffers ends while priming. Only an
      // end of file before the last frame (a truncated movie) is an error.
      break;
    } else if (rc != kVqaNoBuffer && rc != kVqaSleeping) {
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
 *     long Load_VQF(VqaPlayerState *, int32_t);
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

static int32_t Load_VQF(VqaPlayerState* vqap, int32_t frame_iffsize) {
  int64_t bytes_loaded = 0;  // 64-bit: sums sizes up to 2^31 each.

  /* Dereference commonly used data members for quicker access. */
  VqaMovie* vqabuf = vqap->movie.get();
  VqaFrame* curframe = vqabuf->loader.current_frame;
  const int32_t framesize = PadSize(frame_iffsize);
  VqaDrawer* drawer = &vqap->movie->drawer;
  ChunkHeader* chunk = &vqabuf->loader.chunk_header;

  /*-------------------------------------------------------------------------
   * FRAME LOADING LOOP.
   *-----------------------------------------------------------------------*/
  while (bytes_loaded < framesize) {
    /* Read chunk ID */
    if (!vqap->io->ReadObject(*chunk)) {
      return kVqaEndOfMovie;
    }

    const int32_t iffsize = ChunkSize(*chunk);
    if (!IsValidChunkSize(iffsize)) {
      return kVqaErrorRead;
    }

    bytes_loaded += 8;
    bytes_loaded += PadSize(iffsize);

    /* Handle each chunk type */
    switch (chunk->id) {
      /* Full uncompressed codebook */
      case kChunkCbf0:
        if (Load_CBF0(vqap, iffsize)) {
          return kVqaErrorRead;
        }
        break;

      /* Full compressed codebook */
      case kChunkCbfz:
        if (Load_CBFZ(vqap, iffsize)) {
          return kVqaErrorRead;
        }
        break;

      /* Partial uncompressed codebook */
      case kChunkCbp0:
        if (Load_CBP0(vqap, iffsize)) {
          return kVqaErrorRead;
        }
        break;

      /* Partial compressed codebook */
      case kChunkCbpz:
        if (Load_CBPZ(vqap, iffsize)) {
          return kVqaErrorRead;
        }
        break;

      /* Uncompressed palette */
      case kChunkCpl0:
        if (Load_CPL0(vqap, iffsize)) {
          return kVqaErrorRead;
        }

        /* If this is the first occurance of a palette then store it now.
         * This functionality is needed for Monopoly!
         */
        if (drawer->saved_palette_bytes == 0) {
          base::CopyBytes(base::ObjectBytes(drawer->saved_palette),
                          std::as_bytes(std::span(curframe->palette)),
                          curframe->palette_bytes);
          drawer->saved_palette_bytes = curframe->palette_bytes;
        }

        /* Flag this frame as having a palette. */
        curframe->flags |= kFrameHasPalette;
        break;

      /* Compressed palette */
      case kChunkCplz:
        if (Load_CPLZ(vqap, iffsize)) {
          return kVqaErrorRead;
        }

        /* If this is the first occurance of a palette then store it now.
         * This functionality is needed for Monopoly!
         */
        if (drawer->saved_palette_bytes == 0) {
          drawer->saved_palette_bytes = LCW_Uncompress(
              std::span(curframe->palette)
                  .subspan(base::ToSize(curframe->palette_offset)),
              drawer->saved_palette);
        }

        /* Flag this frame as having a palette. */
        curframe->flags |= kFrameHasPalette;
        break;

      /* Uncompressed pointer data */
      case kChunkVpt0:
        if (Load_VPT0(vqap, iffsize)) {
          return kVqaErrorRead;
        }
        break;

      /* Compressed pointer data */
      case kChunkVptz:
      case kChunkVptd:
        if (Load_VPTZ(vqap, iffsize)) {
          return kVqaErrorRead;
        }
        break;

      /* Compressed pointer data */
      case kChunkVptk:
        if (Load_VPTZ(vqap, iffsize)) {
          return kVqaErrorRead;
        }

        /* Flag this frame as being key. */
        curframe->flags |= kFrameKey;
        break;

      /* An unknown chunk in the video frame is an error. */
      default:
        return kVqaErrorRead;
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
 *     long Load_FINF(VqaPlayerState *, int32_t);
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

static int32_t Load_FINF(const VqaPlayerState* vqap, int32_t iffsize) {
  VqaMovie* vqabuf = vqap->movie.get();

  // The table has one 4-byte entry per frame in the header. Copying no more
  // than that and skipping the rest keeps an oversized chunk from writing
  // past it; entries a short chunk leaves out stay zero.
  const auto table_bytes =
      static_cast<int64_t>(vqabuf->frame_offsets.size() * sizeof(uint32_t));
  const int64_t copy_bytes = std::min<int64_t>(iffsize, table_bytes);
  if (copy_bytes > 0 &&
      !vqap->io->Read(std::as_writable_bytes(std::span(vqabuf->frame_offsets))
                          .first(base::ToSize(copy_bytes)))) {
    return kVqaErrorRead;
  }

  const int64_t skip_bytes = PadSize(iffsize) - copy_bytes;
  if (skip_bytes > 0 && !vqap->io->Seek(skip_bytes, SeekOrigin::kCurrent)) {
    return kVqaErrorSeek;
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
 *     long Load_CBF0(VqaPlayerState *, int32_t);
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

static int32_t Load_CBF0(const VqaPlayerState* vqap, int32_t iffsize) {
  /* Dereference commonly used data members for quicker access. */
  VqaLoader* loader = &vqap->movie->loader;
  VqaCodebook* curcb = loader->partial_codebook;

  if (!FitsInBuffer(0, PadSize(iffsize), vqap->movie->codebook_capacity)) {
    return kVqaErrorRead;
  }

  /* Read into the start of the buffer */
  if (!vqap->io->Read(std::span(curcb->buffer), PadSize(iffsize))) {
    return kVqaErrorRead;
  }

  /* Reset the partial codebook counter. */
  loader->partial_count = 0;

  /* Flag this codebook as uncompressed. */
  curcb->flags &= ~kCodebookCompressed;
  curcb->compressed_offset = 0;

  /* Clock pointers to next CB Buffer. */
  loader->full_codebook = curcb;
  loader->partial_codebook = curcb->next;

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
 *     long Load_CBFZ(VqaPlayerState *, int32_t);
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

static int32_t Load_CBFZ(const VqaPlayerState* vqap, int32_t iffsize) {
  /* Dereference commonly used data members for quicker access. */
  VqaLoader* loader = &vqap->movie->loader;
  VqaCodebook* curcb = loader->partial_codebook;
  const int32_t padsize = PadSize(iffsize);

  /* Load the codebook into the end of the buffer. */
  const int32_t lcwoffset = vqap->movie->codebook_capacity - padsize;

  // A chunk larger than the buffer would start before it.
  if (lcwoffset < 0) {
    return kVqaErrorRead;
  }

  const auto buffer = std::span(curcb->buffer).subspan(base::ToSize(lcwoffset));

  if (!vqap->io->Read(buffer, padsize)) {
    return kVqaErrorRead;
  }

  /* Reset the partial codebook counter. */
  loader->partial_count = 0;

  /* Flag this codebook as compressed */
  curcb->flags |= kCodebookCompressed;
  curcb->compressed_offset = lcwoffset;

  /* Clock pointers to next CB Buffer */
  loader->full_codebook = curcb;
  loader->partial_codebook = curcb->next;

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
 *     long Load_CBP0(VqaPlayerState *, int32_t);
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

static int32_t Load_CBP0(const VqaPlayerState* vqap, int32_t iffsize) {
  /* Dereference commonly used data members for quicker access. */
  VqaMovie* vqabuf = vqap->movie.get();
  VqaLoader* loader = &vqabuf->loader;
  VqaCodebook* curcb = loader->partial_codebook;

  /*-------------------------------------------------------------------------
   * ASSEMBLY PARTIAL CODEBOOKS.
   *-----------------------------------------------------------------------*/

  if (!FitsInBuffer(loader->partial_bytes, PadSize(iffsize),
                    vqabuf->codebook_capacity)) {
    return kVqaErrorRead;
  }

  /* Read the partial codebook into the next position in the buffer. */
  const auto buffer =
      std::span(curcb->buffer).subspan(base::ToSize(loader->partial_bytes));

  if (!vqap->io->Read(buffer, PadSize(iffsize))) {
    return kVqaErrorRead;
  }

  /* Accumulate the partial codebook values. */
  loader->partial_bytes += iffsize;
  loader->partial_count++;

  /*-------------------------------------------------------------------------
   * PROCESS FULL CODEBOOK.
   *-----------------------------------------------------------------------*/
  if (std::cmp_equal(loader->partial_count, vqap->header.frames_per_group)) {
    /* Reset the codebook accumulator values */
    loader->partial_count = 0;
    loader->partial_bytes = 0;

    /* Flag this codebook as uncompressed */
    curcb->flags &= ~kCodebookCompressed;
    curcb->compressed_offset = 0;

    /* Go to the next codebook buffer */
    loader->full_codebook = curcb;
    loader->partial_codebook = curcb->next;
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
 *     long Load_CBPZ(VqaPlayerState *, int32_t);
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

static int32_t Load_CBPZ(const VqaPlayerState* vqap, int32_t iffsize) {
  /* Dereference commonly used data members for quicker access */
  VqaMovie* vqabuf = vqap->movie.get();
  VqaLoader* loader = &vqabuf->loader;
  VqaCodebook* curcb = loader->partial_codebook;
  const int32_t padsize = PadSize(iffsize);

  /* Attempt to compute the LCW offset into the codebook buffer by
   * multiplying the size of this chunk by the # frames/group, and adding
   * a small fudge factor on, then subtracting that from the CB buffer size.
   */
  if (loader->partial_bytes == 0) {
    // 64-bit because a large chunk times the group size overflows int32_t.
    // A negative estimate would place the codebook before the buffer.
    const int64_t cboffset =
        int64_t{vqabuf->codebook_capacity} -
        ((int64_t{padsize} * vqap->header.frames_per_group) + 100);
    if (cboffset < 0) {
      return kVqaErrorRead;
    }
    curcb->compressed_offset = static_cast<int32_t>(cboffset);
  }

  // The estimate assumes every part of the group is the size of the first
  // one, so a larger later part can still run off the end.
  if (!FitsInBuffer(int64_t{curcb->compressed_offset} + loader->partial_bytes,
                    padsize, vqabuf->codebook_capacity)) {
    return kVqaErrorRead;
  }

  /*-------------------------------------------------------------------------
   * ASSEMBLE PARTIAL CODEBOOKS.
   *-----------------------------------------------------------------------*/

  /* Read the partial codebook into the next position in the buffer. */
  const auto buffer = std::span(curcb->buffer)
                          .subspan(base::ToSize(curcb->compressed_offset +
                                                loader->partial_bytes));

  if (!vqap->io->Read(buffer, padsize)) {
    return kVqaErrorRead;
  }

  /* Accumulate partial codebook values */
  loader->partial_bytes += iffsize;
  loader->partial_count++;

  /*-------------------------------------------------------------------------
   * PROCESS FULL CODEBOOK.
   *-----------------------------------------------------------------------*/
  if (std::cmp_equal(loader->partial_count, vqap->header.frames_per_group)) {
    /* Reset the codebook accumulator values. */
    loader->partial_count = 0;
    loader->partial_bytes = 0;

    /* Flag this codebook as compressed. */
    curcb->flags |= kCodebookCompressed;

    /* Go to the next codebook buffer */
    loader->full_codebook = curcb;
    loader->partial_codebook = curcb->next;
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
 *     long Load_CPL0(VqaPlayerState *, int32_t);
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

static int32_t Load_CPL0(const VqaPlayerState* vqap, int32_t iffsize) {
  /* Dereference commonly used data members for quicker access. */
  VqaFrame* curframe = vqap->movie->loader.current_frame;

  // The loader copies a frame's palette into the drawer's 256-color palette,
  // so a larger one is malformed and would overrun that copy.
  if (!FitsInBuffer(0, PadSize(iffsize),
                    int64_t{sizeof(VqaDrawer::saved_palette)})) {
    return kVqaErrorRead;
  }

  /* Read the palette into the palette buffer */
  if (!vqap->io->Read(std::span(curframe->palette), PadSize(iffsize))) {
    return kVqaErrorRead;
  }

  /* Flag the palette as uncompressed. */
  curframe->flags &= ~kFramePaletteCompressed;
  curframe->palette_offset = 0;
  curframe->palette_bytes = iffsize;

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
 *     long Load_CPLZ(VqaPlayerState *, int32_t);
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

static int32_t Load_CPLZ(const VqaPlayerState* vqap, int32_t iffsize) {
  /* Dereference commonly used data members for quicker access. */
  VqaFrame* curframe = vqap->movie->loader.current_frame;
  const int32_t padsize = PadSize(iffsize);

  /* Read the palette into the end of the palette buffer. */
  const int32_t lcwoffset = vqap->movie->palette_capacity - padsize;

  // A chunk larger than the buffer would start before it.
  if (lcwoffset < 0) {
    return kVqaErrorRead;
  }

  const auto buffer =
      std::span(curframe->palette).subspan(base::ToSize(lcwoffset));

  if (!vqap->io->Read(buffer, padsize)) {
    return kVqaErrorRead;
  }

  /* Flag this palette as compressed. */
  curframe->flags |= kFramePaletteCompressed;
  curframe->palette_offset = lcwoffset;
  curframe->palette_bytes = iffsize;

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
 *     long Load_VPT0(VqaPlayerState *, int32_t);
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

static int32_t Load_VPT0(const VqaPlayerState* vqap, int32_t iffsize) {
  /* Dereference commonly used data members for quicker access. */
  VqaFrame* curframe = vqap->movie->loader.current_frame;

  if (!FitsInBuffer(0, PadSize(iffsize), vqap->movie->pointers_capacity)) {
    return kVqaErrorRead;
  }

  /* Read the pointers into start of the pointer buffer. */
  if (!vqap->io->Read(std::span(curframe->pointers), PadSize(iffsize))) {
    return kVqaErrorRead;
  }

  /* Flag this frame as uncompressed */
  curframe->flags &= ~kFramePointersCompressed;
  curframe->pointers_offset = 0;

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
 *     long Load_VPTZ(VqaPlayerState *, int32_t);
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

static int32_t Load_VPTZ(const VqaPlayerState* vqap, int32_t iffsize) {
  /* Dereference commonly used data members for quicker access. */
  VqaFrame* curframe = vqap->movie->loader.current_frame;
  const int32_t padsize = PadSize(iffsize);
  const int32_t lcwoffset = vqap->movie->pointers_capacity - padsize;

  // A chunk larger than the buffer would start before it.
  if (lcwoffset < 0) {
    return kVqaErrorRead;
  }

  /* Read the pointers into end of the pointer buffer. */
  const auto buffer =
      std::span(curframe->pointers).subspan(base::ToSize(lcwoffset));

  if (!vqap->io->Read(buffer, padsize)) {
    return kVqaErrorRead;
  }

  /* Flag this frame as compressed. */
  curframe->flags |= kFramePointersCompressed;
  curframe->pointers_offset = lcwoffset;

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
 *     long Load_SND0(VqaPlayerState *, int32_t);
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

static int32_t Load_SND0(VqaPlayerState* vqap, int32_t iffsize) {
  /* Dereference commonly used data members for quicker access. */
  VqaMovie* vqabuf = vqap->movie.get();
  VqaAudio* audio = &vqabuf->audio;
  VqaConfig* config = &vqap->config;
  const int32_t padsize = PadSize(iffsize);

  /* If sound is disabled, or if we're playing from a VOC file, or if
   * there's no Audio Buffer, just skip the chunk.
   */
  if ((config->option_flags & kVqaOptionAudio) == 0 || audio->ring.empty()) {
    if (!vqap->io->Seek(padsize, SeekOrigin::kCurrent)) {
      return kVqaErrorSeek;
    }
    return 0;
  }

  /* Read large startup chunk directly into audio_buffer */
  if (padsize > audio->staging_capacity && audio->write_offset == 0) {
    if (padsize > config->audio_buffer_bytes) {
      return kVqaErrorRead;
    }

    if (!vqap->io->Read(audio->ring, padsize)) {
      return kVqaErrorRead;
    }

    audio->write_offset += iffsize;

    /* Flag the audio frame flags as loaded for the initial audio frame. */
    for (int32_t i = 0; i < iffsize / config->audio_block_bytes; i++) {
      audio->block_loaded.at(base::ToSize(i)) = 1;
    }

    return 0;
  }
  // Only the first audio chunk may exceed TempBuf; it is handled above.
  if (padsize > audio->staging_capacity) {
    return kVqaErrorRead;
  }

  /*  Read data into TempBuf */
  if (!vqap->io->Read(std::span(audio->staging), padsize)) {
    return kVqaErrorRead;
  }

  /* Set the staged_bytes */
  audio->staged_bytes = iffsize;

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
 *     long Load_SND1(VqaPlayerState *, int32_t);
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

static int32_t Load_SND1(VqaPlayerState* vqap, int32_t iffsize) {
  std::span<unsigned char> loadbuf;
  ZapHeader zap{};

  /* Dereference commonly used data members for quicker access. */
  VqaMovie* vqabuf = vqap->movie.get();
  VqaAudio* audio = &vqabuf->audio;
  VqaConfig* config = &vqap->config;
  int32_t padsize = PadSize(iffsize);

  /* If sound is disabled, or if we're playing from a VOC file, or if
   * there's no Audio Buffer, just skip the chunk
   */
  if ((config->option_flags & kVqaOptionAudio) == 0 || audio->ring.empty()) {
    if (!vqap->io->Seek(padsize, SeekOrigin::kCurrent)) {
      return kVqaErrorSeek;
    }
    return 0;
  }

  // The ZAP header is part of the chunk; a shorter chunk would leave a
  // negative payload size.
  if (iffsize < int32_t{sizeof(ZapHeader)}) {
    return kVqaErrorRead;
  }

  /* Read the ZAP audio frame header. */
  if (!vqap->io->ReadObject(zap)) {
    return kVqaErrorRead;
  }

  /* Adjust chunk size */
  padsize -= int32_t{sizeof(ZapHeader)};

  /* Read large startup chunk directly into audio_buffer */
  if (std::cmp_greater(zap.UnCompSize, audio->staging_capacity) &&
      audio->write_offset == 0) {
    if (padsize > config->audio_buffer_bytes ||
        std::cmp_greater(zap.UnCompSize, config->audio_buffer_bytes)) {
      return kVqaErrorRead;
    }

    /* Load RAW uncompressed data. */
    if (zap.UnCompSize == zap.CompSize) {
      if (!vqap->io->Read(audio->ring, padsize)) {
        return kVqaErrorRead;
      }
    } else {
      /* Load compressed data into the end of the buffer. */
      loadbuf = audio->ring.subspan(
          base::ToSize(config->audio_buffer_bytes - padsize));

      if (!vqap->io->Read(loadbuf, padsize)) {
        return kVqaErrorRead;
      }

      /* Uncompress the audio frame. */
      AudioUnzap(loadbuf.data(), audio->ring.data(), zap.UnCompSize);
    }

    /* Set buffer positions & flags */
    audio->write_offset += zap.UnCompSize;

    for (int32_t i = 0; i < zap.UnCompSize / config->audio_block_bytes; i++) {
      audio->block_loaded.at(base::ToSize(i)) = 1;
    }

    return 0;
  }

  // Only the first audio chunk may exceed TempBuf; it is handled above.
  if (padsize > audio->staging_capacity ||
      std::cmp_greater(zap.UnCompSize, audio->staging_capacity)) {
    return kVqaErrorRead;
  }

  /* Load an audio frame. */
  if (zap.UnCompSize == zap.CompSize) {
    /* If the frame is uncompressed the load it in directly. */
    if (!vqap->io->Read(std::span(audio->staging), padsize)) {
      return kVqaErrorRead;
    }
  } else {
    /* Load the audio frame into the end of the buffer. */
    loadbuf = std::span(audio->staging)
                  .subspan(base::ToSize(audio->staging_capacity - padsize));

    if (!vqap->io->Read(loadbuf, padsize)) {
      return kVqaErrorRead;
    }

    /* Uncompress the audio frame. */
    AudioUnzap(loadbuf.data(), audio->TempBuf, zap.UnCompSize);
  }

  /* Set the staged_bytes */
  audio->staged_bytes = zap.UnCompSize;

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
 *     long Load_SND2(VqaPlayerState *, int32_t);
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

static int32_t Load_SND2(VqaPlayerState* vqap, int32_t iffsize) {
  std::span<unsigned char> loadbuf;

  /* Dereference commonly used data members for quicker access. */
  VqaMovie* vqabuf = vqap->movie.get();
  VqaAudio* audio = &vqabuf->audio;
  VqaConfig* config = &vqap->config;
  const int32_t padsize = PadSize(iffsize);

  /* If sound is disabled, or if we're playing from a VOC file, or if
   * there's no Audio Buffer, just skip the chunk
   */
  if ((config->option_flags & kVqaOptionAudio) == 0 || audio->ring.empty()) {
    if (!vqap->io->Seek(padsize, SeekOrigin::kCurrent)) {
      return kVqaErrorSeek;
    }
    return 0;
  }

  // 64-bit so an oversized chunk cannot overflow before the bounds checks.
  const int64_t uncomp_bytes = int64_t{iffsize} * (audio->bits_per_sample / 4);
  if (uncomp_bytes >
      std::max(config->audio_buffer_bytes, audio->staging_capacity)) {
    return kVqaErrorRead;
  }
  const auto uncomp_size = static_cast<int32_t>(uncomp_bytes);

  /* Read large startup chunk directly into audio_buffer */
  if (uncomp_size > audio->staging_capacity && audio->write_offset == 0) {
    if (padsize > config->audio_buffer_bytes ||
        uncomp_size > config->audio_buffer_bytes) {
      return kVqaErrorRead;
    }

    /* Load compressed data into the end of the buffer. */
    loadbuf =
        audio->ring.subspan(base::ToSize(config->audio_buffer_bytes - padsize));

    if (!vqap->io->Read(loadbuf, padsize)) {
      return kVqaErrorRead;
    }

    /* Uncompress the audio frame. */
    audio->adpcm.source = loadbuf;
    audio->adpcm.dest = audio->ring;
    DecompressVqaSosData(&audio->adpcm, uncomp_size);

    /* Set buffer positions & flags */
    audio->write_offset += uncomp_size;

    for (int32_t i = 0; i < uncomp_size / config->audio_block_bytes; i++) {
      audio->block_loaded.at(base::ToSize(i)) = 1;
    }

    return 0;
  }

  // Only the first audio chunk may exceed TempBuf; it is handled above.
  if (padsize > audio->staging_capacity ||
      uncomp_size > audio->staging_capacity) {
    return kVqaErrorRead;
  }

  /* Load an audio frame. */
  loadbuf = std::span(audio->staging)
                .subspan(base::ToSize(audio->staging_capacity - padsize));

  if (!vqap->io->Read(loadbuf, padsize)) {
    return kVqaErrorRead;
  }

  /* Uncompress the audio frame. */
  audio->adpcm.source = loadbuf;
  audio->adpcm.dest = audio->staging;
  DecompressVqaSosData(&audio->adpcm, uncomp_size);

  /* Set the staged_bytes */
  audio->staged_bytes = uncomp_size;

  return 0;
}
