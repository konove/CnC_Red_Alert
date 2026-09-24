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

// File: VqaPlayer, opening and closing a movie (OpenVqa, CloseVqa), and the
// playback loop that takes turns between the loader and the drawer (PlayVqa).
//
// Originally written by Bill Randolph and Denzil E. Long, Jr. at Westwood
// Studios, July 1995, where the loader and drawer ran as tasks off a timer
// interrupt.

#include "winvq/vqa32/vqa_player.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#endif

#include "base/buffer.h"
#include "winvq/vqa32/audio_output.h"
#include "winvq/vqa32/audio_ring.h"
#include "winvq/vqa32/chunk_reader.h"
#include "winvq/vqa32/frame_ring.h"
#include "winvq/vqa32/movie_drawer.h"
#include "winvq/vqa32/movie_loader.h"
#include "winvq/vqa32/vqa_format.h"
#include "winvq/vqa32/vqa_player_state.h"
#include "winvq/vqa32/vqaio.h"
#include "winvq/vqm32/iff.h"

// VqaPlayer is a thin wrapper: each method forwards to the matching entry
// point (OpenVqa(), PlayVqa(), ...) on its state.

VqaPlayer::VqaPlayer() : impl_(std::make_unique<VqaPlayerState>()) {}

VqaPlayer::~VqaPlayer() {
  // Only an open movie needs shutdown; Close() on a never-opened player
  // would call Close() on an io object that may never have been installed.
  if (impl_->movie != nullptr) {
    Close();
  }
}

void VqaPlayer::SetIo(VqaIo* io) { impl_->io = io; }

int VqaPlayer::Open(std::string_view filename, VqaConfig* config) {
  return static_cast<int>(OpenVqa(impl_.get(), filename, config));
}

void VqaPlayer::Close() { CloseVqa(impl_.get()); }

int VqaPlayer::Play(int mode) {
  return static_cast<int>(PlayVqa(impl_.get(), mode));
}

