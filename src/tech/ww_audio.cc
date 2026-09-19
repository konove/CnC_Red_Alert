#include "tech/ww_audio.h"

#include <SDL_audio.h>
#include <SDL_error.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/numeric.h"
#include "port/unaligned.h"
#include "sdllib/aud_decoder.h"
#include "tech/file.h"
#include "tech/game_file.h"

static int ChannelCount(const AudHeader& header) {
  return (header.flags & kAudFlagStereo) != 0 ? 2 : 1;
}

static int BitsPerSample(const AudHeader& header) {
  return (header.flags & kAudFlag16Bit) != 0 ? 16 : 8;
}

// The mixer decodes mono sounds only: 16-bit ADPCM, which is what every score
// is, and for in-memory samples also 8-bit Westwood deltas.
static bool IsSupported(const AudHeader& header, const bool allow_westwood) {
  if (ChannelCount(header) != 1) {
    return false;
  }
  // The byte comes from a file and need not be any AudCompression.
  const auto compression = static_cast<AudCompression>(header.compression);
  if (compression == SCOMP_SOS) {
    return BitsPerSample(header) == 16;
  }
  return compression == SCOMP_WESTWOOD && allow_westwood &&
         BitsPerSample(header) == 8;
}

// Says why a sound was turned down.
static void LogUnsupported(const AudHeader& header) {
  absl::PrintF("\trate %i size %i/%i channels %i bits %i comp %i\n",
               header.sample_rate, header.compressed_bytes,
               header.uncompressed_bytes, ChannelCount(header),
               BitsPerSample(header), header.compression);
}

// Decodes the payload of one block and queues the PCM on `converter`, taking
// the lock of `lock_device` for just the queueing; the audio thread, which
// already holds it, passes 0. A block as long as its decoded size is stored
// raw. Returns false, queueing nothing, if the block is corrupt or its
// compression unknown.
static bool QueueBlock(SDL_AudioStream* converter,
                       const SDL_AudioDeviceID lock_device,
                       const AudCompression compression, AdpcmState& adpcm,
                       const std::span<const std::byte> block,
                       const int decoded_bytes) {
  const auto put = [converter, lock_device](std::span<const std::byte> pcm) {
    SDL_LockAudioDevice(lock_device);
    SDL_AudioStreamPut(converter, pcm.data(), static_cast<int>(pcm.size()));
    SDL_UnlockAudioDevice(lock_device);
  };
  if (std::cmp_equal(block.size(), decoded_bytes)) {
    put(block);
    return true;
  }
  if (compression == SCOMP_SOS) {
    const auto samples = DecodeAdpcmBlock(adpcm, block);
    put(std::as_bytes(std::span(samples)));
    return true;
  }
  if (compression == SCOMP_WESTWOOD) {
    const auto samples = DecodeWestwoodBlock(block);
    if (samples) {
      put(std::as_bytes(std::span(*samples)));
    }
    return samples.has_value();
  }
  return false;
}

void AudioMixer::SetVolume(Channel& channel, const int scaled_volume) {
  channel.scaled_volume = scaled_volume;
  // Volumes are products of two [0, 255] factors, but Red Alert scales its
  // own by 256, one past full; unclamped that wrapped the amplitude negative.
  const float normalized =
      std::min(1.0F, static_cast<float>(scaled_volume) / (255.0F * 255.0F));
  // Squared, which is closer to how loud a volume setting sounds.
  channel.amplitude = static_cast<int16_t>(normalized * normalized * 32767.0F);
}

void AudioMixer::EndInput(Channel& channel) {
  channel.remaining_input = {};
  SDL_AudioStreamFlush(channel.converter);
}

void AudioMixer::RefillConverter(Channel& channel) const {
  // Assumes the device rate is not lower than the sample's.
  int samples_needed =
      std::min(int{output_spec_.samples}, channel.samples_left);
  while (samples_needed > 0) {
    if (channel.remaining_input.size() < sizeof(AudBlockHeader)) {
      EndInput(channel);
      return;
    }
    const auto block_header =
        port::ReadUnaligned<AudBlockHeader>(channel.remaining_input);
    channel.remaining_input =
        channel.remaining_input.subspan(sizeof(block_header));
    if (block_header.compressed_bytes > channel.remaining_input.size() ||
        block_header.decoded_bytes == 0 ||
        !QueueBlock(
            channel.converter, 0, channel.compression, channel.adpcm,
            channel.remaining_input.first(block_header.compressed_bytes),
            block_header.decoded_bytes)) {
      EndInput(channel);
      return;
    }
    channel.remaining_input =
        channel.remaining_input.subspan(block_header.compressed_bytes);

    const int decoded_samples =
        block_header.decoded_bytes / (channel.format.bits / 8);
    channel.samples_left -= decoded_samples;
    samples_needed -= decoded_samples;
  }

  if (channel.samples_left <= 0) {
    EndInput(channel);
  }
}

