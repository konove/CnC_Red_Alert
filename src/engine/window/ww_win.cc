#include "engine/window/ww_win.h"

#include <SDL_events.h>

#include <cstdio>

#include "absl/strings/str_format.h"
#include "engine/window/display.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

namespace {
EventHandler g_event_handler = nullptr;
PumpHandler g_pump_handler = nullptr;
}  // namespace

void SetEventHandler(const EventHandler handler) { g_event_handler = handler; }
void SetPumpHandler(const PumpHandler handler) { g_pump_handler = handler; }

int Change_Window(int /*windnum*/) {
  absl::PrintF("%s\n", __func__);
  return 0;
}

void SDL_Event_Loop() {
#ifdef __EMSCRIPTEN__
  // sometimes we loop waiting for input
  // which isn't going to happen if the browser never gets control
  emscripten_sleep(0);
#endif

  if (g_pump_handler) {
    g_pump_handler();
  }

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
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
