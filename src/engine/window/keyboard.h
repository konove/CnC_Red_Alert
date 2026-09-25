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
// key numbers (KN_*) and characters (KA_*) they are compared against, and the
// modifier bits combined with both. It began as Philip W. Gorrow's Westwood
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
inline constexpr uint32_t kKeyVirtualBit = 0x1000;
inline constexpr uint32_t kKeyButtonBit = 0x8000;

// The low byte of a key number, which holds the key's scancode.
inline constexpr uint32_t kScancodeMask = 0xFFU;

// The part of a key value that says which key it is: the code and whether it
// is a virtual key, without the shift, release and button bits that say how
// it was pressed.
inline constexpr uint32_t kKeyCodeMask = kKeyVirtualBit | kScancodeMask;

// Returns which key `key` is, however it was pressed. A release matches too;
// test kKeyReleaseBit as well to tell a press from a release.
constexpr int KeyCode(const int key) {
  return static_cast<int>(static_cast<uint32_t>(key) & kKeyCodeMask);
}

// Key codes, named after the Windows virtual keys the original used. Their
// values are the SDL scancodes of the same keys, so a key event needs no
// translation. The mouse buttons take scancodes 1-3, which SDL never reports
// for a key, and VK_CONTROL, VK_SHIFT and VK_MENU are the left-hand modifier
// keys.
#define VK_LBUTTON 1
#define VK_RBUTTON 2
#define VK_MBUTTON 3

#define VK_A 4
#define VK_B 5
#define VK_C 6
#define VK_D 7
#define VK_E 8
#define VK_F 9
#define VK_G 10
#define VK_H 11
#define VK_I 12
#define VK_J 13
#define VK_K 14
#define VK_L 15
#define VK_M 16
#define VK_N 17
#define VK_O 18
#define VK_P 19
#define VK_Q 20
#define VK_R 21
#define VK_S 22
#define VK_T 23
#define VK_U 24
#define VK_V 25
#define VK_W 26
#define VK_X 27
#define VK_Y 28
#define VK_Z 29

#define VK_1 30
#define VK_2 31
#define VK_3 32
#define VK_4 33
#define VK_5 34
#define VK_6 35
#define VK_7 36
#define VK_8 37
#define VK_9 38
#define VK_0 39

#define VK_RETURN 40
#define VK_ESCAPE 41
#define VK_BACK 42
#define VK_TAB 43
#define VK_SPACE 44

#define VK_OEM_MINUS 45
#define VK_OEM_PLUS 46  // =
#define VK_OEM_4 47     // [
#define VK_OEM_6 48     // ]
#define VK_OEM_5 49     // backslash
#define VK_OEM_1 51     // ;
#define VK_OEM_7 52     // '
#define VK_OEM_3 53     // `
#define VK_OEM_COMMA 54
#define VK_OEM_PERIOD 55
#define VK_OEM_2 56  // /

#define VK_CAPITAL 57

#define VK_F1 58
#define VK_F2 59
#define VK_F3 60
#define VK_F4 61
#define VK_F5 62
#define VK_F6 63
#define VK_F7 64
#define VK_F8 65
#define VK_F9 66
#define VK_F10 67
#define VK_F11 68
#define VK_F12 69

#define VK_SNAPSHOT 70
#define VK_SCROLL 71
#define VK_PAUSE 72
#define VK_INSERT 73

#define VK_HOME 74
#define VK_PRIOR 75
#define VK_DELETE 76
#define VK_END 77
#define VK_NEXT 78
#define VK_RIGHT 79
#define VK_LEFT 80
#define VK_DOWN 81
#define VK_UP 82

#define VK_NUMLOCK 83

#define VK_DIVIDE 84
#define VK_MULTIPLY 85
#define VK_SUBTRACT 86
#define VK_ADD 87
#define VK_NUMPAD1 89
#define VK_NUMPAD2 90
#define VK_NUMPAD3 91
#define VK_NUMPAD4 92
#define VK_NUMPAD5 93
#define VK_NUMPAD6 94
#define VK_NUMPAD7 95
#define VK_NUMPAD8 96
#define VK_NUMPAD9 97
#define VK_NUMPAD0 98
#define VK_DECIMAL 99

