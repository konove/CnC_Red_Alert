// Ownership semantics of ListClass items: the list copies what it is given.
#include "td/ui/list.h"

#include <string>
#include <string_view>

#include "base/array.h"
#include "base/installed.h"
#include "game/ui/gadget.h"
#include "game/ui/gadget_list.h"
#include "game/ui/standard_colors.h"
#include "gtest/gtest.h"
#include "td/assets/assets.h"
#include "td/core/defines.h"
#include "td/ui/cheklist.h"
#include "td/ui/msglist.h"

namespace td {

namespace {

// Measuring a list item reads the font it is printed in, which Game installs
// in the real game. A default Assets has no fonts, so the items measure as
// nothing, which is all these tests need.
class InstalledAssets {
 protected:
  Assets assets_;
  base::Installed<Assets>::Scope assets_scope_{assets_};
};

class TdListClassTest : public InstalledAssets, public testing::Test {
 protected:
  // No scroll arrow shapes: the tests never draw.
  ListClass list_{0, 0, 0, 100, 60, FontType::k6Point, TPF_NONE, {}, {}};
};

// The scroll bar is three gadgets of its own, which go where the list box goes.
TEST_F(TdListClassTest, TheScrollBarFollowsTheListBoxIntoAGadgetList) {
  game::GadgetClass button(0, 0, 10, 10, 0);
  game::GadgetList gadgets;
  list_.Add_Scroll_Bar();
  gadgets.Add(list_);
  gadgets.Add(button);

  ASSERT_EQ(gadgets.gadgets().size(), 5U);
  EXPECT_EQ(base::At(gadgets.gadgets(), 0), &list_);
  EXPECT_EQ(base::At(gadgets.gadgets(), 4), &button);

  gadgets.Remove(list_);
  ASSERT_EQ(gadgets.gadgets().size(), 1U);
  EXPECT_EQ(base::At(gadgets.gadgets(), 0), &button);
}

TEST_F(TdListClassTest, AScrollBarAddedLaterJoinsTheListBoxesGadgetList) {
  game::GadgetClass button(0, 0, 10, 10, 0);
  game::GadgetList gadgets;
  gadgets.Add(list_);
  gadgets.Add(button);
  list_.Add_Scroll_Bar();

  ASSERT_EQ(gadgets.gadgets().size(), 5U);
  EXPECT_EQ(base::At(gadgets.gadgets(), 0), &list_);
  EXPECT_EQ(base::At(gadgets.gadgets(), 4), &button);

  list_.Remove_Scroll_Bar();
  EXPECT_EQ(gadgets.gadgets().size(), 2U);
}

TEST_F(TdListClassTest, ClearingTheGadgetListReleasesTheScrollBar) {
  game::GadgetList gadgets;
  list_.Add_Scroll_Bar();
  gadgets.Add(list_);
  gadgets.Clear();
  EXPECT_TRUE(gadgets.empty());

  // Nothing is left behind that a second add would trip over.
  gadgets.Add(list_);
  EXPECT_EQ(gadgets.gadgets().size(), 4U);
}

TEST_F(TdListClassTest, AddItemCopiesTheText) {
  char buffer[] = "first";
  EXPECT_EQ(list_.Add_Item(buffer), 0);
  buffer[0] = 'X';

  ASSERT_EQ(list_.Count(), 1);
  EXPECT_STREQ(list_.Get_Item(0), "first");
}

TEST_F(TdListClassTest, NullTextAddsNothing) {
  list_.Add_Item("only");
  EXPECT_EQ(list_.Add_Item(static_cast<const char*>(nullptr)), 0);
  EXPECT_EQ(list_.Count(), 1);
}

TEST_F(TdListClassTest, EmptyListHasNoItems) {
  EXPECT_EQ(list_.Get_Item(0), nullptr);
  EXPECT_EQ(list_.Current_Item(), nullptr);
}

TEST_F(TdListClassTest, RemoveItemByContentRemovesTheFirstMatch) {
  list_.Add_Item("alpha");
  list_.Add_Item("beta");
  list_.Add_Item("alpha");

  list_.Remove_Item("alpha");

  ASSERT_EQ(list_.Count(), 2);
  EXPECT_STREQ(list_.Get_Item(0), "beta");
  EXPECT_STREQ(list_.Get_Item(1), "alpha");
}

TEST_F(TdListClassTest, RemoveItemAcceptsAPointerFromGetItem) {
  list_.Add_Item("alpha");
  list_.Add_Item("beta");

  list_.Remove_Item(list_.Get_Item(1));

  ASSERT_EQ(list_.Count(), 1);
  EXPECT_STREQ(list_.Get_Item(0), "alpha");
}

TEST_F(TdListClassTest, RemoveItemIgnoresUnknownTextAndIndex) {
  list_.Add_Item("alpha");

  list_.Remove_Item("missing");
  list_.Remove_Item(static_cast<const char*>(nullptr));
  list_.Remove_Item(-1);
  list_.Remove_Item(1);

  EXPECT_EQ(list_.Count(), 1);
}

TEST_F(TdListClassTest, SetItemReplacesTheText) {
  list_.Add_Item("alpha");
  list_.Add_Item("beta");

  list_.Set_Item(1, std::string_view("gamma"));
  list_.Set_Item(2, "ignored");
  list_.Set_Item(-1, "ignored");

  ASSERT_EQ(list_.Count(), 2);
  EXPECT_STREQ(list_.Get_Item(0), "alpha");
  EXPECT_STREQ(list_.Get_Item(1), "gamma");
}

TEST_F(TdListClassTest, ClearRemovesEverything) {
  list_.Add_Item("alpha");
  list_.Add_Item("beta");

  list_.Clear();

  EXPECT_EQ(list_.Count(), 0);
  EXPECT_EQ(list_.Get_Item(0), nullptr);
}

class TdCheckListClassTest : public InstalledAssets, public testing::Test {};

TEST_F(TdCheckListClassTest, CheckItemWritesTheGlyphIntoTheOwnedText) {
  CheckListClass list(0, 0, 0, 100, 60, FontType::k6Point, TPF_NONE, {}, {});
  const std::string source = " trigger";
  list.Add_Item(source.c_str());

  list.Check_Item(0, 1);
  EXPECT_TRUE(list.Is_Checked(0));
  EXPECT_EQ(std::string_view(list.Get_Item(0)).front(),
            CheckListClass::kCheckChar);
  EXPECT_EQ(std::string_view(list.Get_Item(0)).substr(1), "trigger");
  EXPECT_EQ(source, " trigger");

  list.Check_Item(0, 0);
  EXPECT_FALSE(list.Is_Checked(0));
  EXPECT_EQ(std::string_view(list.Get_Item(0)).front(),
            CheckListClass::kUncheckChar);

  list.Check_Item(3, 1);
  EXPECT_FALSE(list.Is_Checked(3));
}

TEST(TdMessageListTest, OversizedEditPrefixIsClampedToOwnedStorage) {
  MessageListClass messages;
  messages.Init(0, 0, 4, 80, 8);
  std::string prefix(300, 'x');
  ASSERT_NE(messages.Add_Edit(game::kTransparentBlack, FontType::k6Point,
                              TPF_NONE, prefix.data(), 100),
            nullptr);
  ASSERT_NE(messages.Get_Edit_Buf(), nullptr);
  EXPECT_STREQ(messages.Get_Edit_Buf(), "");
}

TEST(TdMessageListTest, TailMergesIntoMatchingPlayersOwnedBuffer) {
  MessageListClass messages;
  messages.Init(0, 0, 4, 80, 8);
  char head[] = "Alice:abcdefghijklmnopqrstuvw";
  char tail[] = "Alice:tail";
  auto* label =
      messages.Add_Message(head, game::kTransparentBlack, FontType::k6Point,
                           TPF_NONE, -1, MESSAGE_HEAD_MAGIC_NUMBER, 123);
  ASSERT_NE(label, nullptr);
  EXPECT_EQ(
      messages.Add_Message(tail, game::kTransparentBlack, FontType::k6Point,
                           TPF_NONE, -1, MESSAGE_HEAD_MAGIC_NUMBER + 1, 123),
      label);
  EXPECT_EQ(std::string_view(label->Text), "Alice:abcdefghijklmnopqrstuvwtail");
  EXPECT_EQ(messages.Num_Messages(), 1);
}

TEST(TdMessageListTest, MatchingCrcDoesNotMergeDifferentPlayers) {
  MessageListClass messages;
  messages.Init(0, 0, 4, 80, 8);
  char head[] = "Alice:head";
  char tail[] = "Bob:tail";
  auto* label =
      messages.Add_Message(head, game::kTransparentBlack, FontType::k6Point,
                           TPF_NONE, -1, MESSAGE_HEAD_MAGIC_NUMBER, 123);
  ASSERT_NE(label, nullptr);
  EXPECT_NE(
      messages.Add_Message(tail, game::kTransparentBlack, FontType::k6Point,
                           TPF_NONE, -1, MESSAGE_HEAD_MAGIC_NUMBER + 1, 123),
      label);
  EXPECT_EQ(messages.Num_Messages(), 2);
}

TEST(TdMessageListTest, SameTextInAnotherFontIsNotADuplicate) {
  MessageListClass messages;
  messages.Init(0, 0, 4, 80, 8);
  char text[] = "Alice:hello";
  auto* label = messages.Add_Message(text, game::kTransparentBlack,
                                     FontType::k6Point, TPF_NONE, -1, 0, 0);
  ASSERT_NE(label, nullptr);
  EXPECT_EQ(messages.Add_Message(text, game::kTransparentBlack,
                                 FontType::k6Point, TPF_NONE, -1, 0, 0),
            label);
  EXPECT_NE(messages.Add_Message(text, game::kTransparentBlack,
                                 FontType::k8Point, TPF_NONE, -1, 0, 0),
            label);
  EXPECT_EQ(messages.Num_Messages(), 2);
}

TEST(TdMessageListTest, TailDoesNotMergeIntoMessageInAnotherFont) {
  MessageListClass messages;
  messages.Init(0, 0, 4, 80, 8);
  char head[] = "Alice:head";
  char tail[] = "Alice:tail";
  auto* label =
      messages.Add_Message(head, game::kTransparentBlack, FontType::k6Point,
                           TPF_NONE, -1, MESSAGE_HEAD_MAGIC_NUMBER, 123);
  ASSERT_NE(label, nullptr);
  EXPECT_NE(
      messages.Add_Message(tail, game::kTransparentBlack, FontType::k8Point,
                           TPF_NONE, -1, MESSAGE_HEAD_MAGIC_NUMBER + 1, 123),
      label);
  EXPECT_EQ(std::string_view(label->Text), "Alice:head");
  EXPECT_EQ(messages.Num_Messages(), 2);
}

}  // namespace

}  // namespace td
