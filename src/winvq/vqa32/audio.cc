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

// File: the VQA player's sound output and the movie clock. The loader fills
// the audio ring (CopyStagedAudio); an SDL audio callback installed here plays
// it through an SDL_AudioStream that converts it to the device's format; and
// the clock that paces the frames is the amount of sound played, or the
// system clock when there is none.
//
// Originally written by Bill Randolph and Denzil E. Long, Jr. at Westwood
// Studios, August 1995, for HMI's DOS sound drivers; ported to DirectSound by
// Steve T. in January 1996, and later to SDL.

#include <SDL_audio.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "base/buffer.h"
#include "base/numeric.h"
#include "winvq/vqa32/vqa_player.h"
#include "winvq/vqa32/vqa_player_state.h"

// The sound system serves one movie at a time: the state below is shared by
// every VqaPlayerState, and a second movie opened or started while one plays
// takes it over. The callback reads it on the audio thread, so the main
// thread changes what the callback reads only with the device locked.

// The movie the callback plays, from StartMovieAudio() to StopMovieAudio();
// nullptr otherwise.
static VqaPlayerState* playing_state = nullptr;
// A movie's sound is playing, from StartMovieAudio() to StopMovieAudio();
// that movie's audio.flags carry kAudioPlaying too.
static bool sound_playing = false;
// The clock SetMovieClock() chose, kVqaClockAudio or kVqaClockSystem. Zero
// before the first call, which reads the system clock.
static int active_clock;

// Added to the chosen clock so it reads the time SetMovieClock() was given.
static int64_t clock_offset_ticks = 0;

// Set while the game window is out of focus (PauseVqaAudio()). The callback
// then plays nothing, so the audio clock, and the frames with it, wait.
static bool sound_paused = false;
// Converts the movie's sound to the device's format; created by
// OpenMovieAudio(), freed by CloseMovieAudio().
static SDL_AudioStream* sound_converter = nullptr;
// Input bytes per output byte of sound_converter, in 17.15 fixed point (32768
// is 1.0), so ReadMovieClock() can count converted bytes as movie bytes.
static int64_t converter_byte_ratio = 1 << 15;

// The mixer installed in the client's callback slot. The client's SDL audio
// callback calls it on the audio thread, with the device locked, to fill
// device_buffer with the movie's sound in the device's format; the client has
// silenced the buffer first, so returning early plays silence.
static void MixMovieSound(const std::span<std::byte> device_buffer) {
  if (!playing_state) {
    return;
  }
  auto* audio = &playing_state->movie->audio;
  if (!(audio->flags & kAudioPlaying) || sound_paused || !sound_converter) {
    return;
  }

  auto* config = &playing_state->config;
  const int device_bytes = static_cast<int>(device_buffer.size());

  // Convert whole ring blocks until there is a buffer's worth of output. The
  // remainder stays in sound_converter for the next call.
  while (SDL_AudioStreamAvailable(sound_converter) < device_bytes) {
    SDL_AudioStreamPut(sound_converter,
                       audio->ring
                           .subspan(base::ToSize(audio->play_offset),
                                    base::ToSize(config->audio_block_bytes))
                           .data(),
                       config->audio_block_bytes);

    int32_t next_block = audio->play_block + 1;
    if (next_block >= audio->block_count) {
      next_block = 0;
    }

    // Move on only once the loader has filled the next block, freeing this
    // one for it. Otherwise play this block again: the loader has fallen
    // behind, and a repeat is less jarring than a gap.
    if (audio->block_loaded.at(base::ToSize(next_block)) == 1) {
      audio->block_loaded.at(base::ToSize(audio->play_block)) = 0;

      audio->play_offset += config->audio_block_bytes;
      audio->play_block++;

      // The ring is whole blocks, so this wraps together with next_block.
      if (audio->play_offset >= config->audio_buffer_bytes) {
        audio->play_offset = 0;
        audio->play_block = 0;
      }
      audio->blocks_played++;
    } else {
      // A repeat advances the clock only once the whole movie is loaded. Until
      // then the frames wait for the loader to catch up; after, the sound has
      // simply run out, and the last frames must still come due.
      if (vqa_movie_loaded.load(std::memory_order_relaxed)) {
        audio->blocks_played++;
      }
      // Let the drawer skip frames to catch up, so it does not happen again.
      config->draw_flags &= ~kVqaDrawNoSkip;
    }
  }

  SDL_AudioStreamGet(sound_converter, device_buffer.data(), device_bytes);
}

