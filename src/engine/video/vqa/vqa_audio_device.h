// File: VqaAudioDevice, the sound device a VQA movie plays through. The player
// never opens a device itself; whoever plays a movie with sound provides one.
// The games' implementation is MixerVqaAudio in
// engine/video/mixer_vqa_audio.h; tests use a fake.

#ifndef CNC_RED_ALERT_ENGINE_VIDEO_VQA_VQA_AUDIO_DEVICE_H_
#define CNC_RED_ALERT_ENGINE_VIDEO_VQA_VQA_AUDIO_DEVICE_H_

#include <cstddef>
#include <functional>
#include <span>

struct SDL_AudioSpec;

// A sound device shared with the rest of the program. A movie installs its
// mixer while its sound plays; the device calls it from its own thread with
// the device locked. It is BasicLockable: lock() holds that thread off, so
// std::scoped_lock works on it.
class VqaAudioDevice {
 public:
  // Fills a device buffer, which arrives silenced, in the device's format.
  using Mixer = std::function<void(std::span<std::byte> device_buffer)>;

  VqaAudioDevice() = default;
  virtual ~VqaAudioDevice() = default;
  VqaAudioDevice(const VqaAudioDevice&) = delete;
  VqaAudioDevice& operator=(const VqaAudioDevice&) = delete;
  VqaAudioDevice(VqaAudioDevice&&) = delete;
  VqaAudioDevice& operator=(VqaAudioDevice&&) = delete;

  // The device's output format, which the movie's sound is converted to.
  [[nodiscard]] virtual const SDL_AudioSpec& spec() const = 0;

  // Installs mixer. Returns false, installing nothing, when another movie's
  // mixer is installed already.
  virtual bool Attach(Mixer mixer) = 0;
  // Removes the installed mixer, waiting out a call in progress. Safe to call
  // when none is installed.
  virtual void Detach() = 0;

  virtual void lock() = 0;
  virtual void unlock() = 0;
};

#endif  // CNC_RED_ALERT_ENGINE_VIDEO_VQA_VQA_AUDIO_DEVICE_H_
