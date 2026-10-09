#include "game/ui/link.h"

#include "gtest/gtest.h"

namespace game {
namespace {

TEST(LinkTest, ANewNodeIsAListOfOne) {
  const Link node;

  EXPECT_EQ(node.Next(), nullptr);
  EXPECT_EQ(node.Previous(), nullptr);
  EXPECT_EQ(&node.HeadOfList(), &node);
  EXPECT_EQ(&node.TailOfList(), &node);
}

TEST(LinkTest, AppendToAddsBehindTheTailFromAnyNode) {
  Link a;
  Link b;
  Link c;

  EXPECT_EQ(&b.AppendTo(a), &a);
  // `b` stands for the list as well as `a` does.
  EXPECT_EQ(&c.AppendTo(b), &a);

  EXPECT_EQ(a.Next(), &b);
  EXPECT_EQ(b.Next(), &c);
  EXPECT_EQ(c.Next(), nullptr);
  EXPECT_EQ(c.Previous(), &b);
  EXPECT_EQ(b.Previous(), &a);
}

TEST(LinkTest, PrependToAddsInFrontOfTheHeadFromAnyNode) {
  Link a;
  Link b;
  Link c;
  b.AppendTo(a);

  // `b` is the tail; the new node still lands in front of `a`.
  EXPECT_EQ(&c.PrependTo(b), &c);

  EXPECT_EQ(c.Previous(), nullptr);
  EXPECT_EQ(c.Next(), &a);
  EXPECT_EQ(a.Previous(), &c);
  EXPECT_EQ(a.Next(), &b);
}

TEST(LinkTest, InsertAfterAddsInTheMiddleOrAtTheEnd) {
  Link a;
  Link b;
  Link c;
  Link d;
  b.AppendTo(a);

  EXPECT_EQ(&c.InsertAfter(a), &a);
  EXPECT_EQ(a.Next(), &c);
  EXPECT_EQ(c.Previous(), &a);
  EXPECT_EQ(c.Next(), &b);
  EXPECT_EQ(b.Previous(), &c);

  EXPECT_EQ(&d.InsertAfter(b), &a);
  EXPECT_EQ(b.Next(), &d);
  EXPECT_EQ(d.Previous(), &b);
  EXPECT_EQ(d.Next(), nullptr);
}

TEST(LinkTest, TheEndsAreFoundFromAnyNode) {
  Link a;
  Link b;
  Link c;
  b.AppendTo(a);
  c.AppendTo(a);
  const Link& middle = b;

  EXPECT_EQ(&b.HeadOfList(), &a);
  EXPECT_EQ(&b.TailOfList(), &c);
  EXPECT_EQ(&middle.HeadOfList(), &a);
  EXPECT_EQ(&middle.TailOfList(), &c);
}

TEST(LinkTest, ForgetNeighborsLeavesTheNeighborsPointingAtTheNode) {
  Link a;
  Link b;
  Link c;
  b.AppendTo(a);
  c.AppendTo(a);

  b.ForgetNeighbors();

  EXPECT_EQ(b.Next(), nullptr);
  EXPECT_EQ(b.Previous(), nullptr);
  EXPECT_EQ(a.Next(), &b);
  EXPECT_EQ(c.Previous(), &b);

  // Dropping a list means forgetting every node of it.
  a.ForgetNeighbors();
  c.ForgetNeighbors();
}

TEST(LinkTest, DestroyingANodeTakesItOutOfItsList) {
  Link a;
  Link c;
  {
    Link b;
    b.AppendTo(a);
    c.AppendTo(a);
  }

  EXPECT_EQ(a.Next(), &c);
  EXPECT_EQ(c.Previous(), &a);
}

// Adding a node that is still linked is a caller's mistake. A build with
// DCHECKs stops at it.
TEST(LinkDeathTest, AddingANodeThatIsStillLinkedIsCaughtInDebugBuilds) {
  // Without DCHECKs the add goes through and leaves `a` pointing at `b`, so
  // every node forgets its neighbours before it is destroyed.
  const auto add_linked_node = [](Link& (Link::*add)(Link&)) {
    Link a;
    Link b;
    Link other;
    b.AppendTo(a);
    (b.*add)(other);
    a.ForgetNeighbors();
    b.ForgetNeighbors();
    other.ForgetNeighbors();
  };

  EXPECT_DEBUG_DEATH(add_linked_node(&Link::AppendTo), "still in one");
  EXPECT_DEBUG_DEATH(add_linked_node(&Link::PrependTo), "still in one");
  EXPECT_DEBUG_DEATH(add_linked_node(&Link::InsertAfter), "still in one");
}

// Only a build without DCHECKs lets the mistake through, and there it can close
// the chain into a ring. The walks to the ends must still come back.
#ifdef NDEBUG
TEST(LinkTest, AWalkDownARingStopsAfterOneLap) {
  Link a;
  Link b;
  b.AppendTo(a);
  b.PrependTo(a);  // a -> b -> a

  EXPECT_EQ(&a.TailOfList(), &a);

  a.ForgetNeighbors();
  b.ForgetNeighbors();
}

TEST(LinkTest, AWalkUpARingStopsAfterOneLap) {
  Link a;
  Link b;
  b.AppendTo(a);
  a.InsertAfter(b);  // a <- b <- a

  EXPECT_EQ(&a.HeadOfList(), &a);

  a.ForgetNeighbors();
  b.ForgetNeighbors();
}
#endif  // NDEBUG

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
