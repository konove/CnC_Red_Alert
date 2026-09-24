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

// File: the VQA loader. OpenVqa() reads a movie's header and frame table and
// allocates its play buffers; LoadNextFrame() then reads the movie a frame at
// a time into the ring of frame buffers, assembling the codebooks and staging
// the sound on the way; SeekVqaFrame() repositions it, and CloseVqa() frees it.
//
// Originally written by Bill Randolph and Denzil E. Long, Jr. at Westwood
// Studios, August 1995.

#include <algorithm>
#include <atomic>
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
#include "winvq/vqa32/vqa_player.h"
#include "winvq/vqa32/vqa_player_state.h"
#include "winvq/vqm32/compress.h"
#include "winvq/vqm32/iff.h"
#include "winvq/vqm32/palette.h"
#include "winvq/vqm32/soscomp.h"

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

int32_t OpenVqa(VqaPlayerState* vqa, std::string_view filename,
                VqaConfig* config) {
  ChunkHeader chunk{};

  VqaPlayerState* vqap = vqa;
  VqaHeader* header = &vqap->header;

  vqa_movie_loaded.store(false, std::memory_order_relaxed);

  // The file must be an IFF FORM of type WVQA.
  if (!vqap->io->Open(filename)) {
    return kVqaErrorOpen;
  }

  if (!vqap->io->ReadObject(chunk)) {
    CloseVqa(vqa);
    return kVqaErrorRead;
  }

  if (chunk.id != ID_FORM || chunk.size == 0) {
    CloseVqa(vqa);
    return kVqaErrorNotVqa;
  }

  // The form type follows the FORM header.
  if (!vqap->io->ReadObject(chunk.id)) {
    CloseVqa(vqa);
    return kVqaErrorRead;
  }

  if (chunk.id != kFormWvqa) {
    CloseVqa(vqa);
    return kVqaErrorNotVqa;
  }

  // Play from a copy of the caller's configuration, or the defaults; the
  // header resolves its -1 fields below.
  if (config != nullptr) {
    vqap->config = *config;
  } else {
    SetVqaConfigDefaults(&vqap->config);
  }

  // Only the copy from here on.
  config = &vqap->config;

  // Read the chunks in front of the frames, up to FINF, the last of them.
  // VQHD must come before it; anything else is skipped.
  bool done = false;

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
      // The movie header, which sizes the play buffers.
      case kChunkVqhd:
        // A second header would leak the first header's buffers.
        if (std::cmp_not_equal(chunk_size, sizeof(VqaHeader)) ||
            vqap->movie != nullptr) {
          CloseVqa(vqa);
          return kVqaErrorNotVqa;
        }

        // Read the header data, and skip the pad byte of an odd chunk.
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

        // Resolve the configuration's -1 defaults from the header.
        if (config->image_width == -1) {
          config->image_width = header->image_width;
        }

        if (config->image_height == -1) {
          config->image_height = header->image_height;
        }

        if (config->frame_rate == -1) {
          config->frame_rate = header->fps;
        }

        if (config->draw_rate == -1) {
          config->draw_rate = header->fps;
        }

        // A draw_rate of 0 is unset too: the drawer divides by it.
        if (config->draw_rate == -1 || config->draw_rate == 0) {
          config->draw_rate = header->fps;
        }

        // Without an alternate track, play the primary one.
        if (header->version > kVqaVersion1 &&
            (header->flags & kVqaHasAltAudio) == 0) {
          config->option_flags &= ~kVqaOptionAltAudio;
        }

        // Allocate the play buffers the header has sized. Sizing the audio
        // ring divides by the block size.
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

      // The frame table; the frames follow it.
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

        done = true;
        break;

      // Chunks the player has no use for, such as PINF.
      default:
        if (!vqap->io->Seek(PadSize(chunk_size), SeekOrigin::kCurrent)) {
          CloseVqa(vqa);
          return kVqaErrorSeek;
        }
        break;
    }
  }

  // Play no sound when there is no audio ring block to play it from: the
  // movie has no sound track, the caller turned it off, or the ring came out
  // smaller than one block (audio_buffer_bytes below audio_block_bytes, or -1
  // when 1.5 seconds of sound is less than one block). No audio code runs for
  // such a movie, so none of it has to handle an empty ring.
  if (vqap->movie->audio.block_loaded.empty()) {
    config->option_flags &= ~kVqaOptionAudio;
  }

  // Start the sound output and the ADPCM decoder for the track.
  if (config->option_flags & kVqaOptionAudio) {
    VqaAudio* audio = &vqap->movie->audio;

    // Originally HMI's DOS sound drivers; now the SDL mixer.
    if (OpenMovieAudio(vqap)) {
      CloseVqa(vqa);
      return kVqaErrorAudio;
    }

    // The decoder state runs on from chunk to chunk, so it starts once here.
    VQA_sosCODECInitStream(&audio->adpcm);

    // The track's format, and its sizes (which nothing reads now). A version 1
    // track is always 22050 Hz 8-bit mono.
    if (header->version == kVqaVersion1) {
      audio->adpcm.bit_size = 8;
      audio->adpcm.uncomp_size = 22050 / header->fps * header->frame_count;
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

  // Preload the frame ring, so playback starts with frames in hand.
  if (PrimeBuffers(vqa) != 0) {
    CloseVqa(vqa);
    return kVqaErrorRead;
  }

  return 0;
}

void CloseVqa(VqaPlayerState* vqa) {
  auto* vqa_handle_p = vqa;
  // Audio is open only once OpenMovieAudio() has run. A failed OpenVqa() can
  // get here earlier, with no data and no audio callback to tear down.
  if (vqa_handle_p->movie != nullptr &&
      (vqa_handle_p->movie->audio.flags & kAudioOpen) != 0) {
    CloseMovieAudio(vqa_handle_p);
  }

  vqa_handle_p->io->Close();

  // Also frees the play buffers.
  vqa->Reset();
}

// Reads chunks until one completes a frame: a VQFR or VQFK container, or, in
// the older format without containers, the vector pointers, which come last.
// The codebook arrives in pieces: each frame of a group carries a partial
// codebook for the next group, and the pieces add up to a full codebook just
// before the next group's first frame is read. The first frame carries a full
// one.
//
// Loading is cooperative. When a sound chunk finds the audio ring full, the
// loader sets kMovieLoaderAsleep and returns kVqaSleeping with the chunk
// header kept, and the next call resumes inside that chunk. So full buffers or
// stalled sound never leave the loader stuck, whatever the buffer sizes.
int32_t LoadNextFrame(VqaPlayerState* vqa) {
  bool frame_loaded = false;

  VqaPlayerState* vqa_handle_p = vqa;
  VqaMovie* vqabuf = vqa_handle_p->movie.get();
  VqaLoader* loader = &vqabuf->loader;
  VqaDrawer* drawer = &vqa_handle_p->movie->drawer;
  VqaFrame* curframe = loader->current_frame;
  ChunkHeader* chunk = &loader->chunk_header;

  int32_t iffsize = ChunkSize(*chunk);

  // Every frame the header counts is loaded.
  if (std::cmp_greater_equal(loader->next_frame_number,
                             vqa_handle_p->header.frame_count)) {
    return kVqaEndOfMovie;
  }

  // The next buffer still holds a frame the drawer has not released. Wait
  // for it, which also gives the drawer the turn.
  if (curframe->flags & kFrameLoaded) {
    return kVqaNoBuffer;
  }

  // A new frame, not a resumed one, uses the last full codebook. It is taken
  // now because the last frame of a group completes the next codebook, which
  // moves full_codebook on.
  if (!(vqabuf->flags & kMovieLoaderAsleep)) {
    frame_loaded = false;

    curframe->codebook = loader->full_codebook;
  }

  while (!frame_loaded) {
    // A resumed loader is inside a chunk already.
    if (!(vqabuf->flags & kMovieLoaderAsleep)) {
      if (!vqa_handle_p->io->ReadObject(*chunk)) {
        return kVqaEndOfMovie;
      }

      iffsize = ChunkSize(*chunk);
      if (!IsValidChunkSize(iffsize)) {
        return kVqaErrorRead;
      }
    }

    switch (chunk->id) {
      // A frame container.
      case kChunkVqfr:
        if (Load_VQF(vqa_handle_p, iffsize)) {
          return kVqaErrorRead;
        }

        frame_loaded = true;
        break;

      // A key frame container.
      case kChunkVqfk:
        if (Load_VQF(vqa_handle_p, iffsize)) {
          return kVqaErrorRead;
        }

        // Flag this frame as being key.
        curframe->flags |= kFrameKey;
        frame_loaded = true;
        break;

      // Full uncompressed codebook.
      case kChunkCbf0:
        if (Load_CBF0(vqa_handle_p, iffsize)) {
          return kVqaErrorRead;
        }
        break;

      // Full compressed codebook.
      case kChunkCbfz:
        if (Load_CBFZ(vqa_handle_p, iffsize)) {
          return kVqaErrorRead;
        }
        break;

      // Partial uncompressed codebook.
      case kChunkCbp0:
        if (Load_CBP0(vqa_handle_p, iffsize)) {
          return kVqaErrorRead;
        }
        break;

      // Partial compressed codebook.
      case kChunkCbpz:
        if (Load_CBPZ(vqa_handle_p, iffsize)) {
          return kVqaErrorRead;
        }
        break;

      // Uncompressed palette.
      case kChunkCpl0:
        if (Load_CPL0(vqa_handle_p, iffsize)) {
          return kVqaErrorRead;
        }

        // Keep the movie's first palette in the drawer, a copy Westwood's
        // Monopoly read from there. The player never reads it: saved_palette
        // is written again before DrawNextFrame() uses it.
        if (drawer->saved_palette_bytes == 0) {
          base::CopyBytes(base::ObjectBytes(drawer->saved_palette),
                          std::as_bytes(std::span(curframe->palette)),
                          curframe->palette_bytes);
          drawer->saved_palette_bytes = curframe->palette_bytes;
        }

        // Flag this frame as having a palette.
        curframe->flags |= kFrameHasPalette;
        break;

      // Compressed palette.
      case kChunkCplz:
        if (Load_CPLZ(vqa_handle_p, iffsize)) {
          return kVqaErrorRead;
        }

        // Keep the movie's first palette in the drawer, a copy Westwood's
        // Monopoly read from there. The player never reads it: saved_palette
        // is written again before DrawNextFrame() uses it.
        if (drawer->saved_palette_bytes == 0) {
          drawer->saved_palette_bytes = LCW_Uncompress(
              std::span(curframe->palette)
                  .subspan(base::ToSize(curframe->palette_offset)),
              drawer->saved_palette);
        }

        // Flag this frame as having a palette.
        curframe->flags |= kFrameHasPalette;
        break;

      // Uncompressed vector pointers.
      case kChunkVpt0:
        if (Load_VPT0(vqa_handle_p, iffsize)) {
          return kVqaErrorRead;
        }

        frame_loaded = true;
        break;

      // Compressed vector pointers.
      case kChunkVptz:
      case kChunkVptd:
        if (Load_VPTZ(vqa_handle_p, iffsize)) {
          return kVqaErrorRead;
        }

        frame_loaded = true;
        break;

      // Compressed vector pointers of a key frame, which is never skipped.
      case kChunkVptk:
        if (Load_VPTZ(vqa_handle_p, iffsize)) {
          return kVqaErrorRead;
        }

        // Flag this frame as being key.
        curframe->flags |= kFrameKey;
        frame_loaded = true;
        break;

      // Sound. SND* chunks are the primary track and SNA* the alternate one;
      // the track not played is skipped. Before staging a chunk, the last
      // one's sound moves from staging into the ring; with no room there the
      // loader sleeps and resumes here.
      case kChunkSnd0:
        if (!(vqa_handle_p->config.option_flags & kVqaOptionAltAudio)) {
          if (CopyStagedAudio(vqa_handle_p) == kVqaSleeping) {
            vqabuf->flags |= kMovieLoaderAsleep;
            return kVqaSleeping;
          }
          vqabuf->flags &= ~kMovieLoaderAsleep;

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
          if (CopyStagedAudio(vqa_handle_p) == kVqaSleeping) {
            vqabuf->flags |= kMovieLoaderAsleep;
            return kVqaSleeping;
          }
          vqabuf->flags &= ~kMovieLoaderAsleep;

          if (Load_SND0(vqa_handle_p, iffsize) != 0) {
            return kVqaErrorRead;
          }
        } else {
          if (!vqa_handle_p->io->Seek(PadSize(iffsize), SeekOrigin::kCurrent)) {
            return kVqaErrorSeek;
          }
        }
        break;

      case kChunkSnd1:
        if (!(vqa_handle_p->config.option_flags & kVqaOptionAltAudio)) {
          if (CopyStagedAudio(vqa_handle_p) == kVqaSleeping) {
            vqabuf->flags |= kMovieLoaderAsleep;
            return kVqaSleeping;
          }
          vqabuf->flags &= ~kMovieLoaderAsleep;

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
          if (CopyStagedAudio(vqa_handle_p) == kVqaSleeping) {
            vqabuf->flags |= kMovieLoaderAsleep;
            return kVqaSleeping;
          }
          vqabuf->flags &= ~kMovieLoaderAsleep;

          if (Load_SND1(vqa_handle_p, iffsize) != 0) {
            return kVqaErrorRead;
          }
        } else {
          if (!vqa_handle_p->io->Seek(PadSize(iffsize), SeekOrigin::kCurrent)) {
            return kVqaErrorSeek;
          }
        }
        break;

      case kChunkSnd2:
        if (!(vqa_handle_p->config.option_flags & kVqaOptionAltAudio)) {
          if (CopyStagedAudio(vqa_handle_p) == kVqaSleeping) {
            vqabuf->flags |= kMovieLoaderAsleep;
            return kVqaSleeping;
          }
          vqabuf->flags &= ~kMovieLoaderAsleep;

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
          if (CopyStagedAudio(vqa_handle_p) == kVqaSleeping) {
            vqabuf->flags |= kMovieLoaderAsleep;
            return kVqaSleeping;
          }
          vqabuf->flags &= ~kMovieLoaderAsleep;

          if (Load_SND2(vqa_handle_p, iffsize) != 0) {
            return kVqaErrorRead;
          }
        } else {
          if (!vqa_handle_p->io->Seek(PadSize(iffsize), SeekOrigin::kCurrent)) {
            return kVqaErrorSeek;
          }
        }
        break;

      // Skip any unknown chunks.
      default:
        if (!vqa_handle_p->io->Seek(PadSize(iffsize), SeekOrigin::kCurrent)) {
          return kVqaErrorSeek;
        }
        break;
    }
  }

  // Number the frame and hand it to the drawer.
  curframe->frame_number = loader->next_frame_number;
  loader->next_frame_number++;

  curframe->flags |= kFrameLoaded;
  loader->current_frame = curframe->next;

  return 0;
}

// TODO: Three defects, none reachable from the games, which never seek. A
// loader asleep in a sound chunk (kMovieLoaderAsleep) stays asleep, so the
// first load after the seek resumes that chunk at the new file position and
// reads frame data as sound. play_block is not reset with the audio ring, so
// the sound starts from a stale block, out of step with the frames. And the
// movie clock is not moved to the target frame, so the drawer waits until it
// catches up by itself.
int32_t SeekVqaFrame(VqaPlayerState* vqa, int32_t framenum) {
  VqaFrame* frame = nullptr;
  int32_t rc = kVqaOk;
  VqaPlayerState* vqap = vqa;
  VqaMovie* vqabuf = vqap->movie.get();
  VqaLoader* loader = &vqabuf->loader;
  VqaHeader* header = &vqap->header;
  VqaConfig* config = &vqap->config;

  VqaAudio* audio = &vqabuf->audio;

  // The ring is about to be refilled.
  const int32_t audio_on = audio->flags & kAudioPlaying;
  StopMovieAudio(vqap);

  if (framenum < 0) {
    rc = kVqaErrorSeek;
  } else if (std::cmp_greater_equal(framenum, header->frame_count)) {
    rc = kVqaEndOfMovie;
  }

  if (rc == kVqaOk) {
    // Set the palette in force at the target: load the nearest frame at or
    // before it that carries one, and set its palette now.
    if (!(config->option_flags & kVqaOptionPaletteOff)) {
      frame = loader->current_frame;

      for (int32_t i = framenum; i >= 0; i--) {
        if (FrameHasPalette(vqabuf->frame_offsets.at(base::ToSize(i)))) {
          rc = vqap->io->Seek(
                   FrameByteOffset(vqabuf->frame_offsets.at(base::ToSize(i))),
                   SeekOrigin::kBegin)
                   ? kVqaOk
                   : kVqaErrorSeek;

          // Reset the loader to load into the current buffer, whatever it
          // holds.
          if (!rc) {
            loader->partial_count = 0;
            loader->partial_bytes = 0;
            loader->full_codebook = vqabuf->codebooks.front().get();
            loader->partial_codebook = vqabuf->codebooks.front().get();
            loader->next_frame_number = 0;
            frame->flags = 0;

            if (LoadNextFrame(vqa) == 0) {
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

    // Rebuild the target's codebook, then load from the target on.
    if (!rc) {
      int32_t group = framenum / header->frames_per_group;
      group = group * header->frames_per_group;

      // A group's codebook arrives in pieces during the group before it, so
      // loading starts there; the first group has its codebook whole.
      if (std::cmp_greater_equal(group, header->frames_per_group)) {
        group -= header->frames_per_group;
      }

      if (vqap->io->Seek(
              FrameByteOffset(vqabuf->frame_offsets.at(base::ToSize(group))),
              SeekOrigin::kBegin)) {
        // Drop the sound loaded so far.
        if (config->option_flags & kVqaOptionAudio && !audio->ring.empty()) {
          std::ranges::fill(audio->block_loaded, 0);
          std::ranges::fill(audio->ring, 0);

          // Start writing half a second into the ring, and count that half
          // second of silence as loaded, as the preload of a new movie would.
          audio->write_offset = audio->sample_rate * audio->channels *
                                (audio->bits_per_sample / 8) / 2;

          for (int32_t i = 0;
               i < audio->write_offset / config->audio_block_bytes; i++) {
            audio->block_loaded.at(base::ToSize(i)) = 1;
          }
        }

        // Load from the start of that group, as if the movie began there.
        loader->partial_count = 0;
        loader->partial_bytes = 0;
        loader->full_codebook = vqabuf->codebooks.front().get();
        loader->partial_codebook = vqabuf->codebooks.front().get();
        loader->next_frame_number = group;

        // Load the frames before the target only for their partial
        // codebooks: each is released as soon as it is loaded, and its sound
        // is dropped.
        for (int32_t i = 0; i < framenum - group; i++) {
          loader->current_frame->flags = 0;

          audio->staged_bytes = 0;

          rc = LoadNextFrame(vqa);
          if (rc != 0) {
            if (rc != kVqaNoBuffer && rc != kVqaSleeping) {
              break;
            }
            rc = 0;
          }
        }

        // Empty the whole frame ring, then refill it from the target.
        if (!rc) {
          loader->current_frame->flags = 0;
          frame = loader->current_frame->next;

          while (frame != loader->current_frame) {
            frame->flags = 0;
            frame = frame->next;
          }

          // The drawer starts where the loader does.
          vqabuf->drawer.current_frame = loader->current_frame;

          rc = PrimeBuffers(vqa);

          // Reaching the end while priming is not an error.
          if (rc == 0 || rc == kVqaEndOfMovie) {
            rc = framenum;
          }
        }
      } else {
        rc = kVqaErrorSeek;
      }
    }
  }

  if (audio_on) {
    StartMovieAudio(vqap);
  }

  return rc;
}

// Allocates the play buffers of a movie with this header: the codebook and
// frame rings, the image buffer when the player draws into its own, the audio
// ring and staging buffer when the sound is on, and the frame table. Resolves
// the -1 default of config->audio_buffer_bytes, and rounds it down to whole
// blocks. Returns nullptr when config asks for no codebook or frame buffers.
static std::unique_ptr<VqaMovie> AllocBuffers(const VqaHeader* header,
                                              VqaConfig* config) {
  if (config->codebook_buffer_count <= 0 || config->frame_buffer_count <= 0) {
    return nullptr;
  }

  auto vqa_ptr = std::make_unique<VqaMovie>();
  VqaMovie* vqa = vqa_ptr.get();

  vqa->drawer.last_time = -kVqaTicksPerSecond;

  // Compressed data is loaded at the end of its buffer and decompressed in
  // place towards the start, so each buffer is the decompressed size plus
  // slack (250 bytes for a codebook, 1 KiB for a palette or the vector
  // pointers): room for the output never to catch up with input still to be
  // read, even for data LCW could not shrink. The sizes are rounded down to a
  // multiple of 4, which kept the DOS buffers DWORD aligned.
  vqa->codebook_capacity =
      ((header->codebook_entries * header->block_width * header->block_height) +
       250) /
      4 * 4;

  // 256 colors of 3 bytes.
  vqa->palette_capacity = (768 + 1024) / 4 * 4;

  // Two bytes per block.
  vqa->pointers_capacity =
      (((header->image_width / header->block_width) *
        (header->image_height / header->block_height) * int{sizeof(int16_t)}) +
       1024) /
      4 * 4;

  // The codebook ring.
  vqa->codebooks.reserve(base::ToSize(config->codebook_buffer_count));

  for (int32_t i = 0; i < config->codebook_buffer_count; i++) {
    auto cbnode = std::make_unique<VqaCodebook>();

    cbnode->buffer.resize(base::ToSize(vqa->codebook_capacity));
    vqa->codebooks.push_back(std::move(cbnode));
  }

  // Link the nodes into a ring.
  for (size_t i = 0; i < vqa->codebooks.size(); i++) {
    const size_t next_idx = (i + 1) % vqa->codebooks.size();
    vqa->codebooks.at(i)->next = vqa->codebooks.at(next_idx).get();
  }

  // The loader starts assembling into the first node.
  VqaCodebook* const first_codebook = vqa->codebooks.front().get();
  vqa->loader.partial_codebook = first_codebook;
  vqa->loader.full_codebook = first_codebook;

  // The frame ring.
  vqa->frames.reserve(base::ToSize(config->frame_buffer_count));

  for (int32_t i = 0; i < config->frame_buffer_count; i++) {
    auto framenode = std::make_unique<VqaFrame>();

    framenode->pointers.resize(base::ToSize(vqa->pointers_capacity));
    framenode->palette.resize(base::ToSize(vqa->palette_capacity));

    framenode->codebook = first_codebook;
    vqa->frames.push_back(std::move(framenode));
  }

  // Link the nodes into a ring.
  for (size_t i = 0; i < vqa->frames.size(); i++) {
    const size_t next_idx = (i + 1) % vqa->frames.size();
    vqa->frames.at(i)->next = vqa->frames.at(next_idx).get();
  }

  // The loader, the drawer and the flipper all start at the first frame.
  VqaFrame* const first_frame = vqa->frames.front().get();
  vqa->loader.current_frame = first_frame;
  vqa->drawer.current_frame = first_frame;
  vqa->flipper.drawn_frame = first_frame;

  // The image buffer: the caller's; else, when the player draws, its own the
  // size of the movie; else none, and the drawer draws nothing.
  if (config->image_buffer.empty()) {
    if ((config->draw_flags & kVqaDrawToBuffer) != 0) {
      vqa->image_storage.resize(static_cast<std::size_t>(header->image_width) *
                                header->image_height);
      vqa->drawer.image_buffer = vqa->image_storage;

      vqa->drawer.image_width = header->image_width;
      vqa->drawer.image_height = header->image_height;
    } else {
      vqa->drawer.image_width = config->image_width;
      vqa->drawer.image_height = config->image_height;
    }
  } else {
    vqa->drawer.image_buffer = config->image_buffer;
    vqa->drawer.image_width = config->image_width;
    vqa->drawer.image_height = config->image_height;
  }

  // The sound's format, its ring and its staging buffer.
  if ((header->flags & kVqaHasAudio) != 0 &&
      (config->option_flags & kVqaOptionAudio) != 0) {
    VqaAudio* audio = &vqa->audio;

    // Version 1 movies only had 22050 Hz 8-bit mono sound.
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

    // By default, as many whole blocks as fit in 1.5 seconds of sound.
    if (config->audio_buffer_bytes == -1) {
      const auto i = (audio->bytes_per_second + (audio->bytes_per_second / 2)) /
                     config->audio_block_bytes;
      config->audio_buffer_bytes = config->audio_block_bytes * i;
    }
    // The ring is filled and played in whole blocks; the audio callback wraps
    // at its end in bytes and at block_count in blocks, which must agree.
    config->audio_buffer_bytes -=
        config->audio_buffer_bytes % config->audio_block_bytes;

    // Less than one block is no ring at all; OpenVqa() then turns the sound
    // off.
    if (config->audio_buffer_bytes > 0) {
      // TODO: A caller's ring is used whatever its size, while block_count
      // and the loader go by audio_buffer_bytes, so a smaller one is indexed
      // past its end. No client provides one.
      if (config->audio_buffer.empty()) {
        audio->ring_storage.resize(base::ToSize(config->audio_buffer_bytes));
        audio->ring = audio->ring_storage;
      } else {
        audio->ring = config->audio_buffer;
      }

      audio->block_count =
          config->audio_buffer_bytes / config->audio_block_bytes;
      audio->block_loaded.resize(base::ToSize(audio->block_count), 0);

      // Staging holds one chunk's sound: twice one frame's worth, for
      // chunks that run long, plus 100 bytes.
      audio->staging_capacity =
          (audio->bytes_per_second / header->fps * 2) + 100;
      audio->staging.resize(base::ToSize(audio->staging_capacity));
    }
  }

  // The FINF frame table, one entry per frame.
  vqa->frame_offsets.resize(header->frame_count);

  return vqa_ptr;
}

// Loads frames until the frame ring is full (frame_buffer_count of them) or
// the movie ends. Returns 0, or the loader's error; the end of a movie shorter
// than the ring is not one.
int32_t PrimeBuffers(VqaPlayerState* vqa) {
  VqaMovie* vqabuf = vqa->movie.get();
  VqaConfig* config = &vqa->config;

  for (int32_t i = 0; i < config->frame_buffer_count; i++) {
    const int32_t rc = LoadNextFrame(vqa);
    if (rc == kVqaEndOfMovie &&
        std::cmp_greater_equal(vqabuf->loader.next_frame_number,
                               vqa->header.frame_count)) {
      // A movie with fewer frames than buffers ends while priming. Only an
      // end of file before the last frame (a truncated movie) is an error.
      break;
    }
    if (rc != 0 && rc != kVqaNoBuffer && rc != kVqaSleeping) {
      return rc;
    }
  }

  return 0;
}

// Loads the chunks inside a VQFR or VQFK frame container of frame_iffsize
// bytes: codebooks, palette and vector pointers. Returns 0, kVqaEndOfMovie
// when the file ends inside it, or kVqaErrorRead for a bad or unknown chunk.
static int32_t Load_VQF(VqaPlayerState* vqap, int32_t frame_iffsize) {
  int64_t bytes_loaded = 0;  // 64-bit: sums sizes up to 2^31 each.

  VqaMovie* vqabuf = vqap->movie.get();
  VqaFrame* curframe = vqabuf->loader.current_frame;
  const int32_t framesize = PadSize(frame_iffsize);
  VqaDrawer* drawer = &vqap->movie->drawer;
  ChunkHeader* chunk = &vqabuf->loader.chunk_header;

  while (bytes_loaded < framesize) {
    if (!vqap->io->ReadObject(*chunk)) {
      return kVqaEndOfMovie;
    }

    const int32_t iffsize = ChunkSize(*chunk);
    if (!IsValidChunkSize(iffsize)) {
      return kVqaErrorRead;
    }

    // The chunk header, and the payload with its pad byte.
    bytes_loaded += 8;
    bytes_loaded += PadSize(iffsize);

    switch (chunk->id) {
      // Full uncompressed codebook.
      case kChunkCbf0:
        if (Load_CBF0(vqap, iffsize)) {
          return kVqaErrorRead;
        }
        break;

      // Full compressed codebook.
      case kChunkCbfz:
        if (Load_CBFZ(vqap, iffsize)) {
          return kVqaErrorRead;
        }
        break;

      // Partial uncompressed codebook.
      case kChunkCbp0:
        if (Load_CBP0(vqap, iffsize)) {
          return kVqaErrorRead;
        }
        break;

      // Partial compressed codebook.
      case kChunkCbpz:
        if (Load_CBPZ(vqap, iffsize)) {
          return kVqaErrorRead;
        }
        break;

      // Uncompressed palette.
      case kChunkCpl0:
        if (Load_CPL0(vqap, iffsize)) {
          return kVqaErrorRead;
        }

        // Keep the movie's first palette in the drawer, a copy Westwood's
        // Monopoly read from there. The player never reads it: saved_palette
        // is written again before DrawNextFrame() uses it.
        if (drawer->saved_palette_bytes == 0) {
          base::CopyBytes(base::ObjectBytes(drawer->saved_palette),
                          std::as_bytes(std::span(curframe->palette)),
                          curframe->palette_bytes);
          drawer->saved_palette_bytes = curframe->palette_bytes;
        }

        // Flag this frame as having a palette.
        curframe->flags |= kFrameHasPalette;
        break;

      // Compressed palette.
      case kChunkCplz:
        if (Load_CPLZ(vqap, iffsize)) {
          return kVqaErrorRead;
        }

        // Keep the movie's first palette in the drawer, a copy Westwood's
        // Monopoly read from there. The player never reads it: saved_palette
        // is written again before DrawNextFrame() uses it.
        if (drawer->saved_palette_bytes == 0) {
          drawer->saved_palette_bytes = LCW_Uncompress(
              std::span(curframe->palette)
                  .subspan(base::ToSize(curframe->palette_offset)),
              drawer->saved_palette);
        }

        // Flag this frame as having a palette.
        curframe->flags |= kFrameHasPalette;
        break;

      // Uncompressed vector pointers.
      case kChunkVpt0:
        if (Load_VPT0(vqap, iffsize)) {
          return kVqaErrorRead;
        }
        break;

      // Compressed vector pointers.
      case kChunkVptz:
      case kChunkVptd:
        if (Load_VPTZ(vqap, iffsize)) {
          return kVqaErrorRead;
        }
        break;

      // Compressed vector pointers of a key frame.
      case kChunkVptk:
        if (Load_VPTZ(vqap, iffsize)) {
          return kVqaErrorRead;
        }

        // Flag this frame as being key.
        curframe->flags |= kFrameKey;
        break;

      // Sound is never inside a frame container, so an unknown chunk here
      // is an error rather than something to skip.
      default:
        return kVqaErrorRead;
    }
  }

  return 0;
}

// Reads the FINF frame table of iffsize bytes into frame_offsets. Returns 0,
// or kVqaErrorRead or kVqaErrorSeek.
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

// The chunk loaders below each read one chunk's payload, iffsize bytes and
// the pad byte, into the loader's current frame or codebook node. Each returns
// 0, or kVqaErrorRead when the chunk does not fit its buffer or the read
// fails.

// Loads a full uncompressed codebook into the node being assembled, which
// becomes the full codebook; the next group's pieces go to the node after it.
static int32_t Load_CBF0(const VqaPlayerState* vqap, int32_t iffsize) {
  VqaLoader* loader = &vqap->movie->loader;
  VqaCodebook* curcb = loader->partial_codebook;

  if (!FitsInBuffer(0, PadSize(iffsize), vqap->movie->codebook_capacity)) {
    return kVqaErrorRead;
  }

  if (!vqap->io->Read(std::span(curcb->buffer), PadSize(iffsize))) {
    return kVqaErrorRead;
  }

  // A full codebook replaces any pieces collected so far.
  loader->partial_count = 0;

  curcb->flags &= ~kCodebookCompressed;
  curcb->compressed_offset = 0;

  // The next group's pieces go to the next node.
  loader->full_codebook = curcb;
  loader->partial_codebook = curcb->next;

  return 0;
}

// As Load_CBF0(), for a compressed codebook: loaded at the end of the buffer
// for Prepare_Frame() to decompress in place.
static int32_t Load_CBFZ(const VqaPlayerState* vqap, int32_t iffsize) {
  VqaLoader* loader = &vqap->movie->loader;
  VqaCodebook* curcb = loader->partial_codebook;
  const int32_t padsize = PadSize(iffsize);

  const int32_t lcwoffset = vqap->movie->codebook_capacity - padsize;

  // A chunk larger than the buffer would start before it.
  if (lcwoffset < 0) {
    return kVqaErrorRead;
  }

  const auto buffer = std::span(curcb->buffer).subspan(base::ToSize(lcwoffset));

  if (!vqap->io->Read(buffer, padsize)) {
    return kVqaErrorRead;
  }

  // A full codebook replaces any pieces collected so far.
  loader->partial_count = 0;

  curcb->flags |= kCodebookCompressed;
  curcb->compressed_offset = lcwoffset;

  // The next group's pieces go to the next node.
  loader->full_codebook = curcb;
  loader->partial_codebook = curcb->next;

  return 0;
}

// Appends one uncompressed piece of the next group's codebook. The group's
// last piece completes it, and it becomes the full codebook.
static int32_t Load_CBP0(const VqaPlayerState* vqap, int32_t iffsize) {
  VqaMovie* vqabuf = vqap->movie.get();
  VqaLoader* loader = &vqabuf->loader;
  VqaCodebook* curcb = loader->partial_codebook;

  if (!FitsInBuffer(loader->partial_bytes, PadSize(iffsize),
                    vqabuf->codebook_capacity)) {
    return kVqaErrorRead;
  }

  const auto buffer =
      std::span(curcb->buffer).subspan(base::ToSize(loader->partial_bytes));

  if (!vqap->io->Read(buffer, PadSize(iffsize))) {
    return kVqaErrorRead;
  }

  // Each piece's pad byte is overwritten by the next piece.
  loader->partial_bytes += iffsize;
  loader->partial_count++;

  // The group's last piece completes the codebook.
  if (std::cmp_equal(loader->partial_count, vqap->header.frames_per_group)) {
    loader->partial_count = 0;
    loader->partial_bytes = 0;

    curcb->flags &= ~kCodebookCompressed;
    curcb->compressed_offset = 0;

    loader->full_codebook = curcb;
    loader->partial_codebook = curcb->next;
  }

  return 0;
}

// As Load_CBP0(), for compressed pieces. They collect towards the end of the
// buffer and are decompressed together, as one, once the codebook is complete.
static int32_t Load_CBPZ(const VqaPlayerState* vqap, int32_t iffsize) {
  VqaMovie* vqabuf = vqap->movie.get();
  VqaLoader* loader = &vqabuf->loader;
  VqaCodebook* curcb = loader->partial_codebook;
  const int32_t padsize = PadSize(iffsize);

  // The group's first piece places the whole compressed codebook, whose size
  // is not known yet: estimated as this piece's size times the pieces in a
  // group, plus 100 bytes, back from the end of the buffer.
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

  const auto buffer = std::span(curcb->buffer)
                          .subspan(base::ToSize(curcb->compressed_offset +
                                                loader->partial_bytes));

  if (!vqap->io->Read(buffer, padsize)) {
    return kVqaErrorRead;
  }

  // Each piece's pad byte is overwritten by the next piece.
  loader->partial_bytes += iffsize;
  loader->partial_count++;

  // The group's last piece completes the codebook.
  if (std::cmp_equal(loader->partial_count, vqap->header.frames_per_group)) {
    loader->partial_count = 0;
    loader->partial_bytes = 0;

    curcb->flags |= kCodebookCompressed;

    loader->full_codebook = curcb;
    loader->partial_codebook = curcb->next;
  }

  return 0;
}

// Loads an uncompressed palette into the frame's palette buffer.
static int32_t Load_CPL0(const VqaPlayerState* vqap, int32_t iffsize) {
  VqaFrame* curframe = vqap->movie->loader.current_frame;

  // The loader copies a frame's palette into the drawer's 256-color palette,
  // so a larger one is malformed and would overrun that copy.
  if (!FitsInBuffer(0, PadSize(iffsize),
                    int64_t{sizeof(VqaDrawer::saved_palette)})) {
    return kVqaErrorRead;
  }

  if (!vqap->io->Read(std::span(curframe->palette), PadSize(iffsize))) {
    return kVqaErrorRead;
  }

  curframe->flags &= ~kFramePaletteCompressed;
  curframe->palette_offset = 0;
  curframe->palette_bytes = iffsize;

  return 0;
}

// Loads a compressed palette at the end of the frame's palette buffer.
// palette_bytes is the compressed size until the drawer decompresses it.
static int32_t Load_CPLZ(const VqaPlayerState* vqap, int32_t iffsize) {
  VqaFrame* curframe = vqap->movie->loader.current_frame;
  const int32_t padsize = PadSize(iffsize);

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

  curframe->flags |= kFramePaletteCompressed;
  curframe->palette_offset = lcwoffset;
  curframe->palette_bytes = iffsize;

  return 0;
}

// Loads uncompressed vector pointers into the frame's pointer buffer.
static int32_t Load_VPT0(const VqaPlayerState* vqap, int32_t iffsize) {
  VqaFrame* curframe = vqap->movie->loader.current_frame;

  if (!FitsInBuffer(0, PadSize(iffsize), vqap->movie->pointers_capacity)) {
    return kVqaErrorRead;
  }

  if (!vqap->io->Read(std::span(curframe->pointers), PadSize(iffsize))) {
    return kVqaErrorRead;
  }

  curframe->flags &= ~kFramePointersCompressed;
  curframe->pointers_offset = 0;

  return 0;
}

// Loads compressed vector pointers at the end of the frame's pointer buffer.
static int32_t Load_VPTZ(const VqaPlayerState* vqap, int32_t iffsize) {
  VqaFrame* curframe = vqap->movie->loader.current_frame;
  const int32_t padsize = PadSize(iffsize);
  const int32_t lcwoffset = vqap->movie->pointers_capacity - padsize;

  // A chunk larger than the buffer would start before it.
  if (lcwoffset < 0) {
    return kVqaErrorRead;
  }

  const auto buffer =
      std::span(curframe->pointers).subspan(base::ToSize(lcwoffset));

  if (!vqap->io->Read(buffer, padsize)) {
    return kVqaErrorRead;
  }

  curframe->flags |= kFramePointersCompressed;
  curframe->pointers_offset = lcwoffset;

  return 0;
}

// The sound chunk loaders stage a chunk's sound in audio.staging, for
// CopyStagedAudio() to move into the ring. The movie's first sound chunk may
// be larger than staging - it preloads the sound - and goes straight into the
// ring instead; a larger chunk anywhere else is an error. With the sound off
// they skip the chunk. Each returns 0, or kVqaErrorRead or kVqaErrorSeek.

// Loads an uncompressed sound chunk.
static int32_t Load_SND0(VqaPlayerState* vqap, int32_t iffsize) {
  VqaMovie* vqabuf = vqap->movie.get();
  VqaAudio* audio = &vqabuf->audio;
  VqaConfig* config = &vqap->config;
  const int32_t padsize = PadSize(iffsize);

  // No sound to load into.
  if ((config->option_flags & kVqaOptionAudio) == 0 || audio->ring.empty()) {
    if (!vqap->io->Seek(padsize, SeekOrigin::kCurrent)) {
      return kVqaErrorSeek;
    }
    return 0;
  }

  // The first chunk, too big for staging, preloads the ring.
  if (padsize > audio->staging_capacity && audio->write_offset == 0) {
    if (padsize > config->audio_buffer_bytes) {
      return kVqaErrorRead;
    }

    if (!vqap->io->Read(audio->ring, padsize)) {
      return kVqaErrorRead;
    }

    // A chunk that fills the ring exactly wraps the write back to its start.
    audio->write_offset =
        (audio->write_offset + iffsize) % config->audio_buffer_bytes;

    // Mark the whole blocks it filled; the next chunk completes a partial
    // last one.
    for (int32_t i = 0; i < iffsize / config->audio_block_bytes; i++) {
      audio->block_loaded.at(base::ToSize(i)) = 1;
    }

    return 0;
  }
  // Only the first chunk may exceed staging.
  if (padsize > audio->staging_capacity) {
    return kVqaErrorRead;
  }

  if (!vqap->io->Read(std::span(audio->staging), padsize)) {
    return kVqaErrorRead;
  }

  audio->staged_bytes = iffsize;

  return 0;
}

// Loads a sound chunk in Westwood's ZAP ADPCM, which starts with a ZapHeader.
static int32_t Load_SND1(VqaPlayerState* vqap, int32_t iffsize) {
  std::span<unsigned char> loadbuf;
  ZapHeader zap{};

  VqaMovie* vqabuf = vqap->movie.get();
  VqaAudio* audio = &vqabuf->audio;
  VqaConfig* config = &vqap->config;
  int32_t padsize = PadSize(iffsize);

  // No sound to load into.
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

  if (!vqap->io->ReadObject(zap)) {
    return kVqaErrorRead;
  }

  // The sound after the header.
  padsize -= int32_t{sizeof(ZapHeader)};

  // The first chunk, too big for staging, preloads the ring.
  if (std::cmp_greater(zap.UnCompSize, audio->staging_capacity) &&
      audio->write_offset == 0) {
    if (padsize > config->audio_buffer_bytes ||
        std::cmp_greater(zap.UnCompSize, config->audio_buffer_bytes)) {
      return kVqaErrorRead;
    }

    // Equal sizes: stored uncompressed.
    if (zap.UnCompSize == zap.CompSize) {
      if (!vqap->io->Read(audio->ring, padsize)) {
        return kVqaErrorRead;
      }
    } else {
      // Loaded at the end of the ring and decompressed towards its start.
      loadbuf = audio->ring.subspan(
          base::ToSize(config->audio_buffer_bytes - padsize));

      if (!vqap->io->Read(loadbuf, padsize)) {
        return kVqaErrorRead;
      }

      // TODO: AudioUnzap() is a stub that writes nothing, so the ring keeps
      // the compressed bytes and whatever was there before, and plays them.
      // The shipped Red Alert movies have no SND1 sound.
      AudioUnzap(loadbuf, audio->ring.first(zap.UnCompSize));
    }

    audio->write_offset =
        (audio->write_offset + zap.UnCompSize) % config->audio_buffer_bytes;

    for (int32_t i = 0; i < zap.UnCompSize / config->audio_block_bytes; i++) {
      audio->block_loaded.at(base::ToSize(i)) = 1;
    }

    return 0;
  }

  // Only the first chunk may exceed staging.
  if (padsize > audio->staging_capacity ||
      std::cmp_greater(zap.UnCompSize, audio->staging_capacity)) {
    return kVqaErrorRead;
  }

  if (zap.UnCompSize == zap.CompSize) {
    if (!vqap->io->Read(std::span(audio->staging), padsize)) {
      return kVqaErrorRead;
    }
  } else {
    // Loaded at the end of staging and decompressed towards its start.
    loadbuf = std::span(audio->staging)
                  .subspan(base::ToSize(audio->staging_capacity - padsize));

    if (!vqap->io->Read(loadbuf, padsize)) {
      return kVqaErrorRead;
    }

    // TODO: As above, AudioUnzap() writes nothing.
    AudioUnzap(loadbuf, std::span(audio->staging).first(zap.UnCompSize));
  }

  audio->staged_bytes = zap.UnCompSize;

  return 0;
}

// Loads a sound chunk in IMA ADPCM, 4 bits a sample.
static int32_t Load_SND2(VqaPlayerState* vqap, int32_t iffsize) {
  std::span<unsigned char> loadbuf;

  VqaMovie* vqabuf = vqap->movie.get();
  VqaAudio* audio = &vqabuf->audio;
  VqaConfig* config = &vqap->config;
  const int32_t padsize = PadSize(iffsize);

  // No sound to load into.
  if ((config->option_flags & kVqaOptionAudio) == 0 || audio->ring.empty()) {
    if (!vqap->io->Seek(padsize, SeekOrigin::kCurrent)) {
      return kVqaErrorSeek;
    }
    return 0;
  }

  // Two samples a byte. 64-bit so an oversized chunk cannot overflow before
  // the bounds checks.
  const int64_t uncomp_bytes = int64_t{iffsize} * (audio->bits_per_sample / 4);
  if (uncomp_bytes >
      std::max(config->audio_buffer_bytes, audio->staging_capacity)) {
    return kVqaErrorRead;
  }
  const auto uncomp_size = static_cast<int32_t>(uncomp_bytes);

  // The first chunk, too big for staging, preloads the ring.
  if (uncomp_size > audio->staging_capacity && audio->write_offset == 0) {
    if (padsize > config->audio_buffer_bytes ||
        uncomp_size > config->audio_buffer_bytes) {
      return kVqaErrorRead;
    }

    // Loaded at the end of the ring and decompressed towards its start.
    loadbuf =
        audio->ring.subspan(base::ToSize(config->audio_buffer_bytes - padsize));

    if (!vqap->io->Read(loadbuf, padsize)) {
      return kVqaErrorRead;
    }

    // TODO: A failed decode (the decoder takes only 16-bit mono) is ignored,
    // so the ring plays whatever it held. The shipped Red Alert movies are
    // all 16-bit mono.
    audio->adpcm.source = loadbuf;
    audio->adpcm.dest = audio->ring;
    DecompressVqaSosData(&audio->adpcm, uncomp_size);

    audio->write_offset =
        (audio->write_offset + uncomp_size) % config->audio_buffer_bytes;

    for (int32_t i = 0; i < uncomp_size / config->audio_block_bytes; i++) {
      audio->block_loaded.at(base::ToSize(i)) = 1;
    }

    return 0;
  }

  // Only the first chunk may exceed staging.
  if (padsize > audio->staging_capacity ||
      uncomp_size > audio->staging_capacity) {
    return kVqaErrorRead;
  }

  // Loaded at the end of staging and decompressed towards its start.
  loadbuf = std::span(audio->staging)
                .subspan(base::ToSize(audio->staging_capacity - padsize));

  if (!vqap->io->Read(loadbuf, padsize)) {
    return kVqaErrorRead;
  }

  // TODO: A failed decode is ignored here too.
  audio->adpcm.source = loadbuf;
  audio->adpcm.dest = audio->staging;
  DecompressVqaSosData(&audio->adpcm, uncomp_size);

  audio->staged_bytes = uncomp_size;

  return 0;
}
