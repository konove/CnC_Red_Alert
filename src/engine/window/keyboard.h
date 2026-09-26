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

// Returns which key `key` is, however it was pressed. A release matches too;
// test kKeyReleaseBit as well to tell a press from a release.
constexpr int KeyCode(const int key) {
  return static_cast<int>(static_cast<uint32_t>(key) & kScancodeMask);
}

// Key numbers: which key, as its SDL scancode, so a key event needs no
// translation. The mouse buttons take 1-3, which SDL never reports for a key.
// The values combine with the KN_*_BIT modifier bits and with KN_BUTTON (see
// ButtonKey()), so the enum stays unscoped. Keys the port cannot tell apart
// share a value: the left and right modifier keys, and each diagonal with its
// navigation key. KN_E_HOME is the numeric keypad's 7, and KN_CENTER its 5.
// Only the keys the games name are listed; any other scancode still arrives
// as its number.
// NOLINTNEXTLINE(cppcoreguidelines-use-enum-class)
enum KeyNumber {
  KN_NONE = 0,

  KN_0 = 39,
  KN_1 = 30,
  KN_2 = 31,
  KN_3 = 32,
  KN_4 = 33,
  KN_5 = 34,
  KN_6 = 35,
  KN_7 = 36,
  KN_8 = 37,
  KN_9 = 38,
  KN_A = 4,
  KN_B = 5,
  KN_BACKSPACE = 42,
  KN_C = 6,
  KN_CENTER = 93,
  KN_COMMA = 54,
  KN_D = 7,
  KN_DELETE = 76,
  KN_DOWN = 81,
  KN_DOWNLEFT = 77,
  KN_DOWNRIGHT = 78,
  KN_E = 8,
  KN_END = 77,
  KN_ESC = 41,
  KN_E_HOME = 95,
  KN_F = 9,
  KN_F1 = 58,
  KN_F10 = 67,
  KN_F11 = 68,
  KN_F12 = 69,
  KN_F2 = 59,
  KN_F3 = 60,
  KN_F4 = 61,
  KN_F5 = 62,
  KN_F6 = 63,
  KN_F7 = 64,
  KN_F8 = 65,
  KN_F9 = 66,
  KN_G = 10,
  KN_GRAVE = 53,
  KN_H = 11,
  KN_HOME = 74,
  // NOLINTNEXTLINE(misc-confusable-identifiers)
  KN_I = 12,
  KN_INSERT = 73,
  KN_J = 13,
  KN_K = 14,
  KN_KEYPAD_RETURN = 40,
  KN_L = 15,
  KN_LALT = 226,
  KN_LCTRL = 224,
  KN_LEFT = 80,
  KN_LMOUSE = 1,
  KN_MMOUSE = 3,
  KN_LSHIFT = 225,
  KN_M = 16,
  KN_N = 17,
  // NOLINTNEXTLINE(misc-confusable-identifiers)
  KN_O = 18,
  KN_P = 19,
  KN_PERIOD = 55,
  KN_PGDN = 78,
  KN_PGUP = 75,
  KN_Q = 20,
  KN_R = 21,
  KN_RALT = 226,
  KN_RCTRL = 224,
  KN_RETURN = 40,
  KN_RIGHT = 79,
  KN_RMOUSE = 2,
  KN_RSHIFT = 225,
  KN_S = 22,
  KN_SLASH = 56,
  KN_SPACE = 44,
  KN_T = 23,
  KN_TAB = 43,
  KN_U = 24,
  KN_UP = 82,
  KN_UPLEFT = 74,
  KN_UPRIGHT = 75,
  KN_V = 25,
  KN_W = 26,
  KN_X = 27,
  KN_Y = 28,
  KN_Z = 29,

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
inline KeyNumber operator&(const KeyNumber a, const KeyNumber b) noexcept {
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  return static_cast<KeyNumber>(static_cast<uint32_t>(a) &
                                static_cast<uint32_t>(b));
}
inline KeyNumber operator~(const KeyNumber a) noexcept {
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  return static_cast<KeyNumber>(~static_cast<uint32_t>(a));
}

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
