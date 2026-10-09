#include "game/ui/link.h"

#include "gtest/gtest.h"

namespace game {
namespace {

TEST(LinkClassTest, AddAfterItselfLeavesTheListAlone) {
  Link head;
  Link node;
  node.Add_Tail(head);

  EXPECT_EQ(&node.Add(node), &head);

  EXPECT_EQ(node.Get_Prev(), &head);
  EXPECT_EQ(node.Get_Next(), nullptr);
  EXPECT_EQ(head.Get_Next(), &node);
}

TEST(LinkClassTest, AddHeadOfTheHeadLeavesTheListAlone) {
  Link head;
  Link node;
  node.Add_Tail(head);

  EXPECT_EQ(&head.Add_Head(node), &head);

  EXPECT_EQ(head.Get_Prev(), nullptr);
  EXPECT_EQ(head.Get_Next(), &node);
  EXPECT_EQ(node.Get_Next(), nullptr);
}

TEST(LinkClassTest, AddTailOfTheTailLeavesTheListAlone) {
  Link head;
  Link node;
  node.Add_Tail(head);

  EXPECT_EQ(&node.Add_Tail(head), &head);

  EXPECT_EQ(node.Get_Prev(), &head);
  EXPECT_EQ(node.Get_Next(), nullptr);
  EXPECT_EQ(head.Get_Prev(), nullptr);
}

}  // namespace
}  // namespace game
