#include "sdllib/ww_audio.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "base/buffer.h"
#include "base/numeric.h"
#include "gtest/gtest.h"
#include "sdllib/aud_decoder.h"
#include "sdllib/file.h"
#include "sdllib/file_access.h"
#include "sdllib/wwstd.h"

// The mixer reads scores through the integer-handle API, which the game
// implements. These stand in for it with one in-memory file.
namespace {
std::vector<std::byte> score_file;  // what "SCORE.AUD" holds
std::size_t score_position = 0;
int open_files = 0;
}  // namespace

int OpenFileHandle(std::string_view file_name, FileAccess /*mode*/) {
  if (file_name != "SCORE.AUD") {
    return kInvalidHandle;
  }
  score_position = 0;
  open_files++;
  return 0;
}

void CloseFileHandle(int /*handle*/) { open_files--; }

int32_t ReadFileHandle(int /*handle*/, std::span<std::byte> buffer) {
  const auto rest = std::span(score_file).subspan(score_position);
  const auto count = std::min(buffer.size(), rest.size());
  base::CopyBytes(buffer, rest, count);
  score_position += count;
  return static_cast<int32_t>(count);
}

int32_t FileHandleSize(int /*handle*/) {
  return static_cast<int32_t>(score_file.size());
}

bool FileExists(std::string_view file_name) { return file_name == "SCORE.AUD"; }

namespace {

constexpr int kRate = 22050;
constexpr int kCallbackSamples = 512;

void Append(std::vector<std::byte>& out, std::span<const std::byte> bytes) {
  out.insert(out.end(), bytes.begin(), bytes.end());
}

// One block: its size, its decoded size, the 0x0000DEAF magic, the payload.
void AppendBlock(std::vector<std::byte>& aud,
                 std::span<const std::byte> payload, int decoded_bytes) {
  const uint16_t sizes[] = {static_cast<uint16_t>(payload.size()),
                            static_cast<uint16_t>(decoded_bytes)};
  const uint32_t magic = 0x0000DEAF;
  Append(aud, std::as_bytes(std::span(sizes)));
  Append(aud, base::ObjectBytes(magic));
  Append(aud, payload);
}

std::vector<std::byte> Header(int uncompressed_bytes, uint8_t flags,
                              AudCompression compression) {
  const AudHeader header{.sample_rate = kRate,
                         .compressed_bytes = 0,
                         .uncompressed_bytes = uncompressed_bytes,
                         .flags = flags,
                         .compression = static_cast<uint8_t>(compression)};
  const auto bytes = base::ObjectBytes(header);
  return {bytes.begin(), bytes.end()};
}

// A mono 16-bit sample of `count` samples of `value`, stored as raw blocks.
std::vector<std::byte> ConstantSample(int count, int16_t value,
                                      uint8_t flags = kAudFlag16Bit) {
  auto aud = Header(count * 2, flags, SCOMP_SOS);
  const std::vector<int16_t> pcm(base::ToSize(count), value);
  AppendBlock(aud, std::as_bytes(std::span(pcm)), count * 2);
  return aud;
}

// A score of `block_count` ADPCM blocks of 64 bytes, and what it decodes to.
struct Score {
  std::vector<std::byte> aud;
  std::vector<int16_t> pcm;
};
Score MakeScore(int block_count) {
  Score score;
  score.aud = Header(block_count * 64 * 4, kAudFlag16Bit, SCOMP_SOS);
  AdpcmState state;
  for (int block = 0; block < block_count; block++) {
    std::vector<std::byte> payload(64);
    unsigned code = static_cast<unsigned>(block) * 11U;
    for (std::byte& packed : payload) {
      packed = static_cast<std::byte>(code & 0xFFU);
      code += 37U;
    }
    AppendBlock(score.aud, payload, 64 * 4);
    const auto decoded = DecodeAdpcmBlock(state, payload);
    score.pcm.insert(score.pcm.end(), decoded.begin(), decoded.end());
  }
  return score;
}

// What full volume makes of a sample: the mixer scales in Q15, rounding down.
int16_t AtFullVolume(int16_t sample) {
  // NOLINTNEXTLINE(bugprone-signed-bitwise)
  return static_cast<int16_t>((sample * 32767) >> 15);
}

class AudioMixerTest : public testing::Test {
 protected:
  void SetUp() override {
    score_file.clear();
    open_files = 0;
    mixer_.OpenWithoutDevice(kRate);
  }

  // Runs one device callback and returns the samples it produced.
  std::vector<int16_t> MixOnce() {
    std::vector<int16_t> output(kCallbackSamples, 0);
    mixer_.Mix(std::as_writable_bytes(std::span(output)));
    return output;
  }

