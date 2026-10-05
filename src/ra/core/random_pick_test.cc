// Tests the random picks against a real Game: the synchronized generator
// belongs to its scenario.

#include "ra/core/random_pick.h"

#include <cstdint>

#include "game/sim/direction.h"
#include "gtest/gtest.h"
#include "ra/map/world.h"
#include "ra/startup/game_test_util.h"

namespace ra {
namespace {

using RandomPickTest = GameFixture;

constexpr uint32_t kSeed = 0x1234;
constexpr int kDraws = 200;

TEST_F(RandomPickTest, RandomPickStaysInRange) {
  TheScenario().sync_rng_.set_seed(kSeed);

  for (int draw = 0; draw < kDraws; draw++) {
    const int value = RandomPick(3, 9);
    EXPECT_GE(value, 3);
    EXPECT_LE(value, 9);
  }
}

TEST_F(RandomPickTest, RandomPickKeepsTheEnumType) {
  TheScenario().sync_rng_.set_seed(kSeed);

  const game::FacingType facing =
      RandomPick(game::FacingType::FACING_E, game::FacingType::FACING_S);

  EXPECT_GE(facing, game::FacingType::FACING_E);
  EXPECT_LE(facing, game::FacingType::FACING_S);
}

TEST_F(RandomPickTest, RandomPickRepeatsFromTheSameSeed) {
  TheScenario().sync_rng_.set_seed(kSeed);
  const int first = RandomPick(0, 1000);
  TheScenario().sync_rng_.set_seed(kSeed);

  EXPECT_EQ(RandomPick(0, 1000), first);
}

TEST_F(RandomPickTest, PercentChanceIsNeverAtZeroAndAlwaysAtHundred) {
  TheScenario().sync_rng_.set_seed(kSeed);

  for (int draw = 0; draw < kDraws; draw++) {
    EXPECT_FALSE(PercentChance(0));
    EXPECT_TRUE(PercentChance(100));
  }
}

TEST_F(RandomPickTest, PercentChanceDrawsFromTheSynchronizedGenerator) {
  TheScenario().sync_rng_.set_seed(kSeed);

  (void)PercentChance(50);

  EXPECT_NE(TheScenario().sync_rng_.seed(), kSeed);
}

TEST_F(RandomPickTest, LocalPicksLeaveTheSynchronizedGeneratorAlone) {
  TheScenario().sync_rng_.set_seed(kSeed);

  for (int draw = 0; draw < kDraws; draw++) {
    const int value = LocalRandomPick(-2, 2);
    EXPECT_GE(value, -2);
    EXPECT_LE(value, 2);
    EXPECT_FALSE(LocalPercentChance(0));
    EXPECT_TRUE(LocalPercentChance(100));
  }

  EXPECT_EQ(TheScenario().sync_rng_.seed(), kSeed);
}

}  // namespace
}  // namespace ra
