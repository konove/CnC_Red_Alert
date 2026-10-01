// more portable replacements for winstub

#include <cstdlib>
#include <span>
#include <utility>

#include "embedded/kRedAlertIcon.h"
#include "engine/audio/audio_mixer.h"
#include "engine/gfx/ico_image.h"
#include "engine/net/net_select.h"
#include "engine/net/nullconn.h"
#include "engine/window/display.h"
#include "engine/window/window_events.h"
#include "game/ui/input.h"
#include "ra/core/config.h"
#include "ra/startup/game_state.h"
#include "ra/startup/startup.h"
#include "ra/startup/winstub.h"

namespace ra {

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
  engine::window::Display& display = engine::window::TheDisplay();
  display.Init(kWindowName, width, height);
  if (auto icon = engine::gfx::DecodeIco(
          std::as_bytes(std::span(embedded::kRedAlertIcon)))) {
    display.SetIcon(*std::move(icon));
  }
}

GameWindowEvents::GameWindowEvents(game::Input& input)
    : WindowEvents(input.keyboard(), input.mouse()) {}

void GameWindowEvents::Pump() { Socket_Select(); }

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
  exit(0);
}

}  // namespace ra
