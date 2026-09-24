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
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/log/log.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "winvq/vqa32/adpcm_decoders.h"
#include "winvq/vqa32/audio_output.h"
#include "winvq/vqa32/audio_ring.h"
#include "winvq/vqa32/chunk_reader.h"
#include "winvq/vqa32/frame_ring.h"
#include "winvq/vqa32/lcw_buffer.h"
#include "winvq/vqa32/vqa_format.h"
#include "winvq/vqa32/vqa_player.h"
#include "winvq/vqa32/vqa_player_state.h"
#include "winvq/vqa32/vqaio.h"
#include "winvq/vqm32/iff.h"

static std::unique_ptr<VqaMovie> AllocateMovie(const VqaHeader* header,
                                               VqaConfig* config);
static int32_t PreloadFrames(VqaPlayerState* state);
static int32_t LoadFrameContainer(VqaPlayerState* state, const Chunk& frame);
static int32_t LoadFullCodebook(const VqaPlayerState* state, const Chunk& chunk,
                                bool compressed);
static int32_t LoadPartialCodebook(const VqaPlayerState* state,
                                   const Chunk& chunk, bool compressed);
static int32_t LoadPalette(const VqaPlayerState* state, const Chunk& chunk,
                           bool compressed);
static int32_t LoadVectorPointers(const VqaPlayerState* state,
                                  const Chunk& chunk, bool compressed);
static int32_t LoadSound(const VqaPlayerState* state, const Chunk& chunk);
static int32_t LoadZapSound(const VqaPlayerState* state, const Chunk& chunk);
static int32_t LoadAdpcmSound(const VqaPlayerState* state, const Chunk& chunk);

