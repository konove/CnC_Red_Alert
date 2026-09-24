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

// Originally written by Bill Randolph and Denzil E. Long, Jr. at Westwood
// Studios, August 1995.

#include "winvq/vqa32/movie_loader.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <mutex>
#include <span>

#include "absl/log/log.h"
#include "base/numeric.h"
#include "winvq/vqa32/adpcm_decoders.h"
#include "winvq/vqa32/audio_output.h"
#include "winvq/vqa32/audio_ring.h"
#include "winvq/vqa32/chunk_reader.h"
#include "winvq/vqa32/frame_ring.h"
#include "winvq/vqa32/lcw_buffer.h"
#include "winvq/vqa32/vqa_audio_device.h"
#include "winvq/vqa32/vqa_format.h"
#include "winvq/vqa32/vqaio.h"

MovieLoader::MovieLoader(VqaIo& io, const VqaHeader& header, FrameRing& ring,
                         AudioRing* const audio, AudioOutput* const output,
                         const AudioFormat& format)
    : reader_(io),
      header_(&header),
      ring_(&ring),
      audio_(audio),
      output_(output),
      format_(format) {}

LoadStatus MovieLoader::LoadNextFrame() {
  // Every frame the header counts is loaded.
  if (next_frame_number_ >= int{header_->frame_count}) {
    return LoadStatus::kEndOfMovie;
  }

  // The next buffer still holds a frame the drawer has not released. Wait
  // for it, which also gives the drawer the turn.
  Frame& frame = ring_->load_frame();
  if (frame.loaded) {
    return LoadStatus::kNoBuffer;
  }

  // A new frame, not a resumed one, uses the last full codebook. It is taken
  // now because the last frame of a group completes the next codebook, which
  // moves full_codebook_ on.
  if (!pending_sound_.has_value()) {
    frame.codebook = full_codebook_;
  }

  bool frame_loaded = false;
  while (!frame_loaded) {
    // A resumed loader is inside a sound chunk already. The end of the file
    // before the last frame is a truncated movie.
    Chunk chunk;
    if (pending_sound_.has_value()) {
      chunk = *pending_sound_;
    } else {
      const auto next = reader_.Next();
      if (!next.has_value()) {
        return LoadStatus::kFailed;
      }
      chunk = *next;
    }

    // Codebooks, palettes and vector pointers. In the older format without
    // frame containers, the vector pointers come last in a frame.
    const FramePart part = LoadFramePart(chunk);
    if (part == FramePart::kFailed) {
      return LoadStatus::kFailed;
    }
    if (part != FramePart::kNone) {
      frame_loaded = part == FramePart::kVectorPointers;
      continue;
    }

    switch (chunk.id) {
      // A frame container, of a key frame for VQFK.
      case kChunkVqfr:
      case kChunkVqfk:
        if (!LoadFrameContainer(chunk)) {
          return LoadStatus::kFailed;
        }
        frame.key = frame.key || chunk.id == kChunkVqfk;
        frame_loaded = true;
        break;

      // Sound. SND* chunks are the primary track and SNA* the alternate one,
      // which is skipped, as is all of it without sound. Before
      // staging a chunk, the last one's sound moves from staging into the
      // ring; with no room there the loader stops and resumes here.
      case kChunkSnd0:
      case kChunkSnd1:
      case kChunkSnd2:
      case kChunkSna0:
      case kChunkSna1:
      case kChunkSna2: {
        const bool alternate_chunk = chunk.id == kChunkSna0 ||
                                     chunk.id == kChunkSna1 ||
                                     chunk.id == kChunkSna2;
        if (audio_ == nullptr || alternate_chunk) {
          if (!reader_.Skip(chunk)) {
            return LoadStatus::kFailed;
          }
          break;
        }

        if (!CopyStagedSound()) {
          pending_sound_ = chunk;
          return LoadStatus::kAudioFull;
        }
        pending_sound_.reset();

        if (!LoadSoundChunk(chunk)) {
          return LoadStatus::kFailed;
        }
        break;
      }

      // Skip any unknown chunks.
      default:
        if (!reader_.Skip(chunk)) {
          return LoadStatus::kFailed;
        }
        break;
    }
  }

  // Number the frame and hand it to the drawer.
  frame.frame_number = next_frame_number_++;
  ring_->FinishLoading();
  return LoadStatus::kLoaded;
}

bool MovieLoader::LoadFrameContainer(const Chunk& container) {
  int64_t bytes_loaded = 0;  // 64-bit: sums sizes up to 2^31 each.

  while (bytes_loaded < container.padded_size()) {
    const auto chunk = reader_.Next();
    if (!chunk.has_value()) {
      return false;
    }

    // The chunk header, and the payload with its pad byte.
    bytes_loaded += 8;
    bytes_loaded += chunk->padded_size();

    // Sound is never inside a frame container, so an unknown chunk here is an
    // error rather than something to skip.
    const FramePart part = LoadFramePart(*chunk);
    if (part == FramePart::kFailed || part == FramePart::kNone) {
      return false;
    }
  }
  return true;
}

