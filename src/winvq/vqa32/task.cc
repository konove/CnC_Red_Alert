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

// File: VqaPlayer, and the playback loop that takes turns between the
// loader and the drawer (PlayVqa), with the small entry points around it.
//
// Originally written by Bill Randolph and Denzil E. Long, Jr. at Westwood
// Studios, July 1995, where the loader and drawer ran as tasks off a timer
// interrupt.

#include <atomic>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <utility>

#include "winvq/vqa32/vqaio.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include "winvq/vqa32/vqa_format.h"
#include "winvq/vqa32/vqaplay.h"
#include "winvq/vqa32/vqaplayp.h"

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

int VqaPlayer::SeekFrame(int frame, int fromwhere) {
  return static_cast<int>(SeekVqaFrame(impl_.get(), frame, fromwhere));
}

int VqaPlayer::SetStop(int frame) { return VQA_SetStop(impl_.get(), frame); }

void VqaPlayer::GetInfo(VQAInfo* info) const { VQA_GetInfo(impl_.get(), info); }

void VqaPlayer::GetStats(VQAStatistics* stats) const {
  VQA_GetStats(impl_.get(), stats);
}

std::atomic<bool> vqa_movie_loaded = false;

// Each pass of the loop gives the loader one frame to load and the drawer one
// frame to draw; either may decline (no free buffer, not yet time) and the
// loop comes round again, so a RUN spins until the movie is done. The loader
// runs ahead by up to frame_buffer_count frames, which is what absorbs a slow
// read.
int32_t PlayVqa(VqaPlayerState* state, int32_t mode) {
  VqaMovie* movie = nullptr;
  VqaConfig* config = nullptr;
  VqaDrawer* drawer = nullptr;
  int32_t result = 0;

#ifdef _WIN32
  // Run at high priority while the movie plays, so the busy loop below is not
  // starved of the time slices that keep the frames on schedule. Every return
  // below restores the saved level.
  DWORD process_priority = GetPriorityClass(GetCurrentProcess());
  SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif  // _WIN32

  movie = state->movie.get();
  drawer = &movie->drawer;
  config = &state->config;

  // The first call starts playback. The sound starts first so the clock
  // below can run from it, and only if OpenVqa() preloaded some. A movie
  // whose audio ring came out empty (audio_buffer_bytes 0, or -1 when 1.5
  // seconds of sound is less than one audio_block_bytes block) plays silent.
  if ((movie->flags & kMovieStarted) == 0) {
    ConfigureDrawer(state);

    if ((config->option_flags & kVqaOptionAudio) != 0 &&
        !movie->audio.block_loaded.empty() &&
        movie->audio.block_loaded.front() != 0) {
      StartMovieAudio(state);
    }

    // Set the clock to the time of the first frame loaded, so it is due now.
    const auto first_frame_time = movie->drawer.current_frame->frame_number *
                                  kVqaTicksPerSecond / config->draw_rate;

    SetMovieClock(state, first_frame_time, config->clock_source);
    movie->start_time = ReadMovieClock(state);

    movie->flags |= kMovieStarted;
  }

  switch (mode) {
    case kVqaModePause:
      if ((movie->flags & kMoviePaused) == 0) {
        movie->flags |= kMoviePaused;
        movie->end_time = ReadMovieClock(state);

        // The clock follows the sound, so stopping it stops the clock too.
        if ((movie->audio.flags & kAudioPlaying) != 0) {
          StopMovieAudio(state);
        }
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

        // StartMovieAudio() fails only if some movie's sound is already
        // playing, which ends this one.
        if (((config->option_flags & kVqaOptionAudio) != 0) &&
            (StartMovieAudio(state) != 0)) {
          StopMovieAudio(state);
#ifdef _WIN32
            SetPriorityClass(GetCurrentProcess(), process_priority);
#endif  // _WIN32
            return kVqaEndOfMovie;
        }

        SetMovieClock(state, movie->end_time, config->clock_source);
      }

      // Load, draw, load, draw... until both are done.
      while ((movie->flags & (kMovieDrawerDone | kMovieLoaderDone)) !=
             (kMovieDrawerDone | kMovieLoaderDone)) {
        if ((movie->flags & kMovieLoaderDone) == 0) {
          result = LoadNextFrame(state);
          if (result == 0) {
            movie->loaded_frames++;
          } else {
            // A full ring or a wait on the sound is retried next pass. The
            // end of the file, or any error, ends the loading: the frames
            // already loaded still play.
            if (result != kVqaNoBuffer && result != kVqaSleeping) {
              movie->flags |= kMovieLoaderDone;
              result = 0;
            }
          }
        } else {
          vqa_movie_loaded = true;
        }

        if ((config->draw_flags & kVqaDrawNothing) == 0) {
          result = (*movie->Draw_Frame)(state);
          if (result == 0) {
            movie->drawn_frames++;
            result = movie->drawer.last_drawn_frame;
            // The frame is on screen (the frame_callback showed it), so its
            // buffer can go back to the loader.
            if (ReleaseDrawnFrame(state) != 0) {
              movie->flags |= kMovieDrawerDone | kMovieLoaderDone;
            }
          } else {
            // frame_callback asked to stop.
            if (result == kVqaEndOfMovie) {
              break;
            }
            // Nothing left to draw once nothing more will be loaded.
            if ((movie->flags & kMovieLoaderDone) != 0 &&
                result == kVqaNoBuffer) {
              movie->flags |= kMovieDrawerDone;
            }

            // Too early for the next frame: let the client present or wait
            // instead of the loop spinning.
            if (result == kVqaNotTime && config->event_handler != nullptr) {
              config->event_handler(kVqaEventSync, nullptr, 0);
            }
          }
        } else {
          // Not drawing: discard each frame as soon as it is loaded.
          movie->flags |= kMovieDrawerDone;
          drawer->current_frame->flags = 0;
          drawer->current_frame = drawer->current_frame->next;
        }

        if (mode == kVqaModeWalk) {
          break;
        }
      }
      break;
  }

  if ((movie->flags & (kMovieDrawerDone | kMovieLoaderDone)) ==
          (kMovieDrawerDone | kMovieLoaderDone) ||
      mode == kVqaModeStop) {
    // Read the clock before stopping the sound, since the clock is the
    // amount of sound played.
    movie->end_time = ReadMovieClock(state);

    result = kVqaEndOfMovie;
  }

  // Every return stops the sound, even a walk that will be called again;
  // the next call does not restart it (only a resume from pause does).
  if ((movie->audio.flags & kAudioPlaying) != 0) {
    StopMovieAudio(state);
  }

#ifdef _WIN32
  SetPriorityClass(GetCurrentProcess(), process_priority);
#endif  // _WIN32

  return result;
}

