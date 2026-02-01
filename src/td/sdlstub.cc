// more portable replacements for winstub

#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"
#include "td/globals.h"

#undef WIN32
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

bool ReadyToQuit = 0;

void Focus_Loss();
void Focus_Restore();

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
      }
      break;
    }
    case SDL_QUIT:
      Prog_End();
      VisiblePage.Un_Init();
      HiddenPage.Un_Init();

      fflush(stdout);
      exit(0);
  }
}

// Computes a fast checksum over arbitrary binary data.
//
// Algorithm: Accumulates 32-bit words using rotate-left-by-1 + add.
// Not a true CRC polynomial division, but provides similar error detection
// with better performance. Originally from CRC.ASM in WIN32LIB.
//
// The checksum is computed as:
//   For each 32-bit word: crc = rotl(crc, 1) + word
//   Remaining bytes are packed into a final word (big-endian order).
[[nodiscard]] constexpr uint32_t Calculate_CRC(
    const std::string_view str) noexcept {
  if (str.empty()) {
    return 0;
  }

  uint32_t crc = 0;
  size_t i = 0;

  // Process 32-bit aligned chunks.
  // We reconstruct the integer manually to avoid reinterpret_cast and strict
  // aliasing.
  while (i + 4 <= str.size()) {
    // Note: We cast to uint8_t first to prevent sign-extension (if char is
    // signed).
    const uint32_t word =
        static_cast<uint32_t>(static_cast<uint8_t>(str[i])) |
        static_cast<uint32_t>(static_cast<uint8_t>(str[i + 1])) << 8 |
        static_cast<uint32_t>(static_cast<uint8_t>(str[i + 2])) << 16 |
        static_cast<uint32_t>(static_cast<uint8_t>(str[i + 3])) << 24;

    crc = std::rotl(crc, 1) + word;
    i += 4;
  }

  // Handle remaining 1-3 bytes by packing them into a 32-bit word (Little
  // Endian).
  if (i < str.size()) {
    uint32_t tmp = 0;
    for (size_t j = 0; i + j < str.size(); ++j) {
      tmp |= static_cast<uint32_t>(static_cast<uint8_t>(str[i + j])) << (j * 8);
    }
    crc = std::rotl(crc, 1) + tmp;
  }

  return crc;
}

// SHAKESCR.ASM in WIN32LIB
// based on Shake_The_Screen in RA's conquer.cpp
void Shake_Screen(int shakes) {
  shakes += shakes;

  Hide_Mouse();
  SeenBuff.Blit(HidPage);
  int oldyoff = 0;
  int newyoff = 0;
  while (shakes--) {
    int x = TickCount.Time();

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
    }
#ifdef PORTABLE
    while (x == TickCount.Time()) {
      Video_End_Frame();
    }
#else
    while (x == TickCount);
#endif
  }

  HidPage.Blit(SeenBuff);
  Show_Mouse();
}
