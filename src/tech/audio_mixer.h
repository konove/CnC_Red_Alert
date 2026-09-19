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

// File: AudioMixer, which plays .AUD sounds and streamed scores on the SDL
// audio device. It stands where the Westwood 32-bit library's sound driver did
// (AUDIO.H, Phil W. Gorrow, March 1995).

#ifndef CNC_RED_ALERT_TECH_AUDIO_MIXER_H_
#define CNC_RED_ALERT_TECH_AUDIO_MIXER_H_

#include <SDL_audio.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "absl/base/attributes.h"
#include "sdllib/aud_decoder.h"
#include "tech/file.h"

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

  // Plays the score in the game file `file_name`, loose or packed in a
  // mixfile, at `volume` times the score volume, reading it a block at a time
  // from PumpStreams(). Returns -1 if there is no free channel, the file
  // cannot be opened, or it is not mono 16-bit ADPCM.
  int Stream(std::string_view file_name, int volume);

  // As above, from `file`, which must be open for reading at the AudHeader.
  // The mixer closes and destroys it when the score ends or is stopped, or at
  // once if it cannot be played.
  int Stream(std::unique_ptr<File> file, int volume);

  // Queues the next block of each streamed score that is running low, and
  // closes the files of those that ended or were faded out.
  void PumpStreams();

  void Stop(int handle);
  [[nodiscard]] bool IsPlaying(int handle) const;

  // The same for every channel playing the sample whose data starts at
  // `sample`. A null `sample`, as from a file that failed to load, is never
  // playing.
  void Stop(const void* sample);
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

 private:
  // Windows original had 5 slots; DOS had 4. One slot was reserved for disk
  // streaming, leaving 4 usable slots on both platforms.
  static constexpr int kChannelCount = 4;

  struct SampleFormat {
    uint16_t rate = 0;
    uint8_t channels = 0;
    uint8_t bits = 0;
    friend bool operator==(const SampleFormat&, const SampleFormat&) = default;
  };

  // Fields are ordered by decreasing alignment to minimize padding
  // (clang-analyzer-optin.performance.Padding).
  struct Channel {
    const void* sample_data = nullptr;  // identifies the sample being played
    std::unique_ptr<File> file;         // the streamed score; main thread only
    SDL_AudioStream* converter = nullptr;  // to the device format; mixed from
    std::span<const std::byte> remaining_input;  // blocks not yet decoded

    int priority = 0;
    int play_volume = 255;  // per-sound volume [0, 255], set at play time
    int scaled_volume = 0;  // play_volume * score volume, or * 255
    int fade_step = 0;  // taken off scaled_volume per callback; 0 is no fade
    // What of the header's uncompressed size is still to be decoded from
    // remaining_input. A streamed score ends with its file instead.
    int samples_left = 0;

    int16_t amplitude = 32767;  // scaled_volume as a Q15 mixing factor
    SampleFormat format;        // what `converter` converts from
    AdpcmState adpcm;

    bool playing = false;
    // A score streamed from a file: its volume follows the score volume.
    bool is_score = false;
    // PumpStreams() has more of the file to queue, so an empty converter is
    // an underrun and not the end of the sound.
    bool expecting_data = false;
    AudCompression compression = SCOMP_NONE;
  };

  static void DeviceCallback(void* mixer, uint8_t* device_buffer,
                             int device_bytes);

  [[nodiscard]] static bool IsValidHandle(const int handle) {
    return handle >= 0 && handle < kChannelCount;
  }

  // Returns a free channel, or failing that the first one whose priority is
  // below `priority`, stopped and with any file it held closed. Returns -1 if
  // there is none.
  int AcquireChannel(int priority);

  // Points `channel` at a new sound described by `header` and starts it. The
  // caller holds the device lock and sets the source of the data afterwards.
  void StartChannel(Channel& channel, const AudHeader& header, int priority,
                    int volume, int volume_scale);

  // Gives `channel` a converter from the header's format to the device's,
  // reusing the one it has if the format is the same.
  void ResetConverter(Channel& channel, const AudHeader& header) const;

  // Queues about one callback's worth of an in-memory sample, ending its
  // input once it is complete or turns out corrupt. Audio thread.
  void RefillConverter(Channel& channel) const;

  // Leaves `channel` nothing more to decode, so that it ends when its
  // converter drains.
  static void EndInput(Channel& channel);

  // Sets the volume and the mixing amplitude that follows from it.
  static void SetVolume(Channel& channel, int scaled_volume);

  // Closes the file a channel streams from, after which the channel plays out
  // what is queued. Main thread only.
  void EndFileStream(Channel& channel) const;

  SDL_AudioDeviceID device_ = 0;     // 0 while closed
  SDL_AudioSpec output_spec_{};      // what the device settled on
  std::vector<int16_t> mix_buffer_;  // one callback's worth from a channel
  AudioCallback extra_callback_ = nullptr;
  int score_volume_ = 255;
  std::array<Channel, kChannelCount> channels_;
};

#endif  // CNC_RED_ALERT_TECH_AUDIO_MIXER_H_
