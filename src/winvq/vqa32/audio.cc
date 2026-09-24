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

/****************************************************************************
 *
 *        C O N F I D E N T I A L -- W E S T W O O D  S T U D I O S
 *
 *----------------------------------------------------------------------------
 *
 * PROJECT
 *     VQA player library. (32-Bit protected mode)
 *
 * FILE
 *     audio.c
 *
 * DESCRIPTION
 *     Audio playback and timing.
 *
 * PROGRAMMER
 *     Bill Randolph
 *     Denzil E. Long, Jr.
 *
 * DATE
 *     August 4, 1995
 *
 *
 * HISTORY:
 *     Modified for Win95 Direct Sound - Steve T 1/2/96 6:35AM
 *
 *----------------------------------------------------------------------------
 *
 * PUBLIC
 *     VQA_StartTimerInt - Initialize system timer interrupt.
 *     VQA_StopTimerInt  - Remove system timer interrupt.
 *     SetMovieClock      - Resets current time to given tick value.
 *     ReadMovieClock       - Return current time.
 *     VQA_TimerMethod   - Get timer method being used.
 *     OpenMovieAudio     - Open sound system.
 *     CloseMovieAudio    - Close sound system
 *     StartMovieAudio    - Starts audio playback
 *     StopMovieAudio     - Stop audio playback.
 *     CopyStagedAudio         - Copy data from Audio Temp buf into Audio play
 * buf.
 *
 * PRIVATE
 *     TimerCallback - VQA timer event. (Called by HMI)
 *     AutoDetect    - Auto detect the sound card.
 *     audio_callback - Sound system callback.
 *
 ****************************************************************************/

#include <vector>

#include <chrono>
#include <cstdint>
#include <span>

#include "base/buffer.h"
#include "base/numeric.h"
#include "winvq/vqa32/vqaplay.h"
#include "winvq/vqa32/vqaplayp.h"

/*---------------------------------------------------------------------------
 * PROTOTYPES
 *-------------------------------------------------------------------------*/


#include <SDL_audio.h>

/*---------------------------------------------------------------------------
 * GLOBAL DATA
 *-------------------------------------------------------------------------*/

static VqaPlayerState* VQAP = nullptr;
static uint32_t AudioFlags = 0;  // VQAAUDF_* bits
static int32_t TimerIntCount = 0;
static uint16_t VQATimer = 0;
static int TimerMethod;
static int64_t VQATickCount = 0;

static int64_t TickOffset = 0;

static bool VQAAudioPaused = false;
static SDL_AudioStream* SDLStream = nullptr;
// Input bytes per output byte of SDLStream, in 17.15 fixed point.
static int64_t StreamConvScale = 1 << 15;

static void VQA_Audio_Callback(uint8_t* stream, int len) {
  // called from SDL audio callback
  if (!VQAP) {
    return;
  }
  auto* audio = &VQAP->movie->audio;
  if (!(audio->flags & kAudioPlaying) || VQAAudioPaused || !SDLStream) {
    return;
  }

  auto* config = &VQAP->config;

  while (SDL_AudioStreamAvailable(SDLStream) < len) {
    SDL_AudioStreamPut(SDLStream,
                       audio->ring
                           .subspan(base::ToSize(audio->play_offset),
                                    base::ToSize(config->audio_block_bytes))
                           .data(),
                       config->audio_block_bytes);

    /* Compute the 'next_block' index */
    audio->next_block = audio->play_block + 1;

    if (audio->next_block >= audio->block_count) {
      audio->next_block = 0;
    }

    /* See if the next block has data in it; if so, update the audio
     * buffer play position & the 'play_block' value.
     * If not, don't change anything and replay this block.
     */
    if (audio->block_loaded.at(base::ToSize(audio->next_block)) == 1) {
      /* Update this block's status to loadable (0) */
      audio->block_loaded.at(base::ToSize(audio->play_block)) = 0;

      /* Update position within audio buffer */
      audio->play_offset += config->audio_block_bytes;
      audio->play_block++;

      if (audio->play_offset >= config->audio_buffer_bytes) {
        audio->play_offset = 0;
        audio->play_block = 0;
      }
      audio->blocks_played++;
    } else {
      if (vqa_movie_loaded) {
        audio->blocks_played++;
      }
      /*
      ** Enable frame skipping to prevent this happening again
      */
      config->draw_flags &= ~kVqaDrawNoSkip;
    }
  }

  // output stream
  SDL_AudioStreamGet(SDLStream, stream, len);
}

