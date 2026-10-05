#include "engine/window/scripted_run.h"

#include <limits>
#include <memory>
#include <string>
#include <string_view>

#include "engine/platform/manual_clock.h"
#include "engine/window/display.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "engine/window/player_script.h"
#include "engine/window/scripted_player.h"

namespace engine::window {

ScriptedRun::ScriptedRun(Display& display, KeyBuffer& keyboard,
                         HardwareCursor& mouse, const Pace pace) {
  if (pace == Pace::kFrames) {
    clock_ = std::make_unique<engine::platform::ManualClock>();
    player_ =
        std::make_unique<ScriptedPlayer>(display, keyboard, mouse, *clock_);
  } else {
    player_ = std::make_unique<ScriptedPlayer>(display, keyboard, mouse);
    // Such a run is ended by the game, not by its script, and how many
    // frames that takes is anybody's guess.
    player_->set_frame_budget(std::numeric_limits<int>::max() / 2);
  }
  player_->set_idle_pumps_per_frame(kIdlePumpsPerFrame);
}

std::string ScriptedRun::Load(const std::string_view text,
                              const PlayerScriptHooks& hooks) {
  return LoadPlayerScript(text, *player_, hooks);
}

}  // namespace engine::window
