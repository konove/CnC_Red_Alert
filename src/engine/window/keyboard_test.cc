// Tests for KeyBuffer: what Peek() and Read() report, and how mouse clicks
// and unknown keys are queued.

#include "engine/window/keyboard.h"

#include <SDL_events.h>
#include <SDL_mouse.h>
#include <SDL_scancode.h>
#include <SDL_stdinc.h>


#include "engine/window/ww_mouse.h"
#include "engine/window/ww_win.h"
#include "gtest/gtest.h"

// The hooks keyboard.cc calls out to. Stubbing them keeps the buffer
// semantics under test deterministic and free of a real event pump.
void SDL_Event_Loop() {}
void Update_Mouse_Pos(int /*x*/, int /*y*/) {}
bool IsLeftButtonDown() { return false; }
bool IsRightButtonDown() { return false; }

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
  ASSERT_TRUE(keys.Put({.key = KN_F10}));

  // The bug this guards: a bool return collapsed every key to 1.
  EXPECT_EQ(keys.Peek(), KN_F10);
}

TEST_F(KeyBufferTest, PeekKeepsTheModifierBitsOfThePendingKey) {
  const KeyNumber shifted = Shift(KN_A);
  ASSERT_TRUE(keys.Put({.key = shifted}));

  EXPECT_EQ(keys.Peek(), shifted);
}

TEST_F(KeyBufferTest, PeekDoesNotConsumeTheKey) {
  ASSERT_TRUE(keys.Put({.key = KN_ESC}));

  EXPECT_EQ(keys.Peek(), KN_ESC);
  EXPECT_EQ(keys.Peek(), KN_ESC);
  EXPECT_EQ(keys.Read(), KN_ESC);
  EXPECT_EQ(keys.Peek(), 0);
}

TEST_F(KeyBufferTest, PeekReportsKeysInTheOrderTheyWerePut) {
  ASSERT_TRUE(keys.Put({.key = KN_1}));
  ASSERT_TRUE(keys.Put({.key = KN_2}));

  EXPECT_EQ(keys.Peek(), KN_1);
  EXPECT_EQ(keys.Read(), KN_1);
  EXPECT_EQ(keys.Peek(), KN_2);
  EXPECT_EQ(keys.Read(), KN_2);
}

