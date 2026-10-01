#include "ra/core/search.h"

#include "gtest/gtest.h"

namespace ra {

namespace {

TEST(IndexStorageTest, PreservesEntriesAcrossGrowthSortAndRemoval) {
  IndexClass<int> index;
  for (int i = 30; i >= 0; --i) {
    ASSERT_TRUE(index.Add_Index(i, i * 2));
  }
  EXPECT_EQ(index.Fetch_Index(17), 34);
  EXPECT_TRUE(index.Remove_Index(17));
  EXPECT_FALSE(index.Is_Present(17));
  EXPECT_EQ(index.Fetch_Index(30), 60);
  EXPECT_EQ(index.Fetch_Index(0), 0);
  index.Clear();
  EXPECT_EQ(index.Count(), 0);
  EXPECT_FALSE(index.Is_Present(30));
}

}  // namespace

}  // namespace ra
