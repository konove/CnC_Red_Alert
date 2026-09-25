#include "engine/window/keyboard.h"

#include <SDL_events.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <SDL_scancode.h>

#include <cstddef>
#include <cstdint>
#include <span>

#include "base/array.h"
#include "engine/window/ww_mouse.h"
#include "engine/window/ww_win.h"

WWKeyboardClass* ActiveKeyboard = nullptr;

// Mask for modifier keys that affect gameplay input.
// Excludes toggle modifiers (Caps Lock, Num Lock, Scroll Lock) so that their
// state doesn't interfere with keyboard handling.
constexpr SDL_Keymod kInputModifierMask =
    static_cast<SDL_Keymod>(uint32_t{KMOD_SHIFT} | uint32_t{KMOD_CTRL} |
                            uint32_t{KMOD_ALT} | uint32_t{KMOD_GUI});

WWKeyboardClass::WWKeyboardClass() = default;

int WWKeyboardClass::Check() {
  // Pumping here is what lets the games' "wait for a key" loops, which only
  // call Check(), ever see new input.
  SDL_Event_Loop();

  if (Head == Tail) {
    return 0;
  }

  // Head always addresses a key entry: Buff_Get steps past the two coordinate
  // entries that follow a mouse key, so a click at x or y 0 is never read here.
  return base::At(Buffer, Head);
}

int WWKeyboardClass::Get() {
  while (!Check()) {
  }  // wait for key in buffer
  return Buff_Get();
}

bool WWKeyboardClass::Put(int key) {
  // One slot always stays free: a full buffer would otherwise have Head ==
  // Tail and read as empty.
  const int temp = (Tail + 1) % 256;
  if (temp != Head) {
    base::At(Buffer, Tail) = static_cast<uint16_t>(key);

    Tail = temp;
    return true;
  }
  return false;
}

bool WWKeyboardClass::Put_Key_Message(const int vk_key, const bool release) {
  // Scancode 0 is a key SDL does not know: it would be indistinguishable from
  // Check's empty-buffer result and leave Get spinning. Scancodes above 0xFF
  // (SDL's media and browser keys) would spill into the modifier bits, where
  // "next track" reads as a shifted right click. Neither has a key code, so
  // drop them before any bit is added.
  if (vk_key <= 0 || vk_key > 0xFF) {
    return false;
  }
  // The key number under construction; the WWKEY_* bits are ORed in.
  auto key = static_cast<uint32_t>(vk_key);

  // Mouse buttons get no modifier bits: the DOS version never set them, and
  // the click handlers compare the button without masking them off.
  if (vk_key != VK_LBUTTON && vk_key != VK_MBUTTON && vk_key != VK_RBUTTON) {
    const auto keymod =
        static_cast<SDL_Keymod>(SDL_GetModState() & kInputModifierMask);

    if (keymod & KMOD_SHIFT) {
      key |= WWKEY_SHIFT_BIT;
    }

    if (keymod & KMOD_CTRL) {
      key |= WWKEY_CTRL_BIT;
    }

    if (keymod & KMOD_ALT) {
      key |= WWKEY_ALT_BIT;
    }
  }
  if (release) {
    key |= WWKEY_RLS_BIT;
  }

  return Put(static_cast<int>(key));
}

int WWKeyboardClass::To_ASCII(int num) {
  // A key number is a scancode in the low byte with modifier bits above it.
  const auto bits = static_cast<uint32_t>(num);
  if (bits & WWKEY_RLS_BIT) {
    return 0;
  }

  // SDL_GetKeyFromScancode maps through the keyboard layout but takes no
  // modifiers, so Shift never changes the result. Doing better needs SDL text
  // input events (or SDL3, whose version takes the modifiers).
  const int key =
      SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(bits & 0xFF));

  // SDL keycodes for keys that type a character are that character; every
  // other key's code has SDLK_SCANCODE_MASK set and lands above 'z'.
  if (key <= SDLK_z) {
    return key;
  }

  return 0;
}

void WWKeyboardClass::Clear() { Head = Tail; }

bool WWKeyboardClass::Down(int key) {
  // Gadgets poll the buttons through here to follow a drag or a held button.
  if (Is_Mouse_Key(key)) {
    const auto buttons = SDL_GetMouseState(nullptr, nullptr);

    switch (key) {
      case KN_LMOUSE:
        return (buttons & SDL_BUTTON(1)) != 0;
      case KN_RMOUSE:
        return (buttons & SDL_BUTTON(3)) != 0;
      default:
        break;
    }
  }

  // SDL's modifier state covers both sides of the keyboard, which is what the
  // KN_R* names (equal to their KN_L* twins) ask for.
  if (key == KN_LSHIFT || key == KN_LCTRL || key == KN_LALT) {
    const auto keymod =
        static_cast<SDL_Keymod>(SDL_GetModState() & kInputModifierMask);
    switch (key) {
      case KN_LSHIFT:
        return (keymod & KMOD_SHIFT) != 0;
      case KN_LCTRL:
        return (keymod & KMOD_CTRL) != 0;
      case KN_LALT:
        return (keymod & KMOD_ALT) != 0;
      default:
        break;
    }
  }

  int numkeys = 0;
  const auto* keys = SDL_GetKeyboardState(&numkeys);

  if (key >= 0 && key < numkeys) {
    // SDL_GetKeyboardState returns exactly numkeys state bytes.
    // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
    const std::span states(keys, static_cast<size_t>(numkeys));
    return base::At(states, key) != 0;
  }

  return false;
}

bool WWKeyboardClass::Is_Mouse_Key(int key) {
  // Only the key-code byte; the modifier and release bits say nothing about
  // which key it is.
  key = static_cast<int>(static_cast<uint32_t>(key) & 0xFF);
  return key == VK_LBUTTON || key == VK_MBUTTON || key == VK_RBUTTON;
}

bool WWKeyboardClass::Event_Handler(SDL_Event* event) {
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

      // A click is three entries, queued all or not at all: a button without
      // its position would make Buff_Get read one from past the tail.
      const int free_entries = (Head - Tail + 255) % 256;
      if (free_entries < 3) {
        return true;
      }
      Put_Key_Message(button, event->button.state == SDL_RELEASED);
      Put(event->button.x);
      Put(event->button.y);
      return true;
    }

    case SDL_KEYDOWN:
    case SDL_KEYUP:
      // The key codes are SDL scancodes, so the scancode goes in unchanged.
      Put_Key_Message(event->key.keysym.scancode,
                      event->key.state == SDL_RELEASED);
      break;

    case SDL_MOUSEMOTION:
      Update_Mouse_Pos(event->motion.x, event->motion.y);
      break;
    default:
      break;
  }

  return false;
}

int WWKeyboardClass::Buff_Get() {
  while (!Check()) {
  }  // wait for key in buffer
  const int temp = base::At(Buffer, Head);
  int newhead = Head;
  if (Is_Mouse_Key(temp)) {
    // A click's position rides in the two entries behind the button.
    MouseQX = base::At(Buffer, (Head + 1) % 256);
    MouseQY = base::At(Buffer, (Head + 2) % 256);
    newhead += 3;
  } else {
    newhead += 1;
  }

  newhead %= 256;
  Head = newhead;
  return temp;
}
