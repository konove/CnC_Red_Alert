// more portable replacements for winstub

#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"
#include "td/globals.h"

#include <SDL_events.h>
#include <SDL_video.h>

#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string_view>

#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/timer.h"
#include "td/externs.h"
#include "td/msgbox.h"
#include "td/nullconn.h"
#include "td/palette.h"
#include "td/rand.h"
#include "winvq/vqa32/vqaplay.h"

bool ReadyToQuit = false;

void CCDebugString(const char* /*string*/) {}

void Check_For_Focus_Loss() {
  if (!GameInFocus) {
    SDL_Event_Loop();
    if (GameInFocus) {
      VQA_ResumeAudio();
    }
  }
}

void Memory_Error_Handler() {
  VisiblePage.Clear();
  Set_Palette(GamePalette);
  while (Get_Mouse_State()) {
    Show_Mouse();
  }
  CCMessageBox().Process("Error - out of memory.", "Abort");

  exit(0);
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
  if (Kbd.Event_Handler(event)) {
    return;
  }

  switch (event->type) {
    case SDL_WINDOWEVENT: {
      switch (event->window.event) {
        case SDL_WINDOWEVENT_FOCUS_GAINED:
          GameInFocus = true;
          Focus_Restore();
          break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
          GameInFocus = false;
          Focus_Loss();
          break;
        default:
          break;
      }
      break;
    }
    case SDL_QUIT:
      Prog_End();
      VisiblePage.Un_Init();
      HiddenPage.Un_Init();

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
  SeenBuff.Blit(HidPage);
  const int oldyoff = 0;
  int newyoff = 0;
  while (shakes--) {
    const int x = static_cast<int>(TickCount.Time());

    do {
      newyoff = Sim_Random_Pick(0, 2) - 1;
    } while (newyoff == oldyoff);
    switch (newyoff) {
      case -1:
        HidPage.Blit(SeenBuff, 0, 2, 0, 0, 640, 398);
        break;
      case 0:
        HidPage.Blit(SeenBuff);
        break;
      case 1:
        HidPage.Blit(SeenBuff, 0, 0, 0, 2, 640, 398);
        break;
      default:
        break;
    }
    while (x == TickCount.Time()) {
      Video_End_Frame();
    }
  }

  HidPage.Blit(SeenBuff);
  Show_Mouse();
}
