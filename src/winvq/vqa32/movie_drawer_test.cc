#include "winvq/vqa32/movie_drawer.h"

#include <vector>

#include "gtest/gtest.h"
#include "winvq/vqa32/frame_ring.h"
#include "winvq/vqa32/movie_clock.h"
#include "winvq/vqa32/vqa_test_util.h"

namespace {

// A drawer for the 8x8, 15 fps SmallHeader() movie over a ring of 3 frames.
class MovieDrawerTest : public testing::Test {
 protected:
  // Loads frames 0..count-1 into the ring.
  void LoadFrames(int count) {
    for (int i = 0; i < count; ++i) {
      ring_.load_frame().frame_number = i;
      ring_.FinishLoading();
    }
  }

  FrameRing ring_{3, 1, 16, 16, 16};
  MovieClock clock_;
  RecordingClient client_;
};

TEST_F(MovieDrawerTest, WaitsForTheLoader) {
  MovieDrawer drawer(ring_, clock_, nullptr, SmallHeader(), client_, false);
  clock_.Set(0, nullptr);

  EXPECT_EQ(drawer.DrawNextFrame(), DrawStatus::kNoFrame);
  EXPECT_TRUE(client_.shown.empty());
}

TEST_F(MovieDrawerTest, WaitsUntilAFrameIsDue) {
  MovieDrawer drawer(ring_, clock_, nullptr, SmallHeader(), client_, false);
  LoadFrames(2);
  // Tick 0 is frame 0; frame 1 is due from tick 4 (60 / 15).
  clock_.Set(0, nullptr);

  EXPECT_EQ(drawer.DrawNextFrame(), DrawStatus::kDrawn);
  EXPECT_EQ(drawer.DrawNextFrame(), DrawStatus::kNotTime);
  EXPECT_EQ(client_.shown, std::vector<int>{0});
}

TEST_F(MovieDrawerTest, ShowsTheMovieSizedFrame) {
  MovieDrawer drawer(ring_, clock_, nullptr, SmallHeader(), client_, false);
  LoadFrames(1);
  clock_.Set(0, nullptr);

  ASSERT_EQ(drawer.DrawNextFrame(), DrawStatus::kDrawn);
  EXPECT_EQ(client_.width, 8);
  EXPECT_EQ(client_.height, 8);
  // The frame carries no palette.
  EXPECT_TRUE(client_.last_palette.empty());
}

TEST_F(MovieDrawerTest, DrawsLateFramesWhenSkippingIsOff) {
  MovieDrawer drawer(ring_, clock_, nullptr, SmallHeader(), client_, false);
  LoadFrames(3);
  // Tick 9 is frame 2.
  clock_.Set(9, nullptr);

  EXPECT_EQ(drawer.DrawNextFrame(), DrawStatus::kDrawn);
  EXPECT_EQ(drawer.DrawNextFrame(), DrawStatus::kDrawn);
  EXPECT_EQ(client_.shown, (std::vector<int>{0, 1}));
  EXPECT_TRUE(client_.skipped.empty());
}

TEST_F(MovieDrawerTest, SkipsLateFramesUnlessAKeyFrame) {
  MovieDrawer drawer(ring_, clock_, nullptr, SmallHeader(), client_, true);
  LoadFrames(3);
  ring_.draw_frame().key = true;
  // Tick 9 is frame 2.
  clock_.Set(9, nullptr);

  // Frame 0 is a key frame and is drawn however late; frame 1 is skipped.
  EXPECT_EQ(drawer.DrawNextFrame(), DrawStatus::kDrawn);
  EXPECT_EQ(drawer.DrawNextFrame(), DrawStatus::kDrawn);
  EXPECT_EQ(client_.shown, (std::vector<int>{0, 2}));
  EXPECT_EQ(client_.skipped, std::vector<int>{1});
  EXPECT_EQ(drawer.last_drawn_frame(), 2);
}

TEST_F(MovieDrawerTest, AClientThatStopsStopsTheDrawer) {
  MovieDrawer drawer(ring_, clock_, nullptr, SmallHeader(), client_, false);
  LoadFrames(1);
  clock_.Set(0, nullptr);
  client_.stop_after = 1;

  EXPECT_EQ(drawer.DrawNextFrame(), DrawStatus::kStopped);
  // The frame went back to the loader all the same.
  EXPECT_FALSE(ring_.load_frame().loaded);
}

}  // namespace
