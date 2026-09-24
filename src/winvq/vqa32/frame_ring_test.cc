#include "winvq/vqa32/frame_ring.h"

#include "gtest/gtest.h"

namespace {

TEST(FrameRingTest, LoaderAndDrawerStartAtTheSameFrame) {
  FrameRing ring(3, 2, 16, 16, 16);

  EXPECT_EQ(&ring.load_frame(), &ring.draw_frame());
  EXPECT_FALSE(ring.load_frame().loaded);
}

TEST(FrameRingTest, HandsFramesFromTheLoaderToTheDrawerInOrder) {
  FrameRing ring(3, 1, 16, 16, 16);

  ring.load_frame().frame_number = 0;
  ring.FinishLoading();
  ring.load_frame().frame_number = 1;
  ring.FinishLoading();

  ASSERT_TRUE(ring.draw_frame().loaded);
  EXPECT_EQ(ring.draw_frame().frame_number, 0);
  ring.FinishDrawing();
  ASSERT_TRUE(ring.draw_frame().loaded);
  EXPECT_EQ(ring.draw_frame().frame_number, 1);
  ring.FinishDrawing();
  // The loader has not got this far.
  EXPECT_FALSE(ring.draw_frame().loaded);
}

TEST(FrameRingTest, TheLoaderWaitsForAFullRingToDrain) {
  FrameRing ring(2, 1, 16, 16, 16);

  ring.FinishLoading();
  ring.FinishLoading();
  // Back at the first frame, which the drawer still holds.
  EXPECT_TRUE(ring.load_frame().loaded);

  ring.FinishDrawing();
  EXPECT_FALSE(ring.load_frame().loaded);
}

TEST(FrameRingTest, DrawingFreesTheFrameForReuse) {
  FrameRing ring(1, 1, 16, 16, 16);
  Frame& frame = ring.load_frame();
  frame.key = true;
  frame.has_palette = true;
  ring.FinishLoading();

  ring.FinishDrawing();
  EXPECT_FALSE(frame.loaded);
  EXPECT_FALSE(frame.key);
  EXPECT_FALSE(frame.has_palette);
}

TEST(FrameRingTest, CodebooksWrapAround) {
  FrameRing ring(1, 3, 16, 16, 16);

  EXPECT_EQ(ring.next_codebook(0), 1);
  EXPECT_EQ(ring.next_codebook(2), 0);
  EXPECT_EQ(ring.codebook(2).data.capacity(), 16);
}

}  // namespace
