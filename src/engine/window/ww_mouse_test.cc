// Tests for the free mouse functions: which HardwareCursor they reach.

#include "engine/window/ww_mouse.h"

#include <memory>

#include "base/installed.h"
#include "engine/window/display.h"
#include "gtest/gtest.h"

namespace engine::window {
namespace {

// A cursor as large as the games ask for.
constexpr int kCursorSize = 48;

// Each test gets a Display without a window: the mouse grabs and releases the
// mouse through it, and SDL ignores a grab of no window.
class HardwareCursorTest : public testing::Test {
 protected:
  Display display_;
  base::Installed<Display>::Scope display_scope_{display_};
};

TEST_F(HardwareCursorTest, FreeFunctionsForgetADestroyedMouse) {
  auto mouse = std::make_unique<HardwareCursor>(kCursorSize, kCursorSize);
  HideMouse();
  RecordMousePosition(10, 20);
  ASSERT_EQ(MouseHideCount(), 1);
  ASSERT_EQ(MouseX(), 10);

  mouse.reset();

  EXPECT_EQ(MouseHideCount(), 0);
  EXPECT_EQ(MouseX(), 0);
  EXPECT_EQ(MouseY(), 0);
}

// Input::InstallMouse() replaces a mouse by building the new one before the
// old one goes, so the old one's destructor must leave the new one reachable.
TEST_F(HardwareCursorTest, ReplacingAMouseKeepsTheNewOne) {
  auto old_mouse = std::make_unique<HardwareCursor>(kCursorSize, kCursorSize);
  auto new_mouse = std::make_unique<HardwareCursor>(kCursorSize, kCursorSize);

  old_mouse.reset();
  RecordMousePosition(30, 40);

  EXPECT_EQ(new_mouse->x(), 30);
  EXPECT_EQ(MouseY(), 40);
}

}  // namespace
}  // namespace engine::window
