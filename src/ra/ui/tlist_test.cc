// Tests TListClass's item list: insertion at the selected row and sorting.

#include "ra/ui/tlist.h"

#include "base/array.h"
#include "base/installed.h"
#include "base/point.h"
#include "base/rect.h"
#include "engine/gfx/pixel_buffer.h"
#include "game/ui/gadget.h"
#include "game/ui/gadget_list.h"
#include "gtest/gtest.h"
#include "ra/assets/assets.h"
#include "ra/assets/palettes.h"
#include "ra/core/defines.h"

namespace ra {

namespace {

// A list entry that draws nothing; the tests never draw.
struct Item {
  int value;
  void Draw_It(PixelView& /*view*/, int /*index*/, int /*x*/, int /*y*/,
               int /*width*/, int /*height*/, bool /*selected*/,
               FontType /*font*/, TextPrintType /*flags*/) const {}
};

// The list measures the font it prints in, which reads the fonts and colour
// schemes Game installs; default ones measure text as nothing.
class TListClassTest : public testing::Test {
 protected:
  Assets assets_;
  base::Installed<Assets>::Scope assets_scope_{assets_};
  Palettes palettes_;
  base::Installed<Palettes>::Scope palettes_scope_{palettes_};
  TListClass<Item*> list_{1,        0,  0, 100, 40, FontType::k6Point,
                          TPF_NONE, {}, {}};
  Item a_{1};
  Item b_{2};
  Item c_{3};
};

// The scroll bar is three gadgets of its own, which go where the list box goes.
TEST_F(TListClassTest, TheScrollBarFollowsTheListBoxIntoAGadgetList) {
  game::Gadget button(base::Rect{0, 0, 10, 10},
                      game::Gadget::InputFlags::kNone);
  game::GadgetList gadgets;
  gadgets.Add(list_);
  gadgets.Add(button);
  list_.Add_Scroll_Bar();
  ASSERT_EQ(gadgets.gadgets().size(), 5U);
  EXPECT_EQ(base::At(gadgets.gadgets(), 0), &list_);
  EXPECT_EQ(base::At(gadgets.gadgets(), 4), &button);

  // Moving it to another list takes the scroll bar along.
  game::GadgetList other;
  other.Add(list_);
  EXPECT_EQ(gadgets.gadgets().size(), 1U);
  EXPECT_EQ(other.gadgets().size(), 4U);

  other.Clear();
  other.Add(list_);
  EXPECT_EQ(other.gadgets().size(), 4U);
  other.Remove(list_);
  EXPECT_TRUE(other.empty());
}

// Moving the list box moves it and its scroll bar, which stays on its right.
TEST_F(TListClassTest, SetPositionMovesTheListBoxAndItsScrollBar) {
  game::GadgetList gadgets;
  gadgets.Add(list_);
  list_.Add_Scroll_Bar();
  ASSERT_EQ(gadgets.gadgets().size(), 4U);

  list_.SetPosition(base::Point{30, 50});

  EXPECT_EQ(list_.region(), (base::Rect{30, 50, 100, 40}));
  for (const game::Gadget* const part : gadgets.gadgets()) {
    EXPECT_EQ(part->region().right(), 130);
    EXPECT_GE(part->y(), 50);
    EXPECT_LE(part->region().bottom(), 90);
  }
}

// The team-type editor's Insert key puts a mission before the selected one.
TEST_F(TListClassTest, InsertItemPutsTheItemAtTheSelectedRow) {
  list_.Add_Item(&a_);
  list_.Add_Item(&b_);
  list_.Set_Selected_Index(1);

  list_.Insert_Item(&c_);

  ASSERT_EQ(list_.Count(), 3);
  EXPECT_EQ(list_.Get_Item(0), &a_);
  EXPECT_EQ(list_.Get_Item(1), &c_);
  EXPECT_EQ(list_.Get_Item(2), &b_);
}

// With nothing to insert before, the item goes on the end.
TEST_F(TListClassTest, InsertItemIntoAnEmptyListAppends) {
  list_.Insert_Item(&a_);

  ASSERT_EQ(list_.Count(), 1);
  EXPECT_EQ(list_.Get_Item(0), &a_);
}

// The editors sort by name ignoring case, so names can tie; tied entries stay
// in the order they were added.
TEST_F(TListClassTest, SortKeepsEqualItemsInInsertionOrder) {
  Item tied{1};
  list_.Add_Item(&c_);
  list_.Add_Item(&a_);
  list_.Add_Item(&b_);
  list_.Add_Item(&tied);

  list_.Sort([](const Item* left, const Item* right) {
    return left->value < right->value;
  });

  ASSERT_EQ(list_.Count(), 4);
  EXPECT_EQ(list_.Get_Item(0), &a_);
  EXPECT_EQ(list_.Get_Item(1), &tied);
  EXPECT_EQ(list_.Get_Item(2), &b_);
  EXPECT_EQ(list_.Get_Item(3), &c_);
}

}  // namespace

}  // namespace ra
