#include "winvq/vqa32/movie_drawer.h"

#include "gtest/gtest.h"
#include "winvq/vqa32/frame_ring.h"
#include "winvq/vqa32/movie_clock.h"
#include "winvq/vqa32/vqa_player.h"
#include "winvq/vqa32/vqa_test_util.h"

namespace {

// A drawer for the 8x8, 15 fps SmallHeader() movie over a ring of 3 frames.
class MovieDrawerPacingTest : public testing::Test {
 protected:
  MovieDrawerPacingTest() {
    SetVqaConfigDefaults(&config_);
    config_.frame_rate = 15;
    config_.option_flags = 0;
  }

  // Loads frames 0..count-1 into the ring.
  void LoadFrames(int count) {
    for (int i = 0; i < count; ++i) {
      ring_.load_frame().frame_number = i;
      ring_.FinishLoading();
    }
  }

  FrameRing ring_{3, 1, 16, 16, 16};
  MovieClock clock_;
  VqaConfig config_{};
};

TEST_F(MovieDrawerPacingTest, WaitsForTheLoader) {
  MovieDrawer drawer(ring_, clock_, nullptr, SmallHeader(), config_);
  clock_.Set(0, nullptr);

  EXPECT_EQ(drawer.DrawNextFrame(), DrawStatus::kNoFrame);
}

TEST_F(MovieDrawerPacingTest, WaitsUntilAFrameIsDue) {
  MovieDrawer drawer(ring_, clock_, nullptr, SmallHeader(), config_);
  LoadFrames(2);
  // Tick 0 is frame 0; frame 1 is due from tick 4 (60 / 15).
  clock_.Set(0, nullptr);

  EXPECT_EQ(drawer.DrawNextFrame(), DrawStatus::kDrawn);
  EXPECT_EQ(drawer.DrawNextFrame(), DrawStatus::kNotTime);
  EXPECT_EQ(drawer.last_drawn_frame(), 0);
}

TEST_F(MovieDrawerPacingTest, SkipsLateFramesUnlessAKeyFrame) {
  MovieDrawer drawer(ring_, clock_, nullptr, SmallHeader(), config_);
  LoadFrames(3);
  ring_.draw_frame().key = true;
  // Tick 9 is frame 2.
  clock_.Set(9, nullptr);

  // Frame 0 is a key frame and is drawn however late; frame 1 is skipped.
  EXPECT_EQ(drawer.DrawNextFrame(), DrawStatus::kDrawn);
  EXPECT_EQ(drawer.last_drawn_frame(), 0);
  EXPECT_EQ(drawer.DrawNextFrame(), DrawStatus::kDrawn);
  EXPECT_EQ(drawer.last_drawn_frame(), 2);
}

TEST_F(MovieDrawerPacingTest, StepDrawsEveryFrameWhateverTheClock) {
  config_.option_flags = kVqaOptionStep;
  MovieDrawer drawer(ring_, clock_, nullptr, SmallHeader(), config_);
  LoadFrames(2);
  clock_.Set(-600, nullptr);

  EXPECT_EQ(drawer.DrawNextFrame(), DrawStatus::kDrawn);
  EXPECT_EQ(drawer.DrawNextFrame(), DrawStatus::kDrawn);
  EXPECT_EQ(drawer.last_drawn_frame(), 1);
}

TEST_F(MovieDrawerPacingTest, DecodesIntoABufferOfItsOwnWithoutTheCallers) {
  config_.draw_flags = kVqaDrawToBuffer;
  config_.image_width = 8;
  config_.image_height = 8;

  const MovieDrawer drawer(ring_, clock_, nullptr, SmallHeader(), config_);
  EXPECT_EQ(drawer.image_buffer().size(), 8U * 8U);
}

TEST_F(MovieDrawerPacingTest, HasNoBufferWhenNotDecoding) {
  const MovieDrawer drawer(ring_, clock_, nullptr, SmallHeader(), config_);
  EXPECT_TRUE(drawer.image_buffer().empty());
}

}  // namespace
