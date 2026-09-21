#include "sdllib/ww_win.h"

#include <SDL_events.h>

#include <cstdio>

#include "absl/strings/str_format.h"
#include "sdllib/display.h"
#include "sdllib/net_select.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

// The window rows the games draw text into. sdllib owns the storage and
// each game fills in its own rows at startup, which is what kept every
// game and every test that links sdllib defining this array for itself.
int WindowList[kWindowCount][8]{};

unsigned int WinX;
unsigned int WinY;
unsigned int Window;

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

  // this is replacing WSAAsyncSelect, which would send through the windows
  // event loop
  Socket_Select();

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (TheDisplay().IsRedrawEvent(event.type)) {
      TheDisplay().EndFrame();
      continue;
    }
    SDL_Event_Handler(&event);
  }
}

void SDL_Send_Quit() {
  SDL_Event quit_event;
  quit_event.type = SDL_QUIT;
  SDL_PushEvent(&quit_event);
}
