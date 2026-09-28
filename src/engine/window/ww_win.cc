#include "engine/window/ww_win.h"

#include <SDL_events.h>

#include "engine/window/display.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

namespace {
// The hooks the game installed; nullptr until it does.
EventHandler g_event_handler = nullptr;
PumpHandler g_pump_handler = nullptr;
}  // namespace

void SetEventHandler(const EventHandler handler) { g_event_handler = handler; }
void SetPumpHandler(const PumpHandler handler) { g_pump_handler = handler; }

void SDL_Event_Loop() {
#ifdef __EMSCRIPTEN__
  // The games wait for input by spinning on this loop, and the browser only
  // delivers input while it has control, so yield to it on every pass.
  emscripten_sleep(0);
#endif

  if (g_pump_handler) {
    g_pump_handler();
  }

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    // Display posts this from its timer thread when drawing has waited in the
    // surface long enough; the game never sees it.
    if (engine::window::TheDisplay().IsRedrawEvent(event.type)) {
      engine::window::TheDisplay().EndFrame();
      continue;
    }
    if (g_event_handler) {
      g_event_handler(&event);
    }
  }
}

void SDL_Send_Quit() {
  SDL_Event quit_event;
  quit_event.type = SDL_QUIT;
  SDL_PushEvent(&quit_event);
}