/****************************************************************************
 *
 * NAME
 *     VQA_StartTimerInt - Initialize system timer interrupt.
 *
 * SYNOPSIS
 *     Error = VQA_StartTimerInt(VQA, Init)
 *
 *     long VQA_StartTimerInt(VQAHandeP *, long);
 *
 * FUNCTION
 *     Initialize the HMI timer system and add our own timer event. If the
 *     system has already been initialized then we are given access to the
 *     the timer system.
 *
 * INPUTS
 *     VQA  - Pointer to private VqaPlayerState structure.
 *     Init - Initialize HMI timer system flag. (TRUE = Initialize)
 *
 * RESULT
 *     Error - 0 if successful, -1 if error.
 *
 ****************************************************************************/

int32_t VQA_StartTimerInt(const VqaPlayerState* vqap, int32_t /*init*/) {
  /* Dereference for quick access. */
  VqaAudio* audio = &vqap->movie->audio;

  /* Register the VQA_TickCount timer event. */
  if ((AudioFlags & VQAAUDF_HMITIMER) == HMI_UNINIT << VQAAUDB_HMITIMER) {
    // TODO: add timer (VQATimer)

    if (VQATimer) {
      /* Flag the timer interrupt as being registered. */
      AudioFlags |= HMI_VQAINIT << VQAAUDB_HMITIMER;
    } else {
      return -1;
    }
  }

  /* Flag availability of the timer interrupt. */
  audio->flags |= HMI_VQAINIT << VQAAUDB_HMITIMER;

  /* Increment the timer interrupt usage count. */
  TimerIntCount++;

  return 0;
}

/****************************************************************************
 *
 * NAME
 *     VQA_StopTimerInt - Remove system timer interrupt.
 *
 * SYNOPSIS
 *     VQA_StopTimerInt()
 *
 *     void VQA_StopTimerInt();
 *
 * FUNCTION
 *     Remove our timer event from the HMI timer system. Uninitialize the
 *     HMI timer system if we initialized it.
 *
 * INPUTS
 *     NONE
 *
 * RESULT
 *     NONE
 *
 ****************************************************************************/

void VQA_StopTimerInt(VqaPlayerState* /*vqap*/) {
  /* Decrement the timer interrupt usage count. */
  if (TimerIntCount) {
    TimerIntCount--;
  }

  /* Remove the timer interrupt if it is initialized and the use count is
   * zero. The Windows player clears the shared availability flag either way
   * (the DOS player cleared only the caller's).
   */
  // TODO: remove timer
  AudioFlags &= ~VQAAUDF_HMITIMER;
}

/****************************************************************************
 *
 * NAME
 *     OpenMovieAudio - Open sound system.
 *
 * SYNOPSIS
 *     Error = OpenMovieAudio(VqaPlayerState)
 *
 *     long OpenMovieAudio(VqaPlayerState *);
 *
 * FUNCTION
 *     Initialise the sound system. Create a direct sound object and the
 *     direct sound primary sound buffer if they dont already exist.
 *
 * INPUTS
 *     VqaPlayerState - Pointer to private VqaPlayerState.
 *
 * RESULT
 *     Error - 0 if successful, -1 if error.
 *
 ****************************************************************************/

static int OpenCount = 0;

