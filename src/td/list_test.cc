// Ownership semantics of ListClass items: the list copies what it is given.
#include "td/list.h"

#include <string>
#include <string_view>

#include "gtest/gtest.h"
#include "td/cheklist.h"
#include "td/defines.h"
#include "td/msglist.h"
#include "td/txtlabel.h"

namespace {

class TdListClassTest : public testing::Test {
 protected:
  // No scroll arrow shapes: the tests never draw.
  ListClass list_{0, 0, 0, 100, 60, TPF_6POINT, {}, {}};
};

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

TEST(TdCheckListClassTest, CheckItemWritesTheGlyphIntoTheOwnedText) {
  CheckListClass list(0, 0, 0, 100, 60, TPF_6POINT, {}, {});
  const std::string source = " trigger";
  list.Add_Item(source.c_str());

  list.Check_Item(0, 1);
  EXPECT_TRUE(list.Is_Checked(0));
  EXPECT_EQ(list.Get_Item(0)[0], CheckListClass::kCheckChar);
  EXPECT_EQ(std::string_view(list.Get_Item(0)).substr(1), "trigger");
  EXPECT_EQ(source, " trigger");

  list.Check_Item(0, 0);
  EXPECT_FALSE(list.Is_Checked(0));
  EXPECT_EQ(list.Get_Item(0)[0], CheckListClass::kUncheckChar);

  list.Check_Item(3, 1);
  EXPECT_FALSE(list.Is_Checked(3));
}

TEST(TdMessageListTest, OversizedEditPrefixIsClampedToOwnedStorage) {
  MessageListClass messages;
  messages.Init(0, 0, 4, 80, 8);
  std::string prefix(300, 'x');
  ASSERT_NE(messages.Add_Edit(0, TPF_6POINT, prefix.data(), 100), nullptr);
  ASSERT_NE(messages.Get_Edit_Buf(), nullptr);
  EXPECT_STREQ(messages.Get_Edit_Buf(), "");
}

TEST(TdMessageListTest, TailMergesIntoMatchingPlayersOwnedBuffer) {
  MessageListClass messages;
  messages.Init(0, 0, 4, 80, 8);
  char head[] = "Alice:abcdefghijklmnopqrstuvw";
  char tail[] = "Alice:tail";
  auto* label = messages.Add_Message(head, 0, TPF_6POINT, -1,
                                     MESSAGE_HEAD_MAGIC_NUMBER, 123);
  ASSERT_NE(label, nullptr);
  EXPECT_EQ(messages.Add_Message(tail, 0, TPF_6POINT, -1,
                                 MESSAGE_HEAD_MAGIC_NUMBER + 1, 123),
            label);
  EXPECT_EQ(std::string_view(label->Text), "Alice:abcdefghijklmnopqrstuvwtail");
  EXPECT_EQ(messages.Num_Messages(), 1);
}

TEST(TdMessageListTest, MatchingCrcDoesNotMergeDifferentPlayers) {
  MessageListClass messages;
  messages.Init(0, 0, 4, 80, 8);
  char head[] = "Alice:head";
  char tail[] = "Bob:tail";
  auto* label = messages.Add_Message(head, 0, TPF_6POINT, -1,
                                     MESSAGE_HEAD_MAGIC_NUMBER, 123);
  ASSERT_NE(label, nullptr);
  EXPECT_NE(messages.Add_Message(tail, 0, TPF_6POINT, -1,
                                 MESSAGE_HEAD_MAGIC_NUMBER + 1, 123),
            label);
  EXPECT_EQ(messages.Num_Messages(), 2);
}

}  // namespace
