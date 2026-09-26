#include "engine/window/keyboard.h"

#include <SDL_events.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <SDL_scancode.h>

#include <cstdint>
#include <span>

#include "base/array.h"
#include "base/numeric.h"
#include "engine/window/ww_mouse.h"
#include "engine/window/ww_win.h"

namespace engine::window {

// Mask for modifier keys that affect gameplay input.
// Excludes toggle modifiers (Caps Lock, Num Lock, Scroll Lock) so that their
// state doesn't interfere with keyboard handling.
constexpr auto kInputModifierMask =
    static_cast<SDL_Keymod>(uint32_t{KMOD_SHIFT} | uint32_t{KMOD_CTRL} |
                            uint32_t{KMOD_ALT} | uint32_t{KMOD_GUI});

// A Windows virtual-key code and the key number it names.
struct WindowsKey {
  int windows_key;
  int key;
};

// The Windows virtual-key codes the original Windows game used, and the keys
// they name here. A key number is an SDL scancode, so most entries name one.
// The left and right modifier codes land on the KN_L* numbers, as the key
// numbers do; where two codes name one key, the one the Windows game wrote
// comes first, so WindowsKeyFromKey() gives it back.
constexpr WindowsKey kWindowsKeys[] = {
    {0x01, KN_LMOUSE},
    {0x02, KN_RMOUSE},
    {0x04, KN_MMOUSE},
    {0x08, SDL_SCANCODE_BACKSPACE},
    {0x09, SDL_SCANCODE_TAB},
    {0x0C, SDL_SCANCODE_KP_5},  // VK_CLEAR: keypad 5 with Num Lock off
    {0x0D, SDL_SCANCODE_RETURN},
    {0x10, KN_LSHIFT},
    {0x11, KN_LCTRL},
    {0x12, KN_LALT},
    {0x13, SDL_SCANCODE_PAUSE},
    {0x14, SDL_SCANCODE_CAPSLOCK},
    {0x1B, SDL_SCANCODE_ESCAPE},
    {0x20, SDL_SCANCODE_SPACE},
    {0x21, SDL_SCANCODE_PAGEUP},
    {0x22, SDL_SCANCODE_PAGEDOWN},
    {0x23, SDL_SCANCODE_END},
    {0x24, SDL_SCANCODE_HOME},
    {0x25, SDL_SCANCODE_LEFT},
    {0x26, SDL_SCANCODE_UP},
    {0x27, SDL_SCANCODE_RIGHT},
    {0x28, SDL_SCANCODE_DOWN},
    {0x2C, SDL_SCANCODE_PRINTSCREEN},
    {0x2D, SDL_SCANCODE_INSERT},
    {0x2E, SDL_SCANCODE_DELETE},
    {0x30, SDL_SCANCODE_0},
    {0x31, SDL_SCANCODE_1},
    {0x32, SDL_SCANCODE_2},
    {0x33, SDL_SCANCODE_3},
    {0x34, SDL_SCANCODE_4},
    {0x35, SDL_SCANCODE_5},
    {0x36, SDL_SCANCODE_6},
    {0x37, SDL_SCANCODE_7},
    {0x38, SDL_SCANCODE_8},
    {0x39, SDL_SCANCODE_9},
    {0x41, SDL_SCANCODE_A},
    {0x42, SDL_SCANCODE_B},
    {0x43, SDL_SCANCODE_C},
    {0x44, SDL_SCANCODE_D},
    {0x45, SDL_SCANCODE_E},
    {0x46, SDL_SCANCODE_F},
    {0x47, SDL_SCANCODE_G},
    {0x48, SDL_SCANCODE_H},
    {0x49, SDL_SCANCODE_I},
    {0x4A, SDL_SCANCODE_J},
    {0x4B, SDL_SCANCODE_K},
    {0x4C, SDL_SCANCODE_L},
    {0x4D, SDL_SCANCODE_M},
    {0x4E, SDL_SCANCODE_N},
    {0x4F, SDL_SCANCODE_O},
    {0x50, SDL_SCANCODE_P},
    {0x51, SDL_SCANCODE_Q},
    {0x52, SDL_SCANCODE_R},
    {0x53, SDL_SCANCODE_S},
    {0x54, SDL_SCANCODE_T},
    {0x55, SDL_SCANCODE_U},
    {0x56, SDL_SCANCODE_V},
    {0x57, SDL_SCANCODE_W},
    {0x58, SDL_SCANCODE_X},
    {0x59, SDL_SCANCODE_Y},
    {0x5A, SDL_SCANCODE_Z},
    {0x60, SDL_SCANCODE_KP_0},
    {0x61, SDL_SCANCODE_KP_1},
    {0x62, SDL_SCANCODE_KP_2},
    {0x63, SDL_SCANCODE_KP_3},
    {0x64, SDL_SCANCODE_KP_4},
    {0x65, SDL_SCANCODE_KP_5},
    {0x66, SDL_SCANCODE_KP_6},
    {0x67, SDL_SCANCODE_KP_7},
    {0x68, SDL_SCANCODE_KP_8},
    {0x69, SDL_SCANCODE_KP_9},
    {0x6A, SDL_SCANCODE_KP_MULTIPLY},
    {0x6B, SDL_SCANCODE_KP_PLUS},
    {0x6D, SDL_SCANCODE_KP_MINUS},
    {0x6E, SDL_SCANCODE_KP_PERIOD},
    {0x6F, SDL_SCANCODE_KP_DIVIDE},
    {0x70, SDL_SCANCODE_F1},
    {0x71, SDL_SCANCODE_F2},
    {0x72, SDL_SCANCODE_F3},
    {0x73, SDL_SCANCODE_F4},
    {0x74, SDL_SCANCODE_F5},
    {0x75, SDL_SCANCODE_F6},
    {0x76, SDL_SCANCODE_F7},
    {0x77, SDL_SCANCODE_F8},
    {0x78, SDL_SCANCODE_F9},
    {0x79, SDL_SCANCODE_F10},
    {0x7A, SDL_SCANCODE_F11},
    {0x7B, SDL_SCANCODE_F12},
    {0x90, SDL_SCANCODE_NUMLOCKCLEAR},
    {0x91, SDL_SCANCODE_SCROLLLOCK},
    {0xA0, KN_LSHIFT},
    {0xA1, KN_RSHIFT},
    {0xA2, KN_LCTRL},
    {0xA3, KN_RCTRL},
    {0xA4, KN_LALT},
    {0xA5, KN_RALT},
    {0xBA, SDL_SCANCODE_SEMICOLON},
    {0xBB, SDL_SCANCODE_EQUALS},
    {0xBC, SDL_SCANCODE_COMMA},
    {0xBD, SDL_SCANCODE_MINUS},
    {0xBE, SDL_SCANCODE_PERIOD},
    {0xBF, SDL_SCANCODE_SLASH},
    {0xC0, SDL_SCANCODE_GRAVE},
    {0xDB, SDL_SCANCODE_LEFTBRACKET},
    {0xDC, SDL_SCANCODE_BACKSLASH},
    {0xDD, SDL_SCANCODE_RIGHTBRACKET},
    {0xDE, SDL_SCANCODE_APOSTROPHE},
};

// The bits above the key code that a stored hotkey keeps. The Windows game's
// virtual-key bit (0x1000) and anything higher are dropped.
constexpr uint32_t kHotkeyModifierBits =
    kKeyShiftBit | kKeyCtrlBit | kKeyAltBit | kKeyReleaseBit;

KeyNumber KeyBuffer::Peek() {
  // Pumping here is what lets the games' "wait for a key" loops, which only
  // call Peek(), ever see new input.
  SDL_Event_Loop();

  if (head_ == tail_) {
    return KN_NONE;
  }

  // head_ always addresses a key entry: Read steps past the two coordinate
  // entries that follow a mouse key, so a click at x or y 0 is never read here.
  // An entry carries modifier bits, so it rarely names an enumerator.
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  return static_cast<KeyNumber>(base::At(entries_, head_));
}

KeyNumber KeyBuffer::Read() {
  // Peek() pumps SDL events, so the wait ends as soon as a key arrives.
  KeyNumber key = Peek();
  while (key == KN_NONE) {
    key = Peek();
  }
  int entry_count = 1;
  if (IsMouseKey(key)) {
    // A click's position rides in the two entries behind the button.
    click_x_ = base::At(entries_, (head_ + 1) % kBufferSize);
    click_y_ = base::At(entries_, (head_ + 2) % kBufferSize);
    entry_count = 3;
  }
  head_ = (head_ + entry_count) % kBufferSize;
  return key;
}

bool KeyBuffer::Put(const int entry) {
  // One slot always stays free: a full buffer would otherwise have head_ ==
  // tail_ and read as empty.
  const int next_tail = (tail_ + 1) % kBufferSize;
  if (next_tail != head_) {
    base::At(entries_, tail_) = static_cast<uint16_t>(entry);

    tail_ = next_tail;
    return true;
  }
  return false;
}

bool KeyBuffer::PutKey(const int key_code, const bool release) {
  // Scancode 0 is a key SDL does not know: it would be indistinguishable from
  // Peek's empty-buffer result and leave Read spinning. Scancodes above 0xFF
  // (SDL's media and browser keys) would spill into the modifier bits, where
  // "next track" reads as a shifted right click. Neither has a key code, so
  // drop them before any bit is added.
  if (key_code <= 0 || key_code > int{kScancodeMask}) {
    return false;
  }
  auto key_number = static_cast<uint32_t>(key_code);

  const auto keymod =
      static_cast<SDL_Keymod>(SDL_GetModState() & kInputModifierMask);
  if (keymod & KMOD_SHIFT) {
    key_number |= kKeyShiftBit;
  }
  if (keymod & KMOD_CTRL) {
    key_number |= kKeyCtrlBit;
  }
  if (keymod & KMOD_ALT) {
    key_number |= kKeyAltBit;
  }
  if (release) {
    key_number |= kKeyReleaseBit;
  }

  return Put(static_cast<int>(key_number));
}

bool KeyBuffer::PutClick(const int button, const bool release, const int x,
                         const int y) {
  // A click is three entries, queued all or not at all: a button without its
  // position would make Read() take one from past the tail. Put() keeps one
  // slot free, so that slot does not count.
  const int free_entries = (head_ - tail_ + kBufferSize - 1) % kBufferSize;
  if (free_entries < 3) {
    return false;
  }

  // No modifier bits: the DOS version never set them on a button, and the
  // click handlers compare the button without masking them off.
  auto key_number = static_cast<uint32_t>(button);
  if (release) {
    key_number |= kKeyReleaseBit;
  }
  Put(static_cast<int>(key_number));
  Put(x);
  Put(y);
  return true;
}

char KeyBuffer::ToAscii(const int key) {
  // A key number is a scancode in the low byte with modifier bits above it.
  const auto bits = static_cast<uint32_t>(key);
  if (bits & kKeyReleaseBit) {
    return '\0';
  }

  // SDL_GetKeyFromScancode maps through the keyboard layout but takes no
  // modifiers, so Shift never changes the result. Doing better needs SDL text
  // input events (or SDL3, whose version takes the modifiers).
  const int keycode =
      SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(bits & kScancodeMask));

