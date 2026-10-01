// Tests OwnedVector, the session's owning list.

#include "ra/core/owned_vector.h"

#include <memory>

#include "gtest/gtest.h"

namespace {

// Counts live instances so the tests can see the list destroy what it owns.
struct Tracked {
  explicit Tracked(int initial) : value(initial) { ++live; }
  ~Tracked() { --live; }
  Tracked(const Tracked&) = delete;
  Tracked& operator=(const Tracked&) = delete;
  Tracked(Tracked&&) = delete;
  Tracked& operator=(Tracked&&) = delete;

  int value;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): counter
  static inline int live = 0;
};

class OwnedVectorTest : public testing::Test {
 protected:
  void SetUp() override { Tracked::live = 0; }
  void TearDown() override { EXPECT_EQ(Tracked::live, 0); }
};

TEST_F(OwnedVectorTest, AddReturnsTheObjectAndKeepsOrder) {
  OwnedVector<Tracked> list;
  Tracked* first = list.Add(std::make_unique<Tracked>(1));
  list.Add(std::make_unique<Tracked>(2));
  EXPECT_EQ(list.Count(), 2);
  EXPECT_EQ(list.at(0), first);
  EXPECT_EQ(list.at(1)->value, 2);
  EXPECT_EQ(Tracked::live, 2);
}

TEST_F(OwnedVectorTest, IdFindsAnObjectOrReturnsMinusOne) {
  OwnedVector<Tracked> list;
  list.Add(std::make_unique<Tracked>(1));
  const Tracked* second = list.Add(std::make_unique<Tracked>(2));
  const Tracked stranger(3);
  EXPECT_EQ(list.ID(second), 1);
  EXPECT_EQ(list.ID(&stranger), -1);
}

TEST_F(OwnedVectorTest, DeleteByPointerDestroysAndKeepsTheRestInOrder) {
  OwnedVector<Tracked> list;
  list.Add(std::make_unique<Tracked>(1));
  Tracked* middle = list.Add(std::make_unique<Tracked>(2));
  list.Add(std::make_unique<Tracked>(3));
  EXPECT_TRUE(list.Delete(middle));
  EXPECT_EQ(Tracked::live, 2);
  ASSERT_EQ(list.Count(), 2);
  EXPECT_EQ(list.at(0)->value, 1);
  EXPECT_EQ(list.at(1)->value, 3);

  const Tracked stranger(4);
  EXPECT_FALSE(list.Delete(&stranger));
  EXPECT_EQ(list.Count(), 2);
}

TEST_F(OwnedVectorTest, DeleteByIndexDestroysTheObject) {
  OwnedVector<Tracked> list;
  list.Add(std::make_unique<Tracked>(1));
  list.Add(std::make_unique<Tracked>(2));
  list.Delete(0);
  EXPECT_EQ(Tracked::live, 1);
  ASSERT_EQ(list.Count(), 1);
  EXPECT_EQ(list.at(0)->value, 2);
}

TEST_F(OwnedVectorTest, ClearAndDestructionDestroyEverything) {
  {
    OwnedVector<Tracked> list;
    list.Add(std::make_unique<Tracked>(1));
    list.Add(std::make_unique<Tracked>(2));
    list.Clear();
    EXPECT_EQ(list.Count(), 0);
    EXPECT_EQ(Tracked::live, 0);
    list.Add(std::make_unique<Tracked>(3));
  }
  EXPECT_EQ(Tracked::live, 0);
}

TEST_F(OwnedVectorTest, SortReordersTheObjectsNotCopies) {
  OwnedVector<Tracked> list;
  Tracked* three = list.Add(std::make_unique<Tracked>(3));
  list.Add(std::make_unique<Tracked>(1));
  list.Add(std::make_unique<Tracked>(2));
  list.Sort(
      [](const Tracked& a, const Tracked& b) { return a.value < b.value; });
  EXPECT_EQ(list.at(0)->value, 1);
  EXPECT_EQ(list.at(1)->value, 2);
  EXPECT_EQ(list.at(2), three);
}

TEST(OwnedVectorDeathTest, AtOutOfRangeFails) {
  OwnedVector<Tracked> list;
  // The switches and formatted diagnostics are inside GoogleTest's macro.
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(static_cast<void>(list.at(0)), "Check failed");
}

}  // namespace
