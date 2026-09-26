/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// File: The keyboard buffer the games read keys and mouse clicks from, the
// key numbers (KN_*) they are compared against, and the modifier bits combined
// with them. It began as Philip W. Gorrow's Westwood
// Keyboard Library (October 1995); the SDL port fills the buffer from SDL
// events instead of Windows messages.

#ifndef CNC_RED_ALERT_ENGINE_WINDOW_KEYBOARD_H_
#define CNC_RED_ALERT_ENGINE_WINDOW_KEYBOARD_H_

#include <SDL_scancode.h>

#include <cstdint>

union SDL_Event;

namespace engine::window {

// Modifier and state bits combined with a key number. They are flags, not an
// enumeration, so they are unsigned bit masks that mix with KeyNumber freely.
// The key code itself fills the low byte, which is why a scancode above 0xFF
// cannot be represented.
inline constexpr uint32_t kKeyShiftBit = 0x100;
inline constexpr uint32_t kKeyCtrlBit = 0x200;
inline constexpr uint32_t kKeyAltBit = 0x400;
inline constexpr uint32_t kKeyReleaseBit = 0x800;
inline constexpr uint32_t kKeyButtonBit = 0x8000;

// The low byte of a key number, which holds the key's scancode.
inline constexpr uint32_t kScancodeMask = 0xFFU;

// Key numbers: which key, as its SDL scancode, so a key event needs no
// translation. The mouse buttons take 1-3, which SDL leaves unused. The values
// combine with the KN_*_BIT modifier bits and with KN_BUTTON (see ButtonKey()),
// so the enum stays unscoped. Keys the port cannot tell apart share a value:
// each right-hand modifier key is its left-hand twin, and each diagonal is its
// navigation key. Only the keys the games name are listed; any other scancode
// still arrives as its number.
// NOLINTNEXTLINE(cppcoreguidelines-use-enum-class)
enum KeyNumber {
  KN_NONE = SDL_SCANCODE_UNKNOWN,

  KN_0 = SDL_SCANCODE_0,
  KN_1 = SDL_SCANCODE_1,
  KN_2 = SDL_SCANCODE_2,
  KN_3 = SDL_SCANCODE_3,
  KN_4 = SDL_SCANCODE_4,
  KN_5 = SDL_SCANCODE_5,
  KN_6 = SDL_SCANCODE_6,
  KN_7 = SDL_SCANCODE_7,
  KN_8 = SDL_SCANCODE_8,
  KN_9 = SDL_SCANCODE_9,
  KN_A = SDL_SCANCODE_A,
  KN_B = SDL_SCANCODE_B,
  KN_BACKSPACE = SDL_SCANCODE_BACKSPACE,
  KN_C = SDL_SCANCODE_C,
  KN_CENTER = SDL_SCANCODE_KP_5,
  KN_COMMA = SDL_SCANCODE_COMMA,
  KN_D = SDL_SCANCODE_D,
  KN_DELETE = SDL_SCANCODE_DELETE,
  KN_DOWN = SDL_SCANCODE_DOWN,
  KN_DOWNLEFT = SDL_SCANCODE_END,
  KN_DOWNRIGHT = SDL_SCANCODE_PAGEDOWN,
  KN_E = SDL_SCANCODE_E,
  KN_END = SDL_SCANCODE_END,
  KN_ESC = SDL_SCANCODE_ESCAPE,
  KN_E_HOME = SDL_SCANCODE_KP_7,
  KN_F = SDL_SCANCODE_F,
  KN_F1 = SDL_SCANCODE_F1,
  KN_F10 = SDL_SCANCODE_F10,
  KN_F11 = SDL_SCANCODE_F11,
  KN_F12 = SDL_SCANCODE_F12,
  KN_F2 = SDL_SCANCODE_F2,
  KN_F3 = SDL_SCANCODE_F3,
  KN_F4 = SDL_SCANCODE_F4,
  KN_F5 = SDL_SCANCODE_F5,
  KN_F6 = SDL_SCANCODE_F6,
  KN_F7 = SDL_SCANCODE_F7,
  KN_F8 = SDL_SCANCODE_F8,
  KN_F9 = SDL_SCANCODE_F9,
  KN_G = SDL_SCANCODE_G,
  KN_GRAVE = SDL_SCANCODE_GRAVE,
  KN_H = SDL_SCANCODE_H,
  KN_HOME = SDL_SCANCODE_HOME,
  // NOLINTNEXTLINE(misc-confusable-identifiers)
  KN_I = SDL_SCANCODE_I,
  KN_INSERT = SDL_SCANCODE_INSERT,
  KN_J = SDL_SCANCODE_J,
  KN_K = SDL_SCANCODE_K,
  KN_KEYPAD_RETURN = SDL_SCANCODE_RETURN,
  KN_L = SDL_SCANCODE_L,
  KN_LALT = SDL_SCANCODE_LALT,
  KN_LCTRL = SDL_SCANCODE_LCTRL,
  KN_LEFT = SDL_SCANCODE_LEFT,
  KN_LMOUSE = 1,
  KN_MMOUSE = 3,
  KN_LSHIFT = SDL_SCANCODE_LSHIFT,
  KN_M = SDL_SCANCODE_M,
  KN_N = SDL_SCANCODE_N,
  // NOLINTNEXTLINE(misc-confusable-identifiers)
  KN_O = SDL_SCANCODE_O,
  KN_P = SDL_SCANCODE_P,
  KN_PERIOD = SDL_SCANCODE_PERIOD,
  KN_PGDN = SDL_SCANCODE_PAGEDOWN,
  KN_PGUP = SDL_SCANCODE_PAGEUP,
  KN_Q = SDL_SCANCODE_Q,
  KN_R = SDL_SCANCODE_R,
  KN_RALT = SDL_SCANCODE_LALT,
  KN_RCTRL = SDL_SCANCODE_LCTRL,
  KN_RETURN = SDL_SCANCODE_RETURN,
  KN_RIGHT = SDL_SCANCODE_RIGHT,
  KN_RMOUSE = 2,
  KN_RSHIFT = SDL_SCANCODE_LSHIFT,
  KN_S = SDL_SCANCODE_S,
  KN_SLASH = SDL_SCANCODE_SLASH,
  KN_SPACE = SDL_SCANCODE_SPACE,
  KN_T = SDL_SCANCODE_T,
  KN_TAB = SDL_SCANCODE_TAB,
  KN_U = SDL_SCANCODE_U,
  KN_UP = SDL_SCANCODE_UP,
  KN_UPLEFT = SDL_SCANCODE_HOME,
  KN_UPRIGHT = SDL_SCANCODE_PAGEUP,
  KN_V = SDL_SCANCODE_V,
  KN_W = SDL_SCANCODE_W,
  KN_X = SDL_SCANCODE_X,
  KN_Y = SDL_SCANCODE_Y,
  KN_Z = SDL_SCANCODE_Z,

