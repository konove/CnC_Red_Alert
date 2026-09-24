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

#ifndef CNC_RED_ALERT_WINVQ_VQA32_VQA_PLAYER_STATE_H_
#define CNC_RED_ALERT_WINVQ_VQA32_VQA_PLAYER_STATE_H_

// File: the VQA player's internal state, VqaPlayerState, and the functions the
// player's source files share. Only vqa32 and its tests include it.
//
// The player is two cooperating parts that take turns in PlayVqa()'s loop on
// one thread. The loader reads frames and their codebooks from the file into
// a FrameRing; the drawer decodes a loaded frame into the image buffer when
// the clock says it is due, shows it and hands its buffer back to the loader.
// The sound runs separately, on the SDL audio thread, from a ring the loader
// fills.
//
// Originally written by Denzil E. Long, Jr. and Bill Randolph at Westwood
// Studios, August 1995.

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "base/numeric.h"
#include "base/types.h"
#include "winvq/vqa32/audio_output.h"
#include "winvq/vqa32/audio_ring.h"
#include "winvq/vqa32/chunk_reader.h"
#include "winvq/vqa32/frame_ring.h"
#include "winvq/vqa32/lcw_buffer.h"
#include "winvq/vqa32/movie_clock.h"
#include "winvq/vqa32/movie_drawer.h"
#include "winvq/vqa32/movie_loader.h"
#include "winvq/vqa32/vqa_format.h"
#include "winvq/vqa32/vqa_player.h"
#include "winvq/vqa32/vqaio.h"

// VqaMovie: everything a movie needs while it is open. Allocated by OpenVqa()
// once the header is read and freed by CloseVqa().
struct VqaMovie {
  explicit VqaMovie(FrameRing frame_ring) : ring(std::move(frame_ring)) {}

  // The loaded frames and their codebooks.
  FrameRing ring;

  // The sound on its way to the device, and what plays it; both empty when
  // the movie plays without sound. Declared in this order so the output,
  // whose mixer reads the ring, goes first.
  std::unique_ptr<AudioRing> audio;
  std::unique_ptr<AudioOutput> audio_output;
  // Paces the frames.
  MovieClock clock;
  // Reads the frames into ring; made once the sound is set up.
  std::unique_ptr<MovieLoader> loader;
  // Draws the frames from ring; made with the loader.
  std::unique_ptr<MovieDrawer> drawer;
  uint32_t flags = 0;        // kMovie* bits
  // The clock reading (kVqaTicksPerSecond) when playback ended or was last
  // paused, where a resumed movie restarts the clock.
  int64_t end_time = 0;
};

// VqaMovie flags.
// The drawer and the loader have finished; kMovieDone is both.
constexpr uint32_t kMovieDrawerDone = base::Bit<uint32_t>(3);
constexpr uint32_t kMovieLoaderDone = base::Bit<uint32_t>(4);
constexpr uint32_t kMovieDone = kMovieDrawerDone | kMovieLoaderDone;
// PlayVqa() has started the clock and the sound.
constexpr uint32_t kMovieStarted = base::Bit<uint32_t>(5);
constexpr uint32_t kMoviePaused = base::Bit<uint32_t>(6);

// VqaPlayerState: the player state behind VqaPlayer, which owns one; tests use
// one directly to reach the internals. It outlives the movies opened on it.
struct VqaPlayerState {
  VqaPlayerState() = default;
  ~VqaPlayerState() = default;
  VqaPlayerState(const VqaPlayerState&) = delete;
  VqaPlayerState& operator=(const VqaPlayerState&) = delete;
  VqaPlayerState(VqaPlayerState&&) = delete;
  VqaPlayerState& operator=(VqaPlayerState&&) = delete;

  // Clears playback state so the player can open another movie. io is
  // intentionally preserved across reset.
  void Reset() {
    movie.reset();
    config = {};
    header = {};
  }

  // The file source installed by VqaPlayer::SetIo(). Not owned.
  VqaIo* io = nullptr;
  // The open movie's buffers: allocated by OpenVqa() and released by
  // CloseVqa(). nullptr while no movie is open.
  std::unique_ptr<VqaMovie> movie;
  // The copy of the caller's configuration, with the -1 defaults resolved
  // from the header.
  VqaConfig config{};
  // The open movie's VQHD header.
  VqaHeader header{};
};

// The player entry points behind VqaPlayer's Open(), Close() and Play(); see
// vqa_player.h for what they do. OpenVqa() and CloseVqa() are
// also the allocation and release of state->movie.
int32_t OpenVqa(VqaPlayerState* state, std::string_view filename,
                VqaConfig* config);
void CloseVqa(VqaPlayerState* state);
int32_t PlayVqa(VqaPlayerState* state, int32_t mode);

#endif  // CNC_RED_ALERT_WINVQ_VQA32_VQA_PLAYER_STATE_H_
