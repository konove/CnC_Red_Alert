// more portable replacements for winstub

#include <SDL_events.h>
#include <SDL_video.h>

#include <cstdlib>

#include "engine/audio/audio_mixer.h"
#include "engine/net/net_select.h"
#include "engine/window/display.h"
#include "engine/window/hardware_cursor.h"
#include "ra/config.h"
#include "ra/game_state.h"
#include "ra/input.h"
#include "ra/nullconn.h"
#include "ra/startup.h"
#include "ra/winstub.h"

void Check_For_Focus_Loss() {
  if (!TheGameState().in_focus()) {
    engine::window::TheDisplay().PumpEvents();
    if (TheGameState().in_focus()) {
      engine::audio::TheAudio().SetExtraPaused(false);
    }
  }
}
static constexpr const char* kWindowName = [] {
  if (config::kIsFrench) {
    return "Alerte Rouge";
  }
  if (config::kIsGerman) {
    return "Alarmstufe Rot";
  }
  return "Red Alert";
}();

void Create_Main_Window(HANDLE /*instance*/, int /*command_show*/, int width,
                        int height) {
  engine::window::TheDisplay().Init(kWindowName, width, height);
}

void WindowEvents::Pump() { Socket_Select(); }

void WindowEvents::HandleEvent(const SDL_Event& event) {
  if (TheKeyboard().HandleEvent(&event)) {
    return;
  }

  switch (event.type) {
    case SDL_MOUSEMOTION:
      TheMouse().RecordPosition(event.motion.x, event.motion.y);
      break;
    case SDL_WINDOWEVENT: {
      switch (event.window.event) {
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
      exit(0);
    default:
      break;
  }
}
