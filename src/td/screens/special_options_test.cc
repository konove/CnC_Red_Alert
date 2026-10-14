// Tests for the developer options dialog, played by a script on a headless
// screen.

#include "td/screens/special_options.h"

#include "base/pooled.h"
#include "engine/window/keyboard.h"
#include "engine/window/scripted_player.h"
#include "gtest/gtest.h"
#include "td/core/defines.h"
#include "td/core/object_heaps.h"
#include "td/core/special_flags.h"
#include "td/map/world.h"
#include "td/net/event.h"
#include "td/net/network.h"
#include "td/objects/house.h"
#include "td/startup/ui_test_util.h"

namespace td {

namespace {

// Where the dialog puts its gadgets: it is 236 by 162 with its corner at 42,
// 19, and its check boxes are ten pixels apart.
constexpr int kCheckBoxX = 42 + 7 + 3;
constexpr int kSeparateHelipadBoxY = 19 + 20 + 3;
constexpr int kThreePointBoxY = 19 + 20 + (5 * 10) + 3;
constexpr int kOkX = 42 + 5 + 5;
constexpr int kCancelX = 42 + 236 - 50 + 5;
constexpr int kButtonsY = 19 + 162 - 15 + 3;

// The dialog in a single-player game: it sends its result as an event from
// the player's house, so there has to be one.
class SpecialOptionsDialogTest : public UiFixture {
 protected:
  SpecialOptionsDialogTest() {
    TheObjectHeaps().house().Reset(1);
    ThePlayer() = base::NewPooled<HouseClass>(HOUSE_GOOD);
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
    return events.First().data<EventClass::OptionsData>().Data;
  }
};

TEST_F(SpecialOptionsDialogTest, OkQueuesTheFlagsAsTheyWere) {
  TheSpecialFlags().mcv_deploy = true;
  TheSpecialFlags().named_civilians = true;
  // The dialog has no box for this one, but passes it on.
  TheSpecialFlags().capture_the_flag = true;
  player().At(2).Click(kOkX, kButtonsY);

  RunSpecialOptionsDialog();

  EXPECT_TRUE(player().finished());
  const SpecialFlags queued = QueuedFlags();
  EXPECT_TRUE(queued.mcv_deploy);
  EXPECT_TRUE(queued.named_civilians);
  EXPECT_TRUE(queued.capture_the_flag);
  EXPECT_FALSE(queued.separate_helipad);
  EXPECT_FALSE(queued.three_point_turns);
}

TEST_F(SpecialOptionsDialogTest, ACheckedBoxIsQueuedButNotInstalled) {
  player()
      .At(2)
      .Click(kCheckBoxX, kThreePointBoxY)
      .At(6)
      .Click(kOkX, kButtonsY);

  RunSpecialOptionsDialog();

  const SpecialFlags queued = QueuedFlags();
  EXPECT_TRUE(queued.three_point_turns);
  EXPECT_FALSE(queued.separate_helipad);
  // Executing the event is what changes the game, on every machine at once.
  EXPECT_FALSE(TheSpecialFlags().three_point_turns);
}

TEST_F(SpecialOptionsDialogTest, ClickingACheckedBoxClearsIt) {
  TheSpecialFlags().separate_helipad = true;
  player()
      .At(2)
      .Click(kCheckBoxX, kSeparateHelipadBoxY)
      .At(6)
      .Click(kOkX, kButtonsY);

  RunSpecialOptionsDialog();

  EXPECT_FALSE(QueuedFlags().separate_helipad);
}

TEST_F(SpecialOptionsDialogTest, CancelQueuesNothing) {
  player()
      .At(2)
      .Click(kCheckBoxX, kThreePointBoxY)
      .At(6)
      .Click(kCancelX, kButtonsY);

  RunSpecialOptionsDialog();

  EXPECT_TRUE(player().finished());
  EXPECT_EQ(TheNetwork().out_list().Count(), 0);
}

TEST_F(SpecialOptionsDialogTest, EscCancelsLikeTheCancelButton) {
  player()
      .At(2)
      .Click(kCheckBoxX, kThreePointBoxY)
      .At(6)
      .Key(engine::window::KN_ESC);

  RunSpecialOptionsDialog();

  EXPECT_TRUE(player().finished());
  EXPECT_EQ(TheNetwork().out_list().Count(), 0);
}

TEST_F(SpecialOptionsDialogTest, StaysOpenUntilAnswered) {
  player().set_frame_budget(30);
  player().At(2).Click(kCheckBoxX, kThreePointBoxY);

  EXPECT_THROW(RunSpecialOptionsDialog(), engine::window::ScriptOverrun);
  EXPECT_EQ(TheNetwork().out_list().Count(), 0);
}

}  // namespace

}  // namespace td