  // SDL keycodes for keys that type a character are that character; every
  // other key's code has SDLK_SCANCODE_MASK set and lands above 'z'.
  return keycode <= SDLK_z ? static_cast<char>(keycode) : '\0';
}

void KeyBuffer::Clear() { head_ = tail_; }

bool KeyBuffer::IsDown(const int key) {
  switch (key) {
    // Gadgets poll the buttons through here to follow a drag or a held
    // button.
    case KN_LMOUSE:
      return (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_LMASK) != 0;
    case KN_RMOUSE:
      return (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_RMASK) != 0;
    // SDL's modifier state covers both sides of the keyboard, which is what
    // the KN_R* names (equal to their KN_L* twins) ask for.
    case KN_LSHIFT:
      return IsShiftDown();
    case KN_LCTRL:
      return IsCtrlDown();
    case KN_LALT:
      return IsAltDown();
    default:
      break;
  }

  int key_count = 0;
  const auto* key_states = SDL_GetKeyboardState(&key_count);

  if (key >= 0 && key < key_count) {
    // SDL_GetKeyboardState returns exactly key_count state bytes.
    // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
    const std::span states(key_states, base::ToSize(key_count));
    return base::At(states, key) != 0;
  }

  return false;
}

bool KeyBuffer::IsMouseKey(int key) {
  // Only the key-code byte; the modifier and release bits say nothing about
  // which key it is.
  key = static_cast<int>(static_cast<uint32_t>(key) & kScancodeMask);
  return key == KN_LMOUSE || key == KN_MMOUSE || key == KN_RMOUSE;
}