int32_t OpenMovieAudio(VqaPlayerState* vqap) {
  /* Dereference data memebers for quicker access. */
  VqaConfig* config = &vqap->config;
  VqaMovie* vqabuf = vqap->movie.get();
  VqaAudio* audio = &vqabuf->audio;

  /* Reset the buffer position to the beginning. */
  audio->play_block = 0;

  if (OpenCount) {
    // if we've already initialised make sure we're not in the callback
    // (by unsetting it)
    SDL_LockAudioDevice(config->audio_device_id);
    *config->audio_callback = nullptr;
    SDL_UnlockAudioDevice(config->audio_device_id);
  }

  // setup audio stream
  if (SDLStream) {
    SDL_FreeAudioStream(SDLStream);
  }

  const auto* spec = static_cast<SDL_AudioSpec*>(config->audio_spec);

  SDLStream = SDL_NewAudioStream(
      audio->bits_per_sample == 16 ? AUDIO_S16 : AUDIO_S8,
      static_cast<uint8_t>(audio->channels), audio->sample_rate, spec->format,
      spec->channels, spec->freq);

  // calculate scaling factor
  const int bytes_per_second_in =
      audio->bits_per_sample / 8 * audio->channels * audio->sample_rate;
  const int bytes_per_second_out =
      SDL_AUDIO_BITSIZE(spec->format) / 8 * spec->channels * spec->freq;

  StreamConvScale =
      (int64_t{bytes_per_second_in} * 32768) / bytes_per_second_out;

  // register our audio callback
  *config->audio_callback = VQA_Audio_Callback;

  audio->flags |= HMI_VQAINIT << VQAAUDB_DIGIINIT;
  AudioFlags |= HMI_VQAINIT << VQAAUDB_DIGIINIT;

  OpenCount++;

  return 0;
}

/****************************************************************************
 *
 * NAME
 *     CloseMovieAudio - Close sound system
 *
 * SYNOPSIS
 *     CloseMovieAudio()
 *
 *     void CloseMovieAudio();
 *
 * FUNCTION
 *     Removes VQA's involvement in the audio system.
 *
 * INPUTS
 *     NONE
 *
 * RESULT
 *     NONE
 *
 ****************************************************************************/

void CloseMovieAudio(VqaPlayerState* vqap) {
  /* Dereference for quick access. */
  VqaAudio* audio = &vqap->movie->audio;
  VqaConfig* config = &vqap->config;

  /*
  ** If the audio is still playing then stop it
  */
  StopMovieAudio(vqap);

  audio->flags &= ~VQAAUDF_TIMERINIT;
  AudioFlags &= ~VQAAUDF_TIMERINIT;

  // don't remove the callback if open was called multiple times
  OpenCount--;
  if (OpenCount) {
    return;
  }

  // unregister our audio callback
  // and make sure we're not in it
  SDL_LockAudioDevice(config->audio_device_id);
  *config->audio_callback = nullptr;
  SDL_UnlockAudioDevice(config->audio_device_id);

  if (SDLStream) {
    SDL_FreeAudioStream(SDLStream);
    SDLStream = nullptr;
  }

  audio->flags &= ~VQAAUDF_DIGIINIT;
  AudioFlags &= ~VQAAUDF_DIGIINIT;
  AudioFlags &= ~kAudioPlaying;
}

/****************************************************************************
 *
 * NAME
 *     StartMovieAudio - Starts audio playback
 *
 * SYNOPSIS
 *     Error = StartMovieAudio(VQA)
 *
 *     long StartMovieAudio(VqaPlayerState *);
 *
 * FUNCTION
 *     Start the audio playback for the movie.
 *
 * INPUTS
 *     VQA - Pointer to private VQA handle.
 *
 * RESULT
 *     Error - 0 if successful, or -1 error code.
 *
 ****************************************************************************/

