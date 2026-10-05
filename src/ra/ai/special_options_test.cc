// Tests for the special option flags' starting values, for the mapping from
// a difficulty level to the player's and the computer's settings, and for
// the two dialogs, played by a script on a headless screen.

#include "ra/ai/special_options.h"

#include <stdexcept>

#include "base/installed.h"
#include "base/pooled.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/keyboard.h"
#include "engine/window/scripted_player.h"
#include "gtest/gtest.h"
#include "ra/assets/screen.h"
#include "ra/core/defines.h"
#include "ra/data/rules.h"
#include "ra/data/text_ids.h"
#include "ra/map/world.h"
#include "ra/net/event.h"
#include "ra/net/network.h"
#include "ra/objects/house.h"
#include "ra/startup/ui_test_util.h"

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

// Where the dialog puts its gadgets on the 640x400 screen: the dialog is 472
// by 324, centred, so its corner is at 84, 38.
constexpr int kThreePointBoxX = 84 + 34 + 3;
constexpr int kThreePointBoxY = 38 + 40 + 3;
constexpr int kSpeedBuildBoxX = 84 + 34 + 3;
constexpr int kSpeedBuildBoxY = 38 + 60 + 3;
constexpr int kOkX = 84 + 30 + 5;
constexpr int kCancelX = 84 + 472 - 120 + 5;
constexpr int kButtonsY = 38 + 324 - 30 + 5;

// The dialog in a single-player game: it sends its result as an event from
// the player's house, so there has to be one.
class SpecialOptionsDialogTest : public UiFixture {
 protected:
  SpecialOptionsDialogTest() {
    ThePlayer() = base::NewPooled<HouseClass>(HOUSE_GREECE);
  }

  // The flags the dialog queued for every machine to install, which is its
  // one output. Fails the test if it queued anything but one event.
  static SpecialOptions QueuedOptions() {
    auto& events = TheNetwork().out_list();
    EXPECT_EQ(events.Count(), 1);
    if (events.Count() == 0) {
      return {};
    }
    EXPECT_EQ(events.First().Type, EventClass::SPECIAL);
    return events.First().Data.Options.Data;
  }
};

TEST_F(SpecialOptionsDialogTest, OkQueuesTheFlagsAsTheyWere) {
  TheSpecialOptions().speed_build = true;
  player().At(2).Click(kOkX, kButtonsY);

  RunSpecialOptionsDialog();

  EXPECT_TRUE(player().finished());
  const SpecialOptions queued = QueuedOptions();
  EXPECT_TRUE(queued.speed_build);
  EXPECT_FALSE(queued.three_point_turns);
}

TEST_F(SpecialOptionsDialogTest, BothBoxesStartCheckedWhenBothFlagsAreOn) {
  TheSpecialOptions().speed_build = true;
  TheSpecialOptions().three_point_turns = true;
  player().At(2).Click(kOkX, kButtonsY);

  RunSpecialOptionsDialog();

  const SpecialOptions queued = QueuedOptions();
  EXPECT_TRUE(queued.speed_build);
  EXPECT_TRUE(queued.three_point_turns);
}

TEST_F(SpecialOptionsDialogTest, ACheckedBoxIsQueuedButNotInstalled) {
  player()
      .At(2)
      .Click(kThreePointBoxX, kThreePointBoxY)
      .At(6)
      .Click(kOkX, kButtonsY);

  RunSpecialOptionsDialog();

  EXPECT_TRUE(QueuedOptions().three_point_turns);
  // Executing the event is what changes the game, on every machine at once.
  EXPECT_FALSE(TheSpecialOptions().three_point_turns);
}

TEST_F(SpecialOptionsDialogTest, ClickingACheckedBoxClearsIt) {
  TheSpecialOptions().speed_build = true;
  player()
      .At(2)
      .Click(kSpeedBuildBoxX, kSpeedBuildBoxY)
      .At(6)
      .Click(kOkX, kButtonsY);

  RunSpecialOptionsDialog();

  EXPECT_FALSE(QueuedOptions().speed_build);
}

