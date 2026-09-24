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

#include <atomic>
#include <chrono>
#include <cstdint>
#include <span>
#include <vector>

#include "base/buffer.h"
#include "base/numeric.h"
#include "winvq/vqa32/vqa_player.h"
#include "winvq/vqa32/vqa_player_state.h"

#include <SDL_audio.h>

// The sound system serves one movie at a time: the state below is shared by
// every VqaPlayerState, and a second movie opened or started while one plays
// takes it over.

// The movie the callback plays, from StartMovieAudio() to StopMovieAudio();
// nullptr otherwise.
static VqaPlayerState* VQAP = nullptr;
// kAudioOpen and kAudioPlaying for the movie holding the sound system,
// mirrored in that movie's audio.flags.
static uint32_t AudioFlags = 0;
// The clock SetMovieClock() chose, kVqaClockAudio or kVqaClockSystem. Zero
// before the first call, which reads the system clock.
static int TimerMethod;

// Added to the chosen clock so it reads the time SetMovieClock() was given.
static int64_t TickOffset = 0;

// Set while the game window is out of focus (PauseVqaAudio()). The callback
// then plays nothing, so the audio clock, and the frames with it, wait.
static bool VQAAudioPaused = false;
// Converts the movie's sound to the device's format; created by
// OpenMovieAudio(), freed by CloseMovieAudio().
static SDL_AudioStream* SDLStream = nullptr;
// Input bytes per output byte of SDLStream, in 17.15 fixed point (32768 is
// 1.0), so ReadMovieClock() can count converted bytes as movie bytes.
static int64_t StreamConvScale = 1 << 15;