MovieLoader::FramePart MovieLoader::LoadFramePart(const Chunk& chunk) {
  Frame& frame = ring_->load_frame();
  bool loaded = false;
  FramePart part = FramePart::kLoaded;
  switch (chunk.id) {
    case kChunkCbf0:
    case kChunkCbfz:
      loaded = LoadFullCodebook(chunk, chunk.id == kChunkCbfz);
      break;
    case kChunkCbp0:
    case kChunkCbpz:
      loaded = LoadPartialCodebook(chunk, chunk.id == kChunkCbpz);
      break;
    case kChunkCpl0:
    case kChunkCplz:
      loaded = LoadPalette(chunk, chunk.id == kChunkCplz);
      frame.has_palette = frame.has_palette || loaded;
      break;
    case kChunkVpt0:
    case kChunkVptz:
    case kChunkVptd:
      loaded = LoadVectorPointers(chunk, chunk.id != kChunkVpt0);
      part = FramePart::kVectorPointers;
      break;
    // A key frame's vector pointers; key frames are never skipped.
    case kChunkVptk:
      loaded = LoadVectorPointers(chunk, true);
      frame.key = frame.key || loaded;
      part = FramePart::kVectorPointers;
      break;
    default:
      return FramePart::kNone;
  }
  return loaded ? part : FramePart::kFailed;
}

void MovieLoader::CompleteCodebook() {
  partial_count_ = 0;
  partial_bytes_ = 0;
  full_codebook_ = partial_codebook_;
  partial_codebook_ = ring_->next_codebook(partial_codebook_);
}

bool MovieLoader::LoadFullCodebook(const Chunk& chunk, const bool compressed) {
  // A full codebook replaces any pieces collected so far.
  LcwBuffer& buffer = ring_->codebook(partial_codebook_).data;
  if (!(compressed ? buffer.LoadCompressed(reader_, chunk)
                   : buffer.LoadRaw(reader_, chunk))) {
    return false;
  }
  CompleteCodebook();
  return true;
}

// Raw pieces collect from the start of the buffer. Compressed pieces are
// decompressed together, as one, so they collect back from the end: the
// group's first piece places the whole compressed codebook, whose size is not
// known yet, estimated as this piece's size times the pieces in a group, plus
// 100 bytes. The estimate assumes every piece is the size of the first, so a
// larger later piece can still run off the end.
bool MovieLoader::LoadPartialCodebook(const Chunk& chunk,
                                      const bool compressed) {
  LcwBuffer& buffer = ring_->codebook(partial_codebook_).data;
  const int frames_per_group = header_->frames_per_group;

  if (compressed && partial_bytes_ == 0) {
    // 64-bit because a large chunk times the group size overflows int32_t.
    // A negative estimate would place the codebook before the buffer.
    const int64_t estimated_offset =
        buffer.capacity() -
        ((int64_t{chunk.padded_size()} * frames_per_group) + 100);
    if (estimated_offset < 0) {
      return false;
    }
    partial_offset_ = static_cast<int32_t>(estimated_offset);
  }
  const int32_t start = compressed ? partial_offset_ : 0;

  if (!buffer.ReadAt(reader_, chunk, int64_t{start} + partial_bytes_)) {
    return false;
  }

  // Each piece's pad byte is overwritten by the next piece.
  partial_bytes_ += chunk.size;
  partial_count_++;

  if (partial_count_ == frames_per_group) {
    if (compressed) {
      buffer.SetCompressed(start, partial_bytes_);
    } else {
      buffer.SetRaw(partial_bytes_);
    }
    CompleteCodebook();
  }
  return true;
}

bool MovieLoader::LoadPalette(const Chunk& chunk, const bool compressed) {
  LcwBuffer& palette = ring_->load_frame().palette;
  if (compressed) {
    return palette.LoadCompressed(reader_, chunk);
  }
  // The drawer keeps a skipped frame's palette in a 256-color copy, so a
  // larger raw one is malformed.
  return chunk.padded_size() <= kMaxPaletteBytes &&
         palette.LoadRaw(reader_, chunk);
}

bool MovieLoader::LoadVectorPointers(const Chunk& chunk,
                                     const bool compressed) {
  LcwBuffer& pointers = ring_->load_frame().pointers;
  return compressed ? pointers.LoadCompressed(reader_, chunk)
                    : pointers.LoadRaw(reader_, chunk);
}

bool MovieLoader::CopyStagedSound() {
  const std::scoped_lock<VqaAudioDevice> lock(output_->device());
  return audio_->CopyStaged();
}

