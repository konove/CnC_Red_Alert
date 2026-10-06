// Tests for the mapping from a difficulty level to the player's and the
// computer's settings, and for the difficulty prompt, played by a script on a
// headless screen.

#include "ra/screens/difficulty.h"

#include <stdexcept>

#include "engine/window/keyboard.h"
#include "engine/window/scripted_player.h"
#include "gtest/gtest.h"
#include "ra/core/defines.h"
#include "ra/data/rules.h"
#include "ra/data/text_ids.h"
#include "ra/startup/ui_test_util.h"

namespace ra {

namespace {

// The scale handicaps both sides at once: at the easy end the player's game
// is easy and the computer's side plays at hard, which is the weak setting
// for a computer house.
TEST(DifficultyForLevelTest, TheEndsTradeThePlayerAgainstTheComputer) {
  EXPECT_EQ(DifficultyForLevel(0),
            (DifficultyChoice{.player = DifficultyLevel::kEasy,
                              .computer = DifficultyLevel::kHard}));
  EXPECT_EQ(DifficultyForLevel(4),
            (DifficultyChoice{.player = DifficultyLevel::kHard,
                              .computer = DifficultyLevel::kEasy}));
}

TEST(DifficultyForLevelTest, TheMiddleIsNormalOnBothSides) {
  EXPECT_EQ(DifficultyForLevel(2),
            (DifficultyChoice{.player = DifficultyLevel::kNormal,
                              .computer = DifficultyLevel::kNormal}));
}

// The two fine levels only move the computer's side.
TEST(DifficultyForLevelTest, TheInBetweenLevelsKeepThePlayerOnNormal) {
  EXPECT_EQ(DifficultyForLevel(1),
            (DifficultyChoice{.player = DifficultyLevel::kNormal,
                              .computer = DifficultyLevel::kHard}));
  EXPECT_EQ(DifficultyForLevel(3),
            (DifficultyChoice{.player = DifficultyLevel::kNormal,
                              .computer = DifficultyLevel::kEasy}));
}

TEST(DifficultyForLevelTest, ALevelOffTheScaleIsRejected) {
  EXPECT_THROW(DifficultyForLevel(5), std::out_of_range);
  EXPECT_THROW(DifficultyForLevel(-1), std::out_of_range);
}

// The prompt is 500 by 160, centred, so its corner is at 70, 120; the slider
// is 420 wide from 110, 222 and the OK button is at 470, 244.
constexpr int kSliderLeftX = 110 + 4;
constexpr int kSliderRightX = 110 + 420 - 4;
constexpr int kSliderY = 222 + 8;
constexpr int kPromptOkX = 470 + 5;
constexpr int kPromptOkY = 244 + 5;

using DifficultyPromptTest = UiFixture;

TEST_F(DifficultyPromptTest, ReturnTakesTheMiddleOfTheSlider) {
  player().At(2).Key(engine::window::KN_RETURN);

  EXPECT_EQ(AskDifficulty(false),
            (DifficultyChoice{.player = DifficultyLevel::kNormal,
                              .computer = DifficultyLevel::kNormal}));
}

TEST_F(DifficultyPromptTest, TheOkButtonAnswersToo) {
  player().At(2).Click(kPromptOkX, kPromptOkY);

  EXPECT_EQ(AskDifficulty(false),
            (DifficultyChoice{.player = DifficultyLevel::kNormal,
                              .computer = DifficultyLevel::kNormal}));
  EXPECT_TRUE(player().finished());
}

TEST_F(DifficultyPromptTest, TheLeftEndOfTheSliderIsEasy) {
  player()
      .At(2)
      .Click(kSliderLeftX, kSliderY)
      .At(6)
      .Key(engine::window::KN_RETURN);

  EXPECT_EQ(AskDifficulty(false), DifficultyForLevel(0));
}

TEST_F(DifficultyPromptTest, TheRightEndOfTheSliderIsHard) {
  player()
      .At(2)
      .Click(kSliderRightX, kSliderY)
      .At(6)
      .Key(engine::window::KN_RETURN);

  EXPECT_EQ(AskDifficulty(false), DifficultyForLevel(4));
}

// With fine difficulty the slider has five stops, one per level, instead of
// three spread over them.
TEST_F(DifficultyPromptTest, FineDifficultyStartsOnTheSameMiddle) {
  TheRules().IsFineDifficulty = true;
  player().At(2).Key(engine::window::KN_RETURN);

  EXPECT_EQ(AskDifficulty(false), DifficultyForLevel(2));
}

// An expansion mission is not part of a campaign, so the prompt keeps only
// its first sentence. Both forms have to survive being cut into lines.
TEST_F(DifficultyPromptTest, AnExpansionMissionGetsAShorterPrompt) {
  SetText(TXT_DIFFICULTY, "Choose a level. It lasts the whole campaign.");
  player().At(2).Key(engine::window::KN_RETURN);

  EXPECT_EQ(AskDifficulty(true), DifficultyForLevel(2));
}

TEST_F(DifficultyPromptTest, StaysOpenUntilAnswered) {
  player().set_frame_budget(30);
  player().At(2).Key(engine::window::KN_ESC);

  EXPECT_THROW(AskDifficulty(false), engine::window::ScriptOverrun);
}

}  // namespace

}  // namespace ra