int32_t StartMovieAudio(VqaPlayerState* vqap) {
  /* Save buffers for the callback routine */
  VQAP = vqap;

  /* Dereference commonly used data members for quicker access. */
  VqaConfig* config = &vqap->config;
  VqaAudio* audio = &vqap->movie->audio;

  /* Return if already playing */
  if (AudioFlags & kAudioPlaying) {
    return -1;
  }

  SDL_LockAudioDevice(config->audio_device_id);
  // setup playback
  audio->blocks_played = 0;

  audio->flags |= kAudioPlaying;
  AudioFlags |= kAudioPlaying;

  SDL_UnlockAudioDevice(config->audio_device_id);

  return 0;
}

/****************************************************************************
 *
 * NAME
 *     StopMovieAudio - Stop audio playback.
 *
 * SYNOPSIS
 *     StopMovieAudio(VQA)
 *
 *     void StopMovieAudio(VqaPlayerState *);
 *
 * FUNCTION
 *     Halts the currently playing audio stream.
 *
 * INPUTS
 *     VQA - Pointer to private VqaPlayerState.
 *
 * RESULT
 *     NONE
 *
 ****************************************************************************/

void StopMovieAudio(const VqaPlayerState* vqap) {
  /* Dereference commonly used data members for quicker access. */
  VqaAudio* audio = &vqap->movie->audio;

  /* Just return if not playing */
  if (AudioFlags & kAudioPlaying) {
    // audio->TimerHandle = nullptr;

    // TODO: stop buffer

    audio->flags &= ~kAudioPlaying;
    AudioFlags &= ~kAudioPlaying;
  }

  VQAP = nullptr;
}

/****************************************************************************
 *
 * NAME
 *     CopyStagedAudio - Copy data from Audio Temp buffer into Audio play
 * buffer.
 *
 * SYNOPSIS
 *     Error = CopyStagedAudio(VQA)
 *
 *     long CopyStagedAudio(VqaPlayerState *);
 *
 * FUNCTION
 *     This routine just copies the data in the TempBuf into the correct
 *     spots in the audio play buffer.  If there is no room available in the
 *     audio play buffer, the routine returns kVqaSleeping, which will put
 *     the whole Loader to "sleep" while it waits for a free buffer.
 *
 *     If there's no data in the TempBuf to copy, the routine just returns 0.
 *
 * INPUTS
 *     VQA - Pointer to private VqaPlayerState structure.
 *
 * RESULT
 *     Error - 0 if successful or VQAERR_??? error code.
 *
 ****************************************************************************/

int32_t CopyStagedAudio(VqaPlayerState* vqap) {
  /* Dereference commonly used data members for quicker access. */
  VqaAudio* audio = &vqap->movie->audio;
  VqaConfig* config = &vqap->config;

  /* If audio is disabled, or if we're playing from a VOC file, or if
   * there's no Audio Buffer, or if there's no data to copy, just return 0
   */
  if ((config->option_flags & kVqaOptionAudio) == 0 || audio->ring.empty() ||
      audio->staged_bytes == 0) {
    return 0;
  }

  /* Compute start & end blocks to copy into */
  const int32_t startblock = audio->write_offset / config->audio_block_bytes;
  int32_t endblock =
      (audio->write_offset + audio->staged_bytes) / config->audio_block_bytes;

  if (endblock >= audio->block_count) {
    endblock -= audio->block_count;
  }

  /* If 'endblock' hasn't played yet, return kVqaSleeping */
  if (audio->block_loaded.at(base::ToSize(endblock)) == 1) {
    return kVqaSleeping;
  }

  SDL_LockAudioDevice(config->audio_device_id);

  /* Copy the data:
   *
   *  - If 'startblock' < 'endblock', copy the entire buffer
   *  - Otherwise, fill to the end of the buffer with part of the data, then
   *    copy the rest to the beginning of the buffer
   */
  if (startblock <= endblock) {
    /* Copy data */
    base::CopyBytes(std::as_writable_bytes(
                        audio->ring.subspan(base::ToSize(audio->write_offset))),
                    std::as_bytes(std::span(audio->staging)),
                    audio->staged_bytes);

    /* Adjust current load position */
    audio->write_offset += audio->staged_bytes;

    /* Mark buffer as empty */
    audio->staged_bytes = 0;

    /* Set all blocks to loaded */
    for (int32_t i = startblock; i < endblock; i++) {
      audio->block_loaded.at(base::ToSize(i)) = 1;
    }

    SDL_UnlockAudioDevice(config->audio_device_id);
    return 0;
  }
  /* Compute length of each piece */
  const int32_t len1 = config->audio_buffer_bytes - audio->write_offset;
  const int32_t len2 = audio->staged_bytes - len1;

  /* Copy 1st piece into end of Audio Buffer */
  base::CopyBytes(std::as_writable_bytes(
                      audio->ring.subspan(base::ToSize(audio->write_offset))),
                  std::as_bytes(std::span(audio->staging)), len1);

  /* Copy 2nd piece into start of Audio Buffer */
  base::CopyBytes(
      std::as_writable_bytes(audio->ring),
      std::as_bytes(std::span(audio->staging).subspan(base::ToSize(len1))),
      len2);

  /* Adjust load position */
  audio->write_offset = len2;

  /* Mark buffer as empty */
  audio->staged_bytes = 0;

  /* Set blocks to loaded */
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
    // TODO: resume
    VQAAudioPaused = false;
  }
}