int32_t VQA_SetStop(VqaPlayerState* vqa, int32_t stop) {
  int32_t oldstop = -1;

  auto* header = &vqa->header;

  if (stop > 0 && std::cmp_greater_equal(header->frame_count, stop)) {
    oldstop = header->frame_count;
    header->frame_count = static_cast<uint16_t>(stop);
  }

  return oldstop;
}

void VQA_GetInfo(VqaPlayerState* vqa, VQAInfo* info) {
  const auto* header = &vqa->header;

  info->NumFrames = header->frame_count;
  info->image_height = header->image_height;
  info->image_width = header->image_width;
  info->image_buffer = vqa->movie->drawer.image_buffer;
}

void VQA_GetStats(const VqaPlayerState* vqa, VQAStatistics* stats) {
  VqaMovie* vqabuf = vqa->movie.get();

  stats->allocated_bytes = vqabuf->allocated_bytes;
  stats->start_time = vqabuf->start_time;
  stats->end_time = vqabuf->end_time;
  stats->FramesLoaded = vqabuf->loaded_frames;
  stats->FramesDrawn = vqabuf->drawn_frames;
  stats->FramesSkipped = vqabuf->drawer.skipped_count;
  stats->max_frame_bytes = vqabuf->loader.max_frame_bytes;
  stats->SamplesPlayed = vqabuf->audio.SamplesPlayed;
}

int64_t ReleaseDrawnFrame(const VqaPlayerState* state) {
  auto* movie = state->movie.get();

  if ((movie->flags & kMovieAwaitingRelease) != 0) {
    // Remember the last frame released, for status reporting.
    movie->flipper.LastFrameNum = movie->flipper.drawn_frame->frame_number;

    // Clearing the flags hands the buffer back to the loader.
    movie->flipper.drawn_frame->flags = 0;
    movie->flags &= ~kMovieAwaitingRelease;
  }

  return 0;
}