// Movies opened and not yet closed. Only the last close removes the stream
// and the callback.
static int open_movie_count = 0;

int32_t OpenMovieAudio(VqaPlayerState* state) {
  VqaConfig* config = &state->config;
  VqaAudio* audio = &state->movie->audio;

  if (open_movie_count) {
    // Another movie's stream is about to be freed; take the callback out
    // first. The device lock waits for a call in progress to finish.
    SDL_LockAudioDevice(config->audio_device_id);
    *config->audio_callback = nullptr;
    SDL_UnlockAudioDevice(config->audio_device_id);
  }

  if (sound_converter) {
    SDL_FreeAudioStream(sound_converter);
  }

  const SDL_AudioSpec* spec = config->audio_spec;

  sound_converter = SDL_NewAudioStream(
      audio->bits_per_sample == 16 ? AUDIO_S16 : AUDIO_S8,
      static_cast<uint8_t>(audio->channels), audio->sample_rate, spec->format,
      spec->channels, spec->freq);
  // SDL rejects a spec it cannot convert to, such as an unknown format.
  // Without a stream the callback would play nothing and the audio clock never
  // move, so the movie would wait forever; the format's zero sample size would
  // also divide by zero below.
  if (sound_converter == nullptr) {
    return -1;
  }

  const int bytes_per_second_in =
      audio->bits_per_sample / 8 * audio->channels * audio->sample_rate;
  const int bytes_per_second_out =
      SDL_AUDIO_BITSIZE(spec->format) / 8 * spec->channels * spec->freq;

  converter_byte_ratio =
      (int64_t{bytes_per_second_in} * 32768) / bytes_per_second_out;

  SDL_LockAudioDevice(config->audio_device_id);
  *config->audio_callback = MixMovieSound;
  SDL_UnlockAudioDevice(config->audio_device_id);

  audio->flags |= kAudioOpen;

  open_movie_count++;

  return 0;
}

void CloseMovieAudio(VqaPlayerState* state) {
  VqaAudio* audio = &state->movie->audio;
  VqaConfig* config = &state->config;

  StopMovieAudio(state);

  // Another open movie still uses the stream and the callback.
  open_movie_count--;
  if (open_movie_count) {
    return;
  }

  // The device lock waits out a callback in progress, so the stream can be
  // freed once the slot is clear.
  SDL_LockAudioDevice(config->audio_device_id);
  *config->audio_callback = nullptr;
  SDL_UnlockAudioDevice(config->audio_device_id);

  if (sound_converter) {
    SDL_FreeAudioStream(sound_converter);
    sound_converter = nullptr;
  }

  audio->flags &= ~kAudioOpen;
}

int32_t StartMovieAudio(VqaPlayerState* state) {
  VqaConfig* config = &state->config;
  VqaAudio* audio = &state->movie->audio;

  SDL_LockAudioDevice(config->audio_device_id);
  playing_state = state;

  if (sound_playing) {
    SDL_UnlockAudioDevice(config->audio_device_id);
    return -1;
  }

  // The clock restarts from nothing played; PlayVqa() sets it with
  // SetMovieClock() right after.
  audio->blocks_played = 0;

  audio->flags |= kAudioPlaying;
  sound_playing = true;

  SDL_UnlockAudioDevice(config->audio_device_id);

  return 0;
}

void StopMovieAudio(const VqaPlayerState* state) {
  VqaAudio* audio = &state->movie->audio;

  SDL_LockAudioDevice(state->config.audio_device_id);
  if (sound_playing) {
    // The callback stops pulling from the ring. What it already converted
    // stays in sound_converter, and a restart after a pause plays it first: it
    // is where the sound had got to.
    audio->flags &= ~kAudioPlaying;
    sound_playing = false;
  }

  playing_state = nullptr;
  SDL_UnlockAudioDevice(state->config.audio_device_id);
}

