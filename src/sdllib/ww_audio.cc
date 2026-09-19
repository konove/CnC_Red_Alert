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

#include "absl/base/attributes.h"
#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/types.h"
#include "port/unaligned.h"
#include "sdllib/file.h"
#include "sdllib/file_access.h"
#include "sdllib/wwstd.h"

// Windows original had 5 slots; DOS had 4. One slot was reserved for disk
// streaming, leaving 4 usable slots on both platforms.
constexpr int kChannelCount = 4;

enum class AudCompression : uint8_t {
  SCOMP_NONE = 0,      // No compression -- raw data.
  SCOMP_WESTWOOD = 1,  // Special sliding window delta compression.
  SCOMP_SONARC = 33,   // Sonarc frame compression.
  SCOMP_SOS = 99       // SOS frame compression.
};
using enum AudCompression;

static const int8_t kImaIndexTable[] = {-1, -1, -1, -1, 2, 4, 6, 8,
                                        -1, -1, -1, -1, 2, 4, 6, 8};

static const int16_t kImaStepTable[89] = {
    7,     8,     9,     10,    11,    12,    13,    14,    16,    17,
    19,    21,    23,    25,    28,    31,    34,    37,    41,    45,
    50,    55,    60,    66,    73,    80,    88,    97,    107,   118,
    130,   143,   157,   173,   190,   209,   230,   253,   279,   307,
    337,   371,   408,   449,   494,   544,   598,   658,   724,   796,
    876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
    2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,
    5894,  6484,  7132,  7845,  8630,  9493,  10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};

static const int8_t kWestwood2BitDeltas[] = {-2, -1, 0, 1};
static const int8_t kWestwood4BitDeltas[] = {-9, -8, -6, -5, -4, -3, -2, -1,
                                             0,  1,  2,  3,  4,  5,  6,  8};

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
  int file_handle = -1;  // if this is a file stream

  int16_t amplitude = 32767;  // scaled_volume as a Q15 mixing factor
  uint16_t sample_rate = 0;
  int16_t adpcm_predictor = 0;

  bool playing = false;
  uint8_t channel_count = 0;
  uint8_t bits_per_sample = 0;
  AudCompression compression = SCOMP_NONE;
  int8_t adpcm_step_index = 0;
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

