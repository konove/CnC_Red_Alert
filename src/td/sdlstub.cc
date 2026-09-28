// more portable replacements for winstub

#include <SDL_events.h>
#include <SDL_video.h>

#include <cstdio>
#include <cstdlib>

#include "engine/audio/audio_mixer.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/platform/timer.h"
#include "engine/window/display.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "engine/window/ww_win.h"
#include "td/game_state.h"
#include "td/input.h"
#include "td/nullconn.h"
#include "td/rand.h"
#include "td/screen.h"
#include "td/startup.h"
#include "td/winstub.h"

void CCDebugString(const char* /*string*/) {}

void Check_For_Focus_Loss() {
  if (!TheGameState().in_focus()) {
    PumpEvents();
    if (TheGameState().in_focus()) {
      engine::audio::TheAudio().SetExtraPaused(false);
    }
  }
}

#define WINDOW_NAME "Command & Conquer"

static void SDL_Event_Handler(SDL_Event* event);

void Create_Main_Window(HANDLE /*instance*/, int /*command_show*/, int width,
                        int height) {
  engine::window::TheDisplay().Init(WINDOW_NAME, width, height);
  SetEventHandler(&SDL_Event_Handler);
}

static void SDL_Event_Handler(SDL_Event* event) {
  if (TheKeyboard().HandleEvent(event)) {
    return;
  }

  switch (event->type) {
    case SDL_MOUSEMOTION:
      TheMouse().RecordPosition(event->motion.x, event->motion.y);
      break;
    case SDL_WINDOWEVENT: {
      switch (event->window.event) {
        case SDL_WINDOWEVENT_FOCUS_GAINED:
          TheGameState().in_focus() = true;
          Focus_Restore();
          break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
          TheGameState().in_focus() = false;
          Focus_Loss();
          break;
        case SDL_WINDOWEVENT_SIZE_CHANGED:
        case SDL_WINDOWEVENT_DISPLAY_CHANGED:
          TheMouse().UpdateScale();
          break;
        default:
          break;
      }
      break;
    }
    case SDL_QUIT:
      ShutDown();

      fflush(stdout);
      exit(0);
    default:
      break;
  }
}

// SHAKESCR.ASM in WIN32LIB
// based on Shake_The_Screen in RA's conquer.cpp
void ShakeScreen(int shakes) {
  shakes += shakes;

  TheMouse().Hide();
  TheScreen().visible_view().BlitTo(TheScreen().hidden_view());
  const int oldyoff = 0;
  int newyoff = 0;
  while (shakes--) {
    const int x = static_cast<int>(engine::platform::SystemTicks());

    do {
      newyoff = Sim_Random_Pick(0, 2) - 1;
    } while (newyoff == oldyoff);
    switch (newyoff) {
      case -1:
        TheScreen().hidden_view().BlitTo(TheScreen().visible_view(), 0, 2, 0, 0,
                                         640, 398);
        break;
      case 0:
        TheScreen().hidden_view().BlitTo(TheScreen().visible_view());
        break;
      case 1:
        TheScreen().hidden_view().BlitTo(TheScreen().visible_view(), 0, 0, 0, 2,
                                         640, 398);
        break;
      default:
        break;
    }
    while (x == engine::platform::SystemTicks()) {
      engine::window::TheDisplay().EndFrame();
    }
  }

  TheScreen().hidden_view().BlitTo(TheScreen().visible_view());
  TheMouse().Show();
}
