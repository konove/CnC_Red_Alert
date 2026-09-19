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

/***************************************************************************
 **      C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S      **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Westwood 32 bit Library                  *
 *                                                                         *
 *                    File Name : AUDIO.H                                  *
 *                                                                         *
 *                   Programmer : Phil W. Gorrow                           *
 *                                                                         *
 *                   Start Date : March 10, 1995                           *
 *                                                                         *
 *                  Last Update : March 10, 1995   [PWG]                   *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#ifndef CNC_RED_ALERT_SDLLIB_WW_AUDIO_H_
#define CNC_RED_ALERT_SDLLIB_WW_AUDIO_H_

#include <SDL_audio.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "absl/base/attributes.h"
#include "sdllib/aud_decoder.h"

/*=========================================================================*/
/* AUD file header type
 */
/*=========================================================================*/
// Bits of AudHeader::flags.
constexpr uint8_t kAudFlagStereo = 1;
constexpr uint8_t kAudFlag16Bit = 2;

// PWG 3-14-95: This structure used to have bit fields defined for Stereo
//   and Bits.  These were removed because watcom packs them into a 32 bit
//   flag entry even though they could have fit in a 8 bit entry.
#pragma pack(push, 1)
struct AudHeader {
  uint16_t sample_rate;        // Playback rate (hertz).
  int32_t compressed_bytes;    // Size of the data that follows the header.
  int32_t uncompressed_bytes;  // Size of the data once decoded.
  uint8_t flags;               // kAudFlagStereo, kAudFlag16Bit
  uint8_t compression;         // What kind of compression for this sample?
};
#pragma pack(pop)

// What the VQA player installs to have its sound track mixed in first.
using AudioCallback = void (*)(uint8_t* device_buffer, int device_bytes);

// Plays Westwood .AUD sounds on the SDL audio device: up to four at once, each
// an in-memory sample or a score streamed from a file. A sound is known by the
// handle Play() or Stream() returned, 0..3, or -1 if it could not be started;
// every function accepts a stale or invalid handle and does nothing with it.
//
// The device calls Mix() on SDL's audio thread. Everything else belongs to the
// main thread, which must call PumpStreams() regularly to keep scores fed.
//
// Example:
//   AudioMixer audio;
//   if (audio.Open(22050, /*stereo=*/false)) {
//     const int score = audio.Stream("BIGF226M.AUD", 255);
//     audio.Play(MixArchive::RetrieveData("CANNON1.AUD"));
//     while (audio.IsPlaying(score)) { audio.PumpStreams(); }
//   }
class AudioMixer {
 public:
  AudioMixer() = default;
  ~AudioMixer() { Close(); }

  // The device holds a pointer to the mixer while it is open.
  AudioMixer(const AudioMixer&) = delete;
  AudioMixer& operator=(const AudioMixer&) = delete;
  AudioMixer(AudioMixer&&) = delete;
  AudioMixer& operator=(AudioMixer&&) = delete;

  // Opens the default audio device for 16-bit output and starts it. The device
  // may settle on another rate or channel count; sounds are converted to
  // whatever it took. Returns false if there is no usable device.
  bool Open(int rate, bool stereo);

  // Sets the mixer up for mono 16-bit output at `rate` with no device behind
  // it, for tests, which call Mix() themselves. is_open() stays false.
  void OpenWithoutDevice(int rate);

  // Stops every sound and closes the device. Does nothing if it is closed.
  void Close();

  // The games skip their sound code altogether while this is false.
  [[nodiscard]] bool is_open() const { return device_ != 0; }

  // Silences the device, as when the game loses focus, and starts it again.
  // Sounds do not advance while paused.
  void Pause();
  void Resume();

  // Plays `sample`, a whole .AUD file image that must stay alive until the
  // sound ends or is stopped. `volume` is 0..255. If all channels are busy the
  // first one playing at a lower `priority` is cut off. `pan` is ignored:
  // output is mixed in mono. Returns -1 if no channel could be had or the
  // sample is not mono 16-bit ADPCM or 8-bit Westwood compressed.
  int Play(std::span<const std::byte> sample, int priority = 0xFF,
           int volume = 0xFF, int16_t pan = 0);

  // Plays the score in the game file `file_name` at `volume` times the score
  // volume, reading it a block at a time from PumpStreams(). Returns -1 if
  // there is no free channel, the file cannot be opened, or it is not mono
  // 16-bit ADPCM.
  int Stream(const char* file_name, int volume);