static int ToMixerAmplitude(const int scaled_volume) {
  const float normalized =
      static_cast<float>(scaled_volume) / (255.0F * 255.0F);
  return static_cast<int>(powf(normalized, 2.0F) * 32767.0F);
}
static std::span<const std::byte> DecodeAdpcmBlock(
    Channel& channel, int block_size,
    std::span<const std::byte> input ABSL_ATTRIBUTE_LIFETIME_BOUND) {
  const auto clamp = [](int value, int min, int max) {
    return std::clamp(value, min, max);
  };
  if (block_size < 0 || base::ToSize(block_size) > input.size()) {
    return {};
  }

  for (int i = 0; i < block_size; i++) {
    int16_t samples[2];
    const auto packed = std::to_integer<uint8_t>(input.front());
    input = input.subspan(1);

    // The nibble is the low or high half of the byte: a 4-bit pattern.
    uint8_t nibble = packed & 0xF;
    int step = base::At(kImaStepTable, channel.adpcm_step_index);
    channel.adpcm_step_index = static_cast<int8_t>(clamp(
        channel.adpcm_step_index + base::At(kImaIndexTable, nibble), 0, 88));

    int diff = (((((nibble & 7) * 2) + 1) * step) / 8) * (nibble & 8 ? -1 : 1);
    channel.adpcm_predictor = static_cast<int16_t>(
        clamp(channel.adpcm_predictor + diff, -32768, 32767));

    samples[0] = channel.adpcm_predictor;

    nibble = static_cast<uint8_t>(packed >> 4);
    step = base::At(kImaStepTable, channel.adpcm_step_index);
    channel.adpcm_step_index = static_cast<int8_t>(clamp(
        channel.adpcm_step_index + base::At(kImaIndexTable, nibble), 0, 88));

    diff = (((((nibble & 7) * 2) + 1) * step) / 8) * (nibble & 8 ? -1 : 1);
    channel.adpcm_predictor = static_cast<int16_t>(
        clamp(channel.adpcm_predictor + diff, -32768, 32767));

    samples[1] = channel.adpcm_predictor;

    SDL_AudioStreamPut(channel.converter, samples, sizeof(samples));
  }

  return input;
}
static std::span<const std::byte> DecodeWestwoodBlock(
    const Channel& channel, int block_size, std::span<const std::byte> input) {
  int previous_sample = 0x80;  // Previous sample (starting value).
  if (block_size < 0 || base::ToSize(block_size) > input.size()) {
    return {};
  }
  const auto remaining = input.subspan(base::ToSize(block_size));
  input = input.first(base::ToSize(block_size));

  uint8_t decoded[4];
  while (!input.empty()) {
    auto command = std::to_integer<uint8_t>(input.front());
    input = input.subspan(1);  // Get command byte
    auto count = command & 0x3FU;
    command >>= 6U;

    if (command == 2) {  // Raw sequence?
      // The command holds either a 5 bit delta or a count of
      // raw samples to dump out.
      if (count & 0x20U) {
        // The lower 5 bits are actually a signed delta.
        // Sign extend the delta and add it to the stream.
        const auto delta =
            static_cast<int8_t>(count & 0x10U ? count | 0xE0U : count & 0xFU);

        previous_sample += delta;

        decoded[0] = static_cast<uint8_t>(previous_sample);
        SDL_AudioStreamPut(channel.converter, decoded, 1);
      } else {
        // The lower 5 bits hold a count of the number of raw
        // samples that follow this command. Dump these samples to
        // the output buffer.

        count++;

        // put samples
        if (count > input.size()) {
          return {};
        }
        SDL_AudioStreamPut(channel.converter, input.data(),
                           static_cast<int>(count));
        previous_sample = std::to_integer<uint8_t>(base::At(input, count - 1));
        input = input.subspan(count);
      }
    } else {
      // Check to see if this is a 4 bit delta code sequence.
      count++;
      if (command == 1) {
        // A sequence of 4bit deltas follow. count is the
        // number of nibble packed delta bytes to process.

        do {
          if (input.empty()) {
            return {};
          }
          auto delta_codes = std::to_integer<uint8_t>(input.front());
          input = input.subspan(1);

          for (int i = 0; i < 2; i++, delta_codes >>= 4) {
            previous_sample += base::At(kWestwood4BitDeltas, delta_codes & 0xF);

            if (previous_sample < 0) {
              previous_sample = 0;
            } else if (previous_sample > 0xFF) {
              previous_sample = 0xFF;
            }

            base::At(decoded, i) = static_cast<uint8_t>(previous_sample);
          }

          SDL_AudioStreamPut(channel.converter, decoded, 2);
        } while (--count);

      } else if (command == 0) {
        // A sequence of 2bit deltas follow. count is the
        // number of packed delta bytes to process.

        do {
          if (input.empty()) {
            return {};
          }
          auto delta_codes = std::to_integer<uint8_t>(input.front());
          input = input.subspan(1);

          for (int i = 0; i < 4; i++, delta_codes >>= 2) {
            previous_sample += base::At(kWestwood2BitDeltas, delta_codes & 3);

            if (previous_sample < 0) {
              previous_sample = 0;
            } else if (previous_sample > 0xFF) {
              previous_sample = 0xFF;
            }

            base::At(decoded, i) = static_cast<uint8_t>(previous_sample);
          }

          SDL_AudioStreamPut(channel.converter, decoded, 4);
        } while (--count);
      } else {
        // There is a run of zero deltas.  Zero deltas merely duplicate
        // the 'previous' sample the requested number of times.
        do {
          decoded[0] = static_cast<uint8_t>(previous_sample);
          SDL_AudioStreamPut(channel.converter, decoded, 1);
        } while (--count);
      }
    }
  }
  return remaining.subspan(0);
}

