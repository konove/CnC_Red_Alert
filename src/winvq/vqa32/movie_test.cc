// Tests for Movie: opening and validating a movie, loading its chunks into
// the play buffers, and playing it, on small synthetic movies served by an
// in-memory VqaIo.

#include "winvq/vqa32/movie.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/array.h"
#include "gtest/gtest.h"
#include "winvq/vqa32/audio_ring.h"
#include "winvq/vqa32/frame_ring.h"
#include "winvq/vqa32/lcw_buffer.h"
#include "winvq/vqa32/movie_drawer.h"
#include "winvq/vqa32/vqa_audio_device.h"
#include "winvq/vqa32/vqa_format.h"
#include "winvq/vqa32/vqa_player.h"
#include "winvq/vqa32/vqa_test_util.h"

namespace {

constexpr int kCodebookCapacity = 376;

// Opens movies from fake_ with one frame buffer and one codebook, so opening
// preloads exactly one frame, and reaches into the Movie to check its parts.
class MovieTest : public testing::Test {
 protected:
  MovieTest() {
    options_.frame_buffers = 1;
    options_.codebook_buffers = 1;
  }

  // Opens the movie, keeping it in movie_. Returns the error, or nullopt.
  std::optional<VqaError> Open() {
    auto movie = Movie::Open(fake_, "test.vqa", client_, audio_, options_);
    if (!movie.has_value()) {
      return movie.error();
    }
    movie_ = std::move(*movie);
    return std::nullopt;
  }

  // Plays the sound through device_.
  void EnableAudio() { audio_ = &device_; }

  // Plays the opened movie to its end: the first step starts it, and then
  // every frame is due at once.
  void PlayToTheEnd() {
    movie_->Step();
    movie_->clock().Set(1'000'000, nullptr);
    movie_->Run();
  }

  FakeVqaIo fake_;
  FakeVqaAudioDevice device_;
  VqaAudioDevice* audio_ = nullptr;
  RecordingClient client_;
  VqaOptions options_;
  std::unique_ptr<Movie> movie_;
};

TEST_F(MovieTest, OpensMinimalMovie) {
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), std::nullopt);
  EXPECT_EQ(fake_.pos, static_cast<int64_t>(fake_.data.size()));
}

TEST_F(MovieTest, SkipsTheFrameTable) {
  // Eight entries for a three-frame movie.
  fake_.data = MovieStart(SmallHeader(),
                          {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80});
  AppendFrameEnd(fake_.data);

  ASSERT_EQ(Open(), std::nullopt);
  // The table was skipped whole, so the frame after it loaded.
  EXPECT_EQ(fake_.pos, static_cast<int64_t>(fake_.data.size()));
}

TEST_F(MovieTest, RejectsFinfBeforeHeader) {
  fake_.data = ValidPreamble();
  AppendChunk(fake_.data, "FINF", FinfPayload({0, 0, 0}));

  EXPECT_EQ(Open(), VqaError::kNotVqa);
}

TEST_F(MovieTest, RejectsHeaderWithZeroGroupsize) {
  VqaHeader header = SmallHeader();
  header.frames_per_group = 0;
  fake_.data = MovieStart(header, {0, 0, 0});
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), VqaError::kNotVqa);
}

TEST_F(MovieTest, RejectsHeaderWithZeroBlockSize) {
  VqaHeader header = SmallHeader();
  header.block_width = 0;
  fake_.data = MovieStart(header, {0, 0, 0});
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), VqaError::kNotVqa);
}

TEST_F(MovieTest, RejectsPreFrameChunkOf2GiB) {
  fake_.data = ValidPreamble();
  AppendChunk(fake_.data, "VQHD", HeaderPayload(SmallHeader()));
  // 2^31 reads back as a negative int32_t size.
  AppendChunk(fake_.data, "XXXX", 0x80000000U, {});

  EXPECT_EQ(Open(), VqaError::kNotVqa);
}