// Returns whether the movie's alternate sound track plays: the configuration
// asks for it and the movie has one.
static bool PlaysAlternateTrack(const VqaHeader& header,
                                const VqaConfig& config) {
  return (config.option_flags & kVqaOptionAltAudio) != 0 &&
         (header.flags & kVqaHasAltAudio) != 0;
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

  // The sound's format, its ring and its staging buffer.
  if ((header->flags & kVqaHasAudio) != 0 &&
      (config->option_flags & kVqaOptionAudio) != 0) {
    const int bytes_per_second =
        AudioFormat::FromHeader(*header, PlaysAlternateTrack(*header, *config))
            .bytes_per_second();

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
// the movie ends. Returns 0, or kVqaErrorRead when loading fails; the end of
// a movie shorter than the ring is not a failure.
static int32_t PreloadFrames(VqaPlayerState* state) {
  for (int32_t i = 0; i < state->config.frame_buffer_count; i++) {
    switch (state->movie->loader->LoadNextFrame()) {
      case LoadStatus::kLoaded:
      case LoadStatus::kNoBuffer:
      case LoadStatus::kAudioFull:
        break;
      // A movie with fewer frames than buffers ends while priming.
      case LoadStatus::kEndOfMovie:
        return 0;
      case LoadStatus::kFailed:
      default:
        return kVqaErrorRead;
    }
  }
  return 0;
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
  VqaMovie* movie = state->movie.get();
  const AudioFormat audio_format =
      AudioFormat::FromHeader(*header, PlaysAlternateTrack(*header, *config));
  const bool sound = (config->option_flags & kVqaOptionAudio) != 0;
  if (sound) {
    if (config->audio_device == nullptr) {
      return kVqaErrorAudio;
    }
    movie->audio_output =
        AudioOutput::Create(*config->audio_device, *movie->audio, audio_format);
    if (movie->audio_output == nullptr) {
      return kVqaErrorAudio;
    }
  }

  movie->loader = std::make_unique<MovieLoader>(
      *state->io, *header, movie->ring, sound ? movie->audio.get() : nullptr,
      movie->audio_output.get(), audio_format,
      (config->option_flags & kVqaOptionAltAudio) != 0);
  movie->drawer = std::make_unique<MovieDrawer>(
      movie->ring, movie->clock, movie->audio.get(), *header, *config);

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

// Makes the movie clock read now_ticks, running from the sound when the
// configured clock allows it and the sound is playing, else from the system
// clock.
static void SetMovieClock(VqaMovie* movie, const VqaConfig& config,
                          const int64_t now_ticks) {
  const bool audio_clock = (config.clock_source == kVqaClockDefault ||
                            config.clock_source == kVqaClockAudio) &&
                           movie->audio_output != nullptr &&
                           movie->audio_output->playing();
  movie->clock.Set(now_ticks,
                   audio_clock ? movie->audio_output.get() : nullptr);
}

// Stops the sound, if it plays.
static void StopMovieSound(const VqaMovie* movie) {
  if (movie->audio_output != nullptr) {
    movie->audio_output->Stop();
  }
}

// Each pass of the loop gives the loader one frame to load and the drawer one
// frame to draw; either may decline (no free buffer, not yet time) and the
// loop comes round again, so a RUN spins until the movie is done. The loader
// runs ahead by up to frame_buffer_count frames, which is what absorbs a slow
// read.
int32_t PlayVqa(VqaPlayerState* state, int32_t mode) {
  VqaMovie* const movie = state->movie.get();
  VqaConfig* const config = &state->config;
  int32_t result = 0;

#ifdef _WIN32
  // Run at high priority while the movie plays, so the busy loop below is not
  // starved of the time slices that keep the frames on schedule. Every return
  // below restores the saved level.
  DWORD process_priority = GetPriorityClass(GetCurrentProcess());
  SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif  // _WIN32

  // The first call starts playback. The sound starts first so the clock
  // below can run from it, and only if OpenVqa() preloaded some. With
  // kVqaOptionAudio set the audio ring has at least one block.
  if ((movie->flags & kMovieStarted) == 0) {

    if ((config->option_flags & kVqaOptionAudio) != 0 &&
        movie->audio->block_loaded(0)) {
      movie->audio_output->Start();
    }

    // Set the clock to the time of the first frame loaded, so it is due now.
    const auto first_frame_time = movie->ring.draw_frame().frame_number *
                                  kVqaTicksPerSecond / config->frame_rate;

    SetMovieClock(movie, *config, first_frame_time);

    movie->flags |= kMovieStarted;
  }

  switch (mode) {
    case kVqaModePause:
      if ((movie->flags & kMoviePaused) == 0) {
        movie->flags |= kMoviePaused;
        movie->end_time = movie->clock.Now();

        // The clock follows the sound, so stopping it stops the clock too.
        StopMovieSound(movie);
      }

      result = kVqaPaused;
      break;

    // Shut down below without loading or drawing anything more.
    case kVqaModeStop:
      break;

    case kVqaModeRun:
    case kVqaModeWalk:
    default:

      // Resume a paused movie: the sound, and the clock from where it
      // stopped.
      if ((movie->flags & kMoviePaused) != 0) {
        movie->flags &= ~kMoviePaused;

        // Starting fails only if another movie's sound holds the device,
        // which ends this one.
        if (((config->option_flags & kVqaOptionAudio) != 0) &&
            !movie->audio_output->Start()) {
#ifdef _WIN32
          SetPriorityClass(GetCurrentProcess(), process_priority);
#endif  // _WIN32
          return kVqaEndOfMovie;
        }

        SetMovieClock(movie, *config, movie->end_time);
      }

      // Load, draw, load, draw... until both are done.
      while ((movie->flags & kMovieDone) != kMovieDone) {
        if ((movie->flags & kMovieLoaderDone) == 0) {
          // A full ring or a wait on the sound is retried next pass. The end
          // of the movie, or a failure, ends the loading: the frames already
          // loaded still play.
          const LoadStatus status = movie->loader->LoadNextFrame();
          if (status == LoadStatus::kEndOfMovie ||
              status == LoadStatus::kFailed) {
            movie->flags |= kMovieLoaderDone;
            if (movie->audio != nullptr) {
              movie->audio->MarkMovieLoaded();
            }
            result = 0;
          }
        }

        if ((config->draw_flags & kVqaDrawNothing) == 0) {
          bool stopped = false;
          switch (movie->drawer->DrawNextFrame()) {
            case DrawStatus::kDrawn:
              result = movie->drawer->last_drawn_frame();
              break;
            // frame_callback asked to stop.
            case DrawStatus::kStopped:
              result = kVqaEndOfMovie;
              stopped = true;
              break;
            // Nothing left to draw once nothing more will be loaded.
            case DrawStatus::kNoFrame:
              result = kVqaNoBuffer;
              if ((movie->flags & kMovieLoaderDone) != 0) {
                movie->flags |= kMovieDrawerDone;
              }
              break;
            // Too early for the next frame: let the client present or wait
            // instead of the loop spinning.
            case DrawStatus::kNotTime:
            default:
              result = kVqaNotTime;
              if (config->event_handler != nullptr) {
                config->event_handler(kVqaEventSync, nullptr, 0);
              }
              break;
          }
          if (stopped) {
            break;
          }
        } else {
          // Not drawing: discard each frame as soon as it is loaded.
          movie->flags |= kMovieDrawerDone;
          movie->ring.FinishDrawing();
        }

        if (mode == kVqaModeWalk) {
          break;
        }
      }
      break;
  }

  // Shut down when the movie has played out, was stopped, or its
  // frame_callback asked to stop (the only source of kVqaEndOfMovie above).
  // Only then does the sound stop: a walk returns with it still playing, since
  // the next walk does not restart it and the clock runs from it.
  if ((movie->flags & kMovieDone) == kMovieDone || mode == kVqaModeStop ||
      result == kVqaEndOfMovie) {
    // Read the clock before stopping the sound, since the clock is the
    // amount of sound played.
    movie->end_time = movie->clock.Now();
    StopMovieSound(movie);

    result = kVqaEndOfMovie;
  }

#ifdef _WIN32
  SetPriorityClass(GetCurrentProcess(), process_priority);
#endif  // _WIN32

  return result;
}