/****************************************************************************
 *
 * NAME
 *     SetMovieClock - Resets current time to given tick value.
 *
 * SYNOPSIS
 *     SetMovieClock(Time, Method)
 *
 *     void SetMovieClock(int64_t, int);
 *
 * FUNCTION
 *     Sets 'TickOffset' to a value that will make the current time look like
 *     the time passed in. This function allows the player to be "paused",
 *     by recording the time of the pause, and then setting the timer to
 *     that time. The timer method used by the player is also set. The method
 *     selected is not neccesarily the method that will be used because some
 *     timer methods work with only certain playback conditions. (EX: The
 *     audio DMA timer method cannot be used if there is not any audio
 *     playing.)
 *
 * INPUTS
 *     Time   - Value to set current time to.
 *     Method - Timer method to use.
 *
 * RESULT
 *     NONE
 *
 ****************************************************************************/

void SetMovieClock(VqaPlayerState* vqap, int64_t time, int method) {
  /* If the client does not have a preferencee then pick a method
   * based on the state of the player.
   */
  if (method == kVqaClockDefault) {
    /* If we are playing audio, use the audio DMA position. */
    if (AudioFlags & kAudioPlaying) {
      method = kVqaClockAudio;
    }

    /* Otherwise use the HMI timer if it is initialized. */
    else if (AudioFlags & VQAAUDF_HMITIMER) {
      method = kVqaClockInterrupt;
    }

    /* If all else fails resort the the "jerky" DOS time. */
    else {
      method = kVqaClockSystem;
    }
  } else {
    /* We cannot use the DMA position if there isn't any audio playing. */
    if (!(AudioFlags & kAudioPlaying) && method == kVqaClockAudio) {
      method = kVqaClockInterrupt;
    }

    /* We cannot use the timer if it has not been initialized. */
    if (!(AudioFlags & VQAAUDF_HMITIMER) && method == kVqaClockInterrupt) {
      method = kVqaClockSystem;
    }
  }

  TimerMethod = method;

  TickOffset = 0;
  const int64_t curtime = ReadMovieClock(vqap);
  TickOffset = time - curtime;
}

