#include "game/ui/gadget_list.h"

#include <functional>
#include <memory>

#include "base/installed.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/keyboard.h"
#include "game/ui/control.h"
#include "game/ui/gadget.h"
#include "game/ui/input.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace game {
namespace {

using engine::window::MouseButton;
using testing::ElementsAre;
using testing::IsEmpty;

// A gadget that counts what the list does with it. It takes a left press
// anywhere on its 10x10 square at `x`, 0 unless told to let it pass.
class Probe : public ControlClass {
 public:
  explicit Probe(unsigned id = 0, int x = 0)
      : ControlClass(id, x, 0, 10, 10, kLeftPress) {}

  bool Draw(PixelView& view, bool forced) override {
    if (!ControlClass::Draw(view, forced)) {
      return false;
    }
    ++draws;
    return true;
  }

  int actions = 0;
  int draws = 0;
  int added = 0;
  int removed = 0;
  bool takes_input = true;
  // Run in Action(), before it answers.
  std::function<void()> on_action;

 protected:
  bool Action(unsigned flags, engine::window::KeyNumber& key, int x,
              int y) override {
    ++actions;
    if (on_action) {
      on_action();
    }
    return takes_input && ControlClass::Action(flags, key, x, y);
  }
  void OnAdded(GadgetList& /*list*/) override { ++added; }
  void OnRemoved(GadgetList& /*list*/) override { ++removed; }
};

class GadgetListTest : public testing::Test {
 protected:
  // Queues a left press at `x`, 5.
  void Press(int x) {
    ASSERT_TRUE(input_.keyboard().PutClick(MouseButton::kLeft, false, x, 5));
  }

