#include "game/ui/link.h"

#include "gtest/gtest.h"

namespace game {
namespace {

TEST(LinkTest, AddAfterItselfLeavesTheListAlone) {
  Link head;
  Link node;
  node.AppendTo(head);

  EXPECT_EQ(&node.InsertAfter(node), &head);

  EXPECT_EQ(node.Previous(), &head);
  EXPECT_EQ(node.Next(), nullptr);
  EXPECT_EQ(head.Next(), &node);
}

TEST(LinkTest, AddHeadOfTheHeadLeavesTheListAlone) {
  Link head;
  Link node;
  node.AppendTo(head);

  EXPECT_EQ(&head.PrependTo(node), &head);

  EXPECT_EQ(head.Previous(), nullptr);
  EXPECT_EQ(head.Next(), &node);
  EXPECT_EQ(node.Next(), nullptr);
}

TEST(LinkTest, AddTailOfTheTailLeavesTheListAlone) {
  Link head;
  Link node;
  node.AppendTo(head);

  EXPECT_EQ(&node.AppendTo(head), &head);

  EXPECT_EQ(node.Previous(), &head);
  EXPECT_EQ(node.Next(), nullptr);
  EXPECT_EQ(head.Previous(), nullptr);
}

TEST(LinkTest, RemoveReturnsTheHeadOfWhatIsLeft) {
  Link a;
  Link b;
  Link c;
  b.AppendTo(a);
  c.AppendTo(a);

  EXPECT_EQ(b.Remove(), &a);
  EXPECT_EQ(a.Next(), &c);
  EXPECT_EQ(c.Previous(), &a);
  EXPECT_EQ(b.Next(), nullptr);
  EXPECT_EQ(b.Previous(), nullptr);

  EXPECT_EQ(a.Remove(), &c);
  EXPECT_EQ(c.Previous(), nullptr);

  EXPECT_EQ(c.Remove(), nullptr);
}

}  // namespace
}  // namespace game