TEST_F(MovieTest, RejectsFrameChunkOf2GiB) {
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendChunk(fake_.data, "VQFR", 0x80000000U, {});
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), VqaError::kRead);
}

TEST_F(MovieTest, RejectsChunkOf2GiBInsideFrame) {
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  std::vector<uint8_t> frame;
  AppendChunk(frame, "CBF0", 0xFFFFFFF8U, {});
  AppendChunk(fake_.data, "VQFR", frame);
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), VqaError::kRead);
}

TEST_F(MovieTest, KeyFrameContainerMarksAKeyFrame) {
  // The container's own ID says key frame; the chunks inside it do not.
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  std::vector<uint8_t> frame;
  AppendFrameEnd(frame);
  AppendChunk(fake_.data, "VQFK", frame);

  ASSERT_EQ(Open(), std::nullopt);
  EXPECT_TRUE(movie_->ring().draw_frame().key);
}

TEST_F(MovieTest, PartialCompressedCodebookLoadsAtEstimatedOffset) {
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendChunk(fake_.data, "CBPZ", std::vector<uint8_t>(20, 0xAB));
  AppendFrameEnd(fake_.data);

  ASSERT_EQ(Open(), std::nullopt);
  // Groupsize 1: offset = codebook_capacity - (20 * 1 + 100).
  const LcwBuffer& codebook =
      movie_->ring().codebook(movie_->loader().full_codebook()).data;
  EXPECT_TRUE(codebook.compressed());
  EXPECT_EQ(base::At(codebook.data(), kCodebookCapacity - 121), 0);
  EXPECT_EQ(base::At(codebook.data(), kCodebookCapacity - 120), 0xAB);
}

TEST_F(MovieTest, FullCodebookDiscardsCollectedPieces) {
  // Groupsize 2: a piece of the next codebook, then a full codebook, in frame
  // 0. The two pieces in frames 1 and 2 must then make the next codebook from
  // its start, not after the discarded piece.
  options_.frame_buffers = 3;
  options_.codebook_buffers = 2;
  VqaHeader header = SmallHeader();
  header.frames_per_group = 2;
  fake_.data = MovieStart(header, {0, 0, 0});
  AppendChunk(fake_.data, "CBP0", std::vector<uint8_t>(20, 0xAA));
  AppendChunk(fake_.data, "CBF0", std::vector<uint8_t>(128));
  AppendFrameEnd(fake_.data);
  AppendChunk(fake_.data, "CBP0", std::vector<uint8_t>(20, 0xBB));
  AppendFrameEnd(fake_.data);
  AppendChunk(fake_.data, "CBP0", std::vector<uint8_t>(20, 0xCC));
  AppendFrameEnd(fake_.data);

  ASSERT_EQ(Open(), std::nullopt);
  const LcwBuffer& codebook =
      movie_->ring().codebook(movie_->loader().full_codebook()).data;
  ASSERT_EQ(codebook.size(), 40);
  EXPECT_EQ(base::At(codebook.contents(), 0), 0xBB);
  EXPECT_EQ(base::At(codebook.contents(), 20), 0xCC);
}

TEST_F(MovieTest, RejectsPartialCodebookWithNegativeOffset) {
  // 300 bytes fit the 376-byte codebook, but the estimated start
  // 376 - (300 * 1 + 100) = -24 lies before the buffer.
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendChunk(fake_.data, "CBPZ", std::vector<uint8_t>(300));
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), VqaError::kRead);
}

TEST_F(MovieTest, RejectsPartialCodebooksOverflowingTheEnd) {
  // Groupsize 2: the first 20-byte part sets the offset to
  // 376 - (20 * 2 + 100) = 236; a 200-byte second part would end at 456.
  VqaHeader header = SmallHeader();
  header.frames_per_group = 2;
  fake_.data = MovieStart(header, {0, 0, 0});
  AppendChunk(fake_.data, "CBPZ", std::vector<uint8_t>(20));
  AppendChunk(fake_.data, "CBPZ", std::vector<uint8_t>(200));
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), VqaError::kRead);
}

