#include "game/sim/utracker.h"

#include "gtest/gtest.h"

namespace game {

namespace {

TEST(TypeTallyTest, NetworkRoundTripPreservesEveryCounter) {
  TypeTally tally(3);
  EXPECT_EQ(tally.Get_All_Totals().size(), 3);
  tally.Increment(0);
  tally.Increment(2);
  tally.Increment(2);
  tally.To_Network_Format();
  tally.To_Network_Format();
  tally.To_PC_Format();
  tally.To_PC_Format();
  EXPECT_EQ(tally.Count(0), 1);
  EXPECT_EQ(tally.Count(1), 0);
  EXPECT_EQ(tally.Count(2), 2);
  tally.Decrement(2);
  EXPECT_EQ(tally.Count(2), 1);
  tally.Clear();
  EXPECT_EQ(tally.Count(0), 0);
  EXPECT_EQ(tally.Count(2), 0);
}

// The RA statistics clear the bought counts while they are still in network
// format and count the units left into them; those must be swapped again.
TEST(TypeTallyTest, ClearReturnsToHostFormat) {
  TypeTally tally(1);
  tally.Increment(0);
  tally.To_Network_Format();
  tally.Clear();
  tally.Increment(0);
  tally.To_Network_Format();
  tally.To_PC_Format();
  EXPECT_EQ(tally.Count(0), 1);
}

}  // namespace

}  // namespace game
