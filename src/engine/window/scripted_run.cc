#include "engine/window/scripted_run.h"

#include <string>
#include <string_view>

#include "engine/window/display.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "engine/window/player_script.h"

namespace engine::window {

ScriptedRun::ScriptedRun(Display& display, KeyBuffer& keyboard,
                         HardwareCursor& mouse)
    : player_(display, keyboard, mouse, clock_) {
  player_.set_idle_pumps_per_frame(kIdlePumpsPerFrame);
}

std::string ScriptedRun::Load(const std::string_view text,
                              const PlayerScriptHooks& hooks) {
  return LoadPlayerScript(text, player_, hooks);
}

}  // namespace engine::window
