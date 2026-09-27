// Tests for HardwareCursor's hide count, ShownScope and recorded position.

#include "engine/window/hardware_cursor.h"

#include "gtest/gtest.h"

namespace engine::window {
namespace {

// A cursor as large as the games ask for.
constexpr int kCursorSize = 48;

TEST(HardwareCursorTest, ShowUndoesOneHideAndStopsAtZero) {
  HardwareCursor mouse(kCursorSize, kCursorSize);

  mouse.Hide();
  mouse.Hide();
  mouse.Show();
  EXPECT_EQ(mouse.hide_count(), 1);

  mouse.Show();
  mouse.Show();
  EXPECT_EQ(mouse.hide_count(), 0);
}

TEST(HardwareCursorTest, ForceShowClearsEveryHide) {
  HardwareCursor mouse(kCursorSize, kCursorSize);
  mouse.Hide();
  mouse.Hide();
  mouse.Hide();

  mouse.ForceShow();
  EXPECT_EQ(mouse.hide_count(), 0);

  mouse.ForceShow();
  EXPECT_EQ(mouse.hide_count(), 0);
}

TEST(HardwareCursorTest, ShownScopeShowsAndRestoresTheHideCount) {
  HardwareCursor mouse(kCursorSize, kCursorSize);
  mouse.Hide();
  mouse.Hide();

  {
    const HardwareCursor::ShownScope shown(mouse);
    EXPECT_EQ(mouse.hide_count(), 0);
  }

  EXPECT_EQ(mouse.hide_count(), 2);
}

TEST(HardwareCursorTest, ShownScopeUndoesUnmatchedCallsInside) {
  HardwareCursor mouse(kCursorSize, kCursorSize);
  mouse.Hide();

  {
    const HardwareCursor::ShownScope shown(mouse);
    mouse.Hide();
    mouse.Hide();
  }
  EXPECT_EQ(mouse.hide_count(), 1);

  {
    const HardwareCursor::ShownScope shown(mouse);
    mouse.ForceShow();
  }
  EXPECT_EQ(mouse.hide_count(), 1);
}

TEST(HardwareCursorTest, RecordPositionSetsXAndY) {
  HardwareCursor mouse(kCursorSize, kCursorSize);

  mouse.RecordPosition(10, 20);

  EXPECT_EQ(mouse.x(), 10);
  EXPECT_EQ(mouse.y(), 20);
}

}  // namespace
}  // namespace engine::window
