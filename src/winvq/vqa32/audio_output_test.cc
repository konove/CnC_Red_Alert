#include "winvq/vqa32/audio_output.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

#include "gtest/gtest.h"
#include "winvq/vqa32/audio_ring.h"
#include "winvq/vqa32/vqa_test_util.h"

namespace {

constexpr AudioFormat kMono8Bit{
    .sample_rate = 22050, .channels = 1, .bits_per_sample = 8};

TEST(AudioOutputTest, FailsForAFormatSdlCannotConvertTo) {
  FakeVqaAudioDevice device;
  device.audio_spec.format = 0;
  AudioRing ring(2, 2048, 4096);

  EXPECT_EQ(AudioOutput::Create(device, ring, kMono8Bit), nullptr);
}

TEST(AudioOutputTest, InstallsItsMixerOnlyWhileStarted) {
  FakeVqaAudioDevice device;
  AudioRing ring(2, 2048, 4096);
  const auto output = AudioOutput::Create(device, ring, kMono8Bit);
  ASSERT_NE(output, nullptr);
  EXPECT_FALSE(device.attached());

  ASSERT_TRUE(output->Start());
  EXPECT_TRUE(output->playing());
  EXPECT_TRUE(device.attached());
  output->Stop();
  EXPECT_FALSE(output->playing());
  EXPECT_FALSE(device.attached());
}

TEST(AudioOutputTest, OneMovieAtATimeHoldsTheDevice) {
  FakeVqaAudioDevice device;
  AudioRing first_ring(2, 2048, 4096);
  AudioRing second_ring(2, 2048, 4096);
  const auto first = AudioOutput::Create(device, first_ring, kMono8Bit);
  const auto second = AudioOutput::Create(device, second_ring, kMono8Bit);
  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);

  ASSERT_TRUE(first->Start());
  EXPECT_FALSE(second->Start());
  first->Stop();
  EXPECT_TRUE(second->Start());
}

TEST(AudioOutputTest, DestroyingItRemovesItsMixer) {
  FakeVqaAudioDevice device;
  AudioRing ring(2, 2048, 4096);
  auto output = AudioOutput::Create(device, ring, kMono8Bit);
  ASSERT_NE(output, nullptr);
  ASSERT_TRUE(output->Start());

  output.reset();
  EXPECT_FALSE(device.attached());
}

TEST(AudioOutputTest, PullsBlocksAndCountsThemAsPlayed) {
  FakeVqaAudioDevice device;
  AudioRing ring(4, 2048, 4096);
  ring.CommitPreload(4 * 2048);
  const auto output = AudioOutput::Create(device, ring, kMono8Bit);
  ASSERT_NE(output, nullptr);
  ASSERT_TRUE(output->Start());
  EXPECT_EQ(output->PlayedTicks(), 0);

  std::array<std::byte, 1024> buffer{};
  device.Pump(buffer);
  EXPECT_GT(ring.blocks_played(), 0);
  // What was converted but not handed out yet does not count.
  EXPECT_GE(output->PlayedTicks(), 0);
  EXPECT_LE(output->PlayedTicks(),
            int64_t{ring.blocks_played()} * 2048 * 60 / 22050);
}

}  // namespace
