#include "sdllib/ww_win.h"

#include <SDL.h>
#include <SDL_events.h>
#include <SDL_render.h>
#include <SDL_stdinc.h>
#include <SDL_video.h>

#include <chrono>
#include <cstdio>
#include <thread>

#include "absl/strings/str_format.h"
#include "sdllib/net_select.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

unsigned int WinX;
unsigned int WinY;
unsigned int Window;

SDL_Renderer* SDLRenderer;
Uint32 ForceRenderEventID;

int Change_Window(int /*windnum*/) {
  absl::PrintF("%s\n", __func__);
  return 0;
}

void SDL_Create_Main_Window(const char* title, int width, int height) {
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

  // Window scale multiplier (2x = 1280x800 for 640x400 logical resolution)
  constexpr auto kWindowScale = 3;
  const auto window_width = width * kWindowScale;
  const auto window_height = height * kWindowScale;

  // Created hidden and shown once the renderer exists: SDL's OpenGL renderer
  // destroys and recreates a window whose GL attributes don't match its own,
  // so a window shown here would flash up and be replaced by a second one.
  constexpr Uint32 kWindowFlags = Uint32{SDL_WINDOW_RESIZABLE} |
                                  Uint32{SDL_WINDOW_ALLOW_HIGHDPI} |
                                  Uint32{SDL_WINDOW_HIDDEN};
  MainWindow =
      SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       window_width, window_height, kWindowFlags);

  ForceRenderEventID = SDL_RegisterEvents(1);

  SDLRenderer = SDL_CreateRenderer(static_cast<SDL_Window*>(MainWindow), -1,
                                   SDL_RENDERER_PRESENTVSYNC);

  // Keep logical size at original resolution, SDL will scale to window size
  SDL_RenderSetLogicalSize(SDLRenderer, width, height);
  SDL_RenderSetIntegerScale(SDLRenderer, SDL_TRUE);
  SDL_ShowWindow(static_cast<SDL_Window*>(MainWindow));

  // sometimes the window won't be created until it has content
  // so we get stuck waiting for focus, which it'll never get because it doesn't
  // exist
  SDL_RenderClear(SDLRenderer);
  SDL_RenderPresent(SDLRenderer);
}

void PresentFrame() {
  // Shorter than a 60 Hz refresh, so on a vsync display, where the present
  // itself blocks for the refresh, the floor is never reached and cannot make
  // a frame miss its vblank. The renderer's vsync flag cannot tell the two
  // cases apart: the software renderer sets it without waiting.
  constexpr std::chrono::microseconds kMinInterval(1'000'000 / 70);
  // The earliest time the next present may happen.
  static std::chrono::steady_clock::time_point next_present;
  const auto now = std::chrono::steady_clock::now();
  if (now < next_present) {
    std::this_thread::sleep_until(next_present);
    next_present += kMinInterval;
  } else {
    // Late, or the first present: restart the cadence from now rather than
    // presenting a burst of frames to catch up.
    next_present = now + kMinInterval;
  }
  SDL_RenderPresent(SDLRenderer);
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
    if (event.type == ForceRenderEventID) {
      Video_End_Frame();
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