// What LoadFramePart() made of a chunk.
enum class FramePart {
  kNone,            // Not a part of a frame; nothing was read.
  kFailed,          // A part that did not fit its buffer or failed to read.
  kLoaded,          // A codebook or a palette.
  kVectorPointers,  // The vector pointers.
};
static FramePart LoadFramePart(VqaPlayerState* state, const Chunk& chunk);
static int32_t LoadSoundChunk(VqaPlayerState* state, const Chunk& chunk);

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
  ChunkReader reader(*state->io);
  VqaHeader* header = &state->header;

  // The file must be an IFF FORM of type WVQA.
  const auto form = reader.Next();
  if (!form.has_value()) {
    return form.error() == ChunkError::kEndOfFile ? kVqaErrorRead
                                                  : kVqaErrorNotVqa;
  }

  if (form->id != ID_FORM || form->size == 0) {
    return kVqaErrorNotVqa;
  }

  // The form type follows the FORM header.
  const std::optional<uint32_t> form_type = reader.ReadId();
  if (!form_type.has_value()) {
    return kVqaErrorRead;
  }

  if (*form_type != kFormWvqa) {
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
    const auto next = reader.Next();
    if (!next.has_value()) {
      return next.error() == ChunkError::kEndOfFile ? kVqaErrorRead
                                                    : kVqaErrorNotVqa;
    }
    const Chunk& chunk = *next;

    switch (chunk.id) {
      // The movie header, which sizes the play buffers.
      case kChunkVqhd:
        // Only one header: the play buffers are sized from it.
        if (std::cmp_not_equal(chunk.size, sizeof(VqaHeader)) ||
            state->movie != nullptr) {
          return kVqaErrorNotVqa;
        }

        if (!reader.ReadPayload(chunk, base::ObjectBytes(*header))) {
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

        if (!reader.Skip(chunk)) {
          return kVqaErrorSeek;
        }

        found_frame_table = true;
        break;

      // Chunks the player has no use for, such as PINF.
      default:
        if (!reader.Skip(chunk)) {
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
  if (state->movie->audio == nullptr) {
    config->option_flags &= ~kVqaOptionAudio;
  }

  // Start the sound output: originally HMI's DOS sound drivers, now the SDL
  // mixer.
  if ((config->option_flags & kVqaOptionAudio) != 0) {
    if (config->audio_device == nullptr) {
      return kVqaErrorAudio;
    }
    state->movie->audio_output =
        AudioOutput::Create(*config->audio_device, *state->movie->audio,
                            state->movie->audio_format);
    if (state->movie->audio_output == nullptr) {
      return kVqaErrorAudio;
    }
  }

  // Preload the frame ring, so playback starts with frames in hand.
  if (PreloadFrames(state) != 0) {
    return kVqaErrorRead;
  }

  return 0;
}

int32_t OpenVqa(VqaPlayerState* state, std::string_view filename,
                VqaConfig* config) {

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
  state->io->Close();

  // Frees the play buffers, stopping the sound first.
  state->Reset();
}

// Moves the staged sound into the audio ring, holding the audio thread off
// while it does. Returns false when the ring has no room for it yet.
static bool CopyStagedSound(VqaMovie* movie) {
  const std::scoped_lock<VqaAudioDevice> lock(movie->audio_output->device());
  return movie->audio->CopyStaged();
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
  Frame* frame = &movie->ring.load_frame();
  Chunk& chunk = loader->chunk;
  ChunkReader reader(*state->io);

  // Every frame the header counts is loaded.
  if (std::cmp_greater_equal(loader->next_frame_number,
                             state->header.frame_count)) {
    return kVqaEndOfMovie;
  }

  // The next buffer still holds a frame the drawer has not released. Wait
  // for it, which also gives the drawer the turn.
  if (frame->loaded) {
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
      const auto next = reader.Next();
      if (!next.has_value()) {
        return next.error() == ChunkError::kEndOfFile ? kVqaEndOfMovie
                                                      : kVqaErrorRead;
      }
      chunk = *next;
    }

    // Codebooks, palettes and vector pointers. In the older format without
    // frame containers, the vector pointers come last in a frame.
    const FramePart part = LoadFramePart(state, chunk);
    if (part == FramePart::kFailed) {
      return kVqaErrorRead;
    }
    if (part != FramePart::kNone) {
      frame_loaded = part == FramePart::kVectorPointers;
      continue;
    }

    switch (chunk.id) {
      // A frame container, of a key frame for VQFK.
      case kChunkVqfr:
      case kChunkVqfk:
        if (LoadFrameContainer(state, chunk)) {
          return kVqaErrorRead;
        }

        if (chunk.id == kChunkVqfk) {
          frame->key = true;
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
        const bool alternate_chunk = chunk.id == kChunkSna0 ||
                                     chunk.id == kChunkSna1 ||
                                     chunk.id == kChunkSna2;
        const uint32_t options = state->config.option_flags;
        const bool alternate_track = (options & kVqaOptionAltAudio) != 0;
        if ((options & kVqaOptionAudio) == 0 ||
            alternate_chunk != alternate_track) {
          if (!reader.Skip(chunk)) {
            return kVqaErrorSeek;
          }
          break;
        }

        if (!CopyStagedSound(movie)) {
          movie->flags |= kMovieLoaderAsleep;
          return kVqaSleeping;
        }
        movie->flags &= ~kMovieLoaderAsleep;

        if (LoadSoundChunk(state, chunk) != 0) {
          return kVqaErrorRead;
        }
        break;
      }

      // Skip any unknown chunks.
      default:
        if (!reader.Skip(chunk)) {
          return kVqaErrorSeek;
        }
        break;
    }
  }

  // Number the frame and hand it to the drawer.
  frame->frame_number = loader->next_frame_number;
  loader->next_frame_number++;

  movie->ring.FinishLoading();

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

  // Compressed data is loaded at the end of its buffer and decompressed in
  // place towards the start, so each buffer is the decompressed size plus
  // slack (250 bytes for a codebook, 1 KiB for a palette or the vector
  // pointers): room for the output never to catch up with input still to be
  // read, even for data LCW could not shrink. The sizes are rounded down to a
  // multiple of 4, which kept the DOS buffers DWORD aligned.
  const int32_t codebook_capacity =
      ((header->codebook_entries * header->block_width * header->block_height) +
       250) /
      4 * 4;

  // 256 colors of 3 bytes.
  const int32_t palette_capacity = (768 + 1024) / 4 * 4;

  // Two bytes per block.
  const int32_t pointers_capacity =
      (((header->image_width / header->block_width) *
        (header->image_height / header->block_height) * int{sizeof(int16_t)}) +
       1024) /
      4 * 4;

  // The loader starts assembling the first codebook into the first node, and
  // the loader and the drawer both start at the first frame.
  auto owned_movie = std::make_unique<VqaMovie>(
      FrameRing(config->frame_buffer_count, config->codebook_buffer_count,
                codebook_capacity, pointers_capacity, palette_capacity));
  VqaMovie* movie = owned_movie.get();
  movie->codebook_capacity = codebook_capacity;
  movie->palette_capacity = palette_capacity;
  movie->pointers_capacity = pointers_capacity;

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
    const bool alternate = (config->option_flags & kVqaOptionAltAudio) != 0 &&
                           (header->flags & kVqaHasAltAudio) != 0;
    movie->audio_format = AudioFormat::FromHeader(*header, alternate);
    const int bytes_per_second = movie->audio_format.bytes_per_second();

    // By default, as many whole blocks as fit in 1.5 seconds of sound.
    if (config->audio_buffer_bytes == -1) {
      const auto blocks = (bytes_per_second + (bytes_per_second / 2)) /
                          config->audio_block_bytes;
      config->audio_buffer_bytes = config->audio_block_bytes * blocks;
    }
    // The ring is filled and played in whole blocks.
    config->audio_buffer_bytes -=
        config->audio_buffer_bytes % config->audio_block_bytes;

    // Less than one block is no ring at all; OpenVqa() then turns the sound
    // off. Staging holds one chunk's sound: twice one frame's worth, for
    // chunks that run long, plus 100 bytes.
    if (config->audio_buffer_bytes > 0) {
      movie->audio = std::make_unique<AudioRing>(
          config->audio_buffer_bytes / config->audio_block_bytes,
          config->audio_block_bytes,
          (bytes_per_second / header->fps * 2) + 100);
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

// Loads the chunks inside a VQFR or VQFK frame container: codebooks, palette
// and vector pointers. Returns 0, kVqaEndOfMovie when the file ends inside it,
// or kVqaErrorRead for a bad or unknown chunk.
static int32_t LoadFrameContainer(VqaPlayerState* state, const Chunk& frame) {
  ChunkReader reader(*state->io);
  int64_t bytes_loaded = 0;  // 64-bit: sums sizes up to 2^31 each.

  while (bytes_loaded < frame.padded_size()) {
    const auto chunk = reader.Next();
    if (!chunk.has_value()) {
      return chunk.error() == ChunkError::kEndOfFile ? kVqaEndOfMovie
                                                     : kVqaErrorRead;
    }

    // The chunk header, and the payload with its pad byte.
    bytes_loaded += 8;
    bytes_loaded += chunk->padded_size();

    // Sound is never inside a frame container, so an unknown chunk here is an
    // error rather than something to skip.
    const FramePart part = LoadFramePart(state, *chunk);
    if (part == FramePart::kFailed || part == FramePart::kNone) {
      return kVqaErrorRead;
    }
  }

  return 0;
}

// The chunk loaders below each read one chunk's payload and pad byte into
// the loader's current frame or codebook node. Each returns 0, or
// kVqaErrorRead when the chunk does not fit its buffer or the read fails.

// Makes the codebook being assembled the full codebook, used by the frames
// loaded from now on; the next group's pieces go to the node after it.
static void CompleteCodebook(VqaLoader* loader, const FrameRing& ring) {
  loader->partial_count = 0;
  loader->partial_bytes = 0;
  loader->full_codebook = loader->partial_codebook;
  loader->partial_codebook = ring.next_codebook(loader->partial_codebook);
}

// Loads a full codebook, raw or compressed, into the node being assembled. It
// replaces any pieces collected so far.
static int32_t LoadFullCodebook(const VqaPlayerState* state, const Chunk& chunk,
                                const bool compressed) {
  VqaLoader* loader = &state->movie->loader;
  ChunkReader reader(*state->io);
  LcwBuffer& buffer =
      state->movie->ring.codebook(loader->partial_codebook).data;

  if (!(compressed ? buffer.LoadCompressed(reader, chunk)
                   : buffer.LoadRaw(reader, chunk))) {
    return kVqaErrorRead;
  }

  CompleteCodebook(loader, state->movie->ring);
  return 0;
}

// Appends one piece of the next group's codebook. The group's last piece
// completes it.
//
// Raw pieces collect from the start of the buffer. Compressed pieces are
// decompressed together, as one, so they collect back from the end: the
// group's first piece places the whole compressed codebook, whose size is not
// known yet, estimated as this piece's size times the pieces in a group, plus
// 100 bytes. The estimate assumes every piece is the size of the first, so a
// larger later piece can still run off the end.
static int32_t LoadPartialCodebook(const VqaPlayerState* state,
                                   const Chunk& chunk, const bool compressed) {
  VqaMovie* movie = state->movie.get();
  VqaLoader* loader = &movie->loader;
  ChunkReader reader(*state->io);
  LcwBuffer& buffer = movie->ring.codebook(loader->partial_codebook).data;
  const int frames_per_group = state->header.frames_per_group;

  if (compressed && loader->partial_bytes == 0) {
    // 64-bit because a large chunk times the group size overflows int32_t.
    // A negative estimate would place the codebook before the buffer.
    const int64_t estimated_offset =
        int64_t{movie->codebook_capacity} -
        ((int64_t{chunk.padded_size()} * frames_per_group) + 100);
    if (estimated_offset < 0) {
      return kVqaErrorRead;
    }
    loader->partial_offset = static_cast<int32_t>(estimated_offset);
  }
  const int32_t start = compressed ? loader->partial_offset : 0;

  if (!buffer.ReadAt(reader, chunk, int64_t{start} + loader->partial_bytes)) {
    return kVqaErrorRead;
  }

  // Each piece's pad byte is overwritten by the next piece.
  loader->partial_bytes += chunk.size;
  loader->partial_count++;

  if (loader->partial_count == frames_per_group) {
    if (compressed) {
      buffer.SetCompressed(start, loader->partial_bytes);
    } else {
      buffer.SetRaw(loader->partial_bytes);
    }
    CompleteCodebook(loader, movie->ring);
  }

  return 0;
}

// Loads a palette, raw or compressed, into the frame's palette buffer.
static int32_t LoadPalette(const VqaPlayerState* state, const Chunk& chunk,
                           const bool compressed) {
  Frame* frame = &state->movie->ring.load_frame();
  ChunkReader reader(*state->io);

  // The drawer keeps a skipped frame's palette in its 256-color copy, so a
  // larger raw one is malformed.
  if (!compressed && !FitsInBuffer(0, chunk.padded_size(),
                                   int64_t{sizeof(VqaDrawer::saved_palette)})) {
    return kVqaErrorRead;
  }

  if (!(compressed ? frame->palette.LoadCompressed(reader, chunk)
                   : frame->palette.LoadRaw(reader, chunk))) {
    return kVqaErrorRead;
  }
  return 0;
}

// Loads vector pointers, raw or compressed, into the frame's pointer buffer.
static int32_t LoadVectorPointers(const VqaPlayerState* state,
                                  const Chunk& chunk, const bool compressed) {
  Frame* frame = &state->movie->ring.load_frame();
  ChunkReader reader(*state->io);

  if (!(compressed ? frame->pointers.LoadCompressed(reader, chunk)
                   : frame->pointers.LoadRaw(reader, chunk))) {
    return kVqaErrorRead;
  }
  return 0;
}

// The sound chunk loaders stage a chunk's sound in the audio ring's staging
// buffer, for CopyStaged() to move into the ring. The movie's first sound
// chunk may be larger than staging - it preloads the sound - and goes straight
// into the ring instead; a larger chunk anywhere else is an error. They run
// only with the sound on. Each returns 0, or kVqaErrorRead.

// Loads an uncompressed sound chunk.
static int32_t LoadSound(const VqaPlayerState* state, const Chunk& chunk) {
  AudioRing& audio = *state->movie->audio;
  ChunkReader reader(*state->io);
  const int32_t padded_bytes = chunk.padded_size();

  // The first chunk, too big for staging, preloads the ring.
  if (padded_bytes > audio.staging_capacity() && audio.write_offset() == 0) {
    if (!reader.ReadPayload(chunk, audio.ring())) {
      return kVqaErrorRead;
    }
    audio.CommitPreload(chunk.size);
    return 0;
  }

  // Only the first chunk may exceed staging.
  if (!reader.ReadPayload(chunk, audio.staging())) {
    return kVqaErrorRead;
  }
  audio.Stage(chunk.size);
  return 0;
}

// Loads a sound chunk in Westwood's ZAP ADPCM, which starts with a ZapHeader.
static int32_t LoadZapSound(const VqaPlayerState* state, const Chunk& chunk) {
  AudioRing& audio = *state->movie->audio;
  ChunkReader reader(*state->io);

  // The ZAP header is part of the chunk; a shorter chunk would leave a
  // negative payload size.
  ZapHeader zap_header{};
  if (chunk.size < int32_t{sizeof(ZapHeader)} ||
      !reader.ReadObject(zap_header)) {
    return kVqaErrorRead;
  }

  // The sound after the header.
  const int32_t padded_bytes = chunk.padded_size() - int32_t{sizeof(ZapHeader)};
  const int32_t sound_bytes = zap_header.uncompressed_size;
  const bool stored =
      zap_header.uncompressed_size == zap_header.compressed_size;

  // The first chunk, too big for staging, preloads the ring; any other chunk
  // must fit staging.
  const bool preload =
      sound_bytes > audio.staging_capacity() && audio.write_offset() == 0;
  const std::span<unsigned char> target =
      preload ? audio.ring() : audio.staging();
  if (padded_bytes > std::ssize(target) || sound_bytes > std::ssize(target)) {
    return kVqaErrorRead;
  }

  if (stored) {
    if (!reader.Read(target, padded_bytes)) {
      return kVqaErrorRead;
    }
  } else {
    // Loaded at the end of the target and decompressed towards its start.
    const auto compressed =
        target.subspan(base::ToSize(std::ssize(target) - padded_bytes));
    if (!reader.Read(compressed, padded_bytes)) {
      return kVqaErrorRead;
    }
    // TODO: DecodeZapSound() is a stub that writes nothing, so the target
    // keeps the compressed bytes and whatever was there before, and plays
    // them. The shipped Red Alert movies have no SND1 sound.
    DecodeZapSound(compressed, target.first(base::ToSize(sound_bytes)));
  }

  if (preload) {
    audio.CommitPreload(sound_bytes);
  } else {
    audio.Stage(sound_bytes);
  }
  return 0;
}

// Decodes an IMA ADPCM chunk's sound from source into dest with the movie's
// decoder.
//
// TODO: A format the decoder does not produce, or a failed decode, leaves dest
// as it was, so the ring plays whatever it held. The shipped Red Alert movies
// are all 16-bit mono.
static void DecodeAdpcm(VqaMovie* movie, const std::span<const uint8_t> source,
                        const std::span<uint8_t> dest) {
  const AudioFormat& format = movie->audio_format;
  if (!ImaAdpcmDecoder::Supports(format.channels, format.bits_per_sample)) {
    DLOG_FIRST_N(WARNING, 1)
        << "IMA ADPCM sound with " << format.channels << " channels of "
        << format.bits_per_sample << " bits cannot be decoded";
    return;
  }
  movie->adpcm.Decode(source, dest);
}

// Loads a sound chunk in IMA ADPCM, 4 bits a sample.
static int32_t LoadAdpcmSound(const VqaPlayerState* state, const Chunk& chunk) {
  VqaMovie* movie = state->movie.get();
  AudioRing& audio = *movie->audio;
  ChunkReader reader(*state->io);
  const int32_t padded_bytes = chunk.padded_size();

  // Two samples a byte. 64-bit so an oversized chunk cannot overflow before
  // the bounds checks.
  const int64_t wide_decoded_bytes =
      int64_t{chunk.size} * (movie->audio_format.bits_per_sample / 4);
  if (wide_decoded_bytes >
      std::max(audio.capacity(), audio.staging_capacity())) {
    return kVqaErrorRead;
  }
  const auto decoded_bytes = static_cast<int32_t>(wide_decoded_bytes);

  // The first chunk, too big for staging, preloads the ring; any other chunk
  // must fit staging.
  const bool preload =
      decoded_bytes > audio.staging_capacity() && audio.write_offset() == 0;
  const std::span<unsigned char> target =
      preload ? audio.ring() : audio.staging();
  if (padded_bytes > std::ssize(target) || decoded_bytes > std::ssize(target)) {
    return kVqaErrorRead;
  }

  // Loaded at the end of the target and decompressed towards its start.
  const auto compressed =
      target.subspan(base::ToSize(std::ssize(target) - padded_bytes));
  if (!reader.Read(compressed, padded_bytes)) {
    return kVqaErrorRead;
  }
  DecodeAdpcm(movie, compressed, target.first(base::ToSize(decoded_bytes)));

  if (preload) {
    audio.CommitPreload(decoded_bytes);
  } else {
    audio.Stage(decoded_bytes);
  }
  return 0;
}

// Loads a chunk a frame is built from into the loader's current frame, and
// flags the frame as the chunk says: key, or carrying a palette.
static FramePart LoadFramePart(VqaPlayerState* state, const Chunk& chunk) {
  Frame& frame = state->movie->ring.load_frame();
  int32_t result = 0;
  bool has_palette = false;
  bool key = false;
  FramePart part = FramePart::kLoaded;
  switch (chunk.id) {
    case kChunkCbf0:
    case kChunkCbfz:
      result = LoadFullCodebook(state, chunk, chunk.id == kChunkCbfz);
      break;
    case kChunkCbp0:
    case kChunkCbpz:
      result = LoadPartialCodebook(state, chunk, chunk.id == kChunkCbpz);
      break;
    case kChunkCpl0:
    case kChunkCplz:
      result = LoadPalette(state, chunk, chunk.id == kChunkCplz);
      has_palette = true;
      break;
    case kChunkVpt0:
    case kChunkVptz:
    case kChunkVptd:
      result = LoadVectorPointers(state, chunk, chunk.id != kChunkVpt0);
      part = FramePart::kVectorPointers;
      break;
    // A key frame's vector pointers; key frames are never skipped.
    case kChunkVptk:
      result = LoadVectorPointers(state, chunk, true);
      key = true;
      part = FramePart::kVectorPointers;
      break;
    default:
      return FramePart::kNone;
  }

  if (result != 0) {
    return FramePart::kFailed;
  }
  frame.has_palette = frame.has_palette || has_palette;
  frame.key = frame.key || key;
  return part;
}

// Loads a sound chunk of either track with the loader for its compression.
static int32_t LoadSoundChunk(VqaPlayerState* state, const Chunk& chunk) {
  if (chunk.id == kChunkSnd0 || chunk.id == kChunkSna0) {
    return LoadSound(state, chunk);
  }
  if (chunk.id == kChunkSnd1 || chunk.id == kChunkSna1) {
    return LoadZapSound(state, chunk);
  }
  return LoadAdpcmSound(state, chunk);
}
