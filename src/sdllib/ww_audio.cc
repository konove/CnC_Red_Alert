#include "sdllib/ww_audio.h"

#include <SDL_audio.h>
#include <SDL_error.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/types.h"
#include "port/unaligned.h"
#include "sdllib/aud_decoder.h"
#include "sdllib/file.h"
#include "sdllib/file_access.h"
#include "sdllib/wwstd.h"

// Volumes reach here as products of two [0, 255] factors, but Red Alert
// scales its own by 256, one past full; without the clamp that wrapped the
// 16-bit amplitude negative.
static int ToMixerAmplitude(const int scaled_volume) {
  const float normalized =
      std::min(1.0F, static_cast<float>(scaled_volume) / (255.0F * 255.0F));
  return static_cast<int>(powf(normalized, 2.0F) * 32767.0F);
}

// Decodes the payload of one block and hands the PCM to `put`, which queues
// it on the channel's converter; the two callers differ in how they hold the
// device lock. A block as long as its decoded size is stored raw. Returns
// false, putting nothing, if the block is corrupt or its compression unknown.
template <typename Put>
static bool DecodeBlock(const AudCompression compression, AdpcmState& adpcm,
                        const std::span<const std::byte> block,
                        const int decoded_bytes, Put put) {
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

static void PutPcm(SDL_AudioStream* converter,
                   const std::span<const std::byte> pcm) {
  SDL_AudioStreamPut(converter, pcm.data(), static_cast<int>(pcm.size()));
}

// When nothing more can be decoded, because the sample is complete or
// corrupt, remaining_input is left empty so the channel ends once the
// converter drains.
void AudioMixer::RefillConverter(Channel& channel) const {
  // Assumes the device rate is not lower than the sample's.
  int samples_needed = std::min(int{output_spec_.samples},
                                channel.total_samples - channel.samples_queued);
  bool ok = true;
  while (ok && samples_needed > 0) {
    // Each block starts with its size, its decoded size and the magic
    // 0x0000DEAF, which is not checked.
    if (channel.remaining_input.size() < 8) {
      ok = false;
      break;
    }
    const auto block_bytes =
        port::ReadUnaligned<uint16_t>(channel.remaining_input);
    const auto decoded_bytes =
        port::ReadUnaligned<uint16_t>(channel.remaining_input.subspan(2));
    channel.remaining_input = channel.remaining_input.subspan(8);
    if (block_bytes > channel.remaining_input.size() || decoded_bytes == 0) {
      ok = false;
      break;
    }

    ok = DecodeBlock(channel.compression, channel.adpcm,
                     channel.remaining_input.first(block_bytes), decoded_bytes,
                     [&channel](std::span<const std::byte> pcm) {
                       PutPcm(channel.converter, pcm);
                     });
    channel.remaining_input = channel.remaining_input.subspan(block_bytes);

    const int decoded_samples = decoded_bytes / (channel.bits_per_sample / 8);
    channel.samples_queued += decoded_samples;
    samples_needed -= decoded_samples;
  }

  if (!ok || channel.samples_queued >= channel.total_samples) {
    channel.remaining_input = {};
    SDL_AudioStreamFlush(channel.converter);
  }
}

void AudioMixer::ResetConverter(Channel& channel,
                                const AudHeader& header) const {
  const int channel_count = header.flags & kAudFlagStereo ? 2 : 1;
  const int bits_per_sample = header.flags & kAudFlag16Bit ? 16 : 8;

  // re-allocate stream if needed
  if (std::cmp_not_equal(channel_count, channel.channel_count) ||
      std::cmp_not_equal(bits_per_sample, channel.bits_per_sample) ||
      header.sample_rate != channel.sample_rate) {
    if (channel.converter) {
      SDL_FreeAudioStream(channel.converter);
    }

    channel.converter = SDL_NewAudioStream(
        bits_per_sample == 16 ? AUDIO_S16 : AUDIO_U8,
        static_cast<uint8_t>(channel_count), header.sample_rate,
        output_spec_.format, output_spec_.channels, output_spec_.freq);
  } else {
    SDL_AudioStreamClear(channel.converter);
  }
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

    if (channel.fade_step) {
      // update fade
      channel.scaled_volume -= channel.fade_step;
      if (channel.scaled_volume <= 0) {
        channel.playing = false;
        continue;
      }
      channel.amplitude =
          static_cast<int16_t>(ToMixerAmplitude(channel.scaled_volume));
    }
    const int stream_len = SDL_AudioStreamGet(
        channel.converter, mix_buffer_.data(),
        std::min(device_bytes, static_cast<int>(mix_buffer_.size())));

    // mix into buffer
    const int sample_count = stream_len / int{sizeof(int16_t)};
    for (int s = 0; s < sample_count; s++) {
      const base::ssize offset = s * base::ssize{sizeof(int16_t)};
      const auto mixed_so_far =
          port::ReadUnaligned<int16_t>(output.subspan(base::ToSize(offset)));
      const auto input = port::ReadUnaligned<int16_t>(
          std::span(mix_buffer_).subspan(base::ToSize(offset)));
      // Floor division of a signed sample product keeps the mix rounding.
      const int mixed =
          (input * channel.amplitude) >> 15;  // NOLINT(bugprone-signed-bitwise)
      port::WriteUnaligned(output.subspan(base::ToSize(offset)),
                           static_cast<int16_t>(mixed_so_far + mixed));
    }
  }
}

