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
// Studios, July 1995, where the loader and drawer ran as tasks off a timer
// interrupt.

#include "engine/video/vqa/movie.h"

#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <string_view>

#ifdef _WIN32
#include <windows.h>
#endif

#include "engine/base/buffer.h"
#include "engine/video/vqa/audio_output.h"
#include "engine/video/vqa/audio_ring.h"
#include "engine/video/vqa/chunk_reader.h"
#include "engine/video/vqa/frame_ring.h"
#include "engine/video/vqa/movie_clock.h"
#include "engine/video/vqa/movie_drawer.h"
#include "engine/video/vqa/movie_loader.h"
#include "engine/video/vqa/vqa_audio_device.h"
#include "engine/video/vqa/vqa_format.h"
#include "engine/video/vqa/vqa_player.h"
#include "engine/video/vqa/vqaio.h"

std::expected<std::unique_ptr<Movie>, VqaError> Movie::Open(
    VqaIo& io, const std::string_view name, VqaClient& client,
    VqaAudioDevice* const audio, const VqaOptions& options) {
  if (!io.Open(name)) {
    return std::unexpected(VqaError::kOpen);
  }

  // From here on the movie owns the open file, and destroying it on a failure
  // below closes the file again. The constructor is private, so make_unique
  // cannot reach it.
  std::unique_ptr<Movie> movie(new Movie(io, client));
  if (auto read = movie->ReadHeader(audio, options); !read.has_value()) {
    return std::unexpected(read.error());
  }
  if (auto preload = movie->Preload(options); !preload.has_value()) {
    return std::unexpected(preload.error());
  }
  return movie;
}

Movie::Movie(VqaIo& io, VqaClient& client) : io_(&io), client_(&client) {}

Movie::~Movie() {
  // The sound goes first: its mixer reads the rings.
  audio_output_.reset();
  io_->Close();
}

std::expected<void, VqaError> Movie::ReadHeader(VqaAudioDevice* const audio,
                                                const VqaOptions& options) {
  ChunkReader reader(*io_);

  // The file must be an IFF FORM of type WVQA.
  const auto form = reader.Next();
  if (!form.has_value()) {
    return std::unexpected(form.error() == ChunkError::kEndOfFile
                               ? VqaError::kRead
                               : VqaError::kNotVqa);
  }
  if (form->id != kChunkForm || form->size == 0) {
    return std::unexpected(VqaError::kNotVqa);
  }

  // The form type follows the FORM header.
  const std::optional<uint32_t> form_type = reader.ReadId();
  if (!form_type.has_value()) {
    return std::unexpected(VqaError::kRead);
  }
  if (*form_type != kFormWvqa) {
    return std::unexpected(VqaError::kNotVqa);
  }

  // Read the chunks in front of the frames, up to FINF, the last of them.
  // VQHD must come before it; anything else is skipped.
  while (true) {
    const auto next = reader.Next();
    if (!next.has_value()) {
      return std::unexpected(next.error() == ChunkError::kEndOfFile
                                 ? VqaError::kRead
                                 : VqaError::kNotVqa);
    }
    const Chunk& chunk = *next;

    switch (chunk.id) {
      // The movie header. Only one: the play buffers are sized from it.
      case kChunkVqhd: {
        if (chunk.size != int32_t{sizeof(VqaHeader)} || ring_ != nullptr) {
          return std::unexpected(VqaError::kNotVqa);
        }
        if (!reader.ReadPayload(chunk, base::ObjectBytes(header_))) {
          return std::unexpected(VqaError::kRead);
        }
        // These fields are divisors when sizing buffers and timing playback.
        if (header_.block_width == 0 || header_.block_height == 0 ||
            header_.frames_per_group == 0 || header_.fps == 0) {
          return std::unexpected(VqaError::kNotVqa);
        }
        if (auto allocated = Allocate(audio, options); !allocated.has_value()) {
          return allocated;
        }
        break;
      }

      // The frame table, which the player does not use, is the last chunk
      // before the frames. A movie without a header before it cannot play.
      case kChunkFinf:
        if (ring_ == nullptr) {
          return std::unexpected(VqaError::kNotVqa);
        }
        if (!reader.Skip(chunk)) {
          return std::unexpected(VqaError::kSeek);
        }
        return {};

      // Chunks the player has no use for, such as PINF.
      default:
        if (!reader.Skip(chunk)) {
          return std::unexpected(VqaError::kSeek);
        }
        break;
    }
  }
}

