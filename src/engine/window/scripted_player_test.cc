// Tests for ScriptedPlayer, each against a small loop of the shape the games'
// dialogs have: end a frame, read one event, act on it.

#include "engine/window/scripted_player.h"

#include <vector>

#include "base/array.h"
#include "base/installed.h"
#include "engine/platform/manual_clock.h"
#include "engine/platform/timer.h"
#include "engine/window/display.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/headless_display.h"
#include "engine/window/keyboard.h"
#include "gtest/gtest.h"

namespace engine::window {
namespace {

// A headless Display, installed so that KeyBuffer::Peek() pumps it as it does
// in the games, with the devices and the clock a player needs.
class ScriptedPlayerTest : public ::testing::Test {
 protected:
  ScriptedPlayerTest() { display_.SetVideoMode(8, 4); }

  // One turn of a dialog's loop: ends the frame, then returns the event
  // waiting in the queue, empty if there is none.
  InputEvent Turn() {
    display_.EndFrame();
    return keyboard_.Peek() ? keyboard_.Read() : InputEvent{};
  }

  // Runs turns until `key` is pressed and returns how many it took.
  int TurnsUntilPressed(const KeyNumber key) {
    int turns = 1;
    while (Turn().key != key) {
      ++turns;
    }
    return turns;
  }