TEST_F(SpecialOptionsDialogTest, CancelQueuesNothing) {
  player()
      .At(2)
      .Click(kThreePointBoxX, kThreePointBoxY)
      .At(6)
      .Click(kCancelX, kButtonsY);

  RunSpecialOptionsDialog();

  EXPECT_TRUE(player().finished());
  EXPECT_EQ(TheNetwork().out_list().Count(), 0);
}

TEST_F(SpecialOptionsDialogTest, EscCancelsLikeTheCancelButton) {
  player()
      .At(2)
      .Click(kThreePointBoxX, kThreePointBoxY)
      .At(6)
      .Key(engine::window::KN_ESC);

  RunSpecialOptionsDialog();

  EXPECT_TRUE(player().finished());
  EXPECT_EQ(TheNetwork().out_list().Count(), 0);
}

TEST_F(SpecialOptionsDialogTest, StaysOpenUntilAnswered) {
  player().set_frame_budget(30);
  player().At(2).Click(kThreePointBoxX, kThreePointBoxY);

  EXPECT_THROW(RunSpecialOptionsDialog(), engine::window::ScriptOverrun);
  EXPECT_EQ(TheNetwork().out_list().Count(), 0);
}

TEST_F(SpecialOptionsDialogTest, DrawsOnTheVisiblePage) {
  int drawn = -1;
  player()
      .At(3)
      .Do([&drawn] {
        drawn = 0;
        PixelView& view = TheScreen().visible_view();
        for (int y = 0; y < Screen::kHeight; ++y) {
          for (int x = 0; x < Screen::kWidth; ++x) {
            drawn += view.GetPixel(x, y) != 0 ? 1 : 0;
          }
        }
      })
      .Key(engine::window::KN_ESC);

  RunSpecialOptionsDialog();

  EXPECT_GT(drawn, 0);
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

  EXPECT_EQ(PromptForDifficulty(false),
            (DifficultyChoice{.player = DifficultyLevel::kNormal,
                              .computer = DifficultyLevel::kNormal}));
}

TEST_F(DifficultyPromptTest, TheOkButtonAnswersToo) {
  player().At(2).Click(kPromptOkX, kPromptOkY);

  EXPECT_EQ(PromptForDifficulty(false),
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

  EXPECT_EQ(PromptForDifficulty(false), DifficultyForLevel(0));
}

TEST_F(DifficultyPromptTest, TheRightEndOfTheSliderIsHard) {
  player()
      .At(2)
      .Click(kSliderRightX, kSliderY)
      .At(6)
      .Key(engine::window::KN_RETURN);

  EXPECT_EQ(PromptForDifficulty(false), DifficultyForLevel(4));
}

// With fine difficulty the slider has five stops, one per level, instead of
// three spread over them.
TEST_F(DifficultyPromptTest, FineDifficultyStartsOnTheSameMiddle) {
  TheRules().IsFineDifficulty = true;
  player().At(2).Key(engine::window::KN_RETURN);

  EXPECT_EQ(PromptForDifficulty(false), DifficultyForLevel(2));
}

// An expansion mission is not part of a campaign, so the prompt keeps only
// its first sentence. Both forms have to survive being cut into lines.
TEST_F(DifficultyPromptTest, AnExpansionMissionGetsAShorterPrompt) {
  SetText(TXT_DIFFICULTY, "Choose a level. It lasts the whole campaign.");
  player().At(2).Key(engine::window::KN_RETURN);

  EXPECT_EQ(PromptForDifficulty(true), DifficultyForLevel(2));
}

TEST_F(DifficultyPromptTest, StaysOpenUntilAnswered) {
  player().set_frame_budget(30);
  player().At(2).Key(engine::window::KN_ESC);

  EXPECT_THROW(PromptForDifficulty(false), engine::window::ScriptOverrun);
}

}  // namespace

}  // namespace ra
