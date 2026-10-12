// Tests for the developer options dialog, played by a script on a headless
// screen.

#include "ra/screens/special_options.h"

#include "base/pooled.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/keyboard.h"
#include "engine/window/scripted_player.h"
#include "gtest/gtest.h"
#include "ra/assets/screen.h"
#include "ra/core/defines.h"
#include "ra/core/special_flags.h"
#include "ra/map/world.h"
#include "ra/net/event.h"
#include "ra/net/network.h"
#include "ra/objects/house.h"
#include "ra/startup/ui_test_util.h"

namespace ra {

namespace {

// Where the dialog puts its gadgets on the 640x400 screen: the dialog is 472
// by 324, centred, so its corner is at 84, 38.
constexpr int kSpeedBuildBoxX = 84 + 34 + 3;
constexpr int kSpeedBuildBoxY = 38 + 40 + 3;
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
  static SpecialFlags QueuedFlags() {
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
  TheSpecialFlags().speed_build = true;
  TheSpecialFlags().three_point_turns = true;
  player().At(2).Click(kOkX, kButtonsY);

  RunSpecialOptionsDialog();

  EXPECT_TRUE(player().finished());
  const SpecialFlags queued = QueuedFlags();
  EXPECT_TRUE(queued.speed_build);
  // The dialog has no box for these, but passes them on.
  EXPECT_TRUE(queued.three_point_turns);
  EXPECT_TRUE(queued.ore_grows);
}

TEST_F(SpecialOptionsDialogTest, ACheckedBoxIsQueuedButNotInstalled) {
  player()
      .At(2)
      .Click(kSpeedBuildBoxX, kSpeedBuildBoxY)
      .At(6)
      .Click(kOkX, kButtonsY);

  RunSpecialOptionsDialog();

  EXPECT_TRUE(QueuedFlags().speed_build);
  // Executing the event is what changes the game, on every machine at once.
  EXPECT_FALSE(TheSpecialFlags().speed_build);
}

TEST_F(SpecialOptionsDialogTest, ClickingACheckedBoxClearsIt) {
  TheSpecialFlags().speed_build = true;
  player()
      .At(2)
      .Click(kSpeedBuildBoxX, kSpeedBuildBoxY)
      .At(6)
      .Click(kOkX, kButtonsY);

  RunSpecialOptionsDialog();

  EXPECT_FALSE(QueuedFlags().speed_build);
}

TEST_F(SpecialOptionsDialogTest, CancelQueuesNothing) {
  player()
      .At(2)
      .Click(kSpeedBuildBoxX, kSpeedBuildBoxY)
      .At(6)
      .Click(kCancelX, kButtonsY);

  RunSpecialOptionsDialog();

  EXPECT_TRUE(player().finished());
  EXPECT_EQ(TheNetwork().out_list().Count(), 0);
}

TEST_F(SpecialOptionsDialogTest, EscCancelsLikeTheCancelButton) {
  player()
      .At(2)
      .Click(kSpeedBuildBoxX, kSpeedBuildBoxY)
      .At(6)
      .Key(engine::window::KN_ESC);

  RunSpecialOptionsDialog();

  EXPECT_TRUE(player().finished());
  EXPECT_EQ(TheNetwork().out_list().Count(), 0);
}

TEST_F(SpecialOptionsDialogTest, StaysOpenUntilAnswered) {
  player().set_frame_budget(30);
  player().At(2).Click(kSpeedBuildBoxX, kSpeedBuildBoxY);

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
            drawn += view.GetPixel(x, y).IsTransparent() ? 0 : 1;
          }
        }
      })
      .Key(engine::window::KN_ESC);

  RunSpecialOptionsDialog();

  EXPECT_GT(drawn, 0);
}

}  // namespace

}  // namespace ra
