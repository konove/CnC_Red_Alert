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

#include "winvq/vqa32/vqa_player.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string_view>

#ifdef _WIN32
#include <windows.h>
#endif

#include "winvq/vqa32/vqa_player_state.h"
#include "winvq/vqa32/vqaio.h"

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

std::atomic<bool> vqa_movie_loaded = false;

// Each pass of the loop gives the loader one frame to load and the drawer one
// frame to draw; either may decline (no free buffer, not yet time) and the
// loop comes round again, so a RUN spins until the movie is done. The loader
// runs ahead by up to frame_buffer_count frames, which is what absorbs a slow
// read.
int32_t PlayVqa(VqaPlayerState* state, int32_t mode) {
  VqaMovie* const movie = state->movie.get();
  VqaDrawer* const drawer = &movie->drawer;
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
    ConfigureDrawer(state);

    if ((config->option_flags & kVqaOptionAudio) != 0 &&
        movie->audio.block_loaded.at(0) != 0) {
      StartMovieAudio(state);
    }

    // Set the clock to the time of the first frame loaded, so it is due now.
    const auto first_frame_time = drawer->current_frame->frame_number *
                                  kVqaTicksPerSecond / config->draw_rate;

    SetMovieClock(state, first_frame_time, config->clock_source);

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
      while ((movie->flags & kMovieDone) != kMovieDone) {
        if ((movie->flags & kMovieLoaderDone) == 0) {
          result = LoadNextFrame(state);
          // A full ring or a wait on the sound is retried next pass. The end
          // of the file, or any error, ends the loading: the frames already
          // loaded still play.
          if (result != 0 && result != kVqaNoBuffer && result != kVqaSleeping) {
            movie->flags |= kMovieLoaderDone;
            // The flag carries no other data to the audio thread.
            vqa_movie_loaded.store(true, std::memory_order_relaxed);
            result = 0;
          }
        }

        if ((config->draw_flags & kVqaDrawNothing) == 0) {
          result = DrawNextFrame(state);
          if (result == 0) {
            result = drawer->last_drawn_frame;
            // The frame is on screen (the frame_callback showed it), so its
            // buffer can go back to the loader.
            ReleaseDrawnFrame(state);
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

  // Shut down when the movie has played out, was stopped, or its
  // frame_callback asked to stop (the only source of kVqaEndOfMovie above).
  // Only then does the sound stop: a walk returns with it still playing, since
  // the next walk does not restart it and the clock runs from it.
  if ((movie->flags & kMovieDone) == kMovieDone || mode == kVqaModeStop ||
      result == kVqaEndOfMovie) {
    // Read the clock before stopping the sound, since the clock is the
    // amount of sound played.
    movie->end_time = ReadMovieClock(state);
    if ((movie->audio.flags & kAudioPlaying) != 0) {
      StopMovieAudio(state);
    }

    result = kVqaEndOfMovie;
  }

#ifdef _WIN32
  SetPriorityClass(GetCurrentProcess(), process_priority);
#endif  // _WIN32

  return result;
}

void ReleaseDrawnFrame(const VqaPlayerState* state) {
  auto* movie = state->movie.get();

  if ((movie->flags & kMovieAwaitingRelease) != 0) {
    // Clearing the flags hands the buffer back to the loader.
    movie->flipper.drawn_frame->flags = 0;
    movie->flags &= ~kMovieAwaitingRelease;
  }
}
