#include "game/sim/type_tally.h"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "gtest/gtest.h"

namespace game {

namespace {

TEST(TypeTallyTest, CountsUp) {
  TypeTally tally(3);
  tally.Increment(0);
  tally.Increment(2);
  tally.Increment(2);
  EXPECT_EQ(tally.Count(0), 1);
  EXPECT_EQ(tally.Count(1), 0);
  EXPECT_EQ(tally.Count(2), 2);
}

TEST(TypeTallyTest, CountsListsEveryTypeInOrder) {
  TypeTally tally(3);
  tally.Increment(1);
  tally.Increment(2);
  tally.Increment(2);
  EXPECT_TRUE(
      std::ranges::equal(tally.counts(), std::vector<int32_t>{0, 1, 2}));
}

}  // namespace

}  // namespace game
