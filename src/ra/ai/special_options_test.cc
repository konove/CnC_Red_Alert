// Tests for the special option flags' starting values and for the mapping
// from a difficulty level to the player's and the computer's settings. The
// two dialogs need the display and the input loop and are not covered here.

#include "ra/ai/special_options.h"

#include <stdexcept>

#include "base/installed.h"
#include "gtest/gtest.h"
#include "ra/core/defines.h"

namespace ra {

namespace {

TEST(SpecialOptionsTest, ResetLeavesOnlyTheOreFlagsOn) {
  SpecialOptions options{};
  options.shroud_regrows = true;
  options.speed_build = true;
  options.from_install = true;
  options.capture_the_flag = true;
  options.inert_weapons = true;
  options.three_point_turns = true;

  options.Reset();

  EXPECT_FALSE(options.shroud_regrows);
  EXPECT_FALSE(options.speed_build);
  EXPECT_FALSE(options.from_install);
  EXPECT_FALSE(options.capture_the_flag);
  EXPECT_FALSE(options.inert_weapons);
  EXPECT_FALSE(options.three_point_turns);
  EXPECT_TRUE(options.ore_grows);
  EXPECT_TRUE(options.ore_spreads);
}

TEST(SpecialOptionsTest, TheSpecialOptionsIsTheInstalledInstance) {
  SpecialOptions options{};
  const base::Installed<SpecialOptions>::Scope scope(options);

  TheSpecialOptions().inert_weapons = true;

  EXPECT_EQ(&TheSpecialOptions(), &options);
  EXPECT_TRUE(options.inert_weapons);
}

// The scale handicaps both sides at once: at the easy end the player's game
// is easy and the computer's side plays at hard, which is the weak setting
// for a computer house.
TEST(DifficultyForLevelTest, TheEndsTradeThePlayerAgainstTheComputer) {
  EXPECT_EQ(DifficultyForLevel(0),
            (DifficultyChoice{.player = DIFF_EASY, .computer = DIFF_HARD}));
  EXPECT_EQ(DifficultyForLevel(4),
            (DifficultyChoice{.player = DIFF_HARD, .computer = DIFF_EASY}));
}

TEST(DifficultyForLevelTest, TheMiddleIsNormalOnBothSides) {
  EXPECT_EQ(DifficultyForLevel(2),
            (DifficultyChoice{.player = DIFF_NORMAL, .computer = DIFF_NORMAL}));
}

// The two fine levels only move the computer's side.
TEST(DifficultyForLevelTest, TheInBetweenLevelsKeepThePlayerOnNormal) {
  EXPECT_EQ(DifficultyForLevel(1),
            (DifficultyChoice{.player = DIFF_NORMAL, .computer = DIFF_HARD}));
  EXPECT_EQ(DifficultyForLevel(3),
            (DifficultyChoice{.player = DIFF_NORMAL, .computer = DIFF_EASY}));
}

TEST(DifficultyForLevelTest, ALevelOffTheScaleIsRejected) {
  EXPECT_THROW(DifficultyForLevel(5), std::out_of_range);
  EXPECT_THROW(DifficultyForLevel(-1), std::out_of_range);
}

}  // namespace

}  // namespace ra