void AudioMixer::ResetConverter(Channel& channel,
                                const AudHeader& header) const {
  const SampleFormat format{
      .rate = header.sample_rate,
      .channels = static_cast<uint8_t>(ChannelCount(header)),
      .bits = static_cast<uint8_t>(BitsPerSample(header))};
  if (channel.converter != nullptr && channel.format == format) {
    SDL_AudioStreamClear(channel.converter);
    return;
  }
  SDL_FreeAudioStream(channel.converter);
  channel.converter = SDL_NewAudioStream(
      format.bits == 16 ? AUDIO_S16 : AUDIO_U8, format.channels, format.rate,
      output_spec_.format, output_spec_.channels, output_spec_.freq);
  channel.format = format;
}

void AudioMixer::DeviceCallback(void* mixer, uint8_t* device_buffer,
                                const int device_bytes) {
  if (device_bytes < 0) {
    return;
  }
  // SDL supplies device_bytes writable bytes for the duration of this callback.
  // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
  const std::span device_span(device_buffer, base::ToSize(device_bytes));
  const auto output = std::as_writable_bytes(device_span);
  std::ranges::fill(output, std::byte{});

  auto& self = *static_cast<AudioMixer*>(mixer);
  // A movie's sound track goes in first.
  if (self.extra_callback_) {
    self.extra_callback_(device_buffer, device_bytes);
  }
  self.Mix(output);
}

void AudioMixer::Mix(const std::span<std::byte> output) {
  const int device_bytes = static_cast<int>(output.size());
  for (auto& channel : channels_) {
    if (!channel.playing) {
      continue;
    }

    // An in-memory sample is decoded here as it is needed; a streamed file
    // is fed by PumpStreams() on the main thread.
    if (SDL_AudioStreamAvailable(channel.converter) < device_bytes &&
        !channel.remaining_input.empty()) {
      RefillConverter(channel);
    }
    if (SDL_AudioStreamAvailable(channel.converter) == 0 &&
        channel.remaining_input.empty() && !channel.expecting_data) {
      channel.playing = false;
      continue;
    }

    if (channel.fade_step != 0) {
      if (channel.scaled_volume <= channel.fade_step) {
        channel.playing = false;
        continue;
      }
      SetVolume(channel, channel.scaled_volume - channel.fade_step);
    }
    const int mix_bytes = SDL_AudioStreamGet(
        channel.converter, mix_buffer_.data(),
        std::min(device_bytes,
                 static_cast<int>(mix_buffer_.size() * sizeof(int16_t))));

    // The device buffer is SDL's and need not be aligned for int16_t.
    const int sample_count = mix_bytes / int{sizeof(int16_t)};
    for (int s = 0; s < sample_count; s++) {
      const auto slot = output.subspan(base::ToSize(s) * sizeof(int16_t));
      const int mixed_so_far = port::ReadUnaligned<int16_t>(slot);
      // Floor division of a signed sample product keeps the mix rounding.
      const int product =
          base::At(std::span(mix_buffer_), s) * channel.amplitude;
      const int scaled = product >> 15;  // NOLINT(bugprone-signed-bitwise)
      // Loud sounds on top of each other clip; wrapping would crackle.
      port::WriteUnaligned(slot, static_cast<int16_t>(std::clamp(
                                     mixed_so_far + scaled, -32768, 32767)));
    }
  }
}

void AudioMixer::StartChannel(Channel& channel, const AudHeader& header,
                              const int priority, const int volume,
                              const int volume_scale) {
  ResetConverter(channel, header);

  channel.sample_data = nullptr;
  channel.remaining_input = {};
  channel.priority = priority;
  channel.play_volume = volume;
  SetVolume(channel, volume * volume_scale);
  channel.fade_step = 0;
  channel.samples_left = header.uncompressed_bytes / channel.format.channels /
                         (channel.format.bits / 8);
  channel.adpcm = {};
  channel.is_score = false;
  channel.expecting_data = false;
  channel.compression = static_cast<AudCompression>(header.compression);
  channel.playing = true;
}