  KN_SHIFT_BIT = kKeyShiftBit,
  KN_CTRL_BIT = kKeyCtrlBit,
  KN_ALT_BIT = kKeyAltBit,
  KN_RLSE_BIT = kKeyReleaseBit,
  KN_BUTTON = kKeyButtonBit,
};

// Returns the KeyNumber that GadgetClass::Input() reports when the gadget with
// the given ID is triggered. The KN_BUTTON bit distinguishes a gadget event
// from a real keypress, letting both share one switch on the input value.
//
// Gadget IDs are dialog-local — the same numeric ID means a different button in
// each dialog — so they are passed in rather than enumerated here.
//
// Example:
//   switch (input) {
//     case engine::window::KN_ESC:
//     case engine::window::ButtonKey(BUTTON_CANCEL):
//       ...
//   }
constexpr KeyNumber ButtonKey(const int id) {
  // A gadget ID is not a key code, so the value never names an enumerator.
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  return static_cast<KeyNumber>(static_cast<uint32_t>(id) | KN_BUTTON);
}

// A key number is a key code in the low bits with the KN_*_BIT modifier and
// release bits above it, so a combined or masked value rarely names an
// enumerator. These operators define that representation for KeyNumber in
// place of the games' generic enum operators, and are the only place the
// analyzer's named-enumerator model of the type is set aside.
constexpr KeyNumber operator|(const KeyNumber a, const KeyNumber b) noexcept {
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  return static_cast<KeyNumber>(static_cast<uint32_t>(a) |
                                static_cast<uint32_t>(b));
}
constexpr KeyNumber operator&(const KeyNumber a, const KeyNumber b) noexcept {
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  return static_cast<KeyNumber>(static_cast<uint32_t>(a) &
                                static_cast<uint32_t>(b));
}
constexpr KeyNumber operator~(const KeyNumber a) noexcept {
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  return static_cast<KeyNumber>(~static_cast<uint32_t>(a));
}

// Returns `key` with Shift, Ctrl or Alt held, or released. They are constexpr,
// so a dialog can switch on them:
//   case engine::window::Alt(engine::window::KN_X):
constexpr KeyNumber Shift(const KeyNumber key) { return key | KN_SHIFT_BIT; }
constexpr KeyNumber Ctrl(const KeyNumber key) { return key | KN_CTRL_BIT; }
constexpr KeyNumber Alt(const KeyNumber key) { return key | KN_ALT_BIT; }
constexpr KeyNumber Released(const KeyNumber key) { return key | KN_RLSE_BIT; }

// Returns which key `key` is, without the modifier, release and button bits
// that say how it was pressed. A release matches too; test IsRelease() as well
// to tell a press from a release.
constexpr KeyNumber KeyCode(const KeyNumber key) {
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  return static_cast<KeyNumber>(static_cast<uint32_t>(key) & kScancodeMask);
}

// Returns whether `key` was pressed with Shift, Ctrl or Alt held, or is a
// release.
constexpr bool HasShift(const KeyNumber key) {
  return (static_cast<uint32_t>(key) & kKeyShiftBit) != 0;
}
constexpr bool HasCtrl(const KeyNumber key) {
  return (static_cast<uint32_t>(key) & kKeyCtrlBit) != 0;
}
constexpr bool HasAlt(const KeyNumber key) {
  return (static_cast<uint32_t>(key) & kKeyAltBit) != 0;
}
constexpr bool IsRelease(const KeyNumber key) {
  return (static_cast<uint32_t>(key) & kKeyReleaseBit) != 0;
}

// Returns the gadget ID that ButtonKey() packed into `key`.
constexpr int ButtonId(const KeyNumber key) {
  return static_cast<int>(static_cast<uint32_t>(key) & ~kKeyButtonBit);
}

// Returns whether Shift, Ctrl or Alt is held down right now, on either side
// of the keyboard.
bool IsShiftDown();
bool IsCtrlDown();
bool IsAltDown();

// Returns the key that the Windows virtual-key code in the low byte of
// `windows_key` names, with the Shift, Ctrl, Alt and release bits above it
// carried over. This is the form the original Windows game stored hotkeys in
// (REDALERT.INI's [WinHotkeys]). Returns KN_NONE for 0 and for a code that
// names no key here.
KeyNumber KeyFromWindowsKey(int windows_key);

// Returns the Windows virtual-key code for `key`, with its Shift, Ctrl, Alt and
// release bits carried over: the inverse of KeyFromWindowsKey(). Returns 0 for
// KN_NONE and for a key that has no Windows code.
int WindowsKeyFromKey(KeyNumber key);

// The queue of key presses, key releases and mouse clicks the game reads its
// input from. HandleEvent() fills it from SDL events; the game drains it with
// Peek() and Read(). Each entry is a key number: a scancode in the low byte
// with the kKey*Bit flags above it. A mouse click takes three entries - the
// button, then the x and y position - and Read() returns the button and leaves
// the position in click_x() and click_y().
//
// Example:
//   if (keys.Peek() != KN_NONE) {
//     const KeyNumber key = keys.Read();
//     if (engine::window::KeyBuffer::IsMouseKey(key)) {
//       Click_At(keys.click_x(), keys.click_y());
//     }
//   }
class KeyBuffer {
 public:
  // Returns the key number at the head of the buffer without removing it, or
  // KN_NONE when no key is pending. Also pumps the SDL event loop, so callers
  // that only need that side effect may discard the result.
  KeyNumber Peek();