  // Queues the next block of each streamed score that is running low, and
  // closes the files of those that ended or were faded out.
  void PumpStreams();

  void Stop(int handle);
  // Stops every channel playing the sample whose data starts at `sample`.
  void Stop(const void* sample);

  [[nodiscard]] bool IsPlaying(int handle) const;
  [[nodiscard]] bool IsPlaying(const void* sample) const;

  // Fades the sound to silence over `ticks` 60ths of a second, then stops it.
  void FadeOut(int handle, int ticks);

  // Sets the volume, 0..255, that scales every score, including one playing.
  void SetScoreVolume(int volume);

  // Adds what the channels have to play to `output`, 16-bit samples in the
  // device's format. The device callback; public for tests.
  void Mix(std::span<std::byte> output);

  // The VQA player shares the device: it pauses and locks it by id, converts
  // to its format, and installs a callback that Mix() output is added to.
  [[nodiscard]] uint32_t device_id() const { return device_; }
  [[nodiscard]] SDL_AudioSpec* output_spec() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return &output_spec_;
  }
  [[nodiscard]] AudioCallback* extra_callback_slot()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return &extra_callback_;
  }

  // TD's Nod ending only: reads the .AUD game file `file_name` into a buffer
  // for Play(), empty if there is no such file. FreeSample() stops and frees.
  static std::span<std::byte> LoadSample(const char* file_name);
  void FreeSample(void* sample);

 private:
  // Windows original had 5 slots; DOS had 4. One slot was reserved for disk
  // streaming, leaving 4 usable slots on both platforms.
  static constexpr int kChannelCount = 4;

  // Fields are ordered by decreasing alignment to minimize padding
  // (clang-analyzer-optin.performance.Padding).
  struct Channel {
    const void* sample_data = nullptr;     // identifies the sample being played
    SDL_AudioStream* converter = nullptr;  // to the device format; mixed from
    std::span<const std::byte> remaining_input;  // blocks not yet decoded

    int priority = 0;
    int play_volume = 255;  // per-sound volume [0, 255], set at play time
    int scaled_volume = 0;  // play_volume * score volume, or * 255
    int fade_step = 0;  // taken off scaled_volume per callback; 0 is no fade
    int samples_queued = 0;
    int total_samples = 0;
    int file_handle = -1;  // the streamed file; main thread only

    int16_t amplitude = 32767;  // scaled_volume as a Q15 mixing factor
    uint16_t sample_rate = 0;
    AdpcmState adpcm;

    bool playing = false;
    // A score streamed from a file: its volume follows the score volume.
    bool is_score = false;
    // PumpStreams() has more of the file to queue, so an empty converter is
    // an underrun and not the end of the sound.
    bool expecting_data = false;
    uint8_t channel_count = 0;
    uint8_t bits_per_sample = 0;
    AudCompression compression = AudCompression::SCOMP_NONE;
  };

  static void DeviceCallback(void* mixer, uint8_t* device_buffer,
                             int device_bytes);

  [[nodiscard]] static bool IsValidHandle(int handle) {
    return handle >= 0 && handle < kChannelCount;
  }

  // Returns a free channel, or failing that stops and returns the first one
  // whose priority is below `priority`. Returns -1 if there is none.
  int AcquireChannel(int priority);

  // Points `channel` at a new sound described by `header` and starts it. The
  // caller holds the device lock and sets the source of the data afterwards.
  void StartChannel(Channel& channel, const AudHeader& header, int priority,
                    int volume, int volume_scale);

  // Gives `channel` a converter from the header's format to the device's,
  // reusing the one it has if the format is the same.
  void ResetConverter(Channel& channel, const AudHeader& header) const;

  // Queues about one callback's worth of an in-memory sample. Audio thread.
  void RefillConverter(Channel& channel) const;

  // Closes the file a channel streams from, after which the channel plays out
  // what is queued. Main thread only.
  void EndFileStream(Channel& channel) const;

  SDL_AudioDeviceID device_ = 0;       // 0 while closed
  SDL_AudioSpec output_spec_{};        // what the device settled on
  std::vector<std::byte> mix_buffer_;  // one callback's worth from a channel
  AudioCallback extra_callback_ = nullptr;
  int score_volume_ = 255;
  std::array<Channel, kChannelCount> channels_;
};

#endif  // CNC_RED_ALERT_SDLLIB_WW_AUDIO_H_