int AudioMixer::Stream(const std::string_view file_name, const int volume) {
  auto file = std::make_unique<GameFile>(file_name);
  if (!file->Open()) {
    return -1;
  }
  return Stream(std::move(file), volume);
}

int AudioMixer::Stream(std::unique_ptr<File> file, const int volume) {
  AudHeader header{};
  if (!file || !file->ReadObject(header)) {
    return -1;
  }

  if (!IsSupported(header, /*allow_westwood=*/false)) {
    LogUnsupported(header);
    return -1;
  }

  const int handle = AcquireChannel(0xFF);
  if (handle == -1) {
    return -1;
  }

  auto& channel = channels_.at(base::ToSize(handle));
  SDL_LockAudioDevice(device_);
  StartChannel(channel, header, 0xFF, volume, score_volume_);
  channel.is_score = true;
  channel.expecting_data = true;
  SDL_UnlockAudioDevice(device_);
  channel.file = std::move(file);

  return handle;
}

void AudioMixer::EndFileStream(Channel& channel) const {
  SDL_LockAudioDevice(device_);
  channel.expecting_data = false;
  SDL_AudioStreamFlush(channel.converter);
  SDL_UnlockAudioDevice(device_);

  channel.file.reset();
}

void AudioMixer::PumpStreams() {
  // Keep about a second queued rather than the whole file.
  const int max_queued_bytes = SDL_AUDIO_BITSIZE(output_spec_.format) / 8 *
                               output_spec_.channels * output_spec_.freq;

  for (auto& channel : channels_) {
    if (!channel.file) {
      continue;
    }

    // The audio thread stops a channel whose fade has run out.
    if (!channel.playing) {
      EndFileStream(channel);
      continue;
    }

    if (SDL_AudioStreamAvailable(channel.converter) >= max_queued_bytes) {
      continue;
    }

    // The reads stay outside the device lock so that a slow disk cannot
    // stall the mixer.
    AudBlockHeader block_header{};
    if (!channel.file->ReadObject(block_header)) {
      EndFileStream(channel);  // End of file.
      continue;
    }
    const std::vector<std::byte> block =
        channel.file->ReadBytes(block_header.compressed_bytes);
    const bool truncated = std::ssize(block) != block_header.compressed_bytes;

    if (truncated ||
        !QueueBlock(channel.converter, device_, channel.compression,
                    channel.adpcm, block, block_header.decoded_bytes)) {
      EndFileStream(channel);
    }
  }
}

bool AudioMixer::Open(int rate, bool stereo) {
  Close();

  SDL_AudioSpec desired{};
  desired.freq = rate;
  desired.format = AUDIO_S16;
  desired.channels = stereo ? 2 : 1;
  // 512 samples is 23 ms at the games' 22,050 Hz. Every sound effect can
  // start up to one buffer late, and SDL2's audio thread sleeps for two
  // buffers when the device closes, so 2048 cost 93 ms of latency and 186 ms
  // on every exit.
  desired.samples = 512;
  desired.callback = DeviceCallback;
  desired.userdata = this;

  // The mixing loop only knows 16-bit samples, so the format may not change.
  const int changes = SDL_AUDIO_ALLOW_FREQUENCY_CHANGE |
                      SDL_AUDIO_ALLOW_CHANNELS_CHANGE |
                      SDL_AUDIO_ALLOW_SAMPLES_CHANGE;
  device_ = SDL_OpenAudioDevice(nullptr, 0, &desired, &output_spec_, changes);

  if (!device_) {
    absl::PrintF("AudioMixer::Open: %s\n", SDL_GetError());
    return false;
  }
  mix_buffer_.resize(output_spec_.size / sizeof(int16_t));

  SDL_PauseAudioDevice(device_, 0);
  return true;
}

void AudioMixer::OpenWithoutDevice(const int rate) {
  Close();
  output_spec_.freq = rate;
  output_spec_.format = AUDIO_S16;
  output_spec_.channels = 1;
  output_spec_.samples = 512;
  output_spec_.size = output_spec_.samples * sizeof(int16_t);
  mix_buffer_.resize(output_spec_.size / sizeof(int16_t));
}

void AudioMixer::Close() {
  SDL_CloseAudioDevice(device_);
  device_ = 0;
  mix_buffer_.clear();

  // The games close and reopen the device around their modem dialogs; a
  // channel left pointing at a freed converter would be reused then.
  for (auto& channel : channels_) {
    SDL_FreeAudioStream(channel.converter);
    channel = {};
  }
}

