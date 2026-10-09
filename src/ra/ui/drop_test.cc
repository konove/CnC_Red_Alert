// Tests for a drop-down list in a dialog's gadget list.

#include "ra/ui/drop.h"

#include "base/array.h"
#include "base/installed.h"
#include "engine/gfx/pixel_buffer.h"
#include "game/ui/gadget.h"
#include "game/ui/gadget_list.h"
#include "gtest/gtest.h"
#include "ra/assets/assets.h"
#include "ra/assets/palettes.h"
#include "ra/core/defines.h"

namespace ra {

namespace {

// A typed list entry that draws nothing; the tests never draw.
struct Item {
  [[nodiscard]] static const char* Description() { return "item"; }
  void Draw_It(PixelView& /*view*/, int /*index*/, int /*x*/, int /*y*/,
               int /*width*/, int /*height*/, bool /*selected*/,
               FontType /*font*/, TextPrintType /*flags*/) const {}
};

// Building the list selects and measures the font it prints in, which reads
// the fonts and colour schemes Game installs in the real game. Default ones
// have no fonts, so text measures as nothing, which is all these tests need.
class DropListClassTest : public testing::Test {
 protected:
  Assets assets_;
  base::Installed<Assets>::Scope assets_scope_{assets_};
  Palettes palettes_;
  base::Installed<Palettes>::Scope palettes_scope_{palettes_};
  char text_[16] = {};
  // No arrow shapes: the tests never draw.
  DropListClass drop_{
      1,  text_, sizeof(text_), FontType::k6Point, TPF_NONE, 0, 0, 100, 40,
      {}, {}};
  game::GadgetClass button_{0, 0, 10, 10, 0};
};

TEST_F(DropListClassTest, TheDropButtonFollowsTheFieldIntoAGadgetList) {
  game::GadgetList gadgets;
  gadgets.Add(button_);
  gadgets.Add(drop_);
  ASSERT_EQ(gadgets.gadgets().size(), 3U);
  EXPECT_EQ(base::At(gadgets.gadgets(), 1), &drop_);
  EXPECT_EQ(base::At(gadgets.gadgets(), 2), &drop_.DropButton);

  gadgets.Remove(drop_);
  ASSERT_EQ(gadgets.gadgets().size(), 1U);
  EXPECT_EQ(base::At(gadgets.gadgets(), 0), &button_);
}

// The dropped list is offered input second, as it was in the chain: behind
// the first gadget and in front of whatever it covers.
TEST_F(DropListClassTest, ExpandingPutsTheListSecondInTheGadgetList) {
  game::GadgetList gadgets;
  gadgets.Add(button_);
  gadgets.Add(drop_);
  drop_.Expand();
  ASSERT_EQ(gadgets.gadgets().size(), 4U);
  EXPECT_EQ(base::At(gadgets.gadgets(), 0), &button_);
  EXPECT_EQ(base::At(gadgets.gadgets(), 1), &drop_.List);

  drop_.Collapse();
  EXPECT_EQ(gadgets.gadgets().size(), 3U);
  EXPECT_EQ(drop_.List.list(), nullptr);
}

TEST_F(DropListClassTest, LeavingAGadgetListClosesTheDroppedList) {
  game::GadgetList gadgets;
  gadgets.Add(drop_);
  drop_.Expand();
  gadgets.Remove(drop_);
  EXPECT_TRUE(gadgets.empty());
  EXPECT_FALSE(drop_.IsDropped);
}

TEST_F(DropListClassTest, ClearingTheGadgetListClosesTheDroppedList) {
  game::GadgetList gadgets;
  gadgets.Add(drop_);
  drop_.Expand();
  gadgets.Clear();
  EXPECT_TRUE(gadgets.empty());
  EXPECT_FALSE(drop_.IsDropped);
  EXPECT_EQ(drop_.List.list(), nullptr);

  // And it opens again in the list it is added to next.
  gadgets.Add(drop_);
  drop_.Expand();
  EXPECT_EQ(gadgets.gadgets().size(), 3U);
}

// A drop list that leaves the dialog must not keep the keystrokes.
TEST_F(DropListClassTest, LeavingAGadgetListDropsTheKeyboardFocus) {
  game::GadgetList gadgets;
  gadgets.Add(drop_);
  drop_.Set_Focus();
  ASSERT_TRUE(drop_.Has_Focus());
  drop_.Expand();

  gadgets.Remove(drop_);
  EXPECT_FALSE(drop_.Has_Focus());
  EXPECT_FALSE(drop_.IsDropped);
}

TEST_F(DropListClassTest, LeavingAGadgetListDropsATypedListsKeyboardFocus) {
  char text[16] = {};
  TDropListClass<Item*> typed{
      2,  text, sizeof(text), FontType::k6Point, TPF_NONE, 0, 0, 100, 40,
      {}, {}};
  game::GadgetList gadgets;
  gadgets.Add(typed);
  typed.Set_Focus();
  ASSERT_TRUE(typed.Has_Focus());
  typed.Expand();

  gadgets.Remove(typed);
  EXPECT_FALSE(typed.Has_Focus());
  EXPECT_FALSE(typed.IsDropped);
}

}  // namespace

}  // namespace ra
