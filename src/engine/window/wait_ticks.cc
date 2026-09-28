#include "engine/window/wait_ticks.h"

#include "engine/platform/timer.h"
#include "engine/window/display.h"

namespace engine::window {

void WaitTicks(int ticks) {
  const auto target = platform::SystemTicks() + ticks;

  while (platform::SystemTicks() < target) {
    TheDisplay().EndFrame();
  }
}

}  // namespace engine::window