TEST_F(MovieTest, AcceptsFullPalette) {
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendChunk(fake_.data, "CPL0", std::vector<uint8_t>(768, 7));
  AppendFrameEnd(fake_.data);

  ASSERT_EQ(Open(), std::nullopt);
  const Frame& frame = movie_->ring().draw_frame();
  EXPECT_TRUE(frame.has_palette);
  EXPECT_EQ(frame.palette.size(), 768);
  EXPECT_EQ(base::At(frame.palette.contents(), 767), 7);
}

TEST_F(MovieTest, SkippedFramePaletteIsSetWithTheNextFrameDrawn) {
  // Frames 0 and 1 carry palettes of 7s and 9s, frame 2 none. With the clock
  // at frame 2 the drawer skips 0 and 1 and draws 2, which must set the
  // palette of frame 1, the last one skipped.
  options_.frame_buffers = 3;
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendChunk(fake_.data, "CPL0", std::vector<uint8_t>(768, 7));
  AppendFrameEnd(fake_.data);
  AppendChunk(fake_.data, "CPL0", std::vector<uint8_t>(768, 9));
  AppendFrameEnd(fake_.data);
  AppendFrameEnd(fake_.data);
  options_.skip_late_frames = true;
  ASSERT_EQ(Open(), std::nullopt);

  // 15 fps: tick 9 of 60 per second is frame 2, with 50 ms to spare before
  // frame 3 comes due.
  movie_->clock().Set(9, nullptr);
  ASSERT_EQ(movie_->drawer().DrawNextFrame(), DrawStatus::kDrawn);

  EXPECT_EQ(movie_->drawer().last_drawn_frame(), 2);
  EXPECT_EQ(client_.skipped, (std::vector<int>{0, 1}));
  ASSERT_EQ(client_.last_palette.size(), 768U);
  EXPECT_EQ(client_.last_palette.front(), 9);
}

TEST_F(MovieTest, AnAudioUnderrunLetsTheDrawerSkip) {
  options_.frame_buffers = 3;
  fake_.data = EmptyFrames(SmallSoundHeader());
  EnableAudio();
  ASSERT_EQ(Open(), std::nullopt);
  // 15 fps: tick 9 of 60 per second is frame 2.
  movie_->clock().Set(9, nullptr);

  // Frame 0 is late, but skipping is off.
  ASSERT_EQ(movie_->drawer().DrawNextFrame(), DrawStatus::kDrawn);
  EXPECT_EQ(movie_->drawer().last_drawn_frame(), 0);

  // The sound runs dry: nothing follows the block playing.
  movie_->audio()->Advance();
  ASSERT_TRUE(movie_->audio()->underran());

  // Now frame 1 is skipped for frame 2, the one due.
  ASSERT_EQ(movie_->drawer().DrawNextFrame(), DrawStatus::kDrawn);
  EXPECT_EQ(movie_->drawer().last_drawn_frame(), 2);
}

TEST_F(MovieTest, RejectsUncompressedPaletteOver256Colors) {
  // 800 bytes fit the frame's 1792-byte palette buffer, but not the 768-byte
  // drawer palette the first palette is copied into.
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendChunk(fake_.data, "CPL0", std::vector<uint8_t>(800));
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), VqaError::kRead);
}

// Each chunk type is 2000 bytes, larger than its destination buffer.
class VqaOversizedChunkTest : public MovieTest,
                              public testing::WithParamInterface<const char*> {
};

TEST_P(VqaOversizedChunkTest, RejectsChunkLargerThanItsBuffer) {
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendChunk(fake_.data, GetParam(), std::vector<uint8_t>(2000));
  // Without the bounds check the frame would load, so Open() would succeed.
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), VqaError::kRead);
}

INSTANTIATE_TEST_SUITE_P(AllBufferedChunks, VqaOversizedChunkTest,
                         testing::Values("CBF0", "CBFZ", "CBP0", "CBPZ", "CPL0",
                                         "CPLZ", "VPT0", "VPTZ"));