static bool RefillConverter(Channel& channel) {
  const int samples_per_callback =
      output_spec.samples;  // assume the target rate is not lower

  if (channel.samples_queued == channel.total_samples) {
    return false;
  }

  int samples_needed = std::min(samples_per_callback,
                                channel.total_samples - channel.samples_queued);
  // read blocks until we have enough samples
  while (samples_needed > 0) {
    // read a block
    if (channel.remaining_input.size() < 8) {
      return false;
    }
    const auto block_bytes =
        port::ReadUnaligned<uint16_t>(channel.remaining_input);
    const auto decoded_bytes =
        port::ReadUnaligned<uint16_t>(channel.remaining_input.subspan(2));
    channel.remaining_input = channel.remaining_input.subspan(8);
    if (block_bytes > channel.remaining_input.size() || decoded_bytes == 0) {
      return false;  // there's also a 0000DEAF magic value
    }

    if (block_bytes == decoded_bytes)  // raw block
    {
      SDL_AudioStreamPut(channel.converter, channel.remaining_input.data(),
                         block_bytes);
      channel.remaining_input = channel.remaining_input.subspan(block_bytes);
    } else if (channel.compression == SCOMP_SOS) {  // ADPCM
      channel.remaining_input =
          DecodeAdpcmBlock(channel, block_bytes, channel.remaining_input);
    } else if (channel.compression == SCOMP_WESTWOOD) {
      channel.remaining_input =
          DecodeWestwoodBlock(channel, block_bytes, channel.remaining_input);
    } else {
      return false;  // shouldn't happen, but...
    }

    channel.samples_queued += decoded_bytes / (channel.bits_per_sample / 8);
    samples_needed -= decoded_bytes / (channel.bits_per_sample / 8);
  }

  // written all data, flush the stream
  if (channel.samples_queued == channel.total_samples) {
    SDL_AudioStreamFlush(channel.converter);
  }

  return true;
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
  const auto output_bytes =
      // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
      std::as_writable_bytes(
          std::span(device_buffer, base::ToSize(device_bytes)));
  std::ranges::fill(output_bytes, std::byte{});

  // let VQA do its thing
  if (extra_callback) {
    extra_callback(device_buffer, device_bytes);
  }

  for (auto& channel : mixer_channels) {
    if (!channel.playing) {
      continue;
    }

    // put more data into device_buffer if needed
    // unless it's a file, we do that elsewhere
    if (SDL_AudioStreamAvailable(channel.converter) < device_bytes &&
        !channel.remaining_input.empty()) {
      if (!RefillConverter(channel) &&
          !SDL_AudioStreamAvailable(channel.converter)) {
        // no more data, it's finished
        channel.playing = false;
        continue;
      }
    } else if (!SDL_AudioStreamAvailable(channel.converter) &&
               channel.remaining_input.empty() && channel.file_handle == -1) {
      // if there's no data, pointer or file handle, this is a finished file
      // device_buffer
      channel.playing = false;
      continue;
    }

    if (channel.fade_step) {
      // update fade
      channel.scaled_volume -= channel.fade_step;
      if (channel.scaled_volume <= 0) {
        channel.playing = false;
        break;
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

int StreamSampleFile(const char* filename, int volume,
                     bool /*real_time_start*/) {
  const int handle = AcquireChannel(0xFF);

  if (handle == -1) {
    return -1;
  }

  // try to open file and get header
  const int file_handle = OpenFileHandle(filename, FileAccess::kRead);

  if (file_handle < 0) {
    return -1;
  }

  AudHeader header{};
  if (ReadFileHandle(file_handle, base::ObjectBytes(header)) !=
      sizeof(header)) {
    CloseFileHandle(file_handle);
    return -1;
  }

  const int channel_count = header.flags & kAudFlagStereo ? 2 : 1;
  const int bits_per_sample = header.flags & kAudFlag16Bit ? 16 : 8;

  if (static_cast<AudCompression>(header.compression) != SCOMP_SOS ||
      channel_count != 1 || bits_per_sample != 16) {
    CloseFileHandle(file_handle);
    absl::PrintF("\trate %i size %i/%i channels %i bits %i comp %i\n",
                 header.sample_rate, header.compressed_bytes,
                 header.uncompressed_bytes, channel_count, bits_per_sample,
                 header.compression);
    return -1;
  }

  // setup channel
  SDL_LockAudioDevice(audio_device);
  auto& channel = base::At(mixer_channels, handle);

  channel.sample_data = nullptr;
  channel.playing = true;
  channel.priority = 0xFF;
  channel.play_volume = volume;
  channel.scaled_volume = volume * score_volume;
  channel.amplitude =
      static_cast<int16_t>(ToMixerAmplitude(channel.scaled_volume));
  channel.fade_step = 0;

  ResetConverter(channel, header);

  channel.channel_count = static_cast<uint8_t>(channel_count);
  channel.bits_per_sample = static_cast<uint8_t>(bits_per_sample);
  channel.sample_rate = header.sample_rate;

  channel.samples_queued = 0;
  channel.total_samples =
      header.uncompressed_bytes / channel_count / (bits_per_sample / 8);
  channel.remaining_input = {};

  channel.file_handle = file_handle;

  channel.compression = static_cast<AudCompression>(header.compression);

  if (channel.compression == SCOMP_SOS) {
    channel.adpcm_step_index = 0;
    channel.adpcm_predictor = 0;
  }

  SDL_UnlockAudioDevice(audio_device);

  return handle;
}

void PumpSampleStreams() {
  // update file stream
  for (auto& channel : mixer_channels) {
    if (channel.file_handle == -1) {
      continue;
    }

    if (!channel.playing) {
      // clean up file
      // (may have stopped playing due to fade)
      CloseFileHandle(channel.file_handle);
      channel.file_handle = -1;
      continue;
    }

    // limit how much we buffer so we don't end up with the whole file
    // (not that it's a problem on any modern system, but still)
    const int max_buf = SDL_AUDIO_BITSIZE(output_spec.format) / 8 *
                        output_spec.channels * output_spec.freq;
    if (SDL_AudioStreamAvailable(channel.converter) >= max_buf) {
      continue;
    }

    uint16_t block_header[4];
    if (ReadFileHandle(channel.file_handle, base::ObjectBytes(block_header)) !=
        8) {
      // must be eof

      SDL_LockAudioDevice(audio_device);
      SDL_AudioStreamFlush(channel.converter);
      SDL_UnlockAudioDevice(audio_device);

      CloseFileHandle(channel.file_handle);
      channel.file_handle = -1;
    } else {
      // read block
      const auto block_bytes = block_header[0];
      std::vector<std::byte> block(block_bytes);
      ReadFileHandle(channel.file_handle, block);

      SDL_LockAudioDevice(audio_device);
      DecodeAdpcmBlock(channel, block_bytes, block);

      SDL_UnlockAudioDevice(audio_device);
    }
  }
}

bool OpenAudio(void* /*window*/, int /*bits_per_sample*/, bool stereo, int rate,
               int /*reverse_channels*/) {
  SDL_AudioSpec desired;
  desired.freq = rate;
  desired.format = AUDIO_S16;  // bits_per_sample == 16 ? AUDIO_S16 : AUDIO_S8;
  desired.channels = stereo ? 2 : 1;
  // 512 samples is 23 ms at the games' 22,050 Hz. Every sound effect can
  // start up to one buffer late, and SDL2's audio thread sleeps for two
  // buffers when the device closes, so 2048 cost 93 ms of latency and 186 ms
  // on every exit.
  desired.samples = 512;
  desired.callback = MixChannels;

  // don't allow format change so I need less mising code
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
  mix_buffer.clear();

  for (const auto& channel : mixer_channels) {
    SDL_FreeAudioStream(channel.converter);
  }
}

void StopSample(int handle) {
  if (!IsValidHandle(handle)) {
    return;
  }

  SDL_LockAudioDevice(audio_device);

  base::At(mixer_channels, handle).playing = false;

  SDL_UnlockAudioDevice(audio_device);

  if (base::At(mixer_channels, handle).file_handle != -1) {
    CloseFileHandle(base::At(mixer_channels, handle).file_handle);
    base::At(mixer_channels, handle).file_handle = -1;
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
  if (!IsValidHandle(handle) || sample.empty()) {
    return -1;
  }

  // play it
  if (sample.size() < sizeof(AudHeader)) {
    return -1;
  }
  AudHeader header{};
  base::CopyBytes(base::ObjectBytes(header), sample, sizeof(header));
  const int channel_count = header.flags & kAudFlagStereo ? 2 : 1;
  const int bits_per_sample = header.flags & kAudFlag16Bit ? 16 : 8;

  const bool valid_comp =
      (static_cast<AudCompression>(header.compression) == SCOMP_SOS &&
       bits_per_sample == 16) ||
      (static_cast<AudCompression>(header.compression) == SCOMP_WESTWOOD &&
       bits_per_sample == 8);

  if (!valid_comp || channel_count != 1) {
    absl::PrintF("\trate %i size %i/%i channels %i bits %i comp %i\n",
                 header.sample_rate, header.compressed_bytes,
                 header.uncompressed_bytes, channel_count, bits_per_sample,
                 header.compression);
    return -1;
  }

  StopSample(handle);

  // setup channel
  SDL_LockAudioDevice(audio_device);
  auto& channel = base::At(mixer_channels, handle);
  channel.sample_data = sample.data();
  channel.playing = true;
  channel.priority = priority;
  channel.play_volume = volume;
  channel.scaled_volume = volume * 255;
  channel.amplitude =
      static_cast<int16_t>(ToMixerAmplitude(channel.scaled_volume));
  channel.fade_step = 0;

  ResetConverter(channel, header);

  channel.channel_count = static_cast<uint8_t>(channel_count);
  channel.bits_per_sample = static_cast<uint8_t>(bits_per_sample);
  channel.sample_rate = header.sample_rate;

  channel.samples_queued = 0;
  channel.total_samples =
      header.uncompressed_bytes / channel_count / (bits_per_sample / 8);
  channel.remaining_input = sample.subspan(sizeof(AudHeader));

  channel.compression = static_cast<AudCompression>(header.compression);

  if (channel.compression == SCOMP_SOS) {
    channel.adpcm_step_index = 0;
    channel.adpcm_predictor = 0;
  }

  SDL_UnlockAudioDevice(audio_device);

  return handle;
}

int SetScoreVolume(int volume) {
  const int old = score_volume;
  score_volume = volume;

  for (auto& channel : mixer_channels) {
    if (channel.playing &&
        channel.remaining_input.empty())  // score is a file stream
    {
      channel.scaled_volume = channel.play_volume * score_volume;
      channel.amplitude =
          static_cast<int16_t>(ToMixerAmplitude(channel.scaled_volume));
    }
  }

  return old;
}

void FadeOutSample(int handle, int ticks) {
  // recalse from game ticks, to audio callbacks
  const int fade_time = 1000 / 60 * ticks;
  const int callback_interval = output_spec.samples * 1000 / output_spec.freq;

  // A fade shorter than one callback finishes in a single step.
  const int num_steps = std::max(1, fade_time / callback_interval);

  if (IsSamplePlaying(handle)) {
    SDL_LockAudioDevice(audio_device);
    base::At(mixer_channels, handle).fade_step =
        base::At(mixer_channels, handle).scaled_volume / num_steps;
    SDL_UnlockAudioDevice(audio_device);
  }
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

bool ResumeAudio(bool /*forced*/) {
  SDL_PauseAudioDevice(audio_device, 0);
  return true;
}

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