std::expected<void, VqaError> Movie::Allocate(VqaAudioDevice* const audio,
                                              const VqaOptions& options) {
  if (options.frame_buffers <= 0 || options.codebook_buffers <= 0) {
    return std::unexpected(VqaError::kNoMemory);
  }

  // Compressed data is loaded at the end of its buffer and decompressed in
  // place towards the start, so each buffer is the decompressed size plus
  // slack (250 bytes for a codebook, 1 KiB for a palette or the vector
  // pointers): room for the output never to catch up with input still to be
  // read, even for data LCW could not shrink. The sizes are rounded down to a
  // multiple of 4, which kept the DOS buffers DWORD aligned.
  const int codebook_capacity =
      ((header_.codebook_entries * header_.block_width * header_.block_height) +
       250) /
      4 * 4;
  const int palette_capacity = (kMaxPaletteBytes + 1024) / 4 * 4;
  // Two bytes per block.
  const int pointers_capacity =
      (((header_.image_width / header_.block_width) *
        (header_.image_height / header_.block_height) * int{sizeof(int16_t)}) +
       1024) /
      4 * 4;
  ring_ = std::make_unique<FrameRing>(
      options.frame_buffers, options.codebook_buffers, codebook_capacity,
      pointers_capacity, palette_capacity);

  // The sound, when the movie has some and there is a device to play it on.
  // A ring of less than one block is no ring, and the movie plays without
  // sound, so no sound code has to handle an empty ring.
  const AudioFormat format = AudioFormat::FromHeader(header_);
  if ((header_.flags & kVqaHasAudio) != 0 && audio != nullptr) {
    // Sizing the ring divides by the block size.
    const int block_bytes = options.audio_block_bytes;
    if (block_bytes <= 0) {
      return std::unexpected(VqaError::kAudio);
    }
    // By default, as many whole blocks as fit in 1.5 seconds of sound.
    const int bytes_per_second = format.bytes_per_second();
    const int ring_bytes = options.audio_ring_bytes.value_or(
        (bytes_per_second + (bytes_per_second / 2)) / block_bytes *
        block_bytes);
    const int block_count = ring_bytes / block_bytes;
    if (block_count > 0) {
      // Staging holds one chunk's sound: twice one frame's worth, for chunks
      // that run long, plus 100 bytes.
      audio_ = std::make_unique<AudioRing>(
          block_count, block_bytes, (bytes_per_second / header_.fps * 2) + 100);
      audio_output_ = AudioOutput::Create(*audio, *audio_, format);
      if (audio_output_ == nullptr) {
        return std::unexpected(VqaError::kAudio);
      }
    }
  }

  loader_ = std::make_unique<MovieLoader>(*io_, header_, *ring_, audio_.get(),
                                          audio_output_.get(), format);
  return {};
}

std::expected<void, VqaError> Movie::Preload(const VqaOptions& options) {
  drawer_ = std::make_unique<MovieDrawer>(*ring_, clock_, audio_.get(), header_,
                                          *client_, options.skip_late_frames);

  for (int i = 0; i < options.frame_buffers; ++i) {
    switch (loader_->LoadNextFrame()) {
      case LoadStatus::kLoaded:
      case LoadStatus::kNoBuffer:
      case LoadStatus::kAudioFull:
        break;
      // A movie with fewer frames than buffers ends while priming.
      case LoadStatus::kEndOfMovie:
        return {};
      case LoadStatus::kFailed:
      default:
        return std::unexpected(VqaError::kRead);
    }
  }
  return {};
}

