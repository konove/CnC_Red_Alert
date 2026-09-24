// more portable replacements for winstub

#include <SDL_events.h>
#include <SDL_video.h>

#include <cstdlib>

#include "ra/config.h"
#include "ra/game_state.h"
#include "ra/input.h"
#include "ra/jshell.h"
#include "ra/language.h"
#include "ra/msgbox.h"
#include "ra/nullconn.h"
#include "ra/palette.h"
#include "ra/palettes.h"
#include "ra/screen.h"
#include "ra/startup.h"
#include "ra/winstub.h"
#include "sdllib/display.h"
#include "sdllib/misc.h"
#include "sdllib/pixel_buffer.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"
#include "tech/audio_mixer.h"

void WWDebugString(const char* /*string*/) {}

void Check_For_Focus_Loss() {
  if (!TheGameState().in_focus()) {
    SDL_Event_Loop();
    if (TheGameState().in_focus()) {
      TheAudio().SetExtraPaused(false);
    }
  }
}
void Memory_Error_Handler() {
  TheScreen().visible_page().view().Clear();
  ThePalettes().title_palette().Set();
  while (Get_Mouse_State()) {
    Show_Mouse();
  }
  WWMessageBox().Process(kLanguageText.memory_error, kLanguageText.abort);

  ShutDown();
  exit(EXIT_FAILURE);
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
  TheDisplay().Init(kWindowName, width, height);

  // Audio_Focus_Loss_Function = &Focus_Loss;
  Misc_Focus_Loss_Function = &Focus_Loss;
  Misc_Focus_Restore_Function = &Focus_Restore;
  // Gbuffer_Focus_Loss_Function = &Focus_Loss;
}

void SDL_Event_Handler(SDL_Event* event) {
  if (TheKeyboard().Event_Handler(event)) {
    return;
  }

  switch (event->type) {
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