void AudioMixer::Stop(int handle) {
  if (!IsValidHandle(handle)) {
    return;
  }
  auto& channel = channels_.at(base::ToSize(handle));

  SDL_LockAudioDevice(device_);
  channel.playing = false;
  SDL_UnlockAudioDevice(device_);

  if (channel.file) {
    EndFileStream(channel);
  }
}

bool AudioMixer::IsPlaying(int handle) const {
  if (!IsValidHandle(handle)) {
    return false;
  }
  // The audio thread clears the flag when the sound or its fade runs out.
  SDL_LockAudioDevice(device_);
  const bool playing = channels_.at(base::ToSize(handle)).playing;
  SDL_UnlockAudioDevice(device_);
  return playing;
}

bool AudioMixer::IsPlaying(const void* sample) const {
  // Scores have no sample data, and must not be taken for a null sample.
  if (sample == nullptr) {
    return false;
  }
  // One lock for the scan: this is polled from the games' wait loops.
  SDL_LockAudioDevice(device_);
  const bool playing =
      std::ranges::any_of(channels_, [sample](const Channel& channel) {
        return channel.playing && channel.sample_data == sample;
      });
  SDL_UnlockAudioDevice(device_);
  return playing;
}

void AudioMixer::Stop(const void* sample) {
  if (sample == nullptr) {
    return;
  }
  for (int i = 0; i < kChannelCount; i++) {
    if (channels_.at(base::ToSize(i)).sample_data == sample) {
      Stop(i);
    }
  }
}

int AudioMixer::Play(std::span<const std::byte> sample, int priority,
                     int volume, int16_t /*pan*/) {
  if (sample.size() < sizeof(AudHeader)) {
    return -1;
  }
  const auto header = port::ReadUnaligned<AudHeader>(sample);

  if (!IsSupported(header, /*allow_westwood=*/true)) {
    LogUnsupported(header);
    return -1;
  }

  const int handle = AcquireChannel(priority);
  if (handle == -1) {
    return -1;
  }

  SDL_LockAudioDevice(device_);
  auto& channel = channels_.at(base::ToSize(handle));
  StartChannel(channel, header, priority, volume, 255);
  channel.sample_data = sample.data();
  channel.remaining_input = sample.subspan(sizeof(AudHeader));
  SDL_UnlockAudioDevice(device_);

  return handle;
}

void AudioMixer::SetScoreVolume(int volume) {
  score_volume_ = volume;

  SDL_LockAudioDevice(device_);
  for (auto& channel : channels_) {
    // A score that is fading out keeps its fade.
    if (channel.playing && channel.is_score && channel.fade_step == 0) {
      SetVolume(channel, channel.play_volume * score_volume_);
    }
  }
  SDL_UnlockAudioDevice(device_);
}

void AudioMixer::FadeOut(int handle, int ticks) {
  if (!IsValidHandle(handle)) {
    return;
  }
  auto& channel = channels_.at(base::ToSize(handle));

  SDL_LockAudioDevice(device_);
  if (channel.playing) {
    // The fade advances once per device callback; `ticks` are 60ths of a
    // second. A fade shorter than one callback finishes in a single step.
    const int fade_ms = 1000 / 60 * ticks;
    const int callback_ms =
        std::max(1, output_spec_.samples * 1000 / output_spec_.freq);
    const int step_count = std::max(1, fade_ms / callback_ms);
    // At least 1, or a nearly silent sound would never finish fading.
    channel.fade_step = std::max(1, channel.scaled_volume / step_count);
  }
  SDL_UnlockAudioDevice(device_);
}

int AudioMixer::AcquireChannel(const int priority) {
  for (int i = kChannelCount - 1; i >= 0; i--) {
    if (!channels_.at(base::ToSize(i)).playing) {
      // It can still hold the file of a score that ended or faded out since
      // the last PumpStreams().
      Stop(i);
      return i;
    }
  }

  // All channels busy; evict the first with lower priority.
  for (int i = 0; i < kChannelCount; i++) {
    if (channels_.at(base::ToSize(i)).priority < priority) {
      Stop(i);
      return i;
    }
  }

  return -1;
}

// Pausing changes the state of the audio device, not of the mixer.
// NOLINTNEXTLINE(readability-make-member-function-const)
void AudioMixer::Resume() { SDL_PauseAudioDevice(device_, 0); }

// NOLINTNEXTLINE(readability-make-member-function-const)
void AudioMixer::Pause() { SDL_PauseAudioDevice(device_, 1); }
