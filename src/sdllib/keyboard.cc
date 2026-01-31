#include "sdllib/include/keyboard.h"

#include <SDL_events.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <SDL_scancode.h>

#include <cstring>

#include "sdllib/include/ww_mouse.h"
#include "sdllib/include/ww_win.h"

// Mask for modifier keys that affect gameplay input.
// Excludes toggle modifiers (Caps Lock, Num Lock, Scroll Lock) so that their
// state doesn't interfere with keyboard handling.
constexpr SDL_Keymod kInputModifierMask =
    static_cast<SDL_Keymod>(KMOD_SHIFT | KMOD_CTRL | KMOD_ALT | KMOD_GUI);

WWKeyboardClass::WWKeyboardClass() {
  // clear buffer
  memset(Buffer, 0, 256);
}

bool WWKeyboardClass::Check() {
  // poll for events, return key if any pressed
  SDL_Event_Loop();

  if (Head == Tail) {
    return false;
  }

  return Buffer[Head];
}

int WWKeyboardClass::Get() {
  while (!Check()) {
  }  // wait for key in buffer
  return Buff_Get();
}

bool WWKeyboardClass::Put(int key) {
  int temp = Tail + 1 & 255;
  if (temp != Head) {
    Buffer[Tail] = static_cast<short>(key);

    Tail = temp;
    return true;
  }
  return false;
}

bool WWKeyboardClass::Put_Key_Message(unsigned vk_key, bool release) {
  //
  // Get the status of keyboard modifiers, excluding toggle modifiers (Caps
  // Lock, Num Lock). Note that we do not want to set the shift, ctrl and alt
  // bits for Mouse keypresses as this would be incompatible with the dos
  // version.
  //
  if (vk_key != VK_LBUTTON && vk_key != VK_MBUTTON && vk_key != VK_RBUTTON) {
    auto keymod =
        static_cast<SDL_Keymod>(SDL_GetModState() & kInputModifierMask);

    //
    // Set the proper bits for whatever the key we got is.
    //
    if (keymod & KMOD_SHIFT) {
      vk_key |= WWKEY_SHIFT_BIT;
    }

    if (keymod & KMOD_CTRL) {
      vk_key |= WWKEY_CTRL_BIT;
    }

    if (keymod & KMOD_ALT) {
      vk_key |= WWKEY_ALT_BIT;
    }
  }
  if (release) {
    vk_key |= WWKEY_RLS_BIT;
  }

  //
  // Finally use the put command to enter the key into the keyboard
  // system.
  //
  return Put(vk_key);
}

int WWKeyboardClass::To_ASCII(int num) {
  if (num & WWKEY_RLS_BIT) {
    return 0;
  }

  // this isn't great but we can't do much better without rewriting everything
  // to use textinput events (SDL3 would allow passing the mods in)
  int key = SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(num & 0xFF));

  if (key <= SDLK_z) {
    return key;
  }

  return 0;
}

void WWKeyboardClass::Clear() { Head = Tail; }

int WWKeyboardClass::Down(int key) {
  // gadget uses this to poll mouse buttons
  if (Is_Mouse_Key(key)) {
    auto buttons = SDL_GetMouseState(nullptr, nullptr);

    switch (key) {
      case KN_LMOUSE:
        return buttons & SDL_BUTTON(1);
      case KN_RMOUSE:
        return buttons & SDL_BUTTON(3);
    }
  }

  if (key == KN_LSHIFT || key == KN_LCTRL || key == KN_LALT) {
    auto keymod =
        static_cast<SDL_Keymod>(SDL_GetModState() & kInputModifierMask);
    switch (key) {
      case KN_LSHIFT:
        return keymod & KMOD_SHIFT;
      case KN_LCTRL:
        return keymod & KMOD_CTRL;
      case KN_LALT:
        return keymod & KMOD_ALT;
    }
  }

  int numkeys;
  const auto* keys = SDL_GetKeyboardState(&numkeys);

  if (key < numkeys) {
    return keys[key];
  }

  return 0;
}

bool WWKeyboardClass::Is_Mouse_Key(int key) {
  key &= 0xFF;
  return key == VK_LBUTTON || key == VK_MBUTTON || key == VK_RBUTTON;
}

bool WWKeyboardClass::Event_Handler(SDL_Event* event) {
  switch (event->type) {
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: {
      int button = event->button.button;
      if (button == SDL_BUTTON_RIGHT) {
        button = VK_RBUTTON;
      } else if (button == SDL_BUTTON_MIDDLE) {
        button = VK_MBUTTON;
      } else if (button != SDL_BUTTON_LEFT) {  // left == 1, which is the same
        return false;
      }

      Put_Key_Message(button, event->button.state == SDL_RELEASED);
      Put(event->button.x);
      Put(event->button.y);
      return true;
    }

    case SDL_KEYDOWN:
    case SDL_KEYUP:
      Put_Key_Message(event->key.keysym.scancode,
                      event->key.state == SDL_RELEASED);
      break;

    case SDL_MOUSEMOTION:
      Update_Mouse_Pos(event->motion.x, event->motion.y);
      break;
  }

  return false;
}

int WWKeyboardClass::Buff_Get() {
  while (!Check()) {
  }  // wait for key in buffer
  int temp = Buffer[Head];             // get key out of the buffer
  int newhead = Head;                  // save off head for manipulation
  if (Is_Mouse_Key(temp)) {            // if key is a mouse then
    MouseQX = Buffer[Head + 1 & 255];  //		get the x and y pos
    MouseQY = Buffer[Head + 2 & 255];  //		from the buffer
    newhead += 3;                      //		adjust head forward
  } else {
    newhead += 1;  //		adjust head forward
  }

  newhead &= 255;
  Head = newhead;
  return temp;
}
