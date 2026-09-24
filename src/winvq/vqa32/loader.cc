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
// the sound on the way; CloseVqa() frees it.
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
#include "winvq/vqm32/soscomp.h"

static std::unique_ptr<VqaMovie> AllocateMovie(const VqaHeader* header,
                                               VqaConfig* config);
static int32_t PreloadFrames(VqaPlayerState* state);
static int32_t LoadFrameContainer(VqaPlayerState* state, int32_t frame_bytes);
static int32_t LoadFullCodebook(const VqaPlayerState* state,
                                int32_t chunk_bytes);
static int32_t LoadCompressedFullCodebook(const VqaPlayerState* state,
                                          int32_t chunk_bytes);
static int32_t LoadPartialCodebook(const VqaPlayerState* state,
                                   int32_t chunk_bytes);
static int32_t LoadCompressedPartialCodebook(const VqaPlayerState* state,
                                             int32_t chunk_bytes);
static int32_t LoadPalette(const VqaPlayerState* state, int32_t chunk_bytes);
static int32_t LoadCompressedPalette(const VqaPlayerState* state,
                                     int32_t chunk_bytes);
static int32_t LoadVectorPointers(const VqaPlayerState* state,
                                  int32_t chunk_bytes);
static int32_t LoadCompressedVectorPointers(const VqaPlayerState* state,
                                            int32_t chunk_bytes);
static int32_t LoadSound(VqaPlayerState* state, int32_t chunk_bytes);
static int32_t LoadZapSound(VqaPlayerState* state, int32_t chunk_bytes);
static int32_t LoadAdpcmSound(VqaPlayerState* state, int32_t chunk_bytes);

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

