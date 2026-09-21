// more portable replacements for winstub

#include <SDL_events.h>
#include <SDL_video.h>

#include <cstdio>
#include <cstdlib>

#include "sdllib/graphic_buffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/timer.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"
#include "td/game_state.h"
#include "td/input.h"
#include "td/msgbox.h"
#include "td/nullconn.h"
#include "td/palette.h"
#include "td/palettes.h"
#include "td/rand.h"
#include "td/screen.h"
#include "td/startup.h"
#include "td/winstub.h"
#include "winvq/vqa32/vqaplay.h"

void CCDebugString(const char* /*string*/) {}

void Check_For_Focus_Loss() {
  if (!TheGameState().in_focus()) {
    SDL_Event_Loop();
    if (TheGameState().in_focus()) {
      VQA_ResumeAudio();
    }
  }
}

void Memory_Error_Handler() {
  TheScreen().visible_page().Clear();
  Set_Palette(ThePalettes().game_palette());
  while (Get_Mouse_State()) {
    Show_Mouse();
  }
  CCMessageBox().Process("Error - out of memory.", "Abort");

  ShutDown();
  exit(EXIT_FAILURE);
}

#define WINDOW_NAME "Command & Conquer"

void Create_Main_Window(HANDLE /*instance*/, int /*command_show*/, int width,
                        int height) {
  SDL_Create_Main_Window(WINDOW_NAME, width, height);

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

      fflush(stdout);
      exit(0);
    default:
      break;
  }
}

// SHAKESCR.ASM in WIN32LIB
// based on Shake_The_Screen in RA's conquer.cpp
void Shake_Screen(int shakes) {
  shakes += shakes;

  Hide_Mouse();
  TheScreen().visible_view().Blit(TheScreen().hidden_view());
  const int oldyoff = 0;
  int newyoff = 0;
  while (shakes--) {
    const int x = static_cast<int>(SystemTicks());

    do {
      newyoff = Sim_Random_Pick(0, 2) - 1;
    } while (newyoff == oldyoff);
    switch (newyoff) {
      case -1:
        TheScreen().hidden_view().Blit(TheScreen().visible_view(), 0, 2, 0, 0,
                                       640, 398);
        break;
      case 0:
        TheScreen().hidden_view().Blit(TheScreen().visible_view());
        break;
      case 1:
        TheScreen().hidden_view().Blit(TheScreen().visible_view(), 0, 0, 0, 2,
                                       640, 398);
        break;
      default:
        break;
    }
    while (x == SystemTicks()) {
      Video_End_Frame();
    }
  }

  TheScreen().hidden_view().Blit(TheScreen().visible_view());
  Show_Mouse();
}