  HeadlessDisplay display_;
  base::Installed<Display>::Scope display_scope_{display_};
  KeyBuffer keyboard_;
  HardwareCursor mouse_{48, 48};
  engine::platform::ManualClock clock_;
  ScriptedPlayer player_{display_, keyboard_, mouse_, clock_};
};

TEST_F(ScriptedPlayerTest, AKeyArrivesOnItsFrame) {
  player_.At(3).Key(KN_RETURN);

  EXPECT_EQ(TurnsUntilPressed(KN_RETURN), 3);
  EXPECT_EQ(player_.frame(), 3);
}

TEST_F(ScriptedPlayerTest, AKeyIsAPressThenARelease) {
  player_.Key(KN_A);

  const InputEvent press = Turn();
  const InputEvent release = Turn();

  EXPECT_EQ(press.key, KN_A);
  EXPECT_EQ(KeyCode(release.key), KN_A);
  EXPECT_TRUE(IsRelease(release.key));
  EXPECT_FALSE(Turn());
}

// Without SDL's keyboard there is no layout to say what a key types, so the
// keys that type the same everywhere carry their character with them.
TEST_F(ScriptedPlayerTest, TheEditingKeysCarryTheirCharacter) {
  player_.Key(KN_RETURN).Key(KN_BACKSPACE).Key(KN_SPACE).Key(KN_F1);

  EXPECT_EQ(Turn().character, '\r');
  Turn();
  EXPECT_EQ(Turn().character, '\b');
  Turn();
  EXPECT_EQ(Turn().character, ' ');
  Turn();
  EXPECT_EQ(Turn().character, '\0');
}

TEST_F(ScriptedPlayerTest, TypingPressesTheKeyOfEachCharacter) {
  player_.Type("aZ 09");

  const KeyNumber keys[] = {KN_A, KN_Z, KN_SPACE, KN_0, KN_9};
  const char characters[] = {'a', 'Z', ' ', '0', '9'};
  for (int i = 0; i < 5; ++i) {
    const InputEvent press = Turn();
    EXPECT_EQ(press.key, base::At(keys, i));
    EXPECT_EQ(press.character, base::At(characters, i));
    EXPECT_TRUE(IsRelease(Turn().key));
  }
  EXPECT_FALSE(Turn());
}

TEST_F(ScriptedPlayerTest, TypingPunctuationIsAProgrammingError) {
  player_.Type("a!");

  // The switch is inside GoogleTest's macro.
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(Turn(), "letters, digits and spaces");
}

TEST_F(ScriptedPlayerTest, StepsBeforeAnyAtHappenOnTheFirstFrame) {
  player_.Key(KN_ESC);

  EXPECT_EQ(TurnsUntilPressed(KN_ESC), 1);
}

TEST_F(ScriptedPlayerTest, AClickIsAPressThenAReleaseWhereItWasAimed) {
  player_.Click(120, 80);

  const InputEvent press = Turn();
  const InputEvent release = Turn();

  EXPECT_TRUE(press.IsPress(MouseButton::kLeft));
  EXPECT_EQ(press.x, 120);
  EXPECT_EQ(press.y, 80);
  EXPECT_TRUE(release.IsRelease(MouseButton::kLeft));
  EXPECT_EQ(mouse_.x(), 120);
  EXPECT_EQ(mouse_.y(), 80);
}

TEST_F(ScriptedPlayerTest, AClickCanUseTheRightButton) {
  player_.Click(1, 2, MouseButton::kRight);

  EXPECT_TRUE(Turn().IsPress(MouseButton::kRight));
}

TEST_F(ScriptedPlayerTest, ADragIsAPressMovesAndARelease) {
  player_.At(1).Press(10, 10).At(2).MoveTo(30, 10).At(3).Release(50, 10);

  EXPECT_TRUE(Turn().IsPress(MouseButton::kLeft));
  EXPECT_EQ(mouse_.x(), 10);
  EXPECT_FALSE(Turn());
  EXPECT_EQ(mouse_.x(), 30);
  const InputEvent release = Turn();
  EXPECT_TRUE(release.IsRelease(MouseButton::kLeft));
  EXPECT_EQ(release.x, 50);
}

// The games ask whether a button is held separately from reading the click,
// to tell a drag from a click.
TEST_F(ScriptedPlayerTest, AButtonIsHeldFromItsPressToItsRelease) {
  player_.At(1).Press(10, 10).At(3).Release(50, 10);

  EXPECT_FALSE(IsLeftButtonDown());
  Turn();
  EXPECT_TRUE(IsLeftButtonDown());
  EXPECT_FALSE(IsRightButtonDown());
  Turn();
  EXPECT_TRUE(IsLeftButtonDown());
  Turn();
  EXPECT_FALSE(IsLeftButtonDown());
}

TEST_F(ScriptedPlayerTest, AClickLeavesNoButtonHeld) {
  player_.Click(10, 10).Click(20, 20, MouseButton::kRight);

  Turn();

  EXPECT_FALSE(IsLeftButtonDown());
  EXPECT_FALSE(IsRightButtonDown());
}

TEST_F(ScriptedPlayerTest, StepsOfOneFramePlayInTheOrderQueued) {
  std::vector<int> order;
  player_.At(2).Do([&order] { order.push_back(1); }).Do([&order] {
    order.push_back(2);
  });

  Turn();
  EXPECT_TRUE(order.empty());
  Turn();

  EXPECT_EQ(order, (std::vector<int>{1, 2}));
}

TEST_F(ScriptedPlayerTest, IsFinishedOnceEveryStepHasPlayed) {
  player_.At(2).Key(KN_A);

  EXPECT_FALSE(player_.finished());
  Turn();
  EXPECT_FALSE(player_.finished());
  Turn();
  EXPECT_TRUE(player_.finished());
}

// What lets a fade or a timed wait finish at once: its timer counts frames.
TEST_F(ScriptedPlayerTest, TheClockMovesATickAFrame) {
  const engine::platform::Timer<engine::platform::SystemTickSource> wait{3};
  int frames = 0;
  while (wait.HasTimeLeft()) {
    display_.EndFrame();
    ++frames;
  }

  EXPECT_EQ(frames, 3);
  EXPECT_EQ(clock_.Ticks(), 3);
}

TEST_F(ScriptedPlayerTest, ALoopTheScriptCannotEndIsCutShort) {
  player_.set_frame_budget(5);
  player_.At(2).Key(KN_A);

  // Waits for a key the script never presses.
  EXPECT_THROW(TurnsUntilPressed(KN_RETURN), ScriptOverrun);
  EXPECT_EQ(player_.frame(), 8);
}

TEST_F(ScriptedPlayerTest, AnEmptyScriptCutsALoopShortAfterTheBudget) {
  player_.set_frame_budget(5);

  EXPECT_THROW(TurnsUntilPressed(KN_RETURN), ScriptOverrun);
  EXPECT_EQ(player_.frame(), 6);
}

// A loop that polls the keyboard and never presents would never see a frame's
// step, so it is cut short by the count of its polls instead.
TEST_F(ScriptedPlayerTest, ALoopThatNeverEndsAFrameIsCutShort) {
  player_.Key(KN_RETURN);

  EXPECT_THROW(
      {
        while (!keyboard_.Peek()) {
        }
      },
      ScriptOverrun);
  EXPECT_EQ(player_.frame(), 0);
}

// What a scripted run of the whole game asks for: the loop that only polls
// gets its frames, and with them the key it is waiting for.
TEST_F(ScriptedPlayerTest, IdlePumpsCanEndFramesForALoopThatOnlyPolls) {
  player_.set_idle_pumps_per_frame(4);
  player_.At(3).Key(KN_RETURN);

  int polls = 0;
  while (!keyboard_.Peek()) {
    ++polls;
  }

  EXPECT_EQ(player_.frame(), 3);
  // A frame every few polls, not one every hundred thousand.
  EXPECT_LT(polls, 20);
}

TEST_F(ScriptedPlayerTest, IdlePumpsEndNoFrameForALoopThatPresents) {
  player_.set_idle_pumps_per_frame(4);
  player_.At(3).Key(KN_RETURN);

  EXPECT_EQ(TurnsUntilPressed(KN_RETURN), 3);
  EXPECT_EQ(player_.frame(), 3);
}

TEST_F(ScriptedPlayerTest, GoingBackAFrameIsAProgrammingError) {
  player_.At(5);

  // The switch is inside GoogleTest's macro.
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(player_.At(4), "frame order");
}

TEST(ScriptedPlayerLifetimeTest, DetachesFromTheDisplayWhenDestroyed) {
  HeadlessDisplay display;
  display.SetVideoMode(8, 4);
  KeyBuffer keyboard;
  HardwareCursor mouse(48, 48);
  engine::platform::ManualClock clock;
  {
    const ScriptedPlayer player(display, keyboard, mouse, clock);
  }

  display.EndFrame();

  EXPECT_EQ(clock.Ticks(), 0);
}

}  // namespace
}  // namespace engine::window