void AudioMixer::StartChannel(Channel& channel, const AudHeader& header,
                              const int priority, const int volume,
                              const int volume_scale) {
  const int channel_count = header.flags & kAudFlagStereo ? 2 : 1;
  const int bits_per_sample = header.flags & kAudFlag16Bit ? 16 : 8;

  ResetConverter(channel, header);

  channel.sample_data = nullptr;
  channel.remaining_input = {};
  channel.priority = priority;
  channel.play_volume = volume;
  channel.scaled_volume = volume * volume_scale;
  channel.amplitude =
      static_cast<int16_t>(ToMixerAmplitude(channel.scaled_volume));
  channel.fade_step = 0;
  channel.samples_queued = 0;
  channel.total_samples =
      header.uncompressed_bytes / channel_count / (bits_per_sample / 8);
  channel.sample_rate = header.sample_rate;
  channel.adpcm = {};
  channel.is_score = false;
  channel.expecting_data = false;
  channel.channel_count = static_cast<uint8_t>(channel_count);
  channel.bits_per_sample = static_cast<uint8_t>(bits_per_sample);
  channel.compression = static_cast<AudCompression>(header.compression);
  channel.playing = true;
}

// Says why a sound was turned down.
static void LogUnsupported(const AudHeader& header) {
  absl::PrintF("\trate %i size %i/%i channels %i bits %i comp %i\n",
               header.sample_rate, header.compressed_bytes,
               header.uncompressed_bytes, header.flags & kAudFlagStereo ? 2 : 1,
               header.flags & kAudFlag16Bit ? 16 : 8, header.compression);
}

int AudioMixer::Stream(const char* file_name, int volume) {
  const int handle = AcquireChannel(0xFF);
  if (handle == -1) {
    return -1;
  }
  // A free channel can still hold the file of a score that faded out since
  // the last PumpStreams().
  Stop(handle);

  const int file_handle = OpenFileHandle(file_name, FileAccess::kRead);
  if (file_handle == kInvalidHandle) {
    return -1;
  }

  AudHeader header{};
  if (ReadFileHandle(file_handle, base::ObjectBytes(header)) !=
      sizeof(header)) {
    CloseFileHandle(file_handle);
    return -1;
  }

  // Scores are all mono 16-bit ADPCM, and PumpStreams() relies on it.
  if (static_cast<AudCompression>(header.compression) != SCOMP_SOS ||
      (header.flags & kAudFlagStereo) != 0 ||
      (header.flags & kAudFlag16Bit) == 0) {
    CloseFileHandle(file_handle);
    LogUnsupported(header);
    return -1;
  }

  auto& channel = channels_.at(base::ToSize(handle));
  SDL_LockAudioDevice(device_);
  StartChannel(channel, header, 0xFF, volume, score_volume_);
  channel.is_score = true;
  channel.expecting_data = true;
  SDL_UnlockAudioDevice(device_);
  channel.file_handle = file_handle;

  return handle;
}

void AudioMixer::EndFileStream(Channel& channel) const {
  SDL_LockAudioDevice(device_);
  channel.expecting_data = false;
  SDL_AudioStreamFlush(channel.converter);
  SDL_UnlockAudioDevice(device_);

  CloseFileHandle(channel.file_handle);
  channel.file_handle = -1;
}

