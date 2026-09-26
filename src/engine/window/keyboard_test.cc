// Tests for KeyBuffer: what Peek() and Read() report, and how mouse clicks
// and unknown keys are queued.

#include "engine/window/keyboard.h"

#include <SDL_events.h>
#include <SDL_mouse.h>
#include <SDL_scancode.h>
#include <SDL_stdinc.h>

#include <cstdint>

#include "engine/window/ww_mouse.h"
#include "engine/window/ww_win.h"
#include "gtest/gtest.h"

// The two hooks keyboard.cc calls out to. Stubbing them keeps the buffer
// semantics under test deterministic and free of a real event pump.
void SDL_Event_Loop() {}
void Update_Mouse_Pos(int /*x*/, int /*y*/) {}

namespace engine::window {
namespace {

// Returns a press of the SDL mouse `button` at `x`, `y`.
SDL_Event MakeClick(const Uint8 button, const Sint32 x, const Sint32 y) {
  SDL_Event click{};
  click.button.type = SDL_MOUSEBUTTONDOWN;
  click.button.button = button;
  click.button.state = SDL_PRESSED;
  click.button.x = x;
  click.button.y = y;
  return click;
}

class KeyBufferTest : public ::testing::Test {
 protected:
  KeyBuffer keys;
};

TEST_F(KeyBufferTest, PeekReportsZeroWhenNoKeyIsPending) {
  EXPECT_EQ(keys.Peek(), 0);
}

TEST_F(KeyBufferTest, PeekReportsThePendingKeyNumber) {
  ASSERT_TRUE(keys.Put(KN_F10));

  // The bug this guards: a bool return collapsed every key to 1.
  EXPECT_EQ(keys.Peek(), KN_F10);
}

TEST_F(KeyBufferTest, PeekKeepsTheModifierBitsOfThePendingKey) {
  const int shifted =
      static_cast<int>(static_cast<uint32_t>(KN_A) | kKeyShiftBit);
  ASSERT_TRUE(keys.Put(shifted));

  EXPECT_EQ(keys.Peek(), shifted);
}

TEST_F(KeyBufferTest, PeekDoesNotConsumeTheKey) {
  ASSERT_TRUE(keys.Put(KN_ESC));

  EXPECT_EQ(keys.Peek(), KN_ESC);
  EXPECT_EQ(keys.Peek(), KN_ESC);
  EXPECT_EQ(keys.Read(), KN_ESC);
  EXPECT_EQ(keys.Peek(), 0);
}

TEST_F(KeyBufferTest, PeekReportsKeysInTheOrderTheyWerePut) {
  ASSERT_TRUE(keys.Put(KN_1));
  ASSERT_TRUE(keys.Put(KN_2));

  EXPECT_EQ(keys.Peek(), KN_1);
  EXPECT_EQ(keys.Read(), KN_1);
  EXPECT_EQ(keys.Peek(), KN_2);
  EXPECT_EQ(keys.Read(), KN_2);
}

TEST_F(KeyBufferTest, ClearDiscardsThePendingKey) {
  ASSERT_TRUE(keys.Put(KN_SPACE));
  ASSERT_EQ(keys.Peek(), KN_SPACE);

  keys.Clear();

  EXPECT_EQ(keys.Peek(), 0);
}

// A zero key would be indistinguishable from Peek's empty result, and Read
// would spin on it forever, so the unknown scancode must not reach the buffer.
TEST_F(KeyBufferTest, UnknownScancodeIsNotBuffered) {
  EXPECT_FALSE(keys.PutKey(0));

  EXPECT_EQ(keys.Peek(), 0);
}

// The release of an unknown key gains kKeyReleaseBit, which must not smuggle
// the zero scancode past the check.
TEST_F(KeyBufferTest, UnknownScancodeReleaseIsNotBuffered) {
  EXPECT_FALSE(keys.PutKey(0, /*release=*/true));

  EXPECT_EQ(keys.Peek(), 0);
}

// SDL's media keys have scancodes above 0xFF, which would spill into the
// modifier bits: "next track" (258) would read as a shifted right click.
TEST_F(KeyBufferTest, ScancodeAboveTheKeyCodeByteIsNotBuffered) {
  EXPECT_FALSE(keys.PutKey(SDL_SCANCODE_AUDIONEXT));

  EXPECT_EQ(keys.Peek(), 0);
}

// A click is three entries. When fewer than three slots are free, queuing the
// button without its position would make Read() read the position from past
// the end of the queue.
TEST_F(KeyBufferTest, ClickThatDoesNotFitIsDroppedWhole) {
  // The buffer holds 255 entries; leave two free.
  for (int i = 0; i < 253; ++i) {
    ASSERT_TRUE(keys.Put(KN_A));
  }
  const SDL_Event click = MakeClick(SDL_BUTTON_LEFT, 10, 20);

  keys.HandleEvent(&click);

  for (int i = 0; i < 253; ++i) {
    ASSERT_EQ(keys.Read(), KN_A);
  }
  EXPECT_EQ(keys.Peek(), 0);
}

TEST_F(KeyBufferTest, ClickIsQueuedWithItsPosition) {
  const SDL_Event click = MakeClick(SDL_BUTTON_RIGHT, 10, 20);

  EXPECT_TRUE(keys.HandleEvent(&click));

  EXPECT_EQ(keys.Read(), KN_RMOUSE);
  EXPECT_EQ(keys.click_x(), 10);
  EXPECT_EQ(keys.click_y(), 20);
  EXPECT_EQ(keys.Peek(), 0);
}

TEST_F(KeyBufferTest, MouseClickCoordinatesAreNotReportedAsKeys) {
  // A click queues the button followed by its x and y position; one at the
  // origin puts two zero entries in the buffer behind the button.
  ASSERT_TRUE(keys.PutClick(KN_LMOUSE, /*release=*/false, 0, 0));
  ASSERT_TRUE(keys.Put(KN_Y));

  EXPECT_EQ(keys.Peek(), KN_LMOUSE);
  EXPECT_EQ(keys.Read(), KN_LMOUSE);
  EXPECT_EQ(keys.click_x(), 0);
  EXPECT_EQ(keys.click_y(), 0);

  // The coordinates were stepped over rather than reported as a missing key.
  EXPECT_EQ(keys.Peek(), KN_Y);
}

}  // namespace
}  // namespace engine::window
