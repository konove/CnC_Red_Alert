// Tests for HardwareCursor's shapes, hide count, ShownScope and recorded
// position.

#include "engine/window/hardware_cursor.h"

#include <SDL_mouse.h>

#include <array>
#include <cstddef>

#include "gtest/gtest.h"

namespace engine::window {
namespace {

// A cursor as large as the games ask for.
constexpr int kCursorSize = 48;

// A 2x2 shape of color 1: the ShapeHeader (format 0, height 2, width 2,
// original height 2, 16 bytes in all, 4 once decompressed), then the pixels as
// one LCW literal run of 4 and the LCW end marker.
constexpr auto kShape = std::to_array<std::byte>(
    {std::byte{0}, std::byte{0}, std::byte{2}, std::byte{2}, std::byte{0},
     std::byte{2}, std::byte{16}, std::byte{0}, std::byte{4}, std::byte{0},
     std::byte{0x84}, std::byte{1}, std::byte{1}, std::byte{1}, std::byte{1},
     std::byte{0x80}});

TEST(HardwareCursorTest, SetShapeMakesTheShapeTheSdlCursor) {
  HardwareCursor mouse(kCursorSize, kCursorSize);
  const SDL_Cursor* const before = SDL_GetCursor();

  mouse.SetShape(1, 1, kShape);

  EXPECT_NE(SDL_GetCursor(), nullptr);
  EXPECT_NE(SDL_GetCursor(), before);
}

TEST(HardwareCursorTest, SetShapeKeepsTheCursorForAHotSpotOutsideTheShape) {
  HardwareCursor mouse(kCursorSize, kCursorSize);
  const auto other_shape = kShape;
  mouse.SetShape(0, 0, kShape);
  const SDL_Cursor* const first = SDL_GetCursor();
  ASSERT_NE(first, nullptr);

  mouse.SetShape(2, 0, other_shape);
  EXPECT_EQ(SDL_GetCursor(), first);
  mouse.SetShape(0, 2, other_shape);
  EXPECT_EQ(SDL_GetCursor(), first);
  mouse.SetShape(-1, 0, other_shape);
  EXPECT_EQ(SDL_GetCursor(), first);
  mouse.SetShape(0, -1, other_shape);
  EXPECT_EQ(SDL_GetCursor(), first);
}

TEST(HardwareCursorTest, SetShapeTriesARejectedShapeAgain) {
  HardwareCursor mouse(kCursorSize, kCursorSize);
  const auto other_shape = kShape;
  mouse.SetShape(0, 0, kShape);
  const SDL_Cursor* const first = SDL_GetCursor();

  mouse.SetShape(2, 2, other_shape);
  mouse.SetShape(1, 1, other_shape);

  EXPECT_NE(SDL_GetCursor(), nullptr);
  EXPECT_NE(SDL_GetCursor(), first);
}

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

TEST(HardwareCursorTest, UpdateScaleWithoutAShapeDoesNothing) {
  HardwareCursor mouse(kCursorSize, kCursorSize);

  mouse.UpdateScale();

  EXPECT_EQ(mouse.hide_count(), 0);
}

TEST(HardwareCursorTest, RecordPositionSetsXAndY) {
  HardwareCursor mouse(kCursorSize, kCursorSize);

  mouse.RecordPosition(10, 20);

  EXPECT_EQ(mouse.x(), 10);
  EXPECT_EQ(mouse.y(), 20);
}

}  // namespace
}  // namespace engine::window