int32_t OpenVqa(VqaPlayerState* state, std::string_view filename,
                VqaConfig* config) {
  ChunkHeader chunk{};

  VqaPlayerState* vqap = state;
  VqaHeader* header = &vqap->header;

  vqa_movie_loaded.store(false, std::memory_order_relaxed);

  // The file must be an IFF FORM of type WVQA.
  if (!vqap->io->Open(filename)) {
    return kVqaErrorOpen;
  }

  if (!vqap->io->ReadObject(chunk)) {
    CloseVqa(state);
    return kVqaErrorRead;
  }

  if (chunk.id != ID_FORM || chunk.size == 0) {
    CloseVqa(state);
    return kVqaErrorNotVqa;
  }

  // The form type follows the FORM header.
  if (!vqap->io->ReadObject(chunk.id)) {
    CloseVqa(state);
    return kVqaErrorRead;
  }

  if (chunk.id != kFormWvqa) {
    CloseVqa(state);
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
  bool found_frame_table = false;

  while (!found_frame_table) {
    if (!vqap->io->ReadObject(chunk)) {
      CloseVqa(state);
      return kVqaErrorRead;
    }

    const int32_t chunk_bytes = ChunkSize(chunk);

    // A negative size would make the skip below seek backwards.
    if (!IsValidChunkSize(chunk_bytes)) {
      CloseVqa(state);
      return kVqaErrorNotVqa;
    }

    switch (chunk.id) {
      // The movie header, which sizes the play buffers.
      case kChunkVqhd:
        // A second header would leak the first header's buffers.
        if (std::cmp_not_equal(chunk_bytes, sizeof(VqaHeader)) ||
            vqap->movie != nullptr) {
          CloseVqa(state);
          return kVqaErrorNotVqa;
        }

        // Read the header data, and skip the pad byte of an odd chunk.
        if (!vqap->io->ReadObject(*header) ||
            !vqap->io->Seek(PadSize(chunk_bytes) - chunk_bytes,
                            SeekOrigin::kCurrent)) {
          CloseVqa(state);
          return kVqaErrorRead;
        }

        // These fields are divisors when sizing buffers and timing playback.
        if (header->block_width == 0 || header->block_height == 0 ||
            header->frames_per_group == 0 || header->fps == 0) {
          CloseVqa(state);
          return kVqaErrorNotVqa;
        }

        // Resolve the configuration's -1 defaults from the header.
        if (config->image_width == -1) {
          config->image_width = header->image_width;
        }

        if (config->image_height == -1) {
          config->image_height = header->image_height;
        }

        // A frame_rate of 0 is unset too: the clock divides by it.
        if (config->frame_rate == -1 || config->frame_rate == 0) {
          config->frame_rate = header->fps;
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
          CloseVqa(state);
          return kVqaErrorAudio;
        }

        vqap->movie = AllocateMovie(header, config);
        if (vqap->movie == nullptr) {
          CloseVqa(state);
          return kVqaErrorNoMemory;
        }

        break;

      // The frame table, which the player does not use, is the last chunk
      // before the frames. A movie without a header before it cannot play.
      case kChunkFinf:
        if (vqap->movie == nullptr) {
          CloseVqa(state);
          return kVqaErrorNotVqa;
        }

        if (!vqap->io->Seek(PadSize(chunk_bytes), SeekOrigin::kCurrent)) {
          CloseVqa(state);
          return kVqaErrorSeek;
        }

        found_frame_table = true;
        break;

      // Chunks the player has no use for, such as PINF.
      default:
        if (!vqap->io->Seek(PadSize(chunk_bytes), SeekOrigin::kCurrent)) {
          CloseVqa(state);
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
      CloseVqa(state);
      return kVqaErrorAudio;
    }

    // The decoder state runs on from chunk to chunk, so it starts once here.
    ResetAdpcmStream(&audio->adpcm);

    // The track's format, and its sizes (which nothing reads now). A version 1
    // track is always 22050 Hz 8-bit mono.
    if (header->version == kVqaVersion1) {
      audio->adpcm.bits_per_sample = 8;
      audio->adpcm.uncomp_size = 22050 / header->fps * header->frame_count;
      audio->adpcm.channels = 1;
    } else {
      audio->adpcm.bits_per_sample =
          static_cast<int16_t>(audio->bits_per_sample);
      audio->adpcm.uncomp_size = static_cast<uint32_t>(
          audio->sample_rate / header->fps * (audio->bits_per_sample / 8) *
          audio->channels * header->frame_count);

      audio->adpcm.channels = static_cast<int16_t>(audio->channels);
    }

    audio->adpcm.comp_size =
        audio->adpcm.uncomp_size /
        static_cast<uint32_t>(audio->adpcm.bits_per_sample / 4);
  }

  // Preload the frame ring, so playback starts with frames in hand.
  if (PreloadFrames(state) != 0) {
    CloseVqa(state);
    return kVqaErrorRead;
  }

  return 0;
}

void CloseVqa(VqaPlayerState* state) {
  auto* vqa_handle_p = state;
  // Audio is open only once OpenMovieAudio() has run. A failed OpenVqa() can
  // get here earlier, with no data and no audio callback to tear down.
  if (vqa_handle_p->movie != nullptr &&
      (vqa_handle_p->movie->audio.flags & kAudioOpen) != 0) {
    CloseMovieAudio(vqa_handle_p);
  }

  vqa_handle_p->io->Close();

  // Also frees the play buffers.
  state->Reset();
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
int32_t LoadNextFrame(VqaPlayerState* state) {
  bool frame_loaded = false;

  VqaPlayerState* vqa_handle_p = state;
  VqaMovie* movie = vqa_handle_p->movie.get();
  VqaLoader* loader = &movie->loader;
  VqaDrawer* drawer = &vqa_handle_p->movie->drawer;
  VqaFrame* frame = loader->current_frame;
  ChunkHeader* chunk = &loader->chunk_header;

  int32_t chunk_bytes = ChunkSize(*chunk);

  // Every frame the header counts is loaded.
  if (std::cmp_greater_equal(loader->next_frame_number,
                             vqa_handle_p->header.frame_count)) {
    return kVqaEndOfMovie;
  }

  // The next buffer still holds a frame the drawer has not released. Wait
  // for it, which also gives the drawer the turn.
  if (frame->flags & kFrameLoaded) {
    return kVqaNoBuffer;
  }

  // A new frame, not a resumed one, uses the last full codebook. It is taken
  // now because the last frame of a group completes the next codebook, which
  // moves full_codebook on.
  if (!(movie->flags & kMovieLoaderAsleep)) {
    frame_loaded = false;

    frame->codebook = loader->full_codebook;
  }

  while (!frame_loaded) {
    // A resumed loader is inside a chunk already.
    if (!(movie->flags & kMovieLoaderAsleep)) {
      if (!vqa_handle_p->io->ReadObject(*chunk)) {
        return kVqaEndOfMovie;
      }

      chunk_bytes = ChunkSize(*chunk);
      if (!IsValidChunkSize(chunk_bytes)) {
        return kVqaErrorRead;
      }
    }

    switch (chunk->id) {
      // A frame container.
      case kChunkVqfr:
        if (LoadFrameContainer(vqa_handle_p, chunk_bytes)) {
          return kVqaErrorRead;
        }

        frame_loaded = true;
        break;

      // A key frame container.
      case kChunkVqfk:
        if (LoadFrameContainer(vqa_handle_p, chunk_bytes)) {
          return kVqaErrorRead;
        }

        // Flag this frame as being key.
        frame->flags |= kFrameKey;
        frame_loaded = true;
        break;

      // Full uncompressed codebook.
      case kChunkCbf0:
        if (LoadFullCodebook(vqa_handle_p, chunk_bytes)) {
          return kVqaErrorRead;
        }
        break;

      // Full compressed codebook.
      case kChunkCbfz:
        if (LoadCompressedFullCodebook(vqa_handle_p, chunk_bytes)) {
          return kVqaErrorRead;
        }
        break;

      // Partial uncompressed codebook.
      case kChunkCbp0:
        if (LoadPartialCodebook(vqa_handle_p, chunk_bytes)) {
          return kVqaErrorRead;
        }
        break;

      // Partial compressed codebook.
      case kChunkCbpz:
        if (LoadCompressedPartialCodebook(vqa_handle_p, chunk_bytes)) {
          return kVqaErrorRead;
        }
        break;

      // Uncompressed palette.
      case kChunkCpl0:
        if (LoadPalette(vqa_handle_p, chunk_bytes)) {
          return kVqaErrorRead;
        }

        // Keep the movie's first palette in the drawer, a copy Westwood's
        // Monopoly read from there. The player never reads it: saved_palette
        // is written again before DrawNextFrame() uses it.
        if (drawer->saved_palette_bytes == 0) {
          base::CopyBytes(base::ObjectBytes(drawer->saved_palette),
                          std::as_bytes(std::span(frame->palette)),
                          frame->palette_bytes);
          drawer->saved_palette_bytes = frame->palette_bytes;
        }

        // Flag this frame as having a palette.
        frame->flags |= kFrameHasPalette;
        break;

      // Compressed palette.
      case kChunkCplz:
        if (LoadCompressedPalette(vqa_handle_p, chunk_bytes)) {
          return kVqaErrorRead;
        }

        // Keep the movie's first palette in the drawer, a copy Westwood's
        // Monopoly read from there. The player never reads it: saved_palette
        // is written again before DrawNextFrame() uses it.
        if (drawer->saved_palette_bytes == 0) {
          drawer->saved_palette_bytes =
              LCW_Uncompress(std::span(frame->palette)
                                 .subspan(base::ToSize(frame->palette_offset)),
                             drawer->saved_palette);
        }

        // Flag this frame as having a palette.
        frame->flags |= kFrameHasPalette;
        break;

      // Uncompressed vector pointers.
      case kChunkVpt0:
        if (LoadVectorPointers(vqa_handle_p, chunk_bytes)) {
          return kVqaErrorRead;
        }

        frame_loaded = true;
        break;

      // Compressed vector pointers.
      case kChunkVptz:
      case kChunkVptd:
        if (LoadCompressedVectorPointers(vqa_handle_p, chunk_bytes)) {
          return kVqaErrorRead;
        }

        frame_loaded = true;
        break;

      // Compressed vector pointers of a key frame, which is never skipped.
      case kChunkVptk:
        if (LoadCompressedVectorPointers(vqa_handle_p, chunk_bytes)) {
          return kVqaErrorRead;
        }

        // Flag this frame as being key.
        frame->flags |= kFrameKey;
        frame_loaded = true;
        break;

      // Sound. SND* chunks are the primary track and SNA* the alternate one;
      // the track not played is skipped. Before staging a chunk, the last
      // one's sound moves from staging into the ring; with no room there the
      // loader sleeps and resumes here.
      case kChunkSnd0:
        if (!(vqa_handle_p->config.option_flags & kVqaOptionAltAudio)) {
          if (CopyStagedAudio(vqa_handle_p) == kVqaSleeping) {
            movie->flags |= kMovieLoaderAsleep;
            return kVqaSleeping;
          }
          movie->flags &= ~kMovieLoaderAsleep;

          if (LoadSound(vqa_handle_p, chunk_bytes) != 0) {
            return kVqaErrorRead;
          }
        } else {
          if (!vqa_handle_p->io->Seek(PadSize(chunk_bytes),
                                      SeekOrigin::kCurrent)) {
            return kVqaErrorSeek;
          }
        }
        break;

      case kChunkSna0:
        if (vqa_handle_p->config.option_flags & kVqaOptionAltAudio) {
          if (CopyStagedAudio(vqa_handle_p) == kVqaSleeping) {
            movie->flags |= kMovieLoaderAsleep;
            return kVqaSleeping;
          }
          movie->flags &= ~kMovieLoaderAsleep;

          if (LoadSound(vqa_handle_p, chunk_bytes) != 0) {
            return kVqaErrorRead;
          }
        } else {
          if (!vqa_handle_p->io->Seek(PadSize(chunk_bytes),
                                      SeekOrigin::kCurrent)) {
            return kVqaErrorSeek;
          }
        }
        break;

      case kChunkSnd1:
        if (!(vqa_handle_p->config.option_flags & kVqaOptionAltAudio)) {
          if (CopyStagedAudio(vqa_handle_p) == kVqaSleeping) {
            movie->flags |= kMovieLoaderAsleep;
            return kVqaSleeping;
          }
          movie->flags &= ~kMovieLoaderAsleep;

          if (LoadZapSound(vqa_handle_p, chunk_bytes) != 0) {
            return kVqaErrorRead;
          }
        } else {
          if (!vqa_handle_p->io->Seek(PadSize(chunk_bytes),
                                      SeekOrigin::kCurrent)) {
            return kVqaErrorSeek;
          }
        }
        break;

      case kChunkSna1:
        if (vqa_handle_p->config.option_flags & kVqaOptionAltAudio) {
          if (CopyStagedAudio(vqa_handle_p) == kVqaSleeping) {
            movie->flags |= kMovieLoaderAsleep;
            return kVqaSleeping;
          }
          movie->flags &= ~kMovieLoaderAsleep;

          if (LoadZapSound(vqa_handle_p, chunk_bytes) != 0) {
            return kVqaErrorRead;
          }
        } else {
          if (!vqa_handle_p->io->Seek(PadSize(chunk_bytes),
                                      SeekOrigin::kCurrent)) {
            return kVqaErrorSeek;
          }
        }
        break;

      case kChunkSnd2:
        if (!(vqa_handle_p->config.option_flags & kVqaOptionAltAudio)) {
          if (CopyStagedAudio(vqa_handle_p) == kVqaSleeping) {
            movie->flags |= kMovieLoaderAsleep;
            return kVqaSleeping;
          }
          movie->flags &= ~kMovieLoaderAsleep;

          if (LoadAdpcmSound(vqa_handle_p, chunk_bytes) != 0) {
            return kVqaErrorRead;
          }
        } else {
          if (!vqa_handle_p->io->Seek(PadSize(chunk_bytes),
                                      SeekOrigin::kCurrent)) {
            return kVqaErrorSeek;
          }
        }
        break;

      case kChunkSna2:
        if (vqa_handle_p->config.option_flags & kVqaOptionAltAudio) {
          if (CopyStagedAudio(vqa_handle_p) == kVqaSleeping) {
            movie->flags |= kMovieLoaderAsleep;
            return kVqaSleeping;
          }
          movie->flags &= ~kMovieLoaderAsleep;

          if (LoadAdpcmSound(vqa_handle_p, chunk_bytes) != 0) {
            return kVqaErrorRead;
          }
        } else {
          if (!vqa_handle_p->io->Seek(PadSize(chunk_bytes),
                                      SeekOrigin::kCurrent)) {
            return kVqaErrorSeek;
          }
        }
        break;

      // Skip any unknown chunks.
      default:
        if (!vqa_handle_p->io->Seek(PadSize(chunk_bytes),
                                    SeekOrigin::kCurrent)) {
          return kVqaErrorSeek;
        }
        break;
    }
  }

  // Number the frame and hand it to the drawer.
  frame->frame_number = loader->next_frame_number;
  loader->next_frame_number++;

  frame->flags |= kFrameLoaded;
  loader->current_frame = frame->next;

  return 0;
}

// Allocates the play buffers of a movie with this header: the codebook and
// frame rings, the image buffer when the player draws into its own, the audio
// ring and staging buffer when the sound is on. Resolves
// the -1 default of config->audio_buffer_bytes, and rounds it down to whole
// blocks. Returns nullptr when config asks for no codebook or frame buffers.
static std::unique_ptr<VqaMovie> AllocateMovie(const VqaHeader* header,
                                               VqaConfig* config) {
  if (config->codebook_buffer_count <= 0 || config->frame_buffer_count <= 0) {
    return nullptr;
  }

  auto owned_movie = std::make_unique<VqaMovie>();
  VqaMovie* movie = owned_movie.get();

  // Compressed data is loaded at the end of its buffer and decompressed in
  // place towards the start, so each buffer is the decompressed size plus
  // slack (250 bytes for a codebook, 1 KiB for a palette or the vector
  // pointers): room for the output never to catch up with input still to be
  // read, even for data LCW could not shrink. The sizes are rounded down to a
  // multiple of 4, which kept the DOS buffers DWORD aligned.
  movie->codebook_capacity =
      ((header->codebook_entries * header->block_width * header->block_height) +
       250) /
      4 * 4;

  // 256 colors of 3 bytes.
  movie->palette_capacity = (768 + 1024) / 4 * 4;

  // Two bytes per block.
  movie->pointers_capacity =
      (((header->image_width / header->block_width) *
        (header->image_height / header->block_height) * int{sizeof(int16_t)}) +
       1024) /
      4 * 4;

  // The codebook ring.
  movie->codebooks.reserve(base::ToSize(config->codebook_buffer_count));

  for (int32_t i = 0; i < config->codebook_buffer_count; i++) {
    auto codebook = std::make_unique<VqaCodebook>();

    codebook->buffer.resize(base::ToSize(movie->codebook_capacity));
    movie->codebooks.push_back(std::move(codebook));
  }

  // Link the nodes into a ring.
  for (size_t i = 0; i < movie->codebooks.size(); i++) {
    const size_t next_index = (i + 1) % movie->codebooks.size();
    movie->codebooks.at(i)->next = movie->codebooks.at(next_index).get();
  }

  // The loader starts assembling into the first node.
  VqaCodebook* const first_codebook = movie->codebooks.front().get();
  movie->loader.partial_codebook = first_codebook;
  movie->loader.full_codebook = first_codebook;

  // The frame ring.
  movie->frames.reserve(base::ToSize(config->frame_buffer_count));

  for (int32_t i = 0; i < config->frame_buffer_count; i++) {
    auto frame = std::make_unique<VqaFrame>();

    frame->pointers.resize(base::ToSize(movie->pointers_capacity));
    frame->palette.resize(base::ToSize(movie->palette_capacity));

    frame->codebook = first_codebook;
    movie->frames.push_back(std::move(frame));
  }

  // Link the nodes into a ring.
  for (size_t i = 0; i < movie->frames.size(); i++) {
    const size_t next_index = (i + 1) % movie->frames.size();
    movie->frames.at(i)->next = movie->frames.at(next_index).get();
  }

  // The loader, the drawer and the flipper all start at the first frame.
  VqaFrame* const first_frame = movie->frames.front().get();
  movie->loader.current_frame = first_frame;
  movie->drawer.current_frame = first_frame;
  movie->flipper.drawn_frame = first_frame;

  // The image buffer: the caller's; else, when the player draws, its own the
  // size of the movie; else none, and the drawer draws nothing.
  if (config->image_buffer.empty()) {
    if ((config->draw_flags & kVqaDrawToBuffer) != 0) {
      movie->image_storage.resize(
          static_cast<std::size_t>(header->image_width) * header->image_height);
      movie->drawer.image_buffer = movie->image_storage;

      movie->drawer.image_width = header->image_width;
      movie->drawer.image_height = header->image_height;
    } else {
      movie->drawer.image_width = config->image_width;
      movie->drawer.image_height = config->image_height;
    }
  } else {
    movie->drawer.image_buffer = config->image_buffer;
    movie->drawer.image_width = config->image_width;
    movie->drawer.image_height = config->image_height;
  }

  // The sound's format, its ring and its staging buffer.
  if ((header->flags & kVqaHasAudio) != 0 &&
      (config->option_flags & kVqaOptionAudio) != 0) {
    VqaAudio* audio = &movie->audio;

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
      const auto blocks =
          (audio->bytes_per_second + (audio->bytes_per_second / 2)) /
          config->audio_block_bytes;
      config->audio_buffer_bytes = config->audio_block_bytes * blocks;
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

  return owned_movie;
}

// Loads frames until the frame ring is full (frame_buffer_count of them) or
// the movie ends. Returns 0, or the loader's error; the end of a movie shorter
// than the ring is not one.
int32_t PreloadFrames(VqaPlayerState* state) {
  VqaMovie* movie = state->movie.get();
  VqaConfig* config = &state->config;

  for (int32_t i = 0; i < config->frame_buffer_count; i++) {
    const int32_t result = LoadNextFrame(state);
    if (result == kVqaEndOfMovie &&
        std::cmp_greater_equal(movie->loader.next_frame_number,
                               state->header.frame_count)) {
      // A movie with fewer frames than buffers ends while priming. Only an
      // end of file before the last frame (a truncated movie) is an error.
      break;
    }
    if (result != 0 && result != kVqaNoBuffer && result != kVqaSleeping) {
      return result;
    }
  }

  return 0;
}

// Loads the chunks inside a VQFR or VQFK frame container of frame_iffsize
// bytes: codebooks, palette and vector pointers. Returns 0, kVqaEndOfMovie
// when the file ends inside it, or kVqaErrorRead for a bad or unknown chunk.
static int32_t LoadFrameContainer(VqaPlayerState* state, int32_t frame_bytes) {
  int64_t bytes_loaded = 0;  // 64-bit: sums sizes up to 2^31 each.

  VqaMovie* movie = state->movie.get();
  VqaFrame* frame = movie->loader.current_frame;
  const int32_t padded_frame_bytes = PadSize(frame_bytes);
  VqaDrawer* drawer = &state->movie->drawer;
  ChunkHeader* chunk = &movie->loader.chunk_header;

  while (bytes_loaded < padded_frame_bytes) {
    if (!state->io->ReadObject(*chunk)) {
      return kVqaEndOfMovie;
    }

    const int32_t chunk_bytes = ChunkSize(*chunk);
    if (!IsValidChunkSize(chunk_bytes)) {
      return kVqaErrorRead;
    }

    // The chunk header, and the payload with its pad byte.
    bytes_loaded += 8;
    bytes_loaded += PadSize(chunk_bytes);

    switch (chunk->id) {
      // Full uncompressed codebook.
      case kChunkCbf0:
        if (LoadFullCodebook(state, chunk_bytes)) {
          return kVqaErrorRead;
        }
        break;

      // Full compressed codebook.
      case kChunkCbfz:
        if (LoadCompressedFullCodebook(state, chunk_bytes)) {
          return kVqaErrorRead;
        }
        break;

      // Partial uncompressed codebook.
      case kChunkCbp0:
        if (LoadPartialCodebook(state, chunk_bytes)) {
          return kVqaErrorRead;
        }
        break;

      // Partial compressed codebook.
      case kChunkCbpz:
        if (LoadCompressedPartialCodebook(state, chunk_bytes)) {
          return kVqaErrorRead;
        }
        break;

      // Uncompressed palette.
      case kChunkCpl0:
        if (LoadPalette(state, chunk_bytes)) {
          return kVqaErrorRead;
        }

        // Keep the movie's first palette in the drawer, a copy Westwood's
        // Monopoly read from there. The player never reads it: saved_palette
        // is written again before DrawNextFrame() uses it.
        if (drawer->saved_palette_bytes == 0) {
          base::CopyBytes(base::ObjectBytes(drawer->saved_palette),
                          std::as_bytes(std::span(frame->palette)),
                          frame->palette_bytes);
          drawer->saved_palette_bytes = frame->palette_bytes;
        }

        // Flag this frame as having a palette.
        frame->flags |= kFrameHasPalette;
        break;

      // Compressed palette.
      case kChunkCplz:
        if (LoadCompressedPalette(state, chunk_bytes)) {
          return kVqaErrorRead;
        }

        // Keep the movie's first palette in the drawer, a copy Westwood's
        // Monopoly read from there. The player never reads it: saved_palette
        // is written again before DrawNextFrame() uses it.
        if (drawer->saved_palette_bytes == 0) {
          drawer->saved_palette_bytes =
              LCW_Uncompress(std::span(frame->palette)
                                 .subspan(base::ToSize(frame->palette_offset)),
                             drawer->saved_palette);
        }

        // Flag this frame as having a palette.
        frame->flags |= kFrameHasPalette;
        break;

      // Uncompressed vector pointers.
      case kChunkVpt0:
        if (LoadVectorPointers(state, chunk_bytes)) {
          return kVqaErrorRead;
        }
        break;

      // Compressed vector pointers.
      case kChunkVptz:
      case kChunkVptd:
        if (LoadCompressedVectorPointers(state, chunk_bytes)) {
          return kVqaErrorRead;
        }
        break;

      // Compressed vector pointers of a key frame.
      case kChunkVptk:
        if (LoadCompressedVectorPointers(state, chunk_bytes)) {
          return kVqaErrorRead;
        }

        // Flag this frame as being key.
        frame->flags |= kFrameKey;
        break;

      // Sound is never inside a frame container, so an unknown chunk here
      // is an error rather than something to skip.
      default:
        return kVqaErrorRead;
    }
  }

  return 0;
}

// The chunk loaders below each read one chunk's payload, iffsize bytes and
// the pad byte, into the loader's current frame or codebook node. Each returns
// 0, or kVqaErrorRead when the chunk does not fit its buffer or the read
// fails.

// Loads a full uncompressed codebook into the node being assembled, which
// becomes the full codebook; the next group's pieces go to the node after it.
static int32_t LoadFullCodebook(const VqaPlayerState* state,
                                int32_t chunk_bytes) {
  VqaLoader* loader = &state->movie->loader;
  VqaCodebook* codebook = loader->partial_codebook;

  if (!FitsInBuffer(0, PadSize(chunk_bytes), state->movie->codebook_capacity)) {
    return kVqaErrorRead;
  }

  if (!state->io->Read(std::span(codebook->buffer), PadSize(chunk_bytes))) {
    return kVqaErrorRead;
  }

  // A full codebook replaces any pieces collected so far.
  loader->partial_count = 0;

  codebook->flags &= ~kCodebookCompressed;
  codebook->compressed_offset = 0;

  // The next group's pieces go to the next node.
  loader->full_codebook = codebook;
  loader->partial_codebook = codebook->next;

  return 0;
}

// As LoadFullCodebook(), for a compressed codebook: loaded at the end of the
// buffer for DecompressFrame() to decompress in place.
static int32_t LoadCompressedFullCodebook(const VqaPlayerState* state,
                                          int32_t chunk_bytes) {
  VqaLoader* loader = &state->movie->loader;
  VqaCodebook* codebook = loader->partial_codebook;
  const int32_t padded_bytes = PadSize(chunk_bytes);

  const int32_t compressed_offset =
      state->movie->codebook_capacity - padded_bytes;

  // A chunk larger than the buffer would start before it.
  if (compressed_offset < 0) {
    return kVqaErrorRead;
  }

  const auto buffer =
      std::span(codebook->buffer).subspan(base::ToSize(compressed_offset));

  if (!state->io->Read(buffer, padded_bytes)) {
    return kVqaErrorRead;
  }

  // A full codebook replaces any pieces collected so far.
  loader->partial_count = 0;

  codebook->flags |= kCodebookCompressed;
  codebook->compressed_offset = compressed_offset;

  // The next group's pieces go to the next node.
  loader->full_codebook = codebook;
  loader->partial_codebook = codebook->next;

  return 0;
}

// Appends one uncompressed piece of the next group's codebook. The group's
// last piece completes it, and it becomes the full codebook.
static int32_t LoadPartialCodebook(const VqaPlayerState* state,
                                   int32_t chunk_bytes) {
  VqaMovie* movie = state->movie.get();
  VqaLoader* loader = &movie->loader;
  VqaCodebook* codebook = loader->partial_codebook;

  if (!FitsInBuffer(loader->partial_bytes, PadSize(chunk_bytes),
                    movie->codebook_capacity)) {
    return kVqaErrorRead;
  }

  const auto buffer =
      std::span(codebook->buffer).subspan(base::ToSize(loader->partial_bytes));

  if (!state->io->Read(buffer, PadSize(chunk_bytes))) {
    return kVqaErrorRead;
  }

  // Each piece's pad byte is overwritten by the next piece.
  loader->partial_bytes += chunk_bytes;
  loader->partial_count++;

  // The group's last piece completes the codebook.
  if (std::cmp_equal(loader->partial_count, state->header.frames_per_group)) {
    loader->partial_count = 0;
    loader->partial_bytes = 0;

    codebook->flags &= ~kCodebookCompressed;
    codebook->compressed_offset = 0;

    loader->full_codebook = codebook;
    loader->partial_codebook = codebook->next;
  }

  return 0;
}

// As LoadPartialCodebook(), for compressed pieces. They collect towards the end
// of the buffer and are decompressed together, as one, once the codebook is
// complete.
static int32_t LoadCompressedPartialCodebook(const VqaPlayerState* state,
                                             int32_t chunk_bytes) {
  VqaMovie* movie = state->movie.get();
  VqaLoader* loader = &movie->loader;
  VqaCodebook* codebook = loader->partial_codebook;
  const int32_t padded_bytes = PadSize(chunk_bytes);

  // The group's first piece places the whole compressed codebook, whose size
  // is not known yet: estimated as this piece's size times the pieces in a
  // group, plus 100 bytes, back from the end of the buffer.
  if (loader->partial_bytes == 0) {
    // 64-bit because a large chunk times the group size overflows int32_t.
    // A negative estimate would place the codebook before the buffer.
    const int64_t estimated_offset =
        int64_t{movie->codebook_capacity} -
        ((int64_t{padded_bytes} * state->header.frames_per_group) + 100);
    if (estimated_offset < 0) {
      return kVqaErrorRead;
    }
    codebook->compressed_offset = static_cast<int32_t>(estimated_offset);
  }

  // The estimate assumes every part of the group is the size of the first
  // one, so a larger later part can still run off the end.
  if (!FitsInBuffer(
          int64_t{codebook->compressed_offset} + loader->partial_bytes,
          padded_bytes, movie->codebook_capacity)) {
    return kVqaErrorRead;
  }

  const auto buffer = std::span(codebook->buffer)
                          .subspan(base::ToSize(codebook->compressed_offset +
                                                loader->partial_bytes));

  if (!state->io->Read(buffer, padded_bytes)) {
    return kVqaErrorRead;
  }

  // Each piece's pad byte is overwritten by the next piece.
  loader->partial_bytes += chunk_bytes;
  loader->partial_count++;

  // The group's last piece completes the codebook.
  if (std::cmp_equal(loader->partial_count, state->header.frames_per_group)) {
    loader->partial_count = 0;
    loader->partial_bytes = 0;

    codebook->flags |= kCodebookCompressed;

    loader->full_codebook = codebook;
    loader->partial_codebook = codebook->next;
  }

  return 0;
}

// Loads an uncompressed palette into the frame's palette buffer.
static int32_t LoadPalette(const VqaPlayerState* state, int32_t chunk_bytes) {
  VqaFrame* frame = state->movie->loader.current_frame;

  // The loader copies a frame's palette into the drawer's 256-color palette,
  // so a larger one is malformed and would overrun that copy.
  if (!FitsInBuffer(0, PadSize(chunk_bytes),
                    int64_t{sizeof(VqaDrawer::saved_palette)})) {
    return kVqaErrorRead;
  }

  if (!state->io->Read(std::span(frame->palette), PadSize(chunk_bytes))) {
    return kVqaErrorRead;
  }

  frame->flags &= ~kFramePaletteCompressed;
  frame->palette_offset = 0;
  frame->palette_bytes = chunk_bytes;

  return 0;
}

// Loads a compressed palette at the end of the frame's palette buffer.
// palette_bytes is the compressed size until the drawer decompresses it.
static int32_t LoadCompressedPalette(const VqaPlayerState* state,
                                     int32_t chunk_bytes) {
  VqaFrame* frame = state->movie->loader.current_frame;
  const int32_t padded_bytes = PadSize(chunk_bytes);

  const int32_t compressed_offset =
      state->movie->palette_capacity - padded_bytes;

  // A chunk larger than the buffer would start before it.
  if (compressed_offset < 0) {
    return kVqaErrorRead;
  }

  const auto buffer =
      std::span(frame->palette).subspan(base::ToSize(compressed_offset));

  if (!state->io->Read(buffer, padded_bytes)) {
    return kVqaErrorRead;
  }

  frame->flags |= kFramePaletteCompressed;
  frame->palette_offset = compressed_offset;
  frame->palette_bytes = chunk_bytes;

  return 0;
}

// Loads uncompressed vector pointers into the frame's pointer buffer.
static int32_t LoadVectorPointers(const VqaPlayerState* state,
                                  int32_t chunk_bytes) {
  VqaFrame* frame = state->movie->loader.current_frame;

  if (!FitsInBuffer(0, PadSize(chunk_bytes), state->movie->pointers_capacity)) {
    return kVqaErrorRead;
  }

  if (!state->io->Read(std::span(frame->pointers), PadSize(chunk_bytes))) {
    return kVqaErrorRead;
  }

  frame->flags &= ~kFramePointersCompressed;
  frame->pointers_offset = 0;

  return 0;
}

// Loads compressed vector pointers at the end of the frame's pointer buffer.
static int32_t LoadCompressedVectorPointers(const VqaPlayerState* state,
                                            int32_t chunk_bytes) {
  VqaFrame* frame = state->movie->loader.current_frame;
  const int32_t padded_bytes = PadSize(chunk_bytes);
  const int32_t compressed_offset =
      state->movie->pointers_capacity - padded_bytes;

  // A chunk larger than the buffer would start before it.
  if (compressed_offset < 0) {
    return kVqaErrorRead;
  }

  const auto buffer =
      std::span(frame->pointers).subspan(base::ToSize(compressed_offset));

  if (!state->io->Read(buffer, padded_bytes)) {
    return kVqaErrorRead;
  }

  frame->flags |= kFramePointersCompressed;
  frame->pointers_offset = compressed_offset;

  return 0;
}

// The sound chunk loaders stage a chunk's sound in audio.staging, for
// CopyStagedAudio() to move into the ring. The movie's first sound chunk may
// be larger than staging - it preloads the sound - and goes straight into the
// ring instead; a larger chunk anywhere else is an error. With the sound off
// they skip the chunk. Each returns 0, or kVqaErrorRead or kVqaErrorSeek.

// Loads an uncompressed sound chunk.
static int32_t LoadSound(VqaPlayerState* state, int32_t chunk_bytes) {
  VqaMovie* movie = state->movie.get();
  VqaAudio* audio = &movie->audio;
  VqaConfig* config = &state->config;
  const int32_t padded_bytes = PadSize(chunk_bytes);

  // No sound to load into.
  if ((config->option_flags & kVqaOptionAudio) == 0 || audio->ring.empty()) {
    if (!state->io->Seek(padded_bytes, SeekOrigin::kCurrent)) {
      return kVqaErrorSeek;
    }
    return 0;
  }

  // The first chunk, too big for staging, preloads the ring.
  if (padded_bytes > audio->staging_capacity && audio->write_offset == 0) {
    if (padded_bytes > config->audio_buffer_bytes) {
      return kVqaErrorRead;
    }

    if (!state->io->Read(audio->ring, padded_bytes)) {
      return kVqaErrorRead;
    }

    // A chunk that fills the ring exactly wraps the write back to its start.
    audio->write_offset =
        (audio->write_offset + chunk_bytes) % config->audio_buffer_bytes;

    // Mark the whole blocks it filled; the next chunk completes a partial
    // last one.
    for (int32_t i = 0; i < chunk_bytes / config->audio_block_bytes; i++) {
      audio->block_loaded.at(base::ToSize(i)) = 1;
    }

    return 0;
  }
  // Only the first chunk may exceed staging.
  if (padded_bytes > audio->staging_capacity) {
    return kVqaErrorRead;
  }

  if (!state->io->Read(std::span(audio->staging), padded_bytes)) {
    return kVqaErrorRead;
  }

  audio->staged_bytes = chunk_bytes;

  return 0;
}

// Loads a sound chunk in Westwood's ZAP ADPCM, which starts with a ZapHeader.
static int32_t LoadZapSound(VqaPlayerState* state, int32_t chunk_bytes) {
  std::span<unsigned char> compressed;
  ZapHeader zap_header{};

  VqaMovie* movie = state->movie.get();
  VqaAudio* audio = &movie->audio;
  VqaConfig* config = &state->config;
  int32_t padded_bytes = PadSize(chunk_bytes);

  // No sound to load into.
  if ((config->option_flags & kVqaOptionAudio) == 0 || audio->ring.empty()) {
    if (!state->io->Seek(padded_bytes, SeekOrigin::kCurrent)) {
      return kVqaErrorSeek;
    }
    return 0;
  }

  // The ZAP header is part of the chunk; a shorter chunk would leave a
  // negative payload size.
  if (chunk_bytes < int32_t{sizeof(ZapHeader)}) {
    return kVqaErrorRead;
  }

  if (!state->io->ReadObject(zap_header)) {
    return kVqaErrorRead;
  }

  // The sound after the header.
  padded_bytes -= int32_t{sizeof(ZapHeader)};

  // The first chunk, too big for staging, preloads the ring.
  if (std::cmp_greater(zap_header.UnCompSize, audio->staging_capacity) &&
      audio->write_offset == 0) {
    if (padded_bytes > config->audio_buffer_bytes ||
        std::cmp_greater(zap_header.UnCompSize, config->audio_buffer_bytes)) {
      return kVqaErrorRead;
    }

    // Equal sizes: stored uncompressed.
    if (zap_header.UnCompSize == zap_header.CompSize) {
      if (!state->io->Read(audio->ring, padded_bytes)) {
        return kVqaErrorRead;
      }
    } else {
      // Loaded at the end of the ring and decompressed towards its start.
      compressed = audio->ring.subspan(
          base::ToSize(config->audio_buffer_bytes - padded_bytes));

      if (!state->io->Read(compressed, padded_bytes)) {
        return kVqaErrorRead;
      }

      // TODO: AudioUnzap() is a stub that writes nothing, so the ring keeps
      // the compressed bytes and whatever was there before, and plays them.
      // The shipped Red Alert movies have no SND1 sound.
      AudioUnzap(compressed, audio->ring.first(zap_header.UnCompSize));
    }

    audio->write_offset = (audio->write_offset + zap_header.UnCompSize) %
                          config->audio_buffer_bytes;

    for (int32_t i = 0; i < zap_header.UnCompSize / config->audio_block_bytes;
         i++) {
      audio->block_loaded.at(base::ToSize(i)) = 1;
    }

    return 0;
  }

  // Only the first chunk may exceed staging.
  if (padded_bytes > audio->staging_capacity ||
      std::cmp_greater(zap_header.UnCompSize, audio->staging_capacity)) {
    return kVqaErrorRead;
  }

  if (zap_header.UnCompSize == zap_header.CompSize) {
    if (!state->io->Read(std::span(audio->staging), padded_bytes)) {
      return kVqaErrorRead;
    }
  } else {
    // Loaded at the end of staging and decompressed towards its start.
    compressed =
        std::span(audio->staging)
            .subspan(base::ToSize(audio->staging_capacity - padded_bytes));

    if (!state->io->Read(compressed, padded_bytes)) {
      return kVqaErrorRead;
    }

    // TODO: As above, AudioUnzap() writes nothing.
    AudioUnzap(compressed,
               std::span(audio->staging).first(zap_header.UnCompSize));
  }

  audio->staged_bytes = zap_header.UnCompSize;

  return 0;
}

// Loads a sound chunk in IMA ADPCM, 4 bits a sample.
static int32_t LoadAdpcmSound(VqaPlayerState* state, int32_t chunk_bytes) {
  std::span<unsigned char> compressed;

  VqaMovie* movie = state->movie.get();
  VqaAudio* audio = &movie->audio;
  VqaConfig* config = &state->config;
  const int32_t padded_bytes = PadSize(chunk_bytes);

  // No sound to load into.
  if ((config->option_flags & kVqaOptionAudio) == 0 || audio->ring.empty()) {
    if (!state->io->Seek(padded_bytes, SeekOrigin::kCurrent)) {
      return kVqaErrorSeek;
    }
    return 0;
  }

  // Two samples a byte. 64-bit so an oversized chunk cannot overflow before
  // the bounds checks.
  const int64_t wide_decoded_bytes =
      int64_t{chunk_bytes} * (audio->bits_per_sample / 4);
  if (wide_decoded_bytes >
      std::max(config->audio_buffer_bytes, audio->staging_capacity)) {
    return kVqaErrorRead;
  }
  const auto decoded_bytes = static_cast<int32_t>(wide_decoded_bytes);

  // The first chunk, too big for staging, preloads the ring.
  if (decoded_bytes > audio->staging_capacity && audio->write_offset == 0) {
    if (padded_bytes > config->audio_buffer_bytes ||
        decoded_bytes > config->audio_buffer_bytes) {
      return kVqaErrorRead;
    }

    // Loaded at the end of the ring and decompressed towards its start.
    compressed = audio->ring.subspan(
        base::ToSize(config->audio_buffer_bytes - padded_bytes));

    if (!state->io->Read(compressed, padded_bytes)) {
      return kVqaErrorRead;
    }

    // TODO: A failed decode (the decoder takes only 16-bit mono) is ignored,
    // so the ring plays whatever it held. The shipped Red Alert movies are
    // all 16-bit mono.
    audio->adpcm.source = compressed;
    audio->adpcm.dest = audio->ring;
    DecodeAdpcmSound(&audio->adpcm, decoded_bytes);

    audio->write_offset =
        (audio->write_offset + decoded_bytes) % config->audio_buffer_bytes;

    for (int32_t i = 0; i < decoded_bytes / config->audio_block_bytes; i++) {
      audio->block_loaded.at(base::ToSize(i)) = 1;
    }

    return 0;
  }

  // Only the first chunk may exceed staging.
  if (padded_bytes > audio->staging_capacity ||
      decoded_bytes > audio->staging_capacity) {
    return kVqaErrorRead;
  }

  // Loaded at the end of staging and decompressed towards its start.
  compressed =
      std::span(audio->staging)
          .subspan(base::ToSize(audio->staging_capacity - padded_bytes));

  if (!state->io->Read(compressed, padded_bytes)) {
    return kVqaErrorRead;
  }

  // TODO: A failed decode is ignored here too.
  audio->adpcm.source = compressed;
  audio->adpcm.dest = audio->staging;
  DecodeAdpcmSound(&audio->adpcm, decoded_bytes);

  audio->staged_bytes = decoded_bytes;

  return 0;
}