// The keypad 5, which Windows reports as Clear with Num Lock off.
#define VK_CLEAR VK_NUMPAD5

#define VK_CONTROL 224
#define VK_SHIFT 225
#define VK_MENU 226

// Characters as ToAscii() reports them, with the kKey*Bit flags available for
// callers that carry them along. The codes below the space are the text
// printer's formatting commands (KA_MORE, KA_SETBKGDCOL, ...) and the control
// keys. A character with modifier bits is a bit pattern rather than one of
// these values, so the enum stays unscoped.
// NOLINTNEXTLINE(cppcoreguidelines-use-enum-class)
enum KeyAscii {
  KA_NONE = 0,
  KA_MORE = 1,
  KA_SETBKGDCOL = 2,
  KA_SETFORECOL = 6,
  KA_FORMFEED = 12,
  KA_SPCTAB = 20,
  KA_SETX = 25,
  KA_SETY = 26,

  KA_SPACE = 32,        // space
  KA_EXCLAMATION = 33,  // !
  KA_DQUOTE = 34,       // "
  KA_POUND = 35,        // #
  KA_DOLLAR = 36,       // $
  KA_PERCENT = 37,      // %
  KA_AMPER = 38,        // &
  KA_SQUOTE = 39,       // '
  KA_LPAREN = 40,       // (
  KA_RPAREN = 41,       // )
  KA_ASTERISK = 42,     // *
  KA_PLUS = 43,         // +
  KA_COMMA = 44,        // ,
  KA_MINUS = 45,        // -
  KA_PERIOD = 46,       // .
  KA_SLASH = 47,        // /

  KA_0 = 48,
  KA_1 = 49,
  KA_2 = 50,
  KA_3 = 51,
  KA_4 = 52,
  KA_5 = 53,
  KA_6 = 54,
  KA_7 = 55,
  KA_8 = 56,
  KA_9 = 57,
  KA_COLON = 58,         // :
  KA_SEMICOLON = 59,     // ;
  KA_LESS_THAN = 60,     // <
  KA_EQUAL = 61,         // =
  KA_GREATER_THAN = 62,  // >
  KA_QUESTION = 63,      // ?

  KA_AT = 64,  // @
  KA_A = 65,   // A
  KA_B = 66,   // B
  KA_C = 67,   // C
  KA_D = 68,   // D
  KA_E = 69,   // E
  KA_F = 70,   // F
  KA_G = 71,   // G
  KA_H = 72,   // H
  // Key names spell their key's label, so I/1, O/0 and l/1 look alike by
  // design. NOLINTNEXTLINE(misc-confusable-identifiers)
  KA_I = 73,  // I
  KA_J = 74,  // J
  KA_K = 75,  // K
  KA_L = 76,  // L
  KA_M = 77,  // M
  KA_N = 78,  // N
  // NOLINTNEXTLINE(misc-confusable-identifiers)
  KA_O = 79,  // O

  KA_P = 80,          // P
  KA_Q = 81,          // Q
  KA_R = 82,          // R
  KA_S = 83,          // S
  KA_T = 84,          // T
  KA_U = 85,          // U
  KA_V = 86,          // V
  KA_W = 87,          // W
  KA_X = 88,          // X
  KA_Y = 89,          // Y
  KA_Z = 90,          // Z
  KA_LBRACKET = 91,   // [
  KA_BACKSLASH = 92,  // backslash
  KA_RBRACKET = 93,   // ]
  KA_CARROT = 94,     // ^
  KA_UNDERLINE = 95,  // _

  KA_GRAVE = 96,  // `
  KA_a = 97,      // a
  KA_b = 98,      // b
  KA_c = 99,      // c
  KA_d = 100,     // d
  KA_e = 101,     // e
  KA_f = 102,     // f
  KA_g = 103,     // g
  KA_h = 104,     // h
  KA_i = 105,     // i
  KA_j = 106,     // j
  KA_k = 107,     // k
  // NOLINTNEXTLINE(misc-confusable-identifiers)
  KA_l = 108,  // l
  KA_m = 109,  // m
  KA_n = 110,  // n
  KA_o = 111,  // o