TEST_F(KeyBufferTest, ClearDiscardsThePendingKey) {
  ASSERT_TRUE(keys.Put({.key = KN_SPACE}));
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

// A click is one event, whatever position it carries, so a full buffer drops
// it whole and keeps the keys already queued.
TEST_F(KeyBufferTest, ClickIntoAFullBufferIsDropped) {
  // The buffer holds 255 events.
  for (int i = 0; i < 255; ++i) {
    ASSERT_TRUE(keys.Put({.key = KN_A}));
  }
  const SDL_Event click = MakeClick(SDL_BUTTON_LEFT, 10, 20);

  keys.HandleEvent(&click);

  for (int i = 0; i < 255; ++i) {
    ASSERT_EQ(keys.Read(), KN_A);
  }
  EXPECT_EQ(keys.Peek(), KN_NONE);
}

TEST_F(KeyBufferTest, ClickIsQueuedWithItsButtonAndPosition) {
  const SDL_Event click = MakeClick(SDL_BUTTON_RIGHT, 10, 20);

  EXPECT_TRUE(keys.HandleEvent(&click));

  const InputEvent event = keys.ReadEvent();
  EXPECT_TRUE(event.IsPress(MouseButton::kRight));
  EXPECT_EQ(event.x, 10);
  EXPECT_EQ(event.y, 20);
  EXPECT_EQ(keys.Peek(), KN_NONE);
}

// Until the games read clicks from the event, a click also carries its old
// key number and leaves its position in click_x() and click_y().
TEST_F(KeyBufferTest, ClickStillReadsAsItsKeyNumber) {
  ASSERT_TRUE(keys.PutClick(MouseButton::kLeft, /*release=*/true, 30, 40));

  EXPECT_EQ(keys.Read(), Released(KN_LMOUSE));
  EXPECT_EQ(keys.click_x(), 30);
  EXPECT_EQ(keys.click_y(), 40);
}

TEST_F(KeyBufferTest, ClicksAndKeysComeOutInOrder) {
  ASSERT_TRUE(keys.Put({.key = KN_Q}));
  ASSERT_TRUE(keys.PutClick(MouseButton::kMiddle, /*release=*/false, 0, 0));
  ASSERT_TRUE(keys.Put({.key = KN_Y}));

  EXPECT_EQ(keys.ReadEvent().key, KN_Q);
  EXPECT_TRUE(keys.ReadEvent().IsPress(MouseButton::kMiddle));
  EXPECT_EQ(keys.ReadEvent().key, KN_Y);
}

TEST_F(KeyBufferTest, KeyEventIsNoClick) {
  ASSERT_TRUE(keys.Put({.key = KN_ESC}));

  const InputEvent event = keys.ReadEvent();
  EXPECT_FALSE(event.IsClick());
  EXPECT_FALSE(event.IsPress(MouseButton::kLeft));
}

// The hotkeys a Steam install's REDALERT.INI stores, as Windows virtual-key
// codes, and the keys they name.
TEST(WindowsKeyTest, TranslatesTheShippedHotkeys) {
  EXPECT_EQ(KeyFromWindowsKey(18), KN_LALT);     // VK_MENU
  EXPECT_EQ(KeyFromWindowsKey(17), KN_LCTRL);    // VK_CONTROL
  EXPECT_EQ(KeyFromWindowsKey(16), KN_LSHIFT);   // VK_SHIFT
  EXPECT_EQ(KeyFromWindowsKey(83), KN_S);        // 'S'
  EXPECT_EQ(KeyFromWindowsKey(49), KN_1);        // '1'
  EXPECT_EQ(KeyFromWindowsKey(48), KN_0);        // '0'
  EXPECT_EQ(KeyFromWindowsKey(36), KN_HOME);     // VK_HOME
  EXPECT_EQ(KeyFromWindowsKey(103), KN_E_HOME);  // VK_NUMPAD7
  EXPECT_EQ(KeyFromWindowsKey(120), KN_F9);      // VK_F9
  EXPECT_EQ(KeyFromWindowsKey(38), KN_UP);       // VK_UP
  EXPECT_EQ(KeyFromWindowsKey(27), KN_ESC);      // VK_ESCAPE
  EXPECT_EQ(KeyFromWindowsKey(32), KN_SPACE);    // VK_SPACE
}

TEST(WindowsKeyTest, NoKeyStaysNoKey) {
  EXPECT_EQ(KeyFromWindowsKey(0), KN_NONE);
  EXPECT_EQ(WindowsKeyFromKey(KN_NONE), 0);
}

TEST(WindowsKeyTest, UnknownCodesNameNoKey) {
  EXPECT_EQ(KeyFromWindowsKey(0xFF), KN_NONE);
  EXPECT_EQ(KeyFromWindowsKey(0x0E), KN_NONE);
}

TEST(WindowsKeyTest, CarriesTheModifierBitsAndDropsTheVirtualKeyBit) {
  // 0x41 is 'A'; 0x1000 marked a virtual key in the Windows game.
  EXPECT_EQ(KeyFromWindowsKey(static_cast<int>(0x41U | 0x1000U | kKeyCtrlBit)),
            KN_A | KN_CTRL_BIT);
  EXPECT_EQ(WindowsKeyFromKey(KN_A | KN_SHIFT_BIT | KN_RLSE_BIT),
            static_cast<int>(0x41U | kKeyShiftBit | kKeyReleaseBit));
}

TEST(WindowsKeyTest, MapsTheMouseButtons) {
  EXPECT_EQ(KeyFromWindowsKey(1), KN_LMOUSE);
  EXPECT_EQ(KeyFromWindowsKey(2), KN_RMOUSE);
  EXPECT_EQ(KeyFromWindowsKey(4), KN_MMOUSE);  // VK_MBUTTON is 4, not 3
}

// Saving a hotkey and loading it back gives the same key, for every key a
// default binding uses.
TEST(WindowsKeyTest, RoundTripsTheDefaultBindings) {
  for (const KeyNumber key :
       {KN_LALT, KN_LCTRL, KN_LSHIFT, KN_X, KN_S, KN_G, KN_N,  KN_B,
        KN_F,    KN_HOME,  KN_E_HOME, KN_H, KN_R, KN_A, KN_F9, KN_F10,
        KN_F11,  KN_F12,   KN_E,      KN_T, KN_Y, KN_U, KN_UP, KN_DOWN,
        KN_ESC,  KN_SPACE, KN_Q,      KN_1, KN_2, KN_3, KN_4,  KN_5,
        KN_6,    KN_7,     KN_8,      KN_9, KN_0}) {
    EXPECT_EQ(KeyFromWindowsKey(WindowsKeyFromKey(key)), key) << key;
  }
}

// The left and right modifier keys share a key number, which is written back
// as the generic code the Windows game used.
TEST(WindowsKeyTest, WritesTheGenericModifierCode) {
  EXPECT_EQ(KeyFromWindowsKey(0xA5), KN_RALT);  // VK_RMENU
  EXPECT_EQ(WindowsKeyFromKey(KN_RALT), 0x12);  // VK_MENU
}

// The helpers are constexpr so that dialogs can use them as case labels.
static_assert(Alt(KN_X) == (KN_X | KN_ALT_BIT));
static_assert(KeyCode(Alt(Shift(KN_UP))) == KN_UP);
static_assert(ButtonId(ButtonKey(42)) == 42);

TEST(KeyModifierTest, AddsAndReportsEachModifier) {
  EXPECT_TRUE(HasShift(Shift(KN_A)));
  EXPECT_TRUE(HasCtrl(Ctrl(KN_A)));
  EXPECT_TRUE(HasAlt(Alt(KN_A)));
  EXPECT_TRUE(IsRelease(Released(KN_A)));

  EXPECT_FALSE(HasShift(KN_A));
  EXPECT_FALSE(HasCtrl(Alt(KN_A)));
  EXPECT_FALSE(HasAlt(Ctrl(KN_A)));
  EXPECT_FALSE(IsRelease(Shift(KN_A)));
}

TEST(KeyModifierTest, KeyCodeDropsEveryFlag) {
  EXPECT_EQ(KeyCode(Released(Ctrl(Alt(Shift(KN_Q))))), KN_Q);
  EXPECT_EQ(KeyCode(Released(KN_LMOUSE)), KN_LMOUSE);
  EXPECT_EQ(KeyCode(KN_NONE), KN_NONE);
}

TEST(KeyModifierTest, WithoutModifiersKeepsTheRelease) {
  EXPECT_EQ(WithoutModifiers(Ctrl(Alt(Shift(KN_Q)))), KN_Q);
  EXPECT_EQ(WithoutModifiers(Released(Alt(KN_Q))), Released(KN_Q));
}

TEST(KeyModifierTest, ButtonIdUndoesButtonKey) {
  EXPECT_EQ(ButtonId(ButtonKey(0)), 0);
  EXPECT_EQ(ButtonId(ButtonKey(100)), 100);
}

}  // namespace
}  // namespace engine::window