  // Removes and returns the key number at the head of the buffer, pumping SDL
  // events until one arrives. For a mouse key, also stores the click position
  // for click_x() and click_y().
  KeyNumber Read();

  // Appends one raw entry to the buffer. Returns false, dropping the entry, if
  // the buffer is full.
  bool Put(int entry);

  // Queues the key `key_code` (a bare key number), adding the Shift, Ctrl and
  // Alt bits for the modifier keys held right now and kKeyReleaseBit for a
  // `release`. Returns false if the key was dropped: the buffer is full, or
  // the scancode is 0 (a key SDL does not know), negative, or above 0xFF (a
  // media key, which has no key code). Mouse buttons go through PutClick().
  bool PutKey(int key_code, bool release = false);

  // Queues a click of the mouse `button` (KN_LMOUSE, KN_MMOUSE or
  // KN_RMOUSE) at `x`, `y`: the button, with kKeyReleaseBit for a `release`
  // but no modifier bits, as in the DOS version, followed by the position.
  // Returns false, queuing nothing, if the three entries do not all fit.
  bool PutClick(int button, bool release, int x, int y);

  // Returns the character `key` types on the current keyboard layout, with
  // Shift ignored, so letters come back lower case. Returns '\0' for a
  // release and for a key that types no character up to 'z' (arrows, function
  // keys, Delete, the mouse buttons).
  static char ToAscii(int key);

  // Discards every pending entry.
  void Clear();

  // Returns whether `key` is held down right now, read from SDL's live state
  // rather than from the buffer. Covers the left and right mouse buttons, and
  // either side of the keyboard for Shift, Ctrl and Alt. `key` is a bare key
  // code: with modifier bits set it names a different scancode.
  static bool IsDown(int key);

  // Returns whether `key` is a mouse button, pressed or released, whatever
  // modifier bits it carries. In the buffer such an entry is followed by the
  // click's x and y position.
  static bool IsMouseKey(int key);

  // Queues the key or click an SDL event carries; mouse motion moves the
  // cursor instead. Returns true only for a click, which it consumes, so the
  // game's own handler can skip it; everything else goes on to that handler.
  bool HandleEvent(const SDL_Event* event);

  // The position of the last mouse click Read() returned, in game pixels.
  [[nodiscard]] int click_x() const { return click_x_; }
  [[nodiscard]] int click_y() const { return click_y_; }

 private:
  int click_x_ = 0;
  int click_y_ = 0;

  static constexpr int kBufferSize = 256;

  // A ring buffer of entries. head_ == tail_ means empty, so it holds at most
  // kBufferSize - 1, and every index wraps modulo kBufferSize.
  uint16_t entries_[kBufferSize]{};
  int head_ = 0;  // the entry Read() returns next
  int tail_ = 0;  // where Put() writes the next entry
};

}  // namespace engine::window

#endif  // CNC_RED_ALERT_ENGINE_WINDOW_KEYBOARD_H_
