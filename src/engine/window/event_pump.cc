#include "engine/window/event_pump.h"

#include <SDL_events.h>

#include "engine/window/display.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

namespace engine::window {

namespace {
// The hooks the game installed; nullptr until it does.
EventHandler event_handler = nullptr;
PumpHandler pump_handler = nullptr;
}  // namespace

void SetEventHandler(const EventHandler handler) { event_handler = handler; }
void SetPumpHandler(const PumpHandler handler) { pump_handler = handler; }

void PumpEvents() {
#ifdef __EMSCRIPTEN__
  // The games wait for input by spinning on this loop, and the browser only
  // delivers input while it has control, so yield to it on every pass.
  emscripten_sleep(0);
#endif

  if (pump_handler) {
    pump_handler();
  }

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    // Display posts this from its timer thread when drawing has waited in the
    // surface long enough; the game never sees it.
    if (TheDisplay().IsRedrawEvent(event.type)) {
      TheDisplay().EndFrame();
      continue;
    }
    if (event_handler) {
      event_handler(event);
    }
  }
}

}  // namespace engine::window
