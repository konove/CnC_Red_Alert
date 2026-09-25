#ifndef CNC_RED_ALERT_ENGINE_VIDEO_MIXER_VQA_AUDIO_H_
#define CNC_RED_ALERT_ENGINE_VIDEO_MIXER_VQA_AUDIO_H_

// File: MixerVqaAudio, the sound device VQA movies play through in both games.

#include <SDL_audio.h>

#include <utility>

#include "absl/base/attributes.h"
#include "engine/audio/audio_mixer.h"
#include "engine/video/vqa/vqa_audio_device.h"

// Plays a movie's sound on the game's AudioMixer, ahead of the game's own
// sounds. Pass one to VqaPlayer::Open(); it
// must outlive the open movie, and the mixer must outlive it.
//
// Example:
//   MixerVqaAudio movie_audio(engine::audio::TheAudio());
//   auto player = VqaPlayer::Open(io, name, screen, &movie_audio);
class MixerVqaAudio final : public VqaAudioDevice {
 public:
  explicit MixerVqaAudio(
      engine::audio::AudioMixer& mixer ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : mixer_(&mixer) {}

  [[nodiscard]] const SDL_AudioSpec& spec() const override {
    return mixer_->output_spec();
  }
  bool Attach(Mixer mixer) override {
    return mixer_->AttachExtraCallback(std::move(mixer));
  }
  void Detach() override { mixer_->DetachExtraCallback(); }
  void lock() override { SDL_LockAudioDevice(mixer_->device_id()); }
  void unlock() override { SDL_UnlockAudioDevice(mixer_->device_id()); }

 private:
  engine::audio::AudioMixer* mixer_;
};

#endif  // CNC_RED_ALERT_ENGINE_VIDEO_MIXER_VQA_AUDIO_H_