TEST_F(MovieTest, OpensMovieShorterThanFrameBuffers) {
  options_.frame_buffers = 3;
  VqaHeader header = SmallHeader();
  header.frame_count = 1;
  fake_.data = MovieStart(header, {0});
  AppendFrameEnd(fake_.data);

  ASSERT_EQ(Open(), std::nullopt);
  EXPECT_EQ(movie_->loader().next_frame_number(), 1);
}

TEST_F(MovieTest, TruncatedMovieStillFailsToOpen) {
  // The header promises three frames but the file holds one.
  options_.frame_buffers = 3;
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), VqaError::kRead);
}

TEST_F(MovieTest, RejectsAudioWithZeroBlockBytes) {
  VqaHeader header = SmallHeader();
  header.flags = kVqaHasAudio;
  fake_.data = MovieStart(header, {0, 0, 0});
  AppendFrameEnd(fake_.data);
  EnableAudio();
  options_.audio_block_bytes = 0;

  EXPECT_EQ(Open(), VqaError::kAudio);
  EXPECT_EQ(fake_.closes, 1);
}

TEST_F(MovieTest, PlaysSilentlyWithoutAnAudioRing) {
  VqaHeader header = SmallSoundHeader();
  header.frame_count = 1;
  fake_.data = EmptyFrames(header);
  EnableAudio();
  options_.audio_ring_bytes = 0;
  ASSERT_EQ(Open(), std::nullopt);
  // No sound path runs: there is nothing to play the sound with.
  EXPECT_EQ(movie_->audio(), nullptr);
  EXPECT_EQ(movie_->audio_output(), nullptr);

  PlayToTheEnd();
  EXPECT_EQ(client_.shown, std::vector<int>{0});
  EXPECT_FALSE(device_.attached());
}

TEST_F(MovieTest, WalkKeepsTheSoundPlayingUntilTheEnd) {
  fake_.data = MovieStart(SmallSoundHeader(), {0, 0, 0});
  // More than one frame's staging buffer, so it loads straight into the
  // audio ring as its first two blocks and the sound starts with the movie.
  AppendChunk(fake_.data, "SND0", std::vector<uint8_t>(4096, 0x80));
  AppendFrameEnd(fake_.data);
  AppendFrameEnd(fake_.data);
  AppendFrameEnd(fake_.data);
  EnableAudio();
  ASSERT_EQ(Open(), std::nullopt);

  // The sound, and the clock that follows it, must keep running between
  // steps. The device plays nothing here, so the clock stands at frame 0.
  EXPECT_EQ(movie_->Step(), VqaStepResult::kFrameShown);
  EXPECT_TRUE(movie_->audio_output()->playing());
  EXPECT_EQ(movie_->Step(), VqaStepResult::kWaiting);
  EXPECT_TRUE(movie_->audio_output()->playing());
  EXPECT_TRUE(device_.attached());

  // The step that reaches the end stops it.
  movie_->clock().Set(1'000'000, nullptr);
  VqaStepResult result = VqaStepResult::kWaiting;
  for (int i = 0; i < 10 && result != VqaStepResult::kEnded; ++i) {
    result = movie_->Step();
  }
  EXPECT_EQ(result, VqaStepResult::kEnded);
  EXPECT_FALSE(movie_->audio_output()->playing());
  EXPECT_FALSE(device_.attached());
}

TEST_F(MovieTest, FailsToOpenWhenTheSoundCannotBeConverted) {
  fake_.data = EmptyFrames(SmallSoundHeader());
  EnableAudio();
  // Not an SDL sample format, so SDL cannot convert to it.
  device_.audio_spec.format = 0;

  EXPECT_EQ(Open(), VqaError::kAudio);
  EXPECT_FALSE(device_.attached());
}

