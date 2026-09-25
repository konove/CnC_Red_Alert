// Tests for linking a drop-down list into a dialog's gadget chain.

#include "ra/drop.h"

#include "engine/base/installed.h"
#include "gtest/gtest.h"
#include "ra/assets.h"
#include "ra/defines.h"
#include "ra/gadget.h"
#include "ra/palettes.h"

namespace {

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
  DropListClass drop_{1, text_, sizeof(text_), TPF_6POINT, 0, 0, 100, 40,
                      {}, {}};
  GadgetClass button_{0, 0, 10, 10, 0};
};

// Adding to a list returns the head of the chain, which a dialog's chain
// starts with some other gadget. The skirmish dialog adds its house list to
// a chain headed by a button, and this used to throw std::bad_cast.
TEST_F(DropListClassTest, AddingToAChainReturnsItsHead) {
  EXPECT_EQ(&drop_.Add(button_), &button_);
}

TEST_F(DropListClassTest, AddingTailToAChainReturnsItsHead) {
  EXPECT_EQ(&drop_.Add_Tail(button_), &button_);
}

}  // namespace
