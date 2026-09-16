#include "sdllib/keyboard.h"

#include <cstdint>

#include "gtest/gtest.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"

// The two hooks keyboard.cc calls out to. Stubbing them keeps the buffer
// semantics under test deterministic and free of a real event pump.
void SDL_Event_Loop() {}
void Update_Mouse_Pos(int /*x*/, int /*y*/) {}

namespace {

class KeyboardTest : public ::testing::Test {
 protected:
  WWKeyboardClass keyboard;
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