  KA_p = 112,       // p
  KA_q = 113,       // q
  KA_r = 114,       // r
  KA_s = 115,       // s
  KA_t = 116,       // t
  KA_u = 117,       // u
  KA_v = 118,       // v
  KA_w = 119,       // w
  KA_x = 120,       // x
  KA_y = 121,       // y
  KA_z = 122,       // z
  KA_LBRACE = 123,  // {
  KA_BAR = 124,     // |
  KA_RBRACE = 125,  // }
  KA_TILDA = 126,   // ~

  KA_ESC = '\x1b',
  KA_RETURN = '\r',
  KA_BACKSPACE = '\b',
  KA_TAB = '\t',

  KA_SHIFT_BIT = kKeyShiftBit,
  KA_CTRL_BIT = kKeyCtrlBit,
  KA_ALT_BIT = kKeyAltBit,
  KA_RLSE_BIT = kKeyReleaseBit,
};

// Key numbers: which key, as its VK_* code. The values combine with the
// KN_*_BIT modifier bits and with KN_BUTTON (see ButtonKey()), so the enum
// stays unscoped. Keys the port cannot tell apart share a value: the left
// and right modifier keys, KN_DELETE and KN_E_DELETE, and each diagonal with
// its navigation key. KN_E_* name the numeric keypad's cursor keys.
// NOLINTNEXTLINE(cppcoreguidelines-use-enum-class)
enum KeyNumber {
  KN_NONE = 0,

  KN_0 = VK_0,
  KN_1 = VK_1,
  KN_2 = VK_2,
  KN_3 = VK_3,
  KN_4 = VK_4,
  KN_5 = VK_5,
  KN_6 = VK_6,
  KN_7 = VK_7,
  KN_8 = VK_8,
  KN_9 = VK_9,
  KN_A = VK_A,
  KN_B = VK_B,
  KN_BACKSLASH = VK_OEM_5,
  KN_BACKSPACE = VK_BACK,
  KN_C = VK_C,
  KN_CAPSLOCK = VK_CAPITAL,
  KN_CENTER = VK_CLEAR,
  KN_COMMA = VK_OEM_COMMA,
  KN_D = VK_D,
  KN_DELETE = VK_DELETE,
  KN_DOWN = VK_DOWN,
  KN_DOWNLEFT = VK_END,
  KN_DOWNRIGHT = VK_NEXT,
  KN_E = VK_E,
  KN_END = VK_END,
  KN_EQUAL = VK_OEM_PLUS,
  KN_ESC = VK_ESCAPE,
  KN_E_DELETE = VK_DELETE,
  KN_E_DOWN = VK_NUMPAD2,
  KN_E_END = VK_NUMPAD1,
  KN_E_HOME = VK_NUMPAD7,
  KN_E_INSERT = VK_INSERT,
  KN_E_LEFT = VK_NUMPAD4,
  KN_E_PGDN = VK_NUMPAD3,
  KN_E_PGUP = VK_NUMPAD9,
  KN_E_RIGHT = VK_NUMPAD6,
  KN_E_UP = VK_NUMPAD8,
  KN_F = VK_F,
  KN_F1 = VK_F1,
  KN_F10 = VK_F10,
  KN_F11 = VK_F11,
  KN_F12 = VK_F12,
  KN_F2 = VK_F2,
  KN_F3 = VK_F3,
  KN_F4 = VK_F4,
  KN_F5 = VK_F5,
  KN_F6 = VK_F6,
  KN_F7 = VK_F7,
  KN_F8 = VK_F8,
  KN_F9 = VK_F9,
  KN_G = VK_G,
  KN_GRAVE = VK_OEM_3,
  KN_H = VK_H,
  KN_HOME = VK_HOME,
  // NOLINTNEXTLINE(misc-confusable-identifiers)
  KN_I = VK_I,
  KN_INSERT = VK_INSERT,
  KN_J = VK_J,
  KN_K = VK_K,
  KN_KEYPAD_ASTERISK = VK_MULTIPLY,
  KN_KEYPAD_MINUS = VK_SUBTRACT,
  KN_KEYPAD_PLUS = VK_ADD,
  KN_KEYPAD_RETURN = VK_RETURN,
  KN_KEYPAD_SLASH = VK_DIVIDE,
  KN_L = VK_L,
  KN_LALT = VK_MENU,
  KN_LBRACKET = VK_OEM_4,
  KN_LCTRL = VK_CONTROL,
  KN_LEFT = VK_LEFT,
  KN_LMOUSE = VK_LBUTTON,
  KN_LSHIFT = VK_SHIFT,
  KN_M = VK_M,
  KN_MINUS = VK_OEM_MINUS,
  KN_N = VK_N,
  KN_NUMLOCK = VK_NUMLOCK,
  // NOLINTNEXTLINE(misc-confusable-identifiers)
  KN_O = VK_O,
  KN_P = VK_P,
  KN_PAUSE = VK_PAUSE,
  KN_PERIOD = VK_OEM_PERIOD,
  KN_PGDN = VK_NEXT,
  KN_PGUP = VK_PRIOR,
  KN_PRNTSCRN = VK_SNAPSHOT,
  KN_Q = VK_Q,
  KN_R = VK_R,
  KN_RALT = VK_MENU,
  KN_RBRACKET = VK_OEM_6,
  KN_RCTRL = VK_CONTROL,
  KN_RETURN = VK_RETURN,
  KN_RIGHT = VK_RIGHT,
  KN_RMOUSE = VK_RBUTTON,
  KN_RSHIFT = VK_SHIFT,
  KN_S = VK_S,
  KN_SCROLLLOCK = VK_SCROLL,
  KN_SEMICOLON = VK_OEM_1,
  KN_SLASH = VK_OEM_2,
  KN_SPACE = VK_SPACE,
  KN_SQUOTE = VK_OEM_7,
  KN_T = VK_T,
  KN_TAB = VK_TAB,
  KN_U = VK_U,
  KN_UP = VK_UP,
  KN_UPLEFT = VK_HOME,
  KN_UPRIGHT = VK_PRIOR,
  KN_V = VK_V,
  KN_W = VK_W,
  KN_X = VK_X,
  KN_Y = VK_Y,
  KN_Z = VK_Z,

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
// Peek() and Read(). Each entry is a key number: a VK_* code in the low byte
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

