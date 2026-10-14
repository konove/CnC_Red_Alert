// Tests the container search and removal helpers.

#include "base/algorithm.h"

#include <vector>

#include "gtest/gtest.h"

namespace {

TEST(IndexOfTest, ReturnsTheFirstMatchingIndex) {
  const std::vector<int> values = {4, 7, 9, 7};
  EXPECT_EQ(base::IndexOf(values, 7), 1);
  EXPECT_EQ(base::IndexOf(values, 4), 0);
}

TEST(IndexOfTest, ReturnsMinusOneWhenAbsent) {
  const std::vector<int> values = {4, 7};
  EXPECT_EQ(base::IndexOf(values, 5), -1);
  EXPECT_EQ(base::IndexOf(std::vector<int>{}, 5), -1);
}

// ObjectHeapBase finds its free slot this way, through vector<bool>'s proxy
// iterators.
TEST(IndexOfTest, SearchesAVectorOfBool) {
  EXPECT_EQ(base::IndexOf(std::vector<bool>{true, true, false, true}, false),
            2);
  EXPECT_EQ(base::IndexOf(std::vector<bool>{true, true}, false), -1);
}

TEST(EraseFirstTest, RemovesOnlyTheFirstMatchAndKeepsOrder) {
  std::vector<int> values = {1, 7, 2, 7, 3};
  EXPECT_TRUE(base::EraseFirst(values, 7));
  EXPECT_EQ(values, (std::vector<int>{1, 2, 7, 3}));
}

TEST(EraseFirstTest, ReturnsFalseAndLeavesTheVectorWhenAbsent) {
  std::vector<int> values = {1, 2};
  EXPECT_FALSE(base::EraseFirst(values, 9));
  EXPECT_EQ(values, (std::vector<int>{1, 2}));
}

}  // namespace
