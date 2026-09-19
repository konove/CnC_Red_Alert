#include "tech/ww_audio.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "base/buffer.h"
#include "base/numeric.h"
#include "base/seek_origin.h"
#include "base/types.h"
#include "gtest/gtest.h"
#include "sdllib/aud_decoder.h"
#include "sdllib/file_access.h"
#include "tech/file.h"
#include "tech/memory_file.h"

namespace {

constexpr int kRate = 22050;
constexpr int kCallbackSamples = 512;

// How many ScoreFiles the mixer has not closed yet.
int open_files = 0;

// An in-memory score that is counted while it lives, which is how the tests
// see the mixer let go of a file.
class ScoreFile final : public File {
 public:
  explicit ScoreFile(std::span<const std::byte> aud) : file_(std::ssize(aud)) {
    file_.Open(FileAccess::kReadWrite);
    file_.Write(aud);
    file_.Seek(0, SeekOrigin::kBegin);
    open_files++;
  }
  ~ScoreFile() override { open_files--; }

  ScoreFile(const ScoreFile&) = delete;
  ScoreFile& operator=(const ScoreFile&) = delete;
  ScoreFile(ScoreFile&&) = delete;
  ScoreFile& operator=(ScoreFile&&) = delete;

  [[nodiscard]] std::string_view FileName() const override {
    return file_.FileName();
  }
  void SetName(std::string_view name) override { file_.SetName(name); }
  bool Create() override { return file_.Create(); }
  bool Delete() override { return file_.Delete(); }
  bool IsAvailable() override { return file_.IsAvailable(); }
  [[nodiscard]] bool IsOpen() const override { return file_.IsOpen(); }
  bool Open(std::string_view name, FileAccess access) override {
    return file_.Open(name, access);
  }
  bool Open(FileAccess access) override { return file_.Open(access); }
  using File::Read;
  using File::Write;
  base::ssize Read(std::span<std::byte> buffer) override {
    return file_.Read(buffer);
  }
  base::ssize Write(std::span<const std::byte> buffer) override {
    return file_.Write(buffer);
  }
  [[nodiscard]] bool ok() const override { return file_.ok(); }
  base::ssize Seek(base::ssize offset, SeekOrigin origin) override {
    return file_.Seek(offset, origin);
  }
  base::ssize Size() override { return file_.Size(); }
  void Close() override { file_.Close(); }

 private:
  MemoryFile file_;  // Owns a copy of the score.
};

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
    open_files = 0;
    mixer_.OpenWithoutDevice(kRate);
  }

  int Stream(std::span<const std::byte> aud) {
    return mixer_.Stream(std::make_unique<ScoreFile>(aud), 255);
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

TEST_F(AudioMixerTest, LoudChannelsClipInsteadOfWrapping) {
  const auto loud = ConstantSample(kCallbackSamples, 30000);
  const auto loud_negative = ConstantSample(kCallbackSamples, -30000);
  ASSERT_NE(mixer_.Play(loud), -1);
  ASSERT_NE(mixer_.Play(loud), -1);
  EXPECT_EQ(MixOnce().front(), 32767);

  mixer_.Stop(loud.data());
  ASSERT_NE(mixer_.Play(loud_negative), -1);
  ASSERT_NE(mixer_.Play(loud_negative), -1);
  EXPECT_EQ(MixOnce().front(), -32768);
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

TEST_F(AudioMixerTest, NullSampleIsNotAPlayingScore) {
  ASSERT_NE(Stream(MakeScore(1).aud), -1);  // A score has no sample data.
  EXPECT_FALSE(mixer_.IsPlaying(nullptr));
  mixer_.Stop(nullptr);
  EXPECT_EQ(open_files, 1);
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
  const int handle = Stream(score.aud);
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
  EXPECT_EQ(mixer_.Stream("NO-SUCH-SCORE.AUD", 255), -1);
  EXPECT_EQ(mixer_.Stream(std::unique_ptr<File>(), 255), -1);

  EXPECT_EQ(Stream(ConstantSample(8, 1, 0)), -1);  // Not 16-bit.
  EXPECT_EQ(open_files, 0);

  EXPECT_EQ(Stream(std::vector<std::byte>(4)), -1);  // Not even a header.
  EXPECT_EQ(open_files, 0);
}

TEST_F(AudioMixerTest, TruncatedBlockEndsTheStream) {
  auto aud = MakeScore(2).aud;
  aud.resize(aud.size() - 10);
  const int handle = Stream(aud);
  mixer_.PumpStreams();
  EXPECT_EQ(open_files, 1);
  mixer_.PumpStreams();
  EXPECT_EQ(open_files, 0);
  MixOnce();
  MixOnce();
  EXPECT_FALSE(mixer_.IsPlaying(handle));
}

TEST_F(AudioMixerTest, StoppingAndRestartingAStreamClosesTheOldFile) {
  const auto aud = MakeScore(4).aud;
  const int handle = Stream(aud);
  mixer_.Stop(handle);
  EXPECT_EQ(open_files, 0);

  // A score that faded out is reaped by the next PumpStreams(); starting
  // another before that must not leak its file.
  const int faded = Stream(aud);
  mixer_.PumpStreams();
  mixer_.FadeOut(faded, 1);
  MixOnce();
  ASSERT_FALSE(mixer_.IsPlaying(faded));
  EXPECT_EQ(open_files, 1);
  ASSERT_NE(Stream(aud), -1);
  EXPECT_EQ(open_files, 1);

  mixer_.Close();
  EXPECT_EQ(open_files, 0);
}

TEST_F(AudioMixerTest, ScoreVolumeScalesScoresOnly) {
  const auto sample = ConstantSample(kCallbackSamples, 1000);
  ASSERT_NE(Stream(MakeScore(1).aud), -1);
  mixer_.PumpStreams();
  mixer_.SetScoreVolume(0);
  ASSERT_NE(mixer_.Play(sample), -1);
  EXPECT_EQ(std::ranges::count(MixOnce(), 999), kCallbackSamples);
}

}  // namespace