  // Queues the key `key_code` (a VK_* code), adding the Shift, Ctrl and Alt
  // bits for the modifier keys held right now and kKeyReleaseBit for a
  // `release`. Returns false if the key was dropped: the buffer is full, or
  // the scancode is 0 (a key SDL does not know), negative, or above 0xFF (a
  // media key, which has no key code). Mouse buttons go through PutClick().
  bool PutKey(int key_code, bool release = false);

  // Queues a click of the mouse `button` (VK_LBUTTON, VK_MBUTTON or
  // VK_RBUTTON) at `x`, `y`: the button, with kKeyReleaseBit for a `release`
  // but no modifier bits, as in the DOS version, followed by the position.
  // Returns false, queuing nothing, if the three entries do not all fit.
  bool PutClick(int button, bool release, int x, int y);

  // Returns the character `key` types on the current keyboard layout, with
  // Shift ignored, so letters come back lower case. Returns KA_NONE for a
  // release and for a key that types no character up to 'z' (arrows, function
  // keys, Delete, the mouse buttons).
  static KeyAscii ToAscii(int key);

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

// The key buffer the PeekKey() family reads. Each game points this at its own
// buffer when it builds one, and clears it again afterwards.
extern KeyBuffer* g_active_keyboard;

// The legacy free-function spellings of the g_active_keyboard members.
//
// PeekKey deliberately does not mirror ReadKeyAscii's ASCII translation: its
// callers test whether any key is waiting, and ToAscii reports 0 for key
// releases and for keys that type no character.
inline int PeekKey() { return g_active_keyboard->Peek(); }
inline int ReadKeyAscii() {
  return KeyBuffer::ToAscii(g_active_keyboard->Read());
}
inline int ReadKey() { return g_active_keyboard->Read(); }
inline void ClearKeys() { g_active_keyboard->Clear(); }

}  // namespace engine::window

#endif  // CNC_RED_ALERT_ENGINE_WINDOW_KEYBOARD_H_
