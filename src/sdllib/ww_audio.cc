#include "sdllib/ww_audio.h"

#include <SDL_audio.h>
#include <SDL_error.h>
#include <SDL_stdinc.h>

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

// Windows original had 5 slots; DOS had 4. One slot was reserved for disk
// streaming, leaving 4 usable slots on both platforms.
constexpr int kChannelCount = 4;

SFX_Type SoundType;
Sample_Type SampleType;

static int score_volume = 255;

static SDL_AudioDeviceID audio_device;
static SDL_AudioSpec output_spec;
static std::vector<std::byte> mix_buffer;  // temp buffer for mixing
static AudioCallback extra_callback = nullptr;

// Fields are ordered by decreasing alignment to minimize padding
// (clang-analyzer-optin.performance.Padding).
struct Channel {
  const void* sample_data = nullptr;     // identifies the sample being played
  SDL_AudioStream* converter = nullptr;  // to the device format; mixed from
  std::span<const std::byte> remaining_input;  // blocks not yet decoded

  int priority = 0;
  int play_volume = 255;  // per-sound volume [0, 255], set at play time
  int scaled_volume = 0;  // play_volume * score_volume
  int fade_step = 0;      // taken off scaled_volume per callback; 0 is no fade
  int samples_queued = 0;
  int total_samples = 0;
  int file_handle = -1;  // the streamed file; main thread only

  int16_t amplitude = 32767;  // scaled_volume as a Q15 mixing factor
  uint16_t sample_rate = 0;
  AdpcmState adpcm;

  bool playing = false;
  // A score streamed from a file: its volume follows the score volume.
  bool is_score = false;
  // PumpSampleStreams() has more of the file to queue, so an empty converter
  // is an underrun and not the end of the sound.
  bool expecting_data = false;
  uint8_t channel_count = 0;
  uint8_t bits_per_sample = 0;
  AudCompression compression = SCOMP_NONE;
};

static Channel mixer_channels[kChannelCount];

// Returns true if 'handle' names a real mixer channel.
//
// Handles come from AcquireChannel, but callers store them in long-lived
// state (ThemeClass::Current, for example) and pass them back later, so every
// public entry point validates before indexing mixer_channels.
static bool IsValidHandle(const int handle) {
  return handle >= 0 && handle < kChannelCount;
}

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
static bool DecodeBlock(Channel& channel,
                        const std::span<const std::byte> block,
                        const int decoded_bytes, Put put) {
  if (std::cmp_equal(block.size(), decoded_bytes)) {
    put(block);
    return true;
  }
  if (channel.compression == SCOMP_SOS) {
    const auto samples = DecodeAdpcmBlock(channel.adpcm, block);
    put(std::as_bytes(std::span(samples)));
    return true;
  }
  if (channel.compression == SCOMP_WESTWOOD) {
    const auto samples = DecodeWestwoodBlock(block);
    if (samples) {
      put(std::as_bytes(std::span(*samples)));
    }
    return samples.has_value();
  }
  return false;
}

static void PutPcm(const Channel& channel,
                   const std::span<const std::byte> pcm) {
  SDL_AudioStreamPut(channel.converter, pcm.data(),
                     static_cast<int>(pcm.size()));
}

// Queues about one callback's worth of an in-memory sample. Runs on the audio
// thread. When nothing more can be decoded, because the sample is complete or
// corrupt, remaining_input is left empty so the channel ends once the
// converter drains.
static void RefillConverter(Channel& channel) {
  // Assumes the device rate is not lower than the sample's.
  int samples_needed = std::min(int{output_spec.samples},
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

    ok = DecodeBlock(
        channel, channel.remaining_input.first(block_bytes), decoded_bytes,
        [&channel](std::span<const std::byte> pcm) { PutPcm(channel, pcm); });
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

static void ResetConverter(Channel& channel, const AudHeader& header) {
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
        static_cast<Uint8>(channel_count), header.sample_rate,
        output_spec.format, output_spec.channels, output_spec.freq);
  } else {
    SDL_AudioStreamClear(channel.converter);
  }
}

static void MixChannels(void* /*userdata*/, Uint8* device_buffer,
                        int device_bytes) {
  if (device_bytes < 0) {
    return;
  }
  // SDL supplies device_bytes writable bytes for the duration of this callback.
  // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
  const std::span device_span(device_buffer, base::ToSize(device_bytes));
  const auto output_bytes = std::as_writable_bytes(device_span);
  std::ranges::fill(output_bytes, std::byte{});

  // let VQA do its thing
  if (extra_callback) {
    extra_callback(device_buffer, device_bytes);
  }

  for (auto& channel : mixer_channels) {
    if (!channel.playing) {
      continue;
    }

    // An in-memory sample is decoded here as it is needed; a streamed file
    // is fed by PumpSampleStreams() on the main thread.
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
        channel.converter, mix_buffer.data(),
        std::min(device_bytes, static_cast<int>(mix_buffer.size())));

    // mix into buffer
    const int sample_count = stream_len / int{sizeof(int16_t)};
    for (int s = 0; s < sample_count; s++) {
      const base::ssize offset = s * base::ssize{sizeof(int16_t)};
      const auto output = port::ReadUnaligned<int16_t>(
          output_bytes.subspan(base::ToSize(offset)));
      const auto input = port::ReadUnaligned<int16_t>(
          std::span(mix_buffer).subspan(base::ToSize(offset)));
      // Floor division of a signed sample product keeps the mix rounding.
      const int mixed =
          (input * channel.amplitude) >> 15;  // NOLINT(bugprone-signed-bitwise)
      port::WriteUnaligned(output_bytes.subspan(base::ToSize(offset)),
                           static_cast<int16_t>(output + mixed));
    }
  }
}