  AudioMixer mixer_;
};

TEST_F(AudioMixerTest, PlaysASampleAndEndsWhenItIsDrained) {
  const auto sample = ConstantSample(kCallbackSamples + 100, 1000);
  const int handle = mixer_.Play(sample);
  ASSERT_NE(handle, -1);
  EXPECT_TRUE(mixer_.IsPlaying(handle));
  EXPECT_TRUE(mixer_.IsPlaying(sample.data()));

  auto output = MixOnce();
  EXPECT_EQ(std::ranges::count(output, 999), kCallbackSamples);
  output = MixOnce();
  EXPECT_EQ(std::ranges::count(output, 999), 100);
  EXPECT_EQ(output.back(), 0);
  MixOnce();
  EXPECT_FALSE(mixer_.IsPlaying(handle));
  EXPECT_FALSE(mixer_.IsPlaying(sample.data()));
}

TEST_F(AudioMixerTest, AddsChannelsTogether) {
  const auto sample = ConstantSample(kCallbackSamples, 1000);
  ASSERT_NE(mixer_.Play(sample), -1);
  ASSERT_NE(mixer_.Play(sample), -1);
  EXPECT_EQ(MixOnce().front(), 2 * 999);
}

TEST_F(AudioMixerTest, VolumePastFullIsFullNotInverted) {
  const auto sample = ConstantSample(kCallbackSamples, 1000);
  ASSERT_NE(mixer_.Play(sample, 0xFF, 256), -1);
  EXPECT_EQ(MixOnce().front(), 999);
}

TEST_F(AudioMixerTest, RejectsWhatItCannotDecode) {
  EXPECT_EQ(mixer_.Play({}), -1);
  const auto stereo = ConstantSample(8, 1, kAudFlag16Bit | kAudFlagStereo);
  EXPECT_EQ(mixer_.Play(stereo), -1);
  const auto eight_bit_adpcm = ConstantSample(8, 1, 0);
  EXPECT_EQ(mixer_.Play(eight_bit_adpcm), -1);
  EXPECT_FALSE(mixer_.IsPlaying(3));
}

TEST_F(AudioMixerTest, CorruptBlockEndsTheSample) {
  auto sample = ConstantSample(kCallbackSamples, 1000);
  sample.resize(sample.size() - 10);  // The block now overruns the sample.
  const int handle = mixer_.Play(sample);
  ASSERT_NE(handle, -1);
  EXPECT_EQ(std::ranges::count(MixOnce(), 0), kCallbackSamples);
  EXPECT_FALSE(mixer_.IsPlaying(handle));
}

TEST_F(AudioMixerTest, BusyChannelsYieldOnlyToHigherPriority) {
  const auto sample = ConstantSample(kCallbackSamples, 1000);
  for (int i = 0; i < 4; i++) {
    ASSERT_NE(mixer_.Play(sample, 10), -1);
  }
  EXPECT_EQ(mixer_.Play(sample, 10), -1);
  EXPECT_EQ(mixer_.Play(sample, 5), -1);
  EXPECT_NE(mixer_.Play(sample, 20), -1);
}

TEST_F(AudioMixerTest, StopBySampleStopsEveryChannelPlayingIt) {
  const auto sample = ConstantSample(kCallbackSamples, 1000);
  const auto other = ConstantSample(kCallbackSamples, 1000);
  ASSERT_NE(mixer_.Play(sample), -1);
  ASSERT_NE(mixer_.Play(sample), -1);
  const int kept = mixer_.Play(other);
  mixer_.Stop(sample.data());
  EXPECT_FALSE(mixer_.IsPlaying(sample.data()));
  EXPECT_TRUE(mixer_.IsPlaying(kept));
  EXPECT_EQ(MixOnce().front(), 999);
}

TEST_F(AudioMixerTest, InvalidHandlesAreIgnored) {
  mixer_.Stop(-1);
  mixer_.Stop(4);
  mixer_.FadeOut(-1, 60);
  EXPECT_FALSE(mixer_.IsPlaying(-1));
  EXPECT_FALSE(mixer_.IsPlaying(4));
}

TEST_F(AudioMixerTest, FadeEndsTheSoundWithoutSilencingLaterChannels) {
  const auto sample = ConstantSample(4 * kCallbackSamples, 1000);
  const int later_channel = mixer_.Play(sample);  // Channels fill from the top,
  const int earlier_channel = mixer_.Play(sample);  // and mix from the bottom.
  ASSERT_LT(earlier_channel, later_channel);

  mixer_.FadeOut(earlier_channel, 1);  // Shorter than one callback.
  EXPECT_EQ(MixOnce().front(), 999);
  EXPECT_FALSE(mixer_.IsPlaying(earlier_channel));
  EXPECT_TRUE(mixer_.IsPlaying(later_channel));
}

TEST_F(AudioMixerTest, LongFadeGetsQuieterEachCallback) {
  const auto sample = ConstantSample(8 * kCallbackSamples, 1000);
  const int handle = mixer_.Play(sample);
  mixer_.FadeOut(handle, 60);
  const int first = MixOnce().front();
  const int second = MixOnce().front();
  EXPECT_LT(first, 999);
  EXPECT_LT(second, first);
  EXPECT_TRUE(mixer_.IsPlaying(handle));
}

TEST_F(AudioMixerTest, StreamsAScoreBlockByBlockAndClosesItsFile) {
  const Score score = MakeScore(3);
  score_file = score.aud;
  const int handle = mixer_.Stream("SCORE.AUD", 255);
  ASSERT_NE(handle, -1);
  EXPECT_EQ(open_files, 1);

  // Nothing is queued yet: an underrun, not the end of the score.
  EXPECT_EQ(std::ranges::count(MixOnce(), 0), kCallbackSamples);
  EXPECT_TRUE(mixer_.IsPlaying(handle));

  for (int i = 0; i < 3; i++) {
    mixer_.PumpStreams();
    EXPECT_EQ(open_files, 1);
  }
  mixer_.PumpStreams();  // Finds the end of the file.
  EXPECT_EQ(open_files, 0);

  const auto output = MixOnce();
  ASSERT_EQ(score.pcm.size(), 3U * 128);
  for (std::size_t i = 0; i < score.pcm.size(); i++) {
    ASSERT_EQ(output.at(i), AtFullVolume(score.pcm.at(i))) << i;
  }
  EXPECT_EQ(output.at(score.pcm.size()), 0);
  MixOnce();
  EXPECT_FALSE(mixer_.IsPlaying(handle));
}

TEST_F(AudioMixerTest, StreamFailsCleanly) {
  EXPECT_EQ(mixer_.Stream("MISSING.AUD", 255), -1);

  score_file = ConstantSample(8, 1, 0);  // Not 16-bit.
  EXPECT_EQ(mixer_.Stream("SCORE.AUD", 255), -1);
  EXPECT_EQ(open_files, 0);

  score_file.resize(4);  // Not even a header.
  EXPECT_EQ(mixer_.Stream("SCORE.AUD", 255), -1);
  EXPECT_EQ(open_files, 0);
}

TEST_F(AudioMixerTest, TruncatedBlockEndsTheStream) {
  score_file = MakeScore(2).aud;
  score_file.resize(score_file.size() - 10);
  const int handle = mixer_.Stream("SCORE.AUD", 255);
  mixer_.PumpStreams();
  EXPECT_EQ(open_files, 1);
  mixer_.PumpStreams();
  EXPECT_EQ(open_files, 0);
  MixOnce();
  MixOnce();
  EXPECT_FALSE(mixer_.IsPlaying(handle));
}

TEST_F(AudioMixerTest, StoppingAndRestartingAStreamClosesTheOldFile) {
  score_file = MakeScore(4).aud;
  const int handle = mixer_.Stream("SCORE.AUD", 255);
  mixer_.Stop(handle);
  EXPECT_EQ(open_files, 0);

  // A score that faded out is reaped by the next PumpStreams(); starting
  // another before that must not leak its file.
  const int faded = mixer_.Stream("SCORE.AUD", 255);
  mixer_.PumpStreams();
  mixer_.FadeOut(faded, 1);
  MixOnce();
  ASSERT_FALSE(mixer_.IsPlaying(faded));
  EXPECT_EQ(open_files, 1);
  ASSERT_NE(mixer_.Stream("SCORE.AUD", 255), -1);
  EXPECT_EQ(open_files, 1);

  mixer_.Close();
  EXPECT_EQ(open_files, 0);
}

TEST_F(AudioMixerTest, ScoreVolumeScalesScoresOnly) {
  const Score score = MakeScore(1);
  score_file = score.aud;
  const auto sample = ConstantSample(kCallbackSamples, 1000);
  ASSERT_NE(mixer_.Stream("SCORE.AUD", 255), -1);
  mixer_.PumpStreams();
  mixer_.SetScoreVolume(0);
  ASSERT_NE(mixer_.Play(sample), -1);
  EXPECT_EQ(std::ranges::count(MixOnce(), 999), kCallbackSamples);
}

}  // namespace
