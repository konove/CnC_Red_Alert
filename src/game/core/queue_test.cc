// Tests QueueClass, the fixed-capacity FIFO the network event lists use.

#include "game/core/queue.h"

#include "gtest/gtest.h"

namespace game {

namespace {

TEST(QueueTest, AccessFollowsWrappedLogicalOrder) {
  QueueClass<int, 3> queue;
  ASSERT_TRUE(queue.Add(10));
  ASSERT_TRUE(queue.Add(20));
  EXPECT_EQ(queue.Next(), 1);
  ASSERT_TRUE(queue.Add(30));
  ASSERT_TRUE(queue.Add(40));
  EXPECT_EQ(queue.at(0), 20);
  EXPECT_EQ(queue.at(1), 30);
  EXPECT_EQ(queue.at(2), 40);
  queue.at(1) = 31;
  EXPECT_EQ(queue.at(1), 31);
}

TEST(QueueTest, AddFailsOnceFullAndKeepsTheContents) {
  QueueClass<int, 2> queue;
  ASSERT_TRUE(queue.Add(1));
  ASSERT_TRUE(queue.Add(2));
  EXPECT_FALSE(queue.Add(3));
  EXPECT_EQ(queue.Count(), 2);
  EXPECT_EQ(queue.First(), 1);
  EXPECT_EQ(queue.at(1), 2);
}

// The size need not be a power of two: indexing wraps with %, so a queue of
// five stays first-in, first-out across several trips round its storage.
TEST(QueueTest, NonPowerOfTwoSizeStaysFifoAcrossWraps) {
  QueueClass<int, 5> queue;
  int next_in = 0;
  int next_out = 0;
  for (int round = 0; round < 4; ++round) {
    while (queue.Add(next_in)) {
      ++next_in;
    }
    ASSERT_EQ(queue.Count(), 5);
    for (int i = 0; i < 3; ++i) {
      EXPECT_EQ(queue.First(), next_out);
      ++next_out;
      queue.Next();
    }
  }
  EXPECT_EQ(queue.Count(), 2);
  EXPECT_EQ(queue.at(0), next_out);
  EXPECT_EQ(queue.at(1), next_out + 1);
}

TEST(QueueTest, InitEmptiesTheQueue) {
  QueueClass<int, 4> queue;
  ASSERT_TRUE(queue.Add(1));
  ASSERT_TRUE(queue.Add(2));
  queue.Init();
  EXPECT_EQ(queue.Count(), 0);
  EXPECT_EQ(queue.Next(), 0);
}

TEST(QueueDeathTest, RejectsIndicesOutsideTheActiveEntries) {
  QueueClass<int, 3> queue;
  // GoogleTest's death-test macro formats subprocess diagnostics with libc.
  // NOLINTBEGIN(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH((void)queue.at(0), "Check failed");
  ASSERT_TRUE(queue.Add(10));
  ASSERT_TRUE(queue.Add(20));
  EXPECT_EQ(queue.Next(), 1);
  // Both indices formerly wrapped to valid storage outside the active queue.
  EXPECT_DEATH((void)queue.at(-1), "Check failed");
  EXPECT_DEATH((void)queue.at(1), "Check failed");
  EXPECT_DEATH((void)queue.at(4), "Check failed");
  // NOLINTEND(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
}

}  // namespace

}  // namespace game
