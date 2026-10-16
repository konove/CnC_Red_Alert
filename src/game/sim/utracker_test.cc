#include "game/sim/utracker.h"

#include <cstddef>
#include <vector>

#include "gtest/gtest.h"

namespace game {

namespace {

TEST(TypeTallyTest, CountsUpAndDownAndClears) {
  TypeTally tally(3);
  tally.Increment(0);
  tally.Increment(2);
  tally.Increment(2);
  EXPECT_EQ(tally.Count(0), 1);
  EXPECT_EQ(tally.Count(1), 0);
  EXPECT_EQ(tally.Count(2), 2);
  tally.Decrement(2);
  EXPECT_EQ(tally.Count(2), 1);
  tally.Clear();
  EXPECT_EQ(tally.Count(0), 0);
  EXPECT_EQ(tally.Count(2), 0);
}

// The statistics server reads each count as a 32-bit big-endian integer.
TEST(TypeTallyTest, NetworkBytesAreBigEndian) {
  TypeTally tally(2);
  for (int i = 0; i < 258; ++i) {
    tally.Increment(1);
  }
  tally.Decrement(0);
  EXPECT_EQ(
      tally.NetworkBytes(),
      (std::vector<std::byte>{std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF},
                              std::byte{0xFF}, std::byte{0x00}, std::byte{0x00},
                              std::byte{0x01}, std::byte{0x02}}));
}

}  // namespace

}  // namespace game
