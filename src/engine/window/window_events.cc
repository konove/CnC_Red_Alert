#include "engine/window/window_events.h"

#include <SDL_events.h>
#include <SDL_video.h>

#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"

namespace engine::window {

WindowEvents::WindowEvents(KeyBuffer& keyboard, HardwareCursor& mouse)
    : keyboard_(&keyboard), mouse_(&mouse) {}

void WindowEvents::HandleEvent(const SDL_Event& event) {
  if (keyboard_->HandleEvent(&event)) {
    return;
  }

  switch (event.type) {
    case SDL_MOUSEMOTION:
      mouse_->RecordPosition(event.motion.x, event.motion.y);
      break;
    case SDL_WINDOWEVENT:
      switch (event.window.event) {
        case SDL_WINDOWEVENT_FOCUS_GAINED:
          FocusChanged(true);
          break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
          FocusChanged(false);
          break;
        // The cursor is drawn at the picture's scale, which follows the
        // window's size and the pixel density of the screen it is on.
        case SDL_WINDOWEVENT_SIZE_CHANGED:
        case SDL_WINDOWEVENT_DISPLAY_CHANGED:
          mouse_->UpdateScale();
          break;
        default:
          break;
      }
      break;
    case SDL_QUIT:
      Quit();
      break;
    default:
      break;
  }
}

}  // namespace engine::window