// The mixer installed in the client's callback slot. The client's SDL audio
// callback calls it on the audio thread, with the device locked, to fill
// stream with len bytes of the movie's sound in the device's format; the
// client has silenced the buffer first, so returning early plays silence.
static void VQA_Audio_Callback(uint8_t* stream, int len) {
  if (!VQAP) {
    return;
  }
  auto* audio = &VQAP->movie->audio;
  if (!(audio->flags & kAudioPlaying) || VQAAudioPaused || !SDLStream) {
    return;
  }

  auto* config = &VQAP->config;

  // Convert whole ring blocks until there is a buffer's worth of output. The
  // remainder stays in SDLStream for the next call.
  while (SDL_AudioStreamAvailable(SDLStream) < len) {
    SDL_AudioStreamPut(SDLStream,
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

      // TODO: this wraps at the ring's size in bytes, next_block above at
      // block_count whole blocks. When audio_buffer_bytes is not a multiple of
      // audio_block_bytes (a caller's choice; the -1 default always is), the
      // two disagree: play_block reaches block_count and block_loaded.at()
      // throws on the audio thread, and CopyStagedAudio() writes sound into
      // the tail past the last whole block that never plays, then waits
      // forever for a block that is never freed.
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

  SDL_AudioStreamGet(SDLStream, stream, len);
}

// Movies opened and not yet closed. Only the last close removes the stream
// and the callback.
static int OpenCount = 0;

int32_t OpenMovieAudio(VqaPlayerState* vqap) {
  VqaConfig* config = &vqap->config;
  VqaMovie* vqabuf = vqap->movie.get();
  VqaAudio* audio = &vqabuf->audio;

  audio->play_block = 0;

  if (OpenCount) {
    // Another movie's stream is about to be freed; take the callback out
    // first. The device lock waits for a call in progress to finish.
    SDL_LockAudioDevice(config->audio_device_id);
    *config->audio_callback = nullptr;
    SDL_UnlockAudioDevice(config->audio_device_id);
  }

  if (SDLStream) {
    SDL_FreeAudioStream(SDLStream);
  }

  const auto* spec = static_cast<SDL_AudioSpec*>(config->audio_spec);

  // TODO: SDL_NewAudioStream() fails on an invalid spec (a zero rate or
  // channel count) and returns nullptr, which goes unnoticed: the function
  // still returns 0, the callback then plays nothing, so the audio clock never
  // moves and PlayVqa() waits forever for the next frame. A zero spec->freq
  // also divides by zero below. It should return an error, which OpenVqa()
  // turns into kVqaErrorAudio. Neither game passes such a spec.
  SDLStream = SDL_NewAudioStream(
      audio->bits_per_sample == 16 ? AUDIO_S16 : AUDIO_S8,
      static_cast<uint8_t>(audio->channels), audio->sample_rate, spec->format,
      spec->channels, spec->freq);

  const int bytes_per_second_in =
      audio->bits_per_sample / 8 * audio->channels * audio->sample_rate;
  const int bytes_per_second_out =
      SDL_AUDIO_BITSIZE(spec->format) / 8 * spec->channels * spec->freq;

  StreamConvScale =
      (int64_t{bytes_per_second_in} * 32768) / bytes_per_second_out;

  // TODO: written without the device lock, while the audio thread may be
  // reading the slot in the client's callback - a data race. Every other
  // write to the slot takes the lock.
  *config->audio_callback = VQA_Audio_Callback;

  audio->flags |= kAudioOpen;
  AudioFlags |= kAudioOpen;

  OpenCount++;

  return 0;
}

void CloseMovieAudio(VqaPlayerState* vqap) {
  VqaAudio* audio = &vqap->movie->audio;
  VqaConfig* config = &vqap->config;

  StopMovieAudio(vqap);

  // Another open movie still uses the stream and the callback.
  OpenCount--;
  if (OpenCount) {
    return;
  }

  // The device lock waits out a callback in progress, so the stream can be
  // freed once the slot is clear.
  SDL_LockAudioDevice(config->audio_device_id);
  *config->audio_callback = nullptr;
  SDL_UnlockAudioDevice(config->audio_device_id);

  if (SDLStream) {
    SDL_FreeAudioStream(SDLStream);
    SDLStream = nullptr;
  }

  audio->flags &= ~kAudioOpen;
  AudioFlags &= ~kAudioOpen;
  AudioFlags &= ~kAudioPlaying;
}

int32_t StartMovieAudio(VqaPlayerState* vqap) {
  // TODO: VQAP is set here and cleared in StopMovieAudio() without the device
  // lock, while the callback reads it on the audio thread - a data race, as
  // are the kAudioPlaying clear in StopMovieAudio() and VQAAudioPaused in
  // PauseVqaAudio() and ResumeVqaAudio(). The writes here under the lock are
  // the pattern the rest should follow.
  VQAP = vqap;

  VqaConfig* config = &vqap->config;
  VqaAudio* audio = &vqap->movie->audio;

  if (AudioFlags & kAudioPlaying) {
    return -1;
  }

  SDL_LockAudioDevice(config->audio_device_id);
  // The clock restarts from nothing played; PlayVqa() sets it with
  // SetMovieClock() right after.
  audio->blocks_played = 0;

  audio->flags |= kAudioPlaying;
  AudioFlags |= kAudioPlaying;

  SDL_UnlockAudioDevice(config->audio_device_id);

  return 0;
}

void StopMovieAudio(const VqaPlayerState* vqap) {
  VqaAudio* audio = &vqap->movie->audio;

  if (AudioFlags & kAudioPlaying) {
    // The callback stops pulling from the ring. What it already converted
    // stays in SDLStream, and a restart after a pause plays it first: it is
    // where the sound had got to.
    audio->flags &= ~kAudioPlaying;
    AudioFlags &= ~kAudioPlaying;
  }

  VQAP = nullptr;
}

int32_t CopyStagedAudio(VqaPlayerState* vqap) {
  VqaAudio* audio = &vqap->movie->audio;
  VqaConfig* config = &vqap->config;

  if ((config->option_flags & kVqaOptionAudio) == 0 || audio->ring.empty() ||
      audio->staged_bytes == 0) {
    return 0;
  }

  // The blocks the write starts and ends in. endblock is partly filled at
  // most, so it is not marked loaded below; the next copy completes it.
  const int32_t startblock = audio->write_offset / config->audio_block_bytes;
  int32_t endblock =
      (audio->write_offset + audio->staged_bytes) / config->audio_block_bytes;

  if (endblock >= audio->block_count) {
    endblock -= audio->block_count;
  }

  // The unplayed blocks are one run starting at play_block, so if the last
  // block the write reaches is free, so is every block before it.
  if (audio->block_loaded.at(base::ToSize(endblock)) == 1) {
    return kVqaSleeping;
  }

  SDL_LockAudioDevice(config->audio_device_id);

  // The write fits before the end of the ring.
  if (startblock <= endblock) {
    base::CopyBytes(std::as_writable_bytes(
                        audio->ring.subspan(base::ToSize(audio->write_offset))),
                    std::as_bytes(std::span(audio->staging)),
                    audio->staged_bytes);

    audio->write_offset += audio->staged_bytes;
    audio->staged_bytes = 0;

    for (int32_t i = startblock; i < endblock; i++) {
      audio->block_loaded.at(base::ToSize(i)) = 1;
    }

    SDL_UnlockAudioDevice(config->audio_device_id);
    return 0;
  }
  // The write wraps: fill to the end of the ring, then continue at its start.
  const int32_t len1 = config->audio_buffer_bytes - audio->write_offset;
  const int32_t len2 = audio->staged_bytes - len1;

  base::CopyBytes(std::as_writable_bytes(
                      audio->ring.subspan(base::ToSize(audio->write_offset))),
                  std::as_bytes(std::span(audio->staging)), len1);

  base::CopyBytes(
      std::as_writable_bytes(audio->ring),
      std::as_bytes(std::span(audio->staging).subspan(base::ToSize(len1))),
      len2);

  audio->write_offset = len2;
  audio->staged_bytes = 0;

  for (int32_t i = startblock; i < audio->block_count; i++) {
    audio->block_loaded.at(base::ToSize(i)) = 1;
  }

  for (int32_t i = 0; i < endblock; i++) {
    audio->block_loaded.at(base::ToSize(i)) = 1;
  }

  SDL_UnlockAudioDevice(config->audio_device_id);
  return 0;
}

void PauseVqaAudio() {
  if ((VQAP && VQAP->movie) &&
      (AudioFlags & kAudioPlaying && !VQAAudioPaused)) {
    VQAAudioPaused = true;
  }
}

void ResumeVqaAudio() {
  if ((VQAP && VQAP->movie) && (AudioFlags & kAudioPlaying && VQAAudioPaused)) {
    // The callback picks up at play_block, and the audio clock with it.
    VQAAudioPaused = false;
  }
}

void SetMovieClock(VqaPlayerState* vqap, int64_t time, int method) {
  // The audio clock, the default, needs sound playing; everything else runs
  // on the system clock.
  const bool use_audio =
      (method == kVqaClockDefault || method == kVqaClockAudio) &&
      (AudioFlags & kAudioPlaying) != 0;
  TimerMethod = use_audio ? kVqaClockAudio : kVqaClockSystem;

  TickOffset = 0;
  const int64_t curtime = ReadMovieClock(vqap);
  TickOffset = time - curtime;
}

int64_t ReadMovieClock(VqaPlayerState* vqap) {
  VqaAudio* audio = nullptr;
  VqaConfig* config = nullptr;
  int64_t totalbytes = 0;
  int64_t samples = 0;
  int play_cursor = 0;  // Bytes queued in SDLStream but not yet played
  int64_t ticks = 0;

  switch (TimerMethod) {
    // The audio clock: the movie bytes handed to the callback's stream, less
    // those still waiting in it, as samples, as ticks.
    case kVqaClockAudio:
      audio = &vqap->movie->audio;
      config = &vqap->config;

      SDL_LockAudioDevice(vqap->config.audio_device_id);
      totalbytes = int64_t{audio->blocks_played} * config->audio_block_bytes;

      // The queue is in the device's format; StreamConvScale turns it back
      // into movie bytes. The device's own buffer is not counted, so the
      // clock runs up to one buffer ahead of what is heard.
      play_cursor = SDL_AudioStreamAvailable(SDLStream);
      totalbytes -= (play_cursor * StreamConvScale) / 32768;
      SDL_UnlockAudioDevice(vqap->config.audio_device_id);

      samples = totalbytes / audio->channels;
      samples = samples / (audio->bits_per_sample / 8);

      ticks = samples * kVqaTicksPerSecond / audio->sample_rate;
      ticks += TickOffset;
      break;

    // No sound playing, or not asked for: the system clock.
    default:
    case kVqaClockSystem: {
      // TODO: system_clock is the wall clock. An adjustment to it during a
      // movie without sound (NTP, a manual change) moves the movie clock by
      // the same amount, dropping a run of frames or freezing the movie for
      // as long as the clock went back. steady_clock never jumps.
      const auto now = std::chrono::system_clock::now();
      const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          now.time_since_epoch())
                          .count();

      ticks = ms * kVqaTicksPerSecond / 1000;
      ticks += TickOffset;
    } break;
  }

  return ticks;
}
