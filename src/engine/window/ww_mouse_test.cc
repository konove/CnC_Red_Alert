// Tests for the free mouse functions: which WWMouseClass they reach.

#include "engine/window/ww_mouse.h"

#include <memory>

#include "base/installed.h"
#include "engine/window/display.h"
#include "gtest/gtest.h"

namespace {

// A cursor as large as the games ask for.
constexpr int kCursorSize = 48;

// Each test gets a Display without a window: the mouse grabs and releases the
// mouse through it, and SDL ignores a grab of no window.
class WWMouseTest : public testing::Test {
 protected:
  engine::window::Display display_;
  base::Installed<engine::window::Display>::Scope display_scope_{display_};
};

TEST_F(WWMouseTest, FreeFunctionsForgetADestroyedMouse) {
  auto mouse =
      std::make_unique<WWMouseClass>(nullptr, kCursorSize, kCursorSize);
  Hide_Mouse();
  Update_Mouse_Pos(10, 20);
  ASSERT_EQ(Get_Mouse_State(), 1);
  ASSERT_EQ(Get_Mouse_X(), 10);

  mouse.reset();

  EXPECT_EQ(Get_Mouse_State(), 0);
  EXPECT_EQ(Get_Mouse_X(), 0);
  EXPECT_EQ(Get_Mouse_Y(), 0);
}

// Input::InstallMouse() replaces a mouse by building the new one before the
// old one goes, so the old one's destructor must leave the new one reachable.
TEST_F(WWMouseTest, ReplacingAMouseKeepsTheNewOne) {
  auto old_mouse =
      std::make_unique<WWMouseClass>(nullptr, kCursorSize, kCursorSize);
  auto new_mouse =
      std::make_unique<WWMouseClass>(nullptr, kCursorSize, kCursorSize);

  old_mouse.reset();
  Update_Mouse_Pos(30, 40);

  EXPECT_EQ(new_mouse->Get_Mouse_X(), 30);
  EXPECT_EQ(Get_Mouse_Y(), 40);
}

}  // namespace
