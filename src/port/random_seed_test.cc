#include "port/random_seed.h"

#include <set>

#include "gtest/gtest.h"

namespace {

TEST(RandomSeedTest, IsNonNegativeAndVaries) {
  std::set<int> seeds;
  for (int i = 0; i < 16; ++i) {
    const int seed = port::RandomSeed();
    EXPECT_GE(seed, 0);
    seeds.insert(seed);
  }
  // Sixteen equal draws from the OS entropy source would be a broken source.
  EXPECT_GT(seeds.size(), 1U);
}

}  // namespace
