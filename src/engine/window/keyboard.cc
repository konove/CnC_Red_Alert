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
      return (SDL_GetModState() & KMOD_SHIFT) != 0;
    case KN_LCTRL:
      return (SDL_GetModState() & KMOD_CTRL) != 0;
    case KN_LALT:
      return (SDL_GetModState() & KMOD_ALT) != 0;
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
  return key == VK_LBUTTON || key == VK_MBUTTON || key == VK_RBUTTON;
}

bool KeyBuffer::HandleEvent(const SDL_Event* event) {
  switch (event->type) {
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: {
      // SDL numbers the buttons left, middle, right; the VK codes go left,
      // right, middle.
      int button = event->button.button;
      if (button == SDL_BUTTON_RIGHT) {
        button = VK_RBUTTON;
      } else if (button == SDL_BUTTON_MIDDLE) {
        button = VK_MBUTTON;
      } else if (button != SDL_BUTTON_LEFT) {
        // Extra buttons have no key code. SDL_BUTTON_LEFT is 1, already
        // VK_LBUTTON.
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

}  // namespace engine::window