bool KeyBuffer::HandleEvent(const SDL_Event* event) {
  switch (event->type) {
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: {
      // SDL numbers the buttons left, middle, right; the key numbers go left,
      // right, middle.
      int button = event->button.button;
      if (button == SDL_BUTTON_RIGHT) {
        button = KN_RMOUSE;
      } else if (button == SDL_BUTTON_MIDDLE) {
        button = KN_MMOUSE;
      } else if (button != SDL_BUTTON_LEFT) {
        // Extra buttons have no key code. SDL_BUTTON_LEFT is 1, already
        // KN_LMOUSE.
        return false;
      }

      PutClick(button, event->button.state == SDL_RELEASED, event->button.x,
               event->button.y);
      return true;
    }

    case SDL_KEYDOWN:
    case SDL_KEYUP:
      // The key codes are SDL scancodes, so the scancode goes in unchanged.
      PutKey(event->key.keysym.scancode, event->key.state == SDL_RELEASED);
      break;

    case SDL_MOUSEMOTION:
      Update_Mouse_Pos(event->motion.x, event->motion.y);
      break;
    default:
      break;
  }

  return false;
}

KeyNumber KeyFromWindowsKey(const int windows_key) {
  const auto bits = static_cast<uint32_t>(windows_key);
  const auto code = static_cast<int>(bits & kScancodeMask);
  for (const WindowsKey& entry : kWindowsKeys) {
    if (entry.windows_key == code) {
      // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
      return static_cast<KeyNumber>(static_cast<uint32_t>(entry.key) |
                                    (bits & kHotkeyModifierBits));
    }
  }
  return KN_NONE;
}

int WindowsKeyFromKey(const KeyNumber key) {
  const auto bits = static_cast<uint32_t>(key);
  const auto code = static_cast<int>(bits & kScancodeMask);
  for (const WindowsKey& entry : kWindowsKeys) {
    if (entry.key == code) {
      return static_cast<int>(static_cast<uint32_t>(entry.windows_key) |
                              (bits & kHotkeyModifierBits));
    }
  }
  return 0;
}

bool IsShiftDown() { return (SDL_GetModState() & KMOD_SHIFT) != 0; }

bool IsCtrlDown() { return (SDL_GetModState() & KMOD_CTRL) != 0; }

bool IsAltDown() { return (SDL_GetModState() & KMOD_ALT) != 0; }

}  // namespace engine::window
