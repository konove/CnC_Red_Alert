// File: UiFixture, a GameFixture with a screen to draw on and a scripted
// player at the keyboard.

#ifndef CNC_RED_ALERT_RA_STARTUP_UI_TEST_UTIL_H_
#define CNC_RED_ALERT_RA_STARTUP_UI_TEST_UTIL_H_

#include <cstddef>
#include <vector>

#include "absl/base/attributes.h"
#include "engine/platform/manual_clock.h"
#include "engine/window/scripted_player.h"
#include "ra/startup/game_test_util.h"

namespace ra {

// For tests of the code that blocks in its own loop until the player answers:
// dialogs, menus, timed screens. On top of GameFixture's Game it gives the
// game a headless 640x400 screen, a clock that moves one tick a frame, and a
// ScriptedPlayer whose keys and clicks the loop reads. No window opens and no
// real time passes.
//
// Every font is the same 6x10 block font (MakeBlockFont()), so text and the
// buttons sized to it have a known size: a button is 12 pixels tall and 8
// wider than its text. Every string of the text table is empty until the
// test sets it with SetText(), so a button labelled from the table is 8 wide.
//
// Example:
//   TEST_F(UiFixture, EscClosesTheDialog) {
//     player().At(2).Key(engine::window::KN_ESC);
//     RunSpecialOptionsDialog();
//     EXPECT_TRUE(player().finished());
//   }
class UiFixture : public GameFixture {
 public:
  ~UiFixture() override;

  UiFixture(const UiFixture&) = delete;
  UiFixture& operator=(const UiFixture&) = delete;
  UiFixture(UiFixture&&) = delete;
  UiFixture& operator=(UiFixture&&) = delete;

 protected:
  UiFixture();

  // The player at the keyboard. Queue its whole script before calling the
  // code under test.
  engine::window::ScriptedPlayer& player() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return player_;
  }
  // The clock the game reads instead of real time; the player moves it.
  engine::platform::ManualClock& clock() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return clock_;
  }

 private:
  // The font file behind every FontType; Assets only borrows it.
  std::vector<std::byte> font_;
  engine::platform::ManualClock clock_;
  engine::window::ScriptedPlayer player_;
};

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_STARTUP_UI_TEST_UTIL_H_