/****************************************************************************
 *
 * NAME
 *     ReadMovieClock - Return current time.
 *
 * SYNOPSIS
 *     Time = ReadMovieClock()
 *
 *     int64_t ReadMovieClock();
 *
 * FUNCTION
 *     This routine returns timer ticks computed one of 3 ways:
 *
 *     1) If audio is playing, the timer is based on the DMA buffer position:
 *        Compute the number of audio samples that have actually been played.
 *        The following internal HMI variables are used:
 *
 *          _lpSOSDMAFillCount[drv_handle]: current DMA buffer position
 *          _lpSOSSampleList[drv_handle][samp_handle]:
 *          sampleTotalBytes: total bytes sent by HMI to the DMA buffer
 *          sampleLastFill: HMI's last fill position in DMA buffer
 *
 *        So, the number of samples actually played is:
 *
 *          sampleTotalBytes - <DMA_diff>
 *          where <DMA_diff> is how far ahead sampleLastFill is in front of
 *          _lpSOSDMAFillCount: (sampleLastFill - _lpSOSDMAFillCount)
 *
 *        These values are indices into a circular DMA buffer, so:
 *
 *          if (sampleLastFill >= _lpSOSDMAFillCount)
 *            <DMA_diff> = sampleLastFill - _lpSOSDMAFillCount
 *          else
 *            <DMA_diff> = (DMA_BUF_SIZE - lpSOSDMAFillCount) + sampleLastFill
 *
 *        Note that, if using the stereo driver with mono data, you must
 *        divide LastFill & FillCount by 2, but not TotalBytes. If using the
 *        stereo driver with stereo data, you must divide all 3 variables
 *        by 2.
 *
 *     2) If no audio is playing, but the timer interrupt is running,
 *        VQATickCount is used as the timer
 *
 *     3) If no audio is playing & no timer interrupt is going, the DOS 18.2
 *        system timer is used.
 *
 *     Regardless of the method, TickOffset is used as an offset from the
 *     computed time.
 *
 * INPUTS
 *     NONE
 *
 * RESULT
 *     Time - Time in kVqaTicksPerSecond
 *
 ****************************************************************************/
int64_t ReadMovieClock(VqaPlayerState* vqap) {
  VqaAudio* audio = nullptr;
  VqaConfig* config = nullptr;
  int64_t totalbytes = 0;
  int64_t samples = 0;
  int play_cursor = 0;  // Bytes queued in SDLStream but not yet played
  int64_t ticks = 0;

  switch (TimerMethod) {
    /* If Audio is playing then timing is based on the audio DMA buffer
     * position.
     */
    case kVqaClockAudio:

      /* Dereference commonly used data members for quicker access. */
      audio = &vqap->movie->audio;
      config = &vqap->config;

      SDL_LockAudioDevice(vqap->config.audio_device_id);
      totalbytes = int64_t{audio->blocks_played} * config->audio_block_bytes;

      // offset by any bytes still in the stream
      // there will still be samples in the "hardware" queue, but this is the
      // best we can do
      play_cursor = SDL_AudioStreamAvailable(SDLStream);
      totalbytes -= (play_cursor * StreamConvScale) / 32768;
      SDL_UnlockAudioDevice(vqap->config.audio_device_id);

      samples = totalbytes / audio->channels;
      samples = samples / (audio->bits_per_sample / 8);

      /* The elapsed ticks is calculated by the number of samples
       * processed times the tick resolution per second divided by the
       * sample rate.
       */
      ticks = samples * kVqaTicksPerSecond / audio->sample_rate;
      ticks += TickOffset;
      break;

    /* No audio playing, but timer interrupt is going; use VQATickCount */
    case kVqaClockInterrupt:
      ticks = VQATickCount + TickOffset;
      break;

    /* No interrupts are going at all; use system time */
    default:
    case kVqaClockSystem: {
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

/****************************************************************************
 *
 * NAME
 *     VQA_TimerMethod - Get timer method being used.
 *
 * SYNOPSIS
 *     Method = VQA_TimerMethod()
 *
 *     long VQA_TimerMethod();
 *
 * FUNCTION
 *     Returns the ID of the current timer method being used.
 *
 * INPUTS
 *     NONE
 *
 * RESULT
 *     Method - Method used for the timer.
 *
 ****************************************************************************/

int32_t VQA_TimerMethod() { return TimerMethod; }
