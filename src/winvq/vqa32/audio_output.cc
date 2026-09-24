// Originally the VQA player's DOS sound code, written by Bill Randolph and
// Denzil E. Long, Jr. at Westwood Studios, August 1995, for HMI's sound
// drivers; ported to DirectSound by Steve T. in January 1996, and later to SDL.

#include "winvq/vqa32/audio_output.h"

#include <SDL_audio.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>

#include "winvq/vqa32/audio_ring.h"
#include "winvq/vqa32/movie_clock.h"
#include "winvq/vqa32/vqa_audio_device.h"

void AudioOutput::StreamDeleter::operator()(
    SDL_AudioStream* const stream) const {
  SDL_FreeAudioStream(stream);
}

std::unique_ptr<AudioOutput> AudioOutput::Create(VqaAudioDevice& device,
                                                 AudioRing& ring,
                                                 const AudioFormat& format) {
  const SDL_AudioSpec& spec = device.spec();
  SDL_AudioStream* const converter = SDL_NewAudioStream(
      format.bits_per_sample == 16 ? AUDIO_S16 : AUDIO_S8,
      static_cast<uint8_t>(format.channels), format.sample_rate, spec.format,
      spec.channels, spec.freq);
  // SDL rejects a spec it cannot convert to, such as an unknown format.
  // Without a stream the mixer would play nothing and the audio clock never
  // move, so the movie would wait forever; the format's zero sample size would
  // also divide by zero below.
  if (converter == nullptr) {
    return nullptr;
  }
  // The constructor is private, so make_unique cannot reach it.
  return std::unique_ptr<AudioOutput>(
      new AudioOutput(device, ring, format, converter));
}

AudioOutput::AudioOutput(VqaAudioDevice& device, AudioRing& ring,
                         const AudioFormat& format,
                         SDL_AudioStream* const converter)
    : device_(&device), ring_(&ring), format_(format), converter_(converter) {
  const SDL_AudioSpec& spec = device.spec();
  const int bytes_per_second_out =
      SDL_AUDIO_BITSIZE(spec.format) / 8 * spec.channels * spec.freq;
  converter_byte_ratio_ =
      (int64_t{format.bytes_per_second()} * 32768) / bytes_per_second_out;
}

AudioOutput::~AudioOutput() { Stop(); }

bool AudioOutput::Start() {
  if (playing_) {
    return true;
  }
  {
    // The count restarts from nothing played; the caller sets the clock from
    // it right after.
    const std::scoped_lock<VqaAudioDevice> lock(*device_);
    ring_->ResetBlocksPlayed();
  }
  if (!device_->Attach(
          [this](const std::span<std::byte> buffer) { Mix(buffer); })) {
    return false;
  }
  playing_ = true;
  return true;
}

void AudioOutput::Stop() {
  if (!playing_) {
    return;
  }
  device_->Detach();
  playing_ = false;
}

void AudioOutput::Mix(const std::span<std::byte> device_buffer) {
  const int device_bytes = static_cast<int>(device_buffer.size());

  // Convert whole ring blocks until there is a buffer's worth of output. The
  // remainder stays in the converter for the next call.
  while (SDL_AudioStreamAvailable(converter_.get()) < device_bytes) {
    const std::span<const unsigned char> block = ring_->play_block_bytes();
    SDL_AudioStreamPut(converter_.get(), block.data(),
                       static_cast<int>(block.size()));
    ring_->Advance();
  }

  SDL_AudioStreamGet(converter_.get(), device_buffer.data(), device_bytes);
}

int64_t AudioOutput::PlayedTicks() const {
  // The movie bytes handed to the converter, less those still waiting in it.
  int64_t played_bytes = 0;
  {
    const std::scoped_lock<VqaAudioDevice> lock(*device_);
    played_bytes = int64_t{ring_->blocks_played()} * ring_->block_bytes();
    // The queue is in the device's format; converter_byte_ratio_ turns it
    // back into movie bytes.
    const int queued_bytes = SDL_AudioStreamAvailable(converter_.get());
    played_bytes -= (queued_bytes * converter_byte_ratio_) / 32768;
  }

  const int64_t played_samples =
      played_bytes / format_.channels / (format_.bits_per_sample / 8);
  return played_samples * kVqaTicksPerSecond / format_.sample_rate;
}