int32_t CopyStagedAudio(VqaPlayerState* state) {
  VqaAudio* audio = &state->movie->audio;
  VqaConfig* config = &state->config;

  if ((config->option_flags & kVqaOptionAudio) == 0 || audio->ring.empty() ||
      audio->staged_bytes == 0) {
    return 0;
  }

  // The blocks the write starts and ends in. end_block is partly filled at
  // most, so it is not marked loaded below; the next copy completes it.
  const int32_t start_block = audio->write_offset / config->audio_block_bytes;
  const int32_t end_block = (audio->write_offset + audio->staged_bytes) /
                            config->audio_block_bytes % audio->block_count;

  SDL_LockAudioDevice(config->audio_device_id);

  // The unplayed blocks are one run starting at play_block, so if the last
  // block the write reaches is free, so is every block before it.
  if (audio->block_loaded.at(base::ToSize(end_block)) == 1) {
    SDL_UnlockAudioDevice(config->audio_device_id);
    return kVqaSleeping;
  }

  // Fill towards the end of the ring, and continue at its start with what
  // does not fit.
  const auto staging = std::as_bytes(std::span(audio->staging));
  const int32_t tail_bytes = std::min(
      audio->staged_bytes, config->audio_buffer_bytes - audio->write_offset);
  const int32_t head_bytes = audio->staged_bytes - tail_bytes;
  base::CopyBytes(std::as_writable_bytes(
                      audio->ring.subspan(base::ToSize(audio->write_offset))),
                  staging, tail_bytes);
  base::CopyBytes(std::as_writable_bytes(audio->ring),
                  staging.subspan(base::ToSize(tail_bytes)), head_bytes);

  audio->write_offset =
      (audio->write_offset + audio->staged_bytes) % config->audio_buffer_bytes;
  audio->staged_bytes = 0;

  for (int32_t block = start_block; block != end_block;
       block = (block + 1) % audio->block_count) {
    audio->block_loaded.at(base::ToSize(block)) = 1;
  }

  SDL_UnlockAudioDevice(config->audio_device_id);
  return 0;
}

void PauseVqaAudio() {
  if (sound_playing && playing_state != nullptr) {
    SDL_LockAudioDevice(playing_state->config.audio_device_id);
    sound_paused = true;
    SDL_UnlockAudioDevice(playing_state->config.audio_device_id);
  }
}

void ResumeVqaAudio() {
  if (sound_playing && playing_state != nullptr) {
    // The callback picks up at play_block, and the audio clock with it.
    SDL_LockAudioDevice(playing_state->config.audio_device_id);
    sound_paused = false;
    SDL_UnlockAudioDevice(playing_state->config.audio_device_id);
  }
}

void SetMovieClock(const VqaPlayerState* state, int64_t now_ticks,
                   int clock_source) {
  // The audio clock, the default, needs sound playing; everything else runs
  // on the system clock.
  const bool use_audio =
      (clock_source == kVqaClockDefault || clock_source == kVqaClockAudio) &&
      sound_playing;
  active_clock = use_audio ? kVqaClockAudio : kVqaClockSystem;

  clock_offset_ticks = 0;
  const int64_t clock_ticks = ReadMovieClock(state);
  clock_offset_ticks = now_ticks - clock_ticks;
}

int64_t ReadMovieClock(const VqaPlayerState* state) {
  // No sound playing, or not asked for: the system clock.
  if (active_clock != kVqaClockAudio) {
    // steady_clock, not the wall clock: an adjustment to that mid-movie would
    // drop a run of frames, or freeze the movie while it caught up.
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now().time_since_epoch())
                        .count();
    return (ms * kVqaTicksPerSecond / 1000) + clock_offset_ticks;
  }

  // The audio clock: the movie bytes handed to the callback's stream, less
  // those still waiting in it, as samples, as ticks.
  const VqaAudio& audio = state->movie->audio;
  const VqaConfig& config = state->config;

  SDL_LockAudioDevice(config.audio_device_id);
  int64_t played_bytes =
      int64_t{audio.blocks_played} * config.audio_block_bytes;
  // The queue is in the device's format; converter_byte_ratio turns it back
  // into movie bytes. The device's own buffer is not counted, so the clock
  // runs up to one buffer ahead of what is heard.
  const int queued_bytes = SDL_AudioStreamAvailable(sound_converter);
  played_bytes -= (queued_bytes * converter_byte_ratio) / 32768;
  SDL_UnlockAudioDevice(config.audio_device_id);

  const int64_t played_samples =
      played_bytes / audio.channels / (audio.bits_per_sample / 8);
  return (played_samples * kVqaTicksPerSecond / audio.sample_rate) +
         clock_offset_ticks;
}
