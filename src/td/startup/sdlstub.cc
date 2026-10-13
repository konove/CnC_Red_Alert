// more portable replacements for winstub

#include <cstdio>
#include <cstdlib>
#include <span>
#include <utility>

#include "base/rect.h"
#include "embedded/kConquerIcon.h"
#include "engine/audio/audio_mixer.h"
#include "engine/gfx/ico_image.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/net/nullconn.h"
#include "engine/platform/timer.h"
#include "engine/window/display.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/window_events.h"
#include "game/ui/input.h"
#include "td/assets/screen.h"
#include "td/core/rand.h"
#include "td/startup/game_state.h"
#include "td/startup/startup.h"
#include "td/startup/winstub.h"

namespace td {

void CCDebugString(const char* /*string*/) {}

void Check_For_Focus_Loss() {
  if (!TheGameState().in_focus()) {
    engine::window::TheDisplay().PumpEvents();
    if (TheGameState().in_focus()) {
      engine::audio::TheAudio().SetExtraPaused(false);
    }
  }
}

#define WINDOW_NAME "Command & Conquer"

void Create_Main_Window(HANDLE /*instance*/, int /*command_show*/, int width,
                        int height) {
  engine::window::Display& display = engine::window::TheDisplay();
  display.Init(WINDOW_NAME, width, height);
  if (auto icon = engine::gfx::DecodeIco(
          std::as_bytes(std::span(embedded::kConquerIcon)))) {
    display.SetIcon(*std::move(icon));
  }
}

GameWindowEvents::GameWindowEvents(game::Input& input)
    : WindowEvents(input.keyboard(), input.mouse()) {}

void GameWindowEvents::FocusChanged(const bool in_focus) {
  TheGameState().in_focus() = in_focus;
  if (in_focus) {
    Focus_Restore();
  } else {
    Focus_Loss();
  }
}

void GameWindowEvents::Quit() {
  ShutDown();

  fflush(stdout);
  exit(0);
}

// SHAKESCR.ASM in WIN32LIB
// based on Shake_The_Screen in RA's conquer.cpp
void ShakeScreen(int shakes) {
  shakes += shakes;

  game::TheMouse().Hide();
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
        TheScreen().hidden_view().BlitTo(TheScreen().visible_view(),
                                         base::Rect{0, 2, 640, 398}, 0, 0);
        break;
      case 0:
        TheScreen().hidden_view().BlitTo(TheScreen().visible_view());
        break;
      case 1:
        TheScreen().hidden_view().BlitTo(TheScreen().visible_view(),
                                         base::Rect{0, 0, 640, 398}, 0, 2);
        break;
      default:
        break;
    }
    while (x == engine::platform::SystemTicks()) {
      engine::window::TheDisplay().EndFrame();
    }
  }

  TheScreen().hidden_view().BlitTo(TheScreen().visible_view());
  game::TheMouse().Show();
}

}  // namespace td
