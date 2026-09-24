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

// What LoadFramePart() made of a chunk.
enum class FramePart {
  kNone,            // Not a part of a frame; nothing was read.
  kFailed,          // A part that did not fit its buffer or failed to read.
  kLoaded,          // A codebook or a palette.
  kVectorPointers,  // The vector pointers.
};
static FramePart LoadFramePart(VqaPlayerState* state, uint32_t id,
                               int32_t chunk_bytes);
static int32_t LoadSoundChunk(VqaPlayerState* state, uint32_t id,
                              int32_t chunk_bytes);

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

// Reads an opened movie's chunks up to its frame table, allocates its play
// buffers, starts its sound and preloads the frame ring. Returns 0, or a
// kVqaError* code with the movie left for the caller to close.
static int32_t PrepareMovie(VqaPlayerState* state, VqaConfig* config) {
  ChunkHeader chunk{};

  VqaHeader* header = &state->header;

  // The file must be an IFF FORM of type WVQA.
  if (!state->io->ReadObject(chunk)) {
    return kVqaErrorRead;
  }

  if (chunk.id != ID_FORM || chunk.size == 0) {
    return kVqaErrorNotVqa;
  }

  // The form type follows the FORM header.
  if (!state->io->ReadObject(chunk.id)) {
    return kVqaErrorRead;
  }

  if (chunk.id != kFormWvqa) {
    return kVqaErrorNotVqa;
  }

  // Play from a copy of the caller's configuration, or the defaults; the
  // header resolves its -1 fields below.
  if (config != nullptr) {
    state->config = *config;
  } else {
    SetVqaConfigDefaults(&state->config);
  }

  // Only the copy from here on.
  config = &state->config;

  // Read the chunks in front of the frames, up to FINF, the last of them.
  // VQHD must come before it; anything else is skipped.
  bool found_frame_table = false;

  while (!found_frame_table) {
    if (!state->io->ReadObject(chunk)) {
      return kVqaErrorRead;
    }

    const int32_t chunk_bytes = ChunkSize(chunk);

    // A negative size would make the skip below seek backwards.
    if (!IsValidChunkSize(chunk_bytes)) {
      return kVqaErrorNotVqa;
    }

    switch (chunk.id) {
      // The movie header, which sizes the play buffers.
      case kChunkVqhd:
        // Only one header: the play buffers are sized from it.
        if (std::cmp_not_equal(chunk_bytes, sizeof(VqaHeader)) ||
            state->movie != nullptr) {
          return kVqaErrorNotVqa;
        }

        // Read the header data, and skip the pad byte of an odd chunk.
        if (!state->io->ReadObject(*header) ||
            !state->io->Seek(PadSize(chunk_bytes) - chunk_bytes,
                             SeekOrigin::kCurrent)) {
          return kVqaErrorRead;
        }

        // These fields are divisors when sizing buffers and timing playback.
        if (header->block_width == 0 || header->block_height == 0 ||
            header->frames_per_group == 0 || header->fps == 0) {
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
          return kVqaErrorAudio;
        }

        state->movie = AllocateMovie(header, config);
        if (state->movie == nullptr) {
          return kVqaErrorNoMemory;
        }

        break;

      // The frame table, which the player does not use, is the last chunk
      // before the frames. A movie without a header before it cannot play.
      case kChunkFinf:
        if (state->movie == nullptr) {
          return kVqaErrorNotVqa;
        }

        if (!state->io->Seek(PadSize(chunk_bytes), SeekOrigin::kCurrent)) {
          return kVqaErrorSeek;
        }

        found_frame_table = true;
        break;

      // Chunks the player has no use for, such as PINF.
      default:
        if (!state->io->Seek(PadSize(chunk_bytes), SeekOrigin::kCurrent)) {
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
  if (state->movie->audio.block_loaded.empty()) {
    config->option_flags &= ~kVqaOptionAudio;
  }

  // Start the sound output: originally HMI's DOS sound drivers, now the SDL
  // mixer.
  if ((config->option_flags & kVqaOptionAudio) != 0 && OpenMovieAudio(state)) {
    return kVqaErrorAudio;
  }

  // Preload the frame ring, so playback starts with frames in hand.
  if (PreloadFrames(state) != 0) {
    return kVqaErrorRead;
  }

  return 0;
}

int32_t OpenVqa(VqaPlayerState* state, std::string_view filename,
                VqaConfig* config) {
  vqa_movie_loaded.store(false, std::memory_order_relaxed);

  if (!state->io->Open(filename)) {
    return kVqaErrorOpen;
  }

  const int32_t result = PrepareMovie(state, config);
  if (result != 0) {
    CloseVqa(state);
  }
  return result;
}

void CloseVqa(VqaPlayerState* state) {
  // Audio is open only once OpenMovieAudio() has run. A failed OpenVqa() can
  // get here earlier, with no data and no audio callback to tear down.
  if (state->movie != nullptr &&
      (state->movie->audio.flags & kAudioOpen) != 0) {
    CloseMovieAudio(state);
  }

  state->io->Close();

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

  VqaMovie* movie = state->movie.get();
  VqaLoader* loader = &movie->loader;
  VqaFrame* frame = loader->current_frame;
  ChunkHeader* chunk = &loader->chunk_header;

  int32_t chunk_bytes = ChunkSize(*chunk);

  // Every frame the header counts is loaded.
  if (std::cmp_greater_equal(loader->next_frame_number,
                             state->header.frame_count)) {
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
    frame->codebook = loader->full_codebook;
  }

  while (!frame_loaded) {
    // A resumed loader is inside a chunk already.
    if (!(movie->flags & kMovieLoaderAsleep)) {
      if (!state->io->ReadObject(*chunk)) {
        return kVqaEndOfMovie;
      }

      chunk_bytes = ChunkSize(*chunk);
      if (!IsValidChunkSize(chunk_bytes)) {
        return kVqaErrorRead;
      }
    }

    // Codebooks, palettes and vector pointers. In the older format without
    // frame containers, the vector pointers come last in a frame.
    const FramePart part = LoadFramePart(state, chunk->id, chunk_bytes);
    if (part == FramePart::kFailed) {
      return kVqaErrorRead;
    }
    if (part != FramePart::kNone) {
      frame_loaded = part == FramePart::kVectorPointers;
      continue;
    }

    switch (chunk->id) {
      // A frame container, of a key frame for VQFK.
      case kChunkVqfr:
      case kChunkVqfk:
        if (LoadFrameContainer(state, chunk_bytes)) {
          return kVqaErrorRead;
        }

        if (chunk->id == kChunkVqfk) {
          frame->flags |= kFrameKey;
        }
        frame_loaded = true;
        break;

      // Sound. SND* chunks are the primary track and SNA* the alternate one;
      // the track not played is skipped, and both with the sound off. Before
      // staging a chunk, the last one's sound moves from staging into the
      // ring; with no room there the loader sleeps and resumes here.
      case kChunkSnd0:
      case kChunkSnd1:
      case kChunkSnd2:
      case kChunkSna0:
      case kChunkSna1:
      case kChunkSna2: {
        const bool alternate_chunk = chunk->id == kChunkSna0 ||
                                     chunk->id == kChunkSna1 ||
                                     chunk->id == kChunkSna2;
        const uint32_t options = state->config.option_flags;
        const bool alternate_track = (options & kVqaOptionAltAudio) != 0;
        if ((options & kVqaOptionAudio) == 0 ||
            alternate_chunk != alternate_track) {
          if (!state->io->Seek(PadSize(chunk_bytes), SeekOrigin::kCurrent)) {
            return kVqaErrorSeek;
          }
          break;
        }

        if (CopyStagedAudio(state) == kVqaSleeping) {
          movie->flags |= kMovieLoaderAsleep;
          return kVqaSleeping;
        }
        movie->flags &= ~kMovieLoaderAsleep;

        if (LoadSoundChunk(state, chunk->id, chunk_bytes) != 0) {
          return kVqaErrorRead;
        }
        break;
      }

      // Skip any unknown chunks.
      default:
        if (!state->io->Seek(PadSize(chunk_bytes), SeekOrigin::kCurrent)) {
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

// Links nodes into a ring through their next pointers, the last to the first.
template <class Node>
static void LinkRing(std::vector<std::unique_ptr<Node>>& nodes) {
  for (size_t i = 0; i < nodes.size(); i++) {
    nodes.at(i)->next = nodes.at((i + 1) % nodes.size()).get();
  }
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

  LinkRing(movie->codebooks);

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

  LinkRing(movie->frames);

  // The loader, the drawer and the flipper all start at the first frame.
  VqaFrame* const first_frame = movie->frames.front().get();
  movie->loader.current_frame = first_frame;
  movie->drawer.current_frame = first_frame;
  movie->flipper.drawn_frame = first_frame;

  // The image buffer: the caller's; else, when the player draws, its own the
  // size of the movie; else none, and the drawer draws nothing.
  movie->drawer.image_buffer = config->image_buffer;
  movie->drawer.image_width = config->image_width;
  movie->drawer.image_height = config->image_height;
  if (config->image_buffer.empty() &&
      (config->draw_flags & kVqaDrawToBuffer) != 0) {
    movie->image_storage.resize(static_cast<std::size_t>(header->image_width) *
                                header->image_height);
    movie->drawer.image_buffer = movie->image_storage;
    movie->drawer.image_width = header->image_width;
    movie->drawer.image_height = header->image_height;
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
      audio->ring.resize(base::ToSize(config->audio_buffer_bytes));

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
  const int32_t padded_frame_bytes = PadSize(frame_bytes);
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

    // Sound is never inside a frame container, so an unknown chunk here is an
    // error rather than something to skip.
    const FramePart part = LoadFramePart(state, chunk->id, chunk_bytes);
    if (part == FramePart::kFailed || part == FramePart::kNone) {
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

  // The drawer keeps a skipped frame's palette in its 256-color copy, so a
  // larger one is malformed.
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
// ring instead; a larger chunk anywhere else is an error. They run only with
// the sound on. Each returns 0, or kVqaErrorRead.

// Accounts for the sound a preloading first chunk wrote at the start of the
// ring: moves the write position past it, back to the start when it filled
// the ring exactly, and marks the whole blocks it filled. The next chunk
// completes a partial last block.
static void CommitPreload(VqaAudio* audio, const VqaConfig& config,
                          const int32_t bytes) {
  audio->write_offset =
      (audio->write_offset + bytes) % config.audio_buffer_bytes;
  for (int32_t i = 0; i < bytes / config.audio_block_bytes; i++) {
    audio->block_loaded.at(base::ToSize(i)) = 1;
  }
}

// Loads an uncompressed sound chunk.
static int32_t LoadSound(VqaPlayerState* state, int32_t chunk_bytes) {
  VqaMovie* movie = state->movie.get();
  VqaAudio* audio = &movie->audio;
  VqaConfig* config = &state->config;
  const int32_t padded_bytes = PadSize(chunk_bytes);

  // The first chunk, too big for staging, preloads the ring.
  if (padded_bytes > audio->staging_capacity && audio->write_offset == 0) {
    if (padded_bytes > config->audio_buffer_bytes) {
      return kVqaErrorRead;
    }

    if (!state->io->Read(std::span(audio->ring), padded_bytes)) {
      return kVqaErrorRead;
    }

    CommitPreload(audio, *config, chunk_bytes);

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
  ZapHeader zap_header{};

  VqaMovie* movie = state->movie.get();
  VqaAudio* audio = &movie->audio;
  VqaConfig* config = &state->config;
  int32_t padded_bytes = PadSize(chunk_bytes);

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
      if (!state->io->Read(std::span(audio->ring), padded_bytes)) {
        return kVqaErrorRead;
      }
    } else {
      // Loaded at the end of the ring and decompressed towards its start.
      const auto compressed =
          std::span(audio->ring)
              .subspan(base::ToSize(config->audio_buffer_bytes - padded_bytes));

      if (!state->io->Read(compressed, padded_bytes)) {
        return kVqaErrorRead;
      }

      // TODO: AudioUnzap() is a stub that writes nothing, so the ring keeps
      // the compressed bytes and whatever was there before, and plays them.
      // The shipped Red Alert movies have no SND1 sound.
      AudioUnzap(compressed,
                 std::span(audio->ring).first(zap_header.UnCompSize));
    }

    CommitPreload(audio, *config, zap_header.UnCompSize);

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
    const auto compressed =
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
  VqaMovie* movie = state->movie.get();
  VqaAudio* audio = &movie->audio;
  VqaConfig* config = &state->config;
  const int32_t padded_bytes = PadSize(chunk_bytes);

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
    // decoded_bytes fits the ring, being above staging_capacity.
    if (padded_bytes > config->audio_buffer_bytes) {
      return kVqaErrorRead;
    }

    // Loaded at the end of the ring and decompressed towards its start.
    const auto compressed =
        std::span(audio->ring)
            .subspan(base::ToSize(config->audio_buffer_bytes - padded_bytes));

    if (!state->io->Read(compressed, padded_bytes)) {
      return kVqaErrorRead;
    }

    // TODO: A failed decode (the decoder takes only 16-bit mono) is ignored,
    // so the ring plays whatever it held. The shipped Red Alert movies are
    // all 16-bit mono.
    DecodeAdpcmSound(&audio->adpcm, audio->channels, audio->bits_per_sample,
                     compressed,
                     std::span(audio->ring).first(base::ToSize(decoded_bytes)));

    CommitPreload(audio, *config, decoded_bytes);

    return 0;
  }

  // Only the first chunk may exceed staging.
  if (padded_bytes > audio->staging_capacity ||
      decoded_bytes > audio->staging_capacity) {
    return kVqaErrorRead;
  }

  // Loaded at the end of staging and decompressed towards its start.
  const auto compressed =
      std::span(audio->staging)
          .subspan(base::ToSize(audio->staging_capacity - padded_bytes));

  if (!state->io->Read(compressed, padded_bytes)) {
    return kVqaErrorRead;
  }

  // TODO: A failed decode is ignored here too.
  DecodeAdpcmSound(
      &audio->adpcm, audio->channels, audio->bits_per_sample, compressed,
      std::span(audio->staging).first(base::ToSize(decoded_bytes)));

  audio->staged_bytes = decoded_bytes;

  return 0;
}

// Loads a chunk a frame is built from into the loader's current frame, and
// flags the frame as the chunk says: key, or carrying a palette.
static FramePart LoadFramePart(VqaPlayerState* state, const uint32_t id,
                               const int32_t chunk_bytes) {
  int32_t result = 0;
  uint32_t frame_flags = 0;
  FramePart part = FramePart::kLoaded;
  switch (id) {
    case kChunkCbf0:
      result = LoadFullCodebook(state, chunk_bytes);
      break;
    case kChunkCbfz:
      result = LoadCompressedFullCodebook(state, chunk_bytes);
      break;
    case kChunkCbp0:
      result = LoadPartialCodebook(state, chunk_bytes);
      break;
    case kChunkCbpz:
      result = LoadCompressedPartialCodebook(state, chunk_bytes);
      break;
    case kChunkCpl0:
      result = LoadPalette(state, chunk_bytes);
      frame_flags = kFrameHasPalette;
      break;
    case kChunkCplz:
      result = LoadCompressedPalette(state, chunk_bytes);
      frame_flags = kFrameHasPalette;
      break;
    case kChunkVpt0:
      result = LoadVectorPointers(state, chunk_bytes);
      part = FramePart::kVectorPointers;
      break;
    case kChunkVptz:
    case kChunkVptd:
      result = LoadCompressedVectorPointers(state, chunk_bytes);
      part = FramePart::kVectorPointers;
      break;
    // A key frame's vector pointers; key frames are never skipped.
    case kChunkVptk:
      result = LoadCompressedVectorPointers(state, chunk_bytes);
      frame_flags = kFrameKey;
      part = FramePart::kVectorPointers;
      break;
    default:
      return FramePart::kNone;
  }

  if (result != 0) {
    return FramePart::kFailed;
  }
  state->movie->loader.current_frame->flags |= frame_flags;
  return part;
}

// Loads a sound chunk of either track with the loader for its compression.
static int32_t LoadSoundChunk(VqaPlayerState* state, const uint32_t id,
                              const int32_t chunk_bytes) {
  if (id == kChunkSnd0 || id == kChunkSna0) {
    return LoadSound(state, chunk_bytes);
  }
  if (id == kChunkSnd1 || id == kChunkSna1) {
    return LoadZapSound(state, chunk_bytes);
  }
  return LoadAdpcmSound(state, chunk_bytes);
}