void Movie::Start() {
  started_ = true;

  // The sound starts first, so the clock can run from it, and only if some
  // was preloaded. Another movie's sound holding the device leaves this one
  // silent, on the system clock.
  if (audio_output_ != nullptr && audio_->block_loaded(0)) {
    audio_output_->Start();
  }

  // The clock reads the time of the first frame loaded, so it is due now.
  const int64_t first_frame_ticks = int64_t{ring_->draw_frame().frame_number} *
                                    kVqaTicksPerSecond / header_.fps;
  const bool audio_clock = audio_output_ != nullptr && audio_output_->playing();
  clock_.Set(first_frame_ticks, audio_clock ? audio_output_.get() : nullptr);
}

void Movie::Finish() {
  ended_ = true;
  if (audio_output_ != nullptr) {
    audio_output_->Stop();
  }
}

void Movie::Pause() {
  if (paused_ || ended_) {
    return;
  }
  paused_ = true;
  // Before the first step there is no clock or sound to hold yet.
  if (!started_) {
    return;
  }
  paused_ticks_ = clock_.Now();
  paused_sound_ = audio_output_ != nullptr && audio_output_->playing();
  if (paused_sound_) {
    audio_output_->Stop();
  }
}

void Movie::Resume() {
  if (!paused_) {
    return;
  }
  paused_ = false;
  if (!started_ || ended_) {
    return;
  }
  // The output keeps its place in the ring across Stop() and Start() but
  // counts its ticks from zero again, so the clock is set to read on from
  // where it stopped. Another movie's sound having taken the device since
  // leaves this one on the system clock.
  const bool audio_clock = paused_sound_ && audio_output_->Start();
  clock_.Set(paused_ticks_, audio_clock ? audio_output_.get() : nullptr);
}

// Each step gives the loader one frame to load and the drawer one frame to
// draw; either may decline (no free buffer, not yet time) and the caller
// comes round again. The loader runs ahead by up to frame_buffers frames,
// which is what absorbs a slow read.
VqaStepResult Movie::Step() {
  if (ended_) {
    return VqaStepResult::kEnded;
  }
  // A paused movie neither loads nor draws; the client still gets its idle
  // call, where it would read the input that resumes the movie.
  if (paused_) {
    client_->OnIdle();
    return VqaStepResult::kWaiting;
  }
  if (!started_) {
    Start();
  }

  // A full ring or a wait on the sound is retried next step. The end of the
  // movie, or a failure, ends the loading: the frames already loaded still
  // play.
  if (!loaded_) {
    const LoadStatus status = loader_->LoadNextFrame();
    if (status == LoadStatus::kEndOfMovie || status == LoadStatus::kFailed) {
      loaded_ = true;
      if (audio_ != nullptr) {
        audio_->MarkMovieLoaded();
      }
    }
  }

  switch (drawer_->DrawNextFrame()) {
    case DrawStatus::kDrawn:
      return VqaStepResult::kFrameShown;
    case DrawStatus::kStopped:
      Finish();
      return VqaStepResult::kEnded;
    // Nothing left to draw once nothing more will be loaded.
    case DrawStatus::kNoFrame:
      if (loaded_) {
        Finish();
        return VqaStepResult::kEnded;
      }
      return VqaStepResult::kWaiting;
    // Too early for the next frame: let the client present or wait instead
    // of the caller spinning.
    case DrawStatus::kNotTime:
    default:
      client_->OnIdle();
      return VqaStepResult::kWaiting;
  }
}

void Movie::Run() {
#ifdef _WIN32
  // Run at high priority while the movie plays, so the busy loop below is not
  // starved of the time slices that keep the frames on schedule.
  const DWORD process_priority = GetPriorityClass(GetCurrentProcess());
  SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif  // _WIN32

  while (Step() != VqaStepResult::kEnded) {
  }

#ifdef _WIN32
  SetPriorityClass(GetCurrentProcess(), process_priority);
#endif  // _WIN32
}
