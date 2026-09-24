#include "winvq/vqa32/movie_loader.h"

#include <cstdint>
#include <memory>
#include <vector>

#include "gtest/gtest.h"
#include "winvq/vqa32/audio_output.h"
#include "winvq/vqa32/audio_ring.h"
#include "winvq/vqa32/frame_ring.h"
#include "winvq/vqa32/vqa_format.h"
#include "winvq/vqa32/vqa_test_util.h"

namespace {

constexpr AudioFormat kMono8Bit{
    .sample_rate = 22050, .channels = 1, .bits_per_sample = 8};

// A movie's frames without the chunks in front of them, which the loader
// never sees: each frame is a 4-byte sound chunk of the given ID and the
// vector pointers.
std::vector<uint8_t> SoundFrames(int count, const char* sound_id = "SND0") {
  std::vector<uint8_t> data;
  for (int i = 0; i < count; ++i) {
    AppendChunk(data, sound_id, std::vector<uint8_t>(4, 0x80));
    AppendFrameEnd(data);
  }
  return data;
}

class MovieLoaderTest : public testing::Test {
 protected:
  // A loader over frame_count frames of io_.data, with sound through a ring
  // of three 4-byte blocks and 8 bytes of staging.
  void MakeLoader(int frame_count, bool with_sound) {
    header_ = SmallHeader();
    header_.frame_count = static_cast<uint16_t>(frame_count);
    if (with_sound) {
      audio_ = std::make_unique<AudioRing>(3, 4, 8);
      output_ = AudioOutput::Create(device_, *audio_, kMono8Bit);
      ASSERT_NE(output_, nullptr);
    }
    loader_ = std::make_unique<MovieLoader>(io_, header_, ring_, audio_.get(),
                                            output_.get(), kMono8Bit, false);
  }

  FakeVqaIo io_;
  FakeVqaAudioDevice device_;
  VqaHeader header_{};
  FrameRing ring_{4, 1, 376, 1040, 1792};
  std::unique_ptr<AudioRing> audio_;
  std::unique_ptr<AudioOutput> output_;
  std::unique_ptr<MovieLoader> loader_;
};

TEST_F(MovieLoaderTest, LoadsFramesInOrderUntilTheLast) {
  io_.data = SoundFrames(2);
  MakeLoader(2, /*with_sound=*/false);

  EXPECT_EQ(loader_->LoadNextFrame(), LoadStatus::kLoaded);
  EXPECT_EQ(loader_->LoadNextFrame(), LoadStatus::kLoaded);
  EXPECT_EQ(loader_->LoadNextFrame(), LoadStatus::kEndOfMovie);
  EXPECT_EQ(loader_->next_frame_number(), 2);
  EXPECT_EQ(ring_.draw_frame().frame_number, 0);
  EXPECT_TRUE(ring_.draw_frame().loaded);
}

TEST_F(MovieLoaderTest, SkipsTheSoundOfAMoviePlayedWithoutIt) {
  io_.data = SoundFrames(1);
  MakeLoader(1, /*with_sound=*/false);

  EXPECT_EQ(loader_->LoadNextFrame(), LoadStatus::kLoaded);
  EXPECT_EQ(io_.pos, static_cast<int64_t>(io_.data.size()));
}

TEST_F(MovieLoaderTest, SkipsTheTrackNotPlayed) {
  io_.data = SoundFrames(1, "SNA0");
  MakeLoader(1, /*with_sound=*/true);

  EXPECT_EQ(loader_->LoadNextFrame(), LoadStatus::kLoaded);
  EXPECT_EQ(audio_->staged_bytes(), 0);
}

TEST_F(MovieLoaderTest, StagesTheSoundOfTheTrackPlayed) {
  io_.data = SoundFrames(1);
  MakeLoader(1, /*with_sound=*/true);

  EXPECT_EQ(loader_->LoadNextFrame(), LoadStatus::kLoaded);
  EXPECT_EQ(audio_->staged_bytes(), 4);
}

TEST_F(MovieLoaderTest, WaitsForAFullFrameRing) {
  io_.data = SoundFrames(5);
  MakeLoader(5, /*with_sound=*/false);

  for (int i = 0; i < 4; ++i) {
    ASSERT_EQ(loader_->LoadNextFrame(), LoadStatus::kLoaded);
  }
  EXPECT_EQ(loader_->LoadNextFrame(), LoadStatus::kNoBuffer);
  ring_.FinishDrawing();
  EXPECT_EQ(loader_->LoadNextFrame(), LoadStatus::kLoaded);
}

TEST_F(MovieLoaderTest, ResumesInsideASoundChunkOnceTheRingHasRoom) {
  io_.data = SoundFrames(4);
  MakeLoader(4, /*with_sound=*/true);

  // Each frame's sound moves the last one into the ring: blocks 0 and 1 fill,
  // and frame 3's would reach block 0, which has not played.
  for (int i = 0; i < 3; ++i) {
    ASSERT_EQ(loader_->LoadNextFrame(), LoadStatus::kLoaded);
  }
  ASSERT_EQ(loader_->LoadNextFrame(), LoadStatus::kAudioFull);
  EXPECT_EQ(loader_->next_frame_number(), 3);

  // Playing block 0 frees it; the loader picks up inside frame 3's chunk.
  audio_->Advance();
  EXPECT_EQ(loader_->LoadNextFrame(), LoadStatus::kLoaded);
  EXPECT_EQ(loader_->next_frame_number(), 4);
  EXPECT_EQ(audio_->staged_bytes(), 4);
  EXPECT_EQ(io_.pos, static_cast<int64_t>(io_.data.size()));
}

TEST_F(MovieLoaderTest, ATruncatedMovieFails) {
  io_.data = SoundFrames(1);
  MakeLoader(2, /*with_sound=*/false);

  EXPECT_EQ(loader_->LoadNextFrame(), LoadStatus::kLoaded);
  EXPECT_EQ(loader_->LoadNextFrame(), LoadStatus::kFailed);
}

}  // namespace