void AudioMixer::PumpStreams() {
  for (auto& channel : channels_) {
    if (channel.file_handle == -1) {
      continue;
    }

    // The audio thread stops a channel whose fade has run out.
    if (!channel.playing) {
      EndFileStream(channel);
      continue;
    }

    // Keep about a second queued rather than the whole file.
    const int max_queued_bytes = SDL_AUDIO_BITSIZE(output_spec_.format) / 8 *
                                 output_spec_.channels * output_spec_.freq;
    if (SDL_AudioStreamAvailable(channel.converter) >= max_queued_bytes) {
      continue;
    }

    // Block size, decoded size, and the 0x0000DEAF magic. The reads stay
    // outside the device lock so that a slow disk cannot stall the mixer.
    uint16_t block_header[4];
    if (ReadFileHandle(channel.file_handle, base::ObjectBytes(block_header)) !=
        sizeof(block_header)) {
      EndFileStream(channel);  // End of file.
      continue;
    }
    const auto block_bytes = base::At(block_header, 0);
    const auto decoded_bytes = base::At(block_header, 1);
    std::vector<std::byte> block(block_bytes);
    const bool truncated =
        ReadFileHandle(channel.file_handle, block) != block_bytes;

    const auto put_locked = [this, &channel](std::span<const std::byte> pcm) {
      SDL_LockAudioDevice(device_);
      PutPcm(channel.converter, pcm);
      SDL_UnlockAudioDevice(device_);
    };
    if (truncated || !DecodeBlock(channel.compression, channel.adpcm, block,
                                  decoded_bytes, put_locked)) {
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
  mix_buffer_.resize(output_spec_.size);

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
  mix_buffer_.resize(output_spec_.size);
}

void AudioMixer::Close() {
  SDL_CloseAudioDevice(device_);
  device_ = 0;
  mix_buffer_.clear();

  // The games close and reopen the device around their modem dialogs; a
  // channel left pointing at a freed converter would be reused then.
  for (auto& channel : channels_) {
    if (channel.file_handle != -1) {
      CloseFileHandle(channel.file_handle);
    }
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

  if (channel.file_handle != -1) {
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
  for (int i = 0; i < kChannelCount; i++) {
    if (channels_.at(base::ToSize(i)).sample_data == sample && IsPlaying(i)) {
      return true;
    }
  }

  return false;
}

void AudioMixer::Stop(const void* sample) {
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
  AudHeader header{};
  base::CopyBytes(base::ObjectBytes(header), sample, sizeof(header));

  // Mono only, as 16-bit ADPCM or 8-bit Westwood deltas.
  const bool is_16bit = (header.flags & kAudFlag16Bit) != 0;
  const auto compression = static_cast<AudCompression>(header.compression);
  const bool supported = (compression == SCOMP_SOS && is_16bit) ||
                         (compression == SCOMP_WESTWOOD && !is_16bit);
  if (!supported || (header.flags & kAudFlagStereo) != 0) {
    LogUnsupported(header);
    return -1;
  }

  const int handle = AcquireChannel(priority);
  if (handle == -1) {
    return -1;
  }
  Stop(handle);

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
      channel.scaled_volume = channel.play_volume * score_volume_;
      channel.amplitude =
          static_cast<int16_t>(ToMixerAmplitude(channel.scaled_volume));
    }
  }
  SDL_UnlockAudioDevice(device_);
}

void AudioMixer::FadeOut(int handle, int ticks) {
  if (!IsPlaying(handle)) {
    return;
  }
  // The fade advances once per device callback; `ticks` are 60ths of a
  // second. A fade shorter than one callback finishes in a single step.
  const int fade_ms = 1000 / 60 * ticks;
  const int callback_ms =
      std::max(1, output_spec_.samples * 1000 / output_spec_.freq);
  const int step_count = std::max(1, fade_ms / callback_ms);

  SDL_LockAudioDevice(device_);
  auto& channel = channels_.at(base::ToSize(handle));
  // At least 1, or a nearly silent sound would never finish fading.
  channel.fade_step = std::max(1, channel.scaled_volume / step_count);
  SDL_UnlockAudioDevice(device_);
}

int AudioMixer::AcquireChannel(const int priority) {
  for (int i = kChannelCount - 1; i >= 0; i--) {
    if (!channels_.at(base::ToSize(i)).playing) {
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

// TD
// used for nod ending
static int32_t ReadSampleFile(int file_handle, std::span<std::byte> buffer) {
  AudHeader header{};
  if (buffer.size() <= sizeof(header) || file_handle == kInvalidHandle) {
    return 0;
  }
  const int32_t header_read =
      ReadFileHandle(file_handle, base::ObjectBytes(header));
  if (header_read != sizeof(header) || header.compressed_bytes < 0) {
    return 0;
  }
  const auto payload = buffer.subspan(sizeof(header));
  const auto length =
      std::min(payload.size(), base::ToSize(header.compressed_bytes));
  const int32_t payload_read =
      ReadFileHandle(file_handle, payload.first(length));
  base::CopyBytes(buffer, base::ObjectBytes(header), sizeof(header));
  return header_read + payload_read;
}
std::span<std::byte> AudioMixer::LoadSample(const char* file_name) {
  std::span<std::byte> buffer;

  if (!file_name || !FileExists(file_name)) {
    return {};
  }

  const int file_handle = OpenFileHandle(file_name, FileAccess::kRead);
  if (file_handle != kInvalidHandle) {
    const base::ssize size = base::ToSigned(FileHandleSize(file_handle)) +
                             base::ssize{sizeof(AudHeader)};
    auto* allocation = new std::byte[base::ToSize(size)];
    // Exactly size bytes were allocated above and are released by FreeSample.
    // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
    buffer = std::span(allocation, base::ToSize(size));
    ReadSampleFile(file_handle, buffer);

    CloseFileHandle(file_handle);
  }
  return buffer.subspan(0);
}

void AudioMixer::FreeSample(void* sample) {
  if (sample) {
    Stop(sample);
    delete[] static_cast<std::byte*>(sample);
  }
}
