// File: AudioOutput, which plays a VQA movie's AudioRing on a VqaAudioDevice.

#ifndef CNC_RED_ALERT_WINVQ_VQA32_AUDIO_OUTPUT_H_
#define CNC_RED_ALERT_WINVQ_VQA32_AUDIO_OUTPUT_H_

#include <SDL_audio.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

#include "absl/base/attributes.h"
#include "winvq/vqa32/audio_ring.h"
#include "winvq/vqa32/vqa_audio_device.h"

// Plays a movie's sound: while started, its mixer is installed on the device
// and pulls blocks from the ring, converting them to the device's format.
// Stopping keeps what was already converted, so a restart after a pause picks
// up where the sound had got to. Destroying the output stops it.
//
// Example:
//   auto output = AudioOutput::Create(device, ring, format);
//   if (output != nullptr && output->Start()) { ... output->PlayedTicks() ... }
class AudioOutput {
 public:
  // Returns an output converting format to the device's, or nullptr when SDL
  // cannot convert between the two. device and ring must outlive it.
  static std::unique_ptr<AudioOutput> Create(VqaAudioDevice& device,
                                             AudioRing& ring,
                                             const AudioFormat& format);

  ~AudioOutput();
  AudioOutput(const AudioOutput&) = delete;
  AudioOutput& operator=(const AudioOutput&) = delete;
  AudioOutput(AudioOutput&&) = delete;
  AudioOutput& operator=(AudioOutput&&) = delete;

  // Starts playing the ring from its play_block(), with the count of blocks
  // played back at zero. Returns false when another movie's sound holds the
  // device.
  bool Start();
  // Stops playing. Safe to call when stopped.
  void Stop();
  [[nodiscard]] bool playing() const { return playing_; }

  // How much of the movie's sound has been handed to the device since
  // Start(), in kVqaTicksPerSecond. The device's own buffer is not counted, so
  // this runs up to one buffer ahead of what is heard.
  [[nodiscard]] int64_t PlayedTicks() const;

  // The device, locked while touching ring state the mixer reads.
  [[nodiscard]] VqaAudioDevice& device() const { return *device_; }

 private:
  struct StreamDeleter {
    void operator()(SDL_AudioStream* stream) const;
  };

  AudioOutput(VqaAudioDevice& device ABSL_ATTRIBUTE_LIFETIME_BOUND,
              AudioRing& ring ABSL_ATTRIBUTE_LIFETIME_BOUND,
              const AudioFormat& format, SDL_AudioStream* converter);

  // The installed mixer, on the audio thread with the device locked.
  void Mix(std::span<std::byte> device_buffer);

  VqaAudioDevice* device_;
  AudioRing* ring_;
  AudioFormat format_;
  std::unique_ptr<SDL_AudioStream, StreamDeleter> converter_;
  // Input bytes per output byte of converter_, in 17.15 fixed point (32768 is
  // 1.0), so PlayedTicks() can count converted bytes as movie bytes.
  int64_t converter_byte_ratio_ = 1 << 15;
  bool playing_ = false;
};

#endif  // CNC_RED_ALERT_WINVQ_VQA32_AUDIO_OUTPUT_H_
