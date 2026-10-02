// Tests TListClass's item list: insertion at the selected row.

#include "ra/ui/tlist.h"

#include "base/installed.h"
#include "engine/gfx/pixel_buffer.h"
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
               TextPrintType /*flags*/) const {}
};

// The list measures the font it prints in, which reads the fonts and colour
// schemes Game installs; default ones measure text as nothing.
class TListClassTest : public testing::Test {
 protected:
  Assets assets_;
  base::Installed<Assets>::Scope assets_scope_{assets_};
  Palettes palettes_;
  base::Installed<Palettes>::Scope palettes_scope_{palettes_};
  TListClass<Item*> list_{1, 0, 0, 100, 40, TPF_6POINT, {}, {}};
  Item a_{1};
  Item b_{2};
  Item c_{3};
};

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

}  // namespace

}  // namespace ra