TEST_F(MovieTest, PlaysSilentlyWithoutADevice) {
  fake_.data = EmptyFrames(SmallSoundHeader());

  ASSERT_EQ(Open(), std::nullopt);
  EXPECT_EQ(movie_->audio(), nullptr);
}

TEST_F(MovieTest, RingOfPartBlocksWrapsAfterTheLastWholeBlock) {
  fake_.data = MovieStart(SmallSoundHeader(), {0, 0, 0});
  // Loads straight into the ring as its two whole blocks.
  AppendChunk(fake_.data, "SND0", std::vector<uint8_t>(4096, 0x80));
  AppendFrameEnd(fake_.data);
  AppendFrameEnd(fake_.data);
  AppendFrameEnd(fake_.data);
  EnableAudio();
  // Two 2048-byte blocks and 904 bytes that are not one.
  options_.audio_ring_bytes = 5000;
  ASSERT_EQ(Open(), std::nullopt);
  const AudioRing& audio = *movie_->audio();
  EXPECT_EQ(audio.block_count(), 2);
  EXPECT_EQ(audio.capacity(), 2 * 2048);
  ASSERT_TRUE(movie_->audio_output()->Start());

  // The device pulls the preloaded blocks through the mixer.
  std::array<std::byte, 256> buffer{};
  for (int i = 0; i < 1000 && audio.play_block() != 1; ++i) {
    device_.Pump(buffer);
  }
  EXPECT_EQ(audio.play_block(), 1);
}

TEST_F(MovieTest, FirstSoundChunkFillingTheRingWrapsTheWriteOffset) {
  fake_.data = MovieStart(SmallSoundHeader(), {0, 0, 0});
  // Larger than staging, so it preloads the ring, and exactly as large.
  AppendChunk(fake_.data, "SND0", std::vector<uint8_t>(4096, 0x80));
  AppendFrameEnd(fake_.data);
  AppendFrameEnd(fake_.data);
  AppendFrameEnd(fake_.data);
  EnableAudio();
  options_.audio_ring_bytes = 2 * 2048;
  ASSERT_EQ(Open(), std::nullopt);
  const AudioRing& audio = *movie_->audio();
  EXPECT_EQ(audio.write_offset(), 0);
  EXPECT_TRUE(audio.block_loaded(0));
  EXPECT_TRUE(audio.block_loaded(1));
}

TEST_F(MovieTest, ShowsEveryFrameInOrder) {
  fake_.data = EmptyFrames(SmallHeader());
  ASSERT_EQ(Open(), std::nullopt);

  PlayToTheEnd();
  EXPECT_EQ(client_.shown, (std::vector<int>{0, 1, 2}));
  EXPECT_EQ(movie_->last_frame_shown(), 2);
  EXPECT_EQ(movie_->Step(), VqaStepResult::kEnded);
}

TEST_F(MovieTest, RejectsNegativeBufferCounts) {
  fake_.data = EmptyFrames(SmallHeader());
  options_.frame_buffers = -1;
  EXPECT_EQ(Open(), VqaError::kNoMemory);

  options_.frame_buffers = 1;
  options_.codebook_buffers = -1;
  EXPECT_EQ(Open(), VqaError::kNoMemory);
}

TEST_F(MovieTest, AClientThatStopsEndsTheMovie) {
  fake_.data = EmptyFrames(SmallHeader());
  client_.stop_after = 1;
  ASSERT_EQ(Open(), std::nullopt);

  movie_->Run();
  EXPECT_EQ(client_.shown, std::vector<int>{0});
  EXPECT_EQ(movie_->loader().next_frame_number(), 1);
  EXPECT_EQ(movie_->Step(), VqaStepResult::kEnded);
}

TEST_F(MovieTest, ClosesTheFileWhenDestroyed) {
  fake_.data = EmptyFrames(SmallHeader());
  ASSERT_EQ(Open(), std::nullopt);
  EXPECT_EQ(fake_.closes, 0);

  movie_.reset();
  EXPECT_EQ(fake_.closes, 1);
}

}  // namespace
