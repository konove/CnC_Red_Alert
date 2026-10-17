// Tests for Gadget: enabling and disabling, moving, the base peer hook, the
// region a click has to fall in, and what a destroyed gadget lets go of.

#include "game/ui/gadget.h"

#include <cstdint>
#include <optional>
#include <utility>

#include "base/installed.h"
#include "base/rect.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/keyboard.h"
#include "game/ui/control.h"
#include "game/ui/gadget_list.h"
#include "game/ui/input.h"
#include "gtest/gtest.h"

namespace game {
namespace {

using engine::window::KeyNumber;
using engine::window::MouseButton;

// A gadget on the 10x10 square at (20, 20) that takes left presses and counts
// its actions and draws.
class Probe : public Gadget {
 public:
  explicit Probe(const bool sticky = false)
      : Gadget(base::Rect{20, 20, 10, 10}, kLeftPress, sticky) {}

  bool Draw(PixelView& view, const bool forced) override {
    if (!Gadget::Draw(view, forced)) {
      return false;
    }
    ++draws;
    return true;
  }

  [[nodiscard]] bool IsDisabled() const { return disabled(); }

  int actions = 0;
  int draws = 0;

 protected:
  bool Action(const uint32_t flags, KeyNumber& key, const int x,
              const int y) override {
    ++actions;
    return Gadget::Action(flags, key, x, y);
  }
};

class GadgetTest : public testing::Test {
 protected:
  // Queues a left press at (`x`, `y`) and runs the list over it.
  engine::window::InputEvent PressAt(const int x, const int y) {
    EXPECT_TRUE(input_.keyboard().PutClick(MouseButton::kLeft, false, x, y));
    return list_.Input(view_);
  }

  Input input_{48, 48};
  base::Installed<Input>::Scope input_scope_{input_};
  PixelView view_;
  Probe probe_;
  GadgetList list_;
};

TEST_F(GadgetTest, APressInsideTheRegionIsActedOn) {
  list_.Add(probe_);
  PressAt(20, 29);
  PressAt(29, 20);
  EXPECT_EQ(probe_.actions, 2);
}

TEST_F(GadgetTest, ThePixelsJustOutsideEachEdgeAreIgnored) {
  list_.Add(probe_);
  PressAt(30, 25);
  PressAt(25, 30);
  PressAt(19, 25);
  PressAt(25, 19);
  EXPECT_EQ(probe_.actions, 0);
}

TEST_F(GadgetTest, ADisabledGadgetIsDrawnButNotOfferedInput) {
  list_.Add(probe_);
  probe_.Disable();
  EXPECT_TRUE(probe_.IsDisabled());
  EXPECT_TRUE(probe_.IsToRedraw());

  const engine::window::InputEvent event = PressAt(25, 25);
  EXPECT_EQ(probe_.actions, 0);
  EXPECT_EQ(probe_.draws, 1);
  // The click comes back untaken.
  EXPECT_EQ(event.button, MouseButton::kLeft);
}

TEST_F(GadgetTest, EnablingAgainLetsTheGadgetAct) {
  list_.Add(probe_);
  probe_.Disable();
  PressAt(25, 25);
  probe_.Enable();
  EXPECT_FALSE(probe_.IsDisabled());
  EXPECT_TRUE(probe_.IsToRedraw());

  PressAt(25, 25);
  EXPECT_EQ(probe_.actions, 1);
}

TEST_F(GadgetTest, DisablingAndEnablingDropTheKeyboardFocus) {
  probe_.SetFocus();
  probe_.Disable();
  EXPECT_FALSE(probe_.HasFocus());

  probe_.SetFocus();
  probe_.Enable();
  EXPECT_FALSE(probe_.HasFocus());
}

TEST_F(GadgetTest, SetPositionMovesTheRegion) {
  list_.Add(probe_);
  probe_.SetPosition(0, 5);
  EXPECT_EQ(probe_.x(), 0);
  EXPECT_EQ(probe_.y(), 5);
  EXPECT_EQ(probe_.width(), 10);
  EXPECT_EQ(probe_.height(), 10);

  PressAt(25, 25);
  EXPECT_EQ(probe_.actions, 0);
  PressAt(5, 10);
  EXPECT_EQ(probe_.actions, 1);
}

TEST_F(GadgetTest, TheBasePeerHookLeavesTheKeyAlone) {
  ControlClass peer(7, 0, 0, 1, 1);
  KeyNumber key = engine::window::ButtonKey(7);
  probe_.OnPeerAction(Gadget::kLeftPress, key, peer);
  EXPECT_EQ(key, engine::window::ButtonKey(7));
}

TEST_F(GadgetTest, AGadgetDestroyedWhileItHoldsTheMouseLetsGoOfIt) {
  std::optional<Probe> sticky(std::in_place, /*sticky=*/true);
  list_.Add(*sticky);
  PressAt(25, 25);
  ASSERT_EQ(Gadget::mouse_captor(), &*sticky);

  sticky.reset();
  EXPECT_EQ(Gadget::mouse_captor(), nullptr);
}

TEST_F(GadgetTest, AStickyGadgetListensForTheLeftPressAndRelease) {
  // Constructed with no events at all, it still has to see both ends of the
  // click to capture and release the mouse.
  class Bare : public Gadget {
   public:
    Bare() : Gadget(base::Rect{20, 20, 10, 10}, 0, /*sticky=*/true) {}
  } bare;
  list_.Add(bare);
  PressAt(25, 25);
  EXPECT_EQ(Gadget::mouse_captor(), &bare);

  ASSERT_TRUE(input_.keyboard().PutClick(MouseButton::kLeft, true, 25, 25));
  list_.Input(view_);
  EXPECT_EQ(Gadget::mouse_captor(), nullptr);
}

}  // namespace
}  // namespace game
