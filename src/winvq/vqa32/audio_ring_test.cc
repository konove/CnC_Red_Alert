#include "winvq/vqa32/audio_ring.h"

#include <algorithm>
#include <cstdint>

#include "base/array.h"
#include "gtest/gtest.h"
#include "winvq/vqa32/vqa_format.h"

namespace {

// Stages bytes of sound, all of value fill.
void StageFilled(AudioRing& ring, int bytes, uint8_t fill) {
  std::ranges::fill(ring.staging(), fill);
  ring.Stage(bytes);
}

TEST(AudioFormatTest, ReadsTheSoundTrackFromTheHeader) {
  VqaHeader header{};
  header.version = kVqaVersion2;
  header.sample_rate = 22050;
  header.channels = 1;
  header.bits_per_sample = 16;

  const AudioFormat format = AudioFormat::FromHeader(header);
  EXPECT_EQ(format.sample_rate, 22050);
  EXPECT_EQ(format.bytes_per_second(), 44100);
}

TEST(AudioFormatTest, Version1MoviesAre22050HzMono8Bit) {
  VqaHeader header{};
  header.version = kVqaVersion1;
  header.sample_rate = 44100;

  const AudioFormat format = AudioFormat::FromHeader(header);
  EXPECT_EQ(format.sample_rate, 22050);
  EXPECT_EQ(format.channels, 1);
  EXPECT_EQ(format.bits_per_sample, 8);
}

TEST(AudioRingTest, CopyStagedMarksOnlyCompletedBlocks) {
  AudioRing ring(4, 2048, 4096);

  StageFilled(ring, 1000, 1);
  ASSERT_TRUE(ring.CopyStaged());
  EXPECT_EQ(ring.write_offset(), 1000);
  EXPECT_EQ(ring.staged_bytes(), 0);
  EXPECT_FALSE(ring.block_loaded(0));

  StageFilled(ring, 2048, 1);
  ASSERT_TRUE(ring.CopyStaged());
  EXPECT_EQ(ring.write_offset(), 3048);
  EXPECT_TRUE(ring.block_loaded(0));
  EXPECT_FALSE(ring.block_loaded(1));
}

TEST(AudioRingTest, NothingStagedIsNothingToCopy) {
  AudioRing ring(2, 2048, 4096);

  EXPECT_TRUE(ring.CopyStaged());
  EXPECT_EQ(ring.write_offset(), 0);
}

TEST(AudioRingTest, CopyStagedWrapsAtTheEndOfTheRing) {
  AudioRing ring(4, 2048, 8192);
  // Blocks 0-2 and half of block 3.
  StageFilled(ring, (3 * 2048) + 1024, 1);
  ASSERT_TRUE(ring.CopyStaged());
  // Playing frees blocks 0 and 1, then waits at block 2 for block 3.
  ring.Advance();
  ring.Advance();
  ring.Advance();
  ASSERT_EQ(ring.play_block(), 2);

  // From the middle of block 3 round to the middle of block 0, completing
  // block 3 only.
  StageFilled(ring, 1024 + 500, 2);
  ASSERT_TRUE(ring.CopyStaged());
  EXPECT_EQ(ring.write_offset(), 500);
  EXPECT_TRUE(ring.block_loaded(3));
  EXPECT_FALSE(ring.block_loaded(0));
  EXPECT_EQ(base::At(ring.ring(), (4 * 2048) - 1), 2);
  EXPECT_EQ(base::At(ring.ring(), 499), 2);
  EXPECT_EQ(base::At(ring.ring(), 500), 1);
}

TEST(AudioRingTest, CopyStagedWaitsForUnplayedBlocks) {
  AudioRing ring(4, 2048, 8192);
  StageFilled(ring, 3 * 2048, 1);
  ASSERT_TRUE(ring.CopyStaged());
  ring.Advance();  // Frees block 0 only.

  // Reaching block 1, which has not played, waits with the sound staged.
  StageFilled(ring, 2 * 2048, 2);
  EXPECT_FALSE(ring.CopyStaged());
  EXPECT_EQ(ring.staged_bytes(), 2 * 2048);
  EXPECT_EQ(ring.write_offset(), 3 * 2048);
}

TEST(AudioRingTest, AdvanceReplaysABlockWhenTheNextIsNotLoaded) {
  AudioRing ring(2, 2048, 4096);
  ring.CommitPreload(2048);

  ring.Advance();
  EXPECT_EQ(ring.play_block(), 0);
  EXPECT_TRUE(ring.underran());
  // A replay does not count as played until the movie is loaded.
  EXPECT_EQ(ring.blocks_played(), 0);

  ring.MarkMovieLoaded();
  ring.Advance();
  EXPECT_EQ(ring.play_block(), 0);
  EXPECT_EQ(ring.blocks_played(), 1);
}

TEST(AudioRingTest, AdvanceWrapsFromTheLastBlock) {
  AudioRing ring(2, 2048, 4096);
  ring.CommitPreload(2 * 2048);

  ring.Advance();
  ASSERT_EQ(ring.play_block(), 1);
  EXPECT_FALSE(ring.block_loaded(0));
  ring.CommitPreload(2048);
  ring.Advance();
  EXPECT_EQ(ring.play_block(), 0);
  EXPECT_EQ(ring.blocks_played(), 2);
  EXPECT_FALSE(ring.underran());
}

TEST(AudioRingTest, APreloadFillingTheRingWrapsTheWriteOffset) {
  AudioRing ring(2, 2048, 4096);
  ring.CommitPreload(2 * 2048);
  EXPECT_EQ(ring.write_offset(), 0);
  EXPECT_TRUE(ring.block_loaded(0));
  EXPECT_TRUE(ring.block_loaded(1));

  // Once block 0 has played, the next chunk goes in at the start.
  ring.Advance();
  StageFilled(ring, 100, 1);
  ASSERT_TRUE(ring.CopyStaged());
  EXPECT_EQ(ring.write_offset(), 100);
}

TEST(AudioRingTest, PlaysFromThePlayBlock) {
  AudioRing ring(2, 4, 8);
  std::ranges::fill(ring.ring().subspan(4), uint8_t{7});
  ring.CommitPreload(2 * 4);
  ring.Advance();

  ASSERT_EQ(ring.play_block(), 1);
  EXPECT_EQ(ring.play_block_bytes().size(), 4U);
  EXPECT_EQ(base::At(ring.play_block_bytes(), 0), 7);
}

}  // namespace