// The sound chunk loaders stage a chunk's sound in the audio ring's staging
// buffer, for CopyStaged() to move into the ring. The movie's first sound
// chunk may be larger than staging - it preloads the sound - and goes straight
// into the ring instead; a larger chunk anywhere else is an error.

bool MovieLoader::LoadSoundChunk(const Chunk& chunk) {
  if (chunk.id == kChunkSnd0) {
    return LoadSound(chunk);
  }
  if (chunk.id == kChunkSnd1) {
    return LoadZapSound(chunk);
  }
  return LoadAdpcmSound(chunk);
}

bool MovieLoader::LoadSound(const Chunk& chunk) {
  // The first chunk, too big for staging, preloads the ring.
  if (chunk.padded_size() > audio_->staging_capacity() &&
      audio_->write_offset() == 0) {
    if (!reader_.ReadPayload(chunk, audio_->ring())) {
      return false;
    }
    audio_->CommitPreload(chunk.size);
    return true;
  }

  // Only the first chunk may exceed staging.
  if (!reader_.ReadPayload(chunk, audio_->staging())) {
    return false;
  }
  audio_->Stage(chunk.size);
  return true;
}

bool MovieLoader::LoadZapSound(const Chunk& chunk) {
  // The ZAP header is part of the chunk; a shorter chunk would leave a
  // negative payload size.
  ZapHeader zap_header{};
  if (chunk.size < int32_t{sizeof(ZapHeader)} ||
      !reader_.ReadObject(zap_header)) {
    return false;
  }

  // The sound after the header.
  const int32_t padded_bytes = chunk.padded_size() - int32_t{sizeof(ZapHeader)};
  const int32_t sound_bytes = zap_header.uncompressed_size;
  const bool stored =
      zap_header.uncompressed_size == zap_header.compressed_size;

  // The first chunk, too big for staging, preloads the ring; any other chunk
  // must fit staging.
  const bool preload =
      sound_bytes > audio_->staging_capacity() && audio_->write_offset() == 0;
  const std::span<unsigned char> target =
      preload ? audio_->ring() : audio_->staging();
  if (padded_bytes > std::ssize(target) || sound_bytes > std::ssize(target)) {
    return false;
  }

  if (stored) {
    if (!reader_.Read(target, padded_bytes)) {
      return false;
    }
  } else {
    // Loaded at the end of the target and decompressed towards its start.
    const auto compressed =
        target.subspan(base::ToSize(std::ssize(target) - padded_bytes));
    if (!reader_.Read(compressed, padded_bytes)) {
      return false;
    }
    // TODO: DecodeZapSound() is a stub that writes nothing, so the target
    // keeps the compressed bytes and whatever was there before, and plays
    // them. The shipped Red Alert movies have no SND1 sound.
    DecodeZapSound(compressed, target.first(base::ToSize(sound_bytes)));
  }

  if (preload) {
    audio_->CommitPreload(sound_bytes);
  } else {
    audio_->Stage(sound_bytes);
  }
  return true;
}

bool MovieLoader::LoadAdpcmSound(const Chunk& chunk) {
  const int32_t padded_bytes = chunk.padded_size();

  // Two samples a byte. 64-bit so an oversized chunk cannot overflow before
  // the bounds checks.
  const int64_t wide_decoded_bytes =
      int64_t{chunk.size} * (format_.bits_per_sample / 4);
  if (wide_decoded_bytes >
      std::max(audio_->capacity(), audio_->staging_capacity())) {
    return false;
  }
  const auto decoded_bytes = static_cast<int32_t>(wide_decoded_bytes);

  // The first chunk, too big for staging, preloads the ring; any other chunk
  // must fit staging.
  const bool preload =
      decoded_bytes > audio_->staging_capacity() && audio_->write_offset() == 0;
  const std::span<unsigned char> target =
      preload ? audio_->ring() : audio_->staging();
  if (padded_bytes > std::ssize(target) || decoded_bytes > std::ssize(target)) {
    return false;
  }

  // Loaded at the end of the target and decompressed towards its start.
  const auto compressed =
      target.subspan(base::ToSize(std::ssize(target) - padded_bytes));
  if (!reader_.Read(compressed, padded_bytes)) {
    return false;
  }

  // TODO: A format the decoder does not produce, or a failed decode, leaves
  // the target as it was, so the ring plays whatever it held. The shipped Red
  // Alert movies are all 16-bit mono.
  if (ImaAdpcmDecoder::Supports(format_.channels, format_.bits_per_sample)) {
    adpcm_.Decode(compressed, target.first(base::ToSize(decoded_bytes)));
  } else {
    DLOG_FIRST_N(WARNING, 1)
        << "IMA ADPCM sound with " << format_.channels << " channels of "
        << format_.bits_per_sample << " bits cannot be decoded";
  }

  if (preload) {
    audio_->CommitPreload(decoded_bytes);
  } else {
    audio_->Stage(decoded_bytes);
  }
  return true;
}
