#include "game/ui/link.h"

#include "gtest/gtest.h"

namespace game {
namespace {

TEST(LinkClassTest, AddAfterItselfLeavesTheListAlone) {
  Link head;
  Link node;
  node.AppendTo(head);

  EXPECT_EQ(&node.InsertAfter(node), &head);

  EXPECT_EQ(node.Previous(), &head);
  EXPECT_EQ(node.Next(), nullptr);
  EXPECT_EQ(head.Next(), &node);
}

TEST(LinkClassTest, AddHeadOfTheHeadLeavesTheListAlone) {
  Link head;
  Link node;
  node.AppendTo(head);

  EXPECT_EQ(&head.PrependTo(node), &head);

  EXPECT_EQ(head.Previous(), nullptr);
  EXPECT_EQ(head.Next(), &node);
  EXPECT_EQ(node.Next(), nullptr);
}

TEST(LinkClassTest, AddTailOfTheTailLeavesTheListAlone) {
  Link head;
  Link node;
  node.AppendTo(head);

  EXPECT_EQ(&node.AppendTo(head), &head);

  EXPECT_EQ(node.Previous(), &head);
  EXPECT_EQ(node.Next(), nullptr);
  EXPECT_EQ(head.Previous(), nullptr);
}

}  // namespace
}  // namespace game
