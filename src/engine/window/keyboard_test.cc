// Tests for KeyBuffer: what Check() and Get() report, and how mouse clicks
// and unknown keys are queued.

#include "engine/window/keyboard.h"

#include <SDL_events.h>
#include <SDL_mouse.h>
#include <SDL_scancode.h>

#include <cstdint>

#include "engine/window/ww_mouse.h"
#include "engine/window/ww_win.h"
#include "gtest/gtest.h"

// The two hooks keyboard.cc calls out to. Stubbing them keeps the buffer
// semantics under test deterministic and free of a real event pump.
void SDL_Event_Loop() {}
void Update_Mouse_Pos(int /*x*/, int /*y*/) {}

namespace {

class KeyboardTest : public ::testing::Test {
 protected:
  KeyBuffer keyboard;
};

TEST_F(KeyboardTest, CheckReportsZeroWhenNoKeyIsPending) {
  EXPECT_EQ(keyboard.Check(), 0);
}

TEST_F(KeyboardTest, CheckReportsThePendingKeyNumber) {
  ASSERT_TRUE(keyboard.Put(KN_F10));

  // The bug this guards: a bool return collapsed every key to 1.
  EXPECT_EQ(keyboard.Check(), KN_F10);
}

TEST_F(KeyboardTest, CheckKeepsTheModifierBitsOfThePendingKey) {
  const int shifted =
      static_cast<int>(static_cast<uint32_t>(KN_A) | WWKEY_SHIFT_BIT);
  ASSERT_TRUE(keyboard.Put(shifted));

  EXPECT_EQ(keyboard.Check(), shifted);
}

TEST_F(KeyboardTest, CheckDoesNotConsumeTheKey) {
  ASSERT_TRUE(keyboard.Put(KN_ESC));

  EXPECT_EQ(keyboard.Check(), KN_ESC);
  EXPECT_EQ(keyboard.Check(), KN_ESC);
  EXPECT_EQ(keyboard.Get(), KN_ESC);
  EXPECT_EQ(keyboard.Check(), 0);
}

TEST_F(KeyboardTest, CheckReportsKeysInTheOrderTheyWerePut) {
  ASSERT_TRUE(keyboard.Put(KN_1));
  ASSERT_TRUE(keyboard.Put(KN_2));

  EXPECT_EQ(keyboard.Check(), KN_1);
  EXPECT_EQ(keyboard.Get(), KN_1);
  EXPECT_EQ(keyboard.Check(), KN_2);
  EXPECT_EQ(keyboard.Get(), KN_2);
}

TEST_F(KeyboardTest, ClearDiscardsThePendingKey) {
  ASSERT_TRUE(keyboard.Put(KN_SPACE));
  ASSERT_EQ(keyboard.Check(), KN_SPACE);

  keyboard.Clear();

  EXPECT_EQ(keyboard.Check(), 0);
}

// A zero key would be indistinguishable from Check's empty result, and Get
// would spin on it forever, so the unknown scancode must not reach the buffer.
TEST_F(KeyboardTest, UnknownScancodeIsNotBuffered) {
  EXPECT_FALSE(keyboard.Put_Key_Message(0));

  EXPECT_EQ(keyboard.Check(), 0);
}

// The release of an unknown key gains WWKEY_RLS_BIT, which must not smuggle
// the zero scancode past the check.
TEST_F(KeyboardTest, UnknownScancodeReleaseIsNotBuffered) {
  EXPECT_FALSE(keyboard.Put_Key_Message(0, /*release=*/true));

  EXPECT_EQ(keyboard.Check(), 0);
}

// SDL's media keys have scancodes above 0xFF, which would spill into the
// modifier bits: "next track" (258) would read as a shifted right click.
TEST_F(KeyboardTest, ScancodeAboveTheKeyCodeByteIsNotBuffered) {
  EXPECT_FALSE(keyboard.Put_Key_Message(SDL_SCANCODE_AUDIONEXT));

  EXPECT_EQ(keyboard.Check(), 0);
}

// A click is three entries. When fewer than three slots are free, queuing the
// button without its position would make Get() read the position from past
// the end of the queue.
TEST_F(KeyboardTest, ClickThatDoesNotFitIsDroppedWhole) {
  // The buffer holds 255 entries; leave two free.
  for (int i = 0; i < 253; ++i) {
    ASSERT_TRUE(keyboard.Put(KN_A));
  }
  SDL_Event click{};
  click.button.type = SDL_MOUSEBUTTONDOWN;
  click.button.button = SDL_BUTTON_LEFT;
  click.button.state = SDL_PRESSED;
  click.button.x = 10;
  click.button.y = 20;

  keyboard.Event_Handler(&click);

  for (int i = 0; i < 253; ++i) {
    ASSERT_EQ(keyboard.Get(), KN_A);
  }
  EXPECT_EQ(keyboard.Check(), 0);
}

TEST_F(KeyboardTest, ClickIsQueuedWithItsPosition) {
  SDL_Event click{};
  click.button.type = SDL_MOUSEBUTTONDOWN;
  click.button.button = SDL_BUTTON_RIGHT;
  click.button.state = SDL_PRESSED;
  click.button.x = 10;
  click.button.y = 20;

  EXPECT_TRUE(keyboard.Event_Handler(&click));

  EXPECT_EQ(keyboard.Get(), KN_RMOUSE);
  EXPECT_EQ(keyboard.MouseQX, 10);
  EXPECT_EQ(keyboard.MouseQY, 20);
  EXPECT_EQ(keyboard.Check(), 0);
}

TEST_F(KeyboardTest, MouseClickCoordinatesAreNotReportedAsKeys) {
  // Event_Handler queues a mouse key followed by its x and y position; a click
  // at the origin puts two zero entries in the buffer behind the key.
  ASSERT_TRUE(keyboard.Put_Key_Message(VK_LBUTTON));
  ASSERT_TRUE(keyboard.Put(0));
  ASSERT_TRUE(keyboard.Put(0));
  ASSERT_TRUE(keyboard.Put(KN_Y));

  EXPECT_EQ(keyboard.Check(), KN_LMOUSE);
  EXPECT_EQ(keyboard.Get(), KN_LMOUSE);
  EXPECT_EQ(keyboard.MouseQX, 0);
  EXPECT_EQ(keyboard.MouseQY, 0);

  // The coordinates were stepped over rather than reported as a missing key.
  EXPECT_EQ(keyboard.Check(), KN_Y);
}

}  // namespace
