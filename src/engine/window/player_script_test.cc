#include "engine/window/player_script.h"

#include <string>
#include <string_view>
#include <vector>

#include "base/installed.h"
#include "engine/platform/manual_clock.h"
#include "engine/window/display.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/headless_display.h"
#include "engine/window/keyboard.h"
#include "engine/window/scripted_player.h"
#include "gtest/gtest.h"

namespace engine::window {
namespace {

// A headless Display with a player to load scripts onto, and hooks that
// record what a script asks for.
class PlayerScriptTest : public ::testing::Test {
 protected:
  PlayerScriptTest() {
    display_.SetVideoMode(8, 4);
    hooks_.take_shot = [this](const std::string_view name) {
      shots_.emplace_back(name);
    };
    hooks_.quit = [this] { quit_ = true; };
  }

  std::string Load(const std::string_view text) {
    return LoadPlayerScript(text, player_, hooks_);
  }

  // Ends a frame and returns the event waiting in the queue, if any.
  InputEvent Turn() {
    display_.EndFrame();
    return keyboard_.Peek() ? keyboard_.Read() : InputEvent{};
  }

  HeadlessDisplay display_;
  base::Installed<Display>::Scope display_scope_{display_};
  KeyBuffer keyboard_;
  HardwareCursor mouse_{48, 48};
  engine::platform::ManualClock clock_;
  ScriptedPlayer player_{display_, keyboard_, mouse_, clock_};
  PlayerScriptHooks hooks_;
  std::vector<std::string> shots_;
  bool quit_ = false;
};

TEST_F(PlayerScriptTest, AKeyStepPressesTheNamedKeyOnItsFrame) {
  ASSERT_EQ(Load("at 2 key ESC\n"), "");

  EXPECT_FALSE(Turn());
  EXPECT_EQ(Turn().key, KN_ESC);
}

TEST_F(PlayerScriptTest, KeysAreNamedWithoutRegardToCase) {
  ASSERT_EQ(Load("at 1 key return\nat 3 key a\nat 5 key 7\nat 7 key F10\n"),
            "");

  EXPECT_EQ(Turn().key, KN_RETURN);
  Turn();
  EXPECT_EQ(Turn().key, KN_A);
  Turn();
  EXPECT_EQ(Turn().key, KN_7);
  Turn();
  EXPECT_EQ(Turn().key, KN_F10);
}

TEST_F(PlayerScriptTest, KeydownAndKeyupHoldAModifierAroundOtherSteps) {
  ASSERT_EQ(Load("at 1 keydown SHIFT\nat 2 click 5 6\nat 4 keyup shift\n"), "");

  Turn();
  EXPECT_TRUE(IsShiftDown());
  Turn();
  Turn();
  Turn();
  EXPECT_FALSE(IsShiftDown());
}

TEST_F(PlayerScriptTest, AClickStepClicksThere) {
  ASSERT_EQ(Load("at 1 click 320 200\n"), "");

  const InputEvent press = Turn();
  EXPECT_TRUE(press.IsPress(MouseButton::kLeft));
  EXPECT_EQ(press.x, 320);
  EXPECT_EQ(press.y, 200);
  EXPECT_TRUE(Turn().IsRelease(MouseButton::kLeft));
}

TEST_F(PlayerScriptTest, ARightClickUsesTheRightButton) {
  ASSERT_EQ(Load("at 1 rclick 5 6\n"), "");

  EXPECT_TRUE(Turn().IsPress(MouseButton::kRight));
}

TEST_F(PlayerScriptTest, PressMoveAndReleaseMakeADrag) {
  ASSERT_EQ(Load("at 1 press 10 10\nat 2 move 30 10\nat 3 release 50 10\n"),
            "");

  EXPECT_TRUE(Turn().IsPress(MouseButton::kLeft));
  EXPECT_FALSE(Turn());
  EXPECT_EQ(mouse_.x(), 30);
  EXPECT_TRUE(Turn().IsRelease(MouseButton::kLeft));
}

TEST_F(PlayerScriptTest, ATypeStepTypesItsWord) {
  ASSERT_EQ(Load("at 1 type Ab1\n"), "");

  EXPECT_EQ(Turn().character, 'A');
  Turn();
  EXPECT_EQ(Turn().character, 'b');
  Turn();
  EXPECT_EQ(Turn().character, '1');
}

TEST_F(PlayerScriptTest, ShotAndQuitCallTheHooks) {
  ASSERT_EQ(Load("at 1 shot main_menu\nat 2 shot options\nat 2 quit\n"), "");

  Turn();
  EXPECT_EQ(shots_, (std::vector<std::string>{"main_menu"}));
  EXPECT_FALSE(quit_);
  Turn();
  EXPECT_EQ(shots_, (std::vector<std::string>{"main_menu", "options"}));
  EXPECT_TRUE(quit_);
}

TEST_F(PlayerScriptTest, BlankLinesCommentsAndSpacingAreSkipped) {
  ASSERT_EQ(Load("# opens the menu\n\n   at 1   key\tESC  \r\n# done\n"), "");

  EXPECT_EQ(Turn().key, KN_ESC);
}

TEST_F(PlayerScriptTest, AnEmptyScriptIsFine) {
  EXPECT_EQ(Load(""), "");
  EXPECT_TRUE(player_.finished());
}

TEST_F(PlayerScriptTest, ABadLineIsReportedByNumber) {
  EXPECT_EQ(Load("at 1 key ESC\nat 2 dance\n"), "line 2: unknown step dance");
  EXPECT_EQ(Load("key ESC\n"),
            "line 1: a step is written: at <frame> <step> ...");
  EXPECT_EQ(Load("at 3 key NOPE\n"), "line 1: no key is called NOPE");
  EXPECT_EQ(Load("at 3 click 5\n"), "line 1: click takes x and y");
  EXPECT_EQ(Load("at 3 click five 6\n"), "line 1: click takes two numbers");
  EXPECT_EQ(Load("at 3 key\n"), "line 1: key takes one word");
  EXPECT_EQ(Load("at 3\n"), "line 1: a step is written: at <frame> <step> ...");
  EXPECT_EQ(Load("at 3 key A B\n"), "line 1: key takes one word");
  EXPECT_EQ(Load("at 3 quit now\n"), "line 1: quit takes nothing");
  EXPECT_EQ(Load("at 3 type a!\n"), "line 1: type takes letters and digits");
}

TEST_F(PlayerScriptTest, FramesMustNotGoBack) {
  EXPECT_EQ(Load("at 5 key ESC\nat 4 key ESC\n"),
            "line 2: the frame must be a number no earlier than the one "
            "before");
  EXPECT_EQ(Load("at soon key ESC\n"),
            "line 1: the frame must be a number no earlier than the one "
            "before");
}

// The steps before a bad line stay queued, so a caller must not run a script
// that did not load.
TEST_F(PlayerScriptTest, TheStepsBeforeABadLineAreQueued) {
  ASSERT_NE(Load("at 1 key ESC\nat 2 dance\n"), "");

  EXPECT_EQ(Turn().key, KN_ESC);
}

}  // namespace
}  // namespace engine::window