  Input input_{48, 48};
  base::Installed<Input>::Scope input_scope_{input_};
  PixelView view_;
  // Declared before the list in some tests and after it in others; these are
  // destroyed after `list_`.
  Probe a_{1, 0};
  Probe b_{2, 20};
  Probe c_{3, 40};
  GadgetList list_;
};

TEST_F(GadgetListTest, StartsEmpty) {
  EXPECT_TRUE(list_.empty());
  EXPECT_THAT(list_.gadgets(), IsEmpty());
  EXPECT_EQ(a_.list(), nullptr);
}

TEST_F(GadgetListTest, AddAppendsAndTellsTheGadget) {
  list_.Add(a_);
  list_.Add(b_);
  EXPECT_THAT(list_.gadgets(), ElementsAre(&a_, &b_));
  EXPECT_EQ(a_.list(), &list_);
  EXPECT_EQ(a_.added, 1);
  EXPECT_FALSE(list_.empty());
}

TEST_F(GadgetListTest, AddFrontPutsTheGadgetFirst) {
  list_.Add(a_);
  list_.AddFront(b_);
  EXPECT_THAT(list_.gadgets(), ElementsAre(&b_, &a_));
}

TEST_F(GadgetListTest, AddAfterPutsTheGadgetBehindItsPredecessor) {
  list_.Add(a_);
  list_.Add(b_);
  list_.AddAfter(c_, a_);
  EXPECT_THAT(list_.gadgets(), ElementsAre(&a_, &c_, &b_));
}

TEST_F(GadgetListTest, AddAfterMovesAGadgetThatStoodInFront) {
  list_.Add(a_);
  list_.Add(b_);
  list_.Add(c_);
  list_.AddAfter(a_, b_);
  EXPECT_THAT(list_.gadgets(), ElementsAre(&b_, &a_, &c_));
}

TEST_F(GadgetListTest, AddAfterItselfChangesNothing) {
  list_.Add(a_);
  list_.Add(b_);
  list_.AddAfter(a_, a_);
  EXPECT_THAT(list_.gadgets(), ElementsAre(&a_, &b_));
  EXPECT_EQ(a_.removed, 0);
}

TEST_F(GadgetListTest, AddingAgainMovesTheGadgetToTheEnd) {
  list_.Add(a_);
  list_.Add(b_);
  list_.Add(a_);
  EXPECT_THAT(list_.gadgets(), ElementsAre(&b_, &a_));
  EXPECT_EQ(a_.removed, 1);
  EXPECT_EQ(a_.added, 2);
}

TEST_F(GadgetListTest, AddingToAnotherListTakesTheGadgetOutOfTheFirst) {
  GadgetList other;
  list_.Add(a_);
  list_.Add(b_);
  other.Add(a_);
  EXPECT_THAT(list_.gadgets(), ElementsAre(&b_));
  EXPECT_THAT(other.gadgets(), ElementsAre(&a_));
  EXPECT_EQ(a_.list(), &other);
}

TEST_F(GadgetListTest, MovingToAnotherListKeepsTheFocus) {
  GadgetList other;
  list_.Add(a_);
  a_.SetFocus();
  other.Add(a_);
  EXPECT_TRUE(a_.HasFocus());
}

TEST_F(GadgetListTest, RemoveTakesTheGadgetOutAndDropsItsFocus) {
  list_.Add(a_);
  list_.Add(b_);
  a_.SetFocus();
  list_.Remove(a_);
  EXPECT_THAT(list_.gadgets(), ElementsAre(&b_));
  EXPECT_EQ(a_.list(), nullptr);
  EXPECT_EQ(a_.removed, 1);
  EXPECT_FALSE(a_.HasFocus());
}

TEST_F(GadgetListTest, RemoveDropsTheHoldOnTheMouse) {
  a_.set_sticky(true);
  list_.Add(a_);
  Press(5);
  list_.Input(view_);
  ASSERT_EQ(Gadget::mouse_captor(), &a_);
  list_.Remove(a_);
  EXPECT_EQ(Gadget::mouse_captor(), nullptr);
}

TEST_F(GadgetListTest, RemoveIgnoresAGadgetOfAnotherList) {
  GadgetList other;
  other.Add(a_);
  a_.SetFocus();
  list_.Remove(a_);
  EXPECT_EQ(a_.list(), &other);
  EXPECT_TRUE(a_.HasFocus());
  EXPECT_EQ(a_.removed, 0);
}

TEST_F(GadgetListTest, ClearEmptiesTheListAndKeepsTheFocus) {
  list_.Add(a_);
  list_.Add(b_);
  b_.SetFocus();
  list_.Clear();
  EXPECT_TRUE(list_.empty());
  EXPECT_EQ(a_.list(), nullptr);
  EXPECT_EQ(b_.list(), nullptr);
  EXPECT_EQ(a_.removed, 1);
  EXPECT_EQ(b_.removed, 1);
  EXPECT_TRUE(b_.HasFocus());
  b_.ClearFocus();
}

TEST_F(GadgetListTest, AGadgetDestroyedFirstLeavesTheList) {
  list_.Add(a_);
  {
    Probe temporary;
    list_.Add(temporary);
    list_.Add(b_);
  }
  EXPECT_THAT(list_.gadgets(), ElementsAre(&a_, &b_));
}

TEST_F(GadgetListTest, AListDestroyedFirstReleasesItsGadgets) {
  {
    GadgetList temporary;
    temporary.Add(a_);
    temporary.Add(b_);
  }
  EXPECT_EQ(a_.list(), nullptr);
  EXPECT_EQ(b_.list(), nullptr);
  EXPECT_EQ(a_.removed, 1);
}

TEST_F(GadgetListTest, ExtractFindsAGadgetByItsId) {
  Gadget plain(0, 0, 10, 10, 0);
  list_.Add(plain);
  list_.Add(a_);
  list_.Add(b_);
  EXPECT_EQ(list_.Extract(2), &b_);
  EXPECT_EQ(list_.Extract(9), nullptr);
  // 0 is "no ID", which is also what a plain gadget answers.
  EXPECT_EQ(list_.Extract(0), nullptr);
}

TEST_F(GadgetListTest, IsToRedrawTellsWhetherAGadgetIsFlagged) {
  list_.Add(a_);
  list_.Add(b_);
  list_.DrawAll(view_);
  EXPECT_FALSE(list_.IsToRedraw());
  b_.FlagToRedraw();
  EXPECT_TRUE(list_.IsToRedraw());
}

TEST_F(GadgetListTest, DrawAllDrawsEveryGadgetOrOnlyTheFlaggedOnes) {
  list_.Add(a_);
  list_.Add(b_);
  list_.DrawAll(view_);
  EXPECT_EQ(a_.draws, 1);
  EXPECT_EQ(b_.draws, 1);

  b_.FlagToRedraw();
  list_.DrawAll(view_, /*forced=*/false);
  EXPECT_EQ(a_.draws, 1);
  EXPECT_EQ(b_.draws, 2);
}

TEST_F(GadgetListTest, InputGivesAClickToTheGadgetUnderIt) {
  list_.Add(a_);
  list_.Add(b_);
  Press(25);
  const engine::window::InputEvent event = list_.Input(view_);
  EXPECT_EQ(a_.actions, 0);
  EXPECT_EQ(b_.actions, 1);
  EXPECT_EQ(event.key, engine::window::ButtonKey(2));
  EXPECT_FALSE(event.IsClick());
}

TEST_F(GadgetListTest, InputReturnsAClickNoGadgetTook) {
  list_.Add(a_);
  Press(35);
  const engine::window::InputEvent event = list_.Input(view_);
  EXPECT_EQ(a_.actions, 0);
  EXPECT_TRUE(event.IsPress(MouseButton::kLeft));
  EXPECT_EQ(event.x, 35);
}

TEST_F(GadgetListTest, InputStopsAtTheFirstGadgetThatTakesTheClick) {
  Probe over_a(4, 0);
  list_.Add(over_a);
  list_.Add(a_);
  Press(5);
  list_.Input(view_);
  EXPECT_EQ(over_a.actions, 1);
  EXPECT_EQ(a_.actions, 0);
}

TEST_F(GadgetListTest, InputGoesOnPastAGadgetThatLetsTheClickPass) {
  Probe over_a(4, 0);
  over_a.takes_input = false;
  list_.Add(over_a);
  list_.Add(a_);
  Press(5);
  list_.Input(view_);
  EXPECT_EQ(over_a.actions, 1);
  EXPECT_EQ(a_.actions, 1);
}

TEST_F(GadgetListTest, TheFirstInputDrawsEveryGadgetAndLaterOnesDoNot) {
  list_.Add(a_);
  list_.Add(b_);
  list_.Input(view_);
  EXPECT_EQ(a_.draws, 1);
  EXPECT_EQ(b_.draws, 1);
  list_.Input(view_);
  EXPECT_EQ(a_.draws, 1);
}

TEST_F(GadgetListTest, AddingInFrontDoesNotMakeItANewList) {
  list_.Add(a_);
  list_.Input(view_);
  a_.SetFocus();
  list_.AddFront(b_);
  list_.Input(view_);
  EXPECT_EQ(a_.draws, 1);
  EXPECT_TRUE(a_.HasFocus());
  a_.ClearFocus();
}

TEST_F(GadgetListTest, AnotherListsTurnDrawsEverythingAndDropsTheFocus) {
  GadgetList other;
  other.Add(b_);
  list_.Add(a_);
  list_.Input(view_);
  a_.SetFocus();
  other.Input(view_);
  EXPECT_FALSE(a_.HasFocus());
  list_.Input(view_);
  EXPECT_EQ(a_.draws, 2);
}

TEST_F(GadgetListTest, FlagToRedrawMakesTheNextInputDrawEverything) {
  list_.Add(a_);
  list_.Input(view_);
  list_.FlagToRedraw();
  list_.Input(view_);
  EXPECT_EQ(a_.draws, 2);
}

TEST_F(GadgetListTest, FlagToRedrawOfAnIdleListLeavesTheActiveOneAlone) {
  GadgetList idle;
  list_.Add(a_);
  list_.Input(view_);
  idle.FlagToRedraw();
  list_.Input(view_);
  EXPECT_EQ(a_.draws, 1);
}

// A list that takes the place of a destroyed one, as a dialog's local list
// does on the stack, is a new list.
TEST_F(GadgetListTest, ADestroyedListIsForgotten) {
  auto first = std::make_unique<GadgetList>();
  first->Add(a_);
  first->Input(view_);
  first.reset();
  // Whatever address the next list gets, nothing remembers the old one.
  list_.Add(a_);
  list_.Input(view_);
  EXPECT_EQ(a_.draws, 2);
}

TEST_F(GadgetListTest, AGadgetMayRemoveItselfWhileItIsOfferedInput) {
  a_.on_action = [this] { list_.Remove(a_); };
  a_.takes_input = false;
  Probe under_a(4, 0);
  list_.Add(a_);
  list_.Add(under_a);
  list_.Add(b_);
  Press(5);
  list_.Input(view_);
  EXPECT_THAT(list_.gadgets(), ElementsAre(&under_a, &b_));
}

TEST_F(GadgetListTest, AGadgetMayAddGadgetsWhileItIsOfferedInput) {
  Probe late(4, 0);
  a_.on_action = [&] { list_.Add(late); };
  a_.takes_input = false;
  list_.Add(a_);
  Press(5);
  list_.Input(view_);
  // Added behind the gadget being asked, so it is offered the click too.
  EXPECT_EQ(late.actions, 1);
}

TEST_F(GadgetListTest, AGadgetMayClearTheListWhileItIsOfferedInput) {
  a_.on_action = [this] { list_.Clear(); };
  a_.takes_input = false;
  list_.Add(a_);
  list_.Add(b_);
  Press(5);
  list_.Input(view_);
  EXPECT_TRUE(list_.empty());
  EXPECT_EQ(b_.actions, 0);
}

TEST_F(GadgetListTest, InputOfAnEmptyListStillReadsTheEvent) {
  Press(5);
  const engine::window::InputEvent event = list_.Input(view_);
  EXPECT_TRUE(event.IsPress(MouseButton::kLeft));
}

}  // namespace
}  // namespace game