// Points `channel` at a new sound described by `header` and starts it. The
// caller holds the device lock and sets the source of the data afterwards.
static void StartChannel(Channel& channel, const AudHeader& header,
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

int StreamSampleFile(const char* filename, int volume) {
  const int handle = AcquireChannel(0xFF);
  if (handle == -1) {
    return -1;
  }
  // A free channel can still hold the file of a score that faded out since
  // the last PumpSampleStreams().
  StopSample(handle);

  const int file_handle = OpenFileHandle(filename, FileAccess::kRead);
  if (file_handle == kInvalidHandle) {
    return -1;
  }

  AudHeader header{};
  if (ReadFileHandle(file_handle, base::ObjectBytes(header)) !=
      sizeof(header)) {
    CloseFileHandle(file_handle);
    return -1;
  }

  // Scores are all mono 16-bit ADPCM, and PumpSampleStreams() relies on it.
  if (static_cast<AudCompression>(header.compression) != SCOMP_SOS ||
      (header.flags & kAudFlagStereo) != 0 ||
      (header.flags & kAudFlag16Bit) == 0) {
    CloseFileHandle(file_handle);
    LogUnsupported(header);
    return -1;
  }

  auto& channel = base::At(mixer_channels, handle);
  SDL_LockAudioDevice(audio_device);
  StartChannel(channel, header, 0xFF, volume, score_volume);
  channel.is_score = true;
  channel.expecting_data = true;
  SDL_UnlockAudioDevice(audio_device);
  channel.file_handle = file_handle;

  return handle;
}

// Closes the file a channel streams from, after which the channel plays out
// what is queued. Main thread only.
static void EndFileStream(Channel& channel) {
  SDL_LockAudioDevice(audio_device);
  channel.expecting_data = false;
  SDL_AudioStreamFlush(channel.converter);
  SDL_UnlockAudioDevice(audio_device);

  CloseFileHandle(channel.file_handle);
  channel.file_handle = -1;
}

void PumpSampleStreams() {
  for (auto& channel : mixer_channels) {
    if (channel.file_handle == -1) {
      continue;
    }

    // The audio thread stops a channel whose fade has run out.
    if (!channel.playing) {
      EndFileStream(channel);
      continue;
    }

    // Keep about a second queued rather than the whole file.
    const int max_queued_bytes = SDL_AUDIO_BITSIZE(output_spec.format) / 8 *
                                 output_spec.channels * output_spec.freq;
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

    if (truncated || !DecodeBlock(channel, block, decoded_bytes,
                                  [&channel](std::span<const std::byte> pcm) {
                                    SDL_LockAudioDevice(audio_device);
                                    PutPcm(channel, pcm);
                                    SDL_UnlockAudioDevice(audio_device);
                                  })) {
      EndFileStream(channel);
    }
  }
}

bool OpenAudio(int rate, bool stereo) {
  SDL_AudioSpec desired;
  desired.freq = rate;
  desired.format = AUDIO_S16;
  desired.channels = stereo ? 2 : 1;
  // 512 samples is 23 ms at the games' 22,050 Hz. Every sound effect can
  // start up to one buffer late, and SDL2's audio thread sleeps for two
  // buffers when the device closes, so 2048 cost 93 ms of latency and 186 ms
  // on every exit.
  desired.samples = 512;
  desired.callback = MixChannels;

  // The mixing loop only knows 16-bit samples, so the format may not change.
  const int changes = SDL_AUDIO_ALLOW_FREQUENCY_CHANGE |
                      SDL_AUDIO_ALLOW_CHANNELS_CHANGE |
                      SDL_AUDIO_ALLOW_SAMPLES_CHANGE;
  audio_device =
      SDL_OpenAudioDevice(nullptr, 0, &desired, &output_spec, changes);

  if (!audio_device) {
    absl::PrintF("OpenAudio: %s\n", SDL_GetError());
    return false;
  }
  mix_buffer.resize(output_spec.size);

  SDL_PauseAudioDevice(audio_device, 0);

  SoundType = SFX_SDL;
  SampleType = SAMPLE_SDL;
  return true;
}

void CloseAudio() {
  SDL_CloseAudioDevice(audio_device);
  audio_device = 0;
  mix_buffer.clear();

  // The games close and reopen the device around their modem dialogs; a
  // channel left pointing at a freed converter would be reused then.
  for (auto& channel : mixer_channels) {
    if (channel.file_handle != -1) {
      CloseFileHandle(channel.file_handle);
    }
    SDL_FreeAudioStream(channel.converter);
    channel = {};
  }
  SoundType = SFX_NONE;
  SampleType = SAMPLE_NONE;
}

void StopSample(int handle) {
  if (!IsValidHandle(handle)) {
    return;
  }
  auto& channel = base::At(mixer_channels, handle);

  SDL_LockAudioDevice(audio_device);
  channel.playing = false;
  SDL_UnlockAudioDevice(audio_device);

  if (channel.file_handle != -1) {
    EndFileStream(channel);
  }
}

bool IsSamplePlaying(int handle) {
  if (!IsValidHandle(handle)) {
    return false;
  }
  return base::At(mixer_channels, handle).playing;
}

bool IsSamplePlaying(const void* sample) {
  for (int i = 0; i < kChannelCount; i++) {
    if (base::At(mixer_channels, i).sample_data == sample &&
        IsSamplePlaying(i)) {
      return true;
    }
  }

  return false;
}

void StopSample(const void* sample) {
  for (int i = 0; i < kChannelCount; i++) {
    if (base::At(mixer_channels, i).sample_data == sample) {
      StopSample(i);
    }
  }
}
int PlaySample(std::span<const std::byte> sample, int priority, int volume,
               int16_t panloc) {
  return PlaySampleOnChannel(sample, priority, volume, panloc,
                             AcquireChannel(priority));
}
int PlaySampleOnChannel(std::span<const std::byte> sample, int priority,
                        int volume, int16_t /*panloc*/, int handle) {
  if (!IsValidHandle(handle) || sample.size() < sizeof(AudHeader)) {
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

  StopSample(handle);

  SDL_LockAudioDevice(audio_device);
  auto& channel = base::At(mixer_channels, handle);
  StartChannel(channel, header, priority, volume, 255);
  channel.sample_data = sample.data();
  channel.remaining_input = sample.subspan(sizeof(AudHeader));
  SDL_UnlockAudioDevice(audio_device);

  return handle;
}

int SetScoreVolume(int volume) {
  const int old = score_volume;
  score_volume = volume;

  SDL_LockAudioDevice(audio_device);
  for (auto& channel : mixer_channels) {
    // A score that is fading out keeps its fade.
    if (channel.playing && channel.is_score && channel.fade_step == 0) {
      channel.scaled_volume = channel.play_volume * score_volume;
      channel.amplitude =
          static_cast<int16_t>(ToMixerAmplitude(channel.scaled_volume));
    }
  }
  SDL_UnlockAudioDevice(audio_device);

  return old;
}

void FadeOutSample(int handle, int ticks) {
  if (!IsSamplePlaying(handle)) {
    return;
  }
  // The fade advances once per device callback; `ticks` are 60ths of a
  // second. A fade shorter than one callback finishes in a single step.
  const int fade_ms = 1000 / 60 * ticks;
  const int callback_ms =
      std::max(1, output_spec.samples * 1000 / output_spec.freq);
  const int step_count = std::max(1, fade_ms / callback_ms);

  SDL_LockAudioDevice(audio_device);
  auto& channel = base::At(mixer_channels, handle);
  // At least 1, or a nearly silent sound would never finish fading.
  channel.fade_step = std::max(1, channel.scaled_volume / step_count);
  SDL_UnlockAudioDevice(audio_device);
}

int AcquireChannel(const int priority) {
  for (int i = kChannelCount - 1; i >= 0; i--) {
    if (!base::At(mixer_channels, i).playing) {
      return i;
    }
  }

  // All channels busy; evict the first with lower priority.
  for (int i = 0; i < kChannelCount; i++) {
    if (base::At(mixer_channels, i).priority < priority) {
      StopSample(i);
      return i;
    }
  }

  return -1;
}

int GetDigiHandle() {
  // used to check if audio is initialized
  return audio_device ? 1 : -1;
}

void ResumeAudio() { SDL_PauseAudioDevice(audio_device, 0); }

void PauseAudio() { SDL_PauseAudioDevice(audio_device, 1); }

uint32_t AudioDeviceId() { return audio_device; }

void* AudioOutputSpec() { return &output_spec; }

AudioCallback* ExtraAudioCallbackSlot() { return &extra_callback; }

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
std::span<std::byte> LoadSample(const char* filename) {
  std::span<std::byte> buffer;

  if (!filename || !FileExists(filename)) {
    return {};
  }

  const int file_handle = OpenFileHandle(filename, FileAccess::kRead);
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

void FreeSample(void* sample) {
  if (sample) {
    StopSample(sample);
    delete[] static_cast<std::byte*>(sample);
  }
}
