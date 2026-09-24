// Tests for the VQA player's public interface: VqaPlayer, VqaOptions and the
// errors Open() reports. Movies are small synthetic files served by a
// scripted in-memory VqaIo; movie_test.cc covers the parts inside.

#include "winvq/vqa32/vqa_player.h"

#include <expected>
#include <optional>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "winvq/vqa32/vqa_format.h"
#include "winvq/vqa32/vqa_test_util.h"

namespace {

class VqaPlayerTest : public testing::Test {
 protected:
  // Opens the movie in fake_ without sound.
  std::expected<VqaPlayer, VqaError> Open() {
    return VqaPlayer::Open(fake_, "test.vqa", client_, nullptr);
  }

  FakeVqaIo fake_;
  RecordingClient client_;
};

TEST(VqaOptionsTest, DefaultsSuitTheGames) {
  const VqaOptions options;

  EXPECT_FALSE(options.skip_late_frames);
  EXPECT_EQ(options.frame_buffers, 6);
  EXPECT_EQ(options.codebook_buffers, 3);
  EXPECT_EQ(options.audio_ring_bytes, std::nullopt);
  EXPECT_EQ(options.audio_block_bytes, 2048);
}

TEST_F(VqaPlayerTest, ReportsAFileThatCannotBeOpened) {
  fake_.fail_open = true;

  EXPECT_EQ(Open().error(), VqaError::kOpen);
  EXPECT_EQ(fake_.opens, 1);
  // The file never opened, so the player must not try to close it.
  EXPECT_EQ(fake_.closes, 0);
}

TEST_F(VqaPlayerTest, ReportsAReadErrorAndClosesAnEmptyFile) {
  // No data at all: the first 8-byte header read fails.
  EXPECT_EQ(Open().error(), VqaError::kRead);
  EXPECT_EQ(fake_.closes, 1);
}

TEST_F(VqaPlayerTest, RejectsANonIffFile) {
  AppendBytes(fake_.data, "XXXX");
  AppendBigEndian32(fake_.data, 0x1234);
  AppendBytes(fake_.data, "WVQA");

  EXPECT_EQ(Open().error(), VqaError::kNotVqa);
  EXPECT_EQ(fake_.closes, 1);
}

TEST_F(VqaPlayerTest, RejectsAFormOfZeroSize) {
  AppendBytes(fake_.data, "FORM");
  AppendBigEndian32(fake_.data, 0);
  AppendBytes(fake_.data, "WVQA");

  EXPECT_EQ(Open().error(), VqaError::kNotVqa);
}

TEST_F(VqaPlayerTest, RejectsAFormThatIsNotWvqa) {
  AppendBytes(fake_.data, "FORM");
  AppendBigEndian32(fake_.data, 0x1234);
  AppendBytes(fake_.data, "XXXX");

  EXPECT_EQ(Open().error(), VqaError::kNotVqa);
}

TEST_F(VqaPlayerTest, ReportsAReadErrorWhenTruncatedAfterThePreamble) {
  fake_.data = ValidPreamble();

  EXPECT_EQ(Open().error(), VqaError::kRead);
}

TEST_F(VqaPlayerTest, RejectsAHeaderChunkOfTheWrongSize) {
  fake_.data = ValidPreamble();
  AppendBytes(fake_.data, "VQHD");
  AppendBigEndian32(fake_.data, 4);  // Real VQA headers are much larger.
  AppendBytes(fake_.data, "XXXX");

  EXPECT_EQ(Open().error(), VqaError::kNotVqa);
}

TEST_F(VqaPlayerTest, EveryFailedOpenClosesTheFile) {
  ASSERT_EQ(Open().error(), VqaError::kRead);
  fake_.data = ValidPreamble();
  ASSERT_EQ(Open().error(), VqaError::kRead);

  EXPECT_EQ(fake_.opens, 2);
  EXPECT_EQ(fake_.closes, 2);
}

TEST_F(VqaPlayerTest, PlaysAMovieToTheEnd) {
  // 60 fps, so three frames take 50 ms of the system clock.
  VqaHeader header = SmallHeader();
  header.fps = 60;
  fake_.data = EmptyFrames(header);

  auto player = Open();
  ASSERT_TRUE(player.has_value());
  player->Run();

  EXPECT_EQ(client_.shown, (std::vector<int>{0, 1, 2}));
  EXPECT_EQ(client_.width, 8);
  EXPECT_EQ(client_.height, 8);
  EXPECT_EQ(player->last_frame_shown(), 2);
  EXPECT_EQ(player->Step(), VqaStepResult::kEnded);
}

TEST_F(VqaPlayerTest, AMovedPlayerKeepsTheMovieOpen) {
  fake_.data = EmptyFrames(SmallHeader());
  auto opened = Open();
  ASSERT_TRUE(opened.has_value());

  VqaPlayer player = std::move(*opened);
  EXPECT_EQ(fake_.closes, 0);
  EXPECT_EQ(player.Step(), VqaStepResult::kFrameShown);
}

TEST_F(VqaPlayerTest, DestroyingThePlayerClosesTheFile) {
  fake_.data = EmptyFrames(SmallHeader());
  {
    const auto player = Open();
    ASSERT_TRUE(player.has_value());
  }
  EXPECT_EQ(fake_.closes, 1);
}

TEST_F(VqaPlayerTest, ReportsTheMovieLength) {
  fake_.data = EmptyFrames(SmallHeader());
  const auto player = Open();
  ASSERT_TRUE(player.has_value());

  EXPECT_EQ(player->frame_count(), 3);
  EXPECT_EQ(player->frame_rate(), 15);
}

TEST_F(VqaPlayerTest, APausedPlayerShowsNothingUntilResumed) {
  fake_.data = EmptyFrames(SmallHeader());
  auto player = Open();
  ASSERT_TRUE(player.has_value());

  player->Pause();
  EXPECT_TRUE(player->paused());
  EXPECT_EQ(player->Step(), VqaStepResult::kWaiting);
  EXPECT_TRUE(client_.shown.empty());

  player->Resume();
  EXPECT_EQ(player->Step(), VqaStepResult::kFrameShown);
}

}  // namespace
